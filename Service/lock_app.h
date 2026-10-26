#ifndef __LOCK_APP_H
#define __LOCK_APP_H

#include "main.h"
#include "app_events.h"

/*
 * 门锁业务状态机。
 * 所有输入（按键、指纹、人体感应）以事件形式进入，
 * 所有耗时操作（指纹通信、开门/关门、锁定倒计时、休眠）都是非阻塞的。
 */
void LockApp_Init(void);
void LockApp_HandleEvent(const AppEvent *event);
void LockApp_Poll(void);

#endif /* __LOCK_APP_H */
