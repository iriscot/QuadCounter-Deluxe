#include <furi.h>
#include <gui/gui.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>
#include <input/input.h>
#include <stdlib.h>

#define MAX_COUNT 99
#define BOXTIME 2
#define BOXWIDTH 26
#define BOXWIDTH_BIG 42
#define BOXHEIGHT 26
#define MIDDLE_X 64 - BOXWIDTH / 2
#define MIDDLE_X_BIG 64 - BOXWIDTH_BIG / 2
#define MIDDLE_Y 32 - BOXHEIGHT / 2
#define OFFSET_Y 9

typedef struct {
    FuriMessageQueue* input_queue;
    ViewPort* view_port;
    Gui* gui;
    NotificationApp* notification;
    FuriMutex* mutex;
    int count;
    int count2;
    int count3;
    int count4;
    bool exit_app;
} Counter;

const NotificationSequence sequence_vibro = {
    &message_vibro_on,
    &message_delay_10,
    &message_delay_10,
    &message_vibro_off,

    NULL,
};

static void input_callback(InputEvent* input_event, void* ctx) {
    Counter* counter = ctx;
    if(input_event->type == InputTypePress) {
        switch(input_event->key) {
            case InputKeyLeft:
            case InputKeyRight:
            case InputKeyUp:
            case InputKeyDown:
            case InputKeyBack:
                furi_message_queue_put(counter->input_queue, input_event, FuriWaitForever);
                break;
            default:
                break;
        }
    }
}

static void render_callback(Canvas* const canvas, void* ctx) {
    Counter* counter = ctx;
    furi_mutex_acquire(counter->mutex, FuriWaitForever);

    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);

    // Draw fancy borders
    canvas_draw_frame(canvas, 0, 0, 128, 64);
    canvas_draw_line(canvas, 0, 10, 128, 10);
    canvas_draw_str_aligned(canvas, 64, 2, AlignCenter, AlignTop, "QuadCounter");

    // Draw counter labels
    canvas_draw_str_aligned(canvas, 16, 20, AlignCenter, AlignTop, "LT");
    canvas_draw_str_aligned(canvas, 48, 20, AlignCenter, AlignTop, "RT");
    canvas_draw_str_aligned(canvas, 80, 20, AlignCenter, AlignTop, "UP");
    canvas_draw_str_aligned(canvas, 112, 20, AlignCenter, AlignTop, "DN");

    // Draw counters with big font
    canvas_set_font(canvas, FontBigNumbers);
    char str[10];

    snprintf(str, sizeof(str), "%02d", counter->count);
    canvas_draw_str_aligned(canvas, 16, 35, AlignCenter, AlignTop, str);

    snprintf(str, sizeof(str), "%02d", counter->count2);
    canvas_draw_str_aligned(canvas, 48, 35, AlignCenter, AlignTop, str);

    snprintf(str, sizeof(str), "%02d", counter->count3);
    canvas_draw_str_aligned(canvas, 80, 35, AlignCenter, AlignTop, str);

    snprintf(str, sizeof(str), "%02d", counter->count4);
    canvas_draw_str_aligned(canvas, 112, 35, AlignCenter, AlignTop, str);

    furi_mutex_release(counter->mutex);
}

static void process_input(Counter* counter, InputEvent* event) {
    furi_mutex_acquire(counter->mutex, FuriWaitForever);

    switch(event->key) {
        case InputKeyLeft:
            counter->count = (counter->count + 1) % (MAX_COUNT + 1);
            notification_message(counter->notification, &sequence_vibro);
            break;
        case InputKeyRight:
            counter->count2 = (counter->count2 + 1) % (MAX_COUNT + 1);
            notification_message(counter->notification, &sequence_vibro);
            break;
        case InputKeyUp:
            counter->count3 = (counter->count3 + 1) % (MAX_COUNT + 1);
            notification_message(counter->notification, &sequence_vibro);
            break;
        case InputKeyDown:
            counter->count4 = (counter->count4 + 1) % (MAX_COUNT + 1);
            notification_message(counter->notification, &sequence_vibro);
            break;
        case InputKeyBack:
            counter->exit_app = true;
            break;
        default:
            break;
    }

    furi_mutex_release(counter->mutex);
}

void state_free(Counter* counter) {
    notification_message_block(counter->notification, &sequence_display_backlight_enforce_auto);
    gui_remove_view_port(counter->gui, counter->view_port);
    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_NOTIFICATION);
    view_port_free(counter->view_port);
    furi_message_queue_free(counter->input_queue);
    furi_mutex_free(counter->mutex);
    free(counter);
}

int32_t counter_app(void* p __attribute__((unused))) {
    Counter* counter = malloc(sizeof(Counter));
    counter->count = 0;
    counter->count2 = 0;
    counter->count3 = 0;
    counter->count4 = 0;
    counter->exit_app = false;

    counter->input_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    counter->view_port = view_port_alloc();
    counter->gui = furi_record_open(RECORD_GUI);
    counter->notification = furi_record_open(RECORD_NOTIFICATION);
    counter->mutex = furi_mutex_alloc(FuriMutexTypeNormal);

    view_port_input_callback_set(counter->view_port, input_callback, counter);
    view_port_draw_callback_set(counter->view_port, render_callback, counter);

    gui_add_view_port(counter->gui, counter->view_port, GuiLayerFullscreen);

    InputEvent event;
    while(true) {
        if(furi_message_queue_get(counter->input_queue, &event, FuriWaitForever) == FuriStatusOk) {
            if (counter->exit_app) {
                furi_mutex_release(counter->mutex);
                state_free(counter);
                return 0;
            }
            process_input(counter, &event);
            view_port_update(counter->view_port);
        }
    }
}