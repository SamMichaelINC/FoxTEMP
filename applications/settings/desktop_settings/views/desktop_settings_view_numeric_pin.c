#include "desktop_settings_view_numeric_pin.h"
#include <gui/view.h>
#include <gui/canvas.h>
#include <stdint.h>
#include <string.h>
#include <core/check.h>

#define KEY_ROWS 3
#define KEY_COLS 4

// Pixel-perfect individual grid settings to utilize the full 128x64 display
#define KEY_WIDTH 25     // Unified column widths for a clean look
#define KEY_HEIGHT 13    // Compact button heights to leave balanced top & bottom gaps
#define GRID_START_X 5
#define GRID_START_Y 16
#define X_GAP 2
#define Y_GAP 2          // Grid gaps cleanly separating each individual button
#define SIDEBAR_GAP_X 12 // Expanded to 12 to align the right row perfectly with the top PIN Box

static const char* const main_text_map[9] = {
    "1", "2", "3",
    "4", "5", "6",
    "7", "8", "9"
};

typedef struct {
    uint8_t selected_row;
    uint8_t selected_col;
    uint8_t pin_length;
    uint8_t pin_buffer[8]; // Max length adjusted cleanly to 8 digits
    bool is_confirm_mode;  // Toggles header between "Enter PIN:" and "Confirm:"
} NumericPinModel;

struct DesktopSettingsViewNumericPin {
    View* view;
    DesktopSettingsViewNumericPinCallback callback;
    void* context;
};

static void desktop_settings_view_numeric_pin_draw_callback(Canvas* canvas, void* _model) {
    furi_assert(canvas);
    furi_assert(_model);
    NumericPinModel* model = _model;

    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);

    // 1. Draw Top Header Status Row (Displays "Confirm:" or "Enter PIN:" centered left)
    canvas_set_font(canvas, FontPrimary);
    if(model->is_confirm_mode) {
        // Center the "Confirm:" label perfectly in the 59px left-side column space
        canvas_draw_str_aligned(canvas, 59 / 2, 7, AlignCenter, AlignCenter, "Confirm:");
    } else {
        canvas_draw_str(canvas, 5, 11, "Enter PIN:");
    }

    // Left border aligned to match the left border of the 3rd column (X = 59)
    // Width stretches to reach the right border of the sidebar (X = 124, so width = 65)
    canvas_draw_rframe(canvas, 59, 1, 65, 13, 2);
    
    // Patch the hollow corner pixels on the PIN input box for a solid high-contrast border
    canvas_draw_dot(canvas, 59, 1);
    canvas_draw_dot(canvas, 59 + 65 - 1, 1);
    canvas_draw_dot(canvas, 59, 1 + 13 - 1);
    canvas_draw_dot(canvas, 59 + 65 - 1, 1 + 13 - 1);

    // Draw small filled circular discs inside the input box (only for entered characters)
    uint8_t total_dots = model->pin_length;
    if (total_dots > 0) {
        int16_t box_center_x = 59 + (65 / 2);
        int16_t dot_start_x = box_center_x - (((total_dots * 7) - 1) / 2);

        for(uint8_t i = 0; i < total_dots; i++) {
            int16_t dot_x = dot_start_x + (i * 7);
            canvas_draw_disc(canvas, dot_x, 7, 2); // Clean 2px filled circular disc
        }
    }

    // 2. Draw the Keypad Grid (3 Rows, 4 Columns with equal-width buttons)
    for(uint8_t row = 0; row < KEY_ROWS; row++) {
        for(uint8_t col = 0; col < KEY_COLS; col++) {
            // Compute X coordinate, injecting the sidebar gap offset for column index 3
            int16_t box_x = GRID_START_X + (col * (KEY_WIDTH + X_GAP));
            if(col == 3) {
                box_x += SIDEBAR_GAP_X;
            }
            int16_t box_y = GRID_START_Y + (row * (KEY_HEIGHT + Y_GAP));

            // Invert the colors of the currently selected button
            bool is_selected = (row == model->selected_row && col == model->selected_col);
            if(is_selected) {
                canvas_set_color(canvas, ColorBlack);
                canvas_draw_rbox(canvas, box_x, box_y, KEY_WIDTH, KEY_HEIGHT, 2);
                
                // Patch selection box corners so they appear cleanly filled
                canvas_draw_dot(canvas, box_x, box_y);
                canvas_draw_dot(canvas, box_x + KEY_WIDTH - 1, box_y);
                canvas_draw_dot(canvas, box_x, box_y + KEY_HEIGHT - 1);
                canvas_draw_dot(canvas, box_x + KEY_WIDTH - 1, box_y + KEY_HEIGHT - 1);
                
                canvas_set_color(canvas, ColorWhite);
            } else {
                canvas_set_color(canvas, ColorBlack);
                canvas_draw_rframe(canvas, box_x, box_y, KEY_WIDTH, KEY_HEIGHT, 2);
                
                // Patch frame outline corners so the line is continuous and unbroken
                canvas_draw_dot(canvas, box_x, box_y);
                canvas_draw_dot(canvas, box_x + KEY_WIDTH - 1, box_y);
                canvas_draw_dot(canvas, box_x, box_y + KEY_HEIGHT - 1);
                canvas_draw_dot(canvas, box_x + KEY_WIDTH - 1, box_y + KEY_HEIGHT - 1);
            }

            if(col < 3) {
                // Alphanumeric standard layout (1-9)
                uint8_t idx = (row * 3) + col;
                
                // Draw bold main number perfectly centered inside its individual curved box
                canvas_set_font(canvas, FontPrimary);
                canvas_draw_str_aligned(
                    canvas, 
                    box_x + (KEY_WIDTH / 2), 
                    box_y + (KEY_HEIGHT / 2) + 1, 
                    AlignCenter, 
                    AlignCenter, 
                    main_text_map[idx]
                );
            } else {
                // Action sidebar utility keys (<-/0/Set)
                int16_t text_x = box_x + (KEY_WIDTH / 2);
                int16_t text_y = box_y + (KEY_HEIGHT / 2) + 1;

                if(row == 0) {
                    // Backspace key display "<-" (smaller and bold double strike)
                    canvas_set_font(canvas, FontKeyboard);
                    canvas_draw_str_aligned(canvas, text_x, text_y, AlignCenter, AlignCenter, "<-");
                    canvas_draw_str_aligned(canvas, text_x + 1, text_y, AlignCenter, AlignCenter, "<-");
                } else if(row == 1) {
                    // Bold zero key centered perfectly
                    canvas_set_font(canvas, FontPrimary); 
                    canvas_draw_str_aligned(canvas, text_x, text_y, AlignCenter, AlignCenter, "0");
                } else if(row == 2) {
                    // Set key display (smaller and bold double strike, renamed cleanly from SET)
                    canvas_set_font(canvas, FontKeyboard);
                    canvas_draw_str_aligned(canvas, text_x, text_y, AlignCenter, AlignCenter, "Set");
                    canvas_draw_str_aligned(canvas, text_x + 1, text_y, AlignCenter, AlignCenter, "Set");
                }
            }
            
            // Restore drawing color for safety
            canvas_set_color(canvas, ColorBlack);
        }
    }
}

static bool desktop_settings_view_numeric_pin_input_callback(InputEvent* event, void* context) {
    furi_assert(context);
    DesktopSettingsViewNumericPin* instance = context;
    bool consumed = false;
    bool trigger_callback = false;

    if(event->type == InputTypeShort || event->type == InputTypeRepeat) {
        NumericPinModel* model = view_get_model(instance->view);
        if(event->key == InputKeyUp) {
            model->selected_row = (model->selected_row + KEY_ROWS - 1) % KEY_ROWS;
            consumed = true;
        } else if(event->key == InputKeyDown) {
            model->selected_row = (model->selected_row + 1) % KEY_ROWS;
            consumed = true;
        } else if(event->key == InputKeyLeft) {
            model->selected_col = (model->selected_col + KEY_COLS - 1) % KEY_COLS;
            consumed = true;
        } else if(event->key == InputKeyRight) {
            model->selected_col = (model->selected_col + 1) % KEY_COLS;
            consumed = true;
        } else if(event->key == InputKeyOk) {
            if(model->selected_col < 3) {
                // Clicked 1-9
                if(model->pin_length < 8) { // Up to 8 digits max
                    uint8_t idx = (model->selected_row * 3) + model->selected_col;
                    model->pin_buffer[model->pin_length] = idx + 1;
                    model->pin_length++;
                }
            } else {
                // Clicked action sidebar (<-/0/Set)
                if(model->selected_row == 0) { // DEL ("<-")
                    if(model->pin_length > 0) {
                        model->pin_length--;
                    }
                } else if(model->selected_row == 1) { // 0
                    if(model->pin_length < 8) { // Up to 8 digits max
                        model->pin_buffer[model->pin_length] = 0;
                        model->pin_length++;
                    }
                } else if(model->selected_row == 2) { // Set
                    if(model->pin_length > 0 && instance->callback) {
                        trigger_callback = true; // Flag callback to be executed cleanly AFTER commit
                    }
                }
            }
            consumed = true;
        }
        view_commit_model(instance->view, consumed);

        // Safely trigger the scene event callback only after the view model mutex is completely unlocked!
        if(trigger_callback && instance->callback) {
            instance->callback(true, instance->context);
        }
    } else if(event->type == InputTypeLong && event->key == InputKeyBack) {
        if(instance->callback) {
            instance->callback(false, instance->context);
            consumed = true;
        }
    }
    return consumed;
}

DesktopSettingsViewNumericPin* desktop_settings_view_numeric_pin_alloc(void) {
    DesktopSettingsViewNumericPin* instance = malloc(sizeof(DesktopSettingsViewNumericPin));
    instance->view = view_alloc();
    instance->callback = NULL;
    instance->context = NULL;

    view_allocate_model(instance->view, ViewModelTypeLocking, sizeof(NumericPinModel));
    view_set_context(instance->view, instance);
    view_set_draw_callback(instance->view, desktop_settings_view_numeric_pin_draw_callback);
    view_set_input_callback(instance->view, desktop_settings_view_numeric_pin_input_callback);

    desktop_settings_view_numeric_pin_reset(instance);
    return instance;
}

void desktop_settings_view_numeric_pin_free(DesktopSettingsViewNumericPin* instance) {
    furi_assert(instance);
    view_free(instance->view);
    free(instance);
}

View* desktop_settings_view_numeric_pin_get_view(DesktopSettingsViewNumericPin* instance) {
    furi_assert(instance);
    return instance->view;
}

void desktop_settings_view_numeric_pin_set_callback(
    DesktopSettingsViewNumericPin* instance,
    DesktopSettingsViewNumericPinCallback callback,
    void* context) {
    furi_assert(instance);
    instance->callback = callback;
    instance->context = context;
}

void desktop_settings_view_numeric_pin_reset(DesktopSettingsViewNumericPin* instance) {
    furi_assert(instance);
    NumericPinModel* model = view_get_model(instance->view);
    model->selected_row = 0;
    model->selected_col = 0;
    model->pin_length = 0;
    memset(model->pin_buffer, 0, sizeof(model->pin_buffer));
    // Do NOT reset model->is_confirm_mode here! Let the scene manager handle the lifecycle explicitly.
    view_commit_model(instance->view, true);
}

// Allows scenes to toggle between "Enter PIN:" and "Confirm:" states on the fly
void desktop_settings_view_numeric_pin_set_mode(DesktopSettingsViewNumericPin* instance, bool is_confirm) {
    furi_assert(instance);
    NumericPinModel* model = view_get_model(instance->view);
    model->is_confirm_mode = is_confirm;
    view_commit_model(instance->view, true);
}

// Public API helper allowing the parent Scenes to cleanly retrieve the pin values
void desktop_settings_view_numeric_pin_get_pin(DesktopSettingsViewNumericPin* instance, uint8_t* pin_buffer, uint8_t* pin_length) {
    furi_assert(instance);
    NumericPinModel* model = view_get_model(instance->view);
    if(pin_buffer) {
        memcpy(pin_buffer, model->pin_buffer, model->pin_length);
    }
    if(pin_length) {
        *pin_length = model->pin_length;
    }
    view_commit_model(instance->view, false);
}