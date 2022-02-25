#pragma once

#include "defines.h"

typedef struct event_context{
    //128 bytes
    union{
        i64 i64[2];
        u64 u64[2];
        f64 f64[2];

        i32 i32[4];
        u32 u32[4];
        f32 f32[4];

        i16 i16[8];
        u16 u16[8];

        char c[16];
    }data;
}event_context;

//ON EVENT FUNCTION POINTER
//should return true if handled
typedef b8 (*on_event_fnp)(u16 code, void* sender, void* lestener_inst, event_context data);

b8 initialize_evnets();
void shutdown_event();

/*
    Register to listen for when events are sent with the provided code.
    Events with a duplicated listener/Callback combos will not be registered again and so, the return value will be FALSE
    @param code : the event code to listen for
    @param listener : a pointer to a listener instance, can be 0/NULL
    @param on_event : the Callback function pointer to be invoked when the event code is fired
    @return TRUE if the event is successfully registered, Otherwise FALSE
*/
PANCAKE_API b8 register_event(u16 code, void* listener, on_event_fnp on_event);

/*
    Unregister from listening for when events are sent with the provided code.
    If no matching registration is found the return value will be FALSE
    @param code : the event code to stop listening for
    @param listener : a pointer to a listener instance, can be 0/NULL
    @param on_event : the Callback function pointer to be unregistered
    @return TRUE if the event is successfully unregistered, Otherwise FALSE
*/
PANCAKE_API b8 unregister_event(u16 code, void* listener, on_event_fnp on_event);

/*
    Fire an event to listeners of the given code .
    If an event handler returns TRUE, the event is considered handled and is not passed on to any mor listeners. 
    @param code : the event code to fire .
    @param sender : a pointer to the sender, can be 0/NULL .
    @param context : the event data .
    @return TRUE if handled, Otherwise FALSE
*/
PANCAKE_API b8 fire_event(u16 code, void* sender, event_context context);

// System internal event codes. Application should use codes beyond 255.
typedef enum system_event_code {
    // Shuts the application down on the next frame.
    EVENT_CODE_APPLICATION_QUIT = 0x01,

    // Keyboard key pressed.
    /* Context usage:
     * u16 key_code = data.data.u16[0];
     */
    EVENT_CODE_KEY_PRESSED = 0x02,

    // Keyboard key released.
    /* Context usage:
     * u16 key_code = data.data.u16[0];
     */
    EVENT_CODE_KEY_RELEASED = 0x03,

    // Mouse button pressed.
    /* Context usage:
     * u16 button = data.data.u16[0];
     */
    EVENT_CODE_BUTTON_PRESSED = 0x04,

    // Mouse button released.
    /* Context usage:
     * u16 button = data.data.u16[0];
     */
    EVENT_CODE_BUTTON_RELEASED = 0x05,

    // Mouse moved.
    /* Context usage:
     * u16 x = data.data.u16[0];
     * u16 y = data.data.u16[1];
     */
    EVENT_CODE_MOUSE_MOVED = 0x06,

    // Mouse wheel roled.
    /* Context usage:
     * u8 z_delta = data.data.u8[0];
     */
    EVENT_CODE_MOUSE_WHEEL = 0x07,

    // Resized/resolution changed from the OS.
    /* Context usage:
     * u16 width = data.data.u16[0];
     * u16 height = data.data.u16[1];
     */
    EVENT_CODE_RESIZED = 0x08,

    MAX_EVENT_CODE = 0xFF
} system_event_code; 