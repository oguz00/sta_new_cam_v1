/**
 * @file packet_builder.h
 * @brief Packet generator functions for old and new formats
 * @author oguz00
 * @date 2025-11-16
 */

#ifndef PACKET_BUILDER_H
#define PACKET_BUILDER_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_PACKET_SIZE 32

/**
 * @brief Old packet type (TX or RX)
 */
typedef enum {
    OLD_PACKET_TX = 0xAA,  // Sent to control
    OLD_PACKET_RX = 0x55   // Received from control
} OldPacketType_t;

/**
 * @brief Old packet builder structure
 */
typedef struct {
    uint8_t buffer[MAX_PACKET_SIZE];
    uint8_t length;
    uint8_t data_length;  // Length of data section only
} OldPacket_t;

/**
 * @brief New packet builder structure
 */
typedef struct {
    uint8_t buffer[MAX_PACKET_SIZE];
    uint8_t length;
} NewPacket_t;

/* ========== Old Format Packet Generator ========== */

void OldPacket_Init(OldPacket_t *packet, OldPacketType_t type, uint8_t cmd_id);
bool OldPacket_AddByte(OldPacket_t *packet, uint8_t data);
bool OldPacket_AddUint16(OldPacket_t *packet, uint16_t data);
bool OldPacket_AddUint32(OldPacket_t *packet, uint32_t data);
bool OldPacket_AddBytes(OldPacket_t *packet, const uint8_t *data, uint8_t len);
bool OldPacket_Finalize(OldPacket_t *packet);
const uint8_t* OldPacket_GetBuffer(const OldPacket_t *packet);
uint8_t OldPacket_GetLength(const OldPacket_t *packet);

/* ========== New Format Packet Generator ========== */

void NewPacket_Init(NewPacket_t *packet, uint8_t type);
bool NewPacket_AddByte(NewPacket_t *packet, uint8_t data);
bool NewPacket_AddUint16(NewPacket_t *packet, uint16_t data);
bool NewPacket_AddBytes(NewPacket_t *packet, const uint8_t *data, uint8_t len);
bool NewPacket_Finalize(NewPacket_t *packet);
const uint8_t* NewPacket_GetBuffer(const NewPacket_t *packet);
uint8_t NewPacket_GetLength(const NewPacket_t *packet);

/* ========== Packet Parsing Helpers ========== */

uint8_t OldPacket_GetCommandId(const uint8_t *packet);
bool OldPacket_GetByte(const uint8_t *packet, uint8_t offset, uint8_t *value);
bool OldPacket_GetUint16(const uint8_t *packet, uint8_t offset, uint16_t *value);
bool NewPacket_GetByte(const uint8_t *packet, uint8_t offset, uint8_t *value);
bool NewPacket_GetUint16(const uint8_t *packet, uint8_t offset, uint16_t *value);

#endif /* PACKET_BUILDER_H */
