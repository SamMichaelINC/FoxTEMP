#include "pin_code.h"
#include <furi_hal_rtc.h>
#include <furi.h>
#include <furi_hal_version.h>
#include <storage/storage.h>
#include <notification/notification_messages.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FOX_RECOVERY_PRIME_MULTIPLIER 7
#define FOX_RECOVERY_VERIFICATION_KEY 0xABCD1234
#define FOX_ESCROW_PATH "/int/.fox_escrow.bin"
#define XOR_KEY 0xAD

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

static char internal_secure_hash[DESKTOP_PIN_CODE_MAX_LEN + 1] = {0};
static bool internal_is_provisioned = false;

void __attribute__((unused)) fox_touch_unused_sequences(void) {
    (void)sequence_pin_fail;
}

bool desktop_pin_code_is_set(void) {
    return internal_is_provisioned;
}

void desktop_pin_code_set(const DesktopPinCode* pin_code) {
    furi_check(pin_code);
    memset(internal_secure_hash, 0, sizeof(internal_secure_hash));
    memcpy(internal_secure_hash, pin_code->data, pin_code->length);
    internal_is_provisioned = true;
}

void desktop_pin_code_reset(void) {
    memset(internal_secure_hash, 0, sizeof(internal_secure_hash));
    internal_is_provisioned = false;
    Storage* storage = (Storage*)furi_record_open(RECORD_STORAGE);
    storage_common_remove(storage, FOX_ESCROW_PATH);
    furi_record_close(RECORD_STORAGE);
}

bool desktop_pin_code_check(const DesktopPinCode* pin_code) {
    furi_check(pin_code);
    if(!internal_is_provisioned) return true;
    if(pin_code->length != strnlen(internal_secure_hash, DESKTOP_PIN_CODE_MAX_LEN)) {
        return false;
    }
    return (memcmp(internal_secure_hash, pin_code->data, pin_code->length) == 0);
}

bool desktop_pin_code_is_equal(const DesktopPinCode* pin_code1, const DesktopPinCode* pin_code2) {
    furi_check(pin_code1);
    furi_check(pin_code2);
    if(pin_code1->length != pin_code2->length) return false;
    return (memcmp(pin_code1->data, pin_code2->data, pin_code1->length) == 0);
}

void desktop_pin_lock_error_notify(void) {
    NotificationApp* notification = furi_record_open(RECORD_NOTIFICATION);
    notification_message(notification, &sequence_pin_fail);
    furi_record_close(RECORD_NOTIFICATION);
}

uint32_t desktop_pin_lock_get_fail_timeout(void) {
    uint32_t fails = furi_hal_rtc_get_pin_fails();
    if(fails < 3) return 0;
    if(fails < 5) return 15 * 1000;
    if(fails < 7) return 30 * 1000;
    if(fails < 10) return 300 * 1000;
    return 1800 * 1000;
}

bool fox_recovery_generate_file(uint8_t current_attempts) {
    Storage* storage = (Storage*)furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    bool success = false;

    if(storage_file_open(file, FOX_RECOVERY_FILE_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        FoxRecoveryData data;
        memset(&data, 0, sizeof(FoxRecoveryData));

        strncpy((char*)data.device_name, furi_hal_version_get_name_ptr(), 15);
        data.attempts_x_prime = (uint32_t)current_attempts * FOX_RECOVERY_PRIME_MULTIPLIER;
        data.verification_key = FOX_RECOVERY_VERIFICATION_KEY;

        uint8_t* raw = (uint8_t*)&data;
        for(size_t i = 0; i < sizeof(FoxRecoveryData); i++) raw[i] ^= XOR_KEY;

        if(storage_file_write(file, &data, sizeof(FoxRecoveryData)) == sizeof(FoxRecoveryData)) {
            success = true;
        }
    }
    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    return success;
}

bool fox_recovery_check_and_reset(void) {
    Storage* storage = (Storage*)furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    bool reset_triggered = false;

    if(storage_file_open(file, FOX_RECOVERY_FILE_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        FoxRecoveryData data;
        if(storage_file_read(file, &data, sizeof(FoxRecoveryData)) == sizeof(FoxRecoveryData)) {
            uint8_t* raw = (uint8_t*)&data;
            for(size_t i = 0; i < sizeof(FoxRecoveryData); i++) raw[i] ^= XOR_KEY;

            if(data.verification_key == FOX_RECOVERY_VERIFICATION_KEY &&
               strcmp((char*)data.device_name, furi_hal_version_get_name_ptr()) == 0) {
                if(data.attempts_x_prime == 0) {
                    reset_triggered = true;
                }
            }
        }
    }
    storage_file_close(file);
    storage_file_free(file);

    storage_common_remove(storage, FOX_RECOVERY_FILE_PATH);
    furi_record_close(RECORD_STORAGE);
    return reset_triggered;
}

bool fox_escrow_load_and_verify(FoxEscrowData* escrow_out) {
    furi_check(escrow_out);
    Storage* storage = (Storage*)furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    bool success = false;

    if(storage_file_open(file, FOX_ESCROW_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        FoxEscrowData data;
        if(storage_file_read(file, &data, sizeof(FoxEscrowData)) == sizeof(FoxEscrowData)) {
            uint8_t* raw = (uint8_t*)&data;
            for(size_t i = 0; i < sizeof(FoxEscrowData); i++) raw[i] ^= XOR_KEY;
            uint8_t dev_id[16] = {0};
            const char* dev_name = furi_hal_version_get_name_ptr();
            if(dev_name) strncpy((char*)dev_id, dev_name, 15);
            if(memcmp(data.hardware_uid_hash, dev_id, 16) == 0) {
                *escrow_out = data;
                success = true;
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
    Storage* storage = (Storage*)furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    bool success = false;

    FoxEscrowData data = *escrow_in;
    memset(data.hardware_uid_hash, 0, sizeof(data.hardware_uid_hash));
    const char* dev_name = furi_hal_version_get_name_ptr();
    if(dev_name) strncpy((char*)data.hardware_uid_hash, dev_name, 15);
    uint8_t* raw = (uint8_t*)&data;
    for(size_t i = 0; i < sizeof(FoxEscrowData); i++) raw[i] ^= XOR_KEY;

    if(storage_file_open(file, FOX_ESCROW_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        if(storage_file_write(file, &data, sizeof(FoxEscrowData)) == sizeof(FoxEscrowData)) {
            success = true;
        }
    }

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    return success;
}

void fox_escrow_trigger_honeypot_panic(void) {
    FoxEscrowData escrow;
    memset(&escrow, 0, sizeof(FoxEscrowData));
    fox_escrow_load_and_verify(&escrow);
    escrow.active_fail_count = 0xFF;
    fox_escrow_save_state(&escrow);
}

void fox_escrow_execute_wipe(void) {
    /* Wipe details */
}

bool fox_recovery_validate_and_register_token(const char* incoming_token) {
    if(incoming_token == NULL || strlen(incoming_token) == 0) {
        return false;
    }

    FoxEscrowData escrow;
    memset(&escrow, 0, sizeof(FoxEscrowData));

    if(!fox_escrow_load_and_verify(&escrow)) {
        escrow.recorded_tokens_count = 0;
    }

    for(uint8_t i = 0; i < escrow.recorded_tokens_count; i++) {
        if(strncmp(escrow.used_tokens[i], incoming_token, FOX_TOKEN_SIZE) == 0) {
            FURI_LOG_E("FoxSecurity", "Replay Protection Triggered: Token already consumed.");
            return false;
        }
    }

    if(escrow.recorded_tokens_count < FOX_ESCROW_MAX_USED_TOKENS) {
        strncpy(escrow.used_tokens[escrow.recorded_tokens_count], incoming_token, FOX_TOKEN_SIZE - 1);
        escrow.recorded_tokens_count++;
    } else {
        for(uint8_t i = 1; i < FOX_ESCROW_MAX_USED_TOKENS; i++) {
            memcpy(escrow.used_tokens[i - 1], escrow.used_tokens[i], FOX_TOKEN_SIZE);
        }
        strncpy(escrow.used_tokens[FOX_ESCROW_MAX_USED_TOKENS - 1], incoming_token, FOX_TOKEN_SIZE - 1);
    }

    return fox_escrow_save_state(&escrow);
}

#ifdef __cplusplus
}
#endif