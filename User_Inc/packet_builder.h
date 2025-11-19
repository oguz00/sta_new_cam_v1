/**
 * @file packet_builder.h
 * @brief Dynamic packet generator for old and new camera protocol formats
 *
 * This module provides packet building and parsing functions for both
 * old control format and new camera format protocols. It handles:
 * - Automatic checksum calculation for old format
 * - Automatic length field updates
 * - Buffer overflow protection
 * - Type-safe parameter addition
 *
 * @author oguz00
 * @date 2025-11-17
 * @version 1.0
 *
 * @par MISRA-C:2012 Compliance:
 * This file complies with MISRA-C:2012 guidelines:
 * - Rule 8.2: Function prototypes with named parameters
 * - Rule 8.4: All declarations have complete prototypes
 * - Rule 21.1: No reserved identifiers used
 *
 * @par Old Format Structure:
 * @code
 * [START] [LENGTH] [RESERVED] [CMD_ID] [DATA...] [CS_HIGH] [CS_LOW] [END]
 * @endcode
 *
 * @par New Format Structure:
 * @code
 * [0x55] [0xAA] [LENGTH] [TYPE] [DATA...] [0xF0]
 * @endcode
 */

#ifndef PACKET_BUILDER_H
#define PACKET_BUILDER_H

/*============================================================================*/
/* Includes                                                                   */
/*============================================================================*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/*============================================================================*/
/* Constants and Macros                                                       */
/*============================================================================*/

/**
 * @brief Maximum size of a packet buffer in bytes
 * @note MISRA-C Rule 2.5: Macro has a descriptive name
 */
#define PACKET_BUILDER_MAX_SIZE  (32U)

/*============================================================================*/
/* Type Definitions                                                           */
/*============================================================================*/

/**
 * @brief Old packet direction type
 *
 * Defines whether the packet is transmitted to or received from control
 *
 * @note MISRA-C Rule 8.12: Enum has complete type specification
 */
typedef enum {
    OLD_PACKET_TX = 0xAAU,  /**< Packet transmitted to control (response) */
    OLD_PACKET_RX = 0x55U   /**< Packet received from control (request) */
} OldPacketType_t;

/**
 * @brief Old format packet builder structure
 *
 * Maintains state during packet construction. All fields are managed
 * internally - users should only call the API functions.
 *
 * @note MISRA-C Rule 8.11: Complete type definition provided
 */
typedef struct {
    uint8_t buffer[PACKET_BUILDER_MAX_SIZE]; /**< Internal packet buffer */
    uint8_t length;                          /**< Total packet length (including header/footer) */
    uint8_t data_length;                     /**< Length of data section only (for LENGTH field) */
} OldPacket_t;

/**
 * @brief New format packet builder structure
 *
 * Maintains state during packet construction for new protocol format.
 */
typedef struct {
    uint8_t buffer[PACKET_BUILDER_MAX_SIZE]; /**< Internal packet buffer */
    uint8_t length;                          /**< Total packet length */
} NewPacket_t;

/*============================================================================*/
/* Old Format Packet Generator API                                           */
/*============================================================================*/

/**
 * @brief Initialize an old format packet builder
 *
 * Sets up the packet header with start byte, type, and command ID.
 * Must be called before adding any data.
 *
 * @param[out] packet_ptr   Pointer to packet builder structure (must not be NULL)
 * @param[in]  packet_type  Packet direction (TX or RX)
 * @param[in]  cmd_id       Command ID byte
 *
 * @return void
 *
 * @pre packet_ptr must not be NULL
 * @post packet is initialized with header: [type][0][0][cmd_id]
 *
 * @note MISRA-C Rule 8.2: Named parameters for clarity
 * @note After initialization, packet contains: [type] [0x00] [0x00] [cmd_id]
 * @warning Caller must ensure packet_ptr is valid
 *
 * @par Example:
 * @code
 * OldPacket_t pkt;
 * OldPacket_Init(&pkt, OLD_PACKET_TX, 0x10U);  // Zoom command response
 * @endcode
 */
void OldPacket_Init(
    OldPacket_t *packet_ptr,
    OldPacketType_t packet_type,
    uint8_t cmd_id
);

/**
 * @brief Add a single byte to the packet data section
 *
 * @param[in,out] packet_ptr  Pointer to packet builder (must not be NULL)
 * @param[in]     data_byte   Byte to add
 *
 * @return true if byte added successfully, false if buffer full or NULL pointer
 *
 * @pre packet_ptr must not be NULL
 * @pre packet must have been initialized with OldPacket_Init
 * @post If true returned, data_byte is added and length incremented
 *
 * @note MISRA-C Rule 8.7: Functions used externally have external linkage
 * @note Automatically increments data_length counter
 * @warning Reserves space for checksum (2 bytes) and end byte (1 byte)
 */
bool OldPacket_AddByte(
    OldPacket_t *packet_ptr,
    uint8_t data_byte
);

/**
 * @brief Add a 16-bit unsigned integer (little-endian)
 *
 * @param[in,out] packet_ptr  Pointer to packet builder (must not be NULL)
 * @param[in]     data_u16    16-bit value to add
 *
 * @return true if added successfully, false if insufficient space or NULL pointer
 *
 * @pre packet_ptr must not be NULL
 * @post Value stored as [LOW_BYTE][HIGH_BYTE]
 *
 * @note Value is stored as [LOW_BYTE] [HIGH_BYTE] (little-endian)
 *
 * @par Example:
 * @code
 * bool result = OldPacket_AddUint16(&pkt, 0x1234U);  // Adds: 0x34 0x12
 * if (result) {
 *     // Success
 * }
 * @endcode
 */
bool OldPacket_AddUint16(
    OldPacket_t *packet_ptr,
    uint16_t data_u16
);

/**
 * @brief Add a 32-bit unsigned integer (little-endian)
 *
 * @param[in,out] packet_ptr  Pointer to packet builder (must not be NULL)
 * @param[in]     data_u32    32-bit value to add
 *
 * @return true if added successfully, false if insufficient space or NULL pointer
 *
 * @pre packet_ptr must not be NULL
 * @post Value stored as [BYTE0][BYTE1][BYTE2][BYTE3]
 *
 * @note Value is stored as [BYTE0] [BYTE1] [BYTE2] [BYTE3] (little-endian)
 */
bool OldPacket_AddUint32(
    OldPacket_t *packet_ptr,
    uint32_t data_u32
);

/**
 * @brief Add multiple bytes from an array
 *
 * @param[in,out] packet_ptr  Pointer to packet builder (must not be NULL)
 * @param[in]     data_ptr    Pointer to byte array (must not be NULL)
 * @param[in]     data_len    Number of bytes to add
 *
 * @return true if all bytes added, false if insufficient space or NULL pointer
 *
 * @pre packet_ptr and data_ptr must not be NULL
 * @pre data_len must be > 0
 * @post If true, all bytes from data_ptr are copied
 *
 * @note If return is false, partial data may have been added
 */
bool OldPacket_AddBytes(
    OldPacket_t *packet_ptr,
    const uint8_t *data_ptr,
    uint8_t data_len
);

/**
 * @brief Finalize packet by adding checksum and end byte
 *
 * This function MUST be called after all data is added. It:
 * 1. Updates the LENGTH field (buffer[1])
 * 2. Calculates and appends checksum (2 bytes, big-endian)
 * 3. Appends end byte (0xAA)
 *
 * @param[in,out] packet_ptr  Pointer to packet builder (must not be NULL)
 *
 * @return true if finalized successfully, false if buffer overflow or NULL pointer
 *
 * @pre packet_ptr must not be NULL
 * @pre packet must contain at least header (4 bytes)
 * @post After success, no more data can be added
 *
 * @note After finalization, no more data can be added
 * @warning Always call GetBuffer/GetLength only AFTER Finalize
 *
 * @par Checksum Algorithm:
 * Sum of all bytes from start to end of data (before checksum)
 */
bool OldPacket_Finalize(
    OldPacket_t *packet_ptr
);

/**
 * @brief Get pointer to finalized packet buffer
 *
 * @param[in] packet_ptr  Pointer to finalized packet (must not be NULL)
 *
 * @return Pointer to internal buffer, or NULL if packet_ptr is NULL
 *
 * @pre packet_ptr must not be NULL
 * @pre OldPacket_Finalize must have been called
 *
 * @warning Only call after OldPacket_Finalize()
 * @note Returned pointer is valid until packet structure is destroyed
 * @note MISRA-C Rule 8.13: Pointer declared const as content not modified
 */
const uint8_t* OldPacket_GetBuffer(
    const OldPacket_t *packet_ptr
);

/**
 * @brief Get total length of finalized packet
 *
 * @param[in] packet_ptr  Pointer to finalized packet (must not be NULL)
 *
 * @return Total packet length in bytes, or 0 if packet_ptr is NULL
 *
 * @pre packet_ptr must not be NULL
 * @pre OldPacket_Finalize must have been called
 *
 * @warning Only call after OldPacket_Finalize()
 */
uint8_t OldPacket_GetLength(
    const OldPacket_t *packet_ptr
);

/*============================================================================*/
/* New Format Packet Generator API                                           */
/*============================================================================*/

/**
 * @brief Initialize a new format packet builder
 *
 * Sets up packet header: [0x55] [0xAA] [0x00] [type]
 *
 * @param[out] packet_ptr   Pointer to packet builder structure (must not be NULL)
 * @param[in]  packet_type  Command type byte
 *
 * @return void
 *
 * @pre packet_ptr must not be NULL
 * @post packet initialized with header [0x55][0xAA][0][type]
 *
 * @par Common Types:
 * - 0x01: Control command
 * - 0x02: Configuration command
 * - 0x03: Query command
 * - 0x00: Response/ACK
 */
void NewPacket_Init(
    NewPacket_t *packet_ptr,
    uint8_t packet_type
);

/**
 * @brief Add a single byte to new format packet
 *
 * @param[in,out] packet_ptr  Pointer to packet builder (must not be NULL)
 * @param[in]     data_byte   Byte to add
 *
 * @return true if added successfully, false if buffer full or NULL pointer
 *
 * @pre packet_ptr must not be NULL
 * @post If true, byte added and length incremented
 *
 * @note Reserves space for end byte (0xF0)
 */
bool NewPacket_AddByte(
    NewPacket_t *packet_ptr,
    uint8_t data_byte
);

/**
 * @brief Add a 16-bit value (little-endian) to new format packet
 *
 * @param[in,out] packet_ptr  Pointer to packet builder (must not be NULL)
 * @param[in]     data_u16    16-bit value to add
 *
 * @return true if added successfully, false if insufficient space or NULL pointer
 *
 * @pre packet_ptr must not be NULL
 * @post Value stored as [LOW_BYTE][HIGH_BYTE]
 */
bool NewPacket_AddUint16(
    NewPacket_t *packet_ptr,
    uint16_t data_u16
);

/**
 * @brief Add byte array to new format packet
 *
 * @param[in,out] packet_ptr  Pointer to packet builder (must not be NULL)
 * @param[in]     data_ptr    Pointer to byte array (must not be NULL)
 * @param[in]     data_len    Number of bytes to add
 *
 * @return true if all bytes added, false if insufficient space or NULL pointer
 *
 * @pre packet_ptr and data_ptr must not be NULL
 * @pre data_len must be > 0
 */
bool NewPacket_AddBytes(
    NewPacket_t *packet_ptr,
    const uint8_t *data_ptr,
    uint8_t data_len
);

/**
 * @brief Finalize new format packet
 *
 * This function:
 * 1. Appends end byte (0xF0)
 * 2. Updates LENGTH field (buffer[2]) with total packet length
 *
 * @param[in,out] packet_ptr  Pointer to packet builder (must not be NULL)
 *
 * @return true if finalized successfully, false if buffer overflow or NULL pointer
 *
 * @pre packet_ptr must not be NULL
 * @pre packet must contain at least header (4 bytes)
 * @post After success, packet is complete and ready to send
 *
 * @note No checksum is used in new format - end byte is sufficient
 */
bool NewPacket_Finalize(
    NewPacket_t *packet_ptr
);

/**
 * @brief Get pointer to finalized new format packet buffer
 *
 * @param[in] packet_ptr  Pointer to finalized packet (must not be NULL)
 *
 * @return Pointer to internal buffer, or NULL if packet_ptr is NULL
 *
 * @pre packet_ptr must not be NULL
 * @pre NewPacket_Finalize must have been called
 *
 * @warning Only call after NewPacket_Finalize()
 */
const uint8_t* NewPacket_GetBuffer(
    const NewPacket_t *packet_ptr
);

/**
 * @brief Get total length of finalized new format packet
 *
 * @param[in] packet_ptr  Pointer to finalized packet (must not be NULL)
 *
 * @return Total packet length in bytes, or 0 if packet_ptr is NULL
 *
 * @pre packet_ptr must not be NULL
 * @pre NewPacket_Finalize must have been called
 *
 * @warning Only call after NewPacket_Finalize()
 */
uint8_t NewPacket_GetLength(
    const NewPacket_t *packet_ptr
);

/*============================================================================*/
/* Packet Parsing Helper Functions                                           */
/*============================================================================*/

/**
 * @brief Extract command ID from old format packet
 *
 * @param[in] packet_ptr  Pointer to received packet buffer (must not be NULL)
 *
 * @return Command ID byte (packet[3]), or 0 if packet_ptr is NULL
 *
 * @pre packet_ptr must point to valid packet buffer
 * @post Return value is packet[3]
 *
 * @note Does not validate packet integrity - use VerifyPacket first
 */
uint8_t OldPacket_GetCommandId(
    const uint8_t *packet_ptr
);

/**
 * @brief Extract a byte from old format packet data section
 *
 * @param[in]  packet_ptr  Pointer to packet buffer (must not be NULL)
 * @param[in]  offset      Offset within data section (0-based)
 * @param[out] value_ptr   Pointer to store extracted byte (must not be NULL)
 *
 * @return true if extracted successfully, false if NULL pointer
 *
 * @pre packet_ptr and value_ptr must not be NULL
 * @post If true, *value_ptr contains extracted byte
 *
 * @note Data section starts at packet[4]
 * @warning Does not check if offset is within packet bounds
 *
 * @par Example:
 * @code
 * uint8_t zoom;
 * bool result = OldPacket_GetByte(rx_packet, 0U, &zoom);
 * if (result) {
 *     // Use zoom value
 * }
 * @endcode
 */
bool OldPacket_GetByte(
    const uint8_t *packet_ptr,
    uint8_t offset,
    uint8_t *value_ptr
);

/**
 * @brief Extract a 16-bit value from old format packet (little-endian)
 *
 * @param[in]  packet_ptr  Pointer to packet buffer (must not be NULL)
 * @param[in]  offset      Offset within data section (0-based)
 * @param[out] value_ptr   Pointer to store extracted 16-bit value (must not be NULL)
 *
 * @return true if extracted successfully, false if NULL pointer
 *
 * @pre packet_ptr and value_ptr must not be NULL
 * @post If true, *value_ptr contains 16-bit value in host byte order
 *
 * @note Reads [LOW_BYTE] [HIGH_BYTE] at offset
 */
bool OldPacket_GetUint16(
    const uint8_t *packet_ptr,
    uint8_t offset,
    uint16_t *value_ptr
);

/**
 * @brief Extract a byte from new format packet data section
 *
 * @param[in]  packet_ptr  Pointer to packet buffer (must not be NULL)
 * @param[in]  offset      Offset within data section (0-based)
 * @param[out] value_ptr   Pointer to store extracted byte (must not be NULL)
 *
 * @return true if extracted successfully, false if NULL pointer
 *
 * @pre packet_ptr and value_ptr must not be NULL
 * @post If true, *value_ptr contains extracted byte
 *
 * @note Data section starts at packet[4]
 */
bool NewPacket_GetByte(
    const uint8_t *packet_ptr,
    uint8_t offset,
    uint8_t *value_ptr
);

/**
 * @brief Extract a 16-bit value from new format packet (little-endian)
 *
 * @param[in]  packet_ptr  Pointer to packet buffer (must not be NULL)
 * @param[in]  offset      Offset within data section (0-based)
 * @param[out] value_ptr   Pointer to store extracted 16-bit value (must not be NULL)
 *
 * @return true if extracted successfully, false if NULL pointer
 *
 * @pre packet_ptr and value_ptr must not be NULL
 * @post If true, *value_ptr contains 16-bit value in host byte order
 */
bool NewPacket_GetUint16(
    const uint8_t *packet_ptr,
    uint8_t offset,
    uint16_t *value_ptr
);

#endif /* PACKET_BUILDER_H */

/* End of packet_builder.h */
