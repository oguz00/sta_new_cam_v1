/**
 * @file command_handler.c
 * @brief Command handler: lookup table, translators, response generators
 *
 *
 * Not: Bu versiyon mapping/lookup altyapisi icerir; 3 ornek mapping vardir.
 * Daha fazla komutu command_map[] icine ekleyebilirsiniz
 *
 * Created: 2025-11-18
 */


#include "command_handler.h"
#include "command_tracking.h"
#include "main.h"      /* HAL_GetTick */
#include <string.h>


#define CONSTANT_PL_FOR_CALC_CS  9 // -> 9 byte yanıt paket uzunluğu
/* Bekleyen komutlar buffer'i */
static cmdRingBuffer_t g_pending_commands;

///* Forward declare translator/response functions (implemented below) */
static bool Translator_SimpleSet(
    const uint8_t *ctrl_packet_ptr, uint8_t ctrl_len,
    uint8_t *cam_packet_ptr, uint8_t *cam_len_ptr);

static bool Translator_ZoomSet(
    const uint8_t *ctrl_packet_ptr, uint8_t ctrl_len,
    uint8_t *cam_packet_ptr, uint8_t *cam_len_ptr);

static bool Translator_ReadQuery(
    const uint8_t *ctrl_packet_ptr, uint8_t ctrl_len,
    uint8_t *cam_packet_ptr, uint8_t *cam_len_ptr);

static bool ResponseGen_SimpleACK(
    const uint8_t *cam_resp_ptr, uint8_t cam_len,
    const uint8_t *orig_ctrl_ptr, uint8_t orig_ctrl_len,
    uint8_t *ctrl_resp_ptr, uint8_t *ctrl_resp_len_ptr);

static bool ResponseGen_EchoParam(
    const uint8_t *cam_resp_ptr, uint8_t cam_len,
    const uint8_t *orig_ctrl_ptr, uint8_t orig_ctrl_len,
    uint8_t *ctrl_resp_ptr, uint8_t *ctrl_resp_len_ptr);

static bool ResponseGen_MultiParam(
    const uint8_t *cam_resp_ptr, uint8_t cam_len,
    const uint8_t *orig_ctrl_ptr, uint8_t orig_ctrl_len,
    uint8_t *ctrl_resp_ptr, uint8_t *ctrl_resp_len_ptr);


/* Example entries from the KB table you provided */
/* TODO: Bu noktada binary search ile tablo aranacağı için CTRL_KEY 'leri yani araç tarafından
 * gelen komut byte'larının  sıralı bir şekilde lsiteye girilmesi gerekmektedir!!!
 */
static const CommandMapping_t command_map[] = {
    /* ctrl_key,                  query_id,        type,           cam_cmd,         translator,           response_gen,       match_pload_func   desc */
	{ MAKE_CTRL_KEY(0x00,0x16),  QUERY_NONE, 	CMD_TYPE_READ,	{0x02, 0x01, 0x08},Translator_SimpleSet,ResponseGen_SimpleACK,  nullptr ,"Manuel NUC" },
    { MAKE_CTRL_KEY(0x00,0x2D),  QUERY_NONE, 	CMD_TYPE_SET,  	{0x02, 0x00, 0x04},Translator_SimpleSet,ResponseGen_SimpleACK,  nullptr,"Image Palette BLCK"},
	{ MAKE_CTRL_KEY(0x00,0x2D),  QUERY_NONE, 	CMD_TYPE_SET,  	{0x02, 0x00, 0x04},Translator_SimpleSet,ResponseGen_SimpleACK,  nullptr, "Image Palette WHT"},
	{ MAKE_CTRL_KEY(0x00,0x2D),  QUERY_IMG_PAL, CMD_TYPE_READ,	{0x02, 0x00, 0x04},Translator_SimpleSet,ResponseGen_SimpleACK,  nullptr, "Image Palette RD"},

    /* Add remaining commands, keep sorted by ctrl_key */
};
#define CMD_MAP_COUNT (sizeof(command_map) / sizeof(command_map[0]))

/* Declare in header as extern; define here */
const CommandMapping_t *g_cmd_lookup_table[256];

/**
 * @brief Build lookup table from command_map
 *
 * Fills g_cmd_lookup_table with pointers; first come first served on collisions.
 */
void CommandHandler_BuildLookup(void)
{
    uint32_t i;
    uint32_t idx_u32;
    /* Clear table */
    for (i = 0U; i < 256U; i++) {
        g_cmd_lookup_table[i] = (const CommandMapping_t *)0;
    }

    /* Fill from command_map */
    for (i = 0U; i < CMD_MAP_COUNT; i++) {
        idx_u32 = (uint32_t)command_map[i].ctrl_key;
        if (g_cmd_lookup_table[idx_u32] == (const CommandMapping_t *)0) {
            g_cmd_lookup_table[idx_u32] = &command_map[i];
        } else {
            /* Collision: keep first; translator/matcher should disambiguate if needed */
            /* Optionally implement a collision list here in future. */
        }
    }
}

/**
 * @brief Compare keys for binary search
 * @return <0 if a<b, 0 if equal, >0 if a>b
 */
static int16_t CompareCtrlKey(const void *a, const void *b)
{
    const uint16_t ka = ((const CommandMapping_t *)a)->ctrl_key;
    const uint16_t kb = ((const CommandMapping_t *)b)->ctrl_key;
    if (ka < kb) { return -1; }
    if (ka > kb) { return 1; }
    return 0;
}

/**
 * @brief Find mapping by 16-bit control key using binary search
 * @note command_map[] MUST be sorted by ctrl_key ascending
 */
const CommandMapping_t *FindMappingByCtrlKey(uint16_t key)
{
    int32_t low = 0;
    int32_t high = (int32_t)CMD_MAP_COUNT - 1;
    while (low <= high) {
        int32_t mid = (low + high) >> 1;
        uint16_t mid_key = command_map[mid].ctrl_key;
        if (mid_key == key) {
            return &command_map[mid];
        } else if (mid_key < key) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return (const CommandMapping_t *)0;
}
uint8_t CalculateCtrlChecksum(const uint8_t *packet_ptr, uint8_t len)
{
    uint32_t sum = 0U;
    uint8_t i;

    if ((packet_ptr == NULL) || (len < 4U)) {
        return 0U;
    }

    /* Sum from LEN (index 1) up to byte before CS (len-3) */
    for (i = 0U; i < (len - 3U); i++) {
        sum += (uint32_t)packet_ptr[i];
    }

    return (uint8_t)(sum%256);
}

uint8_t CalculateCamChecksum(const uint8_t *packet_ptr, uint8_t len)
{
    uint8_t xorv = 0U;
    uint8_t i;

    if ((packet_ptr == NULL) || (len < 4U)) {
        return 0U;
    }

    /* XOR from LEN (index 2) up to byte before XOR (len-2) */
    for (i = 2U; i < (len - 2U); i++) {
        xorv ^= packet_ptr[i];
    }

    return xorv;
}

bool VerifyCtrlPacket(const uint8_t *packet_ptr, uint8_t len)
{
    if ((packet_ptr == NULL) || (len < 8U)) {
        return false;
    }

    /* Start may be AA or 55 depending who sends, accept both */
    if (!((packet_ptr[0] == 0xAAU) || (packet_ptr[0] == 0x55U))) {
        return false;
    }

    /* End bytes check */
    if (!((packet_ptr[len - 2U] == CTRL_PKT_END_EB) && (packet_ptr[len - 1U] == CTRL_PKT_END_AA))) {
        return false;
    }

    /* Checksum located at len-3 */
    if (CalculateCtrlChecksum(packet_ptr, len) != packet_ptr[len - 3U]) {
        return false;
    }

    return true;
}

bool VerifyCamPacket(const uint8_t *packet_ptr, uint8_t len)
{
    if ((packet_ptr == NULL) || (len < 6U)) {
        return false;
    }

    if ((packet_ptr[0] != CAM_PKT_START1) || (packet_ptr[1] != CAM_PKT_START2)) {
        return false;
    }

    if (packet_ptr[len - 1U] != CAM_PKT_END) {
        return false;
    }

    if (CalculateCamChecksum(packet_ptr, len) != packet_ptr[len - 2U]) {
        return false;
    }

    return true;
}

void CommandHandler_Init(void)
{
    /* Init pending buffer and lookup table */
    CmdRingBuffer_Init(&g_pending_commands);
    CommandHandler_BuildLookup();
}


TranslationResult_t CommandHandler_TranslateCtrlToCam(
    const uint8_t *ctrl_packet_ptr,
    uint8_t ctrl_len,
    uint8_t *cam_packet_ptr,
    uint8_t *cam_len_ptr)
{
	//    TranslationResult_t result = TRANSLATION_ERROR;
	    if ((ctrl_packet_ptr == NULL) || (cam_packet_ptr == NULL) || (cam_len_ptr == NULL) || (ctrl_len < 8U)) {
	        return TRANSLATION_INVALID_PACKET;
	    }

	    if (!VerifyCtrlPacket(ctrl_packet_ptr, ctrl_len)) {
	        return TRANSLATION_CHECKSUM_ERROR;
	    }
	    /* Extract KB0, KB1 from control packet:
	       AA [LEN] 00 KB0 KB1 ...
	       KB0 == packet[3], KB1 == packet[4]
	    */
	    if (ctrl_len < 5U) {
	           return TRANSLATION_INVALID_PACKET;
	       }
	    uint8_t kb0 = ctrl_packet_ptr[2U];
		uint8_t kb1 = ctrl_packet_ptr[3U];
		uint16_t key = MAKE_CTRL_KEY(kb0, kb1);
		const CommandMapping_t *mapping = FindMappingByCtrlKey(key);
		if (mapping == (const CommandMapping_t *)0) {
			return TRANSLATION_UNKNOWN_CMD;
		}
	    /* If mapping has matcher, check it */
//	    if ((mapping->matcher != NULL) && (mapping->matcher(ctrl_packet_ptr, ctrl_len) == false)) {
//	        return TRANSLATION_UNKNOWN_CMD;
//	    }
	    if(ctrl_len==0x04)
	    {

	    }
	    /* Build camera packet via translator */
	    bool ok = mapping->translator(ctrl_packet_ptr, ctrl_len, cam_packet_ptr, cam_len_ptr);
	    if (!ok) {
	        return TRANSLATION_ERROR;
	    }
	    /* Push to pending buffer (store original ctrl request and mapping pointer) */
	    if (!CmdRingBuffer_PushComplete(
	            &g_pending_commands,
	            ctrl_packet_ptr,
	            (uint32_t)ctrl_len,
	            mapping->query_id,
	            (const void *)mapping)) {
	        return TRANSLATION_QUEUE_FULL;
	    }
	    return TRANSLATION_OK;

}
/**
 * @brief Process incoming camera response and generate control response
 */
TranslationResult_t CommandHandler_ProcessCamResponse(
    const uint8_t *cam_response_ptr,
    uint8_t cam_len,
    uint8_t *ctrl_response_ptr,
    uint8_t *ctrl_len_ptr)
{
    TranslationResult_t result = TRANSLATION_ERROR;
    cmdBlock_t pending;
    const CommandMapping_t *mapping = NULL;
    bool pop_ok;
    bool gen_ok;

    if ((cam_response_ptr == NULL) || (ctrl_response_ptr == NULL) || (ctrl_len_ptr == NULL)) {
        return TRANSLATION_INVALID_PACKET;
    }

    if (!VerifyCamPacket(cam_response_ptr, cam_len)) {
        return TRANSLATION_CHECKSUM_ERROR;
    }

    /* Pop oldest pending command */
    pop_ok = CmdRingBuffer_Pop(&g_pending_commands, &pending);
    if (!pop_ok) {
        return TRANSLATION_INVALID_PACKET;
    }

    mapping = (const CommandMapping_t *)pending.mapping;
    if (mapping == (const CommandMapping_t *)0) {
        return TRANSLATION_ERROR;
    }

    /* Call response generator */
    gen_ok = mapping->response_gen(
        cam_response_ptr,
        cam_len,
        pending.original_request,
        (uint8_t)pending.request_lenth,
        ctrl_response_ptr,
        ctrl_len_ptr);

    if (!gen_ok) {
        return TRANSLATION_ERROR;
    }

    return TRANSLATION_OK;
}

uint8_t CommandHandler_CheckTimeouts(void)
{
    uint8_t removed = 0U;
    uint32_t now = HAL_GetTick();
    bool again;

    do {
        again = CmdRingBuffer_RemoveIfTimeOut(&g_pending_commands, COMMAND_TIMEOUT_MS, now);
        if (again) {
            removed++;
        }
    } while (again);

    return removed;
}

uint32_t CommandHandler_GetPendingCount(void)
{
    return CmdRingBuffer_Size(&g_pending_commands);
}


/* Simple set translator: single payload byte -> camera simple cmd */
static bool Translator_SimpleSet(
    const uint8_t *ctrl_packet_ptr, uint8_t ctrl_len,
    uint8_t *cam_packet_ptr, uint8_t *cam_len_ptr)
{
    uint8_t payload[2]={0x00,0x01};
    uint8_t idx = 0U;
    uint8_t xorv = 0U;
    uint8_t i;

    if ((ctrl_packet_ptr == NULL) || (cam_packet_ptr == NULL) || (cam_len_ptr == NULL) || (ctrl_len < 8U)) {
        return false;
    }

    /* payload typically at index 5 for set commands */


    /* build camera packet: 55 AA LEN B1 B2 B3 PAYLOAD XOR F0
       sample values used here; adjust cam_cmd bytes as needed */

    //TODO:  Bu noktada payload seçimi yapılacak.
    cam_packet_ptr[idx++] = CAM_PKT_START1;    /* 55 */
    cam_packet_ptr[idx++] = CAM_PKT_START2;    /* AA */
    cam_packet_ptr[idx++] = 0x07U;             /* LEN placeholder (55 AA excluded) */
    cam_packet_ptr[idx++] = 0x02U;             /* CMD1 (example) */
    cam_packet_ptr[idx++] = 0x01U;             /* CMD2 (example) */
    cam_packet_ptr[idx++] = 0x08U;             /* CMD3 (example) */
    cam_packet_ptr[idx++] = 0x00U;             /* Zero */
    cam_packet_ptr[idx++] = 0x00U;             /* Zero */
    cam_packet_ptr[idx++] = payload[0];           /* payload */
    cam_packet_ptr[idx++] = payload[1];           /* payload */

    /* XOR from index 2 to before XOR */
    xorv = 0U;
    for (i = 2U; i < idx; i++) {
        xorv ^= cam_packet_ptr[i];
    }

    cam_packet_ptr[idx++] = xorv;
    cam_packet_ptr[idx++] = CAM_PKT_END;

    *cam_len_ptr = idx;
    return true;
}

/* Zoom translator: copy 10-byte zoom payload into camera packet */
static bool Translator_ZoomSet(
    const uint8_t *ctrl_packet_ptr, uint8_t ctrl_len,
    uint8_t *cam_packet_ptr, uint8_t *cam_len_ptr)
{
    uint8_t idx = 0U;
    uint8_t xorv = 0U;
    uint8_t i;
    const uint8_t payload_len = 10U;

    if ((ctrl_packet_ptr == NULL) || (cam_packet_ptr == NULL) || (cam_len_ptr == NULL)) {
        return false;
    }

    /* minimal ctrl length for zoom set expected around 18 (AA + 0x0D + ... ) */
    if (ctrl_len < (5U + payload_len + 3U)) { /* rough check */
        return false;
    }

    /* build header */
    cam_packet_ptr[idx++] = CAM_PKT_START1;
    cam_packet_ptr[idx++] = CAM_PKT_START2;
    cam_packet_ptr[idx++] = 0x00U;  /* LEN filled later */
    cam_packet_ptr[idx++] = 0x02U;  /* CMD1 */
    cam_packet_ptr[idx++] = 0x01U;  /* CMD2 */
    cam_packet_ptr[idx++] = 0x08U;  /* CMD3 (zoom) */

    /* copy 10 bytes from ctrl payload starting at ctrl_packet_ptr[5] */
    for (i = 0U; i < payload_len; i++) {
        cam_packet_ptr[idx++] = ctrl_packet_ptr[5U + i];
    }

    /* update LEN: count of bytes after 55 AA up to and including checksum */
    cam_packet_ptr[2U] = (uint8_t)(idx - 2U);

    /* XOR */
    xorv = 0U;
    for (i = 2U; i < idx; i++) {
        xorv ^= cam_packet_ptr[i];
    }
    cam_packet_ptr[idx++] = xorv;
    cam_packet_ptr[idx++] = CAM_PKT_END;

    *cam_len_ptr = idx;
    return true;
}

/* Read/query translator: map control read into camera query */
static bool Translator_ReadQuery(
    const uint8_t *ctrl_packet_ptr, uint8_t ctrl_len,
    uint8_t *cam_packet_ptr, uint8_t *cam_len_ptr)
{
    uint8_t ctrl_cmd;
    uint8_t cam_query;
    uint8_t idx = 0U;
    uint8_t xorv = 0U;
    uint8_t i;

    if ((ctrl_packet_ptr == NULL) || (cam_packet_ptr == NULL) || (cam_len_ptr == NULL)) {
        return false;
    }

    if (ctrl_len < 8U) {
        return false;
    }

    ctrl_cmd = ctrl_packet_ptr[3U];

    /* map ctrl_cmd to camera query type */
    if (ctrl_cmd == 0x03U) {
        cam_query = 0x10U; /* status */
    } else if (ctrl_cmd == 0x04U) {
        cam_query = 0x20U; /* temperature */
    } else if (ctrl_cmd == 0x05U) {
        cam_query = 0x30U; /* version */
    } else {
        return false;
    }

    /* build camera query packet (example layout) */
    cam_packet_ptr[idx++] = CAM_PKT_START1;
    cam_packet_ptr[idx++] = CAM_PKT_START2;
    cam_packet_ptr[idx++] = 0x07U;   /* LEN */
    cam_packet_ptr[idx++] = 0x02U;   /* CMD1 */
    cam_packet_ptr[idx++] = 0x02U;   /* CMD2 = query */
    cam_packet_ptr[idx++] = cam_query;/* CMD3 = which query */
    /* add dummy payload 4 bytes as in examples */
    cam_packet_ptr[idx++] = 0x00U;
    cam_packet_ptr[idx++] = 0x00U;
    cam_packet_ptr[idx++] = 0x00U;
    cam_packet_ptr[idx++] = 0x00U;

    /* XOR */
    xorv = 0U;
    for (i = 2U; i < idx; i++) {
        xorv ^= cam_packet_ptr[i];
    }
    cam_packet_ptr[idx++] = xorv;
    cam_packet_ptr[idx++] = CAM_PKT_END;

    *cam_len_ptr = idx;
    return true;
}

/* Helper: build simple control response: 55 LEN 00 CMD 33 01 [opt payload] CS EB AA */
static void BuildCtrlResponseHeader(
    uint8_t *buf, uint8_t *pos, uint8_t cmd, uint8_t payload_len)
{
    /* pos points to current write index, start at 0 */
    buf[(*pos)++] = 0x55U;
    buf[(*pos)++] = (uint8_t)(5U + payload_len); /* LEN includes from index1 .. checksum inclusive */
    buf[(*pos)++] = 0x00U;
    buf[(*pos)++] = cmd;
    buf[(*pos)++] = CTRL_PKT_RESP_RESERVE; /* 0x33 */
    /* For set response add fixed 0x01 (ACK) as per spec, for read responses payload will follow */
}

/* Simple ACK response for set commands */
static bool ResponseGen_SimpleACK(
    const uint8_t *cam_resp_ptr, uint8_t cam_len,
    const uint8_t *orig_ctrl_ptr, uint8_t orig_ctrl_len,
    uint8_t *ctrl_resp_ptr, uint8_t *ctrl_resp_len_ptr)
{
    uint8_t pos = 0U;
    uint8_t cmd;


    if ((orig_ctrl_ptr == NULL) || (ctrl_resp_ptr == NULL) || (ctrl_resp_len_ptr == NULL) || (orig_ctrl_len < 4U)) {
        return false;
    }

    cmd = orig_ctrl_ptr[3U];

    /* header */
    BuildCtrlResponseHeader(ctrl_resp_ptr, &pos, cmd, 0U);
    ctrl_resp_ptr[pos++] = CTRL_PKT_RESP_ACK_BYTE; /* 0x01 */
    ctrl_resp_ptr[pos++] = CalculateCtrlChecksum(ctrl_resp_ptr,CONSTANT_PL_FOR_CALC_CS);
    /* end bytes */
    ctrl_resp_ptr[pos++] = CTRL_PKT_END_EB;
    ctrl_resp_ptr[pos++] = CTRL_PKT_END_AA;

    *ctrl_resp_len_ptr = pos;
    return true;
}

/* Echo parameter: return the parameter from original request (payload[0]) */
static bool ResponseGen_EchoParam(
    const uint8_t *cam_resp_ptr, uint8_t cam_len,
    const uint8_t *orig_ctrl_ptr, uint8_t orig_ctrl_len,
    uint8_t *ctrl_resp_ptr, uint8_t *ctrl_resp_len_ptr)
{
    uint8_t pos = 0U;
    uint8_t cmd;
    uint8_t cs;
    uint8_t i;
    uint32_t sum = 0U;
    uint8_t param = 0U;

    if ((orig_ctrl_ptr == NULL) || (ctrl_resp_ptr == NULL) || (ctrl_resp_len_ptr == NULL) || (orig_ctrl_len < 6U)) {
        return false;
    }

    cmd = orig_ctrl_ptr[3U];
    param = orig_ctrl_ptr[5U]; /* echo first payload byte */

    BuildCtrlResponseHeader(ctrl_resp_ptr, &pos, cmd, 1U);
    ctrl_resp_ptr[pos++] = param;

    /* checksum */
    for (i = 1U; i < pos; i++) {
        sum += (uint32_t)ctrl_resp_ptr[i];
    }
    cs = (uint8_t)(sum & 0xFFU);
    ctrl_resp_ptr[pos++] = cs;

    ctrl_resp_ptr[pos++] = CTRL_PKT_END_EB;
    ctrl_resp_ptr[pos++] = CTRL_PKT_END_AA;

    *ctrl_resp_len_ptr = pos;
    return true;
}

/* Multi param response: extract multiple bytes from camera response and convert */
static bool ResponseGen_MultiParam(
    const uint8_t *cam_resp_ptr, uint8_t cam_len,
    const uint8_t *orig_ctrl_ptr, uint8_t orig_ctrl_len,
    uint8_t *ctrl_resp_ptr, uint8_t *ctrl_resp_len_ptr)
{
    uint8_t pos = 0U;
    uint8_t cmd;
    uint8_t cs;
    uint8_t i;
    uint32_t sum = 0U;
    uint8_t b0 = 0U, b1 = 0U, b2 = 0U, b3 = 0U;

    if ((cam_resp_ptr == NULL) || (orig_ctrl_ptr == NULL) || (ctrl_resp_ptr == NULL) || (ctrl_resp_len_ptr == NULL)) {
        return false;
    }

    /* Example: assume camera payload bytes at index 4..7 (depends on cam format) */
    if (cam_len >= 7U) {
        b0 = cam_resp_ptr[4U];
        b1 = (cam_len > 5U) ? cam_resp_ptr[5U] : 0U;
        b2 = (cam_len > 6U) ? cam_resp_ptr[6U] : 0U;
        b3 = (cam_len > 7U) ? cam_resp_ptr[7U] : 0U;
    }

    cmd = orig_ctrl_ptr[3U];

    /* build response with 4-byte payload */
    BuildCtrlResponseHeader(ctrl_resp_ptr, &pos, cmd, 4U);
    ctrl_resp_ptr[pos++] = b0;
    ctrl_resp_ptr[pos++] = b1;
    ctrl_resp_ptr[pos++] = b2;
    ctrl_resp_ptr[pos++] = b3;

    /* checksum */
    for (i = 1U; i < pos; i++) {
        sum += (uint32_t)ctrl_resp_ptr[i];
    }
    cs = (uint8_t)(sum & 0xFFU);
    ctrl_resp_ptr[pos++] = cs;

    ctrl_resp_ptr[pos++] = CTRL_PKT_END_EB;
    ctrl_resp_ptr[pos++] = CTRL_PKT_END_AA;

    *ctrl_resp_len_ptr = pos;
    return true;
}
