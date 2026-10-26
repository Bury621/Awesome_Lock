# Awesome Lock

基于 **STM32F407VET6** 的宿舍门禁锁固件：密码 / 指纹开锁、1.8" TFT 显示、4×4 矩阵键盘、28BYJ-48 步进电机驱动门锁，支持管理员菜单、指纹录入/清空、防暴力破解锁定和自动熄屏。

> 当前版本：**v2.1（稳定版）**
>
> 这一版重点修复了重构后实机出现的“开门/关门过程中卡死、自动复位”问题：最终将电机驱动恢复为重构前的实现，并保留显示、休眠、指纹、掉电存储等重构改进。

## 功能

- 密码开锁，密码保存在 Flash（Sector 5），带 CRC32 校验
- 指纹验证 / 录入 / 清空，指纹匹配采用严格确认包判定
- 管理员菜单：`Reset PWD`、`Register FP`、`Clear all FP`
- 密码连续错误 3 次进入锁机倒计时（默认 600 秒）
- 开门后自动关门、自动返回主界面
- 5 秒无操作自动熄屏；按键 / 指纹触摸 / 人体感应唤醒
- 锁机倒计时只刷新数字脏区，不整屏重绘
- 独立看门狗，主循环异常可自动复位

## 硬件

| 模块 | 型号 / 接口 |
| --- | --- |
| MCU | STM32F407VET6，168 MHz |
| 显示 | ST7735S 128×160 TFT，硬件 SPI2 |
| 键盘 | 4×4 矩阵键盘，I2C（地址 `0x65`）+ 中断 |
| 指纹 | FPM10A 类模块，UART5，57600 8N1 |
| 电机 | 28BYJ-48 + ULN2003，TIM6 步进 |
| 存储 | 内部 Flash Sector 5（`0x08020000`） |
| 调试串口 | USART1，9600 8N1（默认关闭输出） |

## 快速开始

### Keil MDK

1. 用 Keil MDK（ARM Compiler 6）打开 `MDK-ARM/Awesome_Lock.uvprojx`；
2. 编译并下载到 `0x08000000`；
3. 首次启动密码为 `1234`，管理员密码见 `Config/Config.h` 的 `ADMIN_PWD`。

### EIDE / VS Code

用 EIDE 打开 `MDK-ARM/Awesome_Lock.code-workspace`，选择 ARM Compiler 6 后编译下载。

> 仓库不跟踪 `.hex` / `.bin`（见 `.gitignore`），请本地编译生成。
>
> 如果从旧固件升级：密码存储已从 Sector 3 移到 Sector 5，旧密码不会沿用；首次启动请用默认密码 `1234`，再进管理员菜单修改。

## 关键配置

所有可调参数集中在 [`Config/Config.h`](Config/Config.h)：

| 参数 | 默认值 | 说明 |
| --- | --- | --- |
| `OPEN_DOOR_ANGLE` | `75` | 开门旋转角度（度） |
| `OPEN_DOOR_STEP_US` | `490` | 步进间隔；配合 TIM6 500 kHz 时为 980 µs |
| `OPEN_DOOR_TIME_MS` | `2500` | 开门后自动关门时间 |
| `OPEN_DOOR_HOLD_MS` | `4000` | 开门后返回主界面时间 |
| `SLEEPING_TIME` | `5` | 无操作熄屏时间（秒） |
| `FAIL_LOCK_COUNT` | `3` | 密码连续错误锁定次数 |
| `FAIL_LOCK_TIME` | `600` | 锁机时间（秒） |
| `FPC_VERIFY_CONFIRM_BYTE` | `0x05` | 指纹匹配确认字节 |

### 电机时序（重要）

当前 TIM6 配置为：

```c
htim6.Init.Prescaler = 167;   /* 84 MHz / 168 = 500 kHz，2 µs/计数 */
htim6.Init.Period    = 199;
```

`OPEN_DOOR_STEP_US = 490`，实际步进间隔为 `490 × 2 µs = 980 µs`。

> 如果以后把 TIM6 改回 1 MHz（`Prescaler=83, Period=499`），必须把 `OPEN_DOOR_STEP_US` 同步改成 `980`，否则电机速度会不对。

## 设计要点

1. **中断只投递事件**：EXTI / UART 接收中断不读 I2C、不等 UART、不刷屏、不延时。
2. **主循环状态机**：按键、密码、管理员菜单、指纹、开关门、锁定倒计时、休眠均为非阻塞轮询。
3. **中断优先级**：`TIM6=0`、`SysTick=1`、`UART5=2`、`EXTI=3`；所有 ISR 内都不调用带 tick 超时的阻塞 HAL 函数。
4. **看门狗**：主循环喂狗，异常时约 10 秒复位。
5. **显示**：硬件 SPI2，逐点直写 + 批量矩形/清屏；锁机倒计时只刷新数字区域。
6. **密码存储**：Sector 5 + magic/version/length/CRC32，空白或损坏数据自动回退默认密码。
7. **电机驱动**：使用重构前验证稳定的 8 拍半步驱动；不在电机启动/停止路径里加临界区或动态关中断。

## 目录结构

```text
App/        应用初始化、主循环、事件队列、调试串口
Service/    门锁状态机（lock_app）、显示层（ui_views）
Hardware/   LCD、键盘、指纹、步进电机、Flash 密码存储
Config/     所有可调参数
Core/       CubeMX 生成的时钟、GPIO、中断、HAL 配置
Drivers/    STM32F4 HAL 与 CMSIS
docs/       重构说明、测试清单
MDK-ARM/    Keil 工程与启动文件
```

## 调试串口

调试输出默认关闭（现场不接 USART1 时避免日志阻塞主循环）：

```c
/* App/Debug.c */
#define DEBUG_UART_ENABLE 0
```

需要抓日志时改为 `1` 重新编译，USART1 9600 8N1 输出启动信息、指纹结果、电机状态等。

## 文档

- [重构说明与问题分析](docs/重构说明.md)
- [硬件联调测试清单](docs/测试清单.md)

## License

本项目采用 **MIT License**，详见 [LICENSE](LICENSE)。

```text
MIT License

Copyright (c) 2026 Bury

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
