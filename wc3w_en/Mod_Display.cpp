/*
The MIT License (MIT)
Copyright © 2024-2026 Matt Wells

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
#include "modifications.h"
#include "memwrite.h"
#include "configTools.h"
#include "libvlc_Movies.h"
#include "libvlc_Music.h"
#include "wc3w.h"
#include "input_config.h"

#define WIN_MODE_STYLE  WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX

BOOL space_view_has_BG_image = FALSE;

SCALE_TYPE cockpit_scale_type = SCALE_TYPE::fit;
BOOL crop_cockpit_rect = TRUE;
BOOL is_nav_view = FALSE;

BOOL clip_cursor = FALSE;
static bool is_cursor_clipped = false;

UINT clientWidth = 0;
UINT clientHeight = 0;

UINT spaceWidth = 0;
UINT spaceHeight = 0;

BOOL space_use_original_aspect = FALSE;

BOOL is_space_scaled = FALSE;
UINT space_scaled_width = 640;
UINT space_scaled_height = 480;

LARGE_INTEGER nav_update_time{ 0 };

BYTE alternate_space_colour_pal_off = 0x11;


//_______________________________________________________
static void Change_Profile_Type(PROFILE_TYPE new_profile) {

    static PROFILE_TYPE last_profile_type = current_pro_type;

    current_pro_type = new_profile;

    //clear keyboard on profile change incase button is down during transition.
    if (last_profile_type != current_pro_type) {
        Clear_Key_States();
        last_profile_type = current_pro_type;
        if (last_profile_type == PROFILE_TYPE::Space)
            Debug_Info("Change_Profile_Type SPACE");
        else if (last_profile_type == PROFILE_TYPE::GUI)
            Debug_Info("Change_Profile_Type GUI");
        else if (last_profile_type == PROFILE_TYPE::NAV)
            Debug_Info("Change_Profile_Type NAV");
    }
}


//___________________________
static BOOL IsMouseInClient() {
    //check if mouse within client rect.
    RECT rcClient;
    POINT p{ 0,0 }, m{ 0,0 };

    GetCursorPos(&m);

    ClientToScreen(*p_wc3_hWinMain, &p);
    GetClientRect(*p_wc3_hWinMain, &rcClient);

    rcClient.left += p.x;
    rcClient.top += p.y;
    rcClient.right += p.x;
    rcClient.bottom += p.y;


    if (m.x < rcClient.left || m.x > rcClient.right)
        return FALSE;
    if (m.y < rcClient.top || m.y > rcClient.bottom)
        return FALSE;
    return TRUE;
}


//___________________________
static BOOL ClipMouseCursor() {

    POINT p{ 0,0 };
    if (!ClientToScreen(*p_wc3_hWinMain, &p))
        return FALSE;
    RECT rcClient;
    if (!GetClientRect(*p_wc3_hWinMain, &rcClient))
        return FALSE;
    rcClient.left += p.x;
    rcClient.top += p.y;
    rcClient.right += p.x;
    rcClient.bottom += p.y;

    return ClipCursor(&rcClient);
}


//________________________________________________________________________________
static void SetWindowTitle(HWND hwnd, const wchar_t* msg, UINT width, UINT height) {

    wchar_t winText[64];
    swprintf_s(winText, 64, L"%S  @%ix%i   %s", p_wc3_szAppName, width, height, msg);
    SendMessage(hwnd, WM_SETTEXT, (WPARAM)0, (LPARAM)winText);
}


//_______________________________________________________
static void SetWindowTitle(HWND hwnd, const wchar_t* msg) {

    SetWindowTitle(hwnd, msg, clientWidth, clientHeight);
}


//____________________________________________________________________
static bool Check_Window_GUI_Scaling_Limits(HWND hwnd, RECT* p_rc_win, bool set_window_title) {
    if (!p_rc_win)
        return false;
    bool resized = false;
    DWORD dwStyle = 0;
    DWORD dwExStyle = 0;
    dwStyle = GetWindowLong(hwnd, GWL_STYLE);
    dwExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

    //get the dimensions of the window frame style.
    RECT rc_style{ 0,0,0,0 };
    AdjustWindowRectEx(&rc_style, dwStyle, false, dwExStyle);
    RECT rc_client;
    CopyRect(&rc_client, p_rc_win);
    //subtract the window style rectangle leaving the client rectangle.
    rc_client.left -= rc_style.left;
    rc_client.top -= rc_style.top;
    rc_client.right -= rc_style.right;
    rc_client.bottom -= rc_style.bottom;


    LONG client_width = rc_client.right - rc_client.left;
    LONG client_height = rc_client.bottom - rc_client.top;

    //prevent window dimensions going beyond what is supported by your graphics card.
    if (client_width > (LONG)max_texDim || client_height > (LONG)max_texDim) {
        if (client_width > (LONG)max_texDim)
            client_width = (LONG)max_texDim;
        if (client_height > (LONG)max_texDim)
            client_height = (LONG)max_texDim;
        rc_client.right = rc_client.left + client_width;
        rc_client.bottom = rc_client.top + client_height;
        //add the client and style rects to get the window rect.
        p_rc_win->left = rc_client.left + rc_style.left;
        p_rc_win->top = rc_client.top + rc_style.top;
        p_rc_win->right = rc_client.right + rc_style.right;
        p_rc_win->bottom = rc_client.bottom + rc_style.bottom;
        resized = true;
    }



    //prevent window dimensions going under the minumum values of 640x480.
    if (client_width < GUI_WIDTH || client_height < GUI_HEIGHT) {
        if (client_width < GUI_WIDTH)
            client_width = GUI_WIDTH;
        if (client_height < GUI_HEIGHT)
            client_height = GUI_HEIGHT;

        rc_client.right = rc_client.left + client_width;
        rc_client.bottom = rc_client.top + client_height;
        //add the client and style rects to get the window rect.
        p_rc_win->left = rc_client.left + rc_style.left;
        p_rc_win->top = rc_client.top + rc_style.top;
        p_rc_win->right = rc_client.right + rc_style.right;
        p_rc_win->bottom = rc_client.bottom + rc_style.bottom;
        resized = true;

    }
    if (set_window_title)
        SetWindowTitle(hwnd, L"", client_width, client_height);
    //Debug_Info("Check_Window_GUI_Scaling_Limits w:%d, h:%d", client_width, client_height);
    return resized;
}


//________________________
static bool Display_Exit() {
    if (pMovie_vlc)
        delete pMovie_vlc;
    pMovie_vlc = nullptr;

    if (pMovie_vlc_Inflight)
        delete pMovie_vlc_Inflight;
    pMovie_vlc_Inflight = nullptr;

    Display_Dx_Destroy();
    return 0;
}


//_________________________________
static BOOL Window_Setup(HWND hwnd) {
    
    Check_Command_Line_Overrides();

    if (*p_wc3_subtitles_enabled == 0) {
        if (ConfigReadInt_InGame(L"MAIN", L"ENABLE_SUBTITLES", CONFIG_MAIN_ENABLE_SUBTITLES))
            *p_wc3_subtitles_enabled = 1;
    }
    if (*p_wc3_language_ref == 0) {
        int lang = ConfigReadInt_InGame(L"MAIN", L"LANGUAGE_REF", CONFIG_MAIN_LANGUAGE_REF);
        if (lang >= 0 && lang < 3)
            *p_wc3_language_ref = lang;
    }

    if (!*p_wc3_is_windowed) {
        if (ConfigReadInt_InGame(L"MAIN", L"WINDOWED", CONFIG_MAIN_WINDOWED))
            *p_wc3_is_windowed = true;
    }
    
    if (!*p_wc3_movie_no_interlace) {
        if (!ConfigReadInt(L"MOVIES", L"SHOW_ORIGINAL_MOVIES_INTERLACED", CONFIG_MOVIES_SHOW_ORIGINAL_INTERLACED))
            *p_wc3_movie_no_interlace = true;
    }

    *p_wc3_gamma_val = ConfigReadInt_InGame(L"MAIN", L"GAMMA_LEVEL", CONFIG_MAIN_GAMMA_LEVEL);

    if (ConfigReadInt(L"SPACE", L"USE_ORIGINAL_ASPECT_RATIO", CONFIG_SPACE_USE_ORIGINAL_ASPECT_RATIO))
        space_use_original_aspect = TRUE;
    
    int COCKPIT_MAINTAIN_ASPECT_RATIO = ConfigReadInt(L"SPACE", L"COCKPIT_MAINTAIN_ASPECT_RATIO", CONFIG_SPACE_COCKPIT_MAINTAIN_ASPECT_RATIO);
    if (COCKPIT_MAINTAIN_ASPECT_RATIO == 0)
        cockpit_scale_type = SCALE_TYPE::fill;
    else if (COCKPIT_MAINTAIN_ASPECT_RATIO < 0)
        crop_cockpit_rect = FALSE;

    if (ConfigReadInt(L"SPACE", L"IS_SPACE_SCALED", CONFIG_SPACE_IS_SPACE_SCALED)) {
        is_space_scaled = TRUE;
        space_scaled_height = ConfigReadInt(L"SPACE", L"SCALED_SPACE_HEIGHT", CONFIG_SPACE_SCALED_SPACE_HEIGHT);
        if (space_scaled_height < 3)
            space_scaled_height = 3;
        space_scaled_width = (UINT)(space_scaled_height * (4.0f / 3.0f));
    }

    if (*p_wc3_is_windowed) {
        Debug_Info("Window Setup: Windowed");
        WINDOWPLACEMENT winPlace{ 0 };
        winPlace.length = sizeof(WINDOWPLACEMENT);
        
        SetWindowLongPtr(hwnd, GWL_STYLE, WIN_MODE_STYLE);
        //Debug_Info("is_windowed set style");

        if (ConfigReadWinData(L"MAIN", L"WIN_DATA", &winPlace)) {
            if (winPlace.showCmd != SW_MAXIMIZE)
                winPlace.showCmd = SW_SHOWNORMAL;
        }
        else {
            GetWindowPlacement(hwnd, &winPlace);
            winPlace.showCmd = SW_SHOWNORMAL;
            Debug_Info("is_windowed GetWindowPlacement");
        }
        if (winPlace.showCmd == SW_SHOWNORMAL) //if the window isn't maximized
            Check_Window_GUI_Scaling_Limits(hwnd, &winPlace.rcNormalPosition, false);
        
        SetWindowPlacement(hwnd, &winPlace);
    }
    else {
        Debug_Info("Window Setup: Fullscreen");
        SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP);
        SetWindowPos(hwnd, 0, 0, 0, 0, 0, 0);
        ShowWindow(hwnd, SW_MAXIMIZE);
    }


    RECT clientRect;
    GetClientRect(hwnd, &clientRect);

    //Get the window client width and height.
    clientWidth = clientRect.right - clientRect.left;
    clientHeight = clientRect.bottom - clientRect.top;



    Display_Dx_Setup(hwnd, clientWidth, clientHeight);
    
    *p_wc3_mouse_centre_x = (LONG)spaceWidth / 2;
    *p_wc3_mouse_centre_y = (LONG)spaceHeight / 2;

    Set_Gamma_Offset(*p_wc3_gamma_val);

    //QueryPerformanceFrequency(&Frequency);

    //Set the movement update time for Navigation screen, which was unregulated and way to fast on modern computers.
    DXGI_RATIONAL refreshRate{};
    refreshRate.Denominator = 1;
    refreshRate.Numerator = ConfigReadInt(L"SPACE", L"NAV_SCREEN_KEY_RESPONSE_HZ", CONFIG_SPACE_NAV_SCREEN_KEY_RESPONSE_HZ);
    nav_update_time.QuadPart = p_wc3_frequency->QuadPart / refreshRate.Numerator;

    DXGI_RATIONAL refreshRate_Mon{};
    Get_Monitor_Refresh_Rate(hwnd, &refreshRate_Mon);
    //Set the max refresh rate in space, original 24 FPS. Set to zero to use screen refresh rate, a negative value will use the original value.  
    refreshRate.Numerator = ConfigReadInt(L"SPACE", L"SPACE_REFRESH_RATE_HZ", CONFIG_SPACE_SPACE_REFRESH_RATE_HZ);
    if ((int)refreshRate.Numerator < 0)
        refreshRate.Numerator = 24;
    else if (refreshRate.Numerator == 0)
        refreshRate.Numerator = refreshRate_Mon.Numerator, refreshRate.Denominator = refreshRate_Mon.Denominator;
    
    //ensure that the max refresh rate is not greater than the monitors refresh rate.
    if ((float)refreshRate.Numerator / refreshRate.Denominator > (float)refreshRate_Mon.Numerator / refreshRate_Mon.Denominator)
        refreshRate.Numerator = refreshRate_Mon.Numerator, refreshRate.Denominator = refreshRate_Mon.Denominator;

    Debug_Info("space refresh rate max: n:%d / d:%d, HZ:%f", refreshRate.Numerator, refreshRate.Denominator, (float)refreshRate.Numerator / refreshRate.Denominator);
    p_wc3_space_time_max->QuadPart = p_wc3_frequency->QuadPart * refreshRate.Denominator / refreshRate.Numerator;

    //Debug_Info("frequency: %d, %d, space time max: %d, %d, space time min: %d, %d", p_wc3_frequency->LowPart, p_wc3_frequency->HighPart, p_wc3_space_time_max->LowPart, p_wc3_space_time_max->HighPart, p_wc3_space_time_min->LowPart, p_wc3_space_time_min->HighPart);
    Debug_Info("Window Setup: Done");
    return 1;
}


//__________________________
static void Window_Resized() {

    RECT clientRect;
    GetClientRect(*p_wc3_hWinMain, &clientRect);

    //Get the window client width and height.
    clientWidth = clientRect.right - clientRect.left;
    clientHeight = clientRect.bottom - clientRect.top;


    Display_Dx_Resize(clientWidth, clientHeight);

    *p_wc3_mouse_centre_x = (LONG)spaceWidth / 2;
    *p_wc3_mouse_centre_y = (LONG)spaceHeight / 2;

    if (is_cursor_clipped) {
        //Debug_Info("Window_Resized - is_cursor_clipped");
        if (ClipMouseCursor()) {
            //Debug_Info("Window_Resized - Mouse Cursor Clipped");
        }
    }
    if (*p_wc3_is_windowed) {
        WINDOWPLACEMENT winPlace{ 0 };
        winPlace.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(*p_wc3_hWinMain, &winPlace);
        ConfigWriteWinData(L"MAIN", L"WIN_DATA", &winPlace);
    }
}


//________________________
static BYTE* LockSurface() {

    if (surface_gui == nullptr)
        return nullptr;

    VOID* pSurface = nullptr;

    if (surface_gui->Lock(&pSurface, p_wc3_main_surface_pitch) != S_OK)
        return nullptr;

    return (BYTE*)pSurface;
}


//_____________________________
static void UnlockShowSurface() {

    if (surface_gui == nullptr)
        return;
    surface_gui->Unlock();
    Display_Dx_Present(PRESENT_TYPE::gui);
}


//_____________________________
static void Clear_GUI_Surface() {
    if (surface_gui)
        surface_gui->Clear_Texture(0);
}


//_________________________________________________________
static void DXBlt(BYTE* fBuff, DWORD subY, DWORD subHeight) {

    if (surface_gui == nullptr)
        return;

    LONG fWidth = GUI_WIDTH;

    if (fBuff == NULL || fBuff == (BYTE*)*pp_wc3_DIB_vBits) {
        fBuff = (BYTE*)*pp_wc3_DIB_vBits;
        fWidth = (*pp_wc3_DIB_Bitmapinfo)->bmiHeader.biWidth;
        //Debug_Info("DXBlt - db w=%d, h =%d", fWidth, -(*pp_wc3_DIB_Bitmapinfo)->bmiHeader.biHeight);
    }
    //else
        //Debug_Info("DXBlt - buffer provided");

    BYTE* pSurface = nullptr;

    if (surface_gui->Lock((VOID**)&pSurface, p_wc3_main_surface_pitch) != S_OK)
        return;

    fBuff += subY * fWidth;
    pSurface += subY * *p_wc3_main_surface_pitch;
    for (UINT y = 0; y < subHeight; y++) {
        for (LONG x = 0; x < fWidth; x++)
            pSurface[x] = fBuff[x];

        pSurface += *p_wc3_main_surface_pitch;
        fBuff += fWidth;
    }

    surface_gui->Unlock();
    Display_Dx_Present(PRESENT_TYPE::gui);

}


// Adjust width, height and centre target point of FIRST person POV space views.
// __fastcall used here as class "this" is parsed on the ecx register.
//_____________________________________________________________
static void __fastcall Set_Space_View_POV1(void* p_space_class) {

    Set_Space2D_Surface_SamplerState_From_Config();

    WORD* p_view_vars = (WORD*)p_space_class;
    DWORD* p_cockpit_class = ((DWORD**)p_space_class)[67]; //[p_space_struct + 0x10C]
    DWORD cockpit_view_type = p_cockpit_class[8];//[p_cockpit_struct + 0x20] //view_type: cockpit = 0, left = 1, rear = 2, right = 3, hud = 4.

    space_view_has_BG_image = FALSE;
    SCALE_TYPE scale_type = SCALE_TYPE::fit;


    WORD width = (WORD)spaceWidth;
    WORD height = (WORD)spaceHeight;

    p_view_vars[4] = width;
    p_view_vars[5] = height;

    switch (cockpit_view_type) {
    case 0:
        p_view_vars[6] = (WORD)(*p_wc3_x_centre_cockpit / (float)GUI_WIDTH * width);
        p_view_vars[7] = (WORD)(*p_wc3_y_centre_cockpit / (float)GUI_HEIGHT * height);
        space_view_has_BG_image = TRUE;
        scale_type = cockpit_scale_type;
        Debug_Info_Flight("Set Space View - CockPit");
        Debug_Info_Flight("centre_x=%d, centre_y=%d, new_centre_x=%d, new_centre_y=%d", *p_wc3_x_centre_cockpit, *p_wc3_y_centre_cockpit, p_view_vars[6], p_view_vars[7]);
        break;
    case 1:
        if(*p_wc3_view_cockpit_or_hud == SPACE_VIEW_TYPE::Cockpit && Get_Cockpit_HD_BG_Surface(static_cast<WORD>(SPACE_VIEW_TYPE::CockLeft)))
            space_view_has_BG_image = TRUE;
        Debug_Info_Flight("Set Space View - Left");
        break;
    case 2:
        p_view_vars[6] = (WORD)(*p_wc3_x_centre_rear / (float)GUI_WIDTH * width);
        p_view_vars[7] = (WORD)(*p_wc3_y_centre_rear / (float)GUI_HEIGHT * height);
        Debug_Info_Flight("Set Space View - Rear");
        Debug_Info_Flight("centre_x=%d, centre_y=%d, new_centre_x=%d, new_centre_y=%d", *p_wc3_x_centre_rear, *p_wc3_y_centre_rear, p_view_vars[6], p_view_vars[7]);
        if (p_cockpit_class[1] || *p_wc3_view_cockpit_or_hud == SPACE_VIEW_TYPE::Cockpit && Get_Cockpit_HD_BG_Surface(static_cast<WORD>(SPACE_VIEW_TYPE::CockBack))) {//the pointer stored here seems to be related to a background image, enable cockpit view if present to fill black area beyond background.
            space_view_has_BG_image = TRUE;
            scale_type = cockpit_scale_type;
        }
        break;
    case 3:
        if (*p_wc3_view_cockpit_or_hud == SPACE_VIEW_TYPE::Cockpit && Get_Cockpit_HD_BG_Surface(static_cast<WORD>(SPACE_VIEW_TYPE::CockRight)))
            space_view_has_BG_image = TRUE;
        Debug_Info_Flight("Set Space View - Right");
        break;
    case 4:
        p_view_vars[6] = (WORD)(*p_wc3_x_centre_hud / (float)GUI_WIDTH * width);
        p_view_vars[7] = (WORD)(*p_wc3_y_centre_hud / (float)GUI_HEIGHT * height);
        Debug_Info_Flight("Set Space View - HUD");
        Debug_Info_Flight("centre_x=%d, centre_y=%d, new_centre_x=%d, new_centre_y=%d", *p_wc3_x_centre_hud, *p_wc3_y_centre_hud, p_view_vars[6], p_view_vars[7]);
        break;
    default:
        Debug_Info_Flight("Set Space View - Unknown? num:%d", cockpit_view_type);
        break;
    }
    //Debug_Info("Set_Space_View_POV1 - %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d", p_cockpit_class[0], p_cockpit_class[1], p_cockpit_class[2], p_cockpit_class[3], p_cockpit_class[4], p_cockpit_class[5], p_cockpit_class[6], p_cockpit_class[7],
    //    p_cockpit_class[8], p_cockpit_class[9], p_cockpit_class[10], p_cockpit_class[11], p_cockpit_class[12], p_cockpit_class[13], p_cockpit_class[14], p_cockpit_class[15]);
    //if (p_cockpit_class[1]) {
    //    DWORD* p_cockpit_class2 = ((DWORD**)p_cockpit_class)[1];
    //     Debug_Info("Set_Space_View_POV1 p_cockpit_class2- %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d", p_cockpit_class2[0], p_cockpit_class2[1], p_cockpit_class2[2], p_cockpit_class2[3], p_cockpit_class2[4], p_cockpit_class2[5], p_cockpit_class2[6], p_cockpit_class2[7],
    //        p_cockpit_class2[8], p_cockpit_class2[9], p_cockpit_class2[10], p_cockpit_class2[11], p_cockpit_class2[12], p_cockpit_class2[13], p_cockpit_class2[14], p_cockpit_class2[15]);
    //}
    
    surface_space2D->ScaleTo((float)clientWidth, (float)clientHeight, scale_type);
}


//_____________________________________________________
static void __declspec(naked) set_space_view_pov1(void) {
    //structure ptr ecx, holds various details regarding space flight
    //word @ [ecx + 0x8] = view width
    //word @ [ecx + 0xA] = view height
    //word @ [ecx + 0xC] = view x centre
    //word @ [ecx + 0xE] = view y centre
    //ptr & [ecx + 0x10C] ptr to structure containing cockpit_details. view_type is at [cockpit_details + 0x20]. view_type: cockpit = 0, left = 1, rear = 2, right = 3, hud = 4.
    __asm {
        push ebx
        push ebp
        push esi
        push edi


        //push ecx
        call Set_Space_View_POV1
        //add esp, 0x4

        pop edi
        pop esi
        pop ebp
        pop ebx

        ret 0x4
    }
}


// Adjust width, height and centre point of THIRD person POV space views.
// __fastcall used here as class "this" is parsed on the ecx register.
//_____________________________________________________________________________________
static void __fastcall Set_Space_View_POV3(void* p_space_class, DRAW_BUFFER_MAIN* p_db) {

    WORD* p_view_vars = (WORD*)p_space_class;

    //Debug_Info("Set_Space_View_POV3 - %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d", p_view_vars[0], p_view_vars[1], p_view_vars[2], p_view_vars[3], p_view_vars[4], p_view_vars[5], p_view_vars[6], p_view_vars[7],
    //    p_view_vars[8], p_view_vars[9], p_view_vars[10], p_view_vars[11], p_view_vars[12], p_view_vars[13], p_view_vars[14], p_view_vars[15]);
    space_view_has_BG_image = FALSE;

    WORD width = (WORD)spaceWidth;
    WORD height = (WORD)spaceHeight;

    //rear view vdu screen is 122*100, check for it here to prevent it being resized.
    if (p_db && p_db->rc.right == 121 && p_db->rc.bottom == 99) {
        width = 122;
        height = 100;
        //if in cockpit view, make sure flag is set to enable clipping rect.
        if (*p_wc3_view_current_dir == SPACE_VIEW_TYPE::Cockpit)
            space_view_has_BG_image = TRUE;
    }
    else
        surface_space2D->ScaleTo((float)clientWidth, (float)clientHeight, SCALE_TYPE::fit);// dont alter the scale type when drawing rear view vdu.


    p_view_vars[4] = width;
    p_view_vars[5] = height;

    p_view_vars[6] = width / 2;
    p_view_vars[7] = height / 2;

    
    //Debug_Info("Set_Space_View_POV3 - %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d", p_view_vars[0], p_view_vars[1], p_view_vars[2], p_view_vars[3], p_view_vars[4], p_view_vars[5], p_view_vars[6], p_view_vars[7],
    //    p_view_vars[8], p_view_vars[9], p_view_vars[10], p_view_vars[11], p_view_vars[12], p_view_vars[13], p_view_vars[14], p_view_vars[15]);
}


//_____________________________________________________
static void __declspec(naked) set_space_view_pov3(void) {
    //structure ptr ecx, holds various details regarding space flight
    //word @ [ecx + 0x8] = view width
    //word @ [ecx + 0xA] = view height
    //word @ [ecx + 0xC] = view x centre
    //word @ [ecx + 0xE] = view y centre
    __asm {
        push ebx
        push ebp
        push esi
        push edi

        //push ecx
        mov edx, [esp+0x14]
        call Set_Space_View_POV3


        pop edi
        pop esi
        pop ebp
        pop ebx

        ret 0x4
    }
}


//_______________________________
static void Display_Space_Scene() {
    Display_Dx_Present(PRESENT_TYPE::space);
}


//For storing the general buffer and dimensions, that many functions draw to.
//So that we can switch them with the DX11 buffer for drawing 3D space.
DRAW_BUFFER db_3d_backup = { 0 };
BYTE* pbuffer_space_3D = nullptr;
LONG buffer_space_3D_pitch = 0;

BYTE* pbuffer_space_2D = nullptr;
LONG buffer_space_2D_pitch = 0;

BYTE* pbuffer_space_hud_targeting = nullptr;
LONG buffer_space_hud_targeting_pitch = 0;

RECT rc_targeting_main{ 0 };
RECT rc_targeting_scaled{ 0 };
float f_targeting_x_mul = 0.0f;
float f_targeting_y_mul = 0.0f;


//_____________________________________
static void Set_Space_3D_Surface_Rect() {

    (**pp_wc3_db_game).buff = pbuffer_space_3D;
    (**pp_wc3_db_game).rc_inv.left = buffer_space_3D_pitch - 1;
    (**pp_wc3_db_game).rc_inv.top = spaceHeight - 1;

    (**pp_wc3_db_game_main).rc.left = 0;
    (**pp_wc3_db_game_main).rc.top = 0;
    (**pp_wc3_db_game_main).rc.right = spaceWidth - 1;
    (**pp_wc3_db_game_main).rc.bottom = spaceHeight - 1;
}


//_____________________________________
static void Set_Space_2D_Surface_Rect() {

    (**pp_wc3_db_game).buff = pbuffer_space_2D;
    (**pp_wc3_db_game).rc_inv.left = buffer_space_2D_pitch - 1;
    (**pp_wc3_db_game).rc_inv.top = GUI_HEIGHT - 1;

    (**pp_wc3_db_game_main).rc.left = 0;
    (**pp_wc3_db_game_main).rc.top = 0;
    (**pp_wc3_db_game_main).rc.right = GUI_WIDTH - 1;
    (**pp_wc3_db_game_main).rc.bottom = GUI_HEIGHT - 1;
}


//_______________________________
static void Lock_Space_Surfaces() {

    if (pbuffer_space_3D != nullptr) 
        Debug_Info_Error("Lock_Space_Surfaces: 3D - buffer already locked!!!");
    else if (surface_space3D->Lock((VOID**)&pbuffer_space_3D, &buffer_space_3D_pitch) != S_OK)
        Debug_Info_Error("Lock_Space_Surfaces: 3D - lock failed!!!");
    
    if (pbuffer_space_2D != nullptr)
        Debug_Info_Error("Lock_Space_Surfaces: 2D - buffer already locked!!!");
    else if (surface_space2D->Lock((VOID**)&pbuffer_space_2D, &buffer_space_2D_pitch) != S_OK)
        Debug_Info_Error("Lock_Space_Surfaces: 2D - lock failed!!!");
    else// clear surface to the mask colour.
        memset(pbuffer_space_2D, 0xFF, buffer_space_2D_pitch * GUI_HEIGHT);
    
    if (pbuffer_space_hud_targeting != nullptr)
        Debug_Info_Error("Lock_Space_Surfaces: Targeting - buffer already locked!!!");
    else if (surface_space_targeting_hud->Lock((VOID**)&pbuffer_space_hud_targeting, &buffer_space_hud_targeting_pitch) != S_OK)
        Debug_Info_Error("Lock_Space_Surfaces: Targeting - lock failed!!!");
    else// clear surface to the mask colour.
        memset(pbuffer_space_hud_targeting, 0xFF, buffer_space_hud_targeting_pitch * surface_space_targeting_hud->GetHeight());

    //backup current buffer data
    db_3d_backup.rc_inv.right = (**pp_wc3_db_game_main).rc.right;
    db_3d_backup.rc_inv.bottom = (**pp_wc3_db_game_main).rc.bottom;
    db_3d_backup.rc_inv.left = (**pp_wc3_db_game).rc_inv.left;
    db_3d_backup.rc_inv.top = (**pp_wc3_db_game).rc_inv.top;
    db_3d_backup.buff = (*pp_wc3_db_game)->buff;

    //set buffer for drawing 3d space elements
    Set_Space_3D_Surface_Rect();
}


//_______________________________________________________
static void Lock_Space_Surfaces_POV1(void* p_space_class) {

    if (((WORD*)p_space_class)[4] != (WORD)spaceWidth || ((WORD*)p_space_class)[5] != (WORD)spaceHeight) {
        //Debug_Info("RESIZING SPACE VIEW POV1");
        DWORD* p_cockpit_class = ((DWORD**)p_space_class)[67];
        //Debug_Info("RESIZING SPACE VIEW POV1: %d", p_cockpit_class[8]);
        Set_Space_View_POV1(p_space_class);
    }
    Lock_Space_Surfaces();
}


//__________________________________________________________
static void __declspec(naked) lock_space_surfaces_pov1(void) {

    __asm {
        //push edx
        push ebx
        push esi

        push esi
        call Lock_Space_Surfaces_POV1
        add esp, 0x4

        pop esi
        pop ebx
        //pop edx
        ret
    }
}


//________________________________________________________
static void Lock_Space_Surfaces_POV3(void* p_space_struct) {
    //Debug_Info("Lock_3DSpace_Surface_POV3");
    if (((WORD*)p_space_struct)[4] != (WORD)spaceWidth || ((WORD*)p_space_struct)[5] != (WORD)spaceHeight) {
        //Debug_Info("RESIZING SPACE VIEW POV3");
        Set_Space_View_POV3((WORD*)p_space_struct, nullptr);
    }
    Lock_Space_Surfaces();
}


//__________________________________________________________
static void __declspec(naked) lock_space_surfaces_pov3(void) {

    __asm {
        //push edx
        push ebx
        push ebp
        push esi
        push edi

        push edi
        call Lock_Space_Surfaces_POV3
        add esp, 0x4

        pop edi
        pop esi
        pop ebp
        pop ebx
        //pop edx
        ret
    }
}


//_________________________________
static void UnLock_Space_Surfaces() {

    if (!pbuffer_space_3D) 
        Debug_Info_Error("UnLock_Space_Surfaces: 3D - buffer wasn't locked!!!");
    else
        surface_space3D->Unlock();
    pbuffer_space_3D = nullptr;
    
    if (!pbuffer_space_2D)
        Debug_Info_Error("UnLock_Space_Surfaces: 2D - buffer wasn't locked!!!");
    else
        surface_space2D->Unlock();
    pbuffer_space_2D = nullptr;

    if (!pbuffer_space_hud_targeting)
        Debug_Info_Error("UnLock_Space_Surfaces: Targeting - buffer wasn't locked!!!");
    else
        surface_space_targeting_hud->Unlock();
    pbuffer_space_hud_targeting = nullptr;

    //restore backup buffer data
    (**pp_wc3_db_game).buff = db_3d_backup.buff;

    (**pp_wc3_db_game_main).rc.right = db_3d_backup.rc_inv.right;
    (**pp_wc3_db_game_main).rc.bottom = db_3d_backup.rc_inv.bottom;

    (**pp_wc3_db_game).rc_inv.left = db_3d_backup.rc_inv.left;
    (**pp_wc3_db_game).rc_inv.top = db_3d_backup.rc_inv.top;
}


//___________________________________________________________
static void __declspec(naked) set_space_2d_surface_rect(void) {

    __asm {
        push ebx
        push esi

        call Set_Space_2D_Surface_Rect

        pop esi
        pop ebx
        ret
    }
}


//___________________________________________________________________
static void __declspec(naked) unlock_space_surfaces_and_display(void) {

    __asm {
        push ebx
        push ebp
        push esi

        call UnLock_Space_Surfaces
        call Display_Space_Scene

        pop esi
        pop ebp
        pop ebx
        ret
    }
}


//________________________________________________________________
static void __declspec(naked) set_space_2d_surface_rect_pov3(void) {

    __asm {
        push ecx
        push ebx
        push ebp
        push esi

        call Set_Space_2D_Surface_Rect

        pop esi
        pop ebp
        pop ebx
        pop ecx

        call wc3_draw_hud_view_text
        ret
    }
}


//____________________________________________________________
static void Draw_Hud_Targeting_Elements(void* p_shapes_struct) {
    
    int16_t space_x_bak = *p_wc3_space_x;
    int16_t space_y_bak = *p_wc3_space_y;



    //set main draw buffer to targeting buffer
    (**pp_wc3_db_game).buff = pbuffer_space_hud_targeting;
    (**pp_wc3_db_game).rc_inv.left = buffer_space_hud_targeting_pitch - 1;
    (**pp_wc3_db_game).rc_inv.top = surface_space_targeting_hud->GetHeight() - 1;
    
    RECT* p_rc_cockpit = (RECT*)((BYTE*)p_shapes_struct + 4);

    
    if (p_rc_cockpit->left != -1 || space_use_original_aspect) {

        float x_unit = 1.0f;
        float y_unit = 1.0f;
        float x = 0;
        float y = 0;
        surface_space2D->GetPosition(&x, &y);
        surface_space2D->GetScaledPixelDimensions(&x_unit, &y_unit);

        if (is_space_scaled) {
            float space_scale_x = (float)spaceWidth / clientWidth;
            float space_scale_y = (float)spaceHeight / clientHeight;
            x *= space_scale_x;
            y *= space_scale_y;
            x_unit *= space_scale_x;
            y_unit *= space_scale_y;
        }

        if (p_rc_cockpit->left != -1) {
            (**pp_wc3_db_game_main).rc.left = (LONG)(p_rc_cockpit->left * x_unit + x);
            (**pp_wc3_db_game_main).rc.top = (LONG)(p_rc_cockpit->top * y_unit + y);
            (**pp_wc3_db_game_main).rc.right = (LONG)(p_rc_cockpit->right * x_unit + x);
            (**pp_wc3_db_game_main).rc.bottom = (LONG)(p_rc_cockpit->bottom * y_unit + y);
        }
        else if (space_use_original_aspect) {
            (**pp_wc3_db_game_main).rc.left = (LONG)(x);
            (**pp_wc3_db_game_main).rc.top = (LONG)(y);
            (**pp_wc3_db_game_main).rc.right = (LONG)((GUI_WIDTH - 1) * x_unit + x);
            (**pp_wc3_db_game_main).rc.bottom = (LONG)((GUI_HEIGHT- 1) * y_unit + y);
        }

        *p_wc3_space_x -= (int16_t)(**pp_wc3_db_game_main).rc.left;
        *p_wc3_space_y -= (int16_t)(**pp_wc3_db_game_main).rc.top;
    }
    else {
        //set main draw buffer rect to correctly calculate line and image positions relating to 3D objects.
        (**pp_wc3_db_game_main).rc.left = 0;
        (**pp_wc3_db_game_main).rc.top = 0;
        (**pp_wc3_db_game_main).rc.right = spaceWidth - 1;
        (**pp_wc3_db_game_main).rc.bottom = spaceHeight - 1;
    }
    
    memcpy(&rc_targeting_main, &(**pp_wc3_db_game_main).rc, sizeof(RECT));


    f_targeting_y_mul = (float)surface_space_targeting_hud->GetHeight() / spaceHeight;
    f_targeting_x_mul = (float)surface_space_targeting_hud->GetWidth() / spaceWidth;

    rc_targeting_scaled.left = (LONG)((**pp_wc3_db_game_main).rc.left * f_targeting_x_mul);
    rc_targeting_scaled.top = (LONG)((**pp_wc3_db_game_main).rc.top * f_targeting_y_mul);
    rc_targeting_scaled.right = (LONG)((**pp_wc3_db_game_main).rc.right * f_targeting_x_mul);
    rc_targeting_scaled.bottom = (LONG)((**pp_wc3_db_game_main).rc.bottom * f_targeting_y_mul);


    wc3_draw_hud_targeting_elements(p_shapes_struct);

    *p_wc3_space_x = space_x_bak;
    *p_wc3_space_y = space_y_bak;

    Set_Space_2D_Surface_Rect();
}



//_____________________________________________________________
static void __declspec(naked) draw_hud_targeting_elements(void) {

    __asm {
        push edx
        push ebx
        push ebp
        push esi
        push edi

        push ecx
        call Draw_Hud_Targeting_Elements
        add esp, 0x4

        pop edi
        pop esi
        pop ebp
        pop ebx
        pop edx
        ret
    }
}


//______________________________________________________
static LONG Fix_Hud_Targeting_Rect_Size(int side_length) {

    float length = (float)spaceHeight;
    if (spaceWidth < spaceHeight)
        length = (float)spaceWidth;

    return (LONG)(length / 480.0f * side_length);
}


//_________________________________________________________________
static void __declspec(naked) fix_hud_targeting_rect_min_size(void) {

    __asm {
        push ecx
        push esi

        push 5
        call Fix_Hud_Targeting_Rect_Size
        add esp, 0x4

        mov edx, eax

        pop esi
        pop ecx
        ret
    }
}

//_________________________________________________________________
static void __declspec(naked) fix_hud_targeting_rect_max_size(void) {

    __asm {
        push esi

        push 28
        call Fix_Hud_Targeting_Rect_Size
        add esp, 0x4

        mov edx, eax

        pop esi
        ret
    }
}


//_____________________________________________________________________________________________________________________________
static LONG Draw_Space_Targeting_Line(DRAW_BUFFER_MAIN* p_db, LONG x1, LONG y1, LONG x2, LONG y2, DWORD arg6, DWORD colour_ref) {

    memcpy(&(**pp_wc3_db_game_main).rc, &rc_targeting_scaled, sizeof(RECT));

    x1 = (LONG)((float)x1 * f_targeting_x_mul);
    y1 = (LONG)((float)y1 * f_targeting_y_mul);
    x2 = (LONG)((float)x2 * f_targeting_x_mul);
    y2 = (LONG)((float)y2 * f_targeting_y_mul);

    LONG ret = wc3_draw_line(p_db, x1, y1, x2, y2, arg6, colour_ref);

    memcpy(&(**pp_wc3_db_game_main).rc, &rc_targeting_main, sizeof(RECT));
    return ret;
}


//_________________________________________________________________________________________________________________
static LONG Draw_Space_Targeting_Shape(DRAW_BUFFER_MAIN* p_db, void* shape_data, DWORD shape_num, DWORD x, DWORD y) {

    memcpy(&(**pp_wc3_db_game_main).rc, &rc_targeting_scaled, sizeof(RECT));

    x = (LONG)((float)x * f_targeting_x_mul);
    y = (LONG)((float)y * f_targeting_y_mul);
    LONG ret = wc3_shape_draw(p_db, shape_data, shape_num, x, y);

    memcpy(&(**pp_wc3_db_game_main).rc, &rc_targeting_main, sizeof(RECT));
    return ret;
}


//__________________________________________________________________________________________________________
static DWORD Get_Space_Targeting_Shape_WidthHeight_Locked_Direction_Marker(void* shape_data, LONG shape_num) {

    DWORD wh = wc3_shape_get_width_height(shape_data, shape_num);

    int16_t h = wh >> 16;
    int16_t w = wh & 0x0000FFFF;

    w = (int16_t)(w / f_targeting_x_mul);
    h = (int16_t)(h / f_targeting_y_mul);
    return (w & 0x0000FFFF) | ((h & 0x0000FFFF) << 16);
}


//___________________________________________________________________
static void __declspec(naked) set_input_profile_nav_map_3d_draw(void) {

    __asm {
        pushad
        push PROFILE_NAV
        call Change_Profile_Type
        add esp, 0x4
        popad

        push ebp
        push edi

        push ecx
        call Lock_Space_Surfaces
        pop ecx

        call wc3_nav_screen

        call UnLock_Space_Surfaces

        pop edi
        pop ebp

        pushad
        push PROFILE_SPACE
        call Change_Profile_Type
        add esp, 0x4
        popad
        ret
    }
}


//_______________________________________________________________
static void __declspec(naked) set_space_2d_surface_rect_nav(void) {

    __asm {
        push eax
        push ecx
        push ebx
        push esi

        call Set_Space_2D_Surface_Rect
 
        pop esi
        pop ebx
        pop ecx
        pop eax

        mov ebp, dword ptr ds : [eax + 0x8]
        mov edx, dword ptr ds : [eax + 0xC]
        ret
    }
}


//____________________________________________________________
static void __declspec(naked) nav_unlock_display_relock(void) {

    __asm {
        push ebx
        push esi

        call UnLock_Space_Surfaces
        mov is_nav_view, 1
        call Display_Space_Scene
        mov is_nav_view, 0
        call Lock_Space_Surfaces

        pop esi
        pop ebx

        ret
    }
}


//____________________________________________
static void SetWindowActivation(BOOL isActive) {

    //When game window loses focus, fullscreen mode needs to temporarily be put into windowed mode in order to appear on the taskbar and alt-tab display.
    if (!*p_wc3_is_windowed) {
        if (isActive == FALSE) {//Convert to windowed mode when app loses focus.
            SetWindowLongPtr(*p_wc3_hWinMain, GWL_EXSTYLE, 0);
            SetWindowLongPtr(*p_wc3_hWinMain, GWL_STYLE, WIN_MODE_STYLE | WS_VISIBLE);
            SetWindowPos(*p_wc3_hWinMain, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
            ShowWindow(*p_wc3_hWinMain, SW_RESTORE);
            //Debug_Info("SetWindowActivation full to win");
        }
        else if (isActive) {//Return to fullscreen mode when app regains focus.
            SetWindowLongPtr(*p_wc3_hWinMain, GWL_EXSTYLE, WS_EX_TOPMOST);
            SetWindowLongPtr(*p_wc3_hWinMain, GWL_STYLE, WS_POPUP | WS_VISIBLE);
            SetWindowPos(*p_wc3_hWinMain, HWND_TOPMOST, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
            ShowWindow(*p_wc3_hWinMain, SW_MAXIMIZE);
            //Debug_Info("SetWindowActivation win to full");
        }
    }
}


//________________________________________
void Set_WindowActive_State(BOOL isActive) {
    SetWindowActivation(isActive);
}


//______________________________________
static void Toggle_WindowMode(HWND hwnd) {

    *p_wc3_is_windowed = 1 - *p_wc3_is_windowed;
    ConfigWriteInt_InGame(L"MAIN", L"WINDOWED", *p_wc3_is_windowed);

    if (*p_wc3_is_windowed) {
        Debug_Info("Toggle_WindowMode: Windowed");
        WINDOWPLACEMENT winPlace{ 0 };
        winPlace.length = sizeof(WINDOWPLACEMENT);

        SetWindowLongPtr(hwnd, GWL_STYLE, WIN_MODE_STYLE);

        if (ConfigReadWinData(L"MAIN", L"WIN_DATA", &winPlace)) {
            if (winPlace.showCmd != SW_MAXIMIZE)
                winPlace.showCmd = SW_SHOWNORMAL;
        }
        else {
            GetWindowPlacement(hwnd, &winPlace);
            winPlace.showCmd = SW_SHOWNORMAL;
            Debug_Info("is_windowed GetWindowPlacement");
        }
        if (winPlace.showCmd == SW_SHOWNORMAL) //if the window isn't maximized
            Check_Window_GUI_Scaling_Limits(hwnd, &winPlace.rcNormalPosition, false);

        SetWindowPlacement(hwnd, &winPlace);

    }
    else {//Return to fullscreen mode when app regains focus.
        Debug_Info("Toggle_WindowMode: Fullscreen");
        SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP);
        //SetWindowPos(hwnd, 0, 0, 0, 0, 0, 0);
        ShowWindow(hwnd, SW_MAXIMIZE);
    }
    Display_Dx_Present();
}


//return false to call DefWindowProc
//_____________________________________________________________________________
static bool WinProc_Main(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {

    static bool is_cursor_hidden = true;
    static bool is_in_sizemove = false;

    switch (Message) {
    /*case WM_KEYDOWN:
        if (!(lParam & 0x40000000)) { //The previous key state. The value is 1 if the key is down before the message is sent, or it is zero if the key is up.
            if (wParam == VK_F11) { //Use F11 key to toggle windowed mode.
                if (pMovie_vlc)
                    pMovie_vlc->Pause(true);
                if (pMovie_vlc_Inflight)
                    pMovie_vlc_Inflight->Pause(true);
                Toggle_WindowMode(hwnd);
            }
        }
        break;*/
    case WM_WINDOWPOSCHANGING: {
        WINDOWPOS* winpos = (WINDOWPOS*)lParam;
        //Debug_Info("WM_WINDOWPOSCHANGING size adjusting");
        RECT rcWindow = { winpos->x, winpos->y, winpos->x + winpos->cx, winpos->y + winpos->cy };
        Check_Window_GUI_Scaling_Limits(hwnd, &rcWindow, true);
        winpos->x = rcWindow.left;
        winpos->y = rcWindow.top;
        winpos->cx = rcWindow.right - rcWindow.left;
        winpos->cy = rcWindow.bottom - rcWindow.top;
        return false;
    }
    case WM_WINDOWPOSCHANGED: {
        //Debug_Info("WM_WINDOWPOSCHANGED");
        if (IsIconic(hwnd))
            break;
        WINDOWPOS* winpos = (WINDOWPOS*)lParam;

        if (!is_in_sizemove) {
            if (!(winpos->flags & (SWP_NOSIZE))) {
                //Debug_Info("WM_WINDOWPOSCHANGED is_in_sizemove");
                Window_Resized();
                if (pMovie_vlc) {
                    pMovie_vlc->Pause(false);
                    pMovie_vlc->SetScale();
                }
                if (pMovie_vlc_Inflight) {
                    pMovie_vlc_Inflight->Pause(false);
                    pMovie_vlc_Inflight->Update_Display_Dimensions(nullptr);
                }
            }
            SetWindowTitle(hwnd, L"");
        }
        //SetWindowTitle(hwnd, L"");
        return true; //this should prevent calling DefWindowProc, and stop WM_SIZE and WM_MOVE messages being sent.
    }
    case WM_ENTERSIZEMOVE:
        //Debug_Info("WM_ENTERSIZEMOVE");
        is_in_sizemove = true;
        if (pMovie_vlc)
            pMovie_vlc->Pause(true);
        if (pMovie_vlc_Inflight)
            pMovie_vlc_Inflight->Pause(true);
        break;

    case WM_EXITSIZEMOVE:
        //Debug_Info("WM_EXITSIZEMOVE");
        is_in_sizemove = false;
        Window_Resized();
        if (pMovie_vlc) {
            pMovie_vlc->Pause(false);
            pMovie_vlc->SetScale();
        }
        if (pMovie_vlc_Inflight) {
            pMovie_vlc_Inflight->Pause(false);
            pMovie_vlc_Inflight->Update_Display_Dimensions(nullptr);
        }
        SetWindowTitle(hwnd, L"");
        break;
    case WM_CLOSE: {
        if (IsIconic(hwnd)) {//restore window first - game needs focus to exit
            if (SetForegroundWindow(hwnd))
                ShowWindow(hwnd, SW_RESTORE);
        }
        if (*pp_wc3_music_thread_class) {
            wc3_music_thread_class_destructor(*pp_wc3_music_thread_class);
            wc3_dealocate_mem01(*pp_wc3_music_thread_class);
            *pp_wc3_music_thread_class = nullptr;
            //Debug_Info("pp_wc3_music_thread_class destroyed");
        }
        return 0;
    }
    case WM_ENTERMENULOOP://allows system menu keys to fuction
        Set_WindowActive_State(FALSE);
        break;
    case WM_EXITMENULOOP:
        Set_WindowActive_State(TRUE);
        break;
    case WM_DISPLAYCHANGE:
        break;
    case WM_COMMAND:
        switch (wParam) {
        case 40005:
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            return 0;
            break;
        case 40002:
            //Debug_Info("dx message");
            Window_Setup(hwnd);
            SetWindowTitle(hwnd, L"");
            ShowWindow(hwnd, SW_SHOW);
            return 0;
            break;
        case 40004:
            //Debug_Info("40004 Calibrate Joystick");
            //wc3_unknown_func01();
            return 0;
            break;
        default:
            return 0;
            break;
        }
        break;
    case WM_SYSCOMMAND:
        switch ((wParam & 0xFFF0)) {
        case SC_SCREENSAVE:
        case SC_MONITORPOWER:
            return 0;
            break;
        case SC_MAXIMIZE:
            //Debug_Info("SC_MAXIMIZE");
        case SC_RESTORE:
            //Debug_Info("SC_RESTORE");
            if (pMovie_vlc)
                pMovie_vlc->Pause(true);
            if (pMovie_vlc_Inflight)
                pMovie_vlc_Inflight->Pause(true);
            return 0;
            break;
        default:
            break;
        }
        break;
    case WM_SETCURSOR: {
        DWORD currentWinStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
        if (GetForegroundWindow() == hwnd && (currentWinStyle & WS_POPUP) || (clip_cursor == TRUE)) {
            if (!is_cursor_clipped) {
                if (ClipMouseCursor()) {
                    is_cursor_clipped = true;
                    //Debug_Info("WM_SETCURSOR Mouse Cursor Clipped");
                }
                //else
                //    Debug_Info("WM_SETCURSOR ClipMouseCursor failed");
            }
            break;
        }
        if (is_cursor_clipped) {
            ClipCursor(nullptr);
            is_cursor_clipped = false;
            //Debug_Info("WM_SETCURSOR Mouse Cursor Un-Clipped");
        }
        if (hWin_Config_Control)
            break;//dont alter the cursor visibility when joy config window open.
        WORD ht = LOWORD(lParam);
        if (HTCLIENT == ht) {

            SetCursor(LoadCursor(nullptr, IDC_ARROW));

            if (IsMouseInClient()) {
                if (!is_cursor_hidden) {
                    is_cursor_hidden = true;
                    ShowCursor(false);
                }
            }
            else {
                if (is_cursor_hidden) {
                    is_cursor_hidden = false;
                    ShowCursor(true);
                }
            }
        }
        else {
            if (is_cursor_hidden) {
                is_cursor_hidden = false;
                ShowCursor(true);
            }
        }
        break;
    }
    case WM_ACTIVATEAPP:
        Set_WindowActive_State(wParam);
        if (wParam == FALSE) {
            Debug_Info("WM_ACTIVATEAPP false");
            if (is_cursor_clipped) {
                ClipCursor(nullptr);
                is_cursor_clipped = false;
                //Debug_Info("WM_ACTIVATEAPP false, Mouse Cursor Un-Clipped");
            }
            if (pMovie_vlc)
                pMovie_vlc->Pause(true);
            if (pMovie_vlc_Inflight)
                pMovie_vlc_Inflight->Pause(true);
            if (p_Music_Player)
                p_Music_Player->Pause(true);
        }
        else {
            Debug_Info("WM_ACTIVATEAPP true");
            if (is_cursor_clipped) {
                if (ClipMouseCursor()) {
                    //Debug_Info("WM_ACTIVATEAPP Mouse Cursor Clipped");
                }
            }
            if (pMovie_vlc)
                pMovie_vlc->Pause(false);
            if (pMovie_vlc_Inflight)
                pMovie_vlc_Inflight->Pause(false);
            if (p_Music_Player)
                p_Music_Player->Pause(false);
        }
        return 0;
        //case WM_ERASEBKGND:
        //    return 1;
        //case WM_DESTROY:
        //   return 1;
    case WM_PAINT:
        ValidateRect(hwnd, nullptr);
        return 1;
        break;
    default:
        break;
    }
    return 0;
}


//Add  WM_ENTERSIZEMOVE and WM_EXITSIZEMOVE checks to movie message check
//_____________________________________________________________
static void __declspec(naked) winproc_movie_message_check(void) {

    __asm {
        add eax, 0x111
        cmp eax, WM_COMMAND
        jne check2
        mov cl, 0
        ret
        check2 :
        cmp eax, WM_INITMENU
            jne check3
            mov cl, 1
            ret
            check3 :
        /*cmp eax, WM_LBUTTONDBLCLK
            jne check4
            mov cl, 2
            ret
            check4 :
        cmp eax, WM_RBUTTONDBLCLK
            jne check5
            mov cl, 3
            ret
            check5 :*/
        cmp eax, WM_ENTERSIZEMOVE
            jne check6
            mov cl, 1
            ret
            check6 :
        cmp eax, WM_EXITSIZEMOVE
            jne end_check
            mov cl, 1
            ret
            end_check :
        mov cl, 4
            ret
    }
}


//_______________________________________
static void Check_Optional_Enhancements() {

    if (ConfigReadInt(L"MAIN", L"ENABLE_CONTROLLER_ENHANCEMENTS", CONFIG_MAIN_ENABLE_CONTROLLER_ENHANCEMENTS))
        Modifications_Controller_Enhancements(); //Modifications_Joystick();
    if (ConfigReadInt(L"MAIN", L"ENABLE_MUSIC_ENHANCEMENTS", CONFIG_MAIN_ENABLE_MUSIC_ENHANCEMENTS))
        Modifications_Music();

    if (ConfigReadInt(L"SPACE", L"ENABLE_SPACE_COLOUR_CHANGE", CONFIG_SPACE_ENABLE_SPACE_COLOUR_CHANGE))
        Modifications_Space_Background_Colour();

    if (ConfigReadInt(L"DEBUG", L"REPLACE_ALT_X_MSG_WITH_ROOM_SCENE_ID", CONFIG_DEBUG_REPLACE_ALT_X_MSG_WITH_ROOM_SCENE_ID))
        Modifications_Replace_Alt_X_Msg_With_Room_Scene_ID();

    //Joysticks.Update();
}

//______________________________________________________
static void __declspec(naked) check_no_full_screen(void) {

    __asm {
        pushad
        call Check_Optional_Enhancements
        popad
        
        //set *p_wc3_is_windowed var depending on "-no_full_screen" command line arg.
        cmp eax, 0
        je no_full_screen
        mov al, 1
        no_full_screen:
        mov ebx, p_wc3_is_windowed
        mov byte ptr ds : [ebx] , al
        //re-insert the original code that the funtion call takes up. - set eax to the window handle
        mov eax, p_wc3_hWinMain
        mov eax, dword ptr ds : [eax]
        ret
    }
}


//____________________________________________
static void Conversation_Decision_ClipCursor() {
    clip_cursor = TRUE;
    wc3_conversation_decision_loop();
    clip_cursor = FALSE;
}


//______________________________________________________
static WORD* Translate_Messages_Mouse_ClipCursor_Space() {
    clip_cursor = TRUE;
    wc3_translate_messages(TRUE, FALSE);
    clip_cursor = FALSE;
    return p_wc3_mouse_button_space;
}


//_________________________________________________________
static void __declspec(naked) overide_cursor_clipping(void) {

    __asm {
        //pushad
        //call Print_Space_Time
        //popad
        mov clip_cursor, TRUE
        call wc3_update_input_states
        mov clip_cursor, FALSE
        ret
    }
}


//___________________________________________________________________
static void __declspec(naked) set_input_profile_space_simulator(void) {

    __asm {
        pushad
        push PROFILE_SPACE
        call Change_Profile_Type
        add esp, 0x4
        popad

        call wc3_space_simulator

        pushad
        push PROFILE_GUI
        call Change_Profile_Type
        add esp, 0x4
        popad

        ret
    }
}


//_________________________________________________________________
static void __declspec(naked) set_input_profile_space_mission(void) {

    __asm {
        pushad
        push PROFILE_SPACE
        call Change_Profile_Type
        add esp, 0x4
        popad

        call wc3_space_mission

        pushad
        push PROFILE_GUI
        call Change_Profile_Type
        add esp, 0x4
        popad

        ret
    }
}


//__________________________________________________________________
static void __declspec(naked) options_screen_set_input_profile(void) {

    __asm {
        pushad
        push PROFILE_GUI
        call Change_Profile_Type
        add esp, 0x4
        popad

        call wc3_options_screen

        pushad
        push PROFILE_SPACE
        call Change_Profile_Type
        add esp, 0x4
        popad
        ret
    }
}


//__________________________________________________________________
static void __declspec(naked) replay_screen_set_input_profile(void) {

    __asm {
        pushad
        push PROFILE_GUI
        call Change_Profile_Type
        add esp, 0x4
        popad

        call wc3_replay_screen_main

        pushad
        push PROFILE_SPACE
        call Change_Profile_Type
        add esp, 0x4
        popad
        ret
    }
}


//_________________________________________________________________________
static void __declspec(naked) set_input_profile_space_exit_game_start(void) {

    __asm {
        mov eax, p_wc3_space_exit_game_option_flag
        cmp byte ptr ds:[eax], 0 
        je exit_func
        pushad
        push PROFILE_GUI
        call Change_Profile_Type
        add esp, 0x4
        popad

        exit_func:
        ret
    }
}


//_______________________________________________________________________
static void __declspec(naked) set_input_profile_space_exit_game_end(void) {

    __asm {
        mov edx, p_wc3_space_exit_game_option_flag
        mov byte ptr ds:[edx], al

        pushad
        push PROFILE_SPACE
        call Change_Profile_Type
        add esp, 0x4
        popad
        ret
    }
}


//__________________________________________________________________________
static void __declspec(naked) set_input_profile_space_pause_game_start(void) {

    __asm {
        mov eax, p_wc3_space_pause_game_option_flag
        cmp byte ptr ds : [eax] , 0
        je exit_func
        pushad
        push PROFILE_GUI
        call Change_Profile_Type
        add esp, 0x4
        popad

        exit_func :
        ret
    }
}


//________________________________________________________________________
static void __declspec(naked) set_input_profile_space_pause_game_end(void) {

    __asm {
        mov edx, p_wc3_space_pause_game_option_flag
        mov byte ptr ds : [edx] , al

        pushad
        push PROFILE_SPACE
        call Change_Profile_Type
        add esp, 0x4
        popad
        ret
    }
}


// Throttle movement speed on the Navigation screen, which was unregulated and way to fast on modern computers.
// __fastcall used here as class "this" is parsed on the ecx register.
//__________________________________________________________________
static void __fastcall NavScreen_Movement_Speed_Fix(void*p_this_nav) {
    //return TRUE;////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static LARGE_INTEGER lastPresentTime = { 0 };
    LARGE_INTEGER time = { 0 };
    LARGE_INTEGER ElapsedMicroseconds = { 0 };

    QueryPerformanceCounter(&time);

    ElapsedMicroseconds.QuadPart = time.QuadPart - lastPresentTime.QuadPart;
    if (ElapsedMicroseconds.QuadPart < 0 || ElapsedMicroseconds.QuadPart > nav_update_time.QuadPart) {
        lastPresentTime.QuadPart = time.QuadPart;
        wc3_nav_screen_update_position(p_this_nav);
        return;
    }

    return;
}


//_______________________________________________________________
static void __declspec(naked) nav_screen_movement_speed_fix(void) {

    __asm {
        push ebx
        push ecx
        push edx
        push esi
        push edi
        push ebp

        call NavScreen_Movement_Speed_Fix

        pop ebp
        pop edi
        pop esi
        pop edx
        pop ecx
        pop ebx

        ret
    }
}


//____________________________________________________________
static void __declspec(naked) load_cockpit_hd_background(void) {

    __asm {
        pushad
        push edi
        call Load_Cockpit_HD_Background
        add esp, 0x4
        popad

        //insert original code
        mov ecx, -1
        ret

    }
}


//________________________________________
static BOOL Is_Not_Cockpit_HD_Background() {
    if (Get_Cockpit_HD_BG_Surface())
        return FALSE;
    return TRUE;
}


//____________________________________________________________________
//Check for the existence of a HD cockpit background alternative before drawing regular backgoung image if present.
static void __declspec(naked) check_cockpit_alternate_background(void) {

    __asm {
        push ebx
        push ecx
        push edx
        push edi
        push esi
        push ebp
    
        call Is_Not_Cockpit_HD_Background
 
        pop ebp
        pop esi
        pop edi
        pop edx
        pop ecx
        pop ebx

        //check for hd cockpit background surface.
         test eax, eax
         je exit_func
        
         //insert original code - check if the view has a regular background image.
        mov eax, dword ptr ds : [esi + 0x4]
        test eax, eax
        exit_func:
        ret

    }
}


//_____________________________________________________
static void __declspec(naked) check_cockpit_death(void) {

    __asm {
        mov edi, p_wc3_is_ejecting
        cmp byte ptr ds:[edi], 0x0
        jne exitfunc

        pushad
        //fix fade out for death scene.
        call Destroy_Cockpit_HD_Background//destroy cockpit background so the the original graphic is use on death.
        call Set_Space2D_Surface_SamplerState_Point//ensure point sampler is used as linear sampler induces some artefacts.
        popad

        exitfunc:

        //insert original code
        mov edi, p_wc3_is_ejecting
        cmp byte ptr ds : [edi] , 0x0
        ret

    }
}


//____________________________________________________
static void __declspec(naked) apply_gamma_offset(void) {

    __asm {
        pushad

        mov esi, p_wc3_gamma_val
        push dword ptr ds:[esi]
        call Set_Gamma_Offset
        add esp, 0x4

        popad

        ret
    }
}


//__________________________________________________
static void __declspec(naked) check_gamma_high(void) {
    //replace byte integer for comparison of values larger than 127.
    __asm {
        mov eax, p_wc3_gamma_val
        cmp  dword ptr ds : [eax], 150
        ret
    }
}


//_____________________________________________________
static void __declspec(naked) modify_space_colour(void) {

    __asm {
        mov al, alternate_space_colour_pal_off

        mov edx, p_wc3_space_background_pal_offset
        mov byte ptr ds:[edx], al
        ret
    }
}


//__________________________________________
void Modifications_Space_Background_Colour() {

    alternate_space_colour_pal_off = ConfigReadInt(L"SPACE", L"SPACE_COLOUR_PALETTE_OFFSET", CONFIG_SPACE_COLOUR_PALETTE_OFFSET);
    MemWrite8(0x41FC35, 0xA2, 0xE8);
    FuncWrite32(0x41FC36, 0x4A26F8, (DWORD)&modify_space_colour);
}


//_________________________________________________________
static void Check_System_Keys(WPARAM wParam, LPARAM lParam) {

    //Debug_Info("Check_System_Keys wParam%X, lParam%X", wParam, lParam);
    if ((lParam & (1 << 29)) != 0) { //if ALT key is down.
        if ((lParam & (1 << 31)) != 0) {//if key is down
            if (wParam == VK_RETURN) { //toggle windowed mode.
                if (pMovie_vlc)
                    pMovie_vlc->Pause(true);
                if (pMovie_vlc_Inflight)
                    pMovie_vlc_Inflight->Pause(true);
                Toggle_WindowMode(*p_wc3_hWinMain);
            }
            else if (wParam == 'J') {//controller setup
                if (is_cursor_clipped)
                    ClipCursor(nullptr);
                JoyConfig_Main();
                if (is_cursor_clipped)
                    ClipMouseCursor();
 
            }
            else if (wParam == VK_F4) //close window on ALT + F4
                PostMessage(*p_wc3_hWinMain, WM_CLOSE, 0, 0);
        }
    }
}


//_______________________________________________
static void __declspec(naked) check_sys_key(void) {

    __asm {
        mov eax, dword ptr ss:[esp + 0x18]//lParam
        pushad
        push ecx
        push eax
        call Check_System_Keys
        add esp, 0x8
        popad

        //insert original code
        mov eax, ecx
        shr eax, 0x1F
        ret
    }
}


//___________________________
void Modifications_Display() {

    MemWrite8(0x430EC0, 0x53, 0xE9);
    FuncWrite32(0x430EC1, 0xDB335756, (DWORD)&Display_Exit);

    MemWrite8(0x405630, 0x8B, 0xE9);
    FuncWrite32(0x405631, 0x530C244C, (DWORD)&Palette_Update);

    MemWrite16(0x404A70, 0xEC83, 0x9090);
    MemWrite8(0x404A72, 0x6C, 0x90);
    MemWrite16(0x404A73, 0x3D80, 0xE990);
    FuncWrite32(0x404A75, 0x49F984, (DWORD)&LockSurface);

    MemWrite16(0x404B20, 0x3D80, 0xE990);
    FuncWrite32(0x404B22, 0x49F984, (DWORD)&UnlockShowSurface);

    //disable un-needed colour_fill function.
    MemWrite16(0x431390, 0x4C8B, 0xC033);//xor eax, eax
    MemWrite16(0x431392, 0x0824, 0xC390);

    //replace clear_screen function.
    MemWrite8(0x4057E0, 0x80, 0xE9);
    FuncWrite32(0x4057E1, 0x49F9843D, (DWORD)&Clear_GUI_Surface);
    MemWrite16(0x4057E5, 0x0000, 0x9090);

    MemWrite16(0x41BB80, 0xEC83, 0x9090);
    MemWrite8(0x41BB82, 0x6C, 0x90);
    MemWrite8(0x41BB83, 0xA1, 0xE9);
    FuncWrite32(0x41BB84, 0x4A329C, (DWORD)&LockSurface);

    MemWrite16(0x41BBD0, 0xEC81, 0xE990);
    FuncWrite32(0x41BBD2, 0x84, (DWORD)&UnlockShowSurface);

    //prevent direct draw - create surface func call
    MemWrite16(0x4313D0, 0xEC83, 0xC033);//xor eax, eax
    MemWrite8(0x4313D2, 0x6C, 0xC3);

    //replace direct draw - blit func
    MemWrite16(0x405890, 0x3D80, 0xE990);
    FuncWrite32(0x405892, 0x49F984, (DWORD)&DXBlt);

    //prevent direct draw - palette update func call
    MemWrite8(0x431360, 0x83, 0xC3);

    //replace space first person view setup function
    MemWrite8(0x425B40, 0x8B, 0xE9);
    FuncWrite32(0x425B41, 0x8B042454, (DWORD)&set_space_view_pov1);
    //MemWrite16(0x425B45, 0x0C42, 0x9090);

    //replace direct draw lock surface in draw space first person view function
    MemWrite16(0x425D22, 0x6774, 0x9090);//prevent jumping before this is called

    MemWrite16(0x425D24, 0x3D83, 0xE890);
    FuncWrite32(0x425D26, 0x004A3298, (DWORD)&lock_space_surfaces_pov1);
    MemWrite8(0x425D2A, 0x00, 0x90);
    MemWrite8(0x425D2B, 0x74, 0xEB);//jmp over ddraw stuff

    //replace direct draw stuff in draw space first person view function - set 2d surface for hud etc.
    MemWrite16(0x425E1F, 0x840F, 0x9090);//prevent jumping before this is called
    MemWrite32(0x425E21, 0x000000F9, 0x90909090);

    MemWrite16(0x425E25, 0x3D83, 0xE890);
    FuncWrite32(0x425E27, 0x004A3298, (DWORD)&set_space_2d_surface_rect);
    MemWrite8(0x425E2B, 0x00, 0x90);
    MemWrite16(0x425E2C, 0x840F, 0xE990);//jmp over ddraw stuff

    //replace direct draw stuff in draw space first person view function - unlock space surfaces and display.
    FuncReplace32(0x425F75, 0xFFFDEBA7, (DWORD)&unlock_space_surfaces_and_display);

    //draw targeting elements to 3d space
    FuncReplace32(0x41B1A8, 0x00041614, (DWORD)&draw_hud_targeting_elements);

    //fix the min size of targeting rect to match the ratio between it and the original screen size.
    MemWrite8(0x45D91D, 0xBA, 0xE8);
    FuncWrite32(0x45D91E, 0x05, (DWORD)&fix_hud_targeting_rect_min_size);

    //fix the max size of targeting rect to match the ratio between it and the original screen size.
    MemWrite8(0x45D94B, 0xBA, 0xE8);
    FuncWrite32(0x45D94C, 0x1C, (DWORD)&fix_hud_targeting_rect_max_size);

    //replace direct draw lock surface in draw space third person view function.
    MemWrite16(0x42EB47, 0x6074, 0x9090);//prevent jumping before this is called

    MemWrite16(0x42EB49, 0x3D83, 0xE890);
    FuncWrite32(0x42EB4B, 0x004A3298, (DWORD)&lock_space_surfaces_pov3);
    MemWrite8(0x42EB4F, 0x00, 0x90);
    MemWrite8(0x42EB50, 0x74, 0xEB);//jmp over ddraw stuff

    //replace direct draw stuff in draw space third person view function - set 2d surface for text etc.
    FuncReplace32(0x42EC3C, 0x0002A8C0, (DWORD)&set_space_2d_surface_rect_pov3);

    //replace direct draw stuff in draw space third person view function - unlock space surfaces and display.
    FuncReplace32(0x42EC65, 0xFFFD5EB7, (DWORD)&unlock_space_surfaces_and_display);

    //replace space third person view setup function
    MemWrite8(0x48D050, 0x8B, 0xE9);
    FuncWrite32(0x48D051, 0x56042444, (DWORD)&set_space_view_pov3);

    //skip adjusting targeting hud to cockpit window, this is now done in Draw_Hud_Targeting_Elements
    MemWrite16(0x45C7D7, 0x840F, 0xE990);
    MemWrite8(0x45D21B, 0x74, 0xEB);

    //draw nav screen space view to 3d surface, seperate from 2d elements
    FuncReplace32(0x445120, 0x2C, (DWORD)&set_input_profile_nav_map_3d_draw);

    //set 2d surface for drawing nav screen 2d elements
    MemWrite16(0x44530A, 0x688B, 0xE890);
    FuncWrite32(0x44530C, 0x0C508B08, (DWORD)&set_space_2d_surface_rect_nav);

    //set 3d surface after drawing nav screen 2d elements
    MemWrite16(0x44562F, 0x15FF, 0xE890);
    FuncWrite32(0x445631, 0x49F9BC, (DWORD)&nav_unlock_display_relock);

    //draw nav point cross marker
    FuncReplace32(0x45D445, 0x0142CB, (DWORD)&Draw_Space_Targeting_Shape);

    //fix position of locked target offscreen direction marker
    FuncReplace32(0x45CA59, 0x019221, (DWORD)&Get_Space_Targeting_Shape_WidthHeight_Locked_Direction_Marker);
    //draw locked target offscreen direction marker
    FuncReplace32(0x45CCF1, 0x014A1F, (DWORD)&Draw_Space_Targeting_Shape);

    //draw locked target leading marker
    FuncReplace32(0x45D81A, 0x013EF6, (DWORD)&Draw_Space_Targeting_Shape);

    //draw missile lock target markers
    FuncReplace32(0x45D042, 0x0146CE, (DWORD)&Draw_Space_Targeting_Shape);
    FuncReplace32(0x45D06C, 0x0146A4, (DWORD)&Draw_Space_Targeting_Shape);
    FuncReplace32(0x45D095, 0x01467B, (DWORD)&Draw_Space_Targeting_Shape);
    FuncReplace32(0x45D0BF, 0x014651, (DWORD)&Draw_Space_Targeting_Shape);

    //draw locked target lines
    FuncReplace32(0x45D9A6, 0x01322F, (DWORD)&Draw_Space_Targeting_Line);
    FuncReplace32(0x45D9C0, 0x013215, (DWORD)&Draw_Space_Targeting_Line);
    FuncReplace32(0x45D9DA, 0x0131FB, (DWORD)&Draw_Space_Targeting_Line);
    FuncReplace32(0x45D9F4, 0x0131E1, (DWORD)&Draw_Space_Targeting_Line);

    //draw unlocked target lines
    FuncReplace32(0x45DA2F, 0x0131A6, (DWORD)&Draw_Space_Targeting_Line);
    FuncReplace32(0x45DA53, 0x013182, (DWORD)&Draw_Space_Targeting_Line);
    FuncReplace32(0x45DA77, 0x01315E, (DWORD)&Draw_Space_Targeting_Line);
    FuncReplace32(0x45DA94, 0x013141, (DWORD)&Draw_Space_Targeting_Line);

    FuncReplace32(0x45DAB3, 0x013122, (DWORD)&Draw_Space_Targeting_Line);
    FuncReplace32(0x45DAD1, 0x013104, (DWORD)&Draw_Space_Targeting_Line);
    FuncReplace32(0x45DAEE, 0x0130E7, (DWORD)&Draw_Space_Targeting_Line);
    FuncReplace32(0x45DB08, 0x0130CD, (DWORD)&Draw_Space_Targeting_Line);

    //0041B1C0 | .E8 4F650500   CALL DRAW_IMAGE(*dib_struct, ) ? ; DRAW_COCKPIT_Back_groung
    //MemWrite8(0x41B1C0, 0xE8, 0x90);
    //MemWrite32(0x41B1C1, 0x05654F, 0x90909090);

    //0041B1D0 | .E8 3B000000   CALL 0041B210
    //MemWrite8(0x41B1D0, 0xE8, 0x90);
    //MemWrite32(0x41B1D1, 0x3B, 0x90909090);

    //0041B1D7 | .E8 D4020000   CALL 0041B4B0//draw sheilds radar and target details , pilot hands
    //MemWrite8(0x41B1D7, 0xE8, 0x90);
    //MemWrite32(0x41B1D8, 0x02D4, 0x90909090);

    //0041B1DE | .E8 3D040000   CALL 0041B620; [wc3w.0041B620  //draw speed, target speed
    //MemWrite8(0x41B1DE, 0xE8, 0x90);
    //MemWrite32(0x41B1DF, 0x043D, 0x90909090);

    //Set space subtitle text background colour to 0. As original 255 coflicts with the mask colour being used to draw all cockpit/hud elements to a seperate surface.
    MemWrite8(0x4596A6, 0xFF, 0x00);

    //Set program to send window message to setup directx no matter if windowed or fullscreen.
    //after "-no_full_screen" check force dx setup either way
    MemWrite8(0x405013, 0x74, 0xEB);
    //recheck if "-no_full_screen" was set and set *p_wc3_is_windowed var.
    MemWrite8(0x405027, 0xA1, 0xE8);
    FuncWrite32(0x405028, 0x004A5AAC, (DWORD)&check_no_full_screen);

    //replace the main window message checking function for greater functionality.
    MemWrite8(0x405090, 0x8B, 0xE9);
    FuncWrite32(0x405091, 0x53082444, (DWORD)&WinProc_Main);

    //Add  WM_ENTERSIZEMOVE and WM_EXITSIZEMOVE checks to movie message check
    //increase range of message code selection
    MemWrite32(0x41C73B, 0xF5, 0x121);
    //check for the messages
    MemWrite16(0x41C747, 0x888A, 0xE890);
    FuncWrite32(0x41C749, 0x41C8A0, (DWORD)&winproc_movie_message_check);

    //in space - return un-scaled position of mouse as space view isn't scaled
    FuncReplace32(0x4222C2, 0x06112A, (DWORD)&Translate_Messages_Mouse_ClipCursor_Space);

    //clip the cursor while a conversation decision is made.
    FuncReplace32(0x4825D4, 0xFFFFFF08, (DWORD)&Conversation_Decision_ClipCursor);

    //Enable cursor clipping for all WM_SETCURSOR messages within calls to this function, used during space flight.
    FuncReplace32(0x40F75F, 0xFFFF8C8D, (DWORD)&overide_cursor_clipping);
    FuncReplace32(0x430087, 0xFFFD8365, (DWORD)&overide_cursor_clipping);

    //004213D5 | .E8 3A030500   CALL DRAW_IMAGE(*dib_struct, ) ? //draw crosshairs
    //MemWrite8(0x4213D5, 0xE8, 0x90);
    //MemWrite32(0x4213D6, 0x05033A, 0x90909090);

    //fix control reaction speed on the nav screen.
    FuncReplace32(0x44526E, 0xFFFEB1EE, (DWORD)&nav_screen_movement_speed_fix);

    // HD Cockpits--------------------------------------------------------
    MemWrite8(0x45288C, 0xB9, 0xE8);
    FuncWrite32(0x45288D, 0xFFFFFFFF, (DWORD)&load_cockpit_hd_background);

    MemWrite8(0x41B1AC, 0x8B, 0xE8);
    FuncWrite32(0x41B1AD, 0xC0850446, (DWORD)&check_cockpit_alternate_background);

    MemWrite16(0x422016, 0x3D80, 0xE890);
    FuncWrite32(0x422018, 0x4A2DA0, (DWORD)&check_cockpit_death);
    MemWrite8(0x42201C, 0x00, 0x90);
    //--------------------------------------------------------------------

    // Gamma Correction modifications--------------------------------------------------------------------------
    // Use a shader to set the gamma instead of altering the palette.
    // Set default gamma level to "off", instead of "low".
    // Made gamma a global setting instead of being restored from saved games.
    
    //setting level(0-2) value when saving game
    //check gamma - high
    MemWrite16(0x408A5A, 0x3D83, 0xE890);
    FuncWrite32(0x408A5C, 0x49F744, (DWORD)&check_gamma_high);
    MemWrite8(0x408A60, 0x64, 0x90);
    //check gamma - off
    MemWrite8(0x408A6D, 0x41, 100);

    //setting gamma val when loading game from level(0-2) value;
    //jump over this section. don't set gamma from saved game file, make gamma a global value instead.
    MemWrite16(0x409260, 0xF883, 0x42EB);
    MemWrite8(0x409262, 0x01, 0x90);
    //set gamma - low
    //MemWrite32(0x409270, 0x50, 120);
    //set gamma - high
    //MemWrite32(0x40927C, 0x64, 150);
    //set gamma - off
    //MemWrite32(0x409288, 0x41, 100);

    //control selection
    //check lower limit
    MemWrite8(0x412762, 0x1E, 50);
    MemWrite32(0x41276B, 0x1E, 50);
    //check upper limit
    MemWrite32(0x4127A1, 0x8C, 200);

    //options gui - set gamma val on level selection
    //selected - off
    MemWrite32(0x416580, 0x41, 100);
    MemWrite8(0x416585, 0x41, 100);
    //selected - low
    MemWrite32(0x4165AC, 0x50, 120);
    MemWrite8(0x4165B1, 0x50, 120);
    //selected - high
    MemWrite32(0x4165D8, 0x64, 150);
    MemWrite8(0x4165DD, 0x64, 150);

    //options gui - set level button selection
    MemWrite16(0x416931, 0x3D83, 0xE890);
    FuncWrite32(0x416933, 0x49F744, (DWORD)&check_gamma_high);
    MemWrite8(0x416937, 0x64, 0x90);
    //check gamma off
    MemWrite8(0x41697D, 0x41, 100);

    //movie related
    MemWrite8(0x41BF0B, 0x1E, 50);
    MemWrite32(0x41BF2C, 0x80, 200-12);

    //keyboard selection
    //check lower limit
    MemWrite8(0x4504DE, 0x1E, 50);
    MemWrite32(0x4504E7, 0x1E, 50);
    //check upper limit
    MemWrite32(0x4504FE, 0x8C, 200);

    //ignore gamma setup for palette
    MemWrite8(0x41DB50, 0x8B, 0xC3);
    
    MemWrite16(0x4607A9, 0x8B55, 0xE990);
    FuncWrite32(0x4607AB, 0xC03356EC, (DWORD)&apply_gamma_offset);

    //Set the default Gamma setting to it's lowest level.
    MemWrite8(0x49F744, 0x50, 100);
    //------------------------------------------------------------------------------------------

    //set input profile to space when using the flight simulator.
    FuncReplace32(0x404949, 0x0315D3, (DWORD)&set_input_profile_space_simulator);
    //set input profile to space when flying a mission.
    FuncReplace32(0x404957, 0x031725, (DWORD)&set_input_profile_space_mission);

    //set input profile to gui when starting exit game screen.
    MemWrite16(0x44FF43, 0x3D80, 0xE890);
    FuncWrite32(0x44FF45, 0x4B17E8, (DWORD)&set_input_profile_space_exit_game_start);
    MemWrite8(0x44FF49, 0x00, 0x90);
    //set input profile back to space when ending exit game screen.
    MemWrite8(0x450010, 0xA2, 0xE8);
    FuncWrite32(0x450011, 0x4B17E8, (DWORD)&set_input_profile_space_exit_game_end);

    //set input profile to gui when starting pause game screen.
    MemWrite16(0x45001F, 0x3D80, 0xE890);
    FuncWrite32(0x450021, 0x4B17CC, (DWORD)&set_input_profile_space_pause_game_start);
    MemWrite8(0x450025, 0x00, 0x90);
    //set input profile back to space when ending pause game screen.
    MemWrite8(0x450092, 0xA2, 0xE8);
    FuncWrite32(0x450093, 0x4B17CC, (DWORD)&set_input_profile_space_pause_game_end);

    //set input profile to gui when using options screen(Alt+O)
    FuncReplace32(0x45A73E, 0xFFFDD55E, (DWORD)&options_screen_set_input_profile);
    
    //set input profile to gui when on the replay mission screen.
    FuncReplace32(0x4508A2, 0xFFFDF79A, (DWORD)&replay_screen_set_input_profile);

    //check for windowed mode toggle key combo(Alt+Enter) and controller setup key combo(Alt+J) in keyboard procedure.
    MemWrite8(0x482A1D, 0x8B, 0xE8);
    FuncWrite32(0x482A1E, 0x1FE8C1C1, (DWORD)&check_sys_key);
}


