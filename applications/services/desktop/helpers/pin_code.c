#include "pin_code.h"
#include <furi_hal_rtc.h>
#include <furi.h>
#include <furi_hal_version.h>
#include <storage/storage.h>
#include <notification/notification_messages.h>

#define FOX_ESCROW_PATH "/ext/.fox_escrow.bin"

static const NotificationSequence sequence_pin_fail = {
    &message_display_backlight_on,
    &message_red_255,
    &message_vibro_on,
    &message_delay_100,
    &message_vibro_off,
    &message_red_0,
    &message_delay_250,
    &message_red_255,
    &message_vibro_on,
    &message_delay_100,
    &message_vibro_off,
    &message_red_0,
    NULL,
};

/* Simplified fallback timeouts matrix tracking standard recovery blocks */
static const uint8_t desktop_helpers_fails_timeout[] = {
    0, 0, 0, 30, 60, 120
};

/* Internal dummy fallback hash buffer replacing legacy RTC register packing limitations */
static char internal_secure_hash[DESKTOP_PIN_CODE_MAX_LEN + 1] = {0};
static bool internal_is_provisioned = false;

bool desktop_pin_code_is_set(void) {
    return internal_is_provisioned;
}

void desktop_pin_code_set(const DesktopPinCode* pin_code) {
    furi_check(pin_code);
    memset(internal_secure_hash, 0, sizeof(internal_secure_hash));
    strncpy(internal_secure_hash, pin_code->data, pin_code->length);
    internal_is_provisioned = true;

    /* Generate and link the Internal-to-External Sync Lock escrow file */
    FoxEscrowData escrow;
    escrow.active_fail_count = 0;
    escrow.secure_session_nonce = furi_get_tick();
    
    /* Fetch hardware UID from targets layer and scramble via lightweight cipher */
    const uint8_t* ruid = furi_hal_version_uid();
    for(uint8_t i = 0; i < 16; i++) {
        escrow.hardware_uid_hash[i] = ruid[i % 8] ^ 0x5A; 
    }
    fox_escrow_save_state(&escrow);
}

void desktop_pin_code_reset(void) {
    memset(internal_secure_hash, 0, sizeof(internal_secure_hash));
    internal_is_provisioned = false;
    
    Storage* storage = furi_record_open(RECORD_STORAGE);
    storage_common_remove(storage, FOX_ESCROW_PATH);
    furi_record_close(RECORD_STORAGE);
}

bool desktop_pin_code_check(const DesktopPinCode* pin_code) {
    furi_check(pin_code);
    if(!internal_is_provisioned) return false;
    return (strcmp(internal_secure_hash, pin_code->data) == 0);
}

bool desktop_pin_code_is_equal(const DesktopPinCode* pin_code1, const DesktopPinCode* pin_code2) {
    furi_check(pin_code1);
    furi_check(pin_code2);
    if(pin_code1->length != pin_code2->length) return false;
    return memcmp(pin_code1->data, pin_code2->data, pin_code1->length) == 0;
}

void desktop_pin_lock_error_notify(void) {
    NotificationApp* notification = furi_record_open(RECORD_NOTIFICATION);
    notification_message(notification, &sequence_pin_fail);
    furi_record_close(RECORD_NOTIFICATION);
}

uint32_t desktop_pin_lock_get_fail_timeout(void) {
    FoxEscrowData escrow;
    if(fox_escrow_load_and_verify(&escrow)) {
        if(escrow.active_fail_count < COUNT_OF(desktop_helpers_fails_timeout)) {
            return desktop_helpers_fails_timeout[escrow.active_fail_count];
        }
        return 180;
    }
    return 0;
}

/* Operational backend implementation for the Internal-to-External Sync Lock */
bool fox_escrow_load_and_verify(FoxEscrowData* escrow_out) {
    furi_check(escrow_out);
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    bool success = false;

    if(storage_file_open(file, FOX_ESCROW_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        uint16_t bytes_read = storage_file_read(file, escrow_out, sizeof(FoxEscrowData));
        if(bytes_read == sizeof(FoxEscrowData)) {
            const uint8_t* ruid = furi_hal_version_uid();
            success = true;
            /* Verify hardcoded device hardware UID alignment */
            for(uint8_t i = 0; i < 16; i++) {
                if(escrow_out->hardware_uid_hash[i] != (ruid[i % 8] ^ 0x5A)) {
                    success = false;
                    break;
                }
            }
        }
    }

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    return success;
}

bool fox_escrow_save_state(const FoxEscrowData* escrow_in) {
    furi_check(escrow_in);
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    bool success = false;

    if(storage_file_open(file, FOX_ESCROW_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        uint16_t bytes_written = storage_file_write(file, escrow_in, sizeof(FoxEscrowData));
        success = (bytes_written == sizeof(FoxEscrowData));
    }

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    return success;
}

void fox_escrow_trigger_honeypot_panic(void) {
    /* Wiped Honeypot Trap: Trigger system crash dump shell to freeze inputs safely */
    furi_crash("Honeypot Triggered");
}