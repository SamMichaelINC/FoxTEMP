#include "../desktop_settings_app.h"
#include "../views/desktop_settings_view_numeric_pin.h"
#include "desktop_settings_scene.h"
#include <furi.h>

// Explicit internal state tracking for the two-pass validation routine
static uint8_t first_pass_digits[8];
static uint8_t first_pass_len = 0;
static bool confirmation_phase_active = false;

// Custom scene local event definitions
enum {
    LocalSceneEventPinSubmitted,
    LocalSceneEventBackTriggered,
};

void pin_setup_numeric_callback(bool success, void* context) {
    furi_assert(context);
    DesktopSettingsApp* app = context;
    if(success) {
        view_dispatcher_send_custom_event(app->view_dispatcher, LocalSceneEventPinSubmitted);
    } else {
        view_dispatcher_send_custom_event(app->view_dispatcher, LocalSceneEventBackTriggered);
    }
}

void desktop_settings_scene_pin_setup_on_enter(void* context) {
    furi_assert(context);
    DesktopSettingsApp* app = context;

    confirmation_phase_active = false;
    first_pass_len = 0;

    desktop_settings_view_numeric_pin_reset(app->numeric_pin_view);
    desktop_settings_view_numeric_pin_set_mode(app->numeric_pin_view, false); // "Enter PIN:" Mode
    desktop_settings_view_numeric_pin_set_callback(app->numeric_pin_view, pin_setup_numeric_callback, app);

    // FIX: Switch directly to the PIN input view, NOT app->pin_menu_idx!
    view_dispatcher_switch_to_view(app->view_dispatcher, DesktopSettingsAppViewIdPinInput);
}

bool desktop_settings_scene_pin_setup_on_event(void* context, SceneManagerEvent event) {
    furi_assert(context);
    DesktopSettingsApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == LocalSceneEventPinSubmitted) {
            if(!confirmation_phase_active) {
                // PHASE 1: Store the first PIN entry, reset the entry buffer, and swap UI to Confirm mode
                desktop_settings_view_numeric_pin_get_pin(app->numeric_pin_view, first_pass_digits, &first_pass_len);
                
                confirmation_phase_active = true;
                desktop_settings_view_numeric_pin_reset(app->numeric_pin_view);
                desktop_settings_view_numeric_pin_set_mode(app->numeric_pin_view, true); // Changes header to centered "Confirm:"
                
                consumed = true;
            } else {
                // PHASE 2: Fetch confirmation entry and run comparison logic
                uint8_t confirmation_digits[8];
                uint8_t confirmation_len = 0;
                desktop_settings_view_numeric_pin_get_pin(app->numeric_pin_view, confirmation_digits, &confirmation_len);

                if(first_pass_len == confirmation_len && memcmp(first_pass_digits, confirmation_digits, first_pass_len) == 0) {
                    // PIN MATCH SUCCESSFUL!
                    // Convert your visual on-screen numbers (0-9) to Flipper's native arrow key directional layout
                    app->pincode_buffer.length = 0;

                    for(uint8_t i = 0; i < confirmation_len; i++) {
                        uint8_t current_digit = confirmation_digits[i];
                        if(app->pincode_buffer.length >= 12) break; // Keep within native hardware struct boundaries

                        // Map digits to physical hardware arrow directions
                        if(current_digit == 1) {
                            app->pincode_buffer.data[app->pincode_buffer.length++] = InputKeyLeft;
                        } else if(current_digit == 2) {
                            app->pincode_buffer.data[app->pincode_buffer.length++] = InputKeyUp;
                        } else if(current_digit == 3) {
                            app->pincode_buffer.data[app->pincode_buffer.length++] = InputKeyRight;
                        } else if(current_digit == 4) {
                            app->pincode_buffer.data[app->pincode_buffer.length++] = InputKeyLeft;
                            if(app->pincode_buffer.length < 12) app->pincode_buffer.data[app->pincode_buffer.length++] = InputKeyLeft;
                        } else if(current_digit == 5) {
                            app->pincode_buffer.data[app->pincode_buffer.length++] = InputKeyOk;
                        } else if(current_digit == 6) {
                            app->pincode_buffer.data[app->pincode_buffer.length++] = InputKeyRight;
                            if(app->pincode_buffer.length < 12) app->pincode_buffer.data[app->pincode_buffer.length++] = InputKeyRight;
                        } else if(current_digit == 7) {
                            app->pincode_buffer.data[app->pincode_buffer.length++] = InputKeyLeft;
                            if(app->pincode_buffer.length < 12) app->pincode_buffer.data[app->pincode_buffer.length++] = InputKeyDown;
                        } else if(current_digit == 8) {
                            app->pincode_buffer.data[app->pincode_buffer.length++] = InputKeyDown;
                        } else if(current_digit == 9) {
                            app->pincode_buffer.data[app->pincode_buffer.length++] = InputKeyRight;
                            if(app->pincode_buffer.length < 12) app->pincode_buffer.data[app->pincode_buffer.length++] = InputKeyDown;
                        } else if(current_digit == 0) {
                            app->pincode_buffer.data[app->pincode_buffer.length++] = InputKeyDown;
                            if(app->pincode_buffer.length < 12) app->pincode_buffer.data[app->pincode_buffer.length++] = InputKeyDown;
                        }
                    }

                    // Open direct access to the secure hardware storage service record
                    Desktop* desktop_service = furi_record_open(RECORD_DESKTOP);
                    
                    // Route to the app's structural storage buffer to cleanly commit the configuration change
                    desktop_set_pin(desktop_service, &app->pincode_buffer);
                    
                    furi_record_close(RECORD_DESKTOP);

                    // Route to your configured success scene automatically
                    scene_manager_next_scene(app->scene_manager, DesktopSettingsAppScenePinSetupDone);
                } else {
                    // PIN MISMATCH: Route to error view layout
                    scene_manager_next_scene(app->scene_manager, DesktopSettingsAppScenePinError);
                }
                consumed = true;
            }
        } else if(event.event == LocalSceneEventBackTriggered) {
            if(confirmation_phase_active) {
                // Revert to initial stage instead of exiting out completely if back is pressed on confirmation screen
                confirmation_phase_active = false;
                desktop_settings_view_numeric_pin_reset(app->numeric_pin_view);
                desktop_settings_view_numeric_pin_set_mode(app->numeric_pin_view, false);
                consumed = true;
            } else {
                consumed = scene_manager_search_and_switch_to_previous_scene(
                    app->scene_manager, DesktopSettingsAppSceneStart);
            }
        }
    }
    return consumed;
}

void desktop_settings_scene_pin_setup_on_exit(void* context) {
    furi_assert(context);
    DesktopSettingsApp* app = context;
    desktop_settings_view_numeric_pin_set_callback(app->numeric_pin_view, NULL, NULL);
}