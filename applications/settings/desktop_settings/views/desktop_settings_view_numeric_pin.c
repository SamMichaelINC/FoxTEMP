#include <gui/view.h>
#include <gui/canvas.h>
#include <stdint.h>
#include <core/check.h>

#define KEY_ROWS 4
#define KEY_COLS 3
#define KEY_WIDTH 36
#define KEY_HEIGHT 10
#define GRID_START_X 8
#define GRID_START_Y 16
#define X_GAP 4
#define Y_GAP 2

static const char* const alpha_subtext_map[12] = {
    "",     "ABC",  "DEF",  
    "GHI",  "JKL",  "MNO",  
    "PQRS", "TUV",  "WXYZ", 
    "CLEAR", "",    "ENTER" 
};

static const char* const main_text_map[12] = {
    "1", "2", "3",
    "4", "5", "6",
    "7", "8", "9",
    "DEL", "0", "ENT"
};

typedef struct {
    uint8_t selected_row;
    uint8_t selected_col;
    uint8_t pin_length;
    uint8_t pin_buffer[10];
} NumericPinModel;

void desktop_settings_view_numeric_pin_draw_callback(Canvas* canvas, void* _model) {
    furi_assert(canvas);
    furi_assert(_model);
    NumericPinModel* model = _model;

    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);

    // 1. Draw Top Status Header Row cleanly without ANY snprintf or FuriString symbols
    canvas_set_font(canvas, FontSecondary);
    
    // Construct text using array assignment arithmetic
    char header_string[] = "Enter PIN:  /10 Digits";
    if(model->pin_length == 10) {
        header_string[11] = '1';
        header_string[12] = '0';
    } else {
        header_string[11] = '0' + (model->pin_length % 10);
        header_string[12] = ' '; // Overwrite structural space padding cleanly
    }
    canvas_draw_str(canvas, 4, 10, header_string);

    // 2. Loop through and draw the custom Alpha-Grid Layout (Y: 16 - 64)
    for(uint8_t row = 0; row < KEY_ROWS; row++) {
        for(uint8_t col = 0; col < KEY_COLS; col++) {
            uint8_t index = (row * KEY_COLS) + col;

            int16_t box_x = GRID_START_X + (col * (KEY_WIDTH + X_GAP));
            int16_t box_y = GRID_START_Y + (row * (KEY_HEIGHT + Y_GAP));

            if(row == model->selected_row && col == model->selected_col) {
                canvas_set_color(canvas, ColorBlack);
                canvas_draw_box(canvas, box_x, box_y, KEY_WIDTH, KEY_HEIGHT);
                canvas_set_color(canvas, ColorWhite);
            } else {
                canvas_set_color(canvas, ColorBlack);
                canvas_draw_rframe(canvas, box_x, box_y, KEY_WIDTH, KEY_HEIGHT, 1);
            }

            canvas_set_font(canvas, FontSecondary);
            if(index == 9 || index == 11) { 
                canvas_draw_str_aligned(canvas, box_x + (KEY_WIDTH / 2), box_y + (KEY_HEIGHT / 2), AlignCenter, AlignCenter, main_text_map[index]);
            } else {
                canvas_draw_str_aligned(canvas, box_x + (KEY_WIDTH / 2) - 6, box_y + (KEY_HEIGHT / 2) + 1, AlignCenter, AlignCenter, main_text_map[index]);
                
                canvas_set_font(canvas, FontKeyboard);
                canvas_draw_str_aligned(canvas, box_x + (KEY_WIDTH / 2) + 6, box_y + (KEY_HEIGHT / 2) + 2, AlignCenter, AlignCenter, alpha_subtext_map[index]);
            }
        }
    }
}