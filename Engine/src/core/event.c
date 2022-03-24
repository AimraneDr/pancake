#include "event.h"
#include "pancake_memory.h"
#include "containers/list.h"

typedef struct registered_event {
    void* listener;
    on_event_fnp callback;
} registered_event;

typedef struct event_code_entry{
    registered_event* events;
}event_code_entry;

//this should be more than enugh codes
#define MAX_MESSAGE_CODES 16384

//state structure
typedef struct event_system_state{
    //lookup table for event codes
    event_code_entry registered[MAX_MESSAGE_CODES];
}event_system_state;
/*
    Event system internal state
*/
static b8 is_initialized = false;
static event_system_state state;

b8 initialize_evnets(){
    if(is_initialized == true){
        return false;
    }
    is_initialized = false;
    pancake_zero_memory(&state,sizeof(state));
    is_initialized = true;
    return true;
}

void shutdown_event(){
    //free the events arrays, the objects pointed at should be destroyed on there own
    for(u64 i=0; i < MAX_MESSAGE_CODES; ++i){
        if(state.registered[i].events != 0){
            list_destroy(state.registered[i].events);
            state.registered[i].events = 0;
        }
    }
}

b8 register_event(u16 code, void* listener, on_event_fnp on_event){
    if(is_initialized == false){
         return false;
   }

    //create our events list if it is null
    if(state.registered[code].events == 0){
        state.registered[code].events = list_create(registered_event);
    }

    // Check for duplication
    u64 registered_count = list_length(state.registered[code].events);
    for(u64 i=0; i < registered_count; ++i){
        if(state.registered[code].events[i].listener == listener){
            //TODO: WARN
            return false;
        }
    }

    // at this point no duplicate was found
    //proceed with registration
    registered_event event;
    event.listener = listener;
    event.callback = on_event;
    list_push(state.registered[code].events, event);

    return true;
}

b8 unregister_event(u16 code, void* listener, on_event_fnp on_event){
    if(is_initialized == false){
         return false;
   }

    // On nothing is registered for the code, boot out.
    if(state.registered[code].events == 0){
        //TODO: WARN
        return false;
    }

    u64 registered_count = list_length(state.registered[code].events);
    
    for(u64 i=0; i < registered_count; ++i){
        registered_event e = state.registered[code].events[i];
        if(e.listener == listener && e.callback == on_event){
            //Found one, Remove it
            registered_event popped_event;
            list_pop_at(state.registered[code].events,i,&popped_event);
            
            return true;
        }
    }

    //Not found
    return false;
}

b8 fire_event(u16 code, void* sender, event_context context){
    if(is_initialized == false){
         return false;
   }

    // On nothing is registered for the code, boot out.
   if(state.registered[code].events == 0){
        return false;
    }

    u64 registered_count = list_length(state.registered[code].events);
    
    for(u64 i=0; i < registered_count; ++i){
        registered_event e = state.registered[code].events[i];
        if(e.callback(code,sender,e.listener,context)){
            //message had been handled , do not send to other listeners
            return true;
        }
    }
    //not found
    return false;
}
