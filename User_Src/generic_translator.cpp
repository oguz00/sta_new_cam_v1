/**
 * @file generic_translator.c
 * @brief Metadata-driven generic translator
 *
 * Use: create CommandMetadata_t entries per command, then call
 * GenericTranslator_FromMetadata(meta, ctrl_pkt, ctrl_len, cam_buf, &cam_len).
 *
 * This produces camera packet: 55 AA [LEN] B1 B2 B3 [payload..] [XOR] F0
 */

#include "commands_metadata.h"
#include "command_handler.h" /* for CAM_PKT_START*, CAM_PKT_END definitions */
#include <string.h>
#include <stdint.h>
#include <stdbool.h>



/* Helper: write camera header */
static uint8_t write_cam_header(uint8_t *buf, uint8_t b1, uint8_t b2, uint8_t b3)
{
    uint8_t idx = 0U;
    buf[idx++] = CAM_PKT_START1; /* 0x55 */
    buf[idx++] = CAM_PKT_START2; /* 0xAA */
    buf[idx++] = 0x00U;          /* LEN placeholder */
    buf[idx++] = b1;
    buf[idx++] = b2;
    buf[idx++] = b3;
    return idx; /* current index */
}

/* Helper: scale 8-bit value (simple integer scaling) */
static uint8_t scale8(uint8_t v, uint8_t mul, uint8_t div)
{
//    if (div == 0U) { return v; }
//    uint32_t r = (uint32_t)v * (uint32_t)mul;
//    r = r / (uint32_t)div;
//    if (r > 0xFFU) { r = 0xFFU; }
//    return (uint8_t)r;
}

/* Generic translator implementation */
bool GenericTranslator_FromMetadata(
    const CommandMetadata_t *meta,
    const uint8_t *ctrl_packet,
    uint8_t ctrl_len,
    uint8_t *cam_packet,
    uint8_t *cam_len)
{
    if ((meta == NULL) || (ctrl_packet == NULL) || (cam_packet == NULL) || (cam_len == NULL)) {
        return false;
    }

    uint8_t idx = write_cam_header(cam_packet, meta->cam_b1, meta->cam_b2, meta->cam_b3);

    /* Apply payload steps */
    for (uint8_t si = 0U; si < meta->step_count; si++) {
        const payload_step_t *step = &meta->steps[si];

        switch (step->action) {
        case PAYACT_NOP:
            /* nothing */
            break;

        case PAYACT_CONST:
            /* write 'length' copies of const_value */
            for (uint8_t k = 0U; k < step->length; k++) {
                cam_packet[idx++] = step->const_value;
            }
            break;

        case PAYACT_COPY:
            /* copy length bytes from ctrl_packet[src_offset] */
            if ((uint32_t)step->src_offset + step->length > (uint32_t)ctrl_len) {
                /* out-of-bounds */
                return false;
            }
            memcpy(&cam_packet[idx], &ctrl_packet[step->src_offset], step->length);
            idx = (uint8_t)(idx + step->length);
            break;

        case PAYACT_SCALE8:
            /* read one byte from ctrl, scale it, write length bytes (usually 1) */
            if ((uint32_t)step->src_offset >= (uint32_t)ctrl_len) { return false; }
            {
                uint8_t v = ctrl_packet[step->src_offset];
                uint8_t out = scale8(v, step->scale_mul, step->scale_div);
                for (uint8_t k = 0U; k < step->length; k++) {
                    cam_packet[idx++] = out;
                }
            }
            break;

        case PAYACT_LOOKUP:
            /* read one index and lookup */
            if ((uint32_t)step->src_offset >= (uint32_t)ctrl_len) { return false; }
            {
                uint8_t idx_in = ctrl_packet[step->src_offset];
                if (idx_in >= step->lookup_len) { return false; }
                uint8_t outv = step->lookup[idx_in];
                for (uint8_t k = 0U; k < step->length; k++) {
                    cam_packet[idx++] = outv;
                }
            }
            break;

        default:
            return false;
        }
    }

/* Fill LEN field: number of bytes from index 2 up to and including checksum */
	if (idx < 3U) { return false; }
	cam_packet[2] = (uint8_t)(idx - 2U);

	/* Compute XOR over bytes index 2..idx-1 */
	uint8_t xorv = 0U;
	for (uint8_t i = 2U; i < idx; i++) { xorv ^= cam_packet[i]; }

	cam_packet[idx++] = xorv;
	cam_packet[idx++] = CAM_PKT_END;

	*cam_len = idx;
	return true;
}


























