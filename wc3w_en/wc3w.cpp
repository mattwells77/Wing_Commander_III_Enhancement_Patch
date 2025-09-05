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

#include "wc3w.h"


char* p_wc3_szAppName = nullptr;
HINSTANCE* pp_hinstWC3W = nullptr;

bool* p_wc3_window_has_focus = nullptr;
bool* p_wc3_is_windowed = nullptr;
HWND* p_wc3_hWinMain = nullptr;
LONG* p_wc3_main_surface_pitch = nullptr;

//DWORD* p_wc3_client_width = nullptr;
//DWORD* p_wc3_client_height = nullptr;

DRAW_BUFFER** pp_wc3_db_game = nullptr;
DRAW_BUFFER_MAIN** pp_wc3_db_game_main = nullptr;

BITMAPINFO** pp_wc3_DIB_Bitmapinfo = nullptr;
VOID** pp_wc3_DIB_vBits = nullptr;

DWORD* p_wc3_gamma_val = nullptr;

LONG* p_wc3_x_centre_cockpit = nullptr;
LONG* p_wc3_y_centre_cockpit = nullptr;
LONG* p_wc3_x_centre_rear = nullptr;
LONG* p_wc3_y_centre_rear = nullptr;
LONG* p_wc3_x_centre_hud = nullptr;
LONG* p_wc3_y_centre_hud = nullptr;

SPACE_VIEW_TYPE* p_wc3_view_current_dir = nullptr;
SPACE_VIEW_TYPE* p_wc3_view_cockpit_or_hud = nullptr;

CAMERA_CLASS_01* p_wc3_camera_01 =nullptr;

bool* p_wc3_is_ejecting = nullptr;

BOOL* p_wc3_is_mouse_present = nullptr;
WORD* p_wc3_mouse_button = nullptr;
WORD* p_wc3_mouse_x = nullptr;
WORD* p_wc3_mouse_y = nullptr;

WORD* p_wc3_mouse_button_space = nullptr;
WORD* p_wc3_mouse_x_space = nullptr;
WORD* p_wc3_mouse_y_space = nullptr;

LONG* p_wc3_mouse_centre_x = nullptr;
LONG* p_wc3_mouse_centre_y = nullptr;

LARGE_INTEGER* p_wc3_frequency = nullptr;
LARGE_INTEGER* p_wc3_space_time_max = nullptr;
LARGE_INTEGER* p_wc3_space_time_min = nullptr;
LARGE_INTEGER* p_wc3_space_time_current = nullptr;
LARGE_INTEGER* p_wc3_space_time4 = nullptr;

LONG* p_wc3_joy_dead_zone = nullptr;

DWORD* p_wc3_joy_buttons = nullptr;
DWORD* p_wc3_joy_pov = nullptr;

LONG* p_wc3_joy_move_x = nullptr;
LONG* p_wc3_joy_move_y = nullptr;
LONG* p_wc3_joy_move_r = nullptr;


LONG* p_wc3_joy_x = nullptr;
LONG* p_wc3_joy_y = nullptr;

LONG* p_wc3_joy_throttle_pos = nullptr;

LONG* p_wc3_ambient_music_volume = nullptr;

BYTE* p_wc3_is_sound_enabled = nullptr;
void* p_wc3_audio_class = nullptr;

bool* p_wc3_movie_no_interlace = nullptr;

MOVIE_CLASS_INFLIGHT_01** pp_movie_class_inflight_01 = nullptr;
MOVIE_CLASS_INFLIGHT_02* p_movie_class_inflight_02 = nullptr;

//a reference to the current sound.
DWORD* p_wc3_inflight_audio_ref = nullptr;
//not sure what this does, made use of when inserting "Inflight_Movie_Audio_Check" function.
BYTE* p_wc3_inflight_audio_unk01 = nullptr;
//holds a pointer to something relating to the speaking pilot for setting the colour of their targeting rect. 
void** pp_wc3_inflight_audio_ship_ptr_for_rect_colour = nullptr;

//buffer rect structures used for drawing inflight movie frames, re-purposed to create a transparent rect in the cockpit/hud graphic for displaying HR movie's through.
DRAW_BUFFER_MAIN* p_wc3_inflight_draw_buff_main = nullptr;
DRAW_BUFFER* p_wc3_inflight_draw_buff = nullptr;

//return address when playing HR movies to skip over regular movie playback.
void* p_wc3_play_inflight_hr_movie_return_address = nullptr;

const char* p_save_game_text_eng = nullptr;
const char* p_save_game_text_ger = nullptr;
const char* p_save_game_text_fre = nullptr;

void** pp_wc3_music_thread_class = nullptr;
void(__thiscall* wc3_music_thread_class_destructor)(void*) = nullptr;

void(*wc3_dealocate_mem01)(void*) = nullptr;
void(*wc3_unknown_func01)() = nullptr;
void(*wc3_update_input_states)() = nullptr;


BYTE* p_wc3_key_pressed_scancode = nullptr;


void(__thiscall* wc3_draw_hud_targeting_elements)(void*) = nullptr;
void(__thiscall* wc3_draw_hud_view_text)(void*) = nullptr;

void(__thiscall* wc3_nav_screen)(void*) = nullptr;
void(__thiscall* wc3_nav_screen_update_position)(void*) = nullptr;

BYTE(*wc3_translate_messages)(BOOL is_flight_mode, BOOL wait_for_message) = nullptr;

void (*wc3_translate_messages_keys)() = nullptr;

void (*wc3_conversation_decision_loop)() = nullptr;

bool (*wc3_message_check_node_add)(bool(*)(HWND, UINT, WPARAM, LPARAM)) = nullptr;
bool (*wc3_message_check_node_remove)(bool(*)(HWND, UINT, WPARAM, LPARAM)) = nullptr;

bool(*wc3_movie_messages)(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) = nullptr;

void(*wc3_handle_movie)(BOOL flag) = nullptr;

BOOL(__thiscall* wc3_sig_movie_play_sequence)(void*, DWORD) = nullptr;

void(*wc3_movie_update_joystick_double_click_exit)() = nullptr;
BOOL(*wc3_movie_exit)() = nullptr;


LONG(*wc3_play_movie)(const char* mve_path, LONG var1, LONG var2, LONG var3, LONG var4, LONG var5, LONG var6) = nullptr;
BOOL(__thiscall* wc3_movie_set_position)(void*, LONG) = nullptr;
void(__thiscall* wc3_movie_update_positon)(void*) = nullptr;

CRITICAL_SECTION* p_wc3_movie_criticalsection = nullptr;
void* p_wc3_movie_class = nullptr;

LONG* p_wc3_movie_branch_list = nullptr;
LONG* p_wc3_movie_branch_current_list_num = nullptr;

LONG* p_wc3_subtitles_enabled = nullptr;
LONG* p_wc3_language_ref = nullptr;


void(*wc3_draw_choice_text_buff)(DWORD* ptr, BYTE* buff) = nullptr;

DWORD* p_wc3_key_scancode = nullptr;

bool* p_wc3_movie_halt_flag = nullptr;

char** p_wc3_movie_branch_subtitle;

BOOL(__thiscall* wc3_load_file_handle)(void*, BOOL print_error_flag, BOOL unknown_flag) = nullptr;
LONG(*wc3_find_file_in_tre)(char* pfile_name) = nullptr;

void (*wc3_copy_rect)(DRAW_BUFFER_MAIN* p_fromBuff, LONG from_x, LONG from_y, DRAW_BUFFER_MAIN* p_toBuff, LONG to_x, LONG to_y, DWORD pal_offset) = nullptr;
LONG(__thiscall* wc3_play_audio_01)(void*, DWORD arg01, DWORD arg02, DWORD arg03, DWORD arg04) = nullptr;
void(__thiscall*wc3_set_music_volume)(void*, LONG level) = nullptr;

void* (*wc3_allocate_mem_main)(DWORD) = nullptr;
void(*wc3_deallocate_mem_main)(void*) = nullptr;

void(*wc3_error_message_box)(const char* format, ...);

//_______________
void WC3W_Setup() {

    p_wc3_window_has_focus = (bool*)0x4A7E48;

    p_wc3_is_windowed = (bool*)0x49F984;

    p_wc3_hWinMain = (HWND*)0x4A5AAC;

    p_wc3_szAppName = (char*)0x4A5A80;

    pp_hinstWC3W = (HINSTANCE*)0x4A7E64;

    p_wc3_main_surface_pitch = (LONG*)0x4A32AC;

    pp_wc3_DIB_Bitmapinfo = (BITMAPINFO**)0x49F964;
    pp_wc3_DIB_vBits = (VOID**)0x49F968;

    p_wc3_gamma_val = (DWORD*)0x49F744;

    pp_wc3_db_game = (DRAW_BUFFER**)0x49F980;
    pp_wc3_db_game_main = (DRAW_BUFFER_MAIN**)0x49F97C;


    p_wc3_x_centre_cockpit = (LONG*)0x4A2D28;
    p_wc3_y_centre_cockpit = (LONG*)0x4A2D2C;
    p_wc3_x_centre_rear = (LONG*)0x4A2D48;
    p_wc3_y_centre_rear = (LONG*)0x4A2D4C;
    p_wc3_x_centre_hud = (LONG*)0x4A2D38;
    p_wc3_y_centre_hud = (LONG*)0x4A2D3C;

    p_wc3_view_current_dir = (SPACE_VIEW_TYPE*)0x4A2D24;
    p_wc3_view_cockpit_or_hud = (SPACE_VIEW_TYPE*)0x4A4C4C;

    p_wc3_camera_01 = (CAMERA_CLASS_01*)0x4B1830;

    p_wc3_is_ejecting = (bool*)0x4A2DA0;

    //p_wc3_client_width = (DWORD*)0x49F9A8;
    //p_wc3_client_height = (DWORD*)0x49F9AC;

    p_wc3_is_mouse_present = (BOOL*)0x4A7E54;
    p_wc3_mouse_button = (WORD*)0x4A7E58;
    p_wc3_mouse_x = (WORD*)0x4A7E5A;
    p_wc3_mouse_y = (WORD*)0x4A7E5C;

    p_wc3_mouse_button_space = (WORD*)0x4A9B90;
    p_wc3_mouse_x_space = (WORD*)0x4A9B92;
    p_wc3_mouse_y_space = (WORD*)0x4A9B94;

    p_wc3_mouse_centre_x = (LONG*)0x4A9B78;
    p_wc3_mouse_centre_y = (LONG*)0x4A9B98;

    p_wc3_joy_dead_zone = (LONG*)0x4A7E40;

    p_wc3_joy_buttons = (DWORD*)0x4B2334;

    p_wc3_joy_move_x = (LONG*)0x4B24D0;
    p_wc3_joy_move_y = (LONG*)0x4B2330;
    p_wc3_joy_move_r = (LONG*)0x4A7E38;

    p_wc3_joy_x = (LONG*)0x4B231C;
    p_wc3_joy_y = (LONG*)0x4B232C;

    p_wc3_joy_throttle_pos = (LONG*)0x4A7E3C;
    p_wc3_joy_pov = (DWORD*)0x4A7E34;

    pp_wc3_music_thread_class = (void**)0x4A3A74;
    wc3_music_thread_class_destructor = (void(__thiscall*)(void*))0x444B30;

    wc3_dealocate_mem01 = (void(*)(void*))0x490810;

    wc3_unknown_func01 = (void(*)())0x4082C0;

    wc3_update_input_states = (void(*)())0x4083F0;

    p_wc3_key_pressed_scancode = (BYTE*)0x4A7E2C;


    wc3_translate_messages = (BYTE(*)(BOOL, BOOL))0x482FB0;

    wc3_translate_messages_keys = (void(*)())0x482920;

    wc3_message_check_node_add = (bool (*)(bool(*pFUNC)(HWND, UINT, WPARAM, LPARAM)))0x483090;
    wc3_message_check_node_remove = (bool (*)(bool(*pFUNC)(HWND, UINT, WPARAM, LPARAM)))0x4830D0;

    wc3_movie_messages = (bool(*)(HWND, UINT, WPARAM, LPARAM))0x41C700;

    wc3_conversation_decision_loop = (void(*)())0x4824E0;

    wc3_handle_movie = (void(*)(BOOL))0x40FD20;

    wc3_sig_movie_play_sequence = (BOOL(__thiscall*)(void*, DWORD))0x4567A0;

    wc3_movie_update_joystick_double_click_exit = (void(*)())0x41BCE0;
    wc3_movie_exit = (BOOL(*)())0x41BE00;

    wc3_play_movie = (LONG(*)(const char*, LONG, LONG, LONG, LONG, LONG, LONG))0x41C240;

    wc3_movie_set_position = (BOOL(__thiscall*)(void*, LONG))0x41EEF0;

    p_wc3_movie_criticalsection = (CRITICAL_SECTION*)0x4A9A80;
    p_wc3_movie_class = (void*)0x4A24CC;
    wc3_movie_update_positon = (void(__thiscall*)(void*))0x41BDB0;


    p_wc3_movie_branch_list = (LONG*)0x4AAA50;
    p_wc3_movie_branch_current_list_num = (LONG*)0x4A24D4;

    p_wc3_subtitles_enabled = (LONG*)0x4A0FDC;
    p_wc3_language_ref = (LONG*)0x4A9720;


    p_wc3_key_scancode = (DWORD*)0x4A9B80;

    p_wc3_movie_halt_flag = (bool*)0x4A24C4;


    wc3_draw_choice_text_buff = (void(*)(DWORD * ptr, BYTE * buff))0x446830;


    wc3_draw_hud_targeting_elements = (void(__thiscall*)(void*))0x45C7C0;

    wc3_draw_hud_view_text = (void(__thiscall*)(void*))0x459500;

    wc3_nav_screen = (void(__thiscall*)(void*))0x445150;

    wc3_nav_screen_update_position = (void(__thiscall *)(void*))0x430460;
  
    wc3_load_file_handle = (BOOL(__thiscall*)(void*, BOOL, BOOL))0x483A60;
    wc3_find_file_in_tre = (LONG(*)(char*))0x484CB0;

    p_wc3_frequency = (LARGE_INTEGER*)0x4A9768;
    p_wc3_space_time_max = (LARGE_INTEGER*)0x4A9A78;
    p_wc3_space_time_min = (LARGE_INTEGER*)0x4A9760;
    p_wc3_space_time_current = (LARGE_INTEGER*)0x4A26E8;
    p_wc3_space_time4 = (LARGE_INTEGER*)0x4A2750;


    p_movie_class_inflight_02 = (MOVIE_CLASS_INFLIGHT_02*)0x4AB340;
    pp_movie_class_inflight_01 = (MOVIE_CLASS_INFLIGHT_01**)0x4A3358;

    wc3_copy_rect = (void (*)(DRAW_BUFFER_MAIN*, LONG, LONG, DRAW_BUFFER_MAIN*, LONG, LONG, DWORD)) 0x473687;
    wc3_play_audio_01 = (LONG(__thiscall*)(void*, DWORD, DWORD, DWORD, DWORD))0x438E10;
    wc3_set_music_volume = (void(__thiscall*)(void*, LONG))0x439370;

    p_wc3_ambient_music_volume = (LONG*)0x4AB7B0;
    p_wc3_is_sound_enabled = (BYTE*)0x4A3A48;
    p_wc3_audio_class = (void*)0x4A9580;

    p_wc3_inflight_audio_ref = (DWORD*)0x4A3354;
    p_wc3_inflight_audio_unk01 = (BYTE*)0x4A3338;
    pp_wc3_inflight_audio_ship_ptr_for_rect_colour = (void**)0x4A3350;

    p_wc3_play_inflight_hr_movie_return_address = (void*)0x423024;

    p_wc3_inflight_draw_buff_main = (DRAW_BUFFER_MAIN*)0x4AB300;
    p_wc3_inflight_draw_buff = (DRAW_BUFFER*)0x4AB318;


    wc3_allocate_mem_main = (void* (*)(DWORD))0x480B0B;
    wc3_deallocate_mem_main = (void(*)(void*))0x480C92;

    p_save_game_text_eng = (char*)0x49FF60;
    p_save_game_text_ger = (char*)0x49FF40;
    p_save_game_text_fre = (char*)0x49FF50;

    p_wc3_movie_no_interlace = (bool*)0x49F73C;

    p_wc3_movie_branch_subtitle = (char**)0x4AAA80;

    wc3_error_message_box = (void(*)(const char*, ...))0x4702B0;
}
