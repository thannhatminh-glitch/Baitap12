#include "stm32f10x.h"                  // Device header
#include "FreeRTOS.h"                   // RTOS:Core&&Cortex-M
#include "task.h"                       // RTOS:Core&&Cortex-M
#include "queue.h"                      // RTOS:Core&&Cortex-M


// dinh nghia cau truc de chua cac tham so cho led 
typedef struct
{
		uint32_t frequency; //hz
		uint32_t duty; // do rong xung (0-100%)	
} Led_config_t;

QueueSetHandle_t xQueue_led_config;
	
void GPIO_config(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef gpio;
	gpio.GPIO_Pin = GPIO_Pin_12;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &gpio);
}	

void TASK_LED_CONTROL(void *pvParameters);
void TASK_UPDATE_PARAMETERS(void *pvParameters);

int main(void )
{
	GPIO_config();
	
	// tao queue de chua 1 phan tu kieu Led_config_t
	xQueue_led_config = xQueueCreate(1, sizeof(Led_config_t));
	if (xQueue_led_config != NULL)
  {
    xTaskCreate(TASK_LED_CONTROL, "LedControl", 128, NULL, 1, NULL); //tao task dieu kien led uu tien 1
    xTaskCreate(TASK_UPDATE_PARAMETERS, "UpdateParams", 128, NULL, 2, NULL); //tao task cap nhat tham so uu tien 2 (cao hon) 
    vTaskStartScheduler(); // bat dau lap lich trinh
	}
	while(1);
}

void TASK_LED_CONTROL( void *pvParameters)
{
	(void)pvParameters;
	Led_config_t led_cfg = {2, 50}; //tham so mac dinh 2hz , 50% duty cycle
	TickType_t T_period, T_on, T_off;
	BaseType_t xStatus;
	
	while(1)
	{
		if(led_cfg.frequency == 0) led_cfg.frequency = 1 ; // tranh chia 0
		if(led_cfg.duty > 100) led_cfg.duty = 100;
		T_period = configTICK_RATE_HZ / led_cfg.frequency;
		
		T_on = T_period* led_cfg.duty /100;
		T_off = T_period - T_on;
		
		if(T_on > 0) // bat led (muc 0) trong khi cho queue
		{
			GPIO_ResetBits(GPIOB, GPIO_Pin_12);
			xStatus = xQueueReceive(xQueue_led_config, &led_cfg, T_on);
			if(xStatus == pdPASS) continue; // Nhan duoc tham so moi, bat dau lai chu ky ngay lap tuc
		}
		
		if(T_off > 0) // tat led (muc 1) trong khi cho queue
		{
			GPIO_SetBits(GPIOB, GPIO_Pin_12);
			xStatus = xQueueReceive(xQueue_led_config, &led_cfg, T_off);
			if(xStatus == pdPASS) continue; // Nhan duoc tham so moi, bat dau lai chu ky ngay lap tuc
		}
	}
}	

void TASK_UPDATE_PARAMETERS (void *pvParameters)
{
	(void)pvParameters;
	// mang 2 chieu khai bao tan so(hz) va do rong xung(%)
	const uint32_t parameters[][2] =
	{
		 {2, 10},  // 2Hz, 10%
     {4, 30},  // 4Hz, 30%
     {6, 50},  // 6Hz, 50%
     {8, 70},  // 8Hz, 70%
		 {10, 90} // 10Hz, 90%
	};
	const uint8_t num_param_sets = sizeof(parameters) / sizeof(parameters[0]);
	uint8_t index = 0;
	Led_config_t params_to_send;
	while(1)
	{
		//chuan bi gia tri de gui
		params_to_send.frequency = parameters[index][0];
		params_to_send.duty = parameters[index][1];
		
		//gui cau truc tham so vao queue, ghi de neu day
		xQueueOverwrite(xQueue_led_config, &params_to_send);
		index++;
		if(index >= num_param_sets)
		{
			index = 0;	//quay lai tu dau
		}
		
		vTaskDelay(pdMS_TO_TICKS(5000)); // 5s
	}
	 
}
	

