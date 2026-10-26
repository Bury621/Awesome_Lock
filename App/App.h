#ifndef __APP_H
#define __APP_H

#include "main.h"

/* 应用初始化（外设已由 CubeMX 初始化完成） */
void App_Init(void);

/* 应用主循环体：处理事件、轮询非阻塞任务、喂狗 */
void App_Running(void);

#endif /* __APP_H */
