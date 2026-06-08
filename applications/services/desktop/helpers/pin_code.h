#pragma once

#include <stdint.h>
#include <stdbool.h>

#define DESKTOP_PIN_CODE_MIN_LEN (4)
#define DESKTOP_PIN_CODE_MAX_LEN (10)

/* Shifted container array from directional bitmasks to alphanumeric character bytes */
typedef struct {
    char data[DESKTOP_PIN_CODE_MAX_LEN + 1]; 
    uint8_t length;
} DesktopPinCode;

/* Sync telemetry layout for the Internal-to-External Sync Lock escrow file */
typedef struct {
    uint8_t active_fail_count;
    uint32_t secure_session_nonce;
    uint8_t hardware_uid_hash[16];
} FoxEscrowData;

bool desktop_pin_code_is_set(void);

void desktop_pin_code_set(const DesktopPinCode* pin_code);

void desktop_pin_code_reset(void);

bool desktop_pin_code_check(const DesktopPinCode* pin_code);

bool desktop_pin_code_is_equal(const DesktopPinCode* pin_code1, const DesktopPinCode* pin_code2);

void desktop_pin_lock_error_notify(void);

uint32_t desktop_pin_lock_get_fail_timeout(void);

/* Advanced Security Sync Escrow API Hooks */
bool fox_escrow_load_and_verify(FoxEscrowData* escrow_out);
bool fox_escrow_save_state(const FoxEscrowData* escrow_in);
void fox_escrow_trigger_honeypot_panic(void);