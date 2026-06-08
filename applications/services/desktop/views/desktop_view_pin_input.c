#include <gui/canvas.h>
#include <furi.h>
#include <gui/view.h>
#include <gui/elements.h>
#include <assets_icons.h>
#include <stdint.h>

#include "desktop_view_pin_input.h"

#define NO_ACTIVITY_TIMEOUT 15000

#define MIN_PIN_LENGTH DESKTOP_PIN_CODE_MIN_LEN
#define MAX_PIN_LENGTH DESKTOP_PIN_CODE_MAX_LEN

/* 12-key linear phone layout map containing digits, backspace, and confirmation */
static const char telephone_row[12] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '<', 'O'};

/* Traditional telephone letter mapping sub-labels */
static const char* telephone_alphas[10] = {
    "", "ABC", "DEF", "GHI", "JKL", "MNO", "PQRS", "TUV", "WXYZ", ""
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
    
    /* Horizontal selection index within the layout line array */
    uint8_t select_col;
    
    uint8_t current_fail_count;
    uint8_t max_allowed_attempts;

    const char* primary_str;
    uint8_t primary_str_x;
    uint8_t primary_str_y;
    const char* button_label;
} DesktopViewPinInputModel;

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
        case InputKeyLeft:
            if(!model->locked_input && model->select_col > 0) model->select_col--;
            break;
        case InputKeyRight:
            if(!model->locked_input && model->select_col < 11) model->select_col++;
            break;
        case InputKeyOk:
            if(!model->locked_input) {
                char selected_char = telephone_row[model->select_col];
                
                if(selected_char == 'O') {
                    if(model->pin.length >= MIN_PIN_LENGTH) {
                        call_done_callback = true;
                        pin_code = model->pin;
                    }
                } else if(selected_char == '<') {
                    if(model->pin.length > 0) {
                        model->pin.length--;
                        model->pin.data[model->pin.length] = '\0';
                    } else {
                        call_back_callback = true;
                    }
                } else {
                    if(model->pin.length < MAX_PIN_LENGTH) {
                        model->pin.data[model->pin.length] = selected_char;
                        model->pin.length++;
                        model->pin.data[model->pin.length] = '\0';
                    }
                }
            }
            break;
        case InputKeyBack:
            if(!model->locked_input) {
                if(model->pin.length > 0) {
                    model->pin.length--;
                    model->pin.data[model->pin.length] = '\0';
                } else {
                    call_back_callback = true;
                }
            }
            break;
        default:
            break;
        }
    }

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
    
    /* 1. Dynamic Primary Message Positioning */
    if(model->primary_str) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 4, AlignCenter, AlignTop, model->primary_str);
    }
    
    /* 2. Masked Entry Value String Formatting Output */
    canvas_set_font(canvas, FontPrimary);
    char display_buffer[MAX_PIN_LENGTH + 1] = {0};
    for(uint8_t i = 0; i < model->pin.length; i++) {
        display_buffer[i] = model->pin_hidden ? '*' : model->pin.data[i];
    }
    canvas_draw_str_aligned(canvas, 64, 18, AlignCenter, AlignTop, display_buffer);

    /* 3. Draw Clean Keypad Row Layout */
    for(uint8_t col = 0; col < 12; col++) {
        uint8_t x = 2 + (col * 10);
        uint8_t y = 44;
        
        if(col == model->select_col) {
            canvas_draw_box(canvas, x - 1, y - 8, 9, 11);
            canvas_set_color(canvas, ColorWhite);
        }
        
        char char_str[2] = {telephone_row[col], '\0'};
        
        /* Handle formatting display adjustments for navigation action indicators */
        if(telephone_row[col] == '<') {
            canvas_set_font(canvas, FontSecondary);
            canvas_draw_str(canvas, x, y, "<-");
        } else if(telephone_row[col] == 'O') {
            canvas_set_font(canvas, FontSecondary);
            canvas_draw_str(canvas, x, y, "OK");
        } else {
            canvas_set_font(canvas, FontPrimary);
            canvas_draw_str(canvas, x, y, char_str);
        }
        
        canvas_set_color(canvas, ColorBlack);
        
        /* 4. Render Alphabet Under-Labels for Numeric Targets */
        if(col >= 1 && col <= 8) {
            canvas_set_font(canvas, FontKeyboard);
            canvas_draw_str_aligned(canvas, x + 3, y + 8, AlignCenter, AlignBottom, telephone_alphas[col]);
        }
    }

    /* 5. Warning Attempt Counter Output Layer */
    if(model->current_fail_count > 0) {
        char telemetry_str[32];
        snprintf(telemetry_str, sizeof(telemetry_str), "Attempt %d of %d", 
                 model->current_fail_count, model->max_allowed_attempts);
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 64, 56, AlignCenter, AlignBottom, telemetry_str);
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

    DesktopViewPinInputModel* model = view_get_model(pin_input->view);
    model->pin.length = 0;
    model->select_col = 0;
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
    model->primary_str = label;
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