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
#define TGUI_DRAK_BLACK  0xFF282728
#define TGUI_BLACK       0xFF323031
#define TGUI_ORANGE      0xFFF4AC45
#define TGUI_RED         0xFFFF5154
#define TGUI_GREY        0xFF8896AB
#define TGUI_GREEN       0xFFC4EBC8

// NOTE: handle to GUI widgets
typedef u32 TGuiHandle;
#define TGUI_INVALID_HANDLE 0

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
    u32 pitch;
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
    // NOTE: GUI events
    TGUI_EVEN_BUTTON,
    TGUI_EVEN_FOCUS,

    // NOTE: externals events
    TGUI_EVENT_MOUSEMOVE,
    TGUI_EVENT_MOUSEDOWN,
    TGUI_EVENT_MOUSEUP,
    TGUI_EVENT_KEYDOWN,
    TGUI_EVENT_KEYUP,

    TGUI_EVENT_COUNT,
} TGuiEventType;

typedef struct TGuiEventButton
{
    TGuiEventType type;
    TGuiHandle handle;
} TGuiEventButton;

typedef struct TGuiEventFocus
{
    TGuiEventType type;
    TGuiHandle handle;
} TGuiEventFocus;

typedef struct TGuiEventMouseMove
{
    TGuiEventType type;
    i32 pos_x;
    i32 pos_y;
} TGuiEventMouseMove;

typedef union TGuiEvent
{
    TGuiEventType type;
    
    // NOTE: GUI events
    TGuiEventButton button;
    TGuiEventFocus focus;
    
    // NOTE: externals events
    TGuiEventMouseMove mouse;
} TGuiEvent;

#define TGUI_EVENT_QUEUE_MAX 128
typedef struct TGuiEventQueue
{
    TGuiEvent queue[TGUI_EVENT_QUEUE_MAX];
    u32 count;
} TGuiEventQueue;

typedef enum TGuiDrawCommandType
{
    TGUI_DRAWCMD_CLEAR,
    TGUI_DRAWCMD_START_CLIPPING,
    TGUI_DRAWCMD_END_CLIPPING,
    TGUI_DRAWCMD_RECT,
    TGUI_DRAWCMD_ROUNDED_RECT,
    TGUI_DRAWCMD_BITMAP,
    TGUI_DRAWCMD_TEXT,
    
    TGUI_DRAWCMD_COUNT,
} TGuiDrawCommandType;

typedef struct TGuiDrawCommand 
{
    TGuiDrawCommandType type;
    TGuiRect descriptor;
    TGuiBitmap *bitmap;
    u32 ratio;
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

typedef enum TGuiWidgetType
{
    TGUI_CONTAINER,
    TGUI_END_CONTAINER,
    TGUI_BUTTON,
    TGUI_CHECKBOX,
    TGUI_SLIDER,
    
    TGUI_COUNT,
} TGuiWidgetType;

typedef enum TGuiLayoutType
{
    TGUI_LAYOUT_NONE,
    TGUI_LAYOUT_HORIZONTAL,
    TGUI_LAYOUT_VERTICAL,
    
    TGUI_LAYOUT_COUNT,
} TGuiLayoutType;

typedef enum TGuiContanerFlags
{
    TGUI_CONTAINER_DYNAMIC   = 1 << 0,
    TGUI_CONTAINER_V_SCROLL  = 1 << 1,
    TGUI_CONTAINER_H_SCROLL  = 1 << 2,
    TGUI_CONTAINER_DRAGGABLE = 1 << 3,
    TGUI_CONTAINER_RESIZABLE = 1 << 4,
} TGuiContanerFlags;

typedef struct TGuiWidgetLayout
{
    TGuiLayoutType type;
    u32 padding;
} TGuiWidgetLayout;

typedef struct TGuiText
{
    TGuiV2 size;
    char *text;
} TGuiText;

typedef struct TGuiWidgetHeader
{
    TGuiHandle handle;

    // NOTE: widget are in a tree 
    TGuiHandle parent;
    TGuiHandle child_first;
    TGuiHandle child_last;
    TGuiHandle sibling_next;
    TGuiHandle sibling_prev;
    
    TGuiWidgetType type;
    TGuiV2 size;
    TGuiV2 position;
    
} TGuiWidgetHeader;

typedef struct TGuiWidgetContainer
{
    TGuiWidgetHeader header;
    //----------------------
    TGuiContanerFlags flags;
    TGuiWidgetLayout layout;
    TGuiV2 dimension;
    TGuiV2 total_dimension;
    TGuiRect vertical_grip;
    TGuiRect horizontal_grip;
    f32 vertical_value;
    f32 horizontal_value;
    b32 grabbing_x;
    b32 grabbing_y;
    b32 dragging;
    b32 visible;
    b32 hot;
} TGuiWidgetContainer;

typedef struct TGuiWidgetButton
{
    TGuiWidgetHeader header;
    //----------------------
    b32 hot;
    b32 active;
    b32 pressed;
    TGuiText text;
} TGuiWidgetButton;

typedef struct TGuiWidgetCheckBox
{
    TGuiWidgetHeader header;
    //----------------------
    b32 hot;
    b32 active;
    b32 checked;
    TGuiText text;
    TGuiV2 box_dimension;
} TGuiWidgetCheckBox;

typedef struct TGuiWidgetSlider
{
    TGuiWidgetHeader header;
    //----------------------
    b32 hot;
    b32 active;
    f32 value;
    f32 ratio;
    TGuiV2 grip_dimension;
} TGuiWidgetSlider;

typedef union TGuiWidget
{
    TGuiWidgetHeader header;
    //----------------------
    TGuiWidgetContainer container;
    TGuiWidgetButton button;
    TGuiWidgetCheckBox checkbox;
    TGuiWidgetSlider slider;
} TGuiWidget;

typedef struct TGuiWidgetFree
{
    TGuiHandle handle;
    struct TGuiWidgetFree *next;
} TGuiWidgetFree;

#define TGUI_DEFAULT_POOL_SIZE 8
typedef struct TGuiWidgetPoolAllocator
{
    TGuiWidget *buffer;
    u32 buffer_size;
    u32 count;
    TGuiWidgetFree *free_list;
} TGuiWidgetPoolAllocator;

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

    TGuiWidgetPoolAllocator widget_allocator;
    TGuiHandle first_root;
    TGuiHandle last_root;

    TGuiHandle widget_active;
} TGuiState;
// TODO: Maybe the state should be provided by the application?
// NOTE: global state (stores all internal state of the GUI)
extern TGuiState tgui_global_state;

//-----------------------------------------------------
// NOTE: GUI lib functions
//-----------------------------------------------------
typedef b32 (*TGuiWidgetFP)(TGuiHandle handle);

TGUI_API TGuiHandle tgui_create_container(i32 x, i32 y, i32 width, i32 height, TGuiContanerFlags flags, TGuiLayoutType layout, b32 visible, u32 padding);
TGUI_API TGuiHandle tgui_create_button(char *label);
TGUI_API TGuiHandle tgui_create_checkbox(char *label);
TGUI_API TGuiHandle tgui_create_slider(void);
TGUI_API void tgui_container_add_widget(TGuiHandle container_handle, TGuiHandle widget_handle);
TGUI_API void tgui_widget_to_root(TGuiHandle widget_handle);
TGUI_API void tgui_set_widget_position(TGuiHandle widget_handle, f32 x, f32 y);

b32 tgui_widget_update(TGuiHandle handle);
b32 tgui_widget_render(TGuiHandle handle);
b32 tgui_widget_recursive_descent_pre_first_to_last(TGuiHandle handle, TGuiWidgetFP function);
b32 tgui_widget_recursive_descent_pos_first_to_last(TGuiHandle handle, TGuiWidgetFP function);
b32 tgui_widget_recursive_descent_pre_last_to_first(TGuiHandle handle, TGuiWidgetFP function);
b32 tgui_widget_recursive_descent_pos_last_to_first(TGuiHandle handle, TGuiWidgetFP function);
TGuiV2 tgui_widget_abs_pos(TGuiHandle handle);

//-----------------------------------------------------
// NOTE: core lib functions
//-----------------------------------------------------
TGUI_API void tgui_init(TGuiBitmap *backbuffer, TGuiFont *font);
TGUI_API void tgui_terminate(void);
TGUI_API void tgui_update(void);
TGUI_API void tgui_draw_command_buffer(void);
TGUI_API void tgui_push_event(TGuiEvent event);
TGUI_API void tgui_push_draw_command(TGuiDrawCommand draw_cmd);
TGUI_API b32 tgui_pull_draw_command(TGuiDrawCommand *draw_cmd);

//-----------------------------------------------------
//  NOTE: memory management functions
//-----------------------------------------------------
void tgui_widget_allocator_create(TGuiWidgetPoolAllocator *allocator);
void tgui_widget_allocator_destroy(TGuiWidgetPoolAllocator *allocator);
TGuiHandle tgui_widget_allocator_pool(TGuiWidgetPoolAllocator *allocator);
void tgui_widget_allocator_free(TGuiWidgetPoolAllocator *allocator, TGuiHandle *handle);
void tgui_widget_set(TGuiHandle handle, TGuiWidget widget);
TGuiWidget *tgui_widget_get(TGuiHandle handle);

TGUI_API TGuiRect tgui_rect_xywh(f32 x, f32 y, f32 width, f32 height);
TGUI_API b32 tgui_point_inside_rect(TGuiV2 point, TGuiRect rect);

//-----------------------------------------------------
// NOTE: DEBUG function
//-----------------------------------------------------
TGuiBitmap tgui_debug_load_bmp(char *path);
void tgui_debug_free_bmp(TGuiBitmap *bitmap);

//-----------------------------------------------------
// NOTE: simple render API function
//-----------------------------------------------------

#define TGUI_DEFAULT_CLIPPING_STACK_SIZE 4
typedef struct TGuiClippingStack
{
    TGuiRect *buffer;
    u32 buffer_size;
    u32 top;
} TGuiClippingStack;
void tgui_clipping_stack_create(TGuiClippingStack *stack);
void tgui_clipping_stack_destoy(TGuiClippingStack *stack);
void tgui_clipping_stack_push(TGuiClippingStack *stack, TGuiRect clipping);
TGuiRect tgui_clipping_stack_pop(TGuiClippingStack *stack);
TGuiRect tgui_clipping_stack_top(TGuiClippingStack *stack);

// TODO: maybe create a renderer struct to save info like this
extern TGuiClippingStack global_clipping_stack;

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
TGUI_API u32 tgui_text_get_width(TGuiFont *font, char *text, u32 height);

#endif // TGUI_WIN32_H
