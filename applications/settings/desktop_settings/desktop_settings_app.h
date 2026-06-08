#pragma once

#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/submenu.h>
#include <gui/modules/variable_item_list.h>
#include <gui/modules/popup.h>
#include <gui/modules/dialog_ex.h>
#include <gui/modules/text_input.h>

#include <desktop/desktop.h>
#include <dialogs/dialogs.h>

#include <desktop/views/desktop_view_pin_input.h>
#include "views/desktop_settings_view_pin_setup_howto.h"
#include "views/desktop_settings_view_pin_setup_howto2.h"
#include "views/desktop_settings_view_numeric_pin.h"

typedef struct {
    Gui* gui;
    DialogsApp* dialogs;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;
    
    Popup* popup;
    Submenu* submenu;
    VariableItemList* variable_item_list;
    DesktopViewPinInput* pin_input_view;
    DesktopSettingsViewPinSetupHowto* pin_setup_howto_view;
    void* pin_setup_howto2_view;
    DesktopSettingsViewNumericPin* numeric_pin_view;
    DialogEx* dialog_ex;
    TextInput* text_input;

    char device_name[64];
    bool save_name;
    uint32_t pin_menu_idx;
    DesktopSettings settings;
    DesktopPinCode pincode_buffer;
} DesktopSettingsApp;