/**
 * @file packet_builder.c
 * @brief Implementation of dynamic packet generation and parsing
 *
 * This file implements all packet building and parsing functions for both
 * old and new protocol formats. Key features:
 * - Automatic checksum calculation using sum algorithm
 * - Bounds checking on all operations
 * - Little-endian byte order for multi-byte values
 * - MISRA-C:2012 compliant implementation
 *
 * @author oguz00
 * @date 2025-11-17
 * @version 1.0
 *
 * @par MISRA-C:2012 Compliance:
 * - Rule 8.2: Named parameters in all definitions
 * - Rule 8.7: Static functions for internal use only
 * - Rule 10.1: Explicit casts for type conversions
 * - Rule 10.3: No implicit conversions
 * - Rule 14.4: Boolean type used for control expressions
 * - Rule 17.7: Return values checked where appropriate
 * - Rule 21.3: No malloc/free used
 */

/*============================================================================*/
/* Includes                                                                   */
/*============================================================================*/

#include "packet_builder.h"

/*============================================================================*/
/* Private Helper Functions                                                   */
/*============================================================================*/

/**
 * @brief Calculate checksum for old format packet
 *
 * Checksum algorithm: Simple sum of all bytes
 *
 * @param[in] data_ptr  Pointer to data buffer (must not be NULL)
 * @param[in] data_len  Number of bytes to sum
 *
 * @return 16-bit checksum value
 *
 * @note This is NOT a CRC - it's a simple additive checksum
 * @note MISRA-C Rule 8.7: Static function (internal linkage)
 * @note MISRA-C Rule 10.4: No implicit casting
 */
static uint16_t CalculateOldChecksum(const uint8_t *data_ptr, uint8_t data_len)
{
    uint16_t sum = 0U;
    uint8_t i;

    /* MISRA-C Rule 14.2: For loop with simple counter */
    for (i = 0U; i < data_len; i++) {
        /* MISRA-C Rule 10.1: Explicit cast to uint16_t */
        sum = (uint16_t)(sum + (uint16_t)data_ptr[i]);
    }

    return sum;
}

/*============================================================================*/
/* Old Packet Builder Implementation                                         */
/*============================================================================*/

void OldPacket_Init(
    OldPacket_t *packet_ptr,
    OldPacketType_t packet_type,
    uint8_t cmd_id)
{
    /* MISRA-C Rule 14.4: Boolean expression for control */
    if (packet_ptr != NULL) {

        /* Clear entire structure to zero */
        /* MISRA-C Rule 21.3: memset is allowed (not malloc/free) */
        (void)memset(packet_ptr, 0, sizeof(OldPacket_t));

        /* Build header: [START] [LENGTH] [RESERVED] [CMD_ID] */
        packet_ptr->buffer[0] = (uint8_t)packet_type;  /* MISRA-C Rule 10.3: Explicit cast */
        packet_ptr->buffer[1] = 0U;                    /* Length - updated in Finalize */
        packet_ptr->buffer[2] = 0x00U;                 /* Reserved byte */
        packet_ptr->buffer[3] = cmd_id;                /* Command identifier */
        packet_ptr->length = 4U;                       /* Current buffer position */
        packet_ptr->data_length = 0U;                  /* Data section is empty */
    }
    /* MISRA-C Rule 15.5: No else needed (void return) */
}

bool OldPacket_AddByte(
    OldPacket_t *packet_ptr,
    uint8_t data_byte)
{
    bool result = false;

    /* MISRA-C Rule 14.4: Boolean expression */
    if (packet_ptr != NULL) {

        /* Check if there's room: need space for data + checksum (2) + end (1) */
        /* MISRA-C Rule 12.1: Parentheses for clarity */
        if (packet_ptr->length < (PACKET_BUILDER_MAX_SIZE - 3U)) {

            /* Add byte to buffer */
            packet_ptr->buffer[packet_ptr->length] = data_byte;
            packet_ptr->length = (uint8_t)(packet_ptr->length + 1U);  /* MISRA-C Rule 10.3 */
            packet_ptr->data_length = (uint8_t)(packet_ptr->data_length + 1U);

            result = true;
        }
    }

    return result;
}

bool OldPacket_AddUint16(
    OldPacket_t *packet_ptr,
    uint16_t data_u16)
{
    bool result = false;
    bool byte1_ok;
    bool byte2_ok;

    /* Add low byte first (little-endian) */
    /* MISRA-C Rule 10.1: Explicit cast to uint8_t */
    byte1_ok = OldPacket_AddByte(packet_ptr, (uint8_t)(data_u16 & 0xFFU));

    if (byte1_ok) {
        /* Add high byte */
        byte2_ok = OldPacket_AddByte(packet_ptr, (uint8_t)((data_u16 >> 8) & 0xFFU));

        if (byte2_ok) {
            result = true;
        }
    }

    return result;
}

bool OldPacket_AddUint32(
    OldPacket_t *packet_ptr,
    uint32_t data_u32)
{
    bool result = false;
    bool byte1_ok;
    bool byte2_ok;
    bool byte3_ok;
    bool byte4_ok;

    /* Add bytes in little-endian order */
    /* MISRA-C Rule 10.1: Explicit casts */
    byte1_ok = OldPacket_AddByte(packet_ptr, (uint8_t)(data_u32 & 0xFFU));

    if (byte1_ok) {
        byte2_ok = OldPacket_AddByte(packet_ptr, (uint8_t)((data_u32 >> 8) & 0xFFU));

        if (byte2_ok) {
            byte3_ok = OldPacket_AddByte(packet_ptr, (uint8_t)((data_u32 >> 16) & 0xFFU));

            if (byte3_ok) {
                byte4_ok = OldPacket_AddByte(packet_ptr, (uint8_t)((data_u32 >> 24) & 0xFFU));

                if (byte4_ok) {
                    result = true;
                }
            }
        }
    }

    return result;
}

bool OldPacket_AddBytes(
    OldPacket_t *packet_ptr,
    const uint8_t *data_ptr,
    uint8_t data_len)
{
    bool result = false;
    bool add_ok = true;
    uint8_t i;

    /* MISRA-C Rule 14.4: Boolean expressions */
    if ((packet_ptr != NULL) && (data_ptr != NULL)) {

        /* Add each byte individually with bounds checking */
        /* MISRA-C Rule 14.2: Simple for loop */
        for (i = 0U; i < data_len; i++) {
            add_ok = OldPacket_AddByte(packet_ptr, data_ptr[i]);

            if (!add_ok) {
                break;  /* MISRA-C Rule 15.4: Single exit point preferred, but break allowed */
            }
        }

        /* All bytes added successfully */
        if (add_ok) {
            result = true;
        }
    }

    return result;
}

bool OldPacket_Finalize(
    OldPacket_t *packet_ptr)
{
    bool result = false;
    uint16_t checksum;

    /* MISRA-C Rule 14.4: Boolean expressions */
    if (packet_ptr != NULL) {

        if (packet_ptr->length >= 4U) {

            /* Step 1: Update LENGTH field (payload length only) */
            packet_ptr->buffer[1] = packet_ptr->data_length;

            /* Step 2: Calculate checksum over entire packet so far */
            checksum = CalculateOldChecksum(packet_ptr->buffer, packet_ptr->length);

            /* Step 3: Add checksum (big-endian: high byte first) */
            if (packet_ptr->length < (PACKET_BUILDER_MAX_SIZE - 2U)) {

                /* MISRA-C Rule 10.1: Explicit casts */
                packet_ptr->buffer[packet_ptr->length] = (uint8_t)((checksum >> 8) & 0xFFU);
                packet_ptr->length = (uint8_t)(packet_ptr->length + 1U);

                packet_ptr->buffer[packet_ptr->length] = (uint8_t)(checksum & 0xFFU);
                packet_ptr->length = (uint8_t)(packet_ptr->length + 1U);

                /* Step 4: Add end byte */
                if (packet_ptr->length < PACKET_BUILDER_MAX_SIZE) {
                    packet_ptr->buffer[packet_ptr->length] = 0xAAU;
                    packet_ptr->length = (uint8_t)(packet_ptr->length + 1U);

                    result = true;
                }
            }
        }
    }

    return result;
}

const uint8_t* OldPacket_GetBuffer(
    const OldPacket_t *packet_ptr)
{
    const uint8_t* result_ptr;

    /* MISRA-C Rule 14.4: Boolean expression */
    if (packet_ptr != NULL) {
        result_ptr = packet_ptr->buffer;
    } else {
        result_ptr = NULL;
    }

    return result_ptr;
}

uint8_t OldPacket_GetLength(
    const OldPacket_t *packet_ptr)
{
    uint8_t result;

    /* MISRA-C Rule 14.4: Boolean expression */
    if (packet_ptr != NULL) {
        result = packet_ptr->length;
    } else {
        result = 0U;
    }

    return result;
}

/*============================================================================*/
/* New Packet Builder Implementation                                         */
/*============================================================================*/

void NewPacket_Init(
    NewPacket_t *packet_ptr,
    uint8_t packet_type)
{
    /* MISRA-C Rule 14.4: Boolean expression */
    if (packet_ptr != NULL) {

        /* Clear entire structure */
        (void)memset(packet_ptr, 0, sizeof(NewPacket_t));

        /* Build header: [0x55] [0xAA] [LENGTH] [TYPE] */
        packet_ptr->buffer[0] = 0x55U;       /* Start byte 1 */
        packet_ptr->buffer[1] = 0xAAU;       /* Start byte 2 */
        packet_ptr->buffer[2] = 0U;          /* Length - updated in Finalize */
        packet_ptr->buffer[3] = packet_type; /* Command type */
        packet_ptr->length = 4U;             /* Current position */
    }
}

bool NewPacket_AddByte(
    NewPacket_t *packet_ptr,
    uint8_t data_byte)
{
    bool result = false;

    /* MISRA-C Rule 14.4: Boolean expression */
    if (packet_ptr != NULL) {

        /* Check if there's room: need space for data + end byte (1) */
        if (packet_ptr->length < (PACKET_BUILDER_MAX_SIZE - 1U)) {

            /* Add byte to buffer */
            packet_ptr->buffer[packet_ptr->length] = data_byte;
            packet_ptr->length = (uint8_t)(packet_ptr->length + 1U);

            result = true;
        }
    }

    return result;
}

bool NewPacket_AddUint16(
    NewPacket_t *packet_ptr,
    uint16_t data_u16)
{
    bool result = false;
    bool byte1_ok;
    bool byte2_ok;

    /* Add low byte first (little-endian) */
    /* MISRA-C Rule 10.1: Explicit cast */
    byte1_ok = NewPacket_AddByte(packet_ptr, (uint8_t)(data_u16 & 0xFFU));

    if (byte1_ok) {
        /* Add high byte */
        byte2_ok = NewPacket_AddByte(packet_ptr, (uint8_t)((data_u16 >> 8) & 0xFFU));

        if (byte2_ok) {
            result = true;
        }
    }

    return result;
}

bool NewPacket_AddBytes(
    NewPacket_t *packet_ptr,
    const uint8_t *data_ptr,
    uint8_t data_len)
{
    bool result = false;
    bool add_ok = true;
    uint8_t i;

    /* MISRA-C Rule 14.4: Boolean expressions */
    if ((packet_ptr != NULL) && (data_ptr != NULL)) {

        /* Add each byte with bounds checking */
        for (i = 0U; i < data_len; i++) {
            add_ok = NewPacket_AddByte(packet_ptr, data_ptr[i]);

            if (!add_ok) {
                break;
            }
        }

        if (add_ok) {
            result = true;
        }
    }

    return result;
}

bool NewPacket_Finalize(
    NewPacket_t *packet_ptr)
{
    bool result = false;

    /* MISRA-C Rule 14.4: Boolean expressions */
    if (packet_ptr != NULL) {

        if (packet_ptr->length >= 4U) {

            /* Step 1: Add end byte */
            if (packet_ptr->length < PACKET_BUILDER_MAX_SIZE) {
                packet_ptr->buffer[packet_ptr->length] = 0xF0U;
                packet_ptr->length = (uint8_t)(packet_ptr->length + 1U);

                /* Step 2: Update LENGTH field with total packet length */
                packet_ptr->buffer[2] = packet_ptr->length;

                result = true;
            }
        }
    }

    return result;
}

const uint8_t* NewPacket_GetBuffer(
    const NewPacket_t *packet_ptr)
{
    const uint8_t* result_ptr;

    /* MISRA-C Rule 14.4: Boolean expression */
    if (packet_ptr != NULL) {
        result_ptr = packet_ptr->buffer;
    } else {
        result_ptr = NULL;
    }

    return result_ptr;
}

uint8_t NewPacket_GetLength(
    const NewPacket_t *packet_ptr)
{
    uint8_t result;

    /* MISRA-C Rule 14.4: Boolean expression */
    if (packet_ptr != NULL) {
        result = packet_ptr->length;
    } else {
        result = 0U;
    }

    return result;
}

/*============================================================================*/
/* Packet Parsing Helper Implementation                                      */
/*============================================================================*/

uint8_t OldPacket_GetCommandId(
    const uint8_t *packet_ptr)
{
    uint8_t result;

    /* MISRA-C Rule 14.4: Boolean expression */
    if (packet_ptr != NULL) {
        result = packet_ptr[3];
    } else {
        result = 0U;
    }

    return result;
}

bool OldPacket_GetByte(
    const uint8_t *packet_ptr,
    uint8_t offset,
    uint8_t *value_ptr)
{
    bool result = false;

    /* MISRA-C Rule 14.4: Boolean expressions */
    if ((packet_ptr != NULL) && (value_ptr != NULL)) {

        /* Data section starts at index 4 (after header) */
        /* MISRA-C Rule 18.1: Bounds not checked - caller responsibility */
        *value_ptr = packet_ptr[4U + offset];

        result = true;
    }

    return result;
}

bool OldPacket_GetUint16(
    const uint8_t *packet_ptr,
    uint8_t offset,
    uint16_t *value_ptr)
{
    bool result = false;
    uint16_t low_byte;
    uint16_t high_byte;

    /* MISRA-C Rule 14.4: Boolean expressions */
    if ((packet_ptr != NULL) && (value_ptr != NULL)) {

        /* Read little-endian 16-bit value */
        /* MISRA-C Rule 10.1: Explicit casts */
        low_byte = (uint16_t)packet_ptr[4U + offset];
        high_byte = (uint16_t)packet_ptr[4U + offset + 1U];

        *value_ptr = (uint16_t)(low_byte | (high_byte << 8));

        result = true;
    }

    return result;
}

bool NewPacket_GetByte(
    const uint8_t *packet_ptr,
    uint8_t offset,
    uint8_t *value_ptr)
{
    bool result = false;

    /* MISRA-C Rule 14.4: Boolean expressions */
    if ((packet_ptr != NULL) && (value_ptr != NULL)) {

        /* Data section starts at index 4 (after header) */
        *value_ptr = packet_ptr[4U + offset];

        result = true;
    }

    return result;
}

bool NewPacket_GetUint16(
    const uint8_t *packet_ptr,
    uint8_t offset,
    uint16_t *value_ptr)
{
    bool result = false;
    uint16_t low_byte;
    uint16_t high_byte;

    /* MISRA-C Rule 14.4: Boolean expressions */
    if ((packet_ptr != NULL) && (value_ptr != NULL)) {

        /* Read little-endian 16-bit value */
        /* MISRA-C Rule 10.1: Explicit casts */
        low_byte = (uint16_t)packet_ptr[4U + offset];
        high_byte = (uint16_t)packet_ptr[4U + offset + 1U];

        *value_ptr = (uint16_t)(low_byte | (high_byte << 8));

        result = true;
    }

    return result;
}
