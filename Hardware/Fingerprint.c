#include "Fingerprint.h"
#include "app_util.h"

/*
 * 指纹模块（FPM10A 兼容协议）非阻塞驱动。
 *
 * 帧格式：
 *   EF 01 FF FF FF FF | PID | LEN_H LEN_L | DATA(LEN-2 字节) | SUM_H SUM_L
 *   校验和 = PID + LEN_H + LEN_L + 所有 DATA 字节
 *   LEN    = DATA 字节数 + 2（校验和占 2 字节）
 *   整帧长度 = 9 + LEN
 *
 * 旧版问题：
 *   - 每次收发都用 HAL_UART_Receive 阻塞等待固定长度，且不检查返回值；
 *   - 在 EXTI 中断里调用，SysTick 同优先级无法喂 tick，超时永远不生效 → 死等；
 *   - 用 is_from_1() 偏移一位的“猜包头”方式处理错位，容易误判；
 *   - 注册时一次阻塞最长 100 秒。
 * 新版：UART 接收中断逐字节喂给解析器，滑窗找包头、校验长度和校验和；
 *       解析出的完整帧放入队列，由主循环的 FPC_Poll() 处理并回调业务层。
 *       中断里只做内存搬运，绝不做 IO、延时或界面操作。
 */

#define FPC_HEADER_LEN       6U
#define FPC_RX_BUF_SIZE      256U
#define FPC_TX_MAX_DATA_LEN  40U
#define FPC_RX_MAX_DATA_LEN  160U
#define FPC_FRAME_QUEUE_SIZE 4U

extern UART_HandleTypeDef huart5;

static const uint8_t s_packet_header[FPC_HEADER_LEN] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct
{
    uint8_t pid;
    uint8_t data_len;
    uint8_t data[FPC_RX_MAX_DATA_LEN];
} FPC_Frame;

static FPC_EventHandler s_handler;
static FPC_Op           s_op;
static bool             s_enroll_waiting_id;
static uint32_t         s_deadline;
static uint32_t         s_ignore_until;
static uint32_t         s_rx_watchdog_tick;

static uint8_t  s_rx_byte;
static uint8_t  s_rx_buf[FPC_RX_BUF_SIZE];
static uint16_t s_rx_len;

/* 中断 → 主循环的完整帧队列 */
static FPC_Frame          s_frame_queue[FPC_FRAME_QUEUE_SIZE];
static volatile uint8_t   s_frame_head;
static volatile uint8_t   s_frame_tail;

static FPC_LightMode s_pending_light;
static bool          s_pending_light_valid;

/*
 * 发送灯光指令后，模块会回一个“确认包”。协议里的确认包不携带原指令号，
 * 如果紧接着发指纹验证，灯光 ACK 可能被误当成验证结果（status=0 → 误开门）。
 * 因此灯光发送后 150ms 内禁止发起新操作，让 ACK 在 FPC_POLL 里被安全丢弃。
 */
static uint32_t s_light_guard_until;

/* -------------------------------------------------------------------------- */
/* 内部工具                                                                    */
/* -------------------------------------------------------------------------- */
static bool fpc_send(uint8_t pid, const uint8_t *data, uint8_t data_len)
{
    uint8_t  frame[FPC_HEADER_LEN + 1U + 2U + FPC_TX_MAX_DATA_LEN + 2U];
    uint16_t index = 0U;
    uint16_t length;
    uint16_t checksum;
    HAL_StatusTypeDef status;

    if ((data == NULL) || (data_len > FPC_TX_MAX_DATA_LEN))
    {
        return false;
    }

    length = (uint16_t)data_len + 2U;

    for (uint16_t i = 0U; i < FPC_HEADER_LEN; i++)
    {
        frame[index++] = s_packet_header[i];
    }
    frame[index++] = pid;
    frame[index++] = (uint8_t)(length >> 8);
    frame[index++] = (uint8_t)(length & 0xFFU);

    checksum = (uint16_t)(pid + (length >> 8) + (length & 0xFFU));
    for (uint16_t i = 0U; i < data_len; i++)
    {
        frame[index++] = data[i];
        checksum = (uint16_t)(checksum + data[i]);
    }

    frame[index++] = (uint8_t)(checksum >> 8);
    frame[index++] = (uint8_t)(checksum & 0xFFU);

    status = HAL_UART_Transmit(&huart5, frame, index, 50U);
    return (status == HAL_OK);
}

static void fpc_finish(FPC_Op op, FPC_Result result, FGP_Status_Code status)
{
    s_op = FPC_OP_NONE;
    s_enroll_waiting_id = false;
    s_deadline = 0U;

    if (s_handler != NULL)
    {
        s_handler(op, result, status);
    }
}

static void fpc_send_light_now(FPC_LightMode mode)
{
    uint8_t data[6];
    uint8_t start_color = 0x00U;
    uint8_t end_color = 0x00U;
    uint8_t light_status;

    switch (mode)
    {
        case FPC_LIGHT_BLUE_BREATH:
            light_status = 0x01U; /* 呼吸灯 */
            start_color = 0x01U;  /* 蓝 */
            end_color = 0x01U;
            break;
        case FPC_LIGHT_RED_CONST:
            light_status = 0x03U; /* 常开 */
            start_color = 0x04U;  /* 红 */
            end_color = 0x04U;
            break;
        case FPC_LIGHT_GREEN_CONST:
            light_status = 0x03U;
            start_color = 0x02U;  /* 绿 */
            end_color = 0x02U;
            break;
        case FPC_LIGHT_OFF:
        default:
            light_status = 0x04U; /* 常闭 */
            break;
    }

    data[0] = 0x3CU; /* 指令：灯光控制 */
    data[1] = light_status;
    data[2] = start_color;
    data[3] = end_color;
    /* 呼吸灯需要时间参数；常开/常闭保持旧版行为填 0 */
    data[4] = (light_status == 0x01U) ? 0x02U : 0x00U;
    data[5] = 0x00U; /* 循环次数，0 = 无限 */

    if (fpc_send(0x01U, data, sizeof(data)))
    {
        s_light_guard_until = HAL_GetTick() + 150U;
    }
}

static void fpc_try_flush_light(void)
{
    if ((s_op == FPC_OP_NONE) && s_pending_light_valid)
    {
        s_pending_light_valid = false;
        fpc_send_light_now(s_pending_light);
    }
}

/* 从索引表数据中取出空闲模板 ID（只使用低 32 个 ID，与原逻辑一致） */
static bool fpc_find_free_id(const uint8_t *data, uint8_t data_len, uint8_t *id)
{
    const uint8_t *bitmap;
    uint8_t bitmap_len;
    uint32_t mask = 0U;

    if ((data == NULL) || (id == NULL) || (data_len < 5U))
    {
        return false;
    }

    /*
     * 模块返回的索引表数据格式为：确认码(0x00) + 索引表。
     * 旧代码也是跳过第 1 个字节，从 data[1] 开始取 4 字节位图。
     */
    bitmap = &data[1];
    bitmap_len = (uint8_t)(data_len - 1U);

    for (uint8_t i = 0U; (i < bitmap_len) && (i < 4U); i++)
    {
        mask |= ((uint32_t)bitmap[i]) << (8U * i);
    }

    for (uint8_t i = 0U; i < 32U; i++)
    {
        if ((mask & (1UL << i)) == 0U)
        {
            *id = i;
            return true;
        }
    }

    return false;
}

static bool fpc_start_enroll_with_id(uint8_t id)
{
    uint8_t param = (uint8_t)(0x1CU |
                              ((uint8_t)FGP_ALLOW_FAINGER_OUT << 5) |
                              (uint8_t)FGP_FINGER_RESGIST_LIGHT);
    uint8_t data[6];

    data[0] = 0x31U; /* PS_AutoEnroll */
    data[1] = 0x00U;
    data[2] = id;
    data[3] = (uint8_t)FGP_COLLECT_FINGER_NUMBER;
    data[4] = 0x00U;
    data[5] = param;

    if (!fpc_send(0x01U, data, sizeof(data)))
    {
        return false;
    }

    s_enroll_waiting_id = false;
    s_op = FPC_OP_ENROLL;
    s_deadline = HAL_GetTick() + FGP_ENROLL_TIMEOUT_MS;
    return true;
}

/* -------------------------------------------------------------------------- */
/* 帧处理（主循环上下文）                                                       */
/* -------------------------------------------------------------------------- */
static void fpc_handle_frame(uint8_t pid, const uint8_t *data, uint8_t data_len)
{
    FGP_Status_Code status;

    if (!App_TimeReached(s_ignore_until))
    {
        return;
    }
    if ((pid != 0x07U) || (data_len < 1U))
    {
        return; /* 只处理确认包 */
    }

    status = (FGP_Status_Code)data[0];

    switch (s_op)
    {
        case FPC_OP_VERIFY:
            if (status == FGP_STATUS_OK)
            {
                fpc_finish(FPC_OP_VERIFY, FPC_RESULT_OK, status);
            }
            else if (status == FGP_STATUS_TIMEOUT)
            {
                fpc_finish(FPC_OP_VERIFY, FPC_RESULT_TIMEOUT, status);
            }
            else
            {
                fpc_finish(FPC_OP_VERIFY, FPC_RESULT_FAIL, status);
            }
            break;

        case FPC_OP_ENROLL:
            if (s_enroll_waiting_id)
            {
                uint8_t id;

                if (status != FGP_STATUS_OK)
                {
                    fpc_finish(FPC_OP_ENROLL, FPC_RESULT_FAIL, status);
                }
                else if (data_len < 5U)
                {
                    /*
                     * 有些模块会先回一个“命令已收到”的短 ACK，再回索引表。
                     * 这里不把它当成失败，继续等索引表，直到超时。
                     */
                }
                else if (!fpc_find_free_id(data, data_len, &id))
                {
                    /* 索引表异常或已满 */
                    fpc_finish(FPC_OP_ENROLL, FPC_RESULT_FAIL,
                               (data_len < 5U) ? FGP_STATUS_PACKET_ERROR
                                               : FGP_STATUS_LIBRARY_FULL);
                }
                else if (!fpc_start_enroll_with_id(id))
                {
                    fpc_finish(FPC_OP_ENROLL, FPC_RESULT_COMM_ERROR,
                               FGP_STATUS_PORT_OPERATION_FAIL);
                }
                else
                {
                    /* 已发出录入命令，继续等待录入结果 */
                }
            }
            else if (status == FGP_STATUS_OK)
            {
                fpc_finish(FPC_OP_ENROLL, FPC_RESULT_OK, status);
            }
            else if (status == FGP_STATUS_TIMEOUT)
            {
                fpc_finish(FPC_OP_ENROLL, FPC_RESULT_TIMEOUT, status);
            }
            else
            {
                fpc_finish(FPC_OP_ENROLL, FPC_RESULT_FAIL, status);
            }
            break;

        case FPC_OP_CLEAR:
            if (status == FGP_STATUS_OK)
            {
                fpc_finish(FPC_OP_CLEAR, FPC_RESULT_OK, status);
            }
            else
            {
                fpc_finish(FPC_OP_CLEAR, FPC_RESULT_FAIL, status);
            }
            break;

        case FPC_OP_NONE:
        default:
            break;
    }
}

/* 中断上下文：把完整帧放入队列 */
static void fpc_push_frame(uint8_t pid, const uint8_t *data, uint8_t data_len)
{
    uint8_t head;
    uint8_t next;

    if (data_len > FPC_RX_MAX_DATA_LEN)
    {
        return;
    }

    uint32_t primask = App_EnterCritical();
    head = s_frame_head;
    next = (uint8_t)((head + 1U) % FPC_FRAME_QUEUE_SIZE);
    if (next != s_frame_tail)
    {
        s_frame_queue[head].pid = pid;
        s_frame_queue[head].data_len = data_len;
        for (uint8_t i = 0U; i < data_len; i++)
        {
            s_frame_queue[head].data[i] = data[i];
        }
        s_frame_head = next;
    }
    App_ExitCritical(primask);
}

static bool fpc_pop_frame(FPC_Frame *frame)
{
    bool popped = false;

    uint32_t primask = App_EnterCritical();
    if (s_frame_tail != s_frame_head)
    {
        *frame = s_frame_queue[s_frame_tail];
        s_frame_tail = (uint8_t)((s_frame_tail + 1U) % FPC_FRAME_QUEUE_SIZE);
        popped = true;
    }
    App_ExitCritical(primask);

    return popped;
}

/* 中断上下文：逐字节解析 */
static void fpc_feed_byte(uint8_t byte)
{
    if (s_rx_len < FPC_RX_BUF_SIZE)
    {
        s_rx_buf[s_rx_len++] = byte;
    }
    else
    {
        (void)memmove(&s_rx_buf[0], &s_rx_buf[1], FPC_RX_BUF_SIZE - 1U);
        s_rx_buf[FPC_RX_BUF_SIZE - 1U] = byte;
    }

    for (;;)
    {
        uint16_t length;
        uint16_t total;
        uint16_t checksum = 0U;
        uint16_t received_crc;
        uint16_t i;

        /* 1. 找到包头 */
        while ((s_rx_len > 0U) && (s_rx_buf[0] != s_packet_header[0]))
        {
            (void)memmove(&s_rx_buf[0], &s_rx_buf[1], s_rx_len - 1U);
            s_rx_len--;
        }

        if (s_rx_len < (FPC_HEADER_LEN + 3U))
        {
            return;
        }

        /* 2. 校验包头 */
        for (i = 1U; i < FPC_HEADER_LEN; i++)
        {
            if (s_rx_buf[i] != s_packet_header[i])
            {
                break;
            }
        }
        if (i != FPC_HEADER_LEN)
        {
            (void)memmove(&s_rx_buf[0], &s_rx_buf[1], s_rx_len - 1U);
            s_rx_len--;
            continue;
        }

        /* 3. 取长度字段（PID 后 2 字节） */
        length = (uint16_t)(((uint16_t)s_rx_buf[7] << 8) | s_rx_buf[8]);
        if ((length < 3U) || (length > (FPC_RX_BUF_SIZE - 9U)))
        {
            (void)memmove(&s_rx_buf[0], &s_rx_buf[1], s_rx_len - 1U);
            s_rx_len--;
            continue;
        }

        total = (uint16_t)(9U + length);
        if (s_rx_len < total)
        {
            return; /* 等后续字节 */
        }

        for (i = FPC_HEADER_LEN; i < (total - 2U); i++)
        {
            checksum = (uint16_t)(checksum + s_rx_buf[i]);
        }
        received_crc = (uint16_t)(((uint16_t)s_rx_buf[total - 2U] << 8) |
                                  s_rx_buf[total - 1U]);

        if (checksum == received_crc)
        {
            uint8_t pid = s_rx_buf[FPC_HEADER_LEN];
            uint8_t data_len = (uint8_t)(length - 2U);
            const uint8_t *data = &s_rx_buf[FPC_HEADER_LEN + 3U];

            fpc_push_frame(pid, data, data_len);

            (void)memmove(&s_rx_buf[0], &s_rx_buf[total], s_rx_len - total);
            s_rx_len = (uint16_t)(s_rx_len - total);
        }
        else
        {
            (void)memmove(&s_rx_buf[0], &s_rx_buf[1], s_rx_len - 1U);
            s_rx_len--;
        }
    }
}

/* -------------------------------------------------------------------------- */
/* 对外接口                                                                    */
/* -------------------------------------------------------------------------- */
void FPC_Init(FPC_EventHandler handler)
{
    s_handler = handler;
    s_op = FPC_OP_NONE;
    s_enroll_waiting_id = false;
    s_deadline = 0U;
    s_ignore_until = 0U;
    s_rx_len = 0U;
    s_frame_head = 0U;
    s_frame_tail = 0U;
    s_pending_light_valid = false;
    s_light_guard_until = 0U;
    s_rx_watchdog_tick = HAL_GetTick();

    HAL_NVIC_SetPriority(UART5_IRQn, 2U, 0U);
    HAL_NVIC_EnableIRQ(UART5_IRQn);

    (void)HAL_UART_Receive_IT(&huart5, &s_rx_byte, 1U);
}

bool FPC_IsBusy(void)
{
    return (s_op != FPC_OP_NONE);
}

bool FPC_StartVerify(void)
{
    static const uint8_t data[6] = {0x32U, 0x50U, 0xFFU, 0xFFU, 0x00U, 0x00U};

    if ((s_op != FPC_OP_NONE) || !App_TimeReached(s_light_guard_until))
    {
        return false;
    }
    if (!fpc_send(0x01U, data, sizeof(data)))
    {
        return false;
    }

    s_op = FPC_OP_VERIFY;
    s_deadline = HAL_GetTick() + FGP_VERIFY_TIMEOUT_MS;
    return true;
}

bool FPC_StartEnroll(void)
{
    static const uint8_t query[2] = {0x1FU, 0x00U};

    if ((s_op != FPC_OP_NONE) || !App_TimeReached(s_light_guard_until))
    {
        return false;
    }
    if (!fpc_send(0x01U, query, sizeof(query)))
    {
        return false;
    }

    s_op = FPC_OP_ENROLL;
    s_enroll_waiting_id = true;
    s_deadline = HAL_GetTick() + 2000U; /* 索引表应答超时 */
    return true;
}

bool FPC_StartClear(void)
{
    static const uint8_t data[1] = {0x0DU};

    if ((s_op != FPC_OP_NONE) || !App_TimeReached(s_light_guard_until))
    {
        return false;
    }
    if (!fpc_send(0x01U, data, sizeof(data)))
    {
        return false;
    }

    s_op = FPC_OP_CLEAR;
    s_deadline = HAL_GetTick() + FGP_CLEAR_TIMEOUT_MS;
    return true;
}

void FPC_Cancel(void)
{
    uint32_t primask = App_EnterCritical();

    s_op = FPC_OP_NONE;
    s_enroll_waiting_id = false;
    s_deadline = 0U;
    s_ignore_until = HAL_GetTick() + 1000U;
    s_rx_len = 0U;

    /* 丢弃已经排队的应答帧 */
    s_frame_tail = s_frame_head;

    App_ExitCritical(primask);
}

void FPC_SetLight(FPC_LightMode mode)
{
    if (s_op != FPC_OP_NONE)
    {
        s_pending_light = mode;
        s_pending_light_valid = true;
        return;
    }

    /* 立即发送时丢弃更早的延迟请求，避免连续两条灯光指令互相覆盖 */
    s_pending_light_valid = false;
    fpc_send_light_now(mode);
}

void FPC_Poll(void)
{
    FPC_Frame frame;

    /* 1. 处理中断收到的完整帧（最多一轮，避免长时间占用主循环） */
    while (fpc_pop_frame(&frame))
    {
        fpc_handle_frame(frame.pid, frame.data, frame.data_len);
    }

    /* 2. 超时处理 */
    if ((s_op != FPC_OP_NONE) && App_TimeReached(s_deadline))
    {
        FPC_Op op = s_op;
        fpc_finish(op, FPC_RESULT_TIMEOUT, FGP_STATUS_TIMEOUT);
    }

    /* 3. 模块空闲时补发延迟的灯光指令 */
    fpc_try_flush_light();

    /* 4. 接收链路自愈：若接收被异常终止则重新挂起 */
    if ((HAL_GetTick() - s_rx_watchdog_tick) >= 500U)
    {
        s_rx_watchdog_tick = HAL_GetTick();
        if (huart5.RxState == HAL_UART_STATE_READY)
        {
            (void)HAL_UART_Receive_IT(&huart5, &s_rx_byte, 1U);
        }
    }
}

/* -------------------------------------------------------------------------- */
/* UART 中断回调（被 HAL 调用，必须保持短小）                                    */
/* -------------------------------------------------------------------------- */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == UART5)
    {
        fpc_feed_byte(s_rx_byte);
        (void)HAL_UART_Receive_IT(&huart5, &s_rx_byte, 1U);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == UART5)
    {
        __HAL_UART_CLEAR_PEFLAG(huart);
        __HAL_UART_CLEAR_FEFLAG(huart);
        __HAL_UART_CLEAR_NEFLAG(huart);
        __HAL_UART_CLEAR_OREFLAG(huart);

        huart->ErrorCode = HAL_UART_ERROR_NONE;
        s_rx_len = 0U;
        (void)HAL_UART_Receive_IT(&huart5, &s_rx_byte, 1U);
    }
}
