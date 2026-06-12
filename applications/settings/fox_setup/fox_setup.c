#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/elements.h>
#include <input/input.h>

// Define our Wizard Pages
typedef enum {
    Page1_Welcome = 1,
    Page2_RenamePrompt,
    Page3_RenameAction,
    Page4_PinPrompt,
    Page5_PinAction,
    Page6_Complete
} FoxSetupPage;

// Application State Model
typedef struct {
    FoxSetupPage current_page;
    FuriMutex* mutex;
} FoxSetupModel;

// Input Event Types
typedef enum {
    EventTypeTick,
    EventTypeKey,
} EventType;

typedef struct {
    EventType type;
    InputEvent input;
} AppEvent;

// --- DRAW CALLBACK ---
static void draw_callback(Canvas* canvas, void* ctx) {
    FoxSetupModel* model = ctx;
    furi_mutex_acquire(model->mutex, FuriWaitForever);

    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);

    switch(model->current_page) {
        case Page1_Welcome:
            // Placeholder for the Fox logo on the left (e.g., 48x64 area)
            canvas_draw_frame(canvas, 0, 0, 48, 64);
            canvas_set_font(canvas, FontSecondary);
            elements_multiline_text_aligned(canvas, 24, 32, AlignCenter, AlignCenter, "FOX\nLOGO");

            // Bold Welcome Text on the right
            canvas_set_font(canvas, FontPrimary);
            elements_multiline_text_aligned(canvas, 88, 28, AlignCenter, AlignCenter, "Welcome\nto\nFoxFW v2.0");

            // Next button bottom right
            elements_button_right(canvas, "Next");
            break;

        case Page2_RenamePrompt:
            // Top Bold Text
            canvas_set_font(canvas, FontPrimary);
            char welcome_str[64];
            snprintf(welcome_str, sizeof(welcome_str), "Welcome, %s,", furi_hal_version_get_name_ptr() ? furi_hal_version_get_name_ptr() : "User");
            canvas_draw_str_aligned(canvas, 64, 15, AlignCenter, AlignCenter, welcome_str);

            // Standard Prompt Text
            canvas_set_font(canvas, FontSecondary);
            elements_multiline_text_aligned(canvas, 64, 35, AlignCenter, AlignCenter, "Would you like to rename\nyour flipper?");

            // Buttons
            elements_button_left(canvas, "Skip");
            elements_button_right(canvas, "Next");
            break;

        case Page3_RenameAction:
            // Placeholder: This is where you'll invoke the Flipper OS rename scene
            canvas_set_font(canvas, FontPrimary);
            canvas_draw_str_aligned(canvas, 64, 25, AlignCenter, AlignCenter, "Renaming...");
            canvas_set_font(canvas, FontSecondary);
            canvas_draw_str_aligned(canvas, 64, 40, AlignCenter, AlignCenter, "(Press OK/Back to return)");
            break;

        case Page4_PinPrompt:
            canvas_set_font(canvas, FontPrimary);
            canvas_draw_str_aligned(canvas, 64, 15, AlignCenter, AlignCenter, "PIN SECURITY");

            canvas_set_font(canvas, FontSecondary);
            elements_multiline_text_aligned(canvas, 64, 35, AlignCenter, AlignCenter, "Protect your flipper with\na PIN? (RECOMMENDED!)");

            elements_button_left(canvas, "Skip");
            elements_button_right(canvas, "Next");
            break;

        case Page5_PinAction:
            // Placeholder: This is where you'll invoke the Flipper OS PIN keypad scene
            canvas_set_font(canvas, FontPrimary);
            canvas_draw_str_aligned(canvas, 64, 25, AlignCenter, AlignCenter, "PIN Setup...");
            canvas_set_font(canvas, FontSecondary);
            canvas_draw_str_aligned(canvas, 64, 40, AlignCenter, AlignCenter, "(Press OK/Back to return)");
            break;

        case Page6_Complete:
            canvas_set_font(canvas, FontPrimary);
            canvas_draw_str_aligned(canvas, 64, 20, AlignCenter, AlignCenter, "Setup Complete!");

            canvas_set_font(canvas, FontSecondary);
            canvas_draw_str_aligned(canvas, 64, 40, AlignCenter, AlignCenter, "Please join our Discord!");

            elements_button_right(canvas, "Finish");
            break;
    }

    furi_mutex_release(model->mutex);
}

// --- INPUT CALLBACK ---
static void input_callback(InputEvent* input_event, void* ctx) {
    FuriMessageQueue* event_queue = ctx;
    AppEvent event = {.type = EventTypeKey, .input = *input_event};
    furi_message_queue_put(event_queue, &event, FuriWaitForever);
}

// --- MAIN ENTRY POINT ---
int32_t fox_setup_app(void* p) {
    UNUSED(p);

    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(AppEvent));
    FoxSetupModel* model = malloc(sizeof(FoxSetupModel));
    model->current_page = Page1_Welcome;
    model->mutex = furi_mutex_alloc(FuriMutexTypeNormal);

    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, draw_callback, model);
    view_port_input_callback_set(view_port, input_callback, event_queue);

    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    AppEvent event;
    bool running = true;

    while(running) {
        if(furi_message_queue_get(event_queue, &event, 100) == FuriStatusOk) {
            if(event.type == EventTypeKey && event.input.type == InputTypeShort) {
                furi_mutex_acquire(model->mutex, FuriWaitForever);

                switch(model->current_page) {
                    case Page1_Welcome:
                        if(event.input.key == InputKeyBack) {
                            running = false; // Closes whole app
                        } else if(event.input.key == InputKeyOk || event.input.key == InputKeyRight) {
                            model->current_page = Page2_RenamePrompt;
                        }
                        break;

                    case Page2_RenamePrompt:
                        if(event.input.key == InputKeyBack || event.input.key == InputKeyLeft) {
                            model->current_page = Page4_PinPrompt; // Skip to PIN
                        } else if(event.input.key == InputKeyOk || event.input.key == InputKeyRight) {
                            model->current_page = Page3_RenameAction; // Enter Rename
                        }
                        break;

                    case Page3_RenameAction:
                        // Simulated return logic from the external settings app
                        if(event.input.key == InputKeyOk || event.input.key == InputKeyBack) {
                            model->current_page = Page4_PinPrompt;
                        }
                        break;

                    case Page4_PinPrompt:
                        if(event.input.key == InputKeyBack || event.input.key == InputKeyLeft) {
                            model->current_page = Page6_Complete; // Skip to Finish
                        } else if(event.input.key == InputKeyOk || event.input.key == InputKeyRight) {
                            model->current_page = Page5_PinAction; // Enter PIN setup
                        }
                        break;

                    case Page5_PinAction:
                        // Simulated return logic from external keypad app
                        if(event.input.key == InputKeyOk || event.input.key == InputKeyBack) {
                            model->current_page = Page6_Complete;
                        }
                        break;

                    case Page6_Complete:
                        if(event.input.key == InputKeyBack || event.input.key == InputKeyOk || event.input.key == InputKeyRight) {
                            running = false; // Setup complete, exit app cleanly
                        }
                        break;
                }

                furi_mutex_release(model->mutex);
                view_port_update(view_port);
            }
        }
    }

    // Cleanup
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_message_queue_free(event_queue);
    furi_mutex_free(model->mutex);
    free(model);
    furi_record_close(RECORD_GUI);

    return 0;
}