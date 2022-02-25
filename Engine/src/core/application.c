#include "application.h"
#include "platform/platform.h"
#include "logger.h"
#include "game_types.h"
#include "core/pancake_memory.h"
#include "core/event.h"
#include "core/inputs.h"
#include "core/clock.h"
#include "renderer/renderer_frontend.h"

typedef struct application_state{
    game* game_inst;
    b8 is_running;
    b8 is_suspended;
    platform_state platform;
    i16 width;
    i16 height;
    Clock clock;
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
    initialize_inputs();

    app_state.is_running = TRUE;
    app_state.is_suspended = FALSE;
    
    if(!initialize_evnets()){
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
    
    //initialize the renderer
    if(!initialize_renderer(game_inst->config.name, &app_state.platform)){
        PANCAKE_FATAL("Failed to initialize the renderer, Aborting application...");
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
    clock_start(&app_state.clock);
    clock_update(&app_state.clock);
    app_state.last_time = app_state.clock.elapsed;

    f64 running_time = 0;
    u8 frame_count = 0;
    f64 target_frame_seconds = 1.0f / 60;


    PANCAKE_INFO(get_memory_usage_str());
    while(app_state.is_running){
        if(!platform_pump_messages(&app_state.platform)){
            app_state.is_running = FALSE;
        }

        if(!app_state.is_suspended){

            //update clock,and get delta_time
            clock_update(&app_state.clock);
            f64 current_time =app_state.clock.elapsed;
            f64 delta = (current_time - app_state.last_time);
            f64 frame_start_time = platform_get_absolute_time();

            if(!app_state.game_inst->Update(app_state.game_inst,(f32)delta)){
                PANCAKE_FATAL("Game Update failed shutting down !");
                app_state.is_running = FALSE;
                break;
            }

            //call the game's render routine
            if(!app_state.game_inst->Redner(app_state.game_inst,(f32)delta)){
                PANCAKE_FATAL("Game Redner failed shutting down !");
                app_state.is_running = FALSE;
                break;
            }
            renderer_packet packet;
            packet.delta_time = delta;
            renderer_draw_frame(&packet);

            //figure out how long the frame took, and if bleow
            f64 frame_end_time = platform_get_absolute_time();
            f64 frame_elapsed_time = frame_end_time - frame_start_time;
            running_time += frame_elapsed_time;
            f64 remaining_seconds = target_frame_seconds - frame_elapsed_time;

            if(remaining_seconds > 0){
                u64 remaining_ms = (remaining_seconds * 1000);

                //if there is time lef, give it back to the OS
                b8 limit_frames = FALSE;
                if(remaining_ms > 0 && limit_frames){
                    platform_sleep(remaining_ms - 1);
                }

                frame_count++;
            }

            // NOTE: Input update/state copying should always be handled
            // after any input should be recorded; I.E. before this line.
            // As a safety, input is the last thing to be updated before
            // this frame ends.
            inputs_update(delta);

            app_state.last_time = current_time;
        }
    }

    app_state.is_running = FALSE;

    //Unregister events
    unregister_event(EVENT_CODE_APPLICATION_QUIT,0,application_on_event);
    unregister_event(EVENT_CODE_KEY_PRESSED,0,application_on_key);
    unregister_event(EVENT_CODE_KEY_RELEASED,0,application_on_key);

    shutdown_event();
    shutdown_inputs();
    shutdown_renderer();
    shutdown_platform(&app_state.platform);

    return TRUE;
}

b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context) {
    switch (code) {
        case EVENT_CODE_APPLICATION_QUIT: {
            PANCAKE_INFO("EVENT_CODE_APPLICATION_QUIT recieved, shutting down.");
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