/**
 * @file command_handler.c
 * @brief Komut cevirici implementasyonu
 *
 * @author oguz00
 * @date 2025-11-17
 * @version 2.0
 */



#include "command_handler.h"
#include "main.h"
#include <string.h>



/* Bekleyen komutlar buffer'i */
static cmdRingBuffer_t g_pending_commands;



uint8_t CalculateCtrlChecksum(const uint8_t *packet_ptr, uint8_t len)
{
    uint16_t sum = 0U;
    uint8_t i;

    /* Kontrol protokolu: Byte'lari topla, mod 256 al */
    /* AA'dan sonra basla, CS'den once bitir */
    if ((packet_ptr != NULL) && (len > 4U)) {

        /* Byte 1'den (LEN) CS'den onceki byte'a kadar topla */
        for (i = 1U; i < (len - 3U); i++) {
            sum = (uint16_t)(sum + (uint16_t)packet_ptr[i]);
        }
    }

    /* Mod 256 (otomatik olur, uint8_t cast ile) */
    return (uint8_t)(sum & 0xFFU);
}

uint8_t CalculateCamChecksum(const uint8_t *packet_ptr, uint8_t len)
{
    uint8_t xor_result = 0U;
    uint8_t i;

    /* Kamera protokolu: XOR hesapla */
    /* 55 AA'dan sonra basla, F0'dan once bitir */
    if ((packet_ptr != NULL) && (len > 4U)) {

        /* Byte 2'den (LEN) XOR CS'den onceki byte'a kadar */
        for (i = 2U; i < (len - 2U); i++) {
            xor_result = xor_result ^ packet_ptr[i];
        }
    }

    return xor_result;
}

bool VerifyCtrlPacket(const uint8_t *packet_ptr, uint8_t len)
{
    bool result = false;
    uint8_t calculated_cs;
    uint8_t received_cs;

    /* Minimum uzunluk: AA LEN 00 CMD XX CS EB AA = 8 byte */
    if ((packet_ptr != NULL) && (len >= 8U)) {

        /* Baslangic byte kontrolu */
        if ((packet_ptr[0] == CTRL_PKT_START_TX) || (packet_ptr[0] == CTRL_PKT_START_RX)) {

            /* Bitis byte'lari kontrolu */
            if ((packet_ptr[len - 2U] == CTRL_PKT_END1) &&
                (packet_ptr[len - 1U] == CTRL_PKT_END2)) {

                /* Checksum kontrolu */
                calculated_cs = CalculateCtrlChecksum(packet_ptr, len);
                received_cs = packet_ptr[len - 3U];  /* CS, EB AA'dan once */

                if (calculated_cs == received_cs) {
                    result = true;
                }
            }
        }
    }

    return result;
}

bool VerifyCamPacket(const uint8_t *packet_ptr, uint8_t len)
{
    bool result = false;
    uint8_t calculated_xor;
    uint8_t received_xor;

    /* Minimum uzunluk: 55 AA LEN STATUS XOR F0 = 6 byte */
    if ((packet_ptr != NULL) && (len >= 6U)) {

        /* Baslangic byte'lari kontrolu */
        if ((packet_ptr[0] == CAM_PKT_START1) && (packet_ptr[1] == CAM_PKT_START2)) {

            /* Bitis byte kontrolu */
            if (packet_ptr[len - 1U] == CAM_PKT_END) {

                /* XOR checksum kontrolu */
                calculated_xor = CalculateCamChecksum(packet_ptr, len);
                received_xor = packet_ptr[len - 2U];  /* XOR, F0'dan once */

                if (calculated_xor == received_xor) {
                    result = true;
                }
            }
        }
    }

    return result;
}



/*
 * Ornek: Basit Set Komutu Ceviri
 * Kontrol: AA 05 00 16 01 00 C6 EB AA
 * Kamera:  55 AA 07 02 01 08 00 00 00 01 0D F0
 */
bool Translator_SimpleSet(
    const uint8_t *ctrl_packet_ptr,
    uint8_t ctrl_len,
    uint8_t *cam_packet_ptr,
    uint8_t *cam_len_ptr)
{
    bool result = false;
    uint8_t payload;
    uint8_t xor_cs;
    uint8_t cam_idx;
    const CommandMapping_t *mapping_ptr;
    uint8_t i;

    /* Bu fonksiyonu cagirancommandon mapping'i alacagiz */
    /* Simdilik ornek mapping kullaniyoruz */

    if ((ctrl_packet_ptr != NULL) && (cam_packet_ptr != NULL) &&
        (cam_len_ptr != NULL) && (ctrl_len >= 8U)) {

        /* Kontrol paketinden payload'u al */
        /* AA [LEN] [00] [CMD] [01] [PAYLOAD] [CS] [EB] [AA] */
        payload = ctrl_packet_ptr[5];  /* Payload byte */

        /* Kamera paketi olustur */
        cam_idx = 0U;
        cam_packet_ptr[cam_idx++] = CAM_PKT_START1;  /* 55 */
        cam_packet_ptr[cam_idx++] = CAM_PKT_START2;  /* AA */
        cam_packet_ptr[cam_idx++] = 0x07U;           /* LEN (placeholder) */
        cam_packet_ptr[cam_idx++] = 0x02U;           /* CMD1 */
        cam_packet_ptr[cam_idx++] = 0x01U;           /* CMD2 */
        cam_packet_ptr[cam_idx++] = 0x08U;           /* CMD3 */
        cam_packet_ptr[cam_idx++] = payload;         /* PAYLOAD */

        /* XOR checksum hesapla (LEN'den payload'a kadar) */
        xor_cs = 0U;
        for (i = 2U; i < cam_idx; i++) {
            xor_cs = xor_cs ^ cam_packet_ptr[i];
        }

        cam_packet_ptr[cam_idx++] = xor_cs;          /* XOR CS */
        cam_packet_ptr[cam_idx++] = CAM_PKT_END;     /* F0 */

        /* Uzunlugu guncelle */
        *cam_len_ptr = cam_idx;

        result = true;
    }

    return result;
}

/*
 * Ornek: Zoom Set Komutu Ceviri
 * Kontrol: AA 0D 00 2A 01 [10 byte payload] CS EB AA
 * Kamera:  55 AA XX 02 01 08 [payload] XOR F0
 */
bool Translator_ZoomSet(
    const uint8_t *ctrl_packet_ptr,
    uint8_t ctrl_len,
    uint8_t *cam_packet_ptr,
    uint8_t *cam_len_ptr)
{
    bool result = false;
    uint8_t payload_len;
    uint8_t xor_cs;
    uint8_t cam_idx;
    uint8_t i;

    if ((ctrl_packet_ptr != NULL) && (cam_packet_ptr != NULL) &&
        (cam_len_ptr != NULL) && (ctrl_len >= 18U)) {  /* Min: AA+13 byte+CS+EB+AA */

        /* Kontrol: AA [LEN=0D] [00] [2A] [01] [10 byte payload] [CS] [EB] [AA] */
        payload_len = 10U;  /* Zoom payload her zaman 10 byte */

        /* Kamera paketi olustur */
        cam_idx = 0U;
        cam_packet_ptr[cam_idx++] = CAM_PKT_START1;  /* 55 */
        cam_packet_ptr[cam_idx++] = CAM_PKT_START2;  /* AA */
        cam_packet_ptr[cam_idx++] = 0x00U;           /* LEN (hesaplanacak) */
        cam_packet_ptr[cam_idx++] = 0x02U;           /* CMD1 */
        cam_packet_ptr[cam_idx++] = 0x01U;           /* CMD2 */
        cam_packet_ptr[cam_idx++] = 0x08U;           /* CMD3 (Zoom command) */

        /* 10 byte payload kopyala */
        for (i = 0U; i < payload_len; i++) {
            cam_packet_ptr[cam_idx++] = ctrl_packet_ptr[5U + i];  /* Payload baslangici: byte 5 */
        }

        /* LEN guncelle (LEN'den sonraki tum byte'lar, XOR ve F0 haric) */
        cam_packet_ptr[2] = (uint8_t)(cam_idx - 2U);  /* 55 AA haric */

        /* XOR checksum hesapla */
        xor_cs = 0U;
        for (i = 2U; i < cam_idx; i++) {
            xor_cs = xor_cs ^ cam_packet_ptr[i];
        }

        cam_packet_ptr[cam_idx++] = xor_cs;          /* XOR CS */
        cam_packet_ptr[cam_idx++] = CAM_PKT_END;     /* F0 */

        *cam_len_ptr = cam_idx;

        result = true;
    }

    return result;
}

/*
 * Ornek: Read Komutu Ceviri
 * Kontrol: AA 05 00 03 00 CS EB AA (Read komutu - payload yok)
 * Kamera:  55 AA 07 02 02 XX 00 00 00 00 XOR F0 (Query komutu)
 */
bool Translator_ReadQuery(
    const uint8_t *ctrl_packet_ptr,
    uint8_t ctrl_len,
    uint8_t *cam_packet_ptr,
    uint8_t *cam_len_ptr)
{
    bool result = false;
    uint8_t ctrl_cmd;
    uint8_t cam_query_cmd;
    uint8_t xor_cs;
    uint8_t cam_idx;
    uint8_t i;

    if ((ctrl_packet_ptr != NULL) && (cam_packet_ptr != NULL) &&
        (cam_len_ptr != NULL) && (ctrl_len >= 8U)) {

        /* Kontrol komutunu al */
        ctrl_cmd = ctrl_packet_ptr[3];  /* CMD byte */

        /* Kontrol komutunu kamera query komutuna esle */
        cam_query_cmd = 0x00U;
        if (ctrl_cmd == 0x03U) {
            cam_query_cmd = 0x10U;  /* Status query */
        } else if (ctrl_cmd == 0x04U) {
            cam_query_cmd = 0x20U;  /* Temperature query */
        } else if (ctrl_cmd == 0x05U) {
            cam_query_cmd = 0x30U;  /* Version query */
        } else {
            /* Bilinmeyen komut */
            return false;
        }

        /* Kamera query paketi olustur */
        cam_idx = 0U;
        cam_packet_ptr[cam_idx++] = CAM_PKT_START1;  /* 55 */
        cam_packet_ptr[cam_idx++] = CAM_PKT_START2;  /* AA */
        cam_packet_ptr[cam_idx++] = 0x07U;           /* LEN */
        cam_packet_ptr[cam_idx++] = 0x02U;           /* CMD1 */
        cam_packet_ptr[cam_idx++] = 0x02U;           /* CMD2 (Query) */
        cam_packet_ptr[cam_idx++] = cam_query_cmd;   /* CMD3 (Query type) */
        cam_packet_ptr[cam_idx++] = 0x00U;           /* Dummy payload */
        cam_packet_ptr[cam_idx++] = 0x00U;
        cam_packet_ptr[cam_idx++] = 0x00U;
        cam_packet_ptr[cam_idx++] = 0x00U;

        /* XOR checksum hesapla */
        xor_cs = 0U;
        for (i = 2U; i < cam_idx; i++) {
            xor_cs = xor_cs ^ cam_packet_ptr[i];
        }

        cam_packet_ptr[cam_idx++] = xor_cs;          /* XOR CS */
        cam_packet_ptr[cam_idx++] = CAM_PKT_END;     /* F0 */

        *cam_len_ptr = cam_idx;

        result = true;
    }

    return result;
}
