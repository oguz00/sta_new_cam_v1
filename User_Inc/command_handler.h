/**
 * @file command_handler.h
 * @brief Kontrol protokolu ve kamera protokolu arasinda ceviri
 *
 * Kontrol Tarafi (Eski Format):
 *   Set:  AA [LEN] 00 [CMD] 01 [PAYLOAD] [CS] EB AA
 *   Read: AA [LEN] 00 [CMD] 00 [CS] EB AA
 *   Resp: 55 [LEN] 00 [CMD] 33 [01/PAYLOAD] [CS] EB AA
 *
 * Kamera Tarafi (Yeni Format):
 *   Cmd:  55 AA [LEN] [CMD1] [CMD2] [CMD3] [PAYLOAD] [XOR] F0
 *   Resp: 55 AA [LEN] [STATUS] [PAYLOAD] [XOR] F0
 *
 * @author oguz00
 * @date 2025-10-17
 * @version 2.0
 */

#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H


#include <stdint.h>
#include <stdbool.h>
#include "command_tracking.h"
#include "packet_builder.h"

/* Make 16-bit key from KB0,KB1 */
#define MAKE_CTRL_KEY(kb0,kb1) ( (uint16_t)( ((uint16_t)(kb0) << 8U) | (uint16_t)(kb1) ) )
/** @brief Komut timeout suresi (milisaniye) */
#define COMMAND_TIMEOUT_MS  (1000U)

/* Kontrol tarafi protokol sabitleri */
#define CTRL_PKT_START_AA       (0xAAU)  /* Set komutu baslangici */
#define CTRL_PKT_START_55       (0x55U)  /* Response baslangici */
#define CTRL_PKT_END_EB           (0xEBU)  /* Son byte 1 */
#define CTRL_PKT_END_AA         (0xAAU)  /* Son byte 2 */
#define CTRL_PKT_RESERVE_SET    (0x01U)  /* Set komutlarinda reserve byte */
#define CTRL_PKT_RESERVE_READ   (0x00U)  /* Read komutlarinda reserve byte */
#define CTRL_PKT_RESP_RESERVE	(0x33U)  /* Response'larda reserve byte */
#define CTRL_PKT_RESP_ACK_BYTE  (0x01U)  /* Set response ACK byte */

/* Kamera tarafi protokol sabitleri */
#define CAM_PKT_START1          (0x55U)  /* Baslangic byte 1 */
#define CAM_PKT_START2          (0xAAU)  /* Baslangic byte 2 */
#define CAM_PKT_END             (0xF0U)  /* Bitis byte */
#define CAM_PKT_ACK_OK          (0x00U)  /* Basarili response */
#define CAM_PKT_ACK_ERROR       (0x01U)  /* Hata response */


#define COMMAND_TIMEOUT_MS      (1000U)
/**
 * @brief Komut tipi
 */
typedef enum {
    CMD_TYPE_SET  = 0U,  /* Set komutu (yazma) */
    CMD_TYPE_READ = 1U   /* Read komutu (okuma) */
} CommandType_t;

/**
 * @brief Ceviri sonuc kodlari
 */
typedef enum {
    TRANSLATION_OK = 0U,
    TRANSLATION_UNKNOWN_CMD,
    TRANSLATION_INVALID_PACKET,
    TRANSLATION_QUEUE_FULL,
    TRANSLATION_CHECKSUM_ERROR,
    TRANSLATION_TIMEOUT,
    TRANSLATION_ERROR
} TranslationResult_t;

/**
 * @brief Kontrol -> Kamera ceviri fonksiyonu
 *
 * Kontrol tarafindan gelen paketi kamera formatina cevirir
 */
typedef bool (*CtrlToCamTranslator_t)(
    const uint8_t *ctrl_packet_ptr,
    uint8_t ctrl_len,
    uint8_t *cam_packet_ptr,
    uint8_t *cam_len_ptr
);

/**
 * @brief Kamera -> Kontrol yanit uretici fonksiyon
 *
 * Kamera yanitindan kontrol yaniti olusturur
 */
typedef bool (*CamToCtrlResponse_t)(
    const uint8_t *cam_response_ptr,
    uint8_t cam_len,
    const uint8_t *original_ctrl_req_ptr,
    uint8_t original_ctrl_len,
    uint8_t *ctrl_response_ptr,
    uint8_t *ctrl_resp_len_ptr
);
/**
 * @brief Opsiyonel matcher: payload bazli daha ince eslesme
 *
 * Return true means mapping matches this ctrl packet (e.g. payload range).
 */
typedef bool (*CtrlMatchFunc_t)(
    const uint8_t *ctrl_packet_ptr,
    uint8_t ctrl_len
);

/**
 * @brief Komut eslestirme kaydi
 *
 * Bu struct flash'ta saklanir. matcher NULL ise translator tek basina yeterlidir.
 */
typedef struct CommandMapping_s {
    uint16_t ctrl_key;                 /* control-side command byte (packet[3]) */
    queryBitEnum query_id;            /* commands_tracking ile iliskilendirir */
    CommandType_t type;               /* set/read */
    uint8_t cam_cmd[3];               /* camera command bytes suggestion */
    CtrlToCamTranslator_t translator; /* ceviri fonksiyonu */
    CamToCtrlResponse_t response_gen; /* yanit uretici */
    CtrlMatchFunc_t matcher;          /* opsiyonel: payload'a gore eslesme */
    const char *desc;                 /* aciklama (readonly) */
}CommandMapping_t;

/* Direct lookup: 256 pointers (small RAM cost, fast lookup) */
extern const CommandMapping_t *g_cmd_lookup_table[256];

/**
 * @brief Build lookup table from static mapping array.
 *
 * Call once at init (CommandHandler_Init does this).
 */
void CommandHandler_BuildLookup(void);

/**
 * @brief Komut isleyiciyi baslat
 */
void CommandHandler_Init(void);

/**
 * CommandHandler_TranslateCtrlToCam - Kontrol paketini kamera paketine cevirir
 *
 * Bu fonksiyon:
 *  - Kontrol paketini dogrular (VerifyCtrlPacket),
 *  - KB0/KB1 anahtarini olusturup command_map uzerinde binary search ile mapping bulur,
 *  - Eger mapping->matcher varsa onu calistirir,
 *  - Mapping'in translator'ini cagirarak kamera paketini uretir,
 *  - Orjinal kontrol istegini pending buffer'a (CmdRingBuffer) ekler.
 *
 *
 */
TranslationResult_t CommandHandler_TranslateCtrlToCam(
    const uint8_t *ctrl_packet_ptr,
    uint8_t ctrl_len,
    uint8_t *cam_packet_ptr,
    uint8_t *cam_len_ptr
);

/**
 * @brief Kamera yanitini kontrol yanitina cevir
 *
 * @param[in]  cam_response_ptr   Kameradan gelen yanit
 * @param[in]  cam_len            Yanit uzunlugu
 * @param[out] ctrl_response_ptr  Kontrole gonderilecek yanit (cikti)
 * @param[out] ctrl_len_ptr       Kontrol yaniti uzunlugu (cikti)
 *
 * @return Ceviri sonucu
 */
TranslationResult_t CommandHandler_ProcessCamResponse(
    const uint8_t *cam_response_ptr,
    uint8_t cam_len,
    uint8_t *ctrl_response_ptr,
    uint8_t *ctrl_len_ptr
);

/**
 * @brief Timeout kontrol et
 *
 * @return Kaldirilan komut sayisi
 */
uint8_t CommandHandler_CheckTimeouts(void);

/**
 * @brief Bekleyen komut sayisi
 *
 * @return Komut sayisi
 */
uint32_t CommandHandler_GetPendingCount(void);

/**
 * @brief Kontrol paketi checksum hesapla (mod 256)
 *
 * @param[in] packet_ptr  Paket buffer
 * @param[in] len         Paket uzunlugu
 *
 * @return Checksum degeri
 */
uint8_t CalculateCtrlChecksum(const uint8_t *packet_ptr, uint8_t len);

/**
 * @brief Kamera paketi XOR checksum hesapla
 *
 * @param[in] packet_ptr  Paket buffer (55 AA'dan sonrasi)
 * @param[in] len         Checksum hesaplanacak uzunluk
 *
 * @return XOR checksum degeri
 */
uint8_t CalculateCamChecksum(const uint8_t *packet_ptr, uint8_t len);

/**
 * @brief Kontrol paketi dogrula
 *
 * @param[in] packet_ptr  Paket buffer
 * @param[in] len         Paket uzunlugu
 *
 * @return true = gecerli, false = gecersiz
 */
bool VerifyCtrlPacket(const uint8_t *packet_ptr, uint8_t len);

/**
 * @brief Kamera paketi dogrula
 *
 * @param[in] packet_ptr  Paket buffer
 * @param[in] len         Paket uzunlugu
 *
 * @return true = gecerli, false = gecersiz
 */
bool VerifyCamPacket(const uint8_t *packet_ptr, uint8_t len);

#endif /* COMMAND_HANDLER_H */

/* command_handler.h sonu */
