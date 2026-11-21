#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>

#define SERVO_CH_NUM 4U

/* 初始化四路 PWM 输出，全部回到中位 1500us */
void servo_init(void);

/* 按脉宽输出，内部按通道配置钳位，单位 us */
void servo_write_us(uint8_t ch, uint16_t us);

/* 按角度输出，单位 0.1 度，0..1800 对应 0..180 度 */
void servo_write_deg(uint8_t ch, int16_t deg_x10);

/* 查询当前脉宽与软限位范围，标定和上报时用 */
uint16_t servo_get_us(uint8_t ch);
uint16_t servo_get_us_min(uint8_t ch);
uint16_t servo_get_us_max(uint8_t ch);

#endif /* SERVO_H */
