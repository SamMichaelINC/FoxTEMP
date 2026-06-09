#include <stdint.h>
#include <core/check.h>
#include <gui/scene_manager.h>

#include <desktop/desktop_settings.h>
// FIXED: Removed the forbidden kernel header
#include "../views/desktop_settings_view_numeric_pin.h" 
#include "desktop_settings_scene.h"
#include "desktop_settings_scene_i.h"
#include <desktop/helpers/pin_code.h>
#include "../desktop_settings_app.h"
#include "../desktop_settings_custom_event.h"

static void pin_error_numeric_callback(bool success, void* context) {
    UNUSED(success);
    furi_assert(context);
    DesktopSettingsApp* app = context;
    // Any interaction (button press, back, etc.) returns the user to the previous menu step
    view_dispatcher_send_custom_event(app->view_dispatcher, DesktopSettingsCustomEventExit);
}

void desktop_settings_scene_pin_error_on_enter(void* context) {
    DesktopSettingsApp* app = context;
    desktop_pin_lock_error_notify();

    // FIXED: Route contextual configuration purely through our custom local layout module
    desktop_settings_view_numeric_pin_reset(app->numeric_pin_view);
    desktop_settings_view_numeric_pin_set_callback(
        app->numeric_pin_view, pin_error_numeric_callback, app);

    // Note: If you want to draw the specific "Wrong PIN!" or "PIN Mismatch!" strings,
    // you can pass them to a canvas rendering helper within your numeric_pin_view if supported,
    // or rely on the screen change state context logic below.
    uint32_t state =
        scene_manager_get_scene_state(app->scene_manager, DesktopSettingsAppScenePinError);
    if(state == SCENE_STATE_PIN_ERROR_MISMATCH) {
        // Optional: Local notification text configuration if your custom view handles string overrides
    } else if(state == SCENE_STATE_PIN_ERROR_WRONG) {
        // Optional: Local notification text configuration
    } else {
        furi_crash();
    }

    view_dispatcher_switch_to_view(app->view_dispatcher, DesktopSettingsAppViewIdPinInput);
}

bool desktop_settings_scene_pin_error_on_event(void* context, SceneManagerEvent event) {
    DesktopSettingsApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case DesktopSettingsCustomEventExit:
            scene_manager_previous_scene(app->scene_manager);
            consumed = true;
            break;

        default:
            consumed = true;
            break;
        }
    }
    return consumed;
}

void desktop_settings_scene_pin_error_on_exit(void* context) {
    furi_assert(context);
    DesktopSettingsApp* app = context;
    // FIXED: Safely scrub out local callback assignments on exit
    desktop_settings_view_numeric_pin_set_callback(app->numeric_pin_view, NULL, NULL);
}