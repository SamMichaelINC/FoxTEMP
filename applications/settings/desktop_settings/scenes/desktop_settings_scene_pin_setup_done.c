#include <furi.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>
#include <gui/scene_manager.h>
#include <gui/view_dispatcher.h>

#include "../desktop_settings_app.h"
#include "../desktop_settings_custom_event.h"
#include <desktop/desktop_settings.h>
// FIXED: Removed the forbidden kernel header
#include "../views/desktop_settings_view_numeric_pin.h"
#include "desktop_settings_scene.h"

static void pin_setup_done_numeric_callback(bool success, void* context) {
    UNUSED(success);
    furi_assert(context);
    DesktopSettingsApp* app = context;

    view_dispatcher_send_custom_event(app->view_dispatcher, DesktopSettingsCustomEventDone);
}

void desktop_settings_scene_pin_setup_done_on_enter(void* context) {
    DesktopSettingsApp* app = context;

    // This part is great—it saves the newly configured PIN to the device storage
    desktop_pin_code_set(&app->pincode_buffer);

    NotificationApp* notification = furi_record_open(RECORD_NOTIFICATION);
    notification_message(notification, &sequence_single_vibro);
    notification_message(notification, &sequence_blink_green_10);
    furi_record_close(RECORD_NOTIFICATION);

    // FIXED: Use our custom local view module instead of the unexported kernel module
    desktop_settings_view_numeric_pin_reset(app->numeric_pin_view);
    desktop_settings_view_numeric_pin_set_callback(
        app->numeric_pin_view, pin_setup_done_numeric_callback, app);
    
    // Note: If you want to lock the input matrix so they just see the confirmation layout,
    // your numeric_pin_view handles drawing the keys, and interacting triggers the done callback.

    view_dispatcher_switch_to_view(app->view_dispatcher, DesktopSettingsAppViewIdPinInput);
}

bool desktop_settings_scene_pin_setup_done_on_event(void* context, SceneManagerEvent event) {
    DesktopSettingsApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case DesktopSettingsCustomEventDone: {
            bool scene_found = false;
            scene_found = scene_manager_search_and_switch_to_previous_scene(
                app->scene_manager, DesktopSettingsAppScenePinMenu);
            if(!scene_found) {
                view_dispatcher_stop(app->view_dispatcher);
            }
            consumed = true;
            break;
        }
        default:
            consumed = true;
            break;
        }
    } else if(event.type == SceneManagerEventTypeBack) {
        consumed = true;
    }
    return consumed;
}

void desktop_settings_scene_pin_setup_done_on_exit(void* context) {
    furi_assert(context);
    DesktopSettingsApp* app = context;
    // FIXED: Safely clean out the local numeric module references
    desktop_settings_view_numeric_pin_set_callback(app->numeric_pin_view, NULL, NULL);
}