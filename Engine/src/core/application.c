#include "application.h"
#include "platform/platform.h"
#include "logger.h"
#include "game_types.h"
#include "core/pancake_memory.h"
#include "core/event.h"
#include "core/inputs.h"

typedef struct application_state{
    game* game_inst;
    b8 is_running;
    b8 is_suspended;
    platform_state platform;
    i16 width;
    i16 height;
    f64 last_time;
}application_state;

static b8 initialized = FALSE;
static application_state app_state;


b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context);
b8 application_on_key(u16 code, void* sender, void* listener_inst, event_context context);


b8 application_create(game* game_inst){
    if(initialized){
        PANCAKE_ERROR("application_create called more than once .");
        return FALSE;
    }

    app_state.game_inst = game_inst;

    //initialize subsystems
    initialize_logging();
    inputs_initialize();

    //TODO: REMOVE THESE
    PANCAKE_FATAL("A test message: %f", 3.14f);
    PANCAKE_ERROR("A test message: %f", 3.14f);
    PANCAKE_WARN("A test message: %f", 3.14f);
    PANCAKE_INFO("A test message: %f", 3.14f);
    PANCAKE_DEBUG("A test message: %f", 3.14f);
    PANCAKE_TRACE("A test message: %f", 3.14f);

    app_state.is_running = TRUE;
    app_state.is_suspended = FALSE;
    
    if(!event_initialize()){
        PANCAKE_FATAL("Events system failed initialization, cannot continue with the application !")
        return FALSE;
    }
    
    //listen for events...
    register_event(EVENT_CODE_APPLICATION_QUIT,0,application_on_event);
    register_event(EVENT_CODE_KEY_PRESSED,0,application_on_key);
    register_event(EVENT_CODE_KEY_RELEASED,0,application_on_key);

    if(!platform_startup(
            &app_state.platform,
            game_inst->config.name,
            game_inst->config.start_pos_x,
            game_inst->config.start_pos_y,
            game_inst->config.start_width,
            game_inst->config.start_height)){
        return FALSE;
    }
    
    //Initialize the game
    if(!app_state.game_inst->Initialize(app_state.game_inst)){
        PANCAKE_FATAL("Could not Initialize the game !");
        return FALSE;
    }

    app_state.game_inst->OnResize(app_state.game_inst,app_state.width,app_state.height);

    initialized = TRUE;
    return TRUE;
}
b8 application_run(){
    PANCAKE_INFO(get_memory_usage_str());
    while(app_state.is_running){
        if(!platform_pump_messages(&app_state.platform)){
            app_state.is_running = FALSE;
        }

        if(!app_state.is_suspended){
            if(!app_state.game_inst->Update(app_state.game_inst,(f32)0)){
                PANCAKE_FATAL("Game Update failed shutting down !");
                app_state.is_running = FALSE;
                break;
            }

            if(!app_state.game_inst->Redner(app_state.game_inst,(f32)0)){
                PANCAKE_FATAL("Game Redner failed shutting down !");
                app_state.is_running = FALSE;
                break;
            }

            // NOTE: Input update/state copying should always be handled
            // after any input should be recorded; I.E. before this line.
            // As a safety, input is the last thing to be updated before
            // this frame ends.
            inputs_update(0);
        }
    }

    app_state.is_running = FALSE;

    //Unregister events
    unregister_event(EVENT_CODE_APPLICATION_QUIT,0,application_on_event);
    unregister_event(EVENT_CODE_KEY_PRESSED,0,application_on_key);
    unregister_event(EVENT_CODE_KEY_RELEASED,0,application_on_key);

    event_shutdown();
    inputs_shatdown();
    platform_shutdown(&app_state.platform);

    return TRUE;
}

b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context) {
    switch (code) {
        case EVENT_CODE_APPLICATION_QUIT: {
            PANCAKE_INFO("EVENT_CODE_APPLICATION_QUIT recieved, shutting down.\n");
            app_state.is_running = FALSE;
            return TRUE;
        }
    }

    return FALSE;
}

b8 application_on_key(u16 code, void* sender, void* listener_inst, event_context context) {
    if (code == EVENT_CODE_KEY_PRESSED) {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_ESCAPE) {
            // NOTE: Technically firing an event to itself, but there may be other listeners.
            event_context data = {};
            fire_event(EVENT_CODE_APPLICATION_QUIT, 0, data);

            // Block anything else from processing this.
            return TRUE;
        } else if (key_code == KEY_A) {
            // Example on checking for a key
            PANCAKE_DEBUG("Explicit - A key pressed!");
        } else {
            PANCAKE_DEBUG("'%c' key pressed in window.", key_code);
        }
    } else if (code == EVENT_CODE_KEY_RELEASED) {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_B) {
            // Example on checking for a key
            PANCAKE_DEBUG("Explicit - B key released!");
        } else {
            PANCAKE_DEBUG("'%c' key released in window.", key_code);
        }
    }
    return FALSE;
}