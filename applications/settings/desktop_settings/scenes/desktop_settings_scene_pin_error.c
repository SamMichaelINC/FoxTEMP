#include <applications/services/desktop/desktop_i.h>
#include "desktop_settings_scene.h"
#include <assets_icons.h>
#include <applications/services/desktop/helpers/pin_code.h>

#include "../desktop_settings_app.h"

void desktop_settings_scene_pin_error_on_enter(void* context) {
    DesktopSettingsApp* app = context;
    
    popup_reset(app->popup);
    popup_set_context(app->popup, app);
    popup_set_header(app->popup, "Incorrect PIN!", 64, 5, AlignCenter, AlignTop);
    popup_set_text(app->popup, "Access Denied.\nToo many failures\nwill trigger wipe.", 64, 26, AlignCenter, AlignTop);
    popup_set_icon(app->popup, 8, 18, &I_DolphinCommon_56x48);
    
    view_dispatcher_switch_to_view(app->view_dispatcher, DesktopSettingsAppViewIdPopup);
}

bool desktop_settings_scene_pin_error_on_event(void* context, SceneManagerEvent event) {
    DesktopSettingsApp* app = context;
    bool consumed = false;
    
    if(event.type == SceneManagerEventTypeBack) {
        consumed = scene_manager_search_and_switch_to_previous_scene(
            app->scene_manager, DesktopSettingsAppScenePinSetup);
    }
    
    return consumed;
}

void desktop_settings_scene_pin_error_on_exit(void* context) {
    DesktopSettingsApp* app = context;
    popup_reset(app->popup);
}