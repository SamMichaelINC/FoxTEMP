#include <stdint.h>
#include <core/check.h>
#include <gui/scene_manager.h>
#include <desktop/helpers/pin_code.h>
#include "../desktop_settings_app.h"
#include "../desktop_settings_custom_event.h"
#include <desktop/desktop_settings.h>

// FIXED: Removed the unexported kernel view header
#include "../views/desktop_settings_view_numeric_pin.h" 
#include "desktop_settings_scene.h"
#include "desktop_settings_scene_i.h"

// Hardware layout translation map for processing the numeric index to Flipper keypresses
static const InputKey pin_hardware_map[10] = {
    InputKeyDown,  // 0 -> Down
    InputKeyLeft,  // 1 -> Left
    InputKeyUp,    // 2 -> Up
    InputKeyRight, // 3 -> Right
    InputKeyDown,  // 4 -> Down
    InputKeyLeft,  // 5 -> Left
    InputKeyUp,    // 6 -> Up
    InputKeyRight, // 7 -> Right
    InputKeyDown,  // 8 -> Down
    InputKeyLeft   // 9 -> Left
};

static void pin_auth_numeric_callback(bool success, void* context) {
    furi_assert(context);
    DesktopSettingsApp* app = context;

    if(success) {
        View* view = desktop_settings_view_numeric_pin_get_view(app->numeric_pin_view);
        
        struct {
            uint8_t selected_row;
            uint8_t selected_col;
            uint8_t pin_length;
            uint8_t pin_buffer[10];
        }* model = view_get_model(view);

        // Translate the input pin index buffer to hardware keys
        for(uint8_t i = 0; i < model->pin_length; i++) {
            uint8_t numeric_digit = model->pin_buffer[i];
            app->pincode_buffer.data[i] = pin_hardware_map[numeric_digit % 10];
        }
        app->pincode_buffer.length = model->pin_length;
        view_commit_model(view, false);

        // Check if the input pin matches the system pin
        if(desktop_pin_code_check(&app->pincode_buffer)) {
            view_dispatcher_send_custom_event(
                app->view_dispatcher, DesktopSettingsCustomEventPinsEqual);
        } else {
            view_dispatcher_send_custom_event(
                app->view_dispatcher, DesktopSettingsCustomEventPinsDifferent);
        }
    } else {
        // Handle back/exit selection from the keyboard view matrix
        view_dispatcher_send_custom_event(app->view_dispatcher, DesktopSettingsCustomEventExit);
    }
}

void desktop_settings_scene_pin_auth_on_enter(void* context) {
    furi_assert(desktop_pin_code_is_set());
    DesktopSettingsApp* app = context;

    // FIXED: Point authentication execution flows strictly to our custom local view modules
    desktop_settings_view_numeric_pin_reset(app->numeric_pin_view);
    desktop_settings_view_numeric_pin_set_callback(
        app->numeric_pin_view, pin_auth_numeric_callback, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, DesktopSettingsAppViewIdPinInput);
}

bool desktop_settings_scene_pin_auth_on_event(void* context, SceneManagerEvent event) {
    DesktopSettingsApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case DesktopSettingsCustomEventPinsDifferent:
            scene_manager_set_scene_state(
                app->scene_manager, DesktopSettingsAppScenePinError, SCENE_STATE_PIN_ERROR_WRONG);
            scene_manager_next_scene(app->scene_manager, DesktopSettingsAppScenePinError);
            consumed = true;
            break;
        case DesktopSettingsCustomEventPinsEqual: {
            uint32_t state =
                scene_manager_get_scene_state(app->scene_manager, DesktopSettingsAppScenePinAuth);
            if(state == SCENE_STATE_PIN_AUTH_CHANGE_PIN) {
                scene_manager_next_scene(app->scene_manager, DesktopSettingsAppScenePinSetupHowto);
            } else if(state == SCENE_STATE_PIN_AUTH_DISABLE) {
                scene_manager_next_scene(app->scene_manager, DesktopSettingsAppScenePinDisable);
            } else {
                furi_crash();
            }
            consumed = true;
            break;
        }
        case DesktopSettingsCustomEventExit:
            scene_manager_search_and_switch_to_previous_scene(
                app->scene_manager, DesktopSettingsAppScenePinMenu);
            consumed = true;
            break;

        default:
            consumed = true;
            break;
        }
    }
    return consumed;
}

void desktop_settings_scene_pin_auth_on_exit(void* context) {
    furi_assert(context);
    DesktopSettingsApp* app = context;
    // FIXED: Safely clean out the local numeric module references
    desktop_settings_view_numeric_pin_set_callback(app->numeric_pin_view, NULL, NULL);
}