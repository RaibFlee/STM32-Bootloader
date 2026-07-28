#ifndef LED_H
#define LED_H

#include "main.h"
/* 定义控制IO的宏 */

//红灯
#define LEDR_TOGGLE		HAL_GPIO_TogglePin(LED_R_GPIO_Port,LED_R_Pin)
#define LEDR_OFF		HAL_GPIO_WritePin(LED_R_GPIO_Port,LED_R_Pin,GPIO_PIN_SET)
#define LEDR_ON				HAL_GPIO_WritePin(LED_R_GPIO_Port,LED_R_Pin,GPIO_PIN_RESET)

//绿灯
#define LEDG_TOGGLE		HAL_GPIO_TogglePin(LED_G_GPIO_Port,LED_G_Pin)
#define LEDG_OFF		HAL_GPIO_WritePin(LED_G_GPIO_Port,LED_G_Pin,GPIO_PIN_SET)
#define LEDG_ON				HAL_GPIO_WritePin(LED_G_GPIO_Port,LED_G_Pin,GPIO_PIN_RESET)

//蓝灯
#define LEDB_TOGGLE		HAL_GPIO_TogglePin(LED_B_GPIO_Port,LED_B_Pin)
#define LEDB_OFF		HAL_GPIO_WritePin(LED_B_GPIO_Port,LED_B_Pin,GPIO_PIN_SET)
#define LEDB_ON				HAL_GPIO_WritePin(LED_B_GPIO_Port,LED_B_Pin,GPIO_PIN_RESET)


#endif /* LED_H */
