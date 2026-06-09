/*
The MIT License (MIT)
Copyright © 2026 Matt Wells

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the “Software”), to deal in the
Software without restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "pch.h"

#include "Display_DX11.h"
#include "input.h"
#include "input_config.h"
#include "wc3w.h"
#include "memwrite.h"
#include "configTools.h"

LONG mouse_x_current = 0;
LONG mouse_y_current = 0;

LONG mouse_client_x = 0;
LONG mouse_client_y = 0;


//______________________________________________________________________________________
static LRESULT Update_Mouse_State(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {

    switch (Message) {

    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK: {
        Mouse.Update_Buttons(wParam);

        LONG x = GET_X_LPARAM(lParam);
        LONG y = GET_Y_LPARAM(lParam);

        mouse_client_x = x;
        mouse_client_y = y;

        *p_wc3_mouse_x_space = (WORD)(x * spaceWidth / clientWidth);
        *p_wc3_mouse_y_space = (WORD)(y * spaceHeight / clientHeight);
        if (surface_gui) {
            float fx = 0;
            float fy = 0;
            surface_gui->GetPosition(&fx, &fy);
            x = (LONG)((x - fx) * GUI_WIDTH / surface_gui->GetScaledWidth());
            y = (LONG)((y - fy) * GUI_HEIGHT / surface_gui->GetScaledHeight());
        }
        else {
            x = x * GUI_WIDTH / clientWidth;
            y = y * GUI_HEIGHT / clientHeight;
        }

        if (x < 0)
            x = 0;
        else if (x >= GUI_WIDTH)
            x = GUI_WIDTH - 1;
        if (y < 0)
            y = 0;
        else if (y >= GUI_HEIGHT)
            y = GUI_HEIGHT - 1;

        mouse_x_current = x;
        mouse_y_current = y;
        *p_wc3_mouse_x = (WORD)x;
        *p_wc3_mouse_y = (WORD)y;

        break;
    }
    case WM_MOUSEWHEEL:
        Mouse.Update_Wheel_Vertical(wParam);
        break;
    case WM_MOUSEHWHEEL:
        Mouse.Update_Wheel_Horizontal(wParam);
        break;
    default:
        break;
    }

    return 0;
}


//____________________________________________
static BOOL Set_Mouse_Position(LONG x, LONG y) {

    if (*p_wc3_is_mouse_present) {
        POINT client{ 0,0 };
        if (ClientToScreen(*p_wc3_hWinMain, &client)) {

            float fx = 0;
            float fy = 0;
            float fwidth = (float)clientWidth;
            float fheight = (float)clientHeight;
            if (surface_gui) {
                surface_gui->GetPosition(&fx, &fy);
                fwidth = surface_gui->GetScaledWidth();
                fheight = surface_gui->GetScaledHeight();
            }

            fx += x * fwidth / GUI_WIDTH;
            LONG ix = (LONG)fx;
            if ((float)ix != fx)
                ix++;
            *p_wc3_mouse_x_space = (WORD)(ix * spaceWidth / clientWidth);
            mouse_client_x = ix;
            ix += client.x;

            fy += y * fheight / GUI_HEIGHT;
            LONG iy = (LONG)fy;
            if ((float)iy != fy)
                iy++;
            *p_wc3_mouse_y_space = (WORD)(iy * spaceHeight / clientHeight);
            mouse_client_y = iy;
            iy += client.y;

            SetCursorPos(ix, iy);
        }

        if (x < 0)
            x = 0;
        else if (x >= GUI_WIDTH)
            x = GUI_WIDTH - 1;
        if (y < 0)
            y = 0;
        else if (y >= GUI_HEIGHT)
            y = GUI_HEIGHT - 1;

        mouse_x_current = x;
        mouse_y_current = y;
        *p_wc3_mouse_x = (WORD)x;
        *p_wc3_mouse_y = (WORD)y;
    }
    return *p_wc3_is_mouse_present;
}


// Replaces a function which was moving the cursor when it strayed beyond the client rect.
// This function allows the mouse to move freely as well as update it's position when outside the client rect.
//________________________________________________
static BOOL Update_Cursor_Position(LONG x, LONG y) {

    //if cursor position is modified by somthing other than the mouse 
    if (x != mouse_x_current || y != mouse_y_current)
        return Set_Mouse_Position(x, y);

    if (*p_wc3_is_mouse_present) {
        POINT p{ 0,0 };
        if (ClientToScreen(*p_wc3_hWinMain, &p)) {
            POINT m{ 0,0 };
            GetCursorPos(&m);

            x = (m.x - p.x);
            y = (m.y - p.y);

            mouse_client_x = x;
            mouse_client_y = y;

            *p_wc3_mouse_x_space = (WORD)(x * spaceWidth / clientWidth);
            *p_wc3_mouse_y_space = (WORD)(y * spaceHeight / clientHeight);

            if (surface_gui) {
                float fx = 0;
                float fy = 0;
                surface_gui->GetPosition(&fx, &fy);
                x = (LONG)((x - fx) * GUI_WIDTH / surface_gui->GetScaledWidth());
                y = (LONG)((y - fy) * GUI_HEIGHT / surface_gui->GetScaledHeight());
            }
            else {
                x = x * GUI_WIDTH / clientWidth;
                y = y * GUI_HEIGHT / clientHeight;
            }
        }

        if (x < 0)
            x = 0;
        else if (x >= GUI_WIDTH)
            x = GUI_WIDTH - 1;
        if (y < 0)
            y = 0;
        else if (y >= GUI_HEIGHT)
            y = GUI_HEIGHT - 1;

        mouse_x_current = x;
        mouse_y_current = y;
        *p_wc3_mouse_x = (WORD)x;
        *p_wc3_mouse_y = (WORD)y;
    }
    return *p_wc3_is_mouse_present;
}


//________________________________________________________
static void __declspec(naked) fix_movement_diamond_x(void) {

    __asm {
        mov esi, spaceWidth
        ret
    }
}


//________________________________________________________
static void __declspec(naked) fix_movement_diamond_y(void) {

    __asm {
        mov esi, spaceHeight
        ret
    }
}

/*
//____________________________________________________
static void __declspec(naked) update_space_mouse(void) {

    __asm {
        mov eax, p_mouse_button_space
        mov edx, p_wc3_mouse_button_space
        ret
    }
}
*/

//________________________________________________________
static LONG Fix_Space_Mouse_Movement(LONG* p_x, LONG* p_y) {
    // Maximum turn speed was being defined by the screen resolution formally 640x480.
    // Higher resolutions were allowing for a greater mouse range and thus a higher turning speed than what was otherwise defined in game.

    LONG centre_x = clientWidth / 2;
    LONG centre_y = clientHeight / 2;

    LONG x = mouse_client_x - centre_x;
    LONG y = mouse_client_y - centre_y;

    LONG range = centre_y;
    if (centre_y > centre_x)
        range = centre_x;

    range = range * Mouse.Axis_Limit_Percentage() / 100;


    int dead_zone = range * Mouse.Deadzone() / 320;
    float mouse_unit = (float)range / 256;

    //apply a small dead zone (320 / 32 = 10).
    if (x < dead_zone && x > -dead_zone)
        x = 0;
    if (y < dead_zone && y > -dead_zone)
        y = 0;

    if (x > range)
        x = range;
    else if (x < -range)
        x = -range;

    if (y > range)
        y = range;
    else if (y < -range)
        y = -range;

    //convert mouse movement value to the ships axis range between -256 and 256 (320 / 256 = 1.25).
    *p_x = (LONG)(x / mouse_unit);
    y = (LONG)(y / mouse_unit);

    if (Mouse.Is_Y_Axis_Inverted())
        *p_y = -y;
    else
        *p_y = y;

    return y;//right click and hold throttle value.
}


//__________________________________________________________
static void __declspec(naked) fix_space_mouse_movement(void) {

    __asm {
        push esi
        push ebp

        push edi //y
        push ecx //x

        //set pointers to x and y vals on the stack
        lea eax, dword ptr ss : [esp] //*p_x
        lea edi, dword ptr ss : [esp + 0x4]//*p_y
        push edi
        push eax
        call Fix_Space_Mouse_Movement
        add esp, 0x8

        pop ecx //x
        pop edx //y

        pop ebp
        pop esi
        ret
    }
}


//__________________________________
//Regulate how often mouse position is sampled for adjusting ship speed.
static BOOL Is_Mouse_Throttle_Time() {

    static LARGE_INTEGER last_throttle_time = { 0 };
    static LARGE_INTEGER update_time{ 0 };
    static bool run_once = false;

    if (!run_once)
        update_time.QuadPart = p_wc3_frequency->QuadPart / 16LL;

    LARGE_INTEGER time = { 0 };
    LARGE_INTEGER ElapsedMicroseconds = { 0 };

    QueryPerformanceCounter(&time);

    ElapsedMicroseconds.QuadPart = time.QuadPart - last_throttle_time.QuadPart;
    if (ElapsedMicroseconds.QuadPart < 0 || ElapsedMicroseconds.QuadPart > update_time.QuadPart) {
        last_throttle_time.QuadPart = time.QuadPart;
        return TRUE;
    }
    return FALSE;
}


//______________________________________________________
static void __declspec(naked) mouse_decrease_speed(void) {

    __asm {
        pushad
        call Is_Mouse_Throttle_Time
        cmp eax, 0
        popad
        je exit_func

        //divide mouse y position from centre by 32 for greater precision. this was formerly a division by 4.
        shr eax, 0x5
        sub dword ptr ds : [edx + 0x8] , eax

        exit_func :
        ret
    }
}


//______________________________________________________
static void __declspec(naked) mouse_increase_speed(void) {

    __asm {
        pushad
        call Is_Mouse_Throttle_Time
        cmp eax, 0
        popad
        je exit_func

        //divide mouse y position from centre by 32 for greater precision. this was formerly a division by 4.
        shr eax, 0x5
        add dword ptr ds : [edx + 0x8] , eax

        exit_func :
        ret
    }
}


//___________________________________________
static void Nav_Mouse_Control(BYTE* p_struct) {

    static bool being_pressed = 0;
    //rotate view if mouse button 2 pressed.
    if (*p_wc3_mouse_button_space & 0x2) {
        if (!being_pressed)//centre mouse on button click.
            Set_Mouse_Position(320, 240);
        being_pressed = true;

        LONG x = (INT16)*p_wc3_mouse_x_space - *p_wc3_mouse_centre_x;
        LONG y = (INT16)*p_wc3_mouse_y_space - *p_wc3_mouse_centre_y;

        x &= 0xFFFFFFFE;
        *(DWORD*)(p_struct + 0xFC) -= x;
        y &= 0xFFFFFFFE;
        *(DWORD*)(p_struct + 0xF8) += y;
    }
    else
        being_pressed = false;
}


//___________________________________________________
static void __declspec(naked) nav_mouse_control(void) {

    __asm {
        push ebx
        push ecx
        push esi
        push edi
        push ebp

        push ecx
        call Nav_Mouse_Control
        add esp, 0x4

        pop ebp
        pop edi
        pop esi
        pop ecx
        pop ebx

        //original code, check if joy button 2 pressed.
        mov eax, p_wc3_joy_buttons
        test byte ptr ds : [eax] , 0x2
        ret

    }
}


//_______________________________________________________________
static void __declspec(naked) centre_mouse_on_mission_start(void) {

    __asm {
        push ebx
        push esi
        push edi
        push ebp

        push 240
        push 320
        call Set_Mouse_Position
        add esp, 0x8

        pop ebp
        pop edi
        pop esi
        pop ebx

        //original code
        add esp, 0x134
        ret 0x8
    }
}

/*
//_______________________________________________
static void Space_Mouse_Is_Enabled(BYTE is_mouse) {

    *p_wc3_controller_mouse = is_mouse;
    if (*p_wc3_controller_mouse) {
        Debug_Info("SPACE MOUSE IS ENABLED!!!!!!!!!!!");
        Set_Mouse_Position(320, 240);
    }
}


//_______________________________________________________________
static void __declspec(naked) space_mouse_is_enabled(void) {
    //004383A7 | .  880D EC2D4A00              MOV BYTE PTR DS : [space_mouse_enabled ? ] , CL
    __asm {
        pushad
        push ecx
        call Space_Mouse_Is_Enabled
        add esp, 0x4
        popad

        ret
    }
}
*/

//_____________________________________________________
static void __declspec(naked) update_alt_o_cursor(void) {
    //Check if the position stored in the structure is valid and adjust if necessary.
    //Before updating the cursor position.
    __asm {
        cmp dword ptr ds : [esi + 0x14] , 0
        jge check_max_x
        mov dword ptr ds : [esi + 0x14] , 0
        jmp check_min_y

        check_max_x :
        cmp dword ptr ds : [esi + 0x14] , GUI_WIDTH
            jl check_min_y
            mov dword ptr ds : [esi + 0x14] , GUI_WIDTH - 1

            check_min_y :
            cmp dword ptr ds : [esi + 0x18] , 0
            jge check_max_y
            mov dword ptr ds : [esi + 0x18] , 0
            jmp check_done

            check_max_y :
        cmp dword ptr ds : [esi + 0x18] , GUI_HEIGHT
            jl check_done
            mov dword ptr ds : [esi + 0x18] , GUI_HEIGHT - 1

            check_done :
            push dword ptr ds : [esi + 0x18]
            push dword ptr ds : [esi + 0x14]
            call Update_Cursor_Position
            add esp, 0x8

            ret
    }
}


//________________________
void Modifications_Mouse() {

    //replaces original function for mouse window client position with scaled gui position.
    MemWrite8(0x483410, 0x8B, 0xE9);
    FuncWrite32(0x483411, 0x56082444, (DWORD)&Update_Mouse_State);

    //replaces original function for mouse window client position with scaled gui position.
    MemWrite8(0x483350, 0x83, 0xE9);
    FuncWrite32(0x483351, 0x54A108EC, (DWORD)&Set_Mouse_Position);
    MemWrite8(0x483355, 0x7E, 0x90);
    MemWrite16(0x483356, 0x004A, 0x9090);

    //prevent setting mouse centre x and y values to 320x240.
    MemWrite16(0x408479, 0x05C7, 0x9090);
    MemWrite32(0x40847B, 0x4A9B78, 0x90909090);
    MemWrite32(0x40847F, 0x0140, 0x90909090);
    MemWrite16(0x408483, 0x05C7, 0x9090);
    MemWrite32(0x408485, 0x4A9B98, 0x90909090);
    MemWrite32(0x408489, 0xF0, 0x90909090);

    //replace set cursor function to allow cursor to freely leave window bounds when in gui mode
    FuncReplace32(0x458F1B, 0x017A11, (DWORD)&Update_Cursor_Position);

    //in space - this moves the cursor if it strays out of client area, using ClipCursor instead.
    MemWrite8(0x4222E9, 0xE8, 0x90);
    MemWrite32(0x4222EA, 0x061062, 0x90909090);

    //ALT-O space settings screen.
    //Jump over the code that keeps the cursor on screen.
    MemWrite16(0x437A86, 0x7E83, 0x2EEB);
    MemWrite16(0x437A88, 0x0014, 0x9090);
    //update the cursor position on the  ALT-O screen.
    FuncReplace32(0x437AD1, 0x04B87B, (DWORD)&update_alt_o_cursor);
    //set pointer to use general mouse pointer data instead of the space flight copy which is set at a different resolution.
    MemWrite32(0x4378BB, 0x4A9B92, (DWORD)0x4A7E5A);//p_wc3_mouse_x
    MemWrite32(0x4378C6, 0x4A9B94, (DWORD)0x4A7E5C);//p_wc3_mouse_y
    MemWrite32(0x4378CF, 0x4A9B90, (DWORD)0x4A7E58); //p_wc3_mouse_button

    //relate the movement diamond to new space view dimensions rather than original 640x480.
    MemWrite8(0x422277, 0xBE, 0xE8);
    FuncWrite32(0x422278, 0x280, (DWORD)&fix_movement_diamond_x);
    MemWrite8(0x42228A, 0xBE, 0xE8);
    FuncWrite32(0x42228B, 0x1E0, (DWORD)&fix_movement_diamond_y);

    // skip copy "general mouse state" to "space mouse state".
    // "general mouse state" & "space mouse state" are now set in Update_Mouse_State , Set_Mouse_Position, Update_Cursor_Position and Mouse.Update_Buttons.
    MemWrite8(0x408426, 0x74, 0xEB);

    //update mouse in space view
    //MemWrite8(0x4083DA, 0xBA, 0xE8);
    //FuncWrite32(0x4083DB, 0x4A9B90, (DWORD)&update_space_mouse);

    // skip copy "general mouse state" to "space mouse state".
    // "general mouse state" & "space mouse state" are now set in Update_Mouse_State , Set_Mouse_Position, Update_Cursor_Position and Mouse.Update_Buttons.
    MemWrite8(0x4222C6, 0xBA, 0xEB);
    MemWrite32(0x4222C7, 0x4A9B90, 0x9090900F);//JMP SHORT 004222D7


    // Mouse turn speed fix--------------------------
    //"MOV EAX, ECX" to "JMP SHORT 00429E79"
    MemWrite16(0x429E5D, 0xC18B, 0x1AEB);
    //"MOV EAX, EDI" to "JMP SHORT 00429EA0"
    MemWrite16(0x429E88, 0xC78B, 0x16EB);

    MemWrite8(0x429EA0, 0x99, 0xE8);
    FuncWrite32(0x429EA1, 0xD08BFBF7, (DWORD)&fix_space_mouse_movement);
    //-----------------------------------------------

    //----better-throttle-regulation-when-using-the-mouse------------------
    MemWrite16(0x42A07B, 0xE8C1, 0xE890);
    FuncWrite32(0x42A07D, 0x08422902, (DWORD)&mouse_decrease_speed);

    MemWrite16(0x42A099, 0xE8C1, 0xE890);
    FuncWrite32(0x42A09B, 0x08420102, (DWORD)&mouse_increase_speed);
    //---------------------------------------------------------------------

    //add mouse view axis rotation to NAV screen.
    MemWrite16(0x430463, 0x05F6, 0xE890);
    FuncWrite32(0x430465, 0x4B2334, (DWORD)&nav_mouse_control);
    MemWrite8(0x430469, 0x02, 0x90);

    //centre mouse at the start of missions
    MemWrite16(0x452861, 0xC481, 0xE990);
    FuncWrite32(0x452863, 0x134, (DWORD)&centre_mouse_on_mission_start);
}
