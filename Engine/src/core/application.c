#include "application.h"
#include "platform/platform.h"
#include "logger.h"
#include "game_types.h"
#include "core/pancake_memory.h"

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

b8 application_create(game* game_inst){
    if(initialized){
        PANCAKE_ERROR("application_create called more than once .");
        return FALSE;
    }

    app_state.game_inst = game_inst;

    //initialize subsystems
    initialize_logging();

    //TODO: REMOVE THESE
    PANCAKE_FATAL("A test message: %f", 3.14f);
    PANCAKE_ERROR("A test message: %f", 3.14f);
    PANCAKE_WARN("A test message: %f", 3.14f);
    PANCAKE_INFO("A test message: %f", 3.14f);
    PANCAKE_DEBUG("A test message: %f", 3.14f);
    PANCAKE_TRACE("A test message: %f", 3.14f);

    app_state.is_running = TRUE;
    app_state.is_suspended = FALSE;
    
    
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
        }
    }

    app_state.is_running = FALSE;

    platform_shutdown(&app_state.platform);

    return TRUE;
}