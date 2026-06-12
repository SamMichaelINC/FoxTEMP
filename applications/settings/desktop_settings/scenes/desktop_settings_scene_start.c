#include <applications.h>
#include <lib/toolbox/value_index.h>

#include "../desktop_settings_app.h"
#include "desktop_settings_scene.h"
#include "desktop_settings_scene_i.h"
#include <power/power_service/power.h>

typedef enum {
    DesktopSettingsWallpaper = 0,
    DesktopSettingsChangeName,
    DesktopSettingsPinSetup,
    DesktopSettingsAutoLockDelay,
    DesktopSettingsUsbInhibitAutoLock,
    DesktopSettingsBatteryDisplay,
    DesktopSettingsClockDisplay,
    DesktopSettingsLockDisconnect,
    DesktopSettingsFavoriteLeftShort,
    DesktopSettingsFavoriteLeftLong,
    DesktopSettingsFavoriteRightShort,
    DesktopSettingsFavoriteRightLong,
    DesktopSettingsFavoriteOkLong,
} DesktopSettingsEntry;

#define AUTO_LOCK_DELAY_COUNT 9
static const char* const auto_lock_delay_text[AUTO_LOCK_DELAY_COUNT] = {
    "OFF", "10s", "15s", "30s", "60s", "90s", "2min", "5min", "10min",
};
static const uint32_t auto_lock_delay_value[AUTO_LOCK_DELAY_COUNT] =
    {0, 10000, 15000, 30000, 60000, 90000, 120000, 300000, 600000};

#define USB_INHIBIT_COUNT 2
static const char* const usb_inhibit_text[USB_INHIBIT_COUNT] = {"OFF", "ON"};
static const uint32_t usb_inhibit_value[USB_INHIBIT_COUNT] = {0, 1};

#define CLOCK_ENABLE_COUNT 2
static const char* const clock_enable_text[CLOCK_ENABLE_COUNT] = {"OFF", "ON"};
static const uint32_t clock_enable_value[CLOCK_ENABLE_COUNT] = {0, 1};

#define BATTERY_VIEW_COUNT 6
static const char* const battery_view_text[BATTERY_VIEW_COUNT] =
    {"Bar", "%", "Inv. %", "Retro 3", "Retro 5", "Bar %"};
static const uint32_t battery_view_value[BATTERY_VIEW_COUNT] = {
    DISPLAY_BATTERY_BAR,
    DISPLAY_BATTERY_PERCENT,
    DISPLAY_BATTERY_INVERTED_PERCENT,
    DISPLAY_BATTERY_RETRO_3,
    DISPLAY_BATTERY_RETRO_5,
    DISPLAY_BATTERY_BAR_PERCENT};

#define LOCK_DISCONNECT_COUNT 2
static const char* const lock_disconnect_text[LOCK_DISCONNECT_COUNT] = {"OFF", "ON"};
static const uint32_t lock_disconnect_value[LOCK_DISCONNECT_COUNT] = {0, 1};

static void desktop_settings_scene_start_var_list_enter_callback(void* context, uint32_t index) {
    DesktopSettingsApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

static void desktop_settings_scene_start_battery_view_changed(VariableItem* item) {
    DesktopSettingsApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, battery_view_text[index]);
    app->settings.displayBatteryPercentage = index;
}

static void desktop_settings_scene_start_clock_enable_changed(VariableItem* item) {
    DesktopSettingsApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, clock_enable_text[index]);
    app->settings.display_clock = index;
}

static void desktop_settings_scene_start_auto_lock_delay_changed(VariableItem* item) {
    DesktopSettingsApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, auto_lock_delay_text[index]);
    app->settings.auto_lock_delay_ms = auto_lock_delay_value[index];
}

static void desktop_settings_scene_start_usb_inhibit_changed(VariableItem* item) {
    DesktopSettingsApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, usb_inhibit_text[index]);
    app->settings.usb_inhibit_auto_lock = usb_inhibit_value[index];
}

static void desktop_settings_scene_start_lock_disconnect_changed(VariableItem* item) {
    DesktopSettingsApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, lock_disconnect_text[index]);
    app->settings.lock_ble_usb_disconnect = lock_disconnect_value[index];
}

void desktop_settings_scene_start_on_enter(void* context) {
    DesktopSettingsApp* app = context;
    VariableItemList* list = app->variable_item_list;
    VariableItem* item;
    uint8_t value_index;

    // 0 — Custom Wallpaper (button)
    variable_item_list_add(list, "Custom Wallpaper", 0, NULL, NULL);

    // 1 — Change Flipper Name (button)
    variable_item_list_add(list, "Change Flipper Name", 0, NULL, app);

    // 2 — PIN Setup (button)
    variable_item_list_add(list, "PIN Setup", 0, NULL, NULL);

    // 3 — Auto Lock Time
    item = variable_item_list_add(
        list, "Auto Lock Time", AUTO_LOCK_DELAY_COUNT,
        desktop_settings_scene_start_auto_lock_delay_changed, app);
    value_index = value_index_uint32(
        app->settings.auto_lock_delay_ms, auto_lock_delay_value, AUTO_LOCK_DELAY_COUNT);
    variable_item_set_current_value_index(item, value_index);
    variable_item_set_current_value_text(item, auto_lock_delay_text[value_index]);

    // 4 — Auto Lock disarm by USB session
    item = variable_item_list_add(
        list, "Auto Lock disarm by USB", USB_INHIBIT_COUNT,
        desktop_settings_scene_start_usb_inhibit_changed, app);
    value_index = value_index_uint32(
        app->settings.usb_inhibit_auto_lock, usb_inhibit_value, USB_INHIBIT_COUNT);
    variable_item_set_current_value_index(item, value_index);
    variable_item_set_current_value_text(item, usb_inhibit_text[value_index]);

    // 5 — Battery View
    item = variable_item_list_add(
        list, "Battery View", BATTERY_VIEW_COUNT,
        desktop_settings_scene_start_battery_view_changed, app);
    value_index = value_index_uint32(
        app->settings.displayBatteryPercentage, battery_view_value, BATTERY_VIEW_COUNT);
    variable_item_set_current_value_index(item, value_index);
    variable_item_set_current_value_text(item, battery_view_text[value_index]);

    // 6 — Show Clock
    item = variable_item_list_add(
        list, "Show Clock", CLOCK_ENABLE_COUNT,
        desktop_settings_scene_start_clock_enable_changed, app);
    value_index =
        value_index_uint32(app->settings.display_clock, clock_enable_value, CLOCK_ENABLE_COUNT);
    variable_item_set_current_value_index(item, value_index);
    variable_item_set_current_value_text(item, clock_enable_text[value_index]);

    // 7 — Disconnect on Lock (USB + BLE)
    item = variable_item_list_add(
        list, "Disconnect on Lock", LOCK_DISCONNECT_COUNT,
        desktop_settings_scene_start_lock_disconnect_changed, app);
    value_index = value_index_uint32(
        app->settings.lock_ble_usb_disconnect, lock_disconnect_value, LOCK_DISCONNECT_COUNT);
    variable_item_set_current_value_index(item, value_index);
    variable_item_set_current_value_text(item, lock_disconnect_text[value_index]);

    // 8-12 — Favourite apps
    variable_item_list_add(list, "Favourite - Left Short", 0, NULL, NULL);
    variable_item_list_add(list, "Favourite - Left Long", 0, NULL, NULL);
    variable_item_list_add(list, "Favourite - Right Short", 0, NULL, NULL);
    variable_item_list_add(list, "Favourite - Right Long", 0, NULL, NULL);
    variable_item_list_add(list, "Favourite - Ok Long", 0, NULL, NULL);

    variable_item_list_set_enter_callback(
        list, desktop_settings_scene_start_var_list_enter_callback, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, DesktopSettingsAppViewVarItemList);
}

bool desktop_settings_scene_start_on_event(void* context, SceneManagerEvent event) {
    DesktopSettingsApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case DesktopSettingsWallpaper:
            scene_manager_next_scene(app->scene_manager, DesktopSettingsAppSceneWallpaperSetup);
            break;
        case DesktopSettingsChangeName:
            scene_manager_next_scene(app->scene_manager, DesktopSettingsAppSceneChangeName);
            break;
        case DesktopSettingsPinSetup:
            scene_manager_next_scene(app->scene_manager, DesktopSettingsAppScenePinMenu);
            break;

        case DesktopSettingsFavoriteLeftShort:
            scene_manager_set_scene_state(
                app->scene_manager,
                DesktopSettingsAppSceneFavorite,
                SCENE_STATE_SET_FAVORITE_APP | FavoriteAppLeftShort);
            scene_manager_next_scene(app->scene_manager, DesktopSettingsAppSceneFavorite);
            break;
        case DesktopSettingsFavoriteLeftLong:
            scene_manager_set_scene_state(
                app->scene_manager,
                DesktopSettingsAppSceneFavorite,
                SCENE_STATE_SET_FAVORITE_APP | FavoriteAppLeftLong);
            scene_manager_next_scene(app->scene_manager, DesktopSettingsAppSceneFavorite);
            break;
        case DesktopSettingsFavoriteRightShort:
            scene_manager_set_scene_state(
                app->scene_manager,
                DesktopSettingsAppSceneFavorite,
                SCENE_STATE_SET_FAVORITE_APP | FavoriteAppRightShort);
            scene_manager_next_scene(app->scene_manager, DesktopSettingsAppSceneFavorite);
            break;
        case DesktopSettingsFavoriteRightLong:
            scene_manager_set_scene_state(
                app->scene_manager,
                DesktopSettingsAppSceneFavorite,
                SCENE_STATE_SET_FAVORITE_APP | FavoriteAppRightLong);
            scene_manager_next_scene(app->scene_manager, DesktopSettingsAppSceneFavorite);
            break;
        case DesktopSettingsFavoriteOkLong:
            scene_manager_set_scene_state(
                app->scene_manager,
                DesktopSettingsAppSceneFavorite,
                SCENE_STATE_SET_FAVORITE_APP | FavoriteAppOkLong);
            scene_manager_next_scene(app->scene_manager, DesktopSettingsAppSceneFavorite);
            break;

        default:
            break;
        }
        consumed = true;
    }
    return consumed;
}

void desktop_settings_scene_start_on_exit(void* context) {
    DesktopSettingsApp* app = context;
    variable_item_list_reset(app->variable_item_list);
    desktop_settings_save(&app->settings);

    Power* power = furi_record_open(RECORD_POWER);
    power_trigger_ui_update(power);
    furi_record_close(RECORD_POWER);
}