#include "main.h"
#include "uart_handler.h"



extern "C"
void app_entry(){
//	/*Kameranin acilmasi icin beklenen sure...*/
//	static volatile uint32_t boot_tick = HAL_GetTick();
//	while(!(HAL_GetTick() - boot_tick > 5000)) { ///Default 60K ms eski kamera,, yeni kamera default  5000 ms baslangıç süresi
//		static volatile uint32_t led_tick = 0;
//		if(HAL_GetTick() - led_tick > 1000) {
//			HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0);
//			led_tick = HAL_GetTick();
//		}
//	}
	UART_Handler_Init();
	for(;;)
	{

	}

}
/*Low-level uart implementasyonlari.*/
extern "C"
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	UART_Handler_RxCplt(huart);
}

extern "C"
void tick_callback(){

	//TODO: sistem saatinden tick alınıp, tick besleme yerlerinde kullanılabilir.
}




