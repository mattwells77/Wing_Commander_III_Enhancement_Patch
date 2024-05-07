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

#pragma once

#define GUI_WIDTH 640
#define GUI_HEIGHT 480

struct DRAW_BUFFER {
    BYTE* buff;
    RECT rc_inv;//rect inverted
};

struct DRAW_BUFFER_MAIN {
    DRAW_BUFFER* db;
    RECT rc;
};

struct VIDframe {
    DWORD* unknown00;//0x00
    DWORD unknown04;//0x04
    DWORD unknown08;//0x08
    DWORD width;//0x0C
    DWORD height;//0x10
    DWORD unknown14;//0x14
    DWORD bitFlag;//0x18
    DWORD unknown1C;//0x1C
    DWORD unknown20;//0x20
    DWORD unknown24;//0x24
    BYTE* buff;//0x28
};


void WC3W_Setup();

extern char* p_wc3_szAppName;

extern bool* p_wc3_window_has_focus;
extern bool* p_wc3_is_windowed;
extern HWND* p_wc3_hWinMain;
extern LONG* p_wc3_main_surface_pitch;

extern DRAW_BUFFER** pp_wc3_db_game;
extern DRAW_BUFFER_MAIN** pp_wc3_db_game_main;

extern BITMAPINFO** pp_wc3_DIB_Bitmapinfo;
extern VOID** pp_wc3_DIB_vBits;


extern LARGE_INTEGER* p_wc3_frequency;
extern LARGE_INTEGER* p_wc3_space_time_max;


extern LONG* p_wc3_x_centre_cockpit;
extern LONG* p_wc3_y_centre_cockpit;
extern LONG* p_wc3_x_centre_rear;
extern LONG* p_wc3_y_centre_rear;
extern LONG* p_wc3_x_centre_hud;
extern LONG* p_wc3_y_centre_hud;

extern BYTE* p_wc3_space_view_type;

extern BOOL* p_wc3_is_mouse_present;

extern WORD* p_wc3_mouse_button;
extern WORD* p_wc3_mouse_x;
extern WORD* p_wc3_mouse_y;

extern WORD* p_wc3_mouse_button_space;
extern WORD* p_wc3_mouse_x_space;
extern WORD* p_wc3_mouse_y_space;

extern LONG* p_wc3_mouse_centre_x;
extern LONG* p_wc3_mouse_centre_y;



extern CRITICAL_SECTION* p_wc3_movie_criticalsection;
extern void* p_wc3_movie_class;

extern LONG* p_wc3_movie_branch_list;
extern LONG* p_wc3_movie_branch_current_list_num;

extern bool* p_wc3_movie_halt_flag;

extern LONG* p_wc3_subtitles_enabled;
extern LONG* p_wc3_language_ref;

extern void(__thiscall* wc3_draw_hud_targeting_elements)(void*);
extern void(__thiscall* wc3_draw_hud_view_text)(void*);

extern void(__thiscall* wc3_nav_screen)(void*);

extern void** pp_wc3_music_thread_class;
extern void(__thiscall* wc3_music_thread_class_destructor)(void*);


extern void(*wc3_dealocate_mem01)(void*);
extern void(*wc3_unknown_func01)();
extern void(*wc3_update_input_states)();
extern BYTE(*wc3_translate_messages)(BOOL is_flight_mode, BOOL wait_for_message);
extern void (*wc3_translate_messages_keys)();
extern void (*wc3_conversation_decision_loop)();
extern void(*wc3_draw_choice_text_buff)(DWORD* ptr, BYTE* buff);


extern void(__thiscall* wc3_nav_screen_update_position)(void*);

extern BOOL(__thiscall* wc3_movie_set_position)(void*, LONG);
extern void(__thiscall* wc3_movie_update_positon)(void*);
extern BOOL(__thiscall* wc3_sig_movie_play_sequence)(void*, DWORD);

extern void(*wc3_movie_update_joystick_double_click_exit)();
extern BOOL(*wc3_movie_exit)();
