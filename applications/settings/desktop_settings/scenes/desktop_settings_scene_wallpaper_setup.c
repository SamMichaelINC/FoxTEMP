#include <furi.h>
#include <storage/storage.h>
#include <gui/modules/variable_item_list.h>
#include <gui/modules/dialog_ex.h>

#include "../desktop_settings_app.h"
#include "desktop_settings_scene.h"
#include "../desktop_settings_custom_event.h"
#include <desktop/desktop_settings.h>

#define WALLPAPER_PATH "/ext/wallpaper.xbm"
#define WALLPAPER_SIZE 1024

typedef enum {
    WallpaperStatusNotFound,
    WallpaperStatusWrongSize,
    WallpaperStatusReady,
} WallpaperFileStatus;

static WallpaperFileStatus wallpaper_check_file(void) {
    Storage* storage = furi_record_open(RECORD_STORAGE);

    if(!storage_file_exists(storage, WALLPAPER_PATH)) {
        furi_record_close(RECORD_STORAGE);
        return WallpaperStatusNotFound;
    }

    File* file = storage_file_alloc(storage);
    bool opened = storage_file_open(file, WALLPAPER_PATH, FSAM_READ, FSOM_OPEN_EXISTING);
    uint64_t size = 0;
    if(opened) {
        size = storage_file_size(file);
        storage_file_close(file);
    }
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    if(!opened) return WallpaperStatusNotFound;
    return (size == WALLPAPER_SIZE) ? WallpaperStatusReady : WallpaperStatusWrongSize;
}

static void wallpaper_dialog_result_callback(DialogExResult result, void* context) {
    UNUSED(result);
    DesktopSettingsApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, DesktopSettingsCustomEventExit);
}

static void wallpaper_toggle_changed(VariableItem* item) {
    DesktopSettingsApp* app = variable_item_get_context(item);
    uint8_t idx = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, idx ? "ON" : "OFF");
    app->settings.wallpaper_enabled = idx;
    desktop_settings_save(&app->settings);
}

void desktop_settings_scene_wallpaper_setup_on_enter(void* context) {
    DesktopSettingsApp* app = context;
    WallpaperFileStatus status = wallpaper_check_file();

    if(status == WallpaperStatusReady) {
        VariableItemList* list = app->variable_item_list;
        variable_item_list_reset(list);

        VariableItem* status_item =
            variable_item_list_add(list, "wallpaper.xbm", 0, NULL, NULL);
        variable_item_set_current_value_text(status_item, "READY");

        VariableItem* toggle =
            variable_item_list_add(list, "Custom Wallpaper", 2, wallpaper_toggle_changed, app);
        variable_item_set_current_value_index(toggle, app->settings.wallpaper_enabled);
        variable_item_set_current_value_text(
            toggle, app->settings.wallpaper_enabled ? "ON" : "OFF");

        variable_item_list_set_enter_callback(list, NULL, NULL);
        view_dispatcher_switch_to_view(app->view_dispatcher, DesktopSettingsAppViewVarItemList);

    } else {
        DialogEx* dialog = app->dialog_ex;
        dialog_ex_reset(dialog);
        dialog_ex_set_header(dialog, "Custom Wallpaper", 64, 3, AlignCenter, AlignTop);
        dialog_ex_set_left_button_text(dialog, "Back");
        dialog_ex_set_result_callback(dialog, wallpaper_dialog_result_callback);
        dialog_ex_set_context(dialog, app);

        if(status == WallpaperStatusNotFound) {
            dialog_ex_set_text(
                dialog,
                "wallpaper.xbm\nNOT FOUND\nin SD root.\n\n128x64px, 1024 bytes",
                64, 22, AlignCenter, AlignTop);
        } else {
            dialog_ex_set_text(
                dialog,
                "wallpaper.xbm FOUND\nbut wrong size.\n\nNeeds to be\n128x64px (1024 bytes)",
                64, 22, AlignCenter, AlignTop);
        }

        view_dispatcher_switch_to_view(app->view_dispatcher, DesktopSettingsAppViewDialogEx);
    }
}

bool desktop_settings_scene_wallpaper_setup_on_event(void* context, SceneManagerEvent event) {
    DesktopSettingsApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == DesktopSettingsCustomEventExit) {
            scene_manager_previous_scene(app->scene_manager);
            consumed = true;
        }
    } else if(event.type == SceneManagerEventTypeBack) {
        scene_manager_previous_scene(app->scene_manager);
        consumed = true;
    }

    return consumed;
}

void desktop_settings_scene_wallpaper_setup_on_exit(void* context) {
    DesktopSettingsApp* app = context;
    variable_item_list_reset(app->variable_item_list);
    dialog_ex_reset(app->dialog_ex);
}
