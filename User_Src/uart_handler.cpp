/**
 * @file uart_handler.c
 * @brief UART receive state machines and forwarding between control and camera
 *
 * - HAL tarafindan tek-bayt RTC kullanilarak DMA/IT ile calisacak sekilde yazildi.
 * - main.c icindeki HAL_UART_RxCpltCallback fonksiyonundan
 *   UART_Handler_RxCplt(huart) cagrilmasi gerekir (aciklama asagida).
 *
 * Not: Bu implementasyon basit, güvenilir. Performans icin DMA/IDLE vs RX FIFO
 * secenekleri daha verimli olabilir.
 */

#include "uart_handler.h"
#include "command_handler.h"
#include "command_tracking.h"
#include "packet_builder.h"
#include "main.h"   /* huart1/huart2 extern tanimi ve HAL_GetTick */
#include <string.h>
#include <stdbool.h>


/* Local receive byte holders (IT ile tek byte olarak alinir) */
static uint8_t ctrl_rx_byte;
static uint8_t cam_rx_byte;
/* Control receive buffer ve indeksler */
static uint8_t control_rx_buf[CONTROL_RX_BUFFER_SIZE];
static uint16_t control_rx_len;

/* Camera receive buffer ve indeksler */
static uint8_t camera_rx_buf[CAMERA_RX_BUFFER_SIZE];
static uint16_t camera_rx_len;

/* Forward declarations for local helpers */
static void control_rx_put_byte(uint8_t b);
static void camera_rx_put_byte(uint8_t b);
static void reset_control_buffer(void);
static void reset_camera_buffer(void);


void UART_Handler_Init(void)
{
    /* Bufferleri sifirla */
    reset_control_buffer();
    reset_camera_buffer();

    /* Baslangicta UART IT ile 1 byte alma islemlerini baslat.
       Burada huart2 -> control, huart1 -> camera (projeye gore degistirin). */

    /* I/O: baslat ISR/IT alimi; HAL_UART_Receive_IT geri cagirdiğinde
       HAL_UART_RxCpltCallback icinde UART_Handler_RxCplt cagirilacak. */
    HAL_UART_Receive_IT(&huart2, &ctrl_rx_byte, 1U);
    HAL_UART_Receive_IT(&huart1, &cam_rx_byte, 1U);
}

/* Bu fonksiyonu HAL_UART_RxCpltCallback icinden cagirin.
   Ornek ekleme:
   void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
   {
       UART_Handler_RxCplt(huart);
   }
*/
void UART_Handler_RxCplt(UART_HandleTypeDef *huart)
{
    if (huart == &vehicle_uart)
    {
        control_rx_put_byte(ctrl_rx_byte);
        HAL_UART_Receive_IT(&huart2, &ctrl_rx_byte, 1U);
    }
    else if (huart == &cam_uart)
    {
        camera_rx_put_byte(cam_rx_byte);
        HAL_UART_Receive_IT(&huart1, &cam_rx_byte, 1U);
    }
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart){
	if(huart == &cam_uart){
		HAL_UART_Receive_IT(&huart1, &cam_rx_byte, 1U);
	}else if(huart == &vehicle_uart){
		HAL_UART_Receive_IT(&huart2, &ctrl_rx_byte, 1U);
	}
}
/* Gonderme: kameraya veriyi yazar */
bool UART_SendToCamera(const uint8_t *data, uint16_t len)
{
    if ((data == NULL) || (len == 0U)) {
        return false;
    }

    /* Non-blocking gonderim istenirse HAL_UART_Transmit_IT kullanin.
       Burada bloklayici kullanimi kolaylik icin tercih edildi. */
    if (HAL_UART_Transmit(&huart1, (uint8_t *)data, (uint16_t)len, 500U) == HAL_OK) {
        return true;
    }

    return false;
}

/* Gonderme: kontrole veriyi yazar */
bool UART_SendToControl(const uint8_t *data, uint16_t len)
{
    if ((data == NULL) || (len == 0U)) {
        return false;
    }

    if (HAL_UART_Transmit(&huart2, (uint8_t *)data, (uint16_t)len, 500U) == HAL_OK) {
        return true;
    }

    return false;
}

/* Bu fonksiyonlar, tam bir paket tespit edildiginde cagirilir. */
/* Paket doğrulama + çeviri + gönderme burada yapılıyor. */

void UART_HandleControlPacket(const uint8_t *pkt, uint16_t len)
{
    uint8_t cam_pkt[64]={0};
    uint8_t cam_len = 0U;
    TranslationResult_t tr;

    /* Paket dogrula */
    if (!VerifyCtrlPacket(pkt, (uint8_t)len)) {
        /* gecerli degilse atla (istege gore hata logu ekle) */
        return;
    }
    /* Cevir kontrol->kamera */
    tr = CommandHandler_TranslateCtrlToCam(pkt, (uint8_t)len, cam_pkt, &cam_len);
    if (tr != TRANSLATION_OK) {
    	//TODO: Bu noktada hata log çıktısı verilebilir.
        /* hatali ceviri, isleme devam etme */
        return;
    }

    /* Kameraya gonder */
    (void)UART_SendToCamera(cam_pkt, (uint16_t)cam_len);
}

void UART_HandleCameraPacket(const uint8_t *pkt, uint16_t len)
{
    uint8_t ctrl_resp[64];
    uint8_t ctrl_len = 0U;
    TranslationResult_t tr;

    /* Paket dogrula */
    if (!VerifyCamPacket(pkt, (uint8_t)len)) {
        return;
    }

    /* Kamera yaniti isle ve eski formata cevir */
    tr = CommandHandler_ProcessCamResponse(pkt, (uint8_t)len, ctrl_resp, &ctrl_len);
    if (tr != TRANSLATION_OK) {
        return;
    }

    /* Kontrole gonder */
    (void)UART_SendToControl(ctrl_resp, (uint16_t)ctrl_len);
}


static void reset_control_buffer(void)
{
    (void)memset(control_rx_buf, 0, sizeof(control_rx_buf));
    control_rx_len = 0U;
}

static void reset_camera_buffer(void)
{
    (void)memset(camera_rx_buf, 0, sizeof(camera_rx_buf));
    camera_rx_len = 0U;
}

/* Her gelen byte control tarafina gelir */
static void control_rx_put_byte(uint8_t b)
{
	static uint32_t last_time = 0;
    static uint16_t index = 0;
    uint32_t now = HAL_GetTick(); // ms cinsinden zaman
    //  Timeout kontrolü: 50 ms boyunca veri gelmezse resetle
    if (now - last_time > 50 && control_rx_len>0) {
//        debugger("UART timeout! Buffer temizleniyor.\r\n");
        reset_control_buffer();
        index = 0;
    }
    last_time = now; // son veri zamanı güncelle
    /* Baslangic aranir: control paketleri genelde 0xAA ile baslar */

    if (control_rx_len == 0U) {
        if (b != 0xAAU && b != 0x55U) { /* bazen 0x55 da gelebilir, tolere edelim */
            /* baslangic degil -> ignore */
        	// TODO: Log: Paket geçersiz!
            return;
        }
    }
    /* Buffer ta dolma kontrolu */
    if (control_rx_len < CONTROL_RX_BUFFER_SIZE) {
        control_rx_buf[control_rx_len++] = b;
    } else {
        /* taştı -> reset */
    	// TODO: debugger("\nPaket boyutu uzun!\r\n");
        reset_control_buffer();
        return;
    }
    /* Paket sonu kontrolu: control paketlerinde son iki byte EB AA */
    if (control_rx_len >= 2U) {
        if ((control_rx_buf[control_rx_len - 2U] == 0xEBU) &&
            (control_rx_buf[control_rx_len - 1U] == 0xAAU)) {
            /* Tam paket alindi */
            UART_HandleControlPacket(control_rx_buf, control_rx_len);
            reset_control_buffer();
        }
    }
//        else {
//            /* long packets: bazı kontrol paketleri büyük olabilir; ayrıca checksum
//               kontrolu ve length field'e göre erken tamamlama yapabilirsiniz.
//               Örnegin packet[1] length bilgisine bakıp (packet[1] + offset) kullanmak daha doğru. */
//            /* Alternatif: eger buffer[1] = len (sayac) ise o uzunluga gore de tetikleyin */
//            if (control_rx_len >= 2U) {
//                uint8_t indicated_len = control_rx_buf[1U];
//                /* indicated_len: Kontrol protokolu tanimina gore boyut kontrolu */
//                /* Burada sadece basit kontrol: eger indicated_len+3 == control_rx_len -> tam paket */
//                /* Not: Gelen formatlarda LEN tanimina gore degisebilir, uyarlayin. */
//                if (indicated_len > 0U) {
//                    uint16_t expected_total = (uint16_t)indicated_len + 2U; /* ornek hesaplama */
//                    if (control_rx_len >= expected_total) {
//                        /* Paket bitti varsayimi */
//                        UART_HandleControlPacket(control_rx_buf, control_rx_len);
//                        reset_control_buffer();
//                    }
//                }
//            }
//        }
//    }
}

/* Her gelen byte kamera tarafina gelir */
static void camera_rx_put_byte(uint8_t b)
{
    /* Kamera paketleri 0x55 0xAA ile baslar */
    if (camera_rx_len == 0U) {
        if (b != 0x55U) {
            return;
        }
    } else if (camera_rx_len == 1U) {
        if (b != 0xAAU) {
            /* baslangic hatasi -> reset */
            reset_camera_buffer();
            return;
        }
    }

    if (camera_rx_len < CAMERA_RX_BUFFER_SIZE) {
        camera_rx_buf[camera_rx_len++] = b;
    } else {
        reset_camera_buffer();
        return;
    }

    /* Kamera paket bitisi F0 (son byte) */
    if (camera_rx_buf[camera_rx_len - 1U] == 0xF0U) {
        /* Tam paket alindi */
        UART_HandleCameraPacket(camera_rx_buf, camera_rx_len);
        reset_camera_buffer();
    } else {
        /* Alternatif: packet[2] length alanina gore hizli bitti kontrolu yapilabilir:
           if (camera_rx_len >= 3) { expected_len = camera_rx_buf[2]; if (camera_rx_len >= expected_len+2) ... } */
    }
}

/* End of file */
