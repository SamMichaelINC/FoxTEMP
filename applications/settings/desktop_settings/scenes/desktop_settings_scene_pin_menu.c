#include <gui/scene_manager.h>
#include <gui/modules/variable_item_list.h>
#include <applications.h>

#include "../desktop_settings_app.h"
#include "desktop_settings_scene.h"
#include "desktop_settings_scene_i.h"
#include "../desktop_settings_custom_event.h"
#include <desktop/desktop_settings.h>

static uint8_t s_pin_action_count = 0;
static VariableItem* s_exceed_item = NULL;

static const char* const s_max_attempts_labels[] = {
    "No Limit", "3", "4", "5", "6", "7", "8", "9", "10"
};

static const char* const s_exceed_labels[] = {
    "Fake Wipe", "Format SD"
};

static uint8_t index_to_max_attempts(uint8_t idx) {
    return (idx == 0) ? 0 : (uint8_t)(idx + 2);
}

static uint8_t max_attempts_to_index(uint8_t val) {
    if(val < 3) return 0;
    if(val > 10) return 8;
    return (uint8_t)(val - 2);
}

static void pin_menu_max_attempts_change(VariableItem* item) {
    DesktopSettingsApp* app = variable_item_get_context(item);
    uint8_t idx = variable_item_get_current_value_index(item);
    app->settings.pin_max_attempts = index_to_max_attempts(idx);
    variable_item_set_current_value_text(item, s_max_attempts_labels[idx]);
    desktop_settings_save(&app->settings);

    if(s_exceed_item != NULL) {
        if(app->settings.pin_max_attempts == 0) {
            variable_item_set_current_value_text(s_exceed_item, "N/A");
        } else {
            variable_item_set_current_value_text(
                s_exceed_item, s_exceed_labels[app->settings.pin_exceed_action]);
        }
    }
}

static void pin_menu_exceed_action_change(VariableItem* item) {
    DesktopSettingsApp* app = variable_item_get_context(item);
    if(app->settings.pin_max_attempts == 0) {
        variable_item_set_current_value_index(item, app->settings.pin_exceed_action);
        variable_item_set_current_value_text(item, "N/A");
        return;
    }
    uint8_t idx = variable_item_get_current_value_index(item);
    app->settings.pin_exceed_action = idx;
    variable_item_set_current_value_text(item, s_exceed_labels[idx]);
    desktop_settings_save(&app->settings);
}

static void pin_menu_enter_callback(void* context, uint32_t index) {
    DesktopSettingsApp* app = context;

    if(index >= s_pin_action_count) return;

    if(!desktop_pin_code_is_set()) {
        scene_manager_next_scene(app->scene_manager, DesktopSettingsAppScenePinSetupHowto);
    } else if(index == 0) {
        scene_manager_set_scene_state(
            app->scene_manager,
            DesktopSettingsAppScenePinAuth,
            SCENE_STATE_PIN_AUTH_CHANGE_PIN);
        scene_manager_next_scene(app->scene_manager, DesktopSettingsAppScenePinAuth);
    } else if(index == 1) {
        scene_manager_set_scene_state(
            app->scene_manager,
            DesktopSettingsAppScenePinAuth,
            SCENE_STATE_PIN_AUTH_DISABLE);
        scene_manager_next_scene(app->scene_manager, DesktopSettingsAppScenePinAuth);
    }
}

void desktop_settings_scene_pin_menu_on_enter(void* context) {
    DesktopSettingsApp* app = context;
    VariableItemList* var_list = app->variable_item_list;

    variable_item_list_reset(var_list);
    s_exceed_item = NULL;

    if(!desktop_pin_code_is_set()) {
        s_pin_action_count = 1;
        variable_item_list_add(var_list, "Set PIN", 0, NULL, NULL);
    } else {
        s_pin_action_count = 2;
        variable_item_list_add(var_list, "Change PIN", 0, NULL, NULL);
        variable_item_list_add(var_list, "Remove PIN", 0, NULL, NULL);
    }

    VariableItem* attempts_item = variable_item_list_add(
        var_list, "MAX Attempts", 9, pin_menu_max_attempts_change, app);
    uint8_t attempts_idx = max_attempts_to_index(app->settings.pin_max_attempts);
    variable_item_set_current_value_index(attempts_item, attempts_idx);
    variable_item_set_current_value_text(attempts_item, s_max_attempts_labels[attempts_idx]);

    s_exceed_item = variable_item_list_add(
        var_list, "On Exceed", 2, pin_menu_exceed_action_change, app);
    variable_item_set_current_value_index(s_exceed_item, app->settings.pin_exceed_action);
    variable_item_set_current_value_text(
        s_exceed_item,
        app->settings.pin_max_attempts == 0 ? "N/A" : s_exceed_labels[app->settings.pin_exceed_action]);

    variable_item_list_set_enter_callback(var_list, pin_menu_enter_callback, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, DesktopSettingsAppViewVarItemList);
}

bool desktop_settings_scene_pin_menu_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void desktop_settings_scene_pin_menu_on_exit(void* context) {
    DesktopSettingsApp* app = context;
    s_exceed_item = NULL;
    variable_item_list_reset(app->variable_item_list);
}