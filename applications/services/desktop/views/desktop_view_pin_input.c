#include <gui/canvas.h>
#include <furi.h>
#include <gui/view.h>
#include <gui/elements.h>
#include <assets_icons.h>
#include <stdint.h>
#include <string.h>

#include "desktop_view_pin_input.h"
#include "../helpers/pin_code.h"

#define NO_ACTIVITY_TIMEOUT 15000

#define MIN_PIN_LENGTH DESKTOP_PIN_CODE_MIN_LEN
#define MAX_PIN_LENGTH DESKTOP_PIN_CODE_MAX_LEN

#define KEY_ROWS 3
#define KEY_COLS 4

// Pixel-perfect layout constants to fit inside the 128x55 safe area
#define KEY_WIDTH 25
#define KEY_HEIGHT 11
#define GRID_START_X 5
#define GRID_START_Y 24
#define X_GAP 2
#define Y_GAP 1
#define SIDEBAR_GAP_X 12

static const char* const main_text_map[9] = {
    "1", "2", "3",
    "4", "5", "6",
    "7", "8", "9"
};

struct DesktopViewPinInput {
    View* view;
    DesktopViewPinInputCallback back_callback;
    DesktopViewPinInputCallback timeout_callback;
    DesktopViewPinInputDoneCallback done_callback;
    void* context;
    FuriTimer* timer;
};

typedef struct {
    DesktopPinCode pin;
    bool pin_hidden;
    bool locked_input;
    uint8_t pin_x;
    uint8_t pin_y;
    
    uint8_t selected_row;
    uint8_t selected_col;
    
    uint8_t current_fail_count;
    uint8_t max_allowed_attempts;

    const char* primary_str;
    uint8_t primary_str_x;
    uint8_t primary_str_y;
    const char* button_label;

    uint8_t typed_digits[15];
    uint8_t typed_digits_len;
} DesktopViewPinInputModel;

static void desktop_view_pin_input_rebuild_pin(DesktopViewPinInputModel* model) {
    model->pin.length = 0;
    for(uint8_t i = 0; i < model->typed_digits_len; i++) {
        uint8_t current_digit = model->typed_digits[i];
        if(model->pin.length >= 12) break;

        if(current_digit == 1) {
            model->pin.data[model->pin.length++] = InputKeyLeft;
        } else if(current_digit == 2) {
            model->pin.data[model->pin.length++] = InputKeyUp;
        } else if(current_digit == 3) {
            model->pin.data[model->pin.length++] = InputKeyRight;
        } else if(current_digit == 4) {
            model->pin.data[model->pin.length++] = InputKeyLeft;
            if(model->pin.length < 12) model->pin.data[model->pin.length++] = InputKeyLeft;
        } else if(current_digit == 5) {
            model->pin.data[model->pin.length++] = InputKeyOk;
        } else if(current_digit == 6) {
            model->pin.data[model->pin.length++] = InputKeyRight;
            if(model->pin.length < 12) model->pin.data[model->pin.length++] = InputKeyRight;
        } else if(current_digit == 7) {
            model->pin.data[model->pin.length++] = InputKeyLeft;
            if(model->pin.length < 12) model->pin.data[model->pin.length++] = InputKeyDown;
        } else if(current_digit == 8) {
            model->pin.data[model->pin.length++] = InputKeyDown;
        } else if(current_digit == 9) {
            model->pin.data[model->pin.length++] = InputKeyRight;
            if(model->pin.length < 12) model->pin.data[model->pin.length++] = InputKeyDown;
        } else if(current_digit == 0) {
            model->pin.data[model->pin.length++] = InputKeyDown;
            if(model->pin.length < 12) model->pin.data[model->pin.length++] = InputKeyDown;
        }
    }
}

static bool desktop_view_pin_input_input(InputEvent* event, void* context) {
    furi_assert(event);
    furi_assert(context);

    DesktopViewPinInput* pin_input = context;
    DesktopViewPinInputModel* model = view_get_model(pin_input->view);

    bool call_back_callback = false;
    bool call_done_callback = false;
    DesktopPinCode pin_code = {0};

    if(event->type == InputTypeShort) {
        switch(event->key) {
        case InputKeyUp:
            if(!model->locked_input) {
                model->selected_row = (model->selected_row + KEY_ROWS - 1) % KEY_ROWS;
            }
            break;
        case InputKeyDown:
            if(!model->locked_input) {
                model->selected_row = (model->selected_row + 1) % KEY_ROWS;
            }
            break;
        case InputKeyLeft:
            if(!model->locked_input) {
                model->selected_col = (model->selected_col + KEY_COLS - 1) % KEY_COLS;
            }
            break;
        case InputKeyRight:
            if(!model->locked_input) {
                model->selected_col = (model->selected_col + 1) % KEY_COLS;
            }
            break;
        case InputKeyOk:
            if(!model->locked_input) {
                if(model->selected_col < 3) {
                    if(model->typed_digits_len < MAX_PIN_LENGTH) {
                        uint8_t idx = (model->selected_row * 3) + model->selected_col;
                        model->typed_digits[model->typed_digits_len++] = idx + 1;
                        desktop_view_pin_input_rebuild_pin(model);
                    }
                } else {
                    if(model->selected_row == 0) {
                        if(model->typed_digits_len > 0) {
                            model->typed_digits_len--;
                            desktop_view_pin_input_rebuild_pin(model);
                        } else {
                            call_back_callback = true;
                        }
                    } else if(model->selected_row == 1) {
                        if(model->typed_digits_len < MAX_PIN_LENGTH) {
                            model->typed_digits[model->typed_digits_len++] = 0;
                            desktop_view_pin_input_rebuild_pin(model);
                        }
                    } else if(model->selected_row == 2) {
                        if(model->typed_digits_len >= MIN_PIN_LENGTH) {
                            call_done_callback = true;
                            pin_code = model->pin;
                        }
                    }
                }
            }
            break;
        case InputKeyBack:
            if(!model->locked_input) {
                if(model->typed_digits_len > 0) {
                    model->typed_digits_len--;
                    desktop_view_pin_input_rebuild_pin(model);
                } else {
                    call_back_callback = true;
                }
            }
            break;
        default:
            break;
        }
    }

    // CRITICAL: Commit model before execution transitions to prevent double-lock deadlocks
    view_commit_model(pin_input->view, true);

    if(call_done_callback && pin_input->done_callback) {
        pin_input->done_callback(&pin_code, pin_input->context);
    } else if(call_back_callback && pin_input->back_callback) {
        pin_input->back_callback(pin_input->context);
    }

    furi_timer_start(pin_input->timer, NO_ACTIVITY_TIMEOUT);

    return true;
}

static void desktop_view_pin_input_draw(Canvas* canvas, void* context) {
    furi_assert(canvas);
    furi_assert(context);

    DesktopViewPinInputModel* model = context;
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    
    canvas_set_font(canvas, FontPrimary);
    
    if(model->locked_input) {
        canvas_draw_icon(canvas, 2, 0, &I_fox_64x64);
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 68, 12, "Device Blocked");
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 68, 26, "Wipe Limit reached.");
        canvas_draw_str(canvas, 68, 38, "Please perform a");
        canvas_draw_str(canvas, 68, 50, "Full Factory Reset");
        canvas_draw_str(canvas, 68, 62, "to restore access.");
        return;
    }

    if(model->primary_str) {
        canvas_draw_str_aligned(canvas, 59 / 2, 16, AlignCenter, AlignCenter, model->primary_str);
    } else {
        canvas_draw_str_aligned(canvas, 59 / 2, 16, AlignCenter, AlignCenter, "Enter PIN:");
    }
    
    canvas_draw_rframe(canvas, 59, 10, 65, 12, 2);

    uint8_t total_dots = model->typed_digits_len;
    if (total_dots > 0) {
        int16_t box_center_x = 59 + (65 / 2);
        int16_t dot_start_x = box_center_x - (((total_dots * 7) - 1) / 2);

        for(uint8_t i = 0; i < total_dots; i++) {
            int16_t dot_x = dot_start_x + (i * 7);
            canvas_draw_disc(canvas, dot_x, 16, 2);
        }
    }

    for(uint8_t row = 0; row < KEY_ROWS; row++) {
        for(uint8_t col = 0; col < KEY_COLS; col++) {
            int16_t box_x = GRID_START_X + (col * (KEY_WIDTH + X_GAP));
            if(col == 3) {
                box_x += SIDEBAR_GAP_X;
            }
            int16_t box_y = GRID_START_Y + (row * (KEY_HEIGHT + Y_GAP));

            bool is_selected = (row == model->selected_row && col == model->selected_col);
            if(is_selected) {
                canvas_set_color(canvas, ColorBlack);
                canvas_draw_rbox(canvas, box_x, box_y, KEY_WIDTH, KEY_HEIGHT, 2);
                canvas_set_color(canvas, ColorWhite);
            } else {
                canvas_set_color(canvas, ColorBlack);
                canvas_draw_rframe(canvas, box_x, box_y, KEY_WIDTH, KEY_HEIGHT, 2);
            }

            if(col < 3) {
                uint8_t idx = (row * 3) + col;
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
                int16_t text_x = box_x + (KEY_WIDTH / 2);
                int16_t text_y = box_y + (KEY_HEIGHT / 2) + 1;

                if(row == 0) {
                    int16_t x = box_x + 6;
                    int16_t y = box_y + 1;

                    canvas_draw_line(canvas, x, y + 4, x + 4, y);
                    canvas_draw_line(canvas, x, y + 4, x + 4, y + 8);
                    canvas_draw_line(canvas, x + 4, y, x + 12, y);
                    canvas_draw_line(canvas, x + 4, y + 8, x + 12, y + 8);
                    canvas_draw_line(canvas, x + 12, y, x + 12, y + 8);

                    canvas_draw_line(canvas, x + 6, y + 2, x + 10, y + 6);
                    canvas_draw_line(canvas, x + 10, y + 2, x + 6, y + 6);
                } else if(row == 1) {
                    canvas_set_font(canvas, FontPrimary); 
                    canvas_draw_str_aligned(canvas, text_x, text_y, AlignCenter, AlignCenter, "0");
                } else if(row == 2) {
                    canvas_set_font(canvas, FontKeyboard);
                    const char* btn_label = model->button_label ? model->button_label : "OK";
                    canvas_draw_str_aligned(canvas, text_x, text_y, AlignCenter, AlignCenter, btn_label);
                }
            }
            canvas_set_color(canvas, ColorBlack);
        }
    }

    if(model->current_fail_count > 0 && model->max_allowed_attempts > 0) {
        char telemetry_str[32];
        
        if((model->current_fail_count + 1) >= model->max_allowed_attempts) {
            snprintf(telemetry_str, sizeof(telemetry_str), "LAST ATTEMPT!");
        } else {
            snprintf(telemetry_str, sizeof(telemetry_str), "Attempt %d/%d", 
                     model->current_fail_count, model->max_allowed_attempts);
        }
        
        canvas_set_font(canvas, FontKeyboard);
        canvas_draw_str_aligned(canvas, GRID_START_X, 11, AlignLeft, AlignBottom, telemetry_str);
    }
}

void desktop_view_pin_input_timer_callback(void* context) {
    DesktopViewPinInput* pin_input = context;
    if(pin_input->timeout_callback) {
        pin_input->timeout_callback(pin_input->context);
    }
}

static void desktop_view_pin_input_enter(void* context) {
    DesktopViewPinInput* pin_input = context;
    furi_timer_start(pin_input->timer, NO_ACTIVITY_TIMEOUT);
}

static void desktop_view_pin_input_exit(void* context) {
    DesktopViewPinInput* pin_input = context;
    furi_timer_stop(pin_input->timer);
}

DesktopViewPinInput* desktop_view_pin_input_alloc(void) {
    DesktopViewPinInput* pin_input = malloc(sizeof(DesktopViewPinInput));
    pin_input->view = view_alloc();
    view_allocate_model(pin_input->view, ViewModelTypeLocking, sizeof(DesktopViewPinInputModel));
    view_set_context(pin_input->view, pin_input);
    view_set_draw_callback(pin_input->view, desktop_view_pin_input_draw);
    view_set_input_callback(pin_input->view, desktop_view_pin_input_input);
    pin_input->timer = furi_timer_alloc(desktop_view_pin_input_timer_callback, FuriTimerTypeOnce, pin_input);
    view_set_enter_callback(pin_input->view, desktop_view_pin_input_enter);
    view_set_exit_callback(pin_input->view, desktop_view_pin_input_exit);

    // Atomic Init sanitation
    DesktopViewPinInputModel* model = view_get_model(pin_input->view);
    memset(model, 0, sizeof(DesktopViewPinInputModel));
    model->pin.length = 0;
    model->typed_digits_len = 0;
    model->selected_row = 0;
    model->selected_col = 0;
    model->current_fail_count = 0;
    model->max_allowed_attempts = 5;
    view_commit_model(pin_input->view, false);

    return pin_input;
}

void desktop_view_pin_input_free(DesktopViewPinInput* pin_input) {
    furi_assert(pin_input);
    furi_timer_free(pin_input->timer);
    view_free(pin_input->view);
    free(pin_input);
}

void desktop_view_pin_input_lock_input(DesktopViewPinInput* pin_input) {
    furi_assert(pin_input);
    DesktopViewPinInputModel* model = view_get_model(pin_input->view);
    model->locked_input = true;
    view_commit_model(pin_input->view, true);
}

void desktop_view_pin_input_unlock_input(DesktopViewPinInput* pin_input) {
    furi_assert(pin_input);
    DesktopViewPinInputModel* model = view_get_model(pin_input->view);
    model->locked_input = false;
    view_commit_model(pin_input->view, true);
}

void desktop_view_pin_input_set_pin(DesktopViewPinInput* pin_input, const DesktopPinCode* pin) {
    furi_assert(pin_input);
    furi_assert(pin);
    DesktopViewPinInputModel* model = view_get_model(pin_input->view);
    model->pin = *pin;
    view_commit_model(pin_input->view, true);
}

void desktop_view_pin_input_reset_pin(DesktopViewPinInput* pin_input) {
    furi_assert(pin_input);
    DesktopViewPinInputModel* model = view_get_model(pin_input->view);
    model->pin.length = 0;
    model->typed_digits_len = 0;
    view_commit_model(pin_input->view, true);
}

void desktop_view_pin_input_hide_pin(DesktopViewPinInput* pin_input, bool pin_hidden) {
    furi_assert(pin_input);
    DesktopViewPinInputModel* model = view_get_model(pin_input->view);
    model->pin_hidden = pin_hidden;
    view_commit_model(pin_input->view, true);
}

void desktop_view_pin_input_update_telemetry(DesktopViewPinInput* pin_input, uint8_t current_fail_count, uint8_t max_allowed_attempts) {
    furi_assert(pin_input);
    DesktopViewPinInputModel* model = view_get_model(pin_input->view);
    model->current_fail_count = current_fail_count;
    model->max_allowed_attempts = max_allowed_attempts;
    view_commit_model(pin_input->view, true);
}

void desktop_view_pin_input_set_label_button(DesktopViewPinInput* pin_input, const char* label) {
    furi_assert(pin_input);
    DesktopViewPinInputModel* model = view_get_model(pin_input->view);
    model->button_label = label;
    view_commit_model(pin_input->view, true);
}

void desktop_view_pin_input_set_label_primary(DesktopViewPinInput* pin_input, uint8_t x, uint8_t y, const char* label) {
    furi_assert(pin_input);
    DesktopViewPinInputModel* model = view_get_model(pin_input->view);
    
    // Intercept "Alphanumeric PIN" text from Desktop Service and rewrite it cleanly
    if(label && strcmp(label, "Alphanumeric PIN") == 0) {
        model->primary_str = "Enter PIN:";
    } else {
        model->primary_str = label;
    }
    
    model->primary_str_x = x;
    model->primary_str_y = y;
    view_commit_model(pin_input->view, true);
}

void desktop_view_pin_input_set_context(DesktopViewPinInput* pin_input, void* context) {
    furi_assert(pin_input);
    pin_input->context = context;
}

void desktop_view_pin_input_set_timeout_callback(DesktopViewPinInput* pin_input, DesktopViewPinInputCallback callback) {
    furi_assert(pin_input);
    pin_input->timeout_callback = callback;
}

void desktop_view_pin_input_set_back_callback(DesktopViewPinInput* pin_input, DesktopViewPinInputCallback callback) {
    furi_assert(pin_input);
    pin_input->back_callback = callback;
}

void desktop_view_pin_input_set_done_callback(DesktopViewPinInput* pin_input, DesktopViewPinInputDoneCallback callback) {
    furi_assert(pin_input);
    pin_input->done_callback = callback;
}

View* desktop_view_pin_input_get_view(DesktopViewPinInput* pin_input) {
    furi_assert(pin_input);
    return pin_input->view;
}

void desktop_view_pin_input_set_label_secondary(DesktopViewPinInput* pin_input, uint8_t x, uint8_t y, const char* label) {
    UNUSED(pin_input);
    UNUSED(x);
    UNUSED(y);
    UNUSED(label);
}

void desktop_view_pin_input_set_label_tertiary(DesktopViewPinInput* pin_input, uint8_t x, uint8_t y, const char* label) {
    UNUSED(pin_input);
    UNUSED(x);
    UNUSED(y);
    UNUSED(label);
}

void desktop_view_pin_input_set_pin_position(DesktopViewPinInput* pin_input, uint8_t x, uint8_t y) {
    UNUSED(pin_input);
    UNUSED(x);
    UNUSED(y);
}