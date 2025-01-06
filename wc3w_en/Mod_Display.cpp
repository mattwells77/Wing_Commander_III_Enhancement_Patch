/*
The MIT License (MIT)
Copyright © 2024 Matt Wells

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
#include "movies_vlclib.h"
#include "wc3w.h"

#define WIN_MODE_STYLE  WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX




BOOL is_cockpit_view = FALSE;
BOOL is_cockpit_fullscreen = TRUE;
BOOL is_nav_view = FALSE;

BOOL clip_cursor = FALSE;
static bool is_cursor_clipped = false;

LibVlc_Movie* pMovie_vlc = nullptr;


UINT clientWidth = 0;
UINT clientHeight = 0;

WORD mouse_state_true[3];
WORD* p_mouse_button_true = &mouse_state_true[0];
WORD* p_mouse_x_true = &mouse_state_true[1];
WORD* p_mouse_y_true = &mouse_state_true[2];

LARGE_INTEGER nav_update_time{ 0 };


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


//___________________________________________________
static BOOL Window_Setup(HWND hwnd, bool is_windowed) {

    if (ConfigReadInt("SPACE", "COCKPIT_MAINTAIN_ASPECT_RATIO", CONFIG_SPACE_COCKPIT_MAINTAIN_ASPECT_RATIO))
        is_cockpit_fullscreen = FALSE;

    if (is_windowed) {
        Debug_Info("Window Setup: Windowed");
        WINDOWPLACEMENT winPlace{ 0 };
        winPlace.length = sizeof(WINDOWPLACEMENT);
        
        SetWindowLongPtr(hwnd, GWL_STYLE, WIN_MODE_STYLE);
        //Debug_Info("is_windowed set style");

        if (ConfigReadWinData("MAIN", "WIN_DATA", &winPlace)) {
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

    *p_wc3_mouse_centre_x = (LONG)clientWidth / 2;
    *p_wc3_mouse_centre_y = (LONG)clientHeight / 2;

    Display_Dx_Setup(hwnd, clientWidth, clientHeight);

    //QueryPerformanceFrequency(&Frequency);

    //Set the movement update time for Navigation screen, which was unregulated and way to fast on modern computers.
    DXGI_RATIONAL refreshRate{};
    refreshRate.Denominator = 1;
    refreshRate.Numerator = ConfigReadInt("SPACE", "NAV_SCREEN_KEY_RESPONCE_HZ", CONFIG_SPACE_NAV_SCREEN_KEY_RESPONCE_HZ);
    nav_update_time.QuadPart = p_wc3_frequency->QuadPart / refreshRate.Numerator;

    //Set the max refresh rate in space, original 24 FPS. Set to zero to use screen refresh rate, a negative value will use the original value.  
    refreshRate.Numerator = ConfigReadInt("SPACE", "SPACE_REFRESH_RATE_HZ", CONFIG_SPACE_SPACE_REFRESH_RATE_HZ);
    if ((int)refreshRate.Numerator < 0)
        refreshRate.Numerator = 24;
    else if (refreshRate.Numerator == 0)
        Get_Monitor_Refresh_Rate(hwnd, &refreshRate);

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

    *p_wc3_mouse_centre_x = (LONG)clientWidth / 2;
    *p_wc3_mouse_centre_y = (LONG)clientHeight / 2;

    Display_Dx_Resize(clientWidth, clientHeight);


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
        ConfigWriteWinData("MAIN", "WIN_DATA", &winPlace);
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


//__________________________________
static void UnlockShowMovieSurface() {

    if (surface_gui == nullptr)
        return;
    surface_gui->Unlock();
    Display_Dx_Present(PRESENT_TYPE::movie);
}


//______________________________________________
static int ColourFill(DWORD ddSurface, int flag) {

    if (surface_gui == nullptr)
        return 0;
    /*  VOID* pSurface = nullptr;
      LONG pitch = 0;
      if (surface_gui->Lock(&pSurface, &pitch) != S_OK)
          return 0;
      memset(pSurface, 0xFF, pitch * GUI_HEIGHT);
      surface_gui->Unlock();
  */
    return 0;
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


//_______________________________________________________________
static void DXBlt_Movie(BYTE* fBuff, DWORD subY, DWORD subHeight) {

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

    Display_Dx_Present(PRESENT_TYPE::movie);

}


//_____________________________________________________________________________________
static BOOL DrawVideoFrame(VIDframe* vidFrame, RGBQUAD* tBuff, UINT tWidth, DWORD flag) {

    if (!surface_movieXAN || vidFrame->width != surface_movieXAN->GetWidth() || vidFrame->height != surface_movieXAN->GetHeight()) {
        if (surface_movieXAN)
            delete surface_movieXAN;
        surface_movieXAN = new GEN_SURFACE(vidFrame->width, vidFrame->height, 8);
        surface_movieXAN->ScaleToScreen(SCALE_TYPE::fit);
        Debug_Info("surface_movieXAN created");
    }
    //Debug_Info("%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X", vidFrame->unknown00, vidFrame->unknown04, vidFrame->unknown08, vidFrame->width, vidFrame->height, vidFrame->unknown14, vidFrame->bitFlag, vidFrame->unknown1C, vidFrame->unknown20, vidFrame->unknown24);

    BYTE* pSurface = nullptr;
    LONG pitch = 0;

    if (surface_movieXAN->Lock((VOID**)&pSurface, &pitch) != S_OK)
        return FALSE;

    BYTE* fBuff = vidFrame->buff;

    for (UINT y = 0; y < vidFrame->height; y++) {
        for (UINT x = 0; x < vidFrame->width; x++)
            pSurface[x] = fBuff[x];

        pSurface += pitch;
        fBuff += vidFrame->width;
    }

    surface_movieXAN->Unlock();
    MovieRT_SetRenderTarget();
    surface_movieXAN->Display();

    return TRUE;
}


// Adjust width, height and centre target point of FIRST person POV space views.
// __fastcall used here as class "this" is parsed on the ecx register.
//___________________________________________________
static void __fastcall Set_Space_View_POV1(void* p_space_class) {

    WORD* p_view_vars = (WORD*)p_space_class;
    DWORD* p_cockpit_class = ((DWORD**)p_space_class)[67]; //[p_space_struct + 0x10C]
    DWORD cockpit_view_type = p_cockpit_class[8];//[p_cockpit_struct + 0x20] //view_type: cockpit = 0, left = 1, rear = 2, right = 3, hud = 4.

    is_cockpit_view = FALSE;
    SCALE_TYPE scale_type = SCALE_TYPE::fit;


    WORD width = (WORD)clientWidth;
    WORD height = (WORD)clientHeight;

    p_view_vars[4] = width;
    p_view_vars[5] = height;

    switch (cockpit_view_type) {
    case 0:
        p_view_vars[6] = (WORD)(*p_wc3_x_centre_cockpit / (float)GUI_WIDTH * width);
        p_view_vars[7] = (WORD)(*p_wc3_y_centre_cockpit / (float)GUI_HEIGHT * height);
        is_cockpit_view = TRUE;
        if (is_cockpit_fullscreen)
            scale_type = SCALE_TYPE::fill;
        Debug_Info("Set Space View - CockPit");
        Debug_Info("centre_x=%d, centre_y=%d, new_centre_x=%d, new_centre_y=%d", *p_wc3_x_centre_cockpit, *p_wc3_y_centre_cockpit, p_view_vars[6], p_view_vars[7]);
        break;
    case 1:
        Debug_Info("Set Space View - Left");
        break;
    case 2:
        p_view_vars[6] = (WORD)(*p_wc3_x_centre_rear / (float)GUI_WIDTH * width);
        p_view_vars[7] = (WORD)(*p_wc3_y_centre_rear / (float)GUI_HEIGHT * height);
        Debug_Info("Set Space View - Rear");
        Debug_Info("centre_x=%d, centre_y=%d, new_centre_x=%d, new_centre_y=%d", *p_wc3_x_centre_rear, *p_wc3_y_centre_rear, p_view_vars[6], p_view_vars[7]);
        break;
    case 3:
        Debug_Info("Set Space View - Right");
        break;
    case 4:
        p_view_vars[6] = (WORD)(*p_wc3_x_centre_hud / (float)GUI_WIDTH * width);
        p_view_vars[7] = (WORD)(*p_wc3_y_centre_hud / (float)GUI_HEIGHT * height);
        Debug_Info("Set Space View - HUD");
        Debug_Info("centre_x=%d, centre_y=%d, new_centre_x=%d, new_centre_y=%d", *p_wc3_x_centre_hud, *p_wc3_y_centre_hud, p_view_vars[6], p_view_vars[7]);
        break;
    default:
        Debug_Info("Set Space View - Unknown? num:%d", cockpit_view_type);
        break;
    }

    surface_space2D->ScaleToScreen(scale_type);
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
    is_cockpit_view = FALSE;

    WORD width = (WORD)clientWidth;
    WORD height = (WORD)clientHeight;

    //rear view vdu screen is 122*100, check for it here to prevent it being resized.
    if (p_db && p_db->rc.right == 121 && p_db->rc.bottom == 99) {
        width = 122;
        height = 100;
        //if in cockpit view, make sure flag is set to enable clipping rect.
        if (*p_wc3_space_view_type == SPACE_VIEW_TYPE::Cockpit)
            is_cockpit_view = TRUE;
    }
    p_view_vars[4] = width;
    p_view_vars[5] = height;

    p_view_vars[6] = width / 2;
    p_view_vars[7] = height / 2;

    surface_space2D->ScaleToScreen(SCALE_TYPE::fit);
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


//For storing the general buffer and dimensions, that many functions draw too.
//So that we can switch them with the DX11 buffer for drawing 3D space.
DRAW_BUFFER db_3d_backup = { 0 };
BYTE* pbuffer_space_3D = nullptr;


//________________________________
static void Lock_3DSpace_Surface() {

    if (pbuffer_space_3D != nullptr) {
        Debug_Info("Lock_3DSpace_Surface - buffer already locked!!!");
        return;
    }
    LONG buffer_space_3D_pitch = 0;

    if (surface_space3D->Lock((VOID**)&pbuffer_space_3D, &buffer_space_3D_pitch) != S_OK) {
        Debug_Info("Lock_3DSpace_Surface - lock failed!!!");
        return;
    }

    //backup current buffer data
    db_3d_backup.rc_inv.right = (**pp_wc3_db_game_main).rc.right;
    db_3d_backup.rc_inv.bottom = (**pp_wc3_db_game_main).rc.bottom;
    db_3d_backup.rc_inv.left = (**pp_wc3_db_game).rc_inv.left;
    db_3d_backup.rc_inv.top = (**pp_wc3_db_game).rc_inv.top;
    db_3d_backup.buff = (*pp_wc3_db_game)->buff;

    //set buffer for drawing 3d space elements
    (**pp_wc3_db_game).buff = pbuffer_space_3D;

    (**pp_wc3_db_game_main).rc.right = clientWidth - 1;
    (**pp_wc3_db_game_main).rc.bottom = clientHeight - 1;

    (**pp_wc3_db_game).rc_inv.left = buffer_space_3D_pitch - 1;
    (**pp_wc3_db_game).rc_inv.top = clientHeight - 1;

}


//_________________________________________________________
static void Lock_3DSpace_Surface_POV1(void* p_space_class) {

    if (((WORD*)p_space_class)[4] != (WORD)clientWidth || ((WORD*)p_space_class)[5] != (WORD)clientHeight) {
        //Debug_Info("RESIZING SPACE VIEW POV1");
        DWORD* p_cockpit_class = ((DWORD**)p_space_class)[67];
        //Debug_Info("RESIZING SPACE VIEW POV1: %d", p_cockpit_class[8]);
        Set_Space_View_POV1(p_space_class);
    }
    Lock_3DSpace_Surface();
}


//___________________________________________________________
static void __declspec(naked) lock_3dspace_surface_pov1(void) {

    __asm {
        //push edx
        push ebx
        push esi

        push esi
        call Lock_3DSpace_Surface_POV1
        add esp, 0x4

        pop esi
        pop ebx
        //pop edx
        ret
    }
}


//_________________________________________________________
static void Lock_3DSpace_Surface_POV3(void* p_space_struct) {
    Debug_Info("Lock_3DSpace_Surface_POV3");
    if (((WORD*)p_space_struct)[4] != (WORD)clientWidth || ((WORD*)p_space_struct)[5] != (WORD)clientHeight) {
        Debug_Info("RESIZING SPACE VIEW POV3");
        Set_Space_View_POV3((WORD*)p_space_struct, nullptr);
    }
    Lock_3DSpace_Surface();
}


//___________________________________________________________
static void __declspec(naked) lock_3dspace_surface_pov3(void) {

    __asm {
        //push edx
        push ebx
        push ebp
        push esi
        push edi

        push edi
        call Lock_3DSpace_Surface_POV3
        add esp, 0x4

        pop edi
        pop esi
        pop ebp
        pop ebx
        //pop edx
        ret
    }
}


//__________________________________
static void UnLock_3DSpace_Surface() {

    if (!pbuffer_space_3D) {
        Debug_Info("UnLock_3DSpace_Surface - buffer wasn't locked!!!");
        return;
    }
    surface_space3D->Unlock();
    pbuffer_space_3D = nullptr;


    //restore backup buffer data
    (**pp_wc3_db_game).buff = db_3d_backup.buff;

    (**pp_wc3_db_game_main).rc.right = db_3d_backup.rc_inv.right;
    (**pp_wc3_db_game_main).rc.bottom = db_3d_backup.rc_inv.bottom;

    (**pp_wc3_db_game).rc_inv.left = db_3d_backup.rc_inv.left;
    (**pp_wc3_db_game).rc_inv.top = db_3d_backup.rc_inv.top;

}


//For storing the general buffer and dimensions, that many functions draw too.
//So that we can switch them with the DX11 buffer for drawing 2D space.
DRAW_BUFFER db_2d_backup = { 0 };
BYTE* pbuffer_space_2D = nullptr;
LONG buffer_space_2D_pitch = 0;

//________________________________
static void Lock_2DSpace_Surface() {
    //Debug_Info("Lock_2DSpace_Surface");
    if (pbuffer_space_2D != nullptr) {
        Debug_Info("Lock_2DSpace_Surface - buffer already locked!!!");
        return;
    }

    if (surface_space2D->Lock((VOID**)&pbuffer_space_2D, &buffer_space_2D_pitch) != S_OK) {
        Debug_Info("Lock_2DSpace_Surface - lock failed!!!");
        return;
    }

    //backup current buffer data
    db_2d_backup.rc_inv.right = (**pp_wc3_db_game_main).rc.right;
    db_2d_backup.rc_inv.bottom = (**pp_wc3_db_game_main).rc.bottom;

    db_2d_backup.rc_inv.left = (**pp_wc3_db_game).rc_inv.left;
    db_2d_backup.rc_inv.top = (**pp_wc3_db_game).rc_inv.top;

    db_2d_backup.buff = (*pp_wc3_db_game)->buff;


    //set buffer for drawing 2d space elements
    (**pp_wc3_db_game).buff = pbuffer_space_2D;

    (**pp_wc3_db_game_main).rc.right = GUI_WIDTH - 1;
    (**pp_wc3_db_game_main).rc.bottom = GUI_HEIGHT - 1;

    (**pp_wc3_db_game).rc_inv.left = buffer_space_2D_pitch - 1;
    (**pp_wc3_db_game).rc_inv.top = GUI_HEIGHT - 1;

}


//__________________________________
static void UnLock_2DSpace_Surface() {
    //Debug_Info("UnLock_2DSpace_Surface");
    if (!pbuffer_space_2D) {
        Debug_Info("UnLock_2DSpace_Surface - buffer wasn't locked!!!");
        return;
    }
    surface_space2D->Unlock();
    pbuffer_space_2D = nullptr;


    //restore backup buffer data
    (**pp_wc3_db_game).buff = db_2d_backup.buff;

    (**pp_wc3_db_game_main).rc.right = db_2d_backup.rc_inv.right;
    (**pp_wc3_db_game_main).rc.bottom = db_2d_backup.rc_inv.bottom;

    (**pp_wc3_db_game).rc_inv.left = db_2d_backup.rc_inv.left;
    (**pp_wc3_db_game).rc_inv.top = db_2d_backup.rc_inv.top;
}


//_________________________________
static void Clear_2DSpace_Surface() {
    if (pbuffer_space_2D == nullptr)
        return;

    memset(pbuffer_space_2D, 0xFF, buffer_space_2D_pitch * GUI_HEIGHT);
    //memset(pbuffer_space_2D, 0x00, buffer_space_2D_pitch * GUI_HEIGHT);

}


//_____________________________________________________________________________
static void __declspec(naked) unlock_3dspace_surface_lock_2dspace_surface(void) {

    __asm {
        push ebx
        push esi

        call UnLock_3DSpace_Surface
        call Lock_2DSpace_Surface
        call Clear_2DSpace_Surface

        pop esi
        pop ebx
        ret
    }
}


//____________________________________________________________________
static void __declspec(naked) unlock_2dspace_surface_and_display(void) {

    __asm {
        push ebx
        push ebp
        push esi

        call UnLock_2DSpace_Surface
        call Display_Space_Scene

        pop esi
        pop ebp
        pop ebx
        ret
    }
}


//__________________________________________________________________________________
static void __declspec(naked) unlock_3dspace_surface_lock_2dspace_surface_pov3(void) {

    __asm {
        push ecx
        push ebx
        push ebp
        push esi

        call UnLock_3DSpace_Surface
        call Lock_2DSpace_Surface
        call Clear_2DSpace_Surface

        pop esi
        pop ebp
        pop ebx
        pop ecx

        call wc3_draw_hud_view_text
        ret
    }
}


//_____________________________________
static void Fix_CockpitViewTargetRect() {

    if (!surface_space2D)
        return;
    float x_unit = 0;
    float y_unit = 0;
    float x = 0;
    float y = 0;
    surface_space2D->GetPosition(&x, &y);
    surface_space2D->GetScaledPixelDimensions(&x_unit, &y_unit);
    (**pp_wc3_db_game_main).rc.left = (LONG)((**pp_wc3_db_game_main).rc.left * x_unit + x);
    (**pp_wc3_db_game_main).rc.top = (LONG)((**pp_wc3_db_game_main).rc.top * y_unit + y);
    (**pp_wc3_db_game_main).rc.right = (LONG)((**pp_wc3_db_game_main).rc.right * x_unit + x);
    (**pp_wc3_db_game_main).rc.bottom = (LONG)((**pp_wc3_db_game_main).rc.bottom * y_unit + y);
}


//______________________________________________________________
static void __declspec(naked) fix_cockpit_view_target_rect(void) {

    __asm {
        push esi
        call Fix_CockpitViewTargetRect
        pop esi
        mov ecx, pp_wc3_db_game_main
        mov ecx, dword ptr ds : [ecx]
        ret
    }
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
        call Lock_3DSpace_Surface
        pop ecx

        call wc3_draw_hud_targeting_elements

        call UnLock_3DSpace_Surface

        pop edi
        pop esi
        pop ebp
        pop ebx
        pop edx
        ret
    }
}


//______________________________________________________
static void __declspec(naked) fix_nav_scrn_display(void) {

    __asm {

        push ebp
        push edi

        push ecx

        call Lock_3DSpace_Surface

        pop ecx
        call wc3_nav_screen

        call UnLock_3DSpace_Surface

        pop edi
        pop ebp
        ret
    }
}

//___________________________________________________________________
static void __declspec(naked) nav_unlock_3d_and_lock_2d_drawing(void) {

    __asm {
        push eax
        push ecx
        push ebx
        push esi

        call UnLock_3DSpace_Surface
        call Lock_2DSpace_Surface
        call Clear_2DSpace_Surface

        pop esi
        pop ebx
        pop ecx
        pop eax

        mov ebp, dword ptr ds : [eax + 0x8]
        mov edx, dword ptr ds : [eax + 0xC]
        ret
    }
}


//_____________________________________________________________________
static void __declspec(naked) nav_unlock_2d_and_display_relock_3d(void) {

    __asm {
        push ebx
        push esi

        call UnLock_2DSpace_Surface
        mov is_nav_view, 1
        call Display_Space_Scene
        mov is_nav_view, 0
        call Lock_3DSpace_Surface

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


//_______________________________________________
static void Set_WindowActive_State(BOOL isActive) {
    SetWindowActivation(isActive);
}



//return false to call DefWindowProc
//_____________________________________________________________________________
static bool WinProc_Main(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {

    static bool is_cursor_hidden = true;
    static bool is_in_sizemove = false;

    switch (Message) {
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

        if (!(winpos->flags & (SWP_NOSIZE)) && !is_in_sizemove) {
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
            SetWindowTitle(hwnd, L"");
        }
        
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
            if (!*p_wc3_is_windowed) {
                if (ConfigReadInt("MAIN", "WINDOWED", CONFIG_MAIN_WINDOWED))
                    *p_wc3_is_windowed = true;
            }
            Window_Setup(hwnd, *p_wc3_is_windowed);
            SetWindowTitle(hwnd, L"");
            ShowWindow(hwnd, SW_SHOW);
            return 0;
            break;
        case 40004:
            wc3_unknown_func01();
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
        cmp eax, WM_LBUTTONDBLCLK
            jne check4
            mov cl, 2
            ret
            check4 :
        cmp eax, WM_RBUTTONDBLCLK
            jne check5
            mov cl, 3
            ret
            check5 :
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


//______________________________________________________
static void __declspec(naked) check_no_full_screen(void) {

    __asm {
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


//______________________________________________________________________________________
static LRESULT Update_Mouse_State(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {

    switch (Message) {
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP: {
        *p_wc3_mouse_button = 0;
        if (wParam & MK_LBUTTON)
            *p_wc3_mouse_button |= 0x1;
        if (wParam & MK_RBUTTON)
            *p_wc3_mouse_button |= 0x2;
        if (wParam & MK_MBUTTON)
            *p_wc3_mouse_button |= 0x4;
        *p_mouse_button_true = *p_wc3_mouse_button;

        LONG x = GET_X_LPARAM(lParam);
        LONG y = GET_Y_LPARAM(lParam);

        *p_mouse_x_true = (WORD)x;
        *p_mouse_y_true = (WORD)y;
        if (surface_gui) {
            float fx = 0;
            float fy = 0;
            surface_gui->GetPosition(&fx, &fy);
            x = (LONG)((x - fx) * GUI_WIDTH / surface_gui->GetDisplayWidth());
            y = (LONG)((y - fy) * GUI_HEIGHT / surface_gui->GetDisplayHeight());
        }
        else {
            x = x * GUI_WIDTH / clientWidth;
            y = y * GUI_HEIGHT / clientHeight;
        }

        *p_wc3_mouse_x = (WORD)x;
        *p_wc3_mouse_y = (WORD)y;

        break;
    }
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
                fwidth = surface_gui->GetDisplayWidth();
                fheight = surface_gui->GetDisplayHeight();
            }

            fx += x * fwidth / GUI_WIDTH;
            LONG ix = (LONG)fx;
            if ((float)ix != fx)
                ix++;
            *p_mouse_x_true = (WORD)ix;
            ix += client.x;

            fy += y * fheight / GUI_HEIGHT;
            LONG iy = (LONG)fy;
            if ((float)iy != fy)
                iy++;
            *p_mouse_y_true = (WORD)iy;
            iy += client.y;

            SetCursorPos(ix, iy);
        }

        /*if (x < 0)
            x = 0;
        else if (x > GUI_WIDTH)
            x = GUI_WIDTH;
        if (y < 0)
            y = 0;
        else if (y > GUI_HEIGHT)
            y = GUI_HEIGHT;*/
        *p_wc3_mouse_x = (WORD)x;
        *p_wc3_mouse_y = (WORD)y;
    }
    return *p_wc3_is_mouse_present;
}


// Replaces a function which was moving the cursor when it strayed beyond the client rect.
// This function allows the mouse to move freely as well as update it's position when outside the client rect.
//________________________________________________
static BOOL Update_Cursor_Position(LONG x, LONG y) {

    if (*p_wc3_is_mouse_present) {
        POINT p{ 0,0 };
        if (ClientToScreen(*p_wc3_hWinMain, &p)) {
            POINT m{ 0,0 };
            GetCursorPos(&m);

            x = (m.x - p.x);
            y = (m.y - p.y);

            *p_mouse_x_true = (WORD)x;
            *p_mouse_y_true = (WORD)y;

            if (surface_gui) {
                float fx = 0;
                float fy = 0;
                surface_gui->GetPosition(&fx, &fy);
                x = (LONG)((x - fx) * GUI_WIDTH / surface_gui->GetDisplayWidth());
                y = (LONG)((y - fy) * GUI_HEIGHT / surface_gui->GetDisplayHeight());
            }
            else {
                x = x * GUI_WIDTH / clientWidth;
                y = y * GUI_HEIGHT / clientHeight;
            }
        }
        /*if (x < 0)
            x = 0;
        else if (x > GUI_WIDTH)
            x = GUI_WIDTH;
        if (y < 0)
            y = 0;
        else if (y > GUI_HEIGHT)
            y = GUI_HEIGHT;*/

        *p_wc3_mouse_x = (WORD)x;
        *p_wc3_mouse_y = (WORD)y;
    }
    return *p_wc3_is_mouse_present;
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
    return p_mouse_button_true;
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


//________________________________________________________
static void __declspec(naked) fix_movement_diamond_x(void) {

    __asm {
        mov esi, clientWidth
        ret
    }
}


//________________________________________________________
static void __declspec(naked) fix_movement_diamond_y(void) {

    __asm {
        mov esi, clientHeight
        ret
    }
}


//____________________________________________________
static void __declspec(naked) update_space_mouse(void) {

    __asm {
        mov eax, p_mouse_button_true
        mov edx, p_wc3_mouse_button_space
        ret
    }
}


//__________________________________________________________________________________________________
static BOOL Play_Movie_Sequence(void* p_wc3_movie_class, void* p_sig_movie_class, DWORD sig_movie_flags) {

    char* mve_path = (char*)((DWORD*)p_wc3_movie_class)[28];
    //Debug_Info("Play_Movie_Loop:  sig_movie_flags:%X", sig_movie_flags);
    Debug_Info("Play_Movie_Sequence: main_path: %s", mve_path);
    Debug_Info("Play_Movie_Sequence: current_list_num: %d", *p_wc3_movie_branch_current_list_num);
    Debug_Info("Play_Movie_Sequence: first branch: %d", *p_wc3_movie_branch_list);
    //Debug_Info("max branches:%d", ((LONG*)p_wc3_movie_class)[21]);

    MovieRT_Clear();

    if (pMovie_vlc)
        delete pMovie_vlc;
    std::string movie_name;
    Get_Movie_Name_From_Path(mve_path, &movie_name);
    pMovie_vlc = new LibVlc_Movie(movie_name, p_wc3_movie_branch_list, *p_wc3_movie_branch_current_list_num);

    BOOL exit_flag = FALSE;
    BOOL play_successfull = FALSE;
    MOVIE_STATE movie_state{ 0 };

    if (pMovie_vlc->Play()) {
        play_successfull = TRUE;
        while (!exit_flag) {
            wc3_translate_messages_keys();
            wc3_movie_update_joystick_double_click_exit();
            exit_flag = wc3_movie_exit();

            if (!pMovie_vlc->IsPlaying(&movie_state)) {
                *p_wc3_movie_branch_current_list_num = movie_state.list_num;
                if (!movie_state.hasPlayed) {
                    Debug_Info("Play_Movie_Sequence: ended BAD, branch:%d, listnum:%d", movie_state.branch, movie_state.list_num);
                    //if branch failed to play, shift to the current branch position so the rest of the movie can be played out using the original player.
                    if (wc3_movie_set_position(p_wc3_movie_class, p_wc3_movie_branch_list[movie_state.list_num]) == FALSE)
                        Debug_Info("Play_Movie_Sequence: wc3_movie_set_position Failed, branch:%d", p_wc3_movie_branch_list[movie_state.list_num]);
                    play_successfull = FALSE;
                }
                else
                    Debug_Info("Play_Movie_Sequence: ended OK, branch:%d, listnum:%d", movie_state.branch, movie_state.list_num);

                exit_flag = TRUE;
            }
        }
    }
    //if alternate movie failed to play, continue movie using original player.
    if (!play_successfull)
        play_successfull = wc3_sig_movie_play_sequence(p_sig_movie_class, sig_movie_flags);
    
    delete pMovie_vlc;
    pMovie_vlc = nullptr;

    Debug_Info("Play_Movie_Sequence: Done");
    return play_successfull;
}


//_____________________________________________________
static void __declspec(naked) play_movie_sequence(void) {

    __asm {
        push ebx
        push ebp

        push [esp+0xC]//sig movie flags? 0x7FFFFFFF
        push ecx
        push ebp
        call Play_Movie_Sequence
        add esp, 0xC

        pop ebp
        pop ebx

        ret 0x4
    }
}


//Finds the number of frames between two SMPTE timecode's, these timecodes are 30 fps.
//SMPTE timecode format hh/mm/ss/frames 30fps
//________________________________________________________________________________________
static LONG Get_Num_Frames_Between_Timecodes_30fps(DWORD timecode_start, DWORD timecode_end) {

    DWORD temp = 0;
    DWORD seconds_30fps = 0;
    DWORD minuts_30fps = 0;
    DWORD hours_30fps = 0;

    DWORD start = timecode_start % 100;
    temp = timecode_start / 100;

    seconds_30fps = temp % 100;
    seconds_30fps *= 30;

    temp /= 100;
    minuts_30fps = temp % 100;
    minuts_30fps *= 1800;

    temp /= 100;
    hours_30fps = temp % 100;
    hours_30fps *= 108000;


    
    start = start + seconds_30fps + minuts_30fps + hours_30fps;
    //Debug_Info("Get_Time_Position: start frames: %d", start);

    DWORD end = timecode_end % 100;
    temp = timecode_end / 100;

    seconds_30fps = temp % 100;
    seconds_30fps *= 30;

    temp /= 100;
    minuts_30fps = temp % 100;
    minuts_30fps *= 1800;

    temp /= 100;
    hours_30fps = temp % 100;
    hours_30fps *= 108000;

    end = end + seconds_30fps + minuts_30fps + hours_30fps;
    //Debug_Info("Get_Time_Position: end frames: %d", end);
    if (end < start)
        return 0;

    return (end - start);
}


//___________________________________________________________
static BOOL Play_Inflight_Movie(HUD_CLASS_01* p_hud_class_01) {
    if (*pp_movie_class_inflight_01) {
        static LARGE_INTEGER inflight_audio_play_start_time{0};
        static LARGE_INTEGER inflight_audio_play_start_offset{0};

        if (!(*p_wc3_inflight_draw_buff).buff && !pMovie_vlc_Inflight) {
            Debug_Info("Play_Inflight_Movie: timecodes: timecode_start_of_file:%d, timecode_start_of_scene:%d, timecode_duration:%d, scene_video_neg_frame_offset:%d", (*pp_movie_class_inflight_01)->timecode_start_of_file_30fps, (*pp_movie_class_inflight_01)->timecode_start_of_audio_30fps, (*pp_movie_class_inflight_01)->timecode_length_from_video_start_30fps, (*pp_movie_class_inflight_01)->video_frame_offset_15fps_neg);
            
            //SMPTE timecode's are used to set the play position in some movie's, seems to be mostly the pilot head's.
            LONG sound_start_frame = Get_Num_Frames_Between_Timecodes_30fps((*pp_movie_class_inflight_01)->timecode_start_of_file_30fps, (*pp_movie_class_inflight_01)->timecode_start_of_audio_30fps);
            if (sound_start_frame < 0)
                sound_start_frame = 0;
            inflight_audio_play_start_offset.QuadPart = static_cast<libvlc_time_t>(sound_start_frame) * 1000 / 30;

            LONG video_start_frame = sound_start_frame - (*pp_movie_class_inflight_01)->video_frame_offset_15fps_neg * 2;
            if (video_start_frame < 0)
                video_start_frame = 0;

            LONG end = Get_Num_Frames_Between_Timecodes_30fps(0, (*pp_movie_class_inflight_01)->timecode_length_from_video_start_30fps) + video_start_frame;
            
            Debug_Info("Play_Inflight_Movie: frames30fps: start:%d,sound_start_frame:%d, end:%d", video_start_frame, sound_start_frame, end);
            Debug_Info("Play_Inflight_Movie:       times: start:%d, sound_start_frame:%d, end:%d", video_start_frame * 1000 / 30, (sound_start_frame) * 1000 / 30, (end) * 1000 / 30);

            RECT rc_dest{ p_hud_class_01->hud_x + p_hud_class_01->comm_x, p_hud_class_01->hud_y + p_hud_class_01->comm_y,  p_hud_class_01->hud_x + p_hud_class_01->comm_x + (LONG)p_movie_class_inflight_02->width - 1, p_hud_class_01->hud_y + p_hud_class_01->comm_y + (LONG)p_movie_class_inflight_02->height - 1 };
            Debug_Info("size:%d,%d,%d,%d", rc_dest.left, rc_dest.top, rc_dest.right, rc_dest.bottom);
            pMovie_vlc_Inflight = new LibVlc_MovieInflight((*pp_movie_class_inflight_01)->file_name, &rc_dest, static_cast<libvlc_time_t>(video_start_frame) * 1000 / 30, static_cast<libvlc_time_t>(end) * 1000 / 30);
            if (!pMovie_vlc_Inflight->Play()) {
                delete pMovie_vlc_Inflight;
                pMovie_vlc_Inflight = nullptr;
                return FALSE;
            }

            //(*p_wc3_inflight_draw_buff).buff needs to exist to evoke the inflight movie destructor function.
            if (!(*p_wc3_inflight_draw_buff).buff) {
                (*p_wc3_inflight_draw_buff).buff = (BYTE*)wc3_allocate_mem_main(p_movie_class_inflight_02->width * p_movie_class_inflight_02->height);
                (*p_wc3_inflight_draw_buff).rc_inv.left = (LONG)p_movie_class_inflight_02->width - 1;
                (*p_wc3_inflight_draw_buff).rc_inv.top = (LONG)p_movie_class_inflight_02->height - 1;
                (*p_wc3_inflight_draw_buff).rc_inv.right = 0;
                (*p_wc3_inflight_draw_buff).rc_inv.bottom = 0;

                (*p_wc3_inflight_draw_buff_main).db = p_wc3_inflight_draw_buff;
                (*p_wc3_inflight_draw_buff_main).rc.left = 0;
                (*p_wc3_inflight_draw_buff_main).rc.top = 0;
                (*p_wc3_inflight_draw_buff_main).rc.right = (LONG)p_movie_class_inflight_02->width - 1;
                (*p_wc3_inflight_draw_buff_main).rc.bottom = (LONG)p_movie_class_inflight_02->height - 1;
            }

            inflight_audio_play_start_time.QuadPart = 0;
        }
        //Debug_Info("Process_Inflight_Movie MVE: current frame:%d", p_movie_class_inflight_02->current_frame);
        //Debug_Info("Process_Inflight_Movie MVE: time count:%d,strt vid:%d,end vid:%d,strt aud:%d,curr:%d", *(DWORD**)0x4AB28C, *(DWORD**)0x4AB338, *(DWORD**)0x4AB2C8, *(DWORD**)0x4AB33C, *(DWORD**)0x4AB354);
        if (!pMovie_vlc_Inflight)
            return FALSE;
        
        libvlc_time_t current_time = pMovie_vlc_Inflight->Check_Play_Time();
        //Debug_Info("Play_Inflight_Movie: current time :%d ms", (int)current_time);

        if (!pMovie_vlc_Inflight->IsPlayInitialised()) {
            //Debug_Info("Play_Inflight_Movie: waiting for movie initialisation");
            return TRUE;
        }
        
        // timecode_start_of_audio_30fps is used as a flag to initiate audio playback.
        // setting this here once playback is initialised and audio start time reached.
        if ((*pp_movie_class_inflight_01)->timecode_start_of_audio_30fps != 0) {
            LARGE_INTEGER inflight_video_play_time{};
            QueryPerformanceCounter(&inflight_video_play_time);
            if (inflight_audio_play_start_time.QuadPart == 0)
                inflight_audio_play_start_time.QuadPart = inflight_video_play_time.QuadPart + inflight_audio_play_start_offset.QuadPart;
            if (inflight_audio_play_start_time.QuadPart < inflight_video_play_time.QuadPart)
                (*pp_movie_class_inflight_01)->timecode_start_of_audio_30fps = 0;
        }

        //check if video display dimensions have changed and update if necessary.
        RECT rc_dest{ p_hud_class_01->hud_x + p_hud_class_01->comm_x, p_hud_class_01->hud_y + p_hud_class_01->comm_y,  p_hud_class_01->hud_x + p_hud_class_01->comm_x + (LONG)p_movie_class_inflight_02->width - 1, p_hud_class_01->hud_y + p_hud_class_01->comm_y + (LONG)p_movie_class_inflight_02->height - 1 };
        pMovie_vlc_Inflight->Update_Display_Dimensions(&rc_dest);

        //set the movie class pointer to null to signal to wc3 that the movie has ended.
        if (pMovie_vlc_Inflight && pMovie_vlc_Inflight->HasPlayed()) {
            *pp_movie_class_inflight_01 = nullptr;
            Debug_Info("Play_Inflight_Movie: Movie Finished");
            return TRUE;
        }

        //clear the rect on the cockpit/hud so that the movie drawn beneath will be visible.
        wc3_copy_rect(p_wc3_inflight_draw_buff_main, 0, 0, *pp_wc3_db_game_main, p_hud_class_01->hud_x + p_hud_class_01->comm_x, p_hud_class_01->hud_y + p_hud_class_01->comm_y, (BYTE)255);
    }
    return TRUE;
}


//_____________________________________________________
static void __declspec(naked) play_inflight_movie(void) {

    __asm {
        push ebx
        push ecx
        push edx
        push edi
        push esi
        push ebp

        push esi
        call Play_Inflight_Movie
        add esp, 0x4

        pop ebp
        pop esi
        pop edi
        pop edx
        pop ecx
        pop ebx

        cmp eax, FALSE
        je play_mve

        pop eax //pop ret address and skip over regular mve playback code 
        jmp p_wc3_play_inflight_hr_movie_return_address
        
        play_mve :
        mov eax, p_wc3_inflight_draw_buff
        cmp dword ptr ds:[eax], 0
        ret
    }
}


//_________________________________
static void Inflight_Movie_Unload() {
    //check if the finished hd movie has audio, and if so return the volume of the background music to normal setting.
    if (pMovie_vlc_Inflight) {
        if (pMovie_vlc_Inflight->HasAudio()) {
            Debug_Info("Inflight_Movie_Unload HasAudio - volume returned to normal");
            wc3_set_music_volume(p_wc3_audio_class, *p_wc3_ambient_music_volume);
        }
        
        delete pMovie_vlc_Inflight;
        pMovie_vlc_Inflight = nullptr;
        Debug_Info("Inflight_Movie_Unload done");
    }
}


//_______________________________________________________
static void __declspec(naked) inflight_movie_unload(void) {

    __asm {
        pushad

        call Inflight_Movie_Unload

        popad

        mov eax, p_wc3_inflight_draw_buff
        cmp dword ptr ds:[eax] , ebx
        ret
    }
}


//______________________________________
static LONG Inflight_Movie_Audio_Check() {
    //check if the hd movie has audio, and if so lower the volume of the background music while playing.
    if (*p_wc3_inflight_audio_ref == 0 && pMovie_vlc_Inflight && pMovie_vlc_Inflight->HasAudio()) {
        Debug_Info("Inflight_Movie_Check_Audio HasAudio - volume lowered");
        LONG audio_vol = *p_wc3_ambient_music_volume - 4;
        if (audio_vol < 0)
            audio_vol = 0;
        wc3_set_music_volume(p_wc3_audio_class, audio_vol);
        return FALSE;

    }
    return *p_wc3_inflight_audio_unk01;
}


//____________________________________________________________
static void __declspec(naked) inflight_movie_audio_check(void) {

    __asm {
        push eax
        
        push ebx
        push ecx
        push edx
        push edi
        push esi
        push ebp

        call Inflight_Movie_Audio_Check

        pop ebp
        pop esi
        pop edi
        pop edx
        pop ecx
        pop ebx

        cmp al, 0

        pop eax
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

    MemWrite16(0x431390, 0x4C8B, 0xE990);
    FuncWrite32(0x431392, 0xEC830824, (DWORD)&ColourFill);

    MemWrite16(0x41BB80, 0xEC83, 0x9090);
    MemWrite8(0x41BB82, 0x6C, 0x90);
    MemWrite8(0x41BB83, 0xA1, 0xE9);
    FuncWrite32(0x41BB84, 0x4A329C, (DWORD)&LockSurface);

    MemWrite16(0x41BBD0, 0xEC81, 0xE990);
    FuncWrite32(0x41BBD2, 0x84, (DWORD)&UnlockShowSurface);

    //in draw movie func, jump over direct draw stuff
    MemWrite8(0x41B9AD, 0x74, 0xEB);

    //prevent direct draw - create surface func call
    MemWrite16(0x4313D0, 0xEC83, 0xC033);//xor eax, eax
    MemWrite8(0x4313D2, 0x6C, 0xC3);

    //replace direct draw - blit func
    MemWrite16(0x405890, 0x3D80, 0xE990);
    FuncWrite32(0x405892, 0x49F984, (DWORD)&DXBlt);

    //prevent direct draw - palette update func call
    MemWrite8(0x431360, 0x83, 0xC3);

    //replace direct draw - draw movie frame func
    MemWrite8(0x444850, 0x83, 0xE9);
    FuncWrite32(0x444851, 0x565304EC, (DWORD)&DrawVideoFrame);

    //Keep 320 movie width - prevent scale to 640
    MemWrite32(0x41C3E7, 0x01, 0x0);

    //in draw movie func
    FuncReplace32(0x41BA23, 0xFFFE90F9, (DWORD)&UnlockShowMovieSurface);

    //in draw movie frame func
    // shouldnt ever be called
   // FuncReplace32(0x41CA0A, 0xFFFE8112, (DWORD)&UnlockShowMovieSurface);
    // shouldnt ever be called
   // FuncReplace32(0x41CA46, 0xFFFE80D6, (DWORD)&UnlockShowMovieSurface);

    //in draw movie frame func
    FuncReplace32(0x41CAEB, 0xFFFE8031, (DWORD)&UnlockShowMovieSurface);
    FuncReplace32(0x41CB8D, 0xFFFE7F8F, (DWORD)&UnlockShowMovieSurface);
    
    //for drawing subtitles - dont know much about these
    FuncReplace32(0x41D4DB, 0xFFFE7641, (DWORD)&UnlockShowMovieSurface);
    FuncReplace32(0x41D8F0, 0xFFFE722C, (DWORD)&UnlockShowMovieSurface);
    FuncReplace32(0x41DA10, 0xFFFE710C, (DWORD)&UnlockShowMovieSurface);

    //replace blit function for movies
    FuncReplace32(0x4147D4, 0xFFFF10B8, (DWORD)&DXBlt_Movie);
    FuncReplace32(0x4147EC, 0xFFFF10A0, (DWORD)&DXBlt_Movie);
    FuncReplace32(0x414A40, 0xFFFF0E4C, (DWORD)&DXBlt_Movie);
    FuncReplace32(0x414A58, 0xFFFF0E34, (DWORD)&DXBlt_Movie);

    //replace blit function for draw choice text
    FuncReplace32(0x414CBC, 0xFFFF0BD0, (DWORD)&DXBlt_Movie);
    FuncReplace32(0x414CD4, 0xFFFF0BB8, (DWORD)&DXBlt_Movie);

    //replace space first person view setup function
    MemWrite8(0x425B40, 0x8B, 0xE9);
    FuncWrite32(0x425B41, 0x8B042454, (DWORD)&set_space_view_pov1);
    //MemWrite16(0x425B45, 0x0C42, 0x9090);

    //replace direct draw lock surface in draw space first person view function
    MemWrite16(0x425D22, 0x6774, 0x9090);//prevent jumping before this is called

    MemWrite16(0x425D24, 0x3D83, 0xE890);
    FuncWrite32(0x425D26, 0x004A3298, (DWORD)&lock_3dspace_surface_pov1);
    MemWrite8(0x425D2A, 0x00, 0x90);
    MemWrite8(0x425D2B, 0x74, 0xEB);//jmp over ddraw stuff

    //replace direct draw stuff in draw space first person view function - unlock 3d space surface then lock 2d surface for hud etc.
    MemWrite16(0x425E1F, 0x840F, 0x9090);//prevent jumping before this is called
    MemWrite32(0x425E21, 0x000000F9, 0x90909090);

    MemWrite16(0x425E25, 0x3D83, 0xE890);
    FuncWrite32(0x425E27, 0x004A3298, (DWORD)&unlock_3dspace_surface_lock_2dspace_surface);
    MemWrite8(0x425E2B, 0x00, 0x90);
    MemWrite16(0x425E2C, 0x840F, 0xE990);//jmp over ddraw stuff

    //replace direct draw stuff in draw space first person view function - unlock 2d surface then display.
    FuncReplace32(0x425F75, 0xFFFDEBA7, (DWORD)&unlock_2dspace_surface_and_display);

    //draw targeting elements to 3d space
    FuncReplace32(0x41B1A8, 0x00041614, (DWORD)&draw_hud_targeting_elements);

    //replace direct draw lock surface in draw space third person view function
    MemWrite16(0x42EB47, 0x6074, 0x9090);//prevent jumping before this is called

    MemWrite16(0x42EB49, 0x3D83, 0xE890);
    FuncWrite32(0x42EB4B, 0x004A3298, (DWORD)&lock_3dspace_surface_pov3);
    MemWrite8(0x42EB4F, 0x00, 0x90);
    MemWrite8(0x42EB50, 0x74, 0xEB);//jmp over ddraw stuff

    //replace direct draw stuff in draw space third person view function - unlock 3d space surface then lock 2d surface for text etc.
    FuncReplace32(0x42EC3C, 0x0002A8C0, (DWORD)&unlock_3dspace_surface_lock_2dspace_surface_pov3);

    //replace direct draw stuff in draw space third person view function - unlock 2d surface then display.
    FuncReplace32(0x42EC65, 0xFFFD5EB7, (DWORD)&unlock_2dspace_surface_and_display);

    //replace space third person view setup function
    MemWrite8(0x48D050, 0x8B, 0xE9);
    FuncWrite32(0x48D051, 0x56042444, (DWORD)&set_space_view_pov3);

    //fix display rectangle for targeting elements
    MemWrite16(0x45C84B, 0x0D8B, 0xE890);
    FuncWrite32(0x45C84D, 0x49F97C, (DWORD)&fix_cockpit_view_target_rect);

    //draw nav screen space view to 3d surface, seperate from 2d elements
    FuncReplace32(0x445120, 0x2C, (DWORD)&fix_nav_scrn_display);

    //set 2d surface for drawing nav screen 2d elements
    MemWrite16(0x44530A, 0x688B, 0xE890);
    FuncWrite32(0x44530C, 0x0C508B08, (DWORD)&nav_unlock_3d_and_lock_2d_drawing);

    //set 3d surface after drawing nav screen 2d elements
    MemWrite16(0x44562F, 0x15FF, 0xE890);
    FuncWrite32(0x445631, 0x49F9BC, (DWORD)&nav_unlock_2d_and_display_relock_3d);

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

    //replaces original function for mouse window client position with scaled gui position.
    MemWrite8(0x483410, 0x8B, 0xE9);
    FuncWrite32(0x483411, 0x56082444, (DWORD)&Update_Mouse_State);


    //replaces original function for mouse window client position with scaled gui position.
    MemWrite8(0x483350, 0x83, 0xE9);
    FuncWrite32(0x483351, 0x54A108EC, (DWORD)&Set_Mouse_Position);
    MemWrite8(0x483355, 0x7E, 0x90);
    MemWrite16(0x483356, 0x004A, 0x9090);

    //replace the main window message checking function for greater functionality.
    MemWrite8(0x405090, 0x8B, 0xE9);
    FuncWrite32(0x405091, 0x53082444, (DWORD)&WinProc_Main);

    //Add  WM_ENTERSIZEMOVE and WM_EXITSIZEMOVE checks to movie message check
    //increase range of message code selection
    MemWrite32(0x41C73B, 0xF5, 0x121);
    //check for the messages
    MemWrite16(0x41C747, 0x888A, 0xE890);
    FuncWrite32(0x41C749, 0x41C8A0, (DWORD)&winproc_movie_message_check);

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

    //in space - return un-scaled position of mouse as space view isn't scaled
    FuncReplace32(0x4222C2, 0x06112A, (DWORD)&Translate_Messages_Mouse_ClipCursor_Space);

    //clip the cursor while a conversation decision is made.
    FuncReplace32(0x4825D4, 0xFFFFFF08, (DWORD)&Conversation_Decision_ClipCursor);

    //Enable cursor clipping for all WM_SETCURSOR messages within calls to this function, used during space flight.
    FuncReplace32(0x40F75F, 0xFFFF8C8D, (DWORD)&overide_cursor_clipping);
    FuncReplace32(0x430087, 0xFFFD8365, (DWORD)&overide_cursor_clipping);

    //ALT-O space settings screen.
    //Jump over the code that keeps the cursor on screen.
    MemWrite16(0x437A86, 0x7E83, 0x2EEB);
    MemWrite16(0x437A88, 0x0014, 0x9090);
    //update the cursor position
    FuncReplace32(0x437AD1, 0x04B87B, (DWORD)&Update_Cursor_Position);
    //set pointer to use general mouse pointer data instead of the space flight copy which is set at a different resolution.
    MemWrite32(0x4378BB, 0x4A9B92, (DWORD)0x4A7E5A);//p_wc3_mouse_x
    MemWrite32(0x4378C6, 0x4A9B94, (DWORD)0x4A7E5C);//p_wc3_mouse_y
    MemWrite32(0x4378CF, 0x4A9B90, (DWORD)0x4A7E58); //p_wc3_mouse_button

    //relate the movement diamond to new space view dimensions rather than original 640x480.
    MemWrite8(0x422277, 0xBE, 0xE8);
    FuncWrite32(0x422278, 0x280, (DWORD)&fix_movement_diamond_x);
    MemWrite8(0x42228A, 0xBE, 0xE8);
    FuncWrite32(0x42228B, 0x1E0, (DWORD)&fix_movement_diamond_y);

    //004213D5 | .E8 3A030500   CALL DRAW_IMAGE(*dib_struct, ) ? //draw crosshairs
    //MemWrite8(0x4213D5, 0xE8, 0x90);
    //MemWrite32(0x4213D6, 0x05033A, 0x90909090);

    //update mouse in space view
    MemWrite8(0x4083DA, 0xBA, 0xE8);
    FuncWrite32(0x4083DB, 0x4A9B90, (DWORD)&update_space_mouse);

    //fix control reaction speed on the nav screen.
    FuncReplace32(0x44526E, 0xFFFEB1EE, (DWORD)&nav_screen_movement_speed_fix);

    //play alternate hires movies
    FuncReplace32(0x41C59E, 0x03A1FE, (DWORD)&play_movie_sequence);

    //nav map movements
    //0043049D | .  81AE F8000000 00080000        SUB DWORD PTR DS : [ESI + 0F8] , 800
 //   MemWrite32(0x4304A3, 0x00000800, 32);
    //004304B0 | .  8186 F8000000 00080000        ADD DWORD PTR DS : [ESI + 0F8] , 800
//    MemWrite32(0x4304B6, 0x00000800, 32);
    //004304C3 | .  81AE FC000000 00080000        SUB DWORD PTR DS : [ESI + 0FC] , 800
 //   MemWrite32(0x4304C9, 0x00000800, 32);
    //004304D6 | .  8186 FC000000 00080000        ADD DWORD PTR DS : [ESI + 0FC] , 800
 //   MemWrite32(0x4304DC, 0x00000800, 32);


    MemWrite16(0x422EA5, 0x3D83, 0xE890);
    FuncWrite32(0x422EA7, 0x4AB318, (DWORD)&play_inflight_movie);
    MemWrite8(0x422EAB, 0x00, 0x90);

    MemWrite16(0x423085, 0x1D39, 0xE890);
    FuncWrite32(0x423087, 0x4AB318, (DWORD)&inflight_movie_unload);

    MemWrite16(0x433731, 0x3D80, 0xE890);
    FuncWrite32(0x433733, 0x4A3338, (DWORD)&inflight_movie_audio_check);
    MemWrite8(0x433737, 0x00, 0x90);

}


