#include "tgui.h"

// TODO: stop using std lib to load files in the future
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// TODO: Maybe the state should be provided by the application?
TGuiState tgui_global_state;

// TODO: maybe create a renderer struct to save info like this
TGuiClippingStack global_clipping_stack;

//-----------------------------------------------------
//  NOTE: inline math functions
//-----------------------------------------------------
static inline TGuiV2 tgui_v2(f32 x, f32 y)
{
    TGuiV2 result;
    result.x = x;
    result.y = y;
    return result;
}

static inline TGuiV2 tgui_v2_add(TGuiV2 a, TGuiV2 b)
{
    TGuiV2 result = tgui_v2(a.x + b.x, a.y + b.y);
    return result;
}

static inline TGuiV2 tgui_v2_sub(TGuiV2 a, TGuiV2 b)
{
    TGuiV2 result = tgui_v2(a.x - b.x, a.y - b.y);
    return result;
}

static inline TGuiV2 tgui_v2_scale(TGuiV2 a, f32 b)
{
    TGuiV2 result = tgui_v2(a.x * b, a.y * b);
    return result;
}

static inline f32 tgui_v2_dot(TGuiV2 a, TGuiV2 b)
{
    f32 result = a.x * b.x + a.y * b.y;
    return result;
}

static inline f32 tgui_v2_lenght_sqrt(TGuiV2 v)
{
    f32 result = tgui_v2_dot(v, v);
    return result;
}

static inline TGuiRect tgui_rect_xywh(f32 x, f32 y, f32 width, f32 height)
{
    TGuiRect result;
    result.x = x;
    result.y = y;
    result.width = width;
    result.height = height;
    return result;
}

static inline f32 tgui_v2_length(TGuiV2 v)
{
    f32 result = sqrtf(tgui_v2_lenght_sqrt(v));
    return result;
}

static inline TGuiV2 tgui_v2_normalize(TGuiV2 v)
{
    TGuiV2 result = {0};
    result.x = v.x / tgui_v2_length(v);
    result.y = v.y / tgui_v2_length(v);
    return result;
}

static b32 tgui_point_inside_rect(TGuiV2 point, TGuiRect rect)
{
    b32 result = point.x >= rect.x && point.x < (rect.x + rect.width) &&
                 point.y >= rect.y && point.y < (rect.y + rect.height);
    return result;
}

//-----------------------------------------------------
// NOTE: GUI lib functions
//-----------------------------------------------------

static inline TGuiWidget *tgui_create_widget(TGuiHandle *handle)
{
    TGuiState *state = &tgui_global_state;
    *handle = tgui_widget_allocator_pool(&state->widget_allocator);
    TGuiWidget *widget = tgui_widget_get(*handle);
    memset(widget, 0, sizeof(TGuiWidget));
    widget->header.handle = *handle;
    
    return widget;
}

inline static TGuiHandle tgui_create_end_container(void)
{
    TGuiHandle handle = TGUI_INVALID_HANDLE;
    TGuiWidget *widget = tgui_create_widget(&handle); 
    widget->header.type = TGUI_END_CONTAINER;
    return handle;
}

TGuiHandle tgui_create_container(i32 x, i32 y, i32 width, i32 height, TGuiContanerFlags flags, TGuiLayoutType layout, b32 visible, u32 padding)
{
    TGuiHandle handle = TGUI_INVALID_HANDLE;
    // NOTE: allocate end container at first so it not change the widget ptr we its allocated
    TGuiWidget *widget = tgui_create_widget(&handle); 
    
    // NOTE: add end container widget to the last child
    TGuiHandle end_container_handle = tgui_create_end_container();
    // NOTE: update the widget pointer (can change with the new allocation)
    widget = tgui_widget_get(handle);
    widget->header.child_first = end_container_handle;
    widget->header.child_last = end_container_handle;
    TGuiWidget *end_container = tgui_widget_get(end_container_handle);
    end_container->header.parent = handle;
    
    widget->header.type = TGUI_CONTAINER;
    widget->header.position = tgui_v2(x, y);
    widget->header.size = tgui_v2(width, height);
    
    widget->container.layout.type = layout;
    widget->container.layout.padding = padding;
    widget->container.flags = flags;
    widget->container.visible = visible;
    
    widget->container.dimension = widget->header.size;
    f32 grip_size = 20.0f;
    if(widget->container.flags & TGUI_CONTAINER_V_SCROLL)
    {
        widget->container.vertical_grip = tgui_rect_xywh(widget->container.dimension.x, 0, grip_size, widget->container.dimension.y);
        widget->header.size.x += grip_size;
    }
    if(widget->container.flags & TGUI_CONTAINER_H_SCROLL)
    {
        widget->container.horizontal_grip = tgui_rect_xywh(0, widget->container.dimension.y, widget->container.dimension.x, grip_size);
        widget->header.size.y += grip_size;
    }

    return handle;
}

static inline void tgui_widget_set_text(TGuiText *text, char *label)
{
    TGuiState *state = &tgui_global_state;
    text->text = label;
    text->size.y = state->font_height;
    text->size.x = tgui_text_get_width(state->font, label, state->font_height);
}

TGuiHandle tgui_create_button(char *label)
{
    TGuiHandle handle = TGUI_INVALID_HANDLE;
    TGuiWidget *widget = tgui_create_widget(&handle); 
    widget->header.type = TGUI_BUTTON;
    widget->header.size = tgui_v2(100, 30);
    widget->button.pressed = false;
    tgui_widget_set_text(&widget->button.text, label);
    return handle;
}

TGuiHandle tgui_create_checkbox(char *label)
{
    TGuiHandle handle = TGUI_INVALID_HANDLE;
    TGuiWidget *widget = tgui_create_widget(&handle); 
    widget->header.type = TGUI_CHECKBOX;
    widget->checkbox.box_dimension = tgui_v2(20, 20);
    tgui_widget_set_text(&widget->checkbox.text, label);
    widget->header.size = widget->checkbox.box_dimension;
    widget->header.size.x += widget->checkbox.text.size.x+5;
    return handle;
}

TGuiHandle tgui_create_slider(void)
{
    TGuiHandle handle = TGUI_INVALID_HANDLE;
    TGuiWidget *widget = tgui_create_widget(&handle); 
    widget->header.type = TGUI_SLIDER;
    widget->slider.ratio = 0.5f;
    widget->slider.value = 0.5f;
    widget->slider.grip_dimension = tgui_v2(20, 20);
    widget->header.size = tgui_v2(200, widget->slider.grip_dimension.y);
    return handle;
}

void tgui_widget_to_root(TGuiHandle widget_handle)
{
    TGuiState *state = &tgui_global_state;
    TGuiWidget *widget = tgui_widget_get(widget_handle);
    if(!state->last_root)
    {
        state->last_root = widget_handle;
        state->first_root = widget_handle;
    }
    else
    {
        TGuiWidget *root = tgui_widget_get(state->first_root);
        widget->header.sibling_next = root->header.handle;
        root->header.sibling_prev = widget_handle;
        state->first_root = widget_handle;
    }
}

void tgui_set_widget_position(TGuiHandle widget_handle, f32 x, f32 y)
{
    TGuiWidget *widget = tgui_widget_get(widget_handle);
    widget->header.position = tgui_v2(x, y);
}

TGuiV2 tgui_widget_abs_pos(TGuiHandle handle)
{
    TGuiWidget *widget = tgui_widget_get(handle);
    TGuiV2 result = widget->header.position;
    TGuiV2 base_pos = {0};
    while(widget->header.parent)
    {
        TGuiWidget *parent = tgui_widget_get(widget->header.parent);
        base_pos = tgui_v2_add(base_pos, parent->header.position);
        widget = parent;
    }
    result = tgui_v2_add(result, base_pos);
    return result; 
}

static void tgui_container_set_container_total_size(TGuiWidgetContainer *container, TGuiWidget *widget)
{
    // NOTE: resize the container
    TGuiV2 total_container_size = tgui_v2(0, 0);
    u32 num_child = 0;
    while(widget && (widget->header.type != TGUI_END_CONTAINER))
    {
        switch(container->layout.type)
        {
            case TGUI_LAYOUT_VERTICAL:
            {
                if(widget->header.size.x > total_container_size.x)
                {
                    total_container_size.x = widget->header.size.x;
                }
                total_container_size.y += widget->header.size.y;
            } break;
            case TGUI_LAYOUT_HORIZONTAL:
            {
                if(widget->header.size.y > total_container_size.y)
                {
                    total_container_size.y = widget->header.size.y;
                }
                total_container_size.x += widget->header.size.x;
            } break;
            case TGUI_LAYOUT_NONE:
            {
            } break;
            case TGUI_LAYOUT_COUNT:
            {
                ASSERT(!"invalid code path");
            } break;
        }
        ++num_child;
        widget = tgui_widget_get(widget->header.sibling_next);
    }
    // NOTE: add the container last padding
    switch(container->layout.type)
    {
        case TGUI_LAYOUT_VERTICAL:
        {
            total_container_size.x += container->layout.padding*2;
            total_container_size.y += container->layout.padding*(num_child+1);
        } break;
        case TGUI_LAYOUT_HORIZONTAL:
        {
            total_container_size.x += container->layout.padding*(num_child+1);
            total_container_size.y += container->layout.padding*2;
        } break;
        case TGUI_LAYOUT_NONE:
        {
        } break;
        case TGUI_LAYOUT_COUNT:
        {
            ASSERT(!"invalid code path");
        } break;
    }
    
    if(container->flags & TGUI_CONTAINER_DYNAMIC)
    {
        container->header.size = total_container_size;
        container->dimension = container->header.size;
    }
    else
    {
        container->total_dimension = total_container_size;
    }
}

static void tgui_container_recalculate_dimension(TGuiWidgetContainer *container)
{
    while(container)
    {
        TGuiWidget *first_child = tgui_widget_get(container->header.child_first);
        if(first_child)
        {
            tgui_container_set_container_total_size(container, first_child);
        }
        TGuiWidget *parent = tgui_widget_get(container->header.parent);
        container = &parent->container;
    }
}

static void tgui_container_set_childs_position(TGuiWidgetContainer *container, TGuiWidget *widget)
{
    TGuiHandle container_last_child = widget->header.sibling_prev;
    widget = tgui_widget_get(container_last_child);
    while(widget)
    {
        TGuiWidget *widget_prev = tgui_widget_get(widget->header.sibling_prev);
        TGuiWidget *widget_next = tgui_widget_get(widget->header.sibling_next);
        TGuiV2 view_port_dimension = tgui_v2_sub(container->total_dimension, container->dimension);
        if(container->layout.type == TGUI_LAYOUT_VERTICAL)
        {
            if(widget->header.handle != container_last_child) 
            {
                widget->header.position.y = widget_next->header.position.y + widget_next->header.size.y + container->layout.padding;
                widget->header.position.x = widget_next->header.position.x;
            }
            else
            {
                widget->header.position.y = container->layout.padding;
                widget->header.position.y -= container->vertical_value * view_port_dimension.y;
                widget->header.position.x = container->layout.padding;
                widget->header.position.x -= container->horizontal_value * view_port_dimension.x;
            }
        }
        if(container->layout.type == TGUI_LAYOUT_HORIZONTAL)
        {
            if(widget->header.handle != container_last_child) 
            {
                widget->header.position.x = widget_next->header.position.x + widget_next->header.size.x + container->layout.padding;
                widget->header.position.y = widget_next->header.position.y;
            }
            else
            {
                widget->header.position.x = container->layout.padding;
                widget->header.position.x -= container->horizontal_value * view_port_dimension.x;
                widget->header.position.y = container->layout.padding;
                widget->header.position.y -= container->vertical_value * view_port_dimension.y;
            }
        }
        widget = widget_prev;
    }
}

static void tgui_container_recalculate_widget_position(TGuiWidgetContainer *container)
{
    while(container)
    {
        TGuiWidget *last_child = tgui_widget_get(container->header.child_last);
        if(last_child)
        {
            tgui_container_set_childs_position(container, last_child);
        }
        TGuiWidget *parent = tgui_widget_get(container->header.parent);
        container = (TGuiWidgetContainer *)parent;
    }
}

void tgui_container_add_widget(TGuiHandle container_handle, TGuiHandle widget_handle)
{
    TGuiWidget *container_widget = tgui_widget_get(container_handle);
    ASSERT(container_widget->header.type == TGUI_CONTAINER);

    TGuiWidgetContainer *container = (TGuiWidgetContainer *)container_widget;
    TGuiWidget *widget = tgui_widget_get(widget_handle);
    
    if(!container->header.child_last)
    {
        container->header.child_last = widget_handle;
    }
    widget->header.parent = container_handle; 
    if(container->header.child_first)
    {
        TGuiWidget *old_first_child = tgui_widget_get(container->header.child_first);
        old_first_child->header.sibling_prev = widget_handle; 
    }
    widget->header.sibling_next = container->header.child_first; 
    container->header.child_first = widget_handle;
    
    // NOTE: recalculate the dimensions
    tgui_container_recalculate_dimension(container);
    // NOTE: set the widget position inside container
    tgui_container_recalculate_widget_position(container);
}

void tgui_widget_update(TGuiHandle handle)
{
    TGuiState *state = &tgui_global_state;
    TGuiV2 mouse = tgui_v2(state->mouse_x, state->mouse_y); 

    
    TGuiWidget *widget = tgui_widget_get(handle);
    // NOTE: if the mouse is not in the parent containar, dont event update the widget
    if(widget->header.parent)
    {
        TGuiWidget *parent = tgui_widget_get(widget->header.parent);
        ASSERT(parent->header.type == TGUI_CONTAINER);
        TGuiV2 widget_abs_pos = tgui_widget_abs_pos(widget->header.parent);
        TGuiRect container_box = tgui_rect_xywh(widget_abs_pos.x, widget_abs_pos.y, parent->container.dimension.x, parent->container.dimension.y);
        if(!tgui_point_inside_rect(mouse, container_box)) return;
    }

    TGuiV2 widget_abs_pos = tgui_widget_abs_pos(handle);
    TGuiRect widget_rect;
    widget_rect.pos = widget_abs_pos;
    // TODO: IMPORTANT!!!!
    // TODO: create a bouding box in header
    if(widget->header.type == TGUI_CHECKBOX)
    {
        widget_rect.dim = widget->checkbox.box_dimension;
    }
    else if(widget->header.type == TGUI_SLIDER)
    {
        widget_rect.x += (widget->slider.value * widget->header.size.x) - (widget->slider.grip_dimension.x*0.5f); 
        widget_rect.dim = widget->slider.grip_dimension;
    }
    else
    {
        widget_rect.dim = widget->header.size;
    }
    if(tgui_point_inside_rect(mouse, widget_rect))
    {
        state->focus = handle;
    }
    
    // TODO: maybe a better way to handle on top containers is walk the tree from last_child to first_child
    // and return on the first event
    b32 is_active = state->active == widget->header.handle;
    b32 is_focus = state->focus == widget->header.handle;
    b32 was_focus = state->last_focus == widget->header.handle;
    switch(widget->header.type)
    {
        case TGUI_CONTAINER:
        {
            if(widget->container.flags & TGUI_CONTAINER_V_SCROLL)
            {
                TGuiRect backgrip;
                backgrip.pos = tgui_v2_add(widget_abs_pos, widget->container.vertical_grip.pos);
                backgrip.dim = widget->container.vertical_grip.dim;
                
                TGuiRect grip;
                TGuiV2 grip_pos = widget->container.vertical_grip.pos;
                grip.dim = widget->container.vertical_grip.dim;
                
                if(widget->container.total_dimension.y)
                {
                    grip.dim.y = (widget->container.dimension.y / widget->container.total_dimension.y) * widget->container.vertical_grip.height;
                    grip_pos.y = widget->container.vertical_value * (widget->container.dimension.y - grip.dim.y);
                    grip.pos = tgui_v2_add(widget_abs_pos, grip_pos);
                    
                    if(tgui_point_inside_rect(mouse, grip) && state->mouse_down)
                    {
                        widget->container.grabbing_y = true; 
                    }
                    if(state->mouse_up)
                    {
                        widget->container.grabbing_y = false; 
                    }
                    if(widget->container.grabbing_y)
                    {
                        f32 mouse_y_rel = (state->mouse_y - backgrip.y) / (widget->container.dimension.y - grip.dim.y);
                        f32 last_mouse_y_rel = (state->last_mouse_y - backgrip.y) / (widget->container.dimension.y - grip.dim.y);
                        f32 y_offset = mouse_y_rel - last_mouse_y_rel; 
                        widget->container.vertical_value += y_offset;
                        if(widget->container.vertical_value < 0) widget->container.vertical_value = 0;
                        if(widget->container.vertical_value > 1) widget->container.vertical_value = 1;
                    }
                }
            }

            if(widget->container.flags & TGUI_CONTAINER_H_SCROLL)
            {
                TGuiRect backgrip;
                backgrip.pos = tgui_v2_add(widget_abs_pos, widget->container.horizontal_grip.pos);
                backgrip.dim = widget->container.horizontal_grip.dim;
                
                TGuiRect grip;
                TGuiV2 grip_pos = widget->container.horizontal_grip.pos;
                grip_pos.x = widget->container.horizontal_value * ( widget->container.dimension.x - widget->container.horizontal_grip.width*0.5f);
                grip.pos = tgui_v2_add(widget_abs_pos, grip_pos);
                grip.dim = widget->container.horizontal_grip.dim;
                if(widget->container.total_dimension.x)
                {
                    grip.dim.x = (widget->container.dimension.x / widget->container.total_dimension.x) * widget->container.horizontal_grip.width;
                    if(tgui_point_inside_rect(mouse, grip) && state->mouse_down)
                    {
                        widget->container.grabbing_x = true; 
                    }
                    if(state->mouse_up)
                    {
                        widget->container.grabbing_x = false; 
                    }
                    if(widget->container.grabbing_x)
                    {
                        f32 mouse_x_rel = (state->mouse_x - backgrip.x) / (widget->container.dimension.x - grip.dim.x);
                        f32 last_mouse_x_rel = (state->last_mouse_x - backgrip.x) / (widget->container.dimension.x - grip.dim.x);
                        f32 x_offset = mouse_x_rel - last_mouse_x_rel; 
                        widget->container.horizontal_value += x_offset;
                        if(widget->container.horizontal_value < 0) widget->container.horizontal_value = 0;
                        if(widget->container.horizontal_value > 1) widget->container.horizontal_value = 1;
                    }
                }
            }
            tgui_container_recalculate_widget_position(&widget->container);
        }break;
        case TGUI_END_CONTAINER:
        {}break;
        case TGUI_BUTTON:
        {
            widget->button.pressed = false;
            if(was_focus && state->mouse_down)
            {
                state->active = widget->header.handle;
            }
            if(is_active && was_focus && state->mouse_up)
            {
                widget->button.pressed = true;
                
            }
            if(is_active && !is_focus)
            {
                state->active = 0;
            }
        }break;
        case TGUI_CHECKBOX:
        {
            if(was_focus && state->mouse_down)
            {
                state->active = widget->header.handle;
            }
            if(is_active && was_focus && state->mouse_up)
            {
                widget->checkbox.active = !widget->checkbox.active;
                
            }
            if(is_active && !is_focus)
            {
                state->active = 0;
            }
        }break;
        case TGUI_SLIDER:
        {
            f32 mouse_x_rel = (mouse.x - widget_abs_pos.x) / widget->header.size.x;
            if(is_focus && state->mouse_is_down)
            {
                state->active = widget->header.handle;
            }
            if(is_active)
            {
                if(mouse_x_rel < 0.0f)
                {
                    mouse_x_rel = 0;
                }
                if(mouse_x_rel > 1.0f)
                {
                    mouse_x_rel = 1.0f;
                }
                widget->slider.value = mouse_x_rel; 
            }
            if(is_active && state->mouse_up)
            {
                state->active = 0;
            }
        }break;
        case TGUI_COUNT:
        {
            ASSERT(!"invalid code path");
        }break;
    }
}

void tgui_widget_render(TGuiHandle handle)
{
    TGuiWidget *widget = tgui_widget_get(handle);
    TGuiV2 widget_abs_pos = tgui_widget_abs_pos(handle);
    TGuiState *state = &tgui_global_state;
    
    switch(widget->header.type)
    {
        case TGUI_CONTAINER:
        {
            TGuiDrawCommand draw_cmd = {0};
            draw_cmd.type = TGUI_DRAWCMD_RECT;
            if(widget->container.flags & TGUI_CONTAINER_DYNAMIC) 
            {
                draw_cmd.type = TGUI_DRAWCMD_ROUNDED_RECT;
                draw_cmd.ratio = 16;
            }
            draw_cmd.descriptor.pos = widget_abs_pos;
            // TODO: maybe create a tgui_get_container_dimension function
            draw_cmd.descriptor.dim = widget->container.dimension;
            draw_cmd.color = TGUI_BLACK;
            tgui_push_draw_command(draw_cmd);
            
            if(widget->container.flags & TGUI_CONTAINER_V_SCROLL)
            {
                TGuiDrawCommand back_grip_cmd = {0};
                back_grip_cmd.type = TGUI_DRAWCMD_RECT;
                back_grip_cmd.descriptor.pos = tgui_v2_add(widget_abs_pos, widget->container.vertical_grip.pos);
                back_grip_cmd.descriptor.dim = widget->container.vertical_grip.dim;
                back_grip_cmd.color = TGUI_ORANGE;
                tgui_push_draw_command(back_grip_cmd);

                TGuiDrawCommand grip_cmd = {0};
                grip_cmd.type = TGUI_DRAWCMD_ROUNDED_RECT;
                grip_cmd.ratio = 4;

                TGuiV2 grip_pos = widget->container.vertical_grip.pos;
                grip_cmd.descriptor.dim = widget->container.vertical_grip.dim;
                if(widget->container.total_dimension.y)
                {
                    grip_cmd.descriptor.dim.y = (widget->container.dimension.y / widget->container.total_dimension.y) * widget->container.vertical_grip.height;
                }
                grip_pos.y = widget->container.vertical_value * (widget->container.dimension.y - grip_cmd.descriptor.dim.y);
                grip_cmd.descriptor.pos = tgui_v2_add(widget_abs_pos, grip_pos);
                grip_cmd.color = TGUI_GREEN;
                tgui_push_draw_command(grip_cmd);
            }
            if(widget->container.flags & TGUI_CONTAINER_H_SCROLL)
            {
                TGuiDrawCommand back_grip_cmd = {0};
                back_grip_cmd.type = TGUI_DRAWCMD_RECT;
                back_grip_cmd.descriptor.pos = tgui_v2_add(widget_abs_pos, widget->container.horizontal_grip.pos);
                back_grip_cmd.descriptor.dim = widget->container.horizontal_grip.dim;
                back_grip_cmd.color = TGUI_ORANGE;
                tgui_push_draw_command(back_grip_cmd);

                TGuiDrawCommand grip_cmd = {0};
                grip_cmd.type = TGUI_DRAWCMD_ROUNDED_RECT;
                grip_cmd.ratio = 4;

                TGuiV2 grip_pos = widget->container.horizontal_grip.pos;
                grip_cmd.descriptor.dim = widget->container.horizontal_grip.dim;
                if(widget->container.total_dimension.x)
                {
                    grip_cmd.descriptor.dim.x = (widget->container.dimension.x / widget->container.total_dimension.x) * widget->container.horizontal_grip.width;
                }
                grip_pos.x = widget->container.horizontal_value * (widget->container.dimension.x - grip_cmd.descriptor.dim.x);
                grip_cmd.descriptor.pos = tgui_v2_add(widget_abs_pos, grip_pos);
                grip_cmd.color = TGUI_GREEN;
                tgui_push_draw_command(grip_cmd);
            }

            if(widget->container.flags & (TGUI_CONTAINER_V_SCROLL | TGUI_CONTAINER_H_SCROLL))
            {
                TGuiDrawCommand start_clip_cmd = {0};
                start_clip_cmd.type = TGUI_DRAWCMD_START_CLIPPING;
                start_clip_cmd.descriptor.pos = widget_abs_pos;
                start_clip_cmd.descriptor.dim = widget->container.dimension;
                tgui_push_draw_command(start_clip_cmd);
            }
        } break;
        case TGUI_END_CONTAINER:
        {
            TGuiWidget *parent = tgui_widget_get(widget->header.parent);
            if(parent->container.flags & (TGUI_CONTAINER_V_SCROLL | TGUI_CONTAINER_H_SCROLL))
            {
                TGuiDrawCommand end_clip_cmd = {0};
                end_clip_cmd.type = TGUI_DRAWCMD_END_CLIPPING;
                tgui_push_draw_command(end_clip_cmd);
            }
        } break;
        case TGUI_BUTTON:
        {
            TGuiDrawCommand draw_cmd = {0};
            draw_cmd.type = TGUI_DRAWCMD_RECT;
            draw_cmd.descriptor.pos = widget_abs_pos;
            draw_cmd.descriptor.dim = widget->header.size;
            
            TGuiWidgetButton *button_data = &widget->button;
            u32 color = TGUI_GREY; 
            if(state->focus == widget->header.handle)
            {
                color = TGUI_GREEN;
            }
            if(state->active == widget->header.handle)
            {
                color = TGUI_ORANGE;
            }
            if(widget->button.pressed)
            {
                color = TGUI_RED;
            }
            draw_cmd.color = color;
            tgui_push_draw_command(draw_cmd);
                
            TGuiRect text_rect;
            text_rect.dim = button_data->text.size;
            text_rect.pos = tgui_v2_sub(tgui_v2_add(widget_abs_pos, tgui_v2_scale(button_data->header.size, 0.5f)), tgui_v2_scale(button_data->text.size, 0.5f));

            TGuiDrawCommand text_cmd = {0};
            text_cmd.type = TGUI_DRAWCMD_TEXT;
            text_cmd.descriptor = text_rect;
            text_cmd.text = button_data->text.text;
            tgui_push_draw_command(text_cmd);
        
        } break;
        case TGUI_CHECKBOX:
        {
            TGuiDrawCommand draw_cmd = {0};
            draw_cmd.type = TGUI_DRAWCMD_ROUNDED_RECT;
            
            TGuiWidgetCheckBox *checkbox_data = &widget->checkbox;
            draw_cmd.descriptor.pos = widget_abs_pos;
            draw_cmd.descriptor.dim = checkbox_data->box_dimension;
            draw_cmd.ratio = 4;

            u32 color = TGUI_RED; 
            if(checkbox_data->active)
            {
                color = TGUI_GREEN;
            }
            draw_cmd.color = color;
            tgui_push_draw_command(draw_cmd);
                
            TGuiRect text_rect;
            text_rect.dim = checkbox_data->text.size;
            text_rect.pos = widget_abs_pos;
            text_rect.pos.y += checkbox_data->box_dimension.y*0.5f - checkbox_data->text.size.y*0.5f;
            text_rect.pos.x += checkbox_data->box_dimension.x + 5;
            TGuiDrawCommand text_cmd = {0};
            text_cmd.type = TGUI_DRAWCMD_TEXT;
            text_cmd.descriptor = text_rect;
            text_cmd.text = checkbox_data->text.text;
            tgui_push_draw_command(text_cmd);
        } break;
        case TGUI_SLIDER:
        {
            TGuiDrawCommand line_cmd = {0};
            line_cmd.type = TGUI_DRAWCMD_RECT;
            line_cmd.descriptor.pos = widget_abs_pos;
            line_cmd.descriptor.pos.y += (widget->header.size.y * (0.5f*widget->slider.ratio));
            line_cmd.descriptor.dim = widget->header.size;
            line_cmd.descriptor.dim.y *= widget->slider.ratio;
            line_cmd.color = TGUI_ORANGE;
            tgui_push_draw_command(line_cmd);

            TGuiWidgetSlider *slider_data = &widget->slider;
            TGuiDrawCommand draw_cmd = {0};
            draw_cmd.type = TGUI_DRAWCMD_ROUNDED_RECT;
            draw_cmd.descriptor.pos = widget_abs_pos;
            draw_cmd.descriptor.dim = slider_data->grip_dimension;
            draw_cmd.descriptor.x += (slider_data->value * widget->header.size.x) - (0.5f*widget->slider.grip_dimension.x);
            draw_cmd.ratio = 4;
            draw_cmd.color = TGUI_GREY;
            tgui_push_draw_command(draw_cmd);
        } break;
        case TGUI_COUNT:
        {
            ASSERT(!"invalid code path");
        } break;
    }
}

void tgui_widget_recursive_descent_first_to_last(TGuiHandle handle, TGuiWidgetFP function)
{
    TGuiWidget *widget = tgui_widget_get(handle);
    while(widget)
    {
        function(widget->header.handle);
        if(widget->header.child_first)
        {
            tgui_widget_recursive_descent_first_to_last(widget->header.child_first, function);
        }
        widget = tgui_widget_get(widget->header.sibling_next);
    }
}

void tgui_widget_recursive_descent_last_to_first(TGuiHandle handle, TGuiWidgetFP function)
{
    TGuiWidget *widget = tgui_widget_get(handle);
    while(widget)
    {
        function(widget->header.handle);
        if(widget->header.child_last)
        {
            tgui_widget_recursive_descent_last_to_first(widget->header.child_last, function);
        }
        widget = tgui_widget_get(widget->header.sibling_prev);
    }
}

//-----------------------------------------------------
//  NOTE: memory management functions
//-----------------------------------------------------

// TODO: move clipping stack to software rendering code
void tgui_clipping_stack_create(TGuiClippingStack *stack)
{
    stack->buffer_size = TGUI_DEFAULT_CLIPPING_STACK_SIZE;
    stack->buffer = (TGuiRect *)malloc(stack->buffer_size*sizeof(TGuiRect));
    stack->top = 0;
}

void tgui_clipping_stack_destoy(TGuiClippingStack *stack)
{
    free(stack->buffer);
    stack->buffer = 0;
    stack->buffer_size = 0;
    stack->top = 0;
}

void tgui_clipping_stack_push(TGuiClippingStack *stack, TGuiRect clipping)
{
    // TODO: remove this ASSERT();
    ASSERT(stack->buffer_size < 16);

    if(stack->top == stack->buffer_size)
    {
        u32 new_buffer_size = stack->buffer_size * 2;
        TGuiRect *new_buffer = (TGuiRect *)malloc(new_buffer_size*sizeof(TGuiRect));
        memcpy(new_buffer, stack->buffer, stack->buffer_size*sizeof(TGuiRect));
        free(stack->buffer);
        stack->buffer = new_buffer;
        stack->buffer_size = new_buffer_size;
    }

    stack->buffer[stack->top++] = clipping;
}

TGuiRect tgui_clipping_stack_pop(TGuiClippingStack *stack)
{
    TGuiRect result = (TGuiRect){0};
    if(stack->top > 0)
    {
        result = stack->buffer[--stack->top];
    }
    return result;
}

TGuiRect tgui_clipping_stack_top(TGuiClippingStack *stack)
{
    TGuiRect result = (TGuiRect){0};
    if(stack->top > 0)
    {
        result = stack->buffer[stack->top - 1];
    }
    return result;
}

void tgui_widget_poll_allocator_create(TGuiWidgetPoolAllocator *allocator)
{
    allocator->buffer_size = TGUI_DEFAULT_POOL_SIZE;
    allocator->buffer = (TGuiWidget *)malloc(allocator->buffer_size*sizeof(TGuiWidget));
    // NOTE: because 0 is a INVALID HANDLE the first element in the pool is reserved
    allocator->count = 1;
    allocator->free_list = 0;
}

void tgui_widget_allocator_destroy(TGuiWidgetPoolAllocator *allocator)
{
    free(allocator->buffer);
    allocator->buffer_size = 0;
    allocator->count = 0;
    allocator->free_list = 0;
}

TGuiHandle tgui_widget_allocator_pool(TGuiWidgetPoolAllocator *allocator)
{
    TGuiHandle handle = TGUI_INVALID_HANDLE;
    if(allocator->free_list)
    {
        handle = allocator->free_list->handle;
        allocator->free_list = allocator->free_list->next;
        return handle;
    }
    
    handle = allocator->count++;
    if(handle >= allocator->buffer_size)
    {
        // NOTE: this is a dynamic pool allocator need to be reallocated here!
        u32 new_buffer_size = allocator->buffer_size * 2;
        TGuiWidget *new_buffer = (TGuiWidget *)malloc(new_buffer_size*sizeof(TGuiWidget));
        memcpy(new_buffer, allocator->buffer, allocator->buffer_size*sizeof(TGuiWidget));
        free(allocator->buffer);
        allocator->buffer = new_buffer;
        allocator->buffer_size = new_buffer_size;
    }
    return handle;
}

void tgui_widget_allocator_free(TGuiWidgetPoolAllocator *allocator, TGuiHandle *handle)
{
    ASSERT(*handle != TGUI_INVALID_HANDLE);
    TGuiWidgetFree *free_widget = (TGuiWidgetFree *)(allocator->buffer + *handle);
    free_widget->handle = *handle;
    free_widget->next = allocator->free_list;
    allocator->free_list = free_widget;
    *handle = TGUI_INVALID_HANDLE;
}

void tgui_widget_set(TGuiHandle handle, TGuiWidget widget)
{
    ASSERT(handle != TGUI_INVALID_HANDLE);
    TGuiState *state = &tgui_global_state;
    state->widget_allocator.buffer[handle] = widget;
}

TGuiWidget *tgui_widget_get(TGuiHandle handle)
{
    TGuiWidget *result = 0;
    if(handle != TGUI_INVALID_HANDLE)
    {
        TGuiState *state = &tgui_global_state;
        result = state->widget_allocator.buffer + handle;
    }
    return result;
}

//-----------------------------------------------------
//  NOTE: core library functions
//-----------------------------------------------------

void tgui_push_event(TGuiEvent event)
{
    TGuiState *state = &tgui_global_state;
    if(state->event_queue.count < TGUI_EVENT_QUEUE_MAX)
    {
        state->event_queue.queue[state->event_queue.count++] = event;
    }
}

void tgui_push_draw_command(TGuiDrawCommand draw_cmd)
{
    TGuiState *state = &tgui_global_state;
    if(state->draw_command_buffer.count< TGUI_DRAW_COMMANDS_MAX)
    {
        state->draw_command_buffer.buffer[state->draw_command_buffer.count++] = draw_cmd;
    }
}

b32 tgui_pull_draw_command(TGuiDrawCommand *draw_cmd)
{
    TGuiState *state = &tgui_global_state;
    if(state->draw_command_buffer.head >= state->draw_command_buffer.count)
    {
        state->draw_command_buffer.head = 0;
        state->draw_command_buffer.count = 0;
        return false;
    }
    *draw_cmd = state->draw_command_buffer.buffer[state->draw_command_buffer.head++];
    return true;
}

// NOTE: core lib functions
void tgui_init(TGuiBitmap *backbuffer, TGuiFont *font)
{
    TGuiState *state = &tgui_global_state;
    
    memset(state, 0, sizeof(TGuiState));
    state->backbuffer = backbuffer;
    state->font = font;
    state->font_height = 9;
    
    tgui_widget_poll_allocator_create(&state->widget_allocator);
    
    tgui_clipping_stack_create(&global_clipping_stack);
    tgui_clipping_stack_push(&global_clipping_stack, tgui_rect_xywh(0, 0, backbuffer->width, backbuffer->height));
}

void tgui_terminate(void)
{
    TGuiState *state = &tgui_global_state;
    tgui_clipping_stack_destoy(&global_clipping_stack);
    tgui_widget_allocator_destroy(&state->widget_allocator);
}

void tgui_update(void)
{
    TGuiState *state = &tgui_global_state;
    // NOTE: clear old state that are not needed any more
    state->mouse_up = false;
    state->mouse_down = false;
    state->last_mouse_x = state->mouse_x;
    state->last_mouse_y = state->mouse_y;
    // NOTE: pull tgui events from the queue
    for(u32 event_index = 0; event_index < state->event_queue.count; ++event_index)
    {
        TGuiEvent *event = state->event_queue.queue + event_index;
        switch(event->type)
        {
            case TGUI_EVENT_KEYDOWN:
            {
                printf("key down event!\n");  
            } break;
            case TGUI_EVENT_KEYUP:
            {
                printf("key up event!\n");  
            } break;
            case TGUI_EVENT_MOUSEMOVE:
            {
                TGuiEventMouseMove *mouse = (TGuiEventMouseMove *)event;
                state->mouse_x = mouse->pos_x;
                state->mouse_y = mouse->pos_y;
            } break;
            case TGUI_EVENT_MOUSEDOWN:
            {
                state->mouse_down = true;
                state->mouse_is_down = true;
            } break;
            case TGUI_EVENT_MOUSEUP:
            {
                state->mouse_up = true;
                state->mouse_is_down = false;
            } break;
            case TGUI_EVEN_BUTTON:
            {
                TGuiEventButton *button = (TGuiEventButton *)event;
                printf("button: %d clicked!\n", button->handle);
            } break;
            case TGUI_EVEN_FOCUS:
            {
            } break;
            case TGUI_EVENT_COUNT:
            {
                ASSERT(!"invalid path code\n");
            } break;
        }
    }
    state->event_queue.count = 0;
    
    // NOTE: update all widget in the state widget tree
    state->last_focus = state->focus;
    state->focus = 0;
    tgui_widget_recursive_descent_first_to_last(state->first_root, tgui_widget_update);
    tgui_widget_recursive_descent_first_to_last(state->first_root, tgui_widget_render);
}

void tgui_draw_command_buffer(void)
{
    TGuiState *state = &tgui_global_state;
    // NOTE: pull tgui draw commands from the buffer 
    TGuiDrawCommand draw_cmd;
    while(tgui_pull_draw_command(&draw_cmd))
    {
        switch(draw_cmd.type)
        {
            case TGUI_DRAWCMD_CLEAR:
            {
                tgui_clear_backbuffer(state->backbuffer);
            } break;
            case TGUI_DRAWCMD_START_CLIPPING:
            {
                tgui_clipping_stack_push(&global_clipping_stack, draw_cmd.descriptor);
            } break;
            case TGUI_DRAWCMD_END_CLIPPING:
            {
                tgui_clipping_stack_pop(&global_clipping_stack);
            } break;
            case TGUI_DRAWCMD_RECT:
            {
                u32 min_x = (u32)draw_cmd.descriptor.x;
                u32 min_y = (u32)draw_cmd.descriptor.y;
                u32 max_x = min_x + (u32)draw_cmd.descriptor.width;
                u32 max_y = min_y + (u32)draw_cmd.descriptor.height;
                tgui_draw_rect(state->backbuffer, min_x, min_y, max_x, max_y, draw_cmd.color);
            } break;
            case TGUI_DRAWCMD_ROUNDED_RECT:
            {
                i32 max_x = draw_cmd.descriptor.x + draw_cmd.descriptor.width;
                i32 max_y = draw_cmd.descriptor.y + draw_cmd.descriptor.height;
                tgui_draw_rounded_rect(state->backbuffer, draw_cmd.descriptor.x, draw_cmd.descriptor.y, max_x, max_y, draw_cmd.color, draw_cmd.ratio);
            } break;
            case TGUI_DRAWCMD_BITMAP:
            {
                tgui_draw_bitmap(state->backbuffer, draw_cmd.bitmap, draw_cmd.descriptor.x, draw_cmd.descriptor.y, draw_cmd.descriptor.width, draw_cmd.descriptor.height);
            } break;
            case TGUI_DRAWCMD_TEXT:
            {
                tgui_draw_text(state->backbuffer, state->font, state->font_height, draw_cmd.descriptor.x, draw_cmd.descriptor.y, draw_cmd.text);
            } break;
            case TGUI_DRAWCMD_COUNT:
            {
                ASSERT(!"invalid code path");
            } break;
        }
    }
}

//-----------------------------------------------------
// NOTE: DEBUG functions
//-----------------------------------------------------
// NOTE: only use in implementation
typedef struct TGuiBmpHeader
{
    u16 id;
    u32 file_size;
    u32 reserved;
    u32 pixel_array_offset;
    u32 dib_header_size;
    u32 width;
    u32 height;
    u16 planes;
    u16 bits_per_pixel;
    u32 compression;
    u32 bmp_size;
} TGuiBmpHeader;

// TODO: dont know if this macro is a good idea, probably not needed
#define READ_U16(data) *((u16 *)data); data = (u16 *)data + 1
#define READ_U32(data) *((u32 *)data); data = (u32 *)data + 1
#define READ_U64(data) *((u64 *)data); data = (u64 *)data + 1

TGuiBitmap tgui_debug_load_bmp(char *path)
{
    FILE *bmp_file = fopen(path, "rb");
    if(bmp_file)
    {
        fseek(bmp_file, 0, SEEK_END);
        u64 bmp_file_size = ftell(bmp_file);
        fseek(bmp_file, 0, SEEK_SET);
        void *bmp_data = (void *)malloc(bmp_file_size); 
        fread(bmp_data, bmp_file_size, 1, bmp_file);
        fclose(bmp_file);
        
        void *temp_bmp_data = bmp_data;
        TGuiBmpHeader bmp_header = {0};
        bmp_header.id =                 READ_U16(temp_bmp_data);
        bmp_header.file_size =          READ_U32(temp_bmp_data);
        bmp_header.reserved =           READ_U32(temp_bmp_data);
        bmp_header.pixel_array_offset = READ_U32(temp_bmp_data);
        bmp_header.dib_header_size =    READ_U32(temp_bmp_data);
        bmp_header.width =              READ_U32(temp_bmp_data);
        bmp_header.height =             READ_U32(temp_bmp_data);
        bmp_header.planes =             READ_U16(temp_bmp_data);
        bmp_header.bits_per_pixel =     READ_U16(temp_bmp_data);
        bmp_header.compression =        READ_U32(temp_bmp_data);
        bmp_header.bmp_size =           READ_U32(temp_bmp_data);
    
        // NOTE: for now only allow 32bits bitmaps
        u32 bytes_per_pixel = bmp_header.bits_per_pixel / 8;
        if(bytes_per_pixel != sizeof(u32))
        {
            ASSERT(!"[ERROR]: only support for 32 bits bitmaps\n");
            return (TGuiBitmap){0};
        }

        u8 *bmp_src = (u8 *)bmp_data + bmp_header.pixel_array_offset;
        TGuiBitmap result = {0};
        result.width = bmp_header.width;
        result.height = bmp_header.height;
        u64 bitmap_size = result.width * result.height * bytes_per_pixel;
        result.pixels = (u32 *)malloc(bitmap_size); 
        
        // NOTE: cannot use memcpy, the bitmap must be flipped 
        // TODO: copy the bitmap pixels in a faster way
        u32 *src_row = (u32 *)bmp_src + (result.height-1) * bmp_header.width;
        u32 *dst_row = result.pixels;
        for(u32 y = 0; y < result.height; ++y)
        {
            u32 *src_pixels = src_row;
            u32 *dst_pixels = dst_row;
            for(u32 x = 0; x < result.width; ++x)
            {
                
                *dst_pixels++ = *src_pixels++;
            }
            src_row -= bmp_header.width;
            dst_row += result.width;
        }
        
        free(bmp_data);
        return result;
    }
    
    // TODO: create a log to print errors
    ASSERT(!"[ERROR]: cannot open file\n");
    return (TGuiBitmap){0};
}

void tgui_debug_free_bmp(TGuiBitmap *bitmap)
{
    free(bitmap->pixels);
    bitmap->pixels = 0;
    bitmap->width = 0;
    bitmap->height = 0;
}

void tgui_clear_backbuffer(TGuiBitmap *backbuffer)
{
    size_t backbuffer_size = (backbuffer->width*backbuffer->height*sizeof(u32));
    memset(backbuffer->pixels, 0, backbuffer_size);
}

//-----------------------------------------------------
// NOTE: simple rendering API
//-----------------------------------------------------

typedef struct TGuiClipResult
{
    i32 min_x; 
    i32 min_y; 
    i32 max_x; 
    i32 max_y; 
    i32 offset_x;
    i32 offset_y;
} TGuiClipResult;

static TGuiClipResult tgui_clip_rect(i32 min_x, i32 min_y, i32 max_x, i32 max_y, TGuiRect clipping)
{
    i32 clip_min_x = (i32)clipping.x;
    i32 clip_min_y = (i32)clipping.y;
    i32 clip_max_x = clip_min_x + (i32)clipping.width;
    i32 clip_max_y = clip_min_y + (i32)clipping.height;
     
    i32 offset_x = 0;
    i32 offset_y = 0;
    if(min_x < clip_min_x)
    {
        offset_x = -(min_x - clip_min_x);
        min_x = clip_min_x;
    }
    if(max_x > clip_max_x)
    {
        max_x = clip_max_x;
    }
    if(min_y < clip_min_y)
    {
        offset_y = -(min_y - clip_min_y);
        min_y = clip_min_y;
    }
    if(max_y > clip_max_y)
    {
        max_y = clip_max_y;
    }

    TGuiClipResult result = {0};
    result.min_x = min_x;
    result.min_y = min_y;
    result.max_x = max_x;
    result.max_y = max_y;
    
    result.offset_x = offset_x;
    result.offset_y = offset_y;
    
    return result;
}

void tgui_draw_rect(TGuiBitmap *backbuffer, i32 min_x, i32 min_y, i32 max_x, i32 max_y, u32 color)
{
    TGuiClipResult clipping = tgui_clip_rect(min_x, min_y, max_x, max_y, tgui_clipping_stack_top(&global_clipping_stack));
    i32 width = clipping.max_x - clipping.min_x;
    i32 height = clipping.max_y - clipping.min_y;
    u8 *row = (u8 *)backbuffer->pixels + clipping.min_y * backbuffer->pitch;
    for(i32 y = 0; y < height; ++y)
    {
        u32 *pixels = (u32 *)row + clipping.min_x;
        for(i32 x = 0; x < width; ++x)
        {
            *pixels++ = color;
        }
        row += backbuffer->pitch;
    }
}

void tgui_draw_rounded_rect(TGuiBitmap *backbuffer, i32 min_x, i32 min_y, i32 max_x, i32 max_y, u32 color, u32 radius)
{
    f32 diameter = radius*2;
    TGuiRect top_left = tgui_rect_xywh(min_x, min_y, diameter, diameter);
    TGuiRect top_right = tgui_rect_xywh(max_x - diameter, min_y, diameter, diameter);
    TGuiRect bottom_right = tgui_rect_xywh(max_x - diameter, max_y - diameter, diameter, diameter);
    TGuiRect bottom_left = tgui_rect_xywh(min_x, max_y - diameter, diameter, diameter);

    TGuiClipResult clipping = tgui_clip_rect(min_x, min_y, max_x, max_y, tgui_clipping_stack_top(&global_clipping_stack));
    i32 width = clipping.max_x - clipping.min_x;
    i32 height = clipping.max_y - clipping.min_y;
    
    u8 *row = (u8 *)backbuffer->pixels + clipping.min_y * backbuffer->pitch;
    for(i32 y = 0; y < height; ++y)
    {
        u32 *pixels = (u32 *)row + clipping.min_x;
        for(i32 x = 0; x < width; ++x)
        {
            u32 *pixel = pixels++;
            f32 alpha = 1.0f;
            
            TGuiV2 pos = tgui_v2(x + clipping.min_x, y + clipping.min_y);
            if(tgui_point_inside_rect(pos, top_left))
            {
                TGuiV2 center = tgui_v2(top_left.x + radius, top_left.y + radius);
                f32 distance = tgui_v2_length(tgui_v2_sub(pos, center));
                i32 distance_i = floorf(distance);
                f32 A = distance - (f32)distance_i;
                if((pos.x <= center.x && pos.y <= center.y) && (distance_i == (i32)radius)) alpha = (1.0f - A);
                if((pos.x < center.x && pos.y < center.y) && distance_i > (i32)radius)
                {
                    continue;
                }
            }
            else if(tgui_point_inside_rect(pos, top_right))
            {
                ++pos.x;
                TGuiV2 center = tgui_v2(top_right.x + radius, top_right.y + radius);
                f32 distance = tgui_v2_length(tgui_v2_sub(pos, center));
                i32 distance_i = floorf(distance);
                f32 A = distance - (f32)distance_i;
                if((pos.x >= center.x && pos.y <= center.y) && (distance_i == (i32)radius)) alpha = (1.0f - A);
                if((pos.x > center.x && pos.y < center.y) && distance_i > (i32)radius)
                {
                    continue;
                }
            }
            else if(tgui_point_inside_rect(pos, bottom_right))
            {
                ++pos.x;
                ++pos.y;
                TGuiV2 center = tgui_v2(bottom_right.x + radius, bottom_right.y + radius);
                f32 distance = tgui_v2_length(tgui_v2_sub(pos, center));
                i32 distance_i = floorf(distance);
                f32 A = distance - (f32)distance_i;
                if((pos.x >= center.x && pos.y >= center.y) && (distance_i == (i32)radius)) alpha = (1.0f - A);
                if((pos.x > center.x && pos.y > center.y) && distance_i > (i32)radius)
                {
                    continue;
                }
            }
            else if(tgui_point_inside_rect(pos, bottom_left))
            {
                ++pos.y;
                TGuiV2 center = tgui_v2(bottom_left.x + radius, bottom_left.y + radius);
                f32 distance = tgui_v2_length(tgui_v2_sub(pos, center));
                i32 distance_i = floorf(distance);
                f32 A = distance - (f32)distance_i;
                if((pos.x <= center.x && pos.y >= center.y) && (distance_i == (i32)radius)) alpha = (1.0f - A);
                if((pos.x < center.x && pos.y > center.y) && distance_i > (i32)radius)
                {
                    continue;
                }
            }
            
            u32 red =   (u32)((color >> 16) & 0xFF) * alpha;
            u32 green = (u32)((color >>  8) & 0xFF) * alpha;
            u32 blue =  (u32)((color >>  0) & 0xFF) * alpha;
            
            *pixel = (255 << 24 | red << 16 | green << 8 | blue << 0);
        }
        row += backbuffer->pitch;
    }
}

void tgui_draw_circle_aa(TGuiBitmap *backbuffer, i32 x, i32 y, u32 color, u32 radius)
{
    TGuiRect rect = tgui_rect_xywh(x - (f32)radius, y - (f32)radius, radius*2, radius*2);
    TGuiClipResult clipping = tgui_clip_rect(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height, tgui_clipping_stack_top(&global_clipping_stack));
    i32 min_x = clipping.min_x;
    i32 min_y = clipping.min_y;
    i32 max_x = clipping.max_x;
    i32 max_y = clipping.max_y;

    u8 *row = (u8 *)backbuffer->pixels + min_y * backbuffer->pitch;
    for(i32 pixel_y = min_y; pixel_y < max_y; ++pixel_y)
    {
        u32 *pixels = (u32 *)row + min_x;
        for(i32 pixel_x = min_x; pixel_x < max_x; ++pixel_x)
        {
            u32 *pixel = pixels++;
            TGuiV2 center = tgui_v2(x , y);
            TGuiV2 pos = tgui_v2(pixel_x , pixel_y);
            if(pos.x > center.x) ++pos.x;
            if(pos.y > center.y) ++pos.y;
            TGuiV2 center_pos = tgui_v2_sub(pos, center);
            
            f32 distance = tgui_v2_length(center_pos);
            i32 distance_i = floorf(distance);
            f32 A = distance - (f32)distance_i;
            
            if(distance_i <= (i32)radius)
            {
                f32 alpha = distance_i == (i32)radius ? (1.0f - A) : 1.0f;
                u32 red =   (u32)((color >> 16) & 0xFF) * alpha;
                u32 green = (u32)((color >>  8) & 0xFF) * alpha;
                u32 blue =  (u32)((color >>  0) & 0xFF) * alpha;
                
                *pixel = (255 << 24 | red << 16 | green << 8 | blue << 0);
            }
        }
        row += backbuffer->pitch;
    }
}

void tgui_copy_bitmap(TGuiBitmap *backbuffer, TGuiBitmap *bitmap, i32 x, i32 y)
{
    i32 min_x = x;
    i32 min_y = y;
    i32 max_x = min_x + bitmap->width;
    i32 max_y = min_y + bitmap->height;
     
    TGuiClipResult clipping = tgui_clip_rect(min_x, min_y, max_x, max_y, tgui_clipping_stack_top(&global_clipping_stack));
    i32 width = clipping.max_x - clipping.min_x;
    i32 height = clipping.max_y - clipping.min_y;

    u8 *row = (u8 *)backbuffer->pixels + clipping.min_y * backbuffer->pitch;
    u32 *bmp_row = bitmap->pixels + clipping.offset_y * bitmap->width;
    for(i32 y = 0; y < height; ++y)
    {
        u32 *pixels = (u32 *)row + clipping.min_x;
        u32 *bmp_pixels = bmp_row + clipping.offset_x;
        for(i32 x = 0; x < width; ++x)
        {
            *pixels++ = *bmp_pixels++;
        }
        row += backbuffer->pitch;
        bmp_row += bitmap->width;
    }
}

// TODO: create a tgui_round_f32u32() function
void tgui_draw_src_dest_bitmap(TGuiBitmap *backbuffer, TGuiBitmap *bitmap, TGuiRect src, TGuiRect dest)
{
    // TODO: Implment alpha bending
    // TODO: Implment filtering for the re-scale bitmap
    i32 min_x = dest.x;
    i32 min_y = dest.y;
    i32 max_x = min_x + dest.width;
    i32 max_y = min_y + dest.height;
     
    TGuiClipResult clipping = tgui_clip_rect(min_x, min_y, max_x, max_y, tgui_clipping_stack_top(&global_clipping_stack));
    i32 dest_width = clipping.max_x - clipping.min_x;
    i32 dest_height = clipping.max_y - clipping.min_y;
    
    i32 src_min_x = src.x;
    i32 src_min_y = src.y;
    i32 src_max_x = src_min_x + src.width;
    i32 src_max_y = src_min_y + src.height;
     
    i32 src_width = src_max_x - src_min_x;
    i32 src_height = src_max_y - src_min_y;

    u8 *row = (u8 *)backbuffer->pixels + clipping.min_y * backbuffer->pitch;
    for(i32 y = 0; y < dest_height; ++y)
    {
        f32 ratio_y = (f32)(y + clipping.offset_y) / (f32)dest.height;
        u32 bitmap_y = src_min_y + (u32)((f32)src_height * ratio_y + 0.5f);
        u32 *pixels = (u32 *)row + clipping.min_x;
        for(i32 x = 0; x < dest_width; ++x)
        {
            f32 ratio_x = (f32)(x + clipping.offset_x) / (f32)dest.width;
            u32 bitmap_x = src_min_x + (u32)((f32)src_width * ratio_x + 0.5f);
            u32 src_color = bitmap->pixels[bitmap_y*bitmap->width + bitmap_x];
            // TODO: implements real alpha bending
            u8 alpha = (u8)((src_color >> 24) & 0xFF);
            if(alpha > 128) 
            {
                *pixels = src_color;
            }
            pixels++;
        }
        row += backbuffer->pitch;
    }
}

void tgui_draw_bitmap(TGuiBitmap *backbuffer, TGuiBitmap *bitmap, i32 x, i32 y, i32 width, i32 height)
{
    TGuiRect src;
    src.x = 0;
    src.y = 0;
    src.width = bitmap->width;
    src.height = bitmap->height;
    
    TGuiRect dest;
    dest.x = x;
    dest.y = y;
    dest.width = width;
    dest.height = height;

    tgui_draw_src_dest_bitmap(backbuffer, bitmap, src, dest);
}

// NOTE: font funtions

TGuiFont tgui_create_font(TGuiBitmap *bitmap, u32 char_width, u32 char_height, u32 num_rows, u32 num_cols)
{
    TGuiFont result = {0};
    result.src_rect.x = 0;
    result.src_rect.y = 0;
    result.src_rect.width = char_width;
    result.src_rect.height = char_height;
    result.num_rows = num_rows;
    result.num_cols = num_cols;
    result.bitmap = bitmap;
    return result;
}

void tgui_draw_char(TGuiBitmap *backbuffer, TGuiFont *font, u32 height, i32 x, i32 y, char character)
{
    ASSERT(font->bitmap && "font must have a bitmap");
    u32 index = (character - ' ');
    u32 x_index = index % font->num_rows;
    u32 y_index = index / font->num_rows;
    
    TGuiRect src;
    src.x = x_index*font->src_rect.width;
    src.y = y_index*font->src_rect.height;
    src.width = font->src_rect.width;
    src.height = font->src_rect.height;
   
    f32 w_ration = (f32)font->src_rect.width / (f32)font->src_rect.height;
    TGuiRect dest;
    dest.x = x;
    dest.y = y;
    dest.width = (w_ration * (f32)height + 0.5f);
    dest.height = height;

    tgui_draw_src_dest_bitmap(backbuffer, font->bitmap, src, dest); 
}

void tgui_draw_text(TGuiBitmap *backbuffer, TGuiFont *font, u32 height, i32 x, i32 y, char *text)
{
    // TODO: the w_ratio is been calculated in two time
    f32 w_ration = (f32)font->src_rect.width / (f32)font->src_rect.height;
    u32 width = (u32)(w_ration * (f32)height + 0.5f);
     
    u32 length = strlen((const char *)text);
    for(u32 index = 0; index < length; ++index)
    {
        tgui_draw_char(backbuffer, font, height, x, y, text[index]);
        x += width;
    }
}

u32 tgui_text_get_width(TGuiFont *font, char *text, u32 height)
{
    f32 w_ration = (f32)font->src_rect.width / (f32)font->src_rect.height;
    u32 width = (u32)(w_ration * (f32)height + 0.5f);
     
    u32 length = strlen((const char *)text);
    u32 result = width * length;
    return result;
}
