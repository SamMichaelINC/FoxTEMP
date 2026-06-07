#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <core/pubsub.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Dolphin Stub Header - Minimal interface for FoxFW v2.0
 * 
 * This stub removes:
 * - Dolphin emotions and happiness states
 * - Deed/achievement tracking
 * - Butthurt mechanics
 * - Level progression
 * 
 * What remains:
 * - Pubsub event notification (minimal broadcast capability)
 * - Basic stats structure (but always returns defaults)
 * 
 * This saves significant RAM and .text sector space by stubbing out
 * the extensive deed tracking and state machine in the full Dolphin service.
 */

#define RECORD_DOLPHIN "dolphin"

typedef struct Dolphin Dolphin;

// Minimal stats - always returns locked/default state
typedef struct {
    uint32_t icounter;      // Always 0
    uint32_t butthurt;      // Always 0
    uint64_t timestamp;     // Always 0
    uint8_t level;          // Always 1 (locked)
    bool level_up_is_pending; // Always false
} DolphinStats;

// Minimal settings - always returns defaults
typedef struct {
    bool happy_mode;        // Always false
} DolphinSettings;

// Pubsub event types (kept for compatibility)
typedef enum {
    DolphinPubsubEventUpdate,
} DolphinPubsubEvent;

// Minimal deed enum - stubs all deed tracking
typedef enum {
    DolphinDeedSubGhzReceiverInfo,
    DolphinDeedSubGhzSave,
    DolphinDeedSubGhzRawRec,
    DolphinDeedSubGhzAddManually,
    DolphinDeedSubGhzSend,
    DolphinDeedSubGhzFrequencyAnalyzer,
    DolphinDeedRfidRead,
    DolphinDeedRfidReadSuccess,
    DolphinDeedRfidSave,
    DolphinDeedRfidEmulate,
    DolphinDeedRfidAdd,
    DolphinDeedNfcRead,
    DolphinDeedNfcReadSuccess,
    DolphinDeedNfcSave,
    DolphinDeedNfcDetectReader,
    DolphinDeedNfcEmulate,
    DolphinDeedNfcKeyAdd,
    DolphinDeedNfcAddSave,
    DolphinDeedNfcAddEmulate,
    DolphinDeedIrSend,
    DolphinDeedIrLearnSuccess,
    DolphinDeedIrSave,
    DolphinDeedIbuttonRead,
    DolphinDeedIbuttonReadSuccess,
    DolphinDeedIbuttonSave,
    DolphinDeedIbuttonEmulate,
    DolphinDeedIbuttonAdd,
    DolphinDeedBadUsbPlayScript,
    DolphinDeedU2fAuthorized,
    DolphinDeedGpioUartBridge,
    DolphinDeedPluginStart,
    DolphinDeedPluginGameStart,
    DolphinDeedPluginGameWin,
    DolphinDeedMAX,
    DolphinDeedTestLeft,
    DolphinDeedTestRight,
} DolphinDeed;

// API stubs

/** Stub: Notify dolphin of a deed (completely ignored in stub) */
void dolphin_deed(DolphinDeed deed);

/** Stub: Get dolphin settings (always returns defaults) */
void dolphin_get_settings(Dolphin* dolphin, DolphinSettings* settings);

/** Stub: Set dolphin settings (ignored in stub) */
void dolphin_set_settings(Dolphin* dolphin, DolphinSettings* settings);

/** Stub: Get dolphin stats (always returns default/level 1) */
DolphinStats dolphin_stats(Dolphin* dolphin);

/** Stub: Flush dolphin state (no-op in stub) */
void dolphin_flush(Dolphin* dolphin);

/** Stub: Upgrade dolphin level (no-op in stub) */
void dolphin_upgrade_level(Dolphin* dolphin);

/** Stub: Get pubsub event notification system (minimal broadcast capability) */
FuriPubSub* dolphin_get_pubsub(Dolphin* dolphin);

#ifdef __cplusplus
}
#endif
