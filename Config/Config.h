#ifndef __CONFIG_H
#define __CONFIG_H

/*
开发者:Bury
版本:1.0

工程配置文件
*/

#define ADMIN_PWD "84364323" //管理员密码
#define DEFULT_PWD "1234" //默认开门密码
#define PWD_MAX_LEN 32 //密码长度限制
#define PWD_MIN_LEN 4 //密码最短限制

#define OPEN_DOOR_ANGLE 75 //开门步进电机旋转角度
#define OPEN_DOOR_SPEED 490 //开门步进电机旋转速度
#define OPEN_DOOR_TIME 2 //开关门间隔时间
#define OPEN_DOOR_RETURN_MAIN_TIME 4 //开门后返回主界面时间

#define FAIL_LOCK_COUNT 3 //密码错误锁机次数
#define FAIL_LOCK_TIME 600 //密码错误锁机时间(秒)
 
#define FGP_TIMEOUT 20000 //指纹采集超时时间(毫秒)
#define FGP_ALLOW_FAINGER_OUT 1 //注册时，多次指纹采集过程中，是否要求手指离开才能进入下一次指纹图像 采集， 0-要求离开；1-不要求离开；
#define FGP_FINGER_RESGIST_LIGHT 1 //bit0：采图背光灯控制位，0-LED 长亮，1-LED 获取图像成功后灭； 
#define FGP_COLLECT_FINGER_NUMBER 6 //采集指纹的次数
#define FGP_MAX_VERIFY 3 //指纹验证失败锁机阈值

#define SLEEPING_TIME 10 //无操作自动休眠时间(秒)

#define MOTOR_REVERSE //电机反转
#define BODY_WAKEUP //人体感应模块唤醒

//高级配置
#define FLASH_EEPROM_BASE_ADDR 0x0800C000U; //密码在Flash中的地址
#define PASSWORD_FLASH_SECTOR FLASH_SECTOR_3 //密码在Flash的页
#endif