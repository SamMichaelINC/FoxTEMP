#include "dolphin_stub.h"
#include <furi.h>
#include <stdlib.h>

/**
 * Dolphin Stub Implementation - FoxFW v2.0
 * 
 * This stub completely removes:
 * - Deed/achievement tracking and scoring
 * - Dolphin emotions and happiness mechanics
 * - Butthurt (sadness) system
 * - Level progression beyond level 1
 * - Persistent state storage
 * 
 * Benefits:
 * - Reduces .text sector usage significantly
 * - Simplifies firmware initialization
 * - Removes dependency on persistent storage for dolphin state
 * - Faster boot times
 * 
 * Limitations:
 * - All deeds are silently ignored
 * - Dolphin always locked at level 1
 * - No emotion/state progression
 * - All stats return default/zero values
 */

/**
 * Stub: Record deed (achievement) - COMPLETELY IGNORED
 * All deed tracking is removed for space savings.
 * Apps call this, but it does nothing.
 */
void dolphin_deed(DolphinDeed deed) {
    UNUSED(deed);
    // No-op: deed tracking completely disabled
}

/**
 * Stub: Get current dolphin settings - RETURNS DEFAULTS
 * Always returns: happy_mode = false
 */
void dolphin_get_settings(Dolphin* dolphin, DolphinSettings* settings) {
    UNUSED(dolphin);
    if(settings) {
        settings->happy_mode = false;
    }
}

/**
 * Stub: Set dolphin settings - IGNORED
 * Settings are not persisted in stub mode
 */
void dolphin_set_settings(Dolphin* dolphin, DolphinSettings* settings) {
    UNUSED(dolphin);
    UNUSED(settings);
    // No-op: settings are not persisted
}

/**
 * Stub: Get dolphin statistics - RETURNS DEFAULTS
 * 
 * Always returns:
 * - icounter: 0 (experience points)
 * - butthurt: 0 (sadness/condition)
 * - timestamp: 0 (last action time)
 * - level: 1 (locked strictly at level 1)
 * - level_up_is_pending: false
 * 
 * This means the Dolphin stays at Level 1 always, matching the
 * L1 folder structure and keeping emotions minimal.
 */
DolphinStats dolphin_stats(Dolphin* dolphin) {
    UNUSED(dolphin);
    DolphinStats stats = {
        .icounter = 0,
        .butthurt = 0,
        .timestamp = 0,
        .level = 1,                // Locked at Level 1
        .level_up_is_pending = false,
    };
    return stats;
}

/**
 * Stub: Flush/save dolphin state - NO-OP
 * No persistent storage in stub mode
 */
void dolphin_flush(Dolphin* dolphin) {
    UNUSED(dolphin);
    // No-op: no persistent state to save
}

/**
 * Stub: Upgrade dolphin level - NO-OP
 * Level is locked at 1, so upgrades do nothing
 */
void dolphin_upgrade_level(Dolphin* dolphin) {
    UNUSED(dolphin);
    // No-op: level locked at 1
}

/**
 * Stub: Get pubsub event notification system
 * 
 * Returns a minimal pubsub instance that can broadcast events.
 * This is necessary for:
 * - Desktop slideshow notification system (desktop needs pubsub)
 * - Any app that subscribes to dolphin events
 * 
 * The pubsub is allocated once and reused. We publish a dummy
 * event immediately to wake up any waiting subscribers.
 */
FuriPubSub* dolphin_get_pubsub(Dolphin* dolphin) {
    UNUSED(dolphin);
    
    // Static pubsub instance - allocated once, reused forever
    static FuriPubSub* stub_pubsub = NULL;
    
    if(stub_pubsub == NULL) {
        // Create the pubsub instance
        stub_pubsub = furi_pubsub_alloc();
        
        // Publish a dummy event immediately to wake up any subscribers
        // This prevents deadlocks in code waiting for dolphin notifications
        uint32_t dummy_event = DolphinPubsubEventUpdate;
        furi_pubsub_publish(stub_pubsub, &dummy_event);
    }
    
    return stub_pubsub;
}
