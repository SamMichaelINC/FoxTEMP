#include <stdint.h>
#include <core/check.h>
#include <gui/scene_manager.h>
#include "../desktop_settings_app.h"
#include "../views/desktop_settings_view_numeric_pin.h"
#include "desktop_settings_scene.h"
#include "desktop_settings_scene_i.h"

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

static void desktop_settings_scene_pin_setup_callback(bool success, void* context) {
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

        // Store the translated hardware keys directly into the app's top-level pincode_buffer
        for(uint8_t i = 0; i < model->pin_length; i++) {
            uint8_t numeric_digit = model->pin_buffer[i];
            app->pincode_buffer.data[i] = pin_hardware_map[numeric_digit % 10];
        }
        app->pincode_buffer.length = model->pin_length;
        view_commit_model(view, false);

        // Advance to the setup confirmation/done state step
        scene_manager_next_scene(app->scene_manager, DesktopSettingsAppScenePinSetupDone);
    }
}

void desktop_settings_scene_pin_setup_on_enter(void* context) {
    DesktopSettingsApp* app = context;

    desktop_settings_view_numeric_pin_reset(app->numeric_pin_view);
    desktop_settings_view_numeric_pin_set_callback(
        app->numeric_pin_view, desktop_settings_scene_pin_setup_callback, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, DesktopSettingsAppViewIdPinInput);
}

bool desktop_settings_scene_pin_setup_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void desktop_settings_scene_pin_setup_on_exit(void* context) {
    DesktopSettingsApp* app = context;
    desktop_settings_view_numeric_pin_set_callback(app->numeric_pin_view, NULL, NULL);
}