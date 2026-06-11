#pragma once

#include <stdint.h>
#include <stdbool.h>

#define DESKTOP_PIN_CODE_MIN_LEN (4)
#define DESKTOP_PIN_CODE_MAX_LEN (10)
#define FOX_RECOVERY_FILE_PATH "/ext/System/Fox.Setup"

#define FOX_ESCROW_MAX_USED_TOKENS (8)
#define FOX_TOKEN_SIZE             (16)

typedef struct {
    char data[DESKTOP_PIN_CODE_MAX_LEN + 1]; 
    uint8_t length;
} DesktopPinCode;

/* Sync telemetry layout containing token use history */
typedef struct {
    uint8_t active_fail_count;
    uint32_t secure_session_nonce;
    uint8_t hardware_uid_hash[16];
    uint8_t wipe_limit;   
    uint8_t wipe_method;  
    
    // Trackers for checking single-use recovery operations
    char used_tokens[FOX_ESCROW_MAX_USED_TOKENS][FOX_TOKEN_SIZE];
    uint8_t recorded_tokens_count;
} FoxEscrowData;

typedef struct {
    uint8_t device_name[16];
    uint32_t attempts_x_prime; 
    uint32_t verification_key; 
} FoxRecoveryData;

// --- LINKAGE PROTECTION LAYER START ---
#ifdef __cplusplus
extern "C" {
#endif

bool desktop_pin_code_is_set(void);
void desktop_pin_code_set(const DesktopPinCode* pin_code);
void desktop_pin_code_reset(void);
bool desktop_pin_code_check(const DesktopPinCode* pin_code);
bool desktop_pin_code_is_equal(const DesktopPinCode* pin_code1, const DesktopPinCode* pin_code2);
void desktop_pin_lock_error_notify(void);
uint32_t desktop_pin_lock_get_fail_timeout(void);

bool fox_escrow_load_and_verify(FoxEscrowData* escrow_out);
bool fox_escrow_save_state(const FoxEscrowData* escrow_in);
void fox_escrow_trigger_honeypot_panic(void);
void fox_escrow_execute_wipe(void);

bool fox_recovery_check_and_reset(void);
bool fox_recovery_generate_file(uint8_t current_attempts);

// Validation routine hook for the single-use enforcement tracking module
bool fox_recovery_validate_and_register_token(const char* incoming_token);

#ifdef __cplusplus
}
#endif
// --- LINKAGE PROTECTION LAYER END ---