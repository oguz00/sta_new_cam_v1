/**
 * @file uart_handler.h
 * @brief UART iletisimi: kontrol <-> kameraya arayuzu
 */
#ifndef UART_HANDLER_H_
#define UART_HANDLER_H_


#include <stdint.h>
#include <stdbool.h>



/* HAL UART handle tipi forward-declaration (STM32 HAL) */
typedef struct __UART_HandleTypeDef UART_HandleTypeDef;

/* Varsayilan extern UART handle'lari - gerekirse projeye gore duzenleyin */
extern UART_HandleTypeDef huart1; /* camera UART (degistirilebilir) */
extern UART_HandleTypeDef huart2; /* control UART (degistirilebilir) */



#define vehicle_uart huart2
#define cam_uart huart1
/* Konfigurasyon sabitleri */
#define CONTROL_RX_BUFFER_SIZE  32U
#define CAMERA_RX_BUFFER_SIZE   48U



/* Baslatma: rx interrupt/IT alimini baslatir */
void UART_Handler_Init(void);


/* Bu fonksiyon cubeMX tarafindan cagirilacak HAL callback icerisinde cagrilacak */
void UART_Handler_RxCplt(UART_HandleTypeDef *huart);

/* Gonderme yardimcilari - bloklayici transmit (kisa paketler icin uygun) */
bool UART_SendToCamera(const uint8_t *data, uint16_t len);
bool UART_SendToControl(const uint8_t *data, uint16_t len);

/* Yardim: icte kullanilan packet islemleri icin cagirilir */
void UART_HandleControlPacket(const uint8_t *pkt, uint16_t len);
void UART_HandleCameraPacket(const uint8_t *pkt, uint16_t len);
















#endif /* UART_HANDLER_H_ */
