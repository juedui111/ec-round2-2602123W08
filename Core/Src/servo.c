#include "servo.h"
#include "tim.h"

typedef struct
{
  uint16_t us_min;      /* 该关节允许的最小脉宽 */
  uint16_t us_max;      /* 该关节允许的最大脉宽 */
  int16_t  soft_min;    /* 软限位下限，单位 0.1 度 */
  int16_t  soft_max;    /* 软限位上限，单位 0.1 度 */
  int8_t   dir;         /* 角度增大时脉宽增大为 +1，反之为 -1 */
} joint_cfg_t;

/* 上电默认值，等四个舵机到齐后逐个实测替换 */
static const joint_cfg_t JOINT[SERVO_CH_NUM] =
{
  { 500U, 2500U,    0, 1800, +1 },
  { 500U, 2500U,    0, 1800, +1 },
  { 500U, 2500U,    0, 1800, +1 },
  { 500U, 2500U,    0, 1800, +1 },
};

static uint16_t s_pulse[SERVO_CH_NUM] = { 1500U, 1500U, 1500U, 1500U };

/* 50Hz 下 1us 对应 2 个计数（72MHz/36 = 2MHz，1 tick = 0.5us） */
#define US_TO_CCR(us) ((uint32_t)(us) * 2U)

void servo_init(void)
{
  (void)HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
  (void)HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
  (void)HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
  (void)HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);

  for (uint8_t ch = 0U; ch < SERVO_CH_NUM; ch++)
  {
    servo_write_us(ch, 1500U);
  }
}

void servo_write_us(uint8_t ch, uint16_t us)
{
  if (ch >= SERVO_CH_NUM)
  {
    return;
  }

  const joint_cfg_t *cfg = &JOINT[ch];

  if (us < cfg->us_min) { us = cfg->us_min; }
  if (us > cfg->us_max) { us = cfg->us_max; }

  s_pulse[ch] = us;

  switch (ch)
  {
  case 0U: __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, US_TO_CCR(us)); break;
  case 1U: __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, US_TO_CCR(us)); break;
  case 2U: __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, US_TO_CCR(us)); break;
  case 3U: __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, US_TO_CCR(us)); break;
  default: break;
  }
}

void servo_write_deg(uint8_t ch, int16_t deg_x10)
{
  if (ch >= SERVO_CH_NUM)
  {
    return;
  }

  const joint_cfg_t *cfg = &JOINT[ch];

  if (deg_x10 < cfg->soft_min) { deg_x10 = cfg->soft_min; }
  if (deg_x10 > cfg->soft_max) { deg_x10 = cfg->soft_max; }

  int32_t angle = (cfg->dir > 0) ? deg_x10 : (1800 - deg_x10);
  int32_t span  = (int32_t)cfg->us_max - (int32_t)cfg->us_min;

  servo_write_us(ch, (uint16_t)((int32_t)cfg->us_min + (angle * span) / 1800));
}

uint16_t servo_get_us(uint8_t ch)
{
  return (ch < SERVO_CH_NUM) ? s_pulse[ch] : 0U;
}

uint16_t servo_get_us_min(uint8_t ch)
{
  return (ch < SERVO_CH_NUM) ? JOINT[ch].us_min : 0U;
}

uint16_t servo_get_us_max(uint8_t ch)
{
  return (ch < SERVO_CH_NUM) ? JOINT[ch].us_max : 0U;
}
