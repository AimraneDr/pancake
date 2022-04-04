#include "application.h"
#include "platform/platform.h"
#include "logger.h"
#include "game_types.h"
#include "core/pancake_memory.h"
#include "memory\linear_allocator.h"
#include "core/event.h"
#include "core/inputs.h"
#include "core/clock.h"
#include "renderer/renderer_frontend.h"

typedef struct application_state{
    game* game_inst;
    b8 is_running;
    b8 is_suspended;
    i16 width;
    i16 height;
    Clock clock;
    f64 last_time;
    linear_allocator systems_allocator;
    
    u64 event_system_memory_requirement;
    void* event_system_state_ptr;

    u64 memory_system_memory_requirement;
    void* memory_system_state_ptr;

    u64 logging_system_memory_requirement;
    void* logging_system_state_ptr;

    u64 input_system_memory_requirement;
    void* input_system_state_ptr;

    u64 platform_system_memory_requirement;
    void* platform_system_state_ptr;

    u64 renderer_system_memory_requirement;
    void* renderer_system_state_ptr;
}application_state;

static application_state* app_state;


b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context);
b8 application_on_key(u16 code, void* sender, void* listener_inst, event_context context);
b8 application_on_resize(u16 code, void* sender, void* listner_inst, event_context context);

b8 application_create(game* game_inst){
    if(game_inst->application_state){
        PANCAKE_ERROR("application_create called more than once .");
        return false;
    }

    game_inst->application_state = pancake_allocate(sizeof(application_state), MEMORY_TAG_AAPLICATION);
    app_state = game_inst->application_state;
    app_state->game_inst = game_inst;
    app_state->is_running = false;
    app_state->is_suspended = false;

    u64 systems_allocator_total_size = 64 * 1024 * 1024;    //64Mb
    linear_allocator_create(systems_allocator_total_size, 0, &app_state->systems_allocator);

    //initialize subsystems



    //initialize memory sub-system
    initialize_memory_system(&app_state->memory_system_memory_requirement, 0);
    app_state->memory_system_state_ptr =  linear_allocator_allocate(&app_state->systems_allocator, app_state->memory_system_memory_requirement);
    initialize_memory_system(&app_state->memory_system_memory_requirement, app_state->memory_system_state_ptr);

    //initialize logging sub-system
    initialize_logging_system(&app_state->logging_system_memory_requirement, 0);
    app_state->logging_system_state_ptr =  linear_allocator_allocate(&app_state->systems_allocator, app_state->logging_system_memory_requirement);
    if(!initialize_logging_system(&app_state->logging_system_memory_requirement, app_state->logging_system_state_ptr)){
        PANCAKE_ERROR("Failed to inintialize Logging System, Shutting Down...");
        return false;
    }

    //Initialize Input sub-system
    initialize_inputs_system(&app_state->input_system_memory_requirement, 0);
    app_state->input_system_state_ptr = linear_allocator_allocate(&app_state->systems_allocator, app_state->input_system_memory_requirement);
    initialize_inputs_system(&app_state->input_system_memory_requirement, app_state->input_system_state_ptr);

    //initialize events sub-system
    initialize_evnets_system(&app_state->event_system_memory_requirement, 0);
    app_state->event_system_state_ptr = linear_allocator_allocate(&app_state->systems_allocator, app_state->event_system_memory_requirement);
    initialize_evnets_system(&app_state->event_system_memory_requirement, app_state->event_system_state_ptr);


    
    //listen for events...
    register_event(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    register_event(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    register_event(EVENT_CODE_KEY_RELEASED, 0, application_on_key);
    register_event(EVENT_CODE_RESIZED, 0, application_on_resize);


    //platform
    platform_system_startup(&app_state->platform_system_memory_requirement, 0, 0, 0, 0, 0, 0);
    app_state->platform_system_state_ptr = linear_allocator_allocate(&app_state->systems_allocator, app_state->platform_system_memory_requirement);
     if (!platform_system_startup(
            &app_state->platform_system_memory_requirement,
            app_state->platform_system_state_ptr,
            game_inst->config.name,
            game_inst->config.start_pos_x,
            game_inst->config.start_pos_y,
            game_inst->config.start_width,
            game_inst->config.start_height)) {
        return false;
    }



    // Renderer system
    initialize_renderer_system(&app_state->renderer_system_memory_requirement, 0, 0);
    app_state->renderer_system_state_ptr = linear_allocator_allocate(&app_state->systems_allocator, app_state->renderer_system_memory_requirement);
    if (!initialize_renderer_system(&app_state->renderer_system_memory_requirement, app_state->renderer_system_state_ptr, game_inst->config.name)) {
        PANCAKE_FATAL("Failed to initialize renderer. Aborting application.");
        return false;
    }



    //Initialize the game
    if(!app_state->game_inst->Initialize(app_state->game_inst)){
        PANCAKE_FATAL("Could not Initialize the game !");
        return false;
    }

    app_state->game_inst->OnResize(app_state->game_inst,app_state->width,app_state->height);

    return true;
}
b8 application_run(){
    app_state->is_running = true;
    clock_start(&app_state->clock);
    clock_update(&app_state->clock);
    app_state->last_time = app_state->clock.elapsed;

    f64 running_time = 0;
    u8 frame_count = 0;
    f64 target_frame_seconds = 1.0f / 60;


    PANCAKE_INFO(get_memory_usage_str());
    while(app_state->is_running){
        if(!platform_pump_messages()){
            app_state->is_running = false;
        }

        if(!app_state->is_suspended){

            //update clock,and get delta_time
            clock_update(&app_state->clock);
            f64 current_time =app_state->clock.elapsed;
            f64 delta = (current_time - app_state->last_time);
            f64 frame_start_time = platform_get_absolute_time();

            if(!app_state->game_inst->Update(app_state->game_inst,(f32)delta)){
                PANCAKE_FATAL("Game Update failed shutting down !");
                app_state->is_running = false;
                break;
            }

            //call the game's render routine
            if(!app_state->game_inst->Redner(app_state->game_inst,(f32)delta)){
                PANCAKE_FATAL("Game Redner failed shutting down !");
                app_state->is_running = false;
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
                b8 limit_frames = false;
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

            app_state->last_time = current_time;
        }
    }

    app_state->is_running = false;

    //Unregister events
    unregister_event(EVENT_CODE_APPLICATION_QUIT,0,application_on_event);
    unregister_event(EVENT_CODE_KEY_PRESSED,0,application_on_key);
    unregister_event(EVENT_CODE_KEY_RELEASED,0,application_on_key);

    shutdown_events_system(&app_state->event_system_state_ptr);
    shutdown_inputs_system(&app_state->input_system_state_ptr);
    shutdown_renderer_system(&app_state->renderer_system_state_ptr);
    platform_system_shutdown(&app_state->platform_system_state_ptr);
    shutdown_memory_system(&app_state->memory_system_state_ptr);
    shutdown_logging_system(&app_state->logging_system_state_ptr);

    return true;
}

void application_get_framebuffer_size(u32* width, u32* height){
    *width = app_state->width;
    *height = app_state->height;
}

b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context) {
    switch (code) {
        case EVENT_CODE_APPLICATION_QUIT: {
            PANCAKE_INFO("EVENT_CODE_APPLICATION_QUIT recieved, shutting down.");
            app_state->is_running = false;
            return true;
        }
    }

    return false;
}

b8 application_on_key(u16 code, void* sender, void* listener_inst, event_context context) {
    if (code == EVENT_CODE_KEY_PRESSED) {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_ESCAPE) {
            // NOTE: Technically firing an event to itself, but there may be other listeners.
            event_context data = {};
            fire_event(EVENT_CODE_APPLICATION_QUIT, 0, data);

            // Block anything else from processing this.
            return true;
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
    return false;
}
b8 application_on_resize(u16 code, void* sender, void* listner_inst, event_context context){
    if(code == EVENT_CODE_RESIZED){
        u16 width = context.data.u16[0];
        u16 height = context.data.u16[1];

        //check if defferent , if so fire an resize event
        if(width != app_state->width || height != app_state->height){
            app_state->width = width;
            app_state->height = height;

            PANCAKE_DEBUG("resize occured : width = %i , height = %i .", width, height);
        }

        //handle minimization
        if(width == 0 || height == 0){
            PANCAKE_INFO("Window minimized, suspend application.");
            app_state->is_suspended = true;
            return true;
        } else {
            if(app_state->is_suspended){
                PANCAKE_INFO("Window restored, resuming application.");
                app_state->is_suspended = false;
            }
            app_state->game_inst->OnResize(app_state->game_inst, width, height);
            renderer_on_resize(width, height);
        }
    }

    //event porpusely not handled to allow otherlistners to get this
    return false;
}