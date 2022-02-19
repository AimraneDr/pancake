#include "defines.h"
#include "platform/platform.h"

//Windows Platform Layer
#if PANCAKE_PLATFORM_WINDOWS

#include "core/logger.h"
#include "core/inputs.h"
#include <Windows.h>
#include <windowsx.h> // parameters input extraction
#include <stdlib.h>

typedef struct internal_state{
    HINSTANCE h_instance;
    HWND hwnd;
}internal_state;

//clock
static f64 clock_frequency;
static LARGE_INTEGER start_time;

LRESULT CALLBACK win32_process_message(HWND hwnd,u32 msg,WPARAM w_param,LPARAM l_param);

b8 platform_startup(platform_state *plat_state, char* application_name, i32 x, i32 y, i32 width, i32 height){
    
    plat_state->internal_state = malloc(sizeof(internal_state));
    internal_state *state = (internal_state *)plat_state->internal_state;

    state->h_instance=GetModuleHandleA(0);

    //setup window class
    HICON icon = LoadIcon(state->h_instance,IDI_APPLICATION);
    WNDCLASSA wc;
    memset(&wc,0,sizeof(wc));
    wc.style = CS_DBLCLKS; // double clicks
    wc.lpfnWndProc = win32_process_message;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = state->h_instance;
    wc.hIcon = icon;
    wc.hCursor = LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground = NULL;    //Transparent
    wc.lpszClassName = "Pancake_window_class";
    //Register window class
    if(!RegisterClassA(&wc)){
        MessageBoxA(0,"Window Register Failed","Error",MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }

    //create window
    u32 client_x =x;
    u32 client_y =y;
    u32 client_width =width;
    u32 client_height=height;

    u32 window_x =client_x;
    u32 window_y =client_y;
    u32 window_width =client_width;
    u32 window_height=client_height;

    u32 window_style = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;
    u32 window_ex_style = WS_EX_APPWINDOW;

    window_style |= WS_MAXIMIZEBOX;
    window_style |= WS_MINIMIZEBOX;
    window_style |= WS_THICKFRAME;

    //Optain the size of the border
    RECT border_rect = {0,0,0,0};
    AdjustWindowRectEx(&border_rect,window_style,0,window_ex_style);

    window_x += border_rect.left;
    window_y += border_rect.top;

    window_width += border_rect.right - border_rect.left;
    window_height += border_rect.bottom - border_rect.top;

    HWND handle = CreateWindowExA(
        window_ex_style,"Pancake_window_class",application_name,
        window_style,window_x,window_y,window_width,window_height,
        0,0,state->h_instance,0
    );

    if(handle == 0){
        MessageBoxA(0,"Window Register Failed","Error",MB_ICONEXCLAMATION | MB_OK);
        PANCAKE_FATAL("Window Register Failed");
        return FALSE;
    }else{
        state->hwnd = handle;
    }

    //show the window
    b32 should_activate = 1;    // TODO: if the window should not accept inputs this should be false
    i32 show_window_command_flags = should_activate ? SW_SHOW : SW_SHOWNOACTIVATE;
    // if initially minimized use SW_MINIMIZE : SW_SHOWMINNOACTIVATE
    // if initially maximized use SW_SHOWMAXIMIZED : SW_MAXIMIZE  
    ShowWindow(state->hwnd,show_window_command_flags);

    //set clock
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    clock_frequency = 1.0 / (f64)frequency.QuadPart;
    QueryPerformanceCounter(&start_time);

    return TRUE;
}

void platform_shutdown(platform_state *plat_state){
    internal_state *state = (internal_state *)plat_state->internal_state;

    if(state->hwnd){
        DestroyWindow(state->hwnd);
        state->hwnd = 0;
    }
}

b8 platform_pump_messages(platform_state* state){
    MSG message;
    while(PeekMessageA(&message,NULL,0,0,PM_REMOVE)){
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
    return TRUE;
}

void *platform_allocate(u64 size,b8 aligned){
    return malloc(size);
}

void platform_free(void *block,b8 aligned){
    free(block);
}

void *platform_zero_memory(void *block, u64 size){
    return memset(block,0,size);
}

void *platform_copy_memory(void *dest,const void *source,u64 size){
    return memcpy(dest,source,size);
}

void *platform_set_memory(void *dest,i32 value,u64 size){
    return memset(dest,value,size);
}


void platform_console_write(const char* msg, u8 colour){
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    //FATAL, ERROR, WARN, INFO, DEBUG, TRACE
    static u8 level[6] = {64, 4, 6, 2, 1, 8};
    SetConsoleTextAttribute(console_handle,level[colour]);

    OutputDebugStringA(msg);
    u64 length = strlen(msg);
    LPDWORD number_writen = 0;
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE),msg,(DWORD)length,number_writen,0);
}

void platform_console_write_error(const char* msg, u8 colour){
    HANDLE console_handle = GetStdHandle(STD_ERROR_HANDLE);
    //FATAL, ERROR, WARN, INFO, DEBUG, TRACE
    static u8 level[6] = {64, 4, 6, 2, 1, 8};
    SetConsoleTextAttribute(console_handle,level[colour]);

    OutputDebugStringA(msg);
    u64 length = strlen(msg);
    LPDWORD number_writen = 0;
    WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE),msg,(DWORD)length,number_writen,0);
}

f64 platform_get_absolute_time(){
    LARGE_INTEGER now_time;
    QueryPerformanceCounter(&now_time);
    return (f64) now_time.QuadPart * clock_frequency;
}

void platform_sleep(u64 ms){
    Sleep(ms);
}

LRESULT CALLBACK win32_process_message(HWND hwnd,u32 msg,WPARAM w_param,LPARAM l_param){
    switch(msg){
        case WM_ERASEBKGND:
            //Notify the OS that erasing will be handled by the application to prevent flicker
            return 1;
        case WM_CLOSE:
            //TODO: fire an event for the application to quit .
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:{
            //get the updated size
            // RECT r;
            // GetClientRect(hwnd,&r);
            // u32 width = r.right-r.left;
            // u32 height = r.bottom-r.top;

            //TODO: fire an event for window resize .
        }break;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:{
            //key pressed or releaseed
            b8 pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
            keys key = (u16)w_param;
            //pass to the inputs subsystem for processing.
            input_process_key(key, pressed);
        }break;
        case WM_MOUSEMOVE:{
            //Mouse position
            i32 x_position = GET_X_LPARAM(l_param);
            i32 y_position = GET_Y_LPARAM(l_param);
            //pass to the inputs subsystem for processing.
            input_process_mouse_move(x_position, y_position);
        }break;
        case WM_MOUSEWHEEL:{
            i32 z_delta = GET_WHEEL_DELTA_WPARAM(w_param);
            if(z_delta != 0){
                //flatten the input to an OS independent (-1, 1)
                z_delta =(z_delta < 0) ? -1 : 1; 
                input_process_mouse_wheel(z_delta);
            }
        }break;
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:{
            b8 pressed = (msg == WM_LBUTTONDOWN);
            m_buttons mouse_btn = BUTTON_LEFT;
            //pass to the inputs subsystem for processing.
            input_process_mouse_button(mouse_btn, pressed);
        }break;
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:{
            b8 pressed = (msg == WM_MBUTTONDOWN);
            m_buttons mouse_btn = BUTTON_MIDDLE; 
            //pass to the inputs subsystem for processing.
            input_process_mouse_button(mouse_btn, pressed);
        }break;
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:{
            b8 pressed = (msg == WM_RBUTTONDOWN);
            m_buttons mouse_btn = BUTTON_RIGHT; 
            //pass to the inputs subsystem for processing.
            input_process_mouse_button(mouse_btn, pressed);
        }break;
    }
    return DefWindowProcA(hwnd,msg,w_param,l_param);
}

#endif // PANCAKE_PLATFORM_WINDOWS