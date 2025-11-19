/**
 * @file commands_tracking.h
 * @brief Bekleyen komutlari takip eden circular buffer
 *
 * @author oguz00
 * @date 2025-11-17
 * @version 1.0
 */
#ifndef COMMAND_TRACKING_H_
#define COMMAND_TRACKING_H_

// Includes->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// command_handler.h ile circular dependency engellemek için
struct CommandMapping_s;


//MAKROLAR
/** @brief Maksimum komut paket uzunlugu */
#define CMD_MAX_LENGTH (32U)
/** @brief Buffer'da tutulabilecek maksimum komut sayisi */
#define CMD_BUFFER_SIZE   (16U)
//Tip tanımları->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
/**
 * @brief Sorgu tipi enum
 *
 * Hangi komutun gonderildigini belirtir
 */
typedef enum {
    QUERY_NONE = 0U,              /**< Sorgu yok */
	QUERY_IMG_PAL,
	QUERY_BR_CT,
	QUERY_AUTO_NUC,
    QUERY_MAX                     /**< Maksimum deger */
} queryBitEnum;

/**
 * @brief Komut blogu yapisi
 *
 * Bekleyen bir komutun tum bilgilerini tutar
 */
#pragma pack(push, 1)
typedef struct{
//	uint8_t expected_response[CMD_MAX_LENGTH]; 		/**< Beklenen yanit paketi */
//	uint32_t response_length; 						/**< Yanit uzunlugu */
	uint8_t original_request[CMD_MAX_LENGTH];		/**< Orjinal istek paketi */
	uint32_t request_lenth;							/**< Istek uzunlugu */
	queryBitEnum nmbr;								/**< Sorgu tipi */
	uint32_t timestamp;								/**< Gonderilme zamani (ms) */
	const void *mapping;							/**< CommandMapping_t pointer */

} cmdBlock_t ;
#pragma pack(pop)
/**
 * @brief Circular buffer yapisi
 *
 * Bekleyen komutlari yoneten ring buffer
 */
#pragma pack(push, 1)
typedef struct{
	cmdBlock_t buffer[CMD_MAX_LENGTH]; /**< Komut blokları dizisi */
	uint32_t head;					   /**< Yazma pozisyonu*/
	uint32_t tail;						/**< Okuma pozisyonu*/
	uint32_t count;						/**< Buffer'daki eleman sayisi*/
}cmdRingBuffer_t;
#pragma pack(pop)
//Fonksiyon prototipleri ->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
/**
 * @brief Buffer'i baslat
 *
 * Tum alanlari sifirlar, buffer'i kullanima hazir hale getirir.
 * Kullanmadan once mutlaka cagirilmali.
 *
 * @param[out] ring_buf_ptr  Buffer pointer (NULL olmamali)
 *
 * @return void
 */
void CmdRingBuffer_Init(cmdRingBuffer_t* ring_buf_ptr);
/**
 * @brief Buffer'a yeni komut ekle
 *
 * Bekleyen komut listesine yeni bir komut ekler.
 * Orjinal istek, beklenen yanit ve metadata saklanir.
 *
 * @param[in,out] ring_buf_ptr       Buffer pointer (NULL olmamali)
 * @param[in]     expected_resp_ptr  Beklenen yanit (NULL olabilir)
 * @param[in]     resp_len           Yanit uzunlugu
 * @param[in]     orig_req_ptr       Orjinal istek paketi (NULL olmamali)
 * @param[in]     req_len            Istek uzunlugu
 * @param[in]     query_type         Sorgu tipi
 * @param[in]     mapping_ptr        Komut mapping pointer (NULL olmamali)
 *
 * @return true = basarili, false = buffer dolu veya gecersiz parametre
 */

bool CmdRingBuffer_PushComplete(
		cmdRingBuffer_t *ring_buf_ptr,
//		const uint8_t *expected_resp_ptr,
//		uint32_t resp_len,
		const uint8_t *org_req_ptr,
		uint32_t req_len,
		queryBitEnum query_type,
		const void *mapping_ptr
		);
/**
 * @brief En eski komutu al ve bu komutu kuyruktan kaldir
 *
 * Buffer'daki en eski (ilk gonderilen) komutu alir ve buffer'dan siler.
 *
 * @param[in,out] ring_buf_ptr  Buffer pointer (NULL olmamali)
 * @param[out]    block_ptr     Alinan komut blogu (NULL olmamali)
 *
 * @return true = basarili, false = buffer bos veya gecersiz parametre
 */
bool CmdRingBuffer_Pop(
    cmdRingBuffer_t *ring_buf_ptr,
    cmdBlock_t *block_ptr
);

/**
 * @brief En eski komutu görmeye yarar ( bu komuta bakmak istediğimizde
 * komut tablodan silinmez)
 *
 * Buffer'daki en eski komutu kopyalar ama buffer'dan silmez.
 *
 * @param[in]  ring_buf_ptr  Buffer pointer (NULL olmamali)
 * @param[out] block_ptr     Kopyalanan komut blogu (NULL olmamali)
 *
 * @return true = basarili, false = buffer bos veya gecersiz parametre
 */
bool CmdRingBuffer_Peek(
    const cmdRingBuffer_t *ring_buf_ptr,
    cmdBlock_t *block_ptr
);

/**
 * @brief Buffer bos mu kontrol et
 *
 * @param[in] ring_buf_ptr  Buffer pointer
 *
 * @return true = bos, false = dolu
 */
bool CmdRingBuffer_IsEmpty(
    const cmdRingBuffer_t *ring_buf_ptr
);

/**
 * @brief Buffer tamamen dolu mu kontrol et
 *
 * @param[in] ring_buf_ptr  Buffer pointer
 *
 * @return true = dolu, false = bos yer var
 */
bool CmdRingBuffer_IsFull(
    const cmdRingBuffer_t *ring_buf_ptr
);

/**
 * @brief Buffer'daki komut sayisini al
 *
 * @param[in] ring_buf_ptr  Buffer pointer
 *
 * @return Bekleyen komut sayisi (0-16 arasi)
 */
uint32_t CmdRingBuffer_Size(
    const cmdRingBuffer_t *ring_buf_ptr
);

/**
 * @brief Buffer'i temizle
 *
 * Tum komutlari siler, buffer'i sifirlar.
 *
 * @param[in,out] ring_buf_ptr  Buffer pointer (NULL olmamali)
 *
 * @return void
 */
void CmdRingBuffer_Clear(
    cmdRingBuffer_t *ring_buf_ptr
);
/**
 * @brief Timeout olan komutu kaldir
 *
 * En eski komutun zamanini kontrol eder.
 * Timeout asilmissa o komutu buffer'dan kaldirir.
 *
 * @param[in,out] ring_buf_ptr   Buffer pointer (NULL olmamali)
 * @param[in]     timeout_ms     Timeout suresi (milisaniye)
 * @param[in]     current_time   Suanki zaman (HAL_GetTick'ten alinan)
 *
 * @return true = komut kaldirildi, false = timeout yok
 *
 * @note Ana dongu icinde periyodik cagirilmali (ornek: her 100ms)
 */
bool CmdRingBuffer_RemoveIfTimeOut(
		cmdRingBuffer_t *ring_buf_ptr,
		uint32_t timeout_ms,
		uint32_t current_time
		);

#endif /* COMMAND_TRACKING_H_ */
