#include "inputs.h"
#include "event.h"
#include "pancake_memory.h"
#include "logger.h"

typedef struct keyboard_state{
    b8 keys[256];
} keyboard_state;

typedef struct mouse_state{
    i16 x;
    i16 y;
    u8 buttons[BUTTON_MAX_BUTTONS];
} mouse_state;

typedef struct input_state{
    keyboard_state keyboard_current;
    keyboard_state keyboard_previous;
    mouse_state mouse_current;
    mouse_state mouse_previous;
} input_state;

//internal state
static b8 initialized = false;
static input_state state = {};

//inplementations

void initialize_inputs(){
    //actully we don't have zero out memory for state because it is a static var and it is zeroed by default
    pancake_zero_memory(&state,sizeof(input_state));
    initialized = true;
    PANCAKE_INFO("Inputs system had been intialized .");
}
void shutdown_inputs(){
    //TODO: shutdown routines when needed
    initialized = false;
}
void inputs_update(f64 delta_time){
    if(!initialized) return;
    
    //copy current states to previous states
    pancake_copy_memory(&state.keyboard_previous,&state.keyboard_current,sizeof(keyboard_state));
    pancake_copy_memory(&state.mouse_previous,&state.mouse_current,sizeof(mouse_state));
}

//keyboar inputs
PANCAKE_API b8 input_key_is_down(keys key){
    if(!initialized) return false;

    return state.keyboard_current.keys[key] == true;
}
PANCAKE_API b8 input_key_is_up(keys key){
    if(!initialized) return true;

    return state.keyboard_current.keys[key] == false;
}
PANCAKE_API b8 input_key_was_down(keys key){
    if(!initialized) return false;

    return state.keyboard_previous.keys[key] == true;
}
PANCAKE_API b8 input_key_was_up(keys key){
    if(!initialized) return true;

    return state.keyboard_previous.keys[key] == false;
}

void input_process_key(keys key, b8 pressed){
    //only handle this if the state had been changed.
    if(state.keyboard_current.keys[key] != pressed){
        //update internal state
        state.keyboard_current.keys[key] = pressed;

        //fire off an event for immediate processing
        event_context context;
        context.data.u16[0] = key;
        fire_event(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, 0, context);
    }
}


//mouse inputs
PANCAKE_API b8 input_mouse_button_is_down(m_buttons button){
    if(!initialized) return false;

    return state.mouse_current.buttons[button] == true;
}
PANCAKE_API b8 input_mouse_button_is_up(m_buttons button){
    if(!initialized) return true;

    return state.mouse_current.buttons[button] == false;
}
PANCAKE_API b8 input_mouse_button_was_down(m_buttons button){
    if(!initialized) return false;

    return state.mouse_previous.buttons[button] == true;
}
PANCAKE_API b8 input_mouse_button_was_up(m_buttons button){
    if(!initialized) return true;

    return state.mouse_previous.buttons[button] == false;
}
PANCAKE_API void input_mouse_get_position(f32* x, f32* y){
    if(!initialized){
        *x = 0;
        *y = 0;
        return;
    }

    *x = state.mouse_current.x;
    *y = state.mouse_current.y;
}
PANCAKE_API void input_mouse_get_previous_position(f32* x, f32* y){
    if(!initialized){
        *x = 0;
        *y = 0;
        return;
    }

    *x = state.mouse_previous.x;
    *y = state.mouse_previous.y;
}

void input_process_mouse_button(m_buttons button, b8 pressed){
    //if the state changed, fire an event
    if(state.mouse_current.buttons[button] != pressed){
        state.mouse_current.buttons[button] = pressed;

        //fire the event
        event_context context;
        context.data.u16[0] = button;
        fire_event(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, 0, context);
    }
}
void input_process_mouse_move(i16 x, i16 y){
    //if the postion changed, fire an event
    if(state.mouse_current.x != x || state.mouse_current.y != y ){
        //NOTE: enable this in debugging
        //PANCAKE_DEBUG("mouse postion => (%i , %i)", x, y);

        //update internal state
        state.mouse_current.x = x;
        state.mouse_current.y = y;

        //fire an event
        event_context context;
        context.data.u16[0] = x;
        context.data.u16[1] = y;
        fire_event(EVENT_CODE_MOUSE_MOVED, 0, context);
    }
}
void input_process_mouse_wheel(i8 z_delta){
    //NOTE: no internal state to update

    //fire an event
    event_context context;
    context.data.u16[0] = z_delta;
    fire_event(EVENT_CODE_MOUSE_WHEEL, 0, context);
}

