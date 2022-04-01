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

// Internal input state pointer
static input_state* state_ptr;

//inplementations

void initialize_inputs_system(u64* memory_requirement, void* state){
    *memory_requirement = sizeof(input_state);
    if (state == 0) {
        return;
    }
    //actully we don't have zero out memory for state because it is a static var and it is zeroed by default
    pancake_zero_memory(state,sizeof(input_state));
    state_ptr = state;
    PANCAKE_INFO("Inputs system had been intialized .");
}
void shutdown_inputs_system(void* state){
    //TODO: shutdown routines when needed
    state_ptr = 0;
}
void inputs_update(f64 delta_time){
    if(!state_ptr) return;
    
    //copy current states to previous states
    pancake_copy_memory(&state_ptr->keyboard_previous,&state_ptr->keyboard_current,sizeof(keyboard_state));
    pancake_copy_memory(&state_ptr->mouse_previous,&state_ptr->mouse_current,sizeof(mouse_state));
}

//keyboar inputs
PANCAKE_API b8 input_key_is_down(keys key){
    if(!state_ptr) return false;

    return state_ptr->keyboard_current.keys[key] == true;
}
PANCAKE_API b8 input_key_is_up(keys key){
    if(!state_ptr) return true;

    return state_ptr->keyboard_current.keys[key] == false;
}
PANCAKE_API b8 input_key_was_down(keys key){
    if(!state_ptr) return false;

    return state_ptr->keyboard_previous.keys[key] == true;
}
PANCAKE_API b8 input_key_was_up(keys key){
    if(!state_ptr) return true;

    return state_ptr->keyboard_previous.keys[key] == false;
}

void input_process_key(keys key, b8 pressed){

    //only handle this if the state had been changed.
    if(state_ptr && state_ptr->keyboard_current.keys[key] != pressed){
        //update internal state
        state_ptr->keyboard_current.keys[key] = pressed;
        
        if (key == KEY_LALT) {
            PANCAKE_INFO("Left alt %s.", pressed ? "pressed" : "released");
        } else if (key == KEY_RALT) {
            PANCAKE_INFO("Right alt %s.", pressed ? "pressed" : "released");
        }

        if (key == KEY_LCONTROL) {
            PANCAKE_INFO("Left ctrl %s.", pressed ? "pressed" : "released");
        } else if (key == KEY_RCONTROL) {
            PANCAKE_INFO("Right ctrl %s.", pressed ? "pressed" : "released");
        }

        if (key == KEY_LSHIFT) {
            PANCAKE_INFO("Left shift %s.", pressed ? "pressed" : "released");
        } else if (key == KEY_RSHIFT) {
            PANCAKE_INFO("Right shift %s.", pressed ? "pressed" : "released");
        }

        //fire off an event for immediate processing
        event_context context;
        context.data.u16[0] = key;
        fire_event(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, 0, context);
    }
}


//mouse inputs
PANCAKE_API b8 input_mouse_button_is_down(m_buttons button){
    if(!state_ptr) return false;

    return state_ptr->mouse_current.buttons[button] == true;
}
PANCAKE_API b8 input_mouse_button_is_up(m_buttons button){
    if(!state_ptr) return true;

    return state_ptr->mouse_current.buttons[button] == false;
}
PANCAKE_API b8 input_mouse_button_was_down(m_buttons button){
    if(!state_ptr) return false;

    return state_ptr->mouse_previous.buttons[button] == true;
}
PANCAKE_API b8 input_mouse_button_was_up(m_buttons button){
    if(!state_ptr) return true;

    return state_ptr->mouse_previous.buttons[button] == false;
}
PANCAKE_API void input_mouse_get_position(f32* x, f32* y){
    if(!state_ptr){
        *x = 0;
        *y = 0;
        return;
    }

    *x = state_ptr->mouse_current.x;
    *y = state_ptr->mouse_current.y;
}
PANCAKE_API void input_mouse_get_previous_position(f32* x, f32* y){
    if(!state_ptr){
        *x = 0;
        *y = 0;
        return;
    }

    *x = state_ptr->mouse_previous.x;
    *y = state_ptr->mouse_previous.y;
}

void input_process_mouse_button(m_buttons button, b8 pressed){
    //if the state changed, fire an event
    if(state_ptr->mouse_current.buttons[button] != pressed){
        state_ptr->mouse_current.buttons[button] = pressed;

        //fire the event
        event_context context;
        context.data.u16[0] = button;
        fire_event(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, 0, context);
    }
}
void input_process_mouse_move(i16 x, i16 y){
    //if the postion changed, fire an event
    if(state_ptr->mouse_current.x != x || state_ptr->mouse_current.y != y ){
        //NOTE: enable this in debugging
        //PANCAKE_DEBUG("mouse postion => (%i , %i)", x, y);

        //update internal state
        state_ptr->mouse_current.x = x;
        state_ptr->mouse_current.y = y;

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

