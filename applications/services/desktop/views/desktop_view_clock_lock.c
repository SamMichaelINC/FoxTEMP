#include "desktop_view_clock_lock.h"
#include <furi.h>
#include <furi_hal.h>
#include <gui/elements.h>
 
typedef struct {
    uint8_t hour;
    uint8_t minute;
} ClockLockModel;
 
struct DesktopClockLockView {
    View* view;
    DesktopClockLockViewCallback callback;
    void* context;
    FuriTimer* timer;
};
 
static void desktop_clock_lock_timer_callback(void* context) {
    DesktopClockLockView* clock_lock = context;
 
    DateTime dt;
    furi_hal_rtc_get_datetime(&dt);
 
    with_view_model(
        clock_lock->view,
        ClockLockModel* model,
        {
            model->hour = dt.hour;
            model->minute = dt.minute;
        },
        true);
}
 
static void desktop_clock_lock_draw_callback(Canvas* canvas, void* model) {
    ClockLockModel* m = model;
 
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", m->hour, m->minute);
 
    canvas_set_font(canvas, FontBigNumbers);
    canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, buffer);
}
 
static bool desktop_clock_lock_input_callback(InputEvent* event, void* context) {
    DesktopClockLockView* clock_lock = context;
 
    if(event->type == InputTypeLong && event->key == InputKeyDown) {
        if(clock_lock->callback) {
            clock_lock->callback(clock_lock->context);
        }
        return true;
    }
 
    return true;
}
 
static void desktop_clock_lock_enter_callback(void* context) {
    DesktopClockLockView* clock_lock = context;
 
    DateTime dt;
    furi_hal_rtc_get_datetime(&dt);
 
    with_view_model(
        clock_lock->view,
        ClockLockModel* model,
        {
            model->hour = dt.hour;
            model->minute = dt.minute;
        },
        true);
 
    furi_timer_start(clock_lock->timer, furi_ms_to_ticks(1000));
}
 
static void desktop_clock_lock_exit_callback(void* context) {
    DesktopClockLockView* clock_lock = context;
    furi_timer_stop(clock_lock->timer);
}
 
DesktopClockLockView* desktop_clock_lock_alloc(void) {
    DesktopClockLockView* clock_lock = malloc(sizeof(DesktopClockLockView));
 
    clock_lock->callback = NULL;
    clock_lock->context = NULL;
 
    clock_lock->view = view_alloc();
    view_set_context(clock_lock->view, clock_lock);
    view_allocate_model(clock_lock->view, ViewModelTypeLocking, sizeof(ClockLockModel));
    view_set_draw_callback(clock_lock->view, desktop_clock_lock_draw_callback);
    view_set_input_callback(clock_lock->view, desktop_clock_lock_input_callback);
    view_set_enter_callback(clock_lock->view, desktop_clock_lock_enter_callback);
    view_set_exit_callback(clock_lock->view, desktop_clock_lock_exit_callback);
 
    clock_lock->timer = furi_timer_alloc(
        desktop_clock_lock_timer_callback, FuriTimerTypePeriodic, clock_lock);
 
    return clock_lock;
}
 
void desktop_clock_lock_free(DesktopClockLockView* clock_lock) {
    furi_assert(clock_lock);
    furi_timer_free(clock_lock->timer);
    view_free(clock_lock->view);
    free(clock_lock);
}
 
View* desktop_clock_lock_get_view(DesktopClockLockView* clock_lock) {
    return clock_lock->view;
}
 
void desktop_clock_lock_set_callback(
    DesktopClockLockView* clock_lock,
    DesktopClockLockViewCallback callback,
    void* context) {
    clock_lock->callback = callback;
    clock_lock->context = context;
}