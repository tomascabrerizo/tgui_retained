#include "tgui.h"
#include "tgui.c"

#include <Windows.h>
#include <stdio.h>
#include <math.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
static HDC global_device_context = 0;
static b32 global_running = false;

// NOTE: backbuffer variables
static BITMAPINFO global_bitmap_info;
static HBITMAP global_bitmap;
static HDC global_backbuffer_dc;
static void *global_backbuffer_data;

static void win32_create_backbuffer(HDC device)
{
    global_bitmap_info.bmiHeader.biSize = sizeof(global_bitmap_info.bmiHeader);
    global_bitmap_info.bmiHeader.biWidth = WINDOW_WIDTH;
    global_bitmap_info.bmiHeader.biHeight = -WINDOW_HEIGHT;
    global_bitmap_info.bmiHeader.biPlanes = 1;
    global_bitmap_info.bmiHeader.biBitCount = 32;
    global_bitmap_info.bmiHeader.biCompression = BI_RGB;
    
    // NOTE: Alloc memory for the backbuffer 
    size_t backbuffer_size = (WINDOW_WIDTH*WINDOW_HEIGHT*sizeof(u32));
    global_backbuffer_data = (u32 *)VirtualAlloc(0, backbuffer_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    global_bitmap = CreateDIBSection(device, &global_bitmap_info, DIB_RGB_COLORS, &global_backbuffer_data, 0, 0);
    
    // NOTE: Create a compatible device context to be able to blit it on window
    global_backbuffer_dc = CreateCompatibleDC(device);
    SelectObject(global_backbuffer_dc, global_bitmap);
}

static void win32_destroy_backbuffer(void)
{
    VirtualFree(global_backbuffer_data, 0, MEM_RELEASE);
}

static LRESULT win32_window_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
    LRESULT result = 0;
    switch(message)
    {
        case WM_CREATE:
        {
        }break;
        case WM_CLOSE:
        {
            global_running = false;
        }break;
        case WM_KEYDOWN:
        {
            TGuiEventKey key_event = {0};
            key_event.type = TGUI_EVENT_KEYDOWN;
            key_event.keycode = tgui_win32_translate_keycode((u32)w_param);
            tgui_push_event((TGuiEvent)key_event);
        }break;
        case WM_KEYUP:
        {
            TGuiEventKey key_event = {0};
            key_event.type = TGUI_EVENT_KEYUP;
            key_event.keycode = tgui_win32_translate_keycode((u32)w_param);
            tgui_push_event((TGuiEvent)key_event);
        }break;
        case WM_MOUSEMOVE:
        {
            TGuiEventMouseMove mouse_event = {0};
            mouse_event.type = TGUI_EVENT_MOUSEMOVE;
            mouse_event.pos_x = (i16)LOWORD(l_param);
            mouse_event.pos_y = (i16)HIWORD(l_param);
            tgui_push_event((TGuiEvent)mouse_event);
        }break;
        case WM_LBUTTONDOWN:
        {
            TGuiEvent mouse_event = {0};
            mouse_event.type = TGUI_EVENT_MOUSEDOWN;
            tgui_push_event(mouse_event);
        }break;
        case WM_LBUTTONUP:
        {
            TGuiEvent mouse_event = {0};
            mouse_event.type = TGUI_EVENT_MOUSEUP;
            tgui_push_event(mouse_event);
        }break;
        case WM_CHAR:
        {
            u8 character = (u8)w_param;
            TGuiEvent char_event = {0};
            char_event.type = TGUI_EVENT_CHAR;
            char_event.character.character = character;
            tgui_push_event(char_event);
        }
        default:
        {
            result = DefWindowProcA(window, message, w_param, l_param);
        }break;
    }
    return result;
}

int main(int argc, char** argv)
{
    UNUSED_VAR(argc);
    UNUSED_VAR(argv);
 
    WNDCLASSA window_class = {0};
    window_class.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    window_class.lpfnWndProc = win32_window_proc;
    window_class.hInstance = GetModuleHandle(0);
    window_class.lpszClassName = "tgui_window_class";

    if(!RegisterClassA(&window_class))
    {
        printf("[ERROR]: cannot register win32 class\n");
        return -1;
    }

    RECT window_rect = {0};
    window_rect.bottom = WINDOW_HEIGHT;
    window_rect.right = WINDOW_WIDTH;
    AdjustWindowRect(&window_rect, WS_VISIBLE|WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU, 0);

    u32 window_width = window_rect.right - window_rect.left;
    u32 window_height = window_rect.bottom - window_rect.top;
    HWND window = CreateWindowExA(0, window_class.lpszClassName, "tgui",
                                  WS_VISIBLE|WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,
                                  100, 100, window_width, window_height,
                                  0, 0, window_class.hInstance, 0);
    if(!window)
    {
        printf("[ERROR]: cannot create win32 window\n");
        return -1;
    }
    
    // TODO: Call this functions on WM_CREATE
    global_device_context = GetDC(window);
    global_running = true;
    win32_create_backbuffer(global_device_context);
   
    u32 frames_per_second = 30;
    u32 ms_per_frame = 1000 / frames_per_second;
    f32 delta_time = 1.0f / (f32)frames_per_second;
    LARGE_INTEGER large_frequency;
    LARGE_INTEGER large_last_time;
    QueryPerformanceFrequency(&large_frequency);
    QueryPerformanceCounter(&large_last_time);
    u64 last_time = large_last_time.QuadPart;
    
    UNUSED_VAR(delta_time);
    
    // NOTE: backbuffer for tgui to draw all the elements
    TGuiBitmap tgui_backbuffer = {0};
    tgui_backbuffer.width = WINDOW_WIDTH;
    tgui_backbuffer.height = WINDOW_HEIGHT;
    tgui_backbuffer.pixels = (u32 *)global_backbuffer_data;
    tgui_backbuffer.pitch = tgui_backbuffer.width * sizeof(u32);
    
    // NOTE: load bitmap for testing
    TGuiBitmap test_bitmap = tgui_debug_load_bmp("data/font.bmp");
    // NOTE: create a font for testing 
    TGuiFont test_font = tgui_create_font(&test_bitmap, 7, 9, 18, 6);
    
    // NOTE: init TGUI lib
    tgui_init(&tgui_backbuffer, &test_font);
    
    TGuiHandle frame1 = tgui_create_container(100, 100, 150, 200, TGUI_CONTAINER_DYNAMIC, TGUI_LAYOUT_VERTICAL, true, 5);
    TGuiHandle frame2 = tgui_create_container(450, 120, 100, 240, TGUI_CONTAINER_V_SCROLL|TGUI_CONTAINER_H_SCROLL, TGUI_LAYOUT_VERTICAL, true, 20);
    TGuiHandle frame3 = tgui_create_container(100, 100, 150, 200, TGUI_CONTAINER_DYNAMIC|TGUI_CONTAINER_DRAGGABLE, TGUI_LAYOUT_HORIZONTAL, true, 10);
    tgui_widget_to_root(frame3);
    tgui_container_add_widget(frame3, frame1);
    tgui_container_add_widget(frame3, frame2);

    TGuiHandle button_box = tgui_create_container(0, 0, 0, 0, TGUI_CONTAINER_DYNAMIC, TGUI_LAYOUT_HORIZONTAL, false, 15);
    tgui_container_add_widget(frame1, button_box);
    TGuiHandle slider_box = tgui_create_container(0, 0, 0, 0, TGUI_CONTAINER_DYNAMIC, TGUI_LAYOUT_VERTICAL, false, 10);
    tgui_container_add_widget(frame1, slider_box);
    TGuiHandle checkbox_box = tgui_create_container(0, 0, 0, 0, TGUI_CONTAINER_DYNAMIC, TGUI_LAYOUT_HORIZONTAL, false, 15);
    tgui_container_add_widget(frame1, checkbox_box);

    TGuiHandle button1 = tgui_create_button("button 1");
    TGuiHandle button2 = tgui_create_button("button 2");
    tgui_container_add_widget(button_box, button1);
    tgui_container_add_widget(button_box, button2);
    
    TGuiHandle slider1 = tgui_create_slider();
    TGuiHandle slider2 = tgui_create_slider();
    tgui_container_add_widget(slider_box, slider1);
    tgui_container_add_widget(slider_box, slider2);
    
    TGuiHandle checkbox1 = tgui_create_checkbox("box 1");
    TGuiHandle checkbox2 = tgui_create_checkbox("box 2");
    TGuiHandle checkbox3 = tgui_create_checkbox("box 3");
    tgui_container_add_widget(checkbox_box, checkbox1);
    tgui_container_add_widget(checkbox_box, checkbox2);
    tgui_container_add_widget(checkbox_box, checkbox3);
    
    #define BUTTONS_COUNT 8
    TGuiHandle buttons[BUTTONS_COUNT];
    for(i32 i = 0; i < BUTTONS_COUNT; ++i)
    {
        buttons[i] = tgui_create_button("button");
        tgui_container_add_widget(frame2, buttons[i]);
    }

    TGuiHandle frame4 = tgui_create_container(0, 0, 0, 0, TGUI_CONTAINER_DYNAMIC|TGUI_CONTAINER_DRAGGABLE, TGUI_LAYOUT_VERTICAL, true, 10);
    #define BUTTONS_COUNT2 8
    TGuiHandle buttons2[BUTTONS_COUNT2];
    for(i32 i = 0; i < BUTTONS_COUNT2; ++i)
    {
        buttons2[i] = tgui_create_button("button 2");
        tgui_container_add_widget(frame4, buttons2[i]);
    }
    
    TGuiHandle frame5 = tgui_create_container(400, 20, 0, 0, TGUI_CONTAINER_DYNAMIC|TGUI_CONTAINER_DRAGGABLE, TGUI_LAYOUT_HORIZONTAL, true, 10);
    tgui_widget_to_root(frame5);
    
    TGuiHandle textbox = tgui_create_textbox(200, 200);
    tgui_set_widget_position(textbox, 50, 50);
    
    tgui_container_add_widget(frame5, textbox);
    tgui_container_add_widget(frame5, frame4);

    printf("[INFO]: widget size %llu (bytes)\n", sizeof(TGuiWidget));
    printf("[INFO]: total allocated used %llu (bytes)\n", tgui_global_state.widget_allocator.count*sizeof(TGuiWidget));
    printf("[INFO]: total allocated size %llu (bytes)\n", tgui_global_state.widget_allocator.buffer_size*sizeof(TGuiWidget));

    while(global_running)
    {
        LARGE_INTEGER large_current_time;
        QueryPerformanceCounter(&large_current_time);
        u64 current_time = large_current_time.QuadPart;
        f32 curret_sec_per_frame = (f32)(current_time - last_time) / (f32)large_frequency.QuadPart;
        u64 current_ms_per_frame = (u64)(curret_sec_per_frame * 1000.0f); 
        if(current_ms_per_frame < ms_per_frame)
        {
            i32 ms_to_sleep = ms_per_frame - current_ms_per_frame;
            Sleep(ms_to_sleep);
        }
        QueryPerformanceCounter(&large_current_time);
        current_time = large_current_time.QuadPart;
        f32 debug_current_ms = (f32)(current_time - last_time) / (f32)large_frequency.QuadPart;
        last_time = current_time;

        MSG message;
        while(PeekMessageA(&message, window, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
        
        // NOTE: Update TGUI lib
        tgui_update();
        // NOTE: clear the screen
        tgui_clear_backbuffer(&tgui_backbuffer);
         
        char debug_str[256];
        u32 font_height = 9;
        sprintf(debug_str, "mouse pos (x:%d, y:%d)", tgui_global_state.mouse_x, tgui_global_state.mouse_y);
        tgui_draw_text(&tgui_backbuffer, &test_font, font_height, 0, tgui_backbuffer.height - font_height, debug_str, strlen(debug_str));
        
        sprintf(debug_str, "ms:%.3f", debug_current_ms);
        tgui_draw_text(&tgui_backbuffer, &test_font, font_height, 0, 0, debug_str, strlen(debug_str));
        sprintf(debug_str, "fps:%d", (u32)(1.0f/debug_current_ms+0.5f));
        tgui_draw_text(&tgui_backbuffer, &test_font, font_height, 0, font_height, debug_str, strlen(debug_str));

        tgui_draw_bitmap(&tgui_backbuffer, &test_bitmap, tgui_backbuffer.width - test_bitmap.width, 0, test_bitmap.width, test_bitmap.height);
        TGuiDrawCommand test_draw_bitmap_command; 
        test_draw_bitmap_command.type = TGUI_DRAWCMD_BITMAP;
        test_draw_bitmap_command.descriptor = tgui_rect_xywh(tgui_backbuffer.width - test_bitmap.width, 0, test_bitmap.width, test_bitmap.height);
        test_draw_bitmap_command.bitmap = &test_bitmap;
        tgui_push_draw_command(test_draw_bitmap_command);
        
        tgui_draw_command_buffer();
        
        // NOTE: Blt the backbuffer on to the destination window
        BitBlt(global_device_context, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, global_backbuffer_dc, 0, 0, SRCCOPY);
    }
    
    tgui_terminate();
    tgui_debug_free_bmp(&test_bitmap);
    win32_destroy_backbuffer();

    return 0;
}
