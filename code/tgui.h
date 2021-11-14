#ifndef TGUI_H
#define TGUI_H

#include <stdint.h>
#include <assert.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint32_t b32;

typedef float f32;
typedef double f64;

#define true 1
#define false 0
#define ASSERT(value) assert(value);
#define UNUSED_VAR(x) ((void)x)
#define OFFSET_OFF(s, p) (u64)(&(((s *)0)->p))
#define TGUI_API __declspec(dllexport)

// NOTE: color pallete
#define TGUI_BLACK  0xFF323031
#define TGUI_ORANGE 0xFFF4AC45
#define TGUI_RED    0xFFFF5154
#define TGUI_GREY   0xFF8896AB
#define TGUI_GREEN  0xFFC4EBC8

typedef struct TGuiV2
{
    f32 x;
    f32 y;
} TGuiV2;

typedef union TGuiRect
{
    struct
    {
        f32 x;
        f32 y;
        f32 width;
        f32 height;
    };
    struct
    {
        TGuiV2 pos;
        TGuiV2 dim;
    };
} TGuiRect;

typedef struct TGuiBitmap
{
    u32 *pixels;
    u32 width;
    u32 height;
} TGuiBitmap;

// NOTE: for now, only support for bitmaps fonts
typedef struct TGuiFont
{
    TGuiBitmap *bitmap;
    TGuiRect src_rect;
    u32 num_rows;
    u32 num_cols;
    // TODO: for now num_cols, src_rect.x and src_rect.y arent used
    // maybe remove them
} TGuiFont;

typedef enum TGuiEventType
{
    TGUI_EVENT_MOUSEMOVE,
    TGUI_EVENT_MOUSEDOWN,
    TGUI_EVENT_MOUSEUP,
    TGUI_EVENT_KEYDOWN,
    TGUI_EVENT_KEYUP,

    TGUI_EVENT_COUNT,
} TGuiEventType;

typedef struct TGuiEventMouseMove
{
    TGuiEventType type;
    i32 pos_x;
    i32 pos_y;
} TGuiEventMouseMove;

typedef union TGuiEvent
{
    TGuiEventType type;
    TGuiEventMouseMove mouse;
} TGuiEvent;

typedef enum TGuiDrawCommandType
{
    TGUI_DRAWCMD_CLEAR,
    TGUI_DRAWCMD_RECT,
    TGUI_DRAWCMD_BITMAP,
    TGUI_DRAWCMD_TEXT,
    
    TGUI_DRAWCMD_COUNT,
} TGuiDrawCommandType;

typedef struct TGuiDrawCommand 
{
    TGuiDrawCommandType type;
    TGuiRect descriptor;
    TGuiBitmap *bitmap;
    u32 color;
    char *text;
} TGuiDrawCommand;

// TODO: make container structs for this queues, like std::vector<> in c++
#define TGUI_DRAW_COMMANDS_MAX 128
typedef struct TGuiDrawCommandBuffer
{
    TGuiDrawCommand buffer[TGUI_DRAW_COMMANDS_MAX];
    u32 head;
    u32 count;
} TGuiDrawCommandBuffer;

#define TGUI_EVENT_QUEUE_MAX 128
typedef struct TGuiEventQueue
{
    TGuiEvent queue[TGUI_EVENT_QUEUE_MAX];
    u32 count;
} TGuiEventQueue;

typedef struct TGuiState
{
    TGuiBitmap *backbuffer;
    TGuiFont *font;
    u32 font_height;

    TGuiDrawCommandBuffer draw_command_buffer;
    TGuiEventQueue event_queue;
    
    i32 mouse_x;
    i32 mouse_y;
    i32 last_mouse_x;
    i32 last_mouse_y;
    b32 mouse_up;
    b32 mouse_down;
    b32 mouse_is_down;
} TGuiState;
// TODO: Maybe the state should be provided by the application?
// NOTE: global state (stores all internal state of the GUI)
extern TGuiState tgui_global_state;

// NOTE: core lib functions
TGUI_API void tgui_init(TGuiBitmap *backbuffer, TGuiFont *font);
TGUI_API void tgui_update(void);
TGUI_API void tgui_draw_command_buffer(void);
TGUI_API void tgui_push_event(TGuiEvent event);
TGUI_API void tgui_push_draw_command(TGuiDrawCommand draw_cmd);
TGUI_API b32 tgui_pull_draw_command(TGuiDrawCommand *draw_cmd);

// NOTE: API utilities
// TODO: make this funtions inline
TGUI_API TGuiV2 tgui_v2(f32 x, f32 y);
TGUI_API TGuiV2 tgui_v2_sub(TGuiV2 a, TGuiV2 b);
TGUI_API f32 tgui_v2_dot(TGuiV2 a, TGuiV2 b);
TGUI_API f32 tgui_v2_lenght_sqrt(TGuiV2 v);
TGUI_API f32 tgui_v2_lenght(TGuiV2 v);
TGUI_API TGuiV2 tgui_v2_normalize(TGuiV2 v);

TGUI_API TGuiRect tgui_rect_xywh(f32 x, f32 y, f32 width, f32 height);
TGUI_API b32 tgui_point_inside_rect(TGuiV2 point, TGuiRect rect);

// NOTE: DEBUG function
TGuiBitmap tgui_debug_load_bmp(char *path);
void tgui_debug_free_bmp(TGuiBitmap *bitmap);

// NOTE: simple render API function
TGUI_API void tgui_clear_backbuffer(TGuiBitmap *backbuffer);
TGUI_API void tgui_draw_circle_aa(TGuiBitmap *backbuffer, i32 x, i32 y, u32 color, u32 radius);
TGUI_API void tgui_draw_rect(TGuiBitmap *backbuffer, i32 min_x, i32 min_y, i32 max_x, i32 max_y, u32 color);
TGUI_API void tgui_draw_rounded_rect(TGuiBitmap *backbuffer, i32 min_x, i32 min_y, i32 max_x, i32 max_y, u32 color, u32 radius);
TGUI_API void tgui_copy_bitmap(TGuiBitmap *backbuffer, TGuiBitmap *bitmap, i32 x, i32 y);
TGUI_API void tgui_draw_bitmap(TGuiBitmap *backbuffer, TGuiBitmap *bitmap, i32 x, i32 y, i32 width, i32 height);
TGUI_API void tgui_draw_src_dest_bitmap(TGuiBitmap *backbuffer, TGuiBitmap *bitmap, TGuiRect src, TGuiRect dest);
// NOTE: font funtions
// TODO: stop using char * (null terminated string) create custom string_view like struct
// NOTE: height is in pixels
TGUI_API TGuiFont tgui_create_font(TGuiBitmap *bitmap, u32 char_width, u32 char_height, u32 num_rows, u32 num_cols);
TGUI_API void tgui_draw_char(TGuiBitmap *backbuffer, TGuiFont *font, u32 height, i32 x, i32 y, char character);
TGUI_API void tgui_draw_text(TGuiBitmap *backbuffer, TGuiFont *font, u32 height, i32 x, i32 y, char *text);
TGUI_API u32 tgui_get_text_width(TGuiFont *font, char *text, u32 height);

#endif // TGUI_WIN32_H
