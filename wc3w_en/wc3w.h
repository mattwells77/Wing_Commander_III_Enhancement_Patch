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

#define NUM_TUNES   34


struct FILE_STRUCT {
    /*0x00*/char path[80];
    /*0x50*/DWORD unk50;
    /*0x54*/DWORD unk54;
    /*0x58*/DWORD unk58;
    /*0x5C*/DWORD file_pos1;//?
    /*0x60*/DWORD file_pos2;//?
    /*0x64*/DWORD file_size;
    /*0x68*/DWORD create_file_dwDesiredAccess;// 0 = GENERIC_READ\GENERIC_WRITE + dwCreationDisposition = CREATE_ALWAYS, 1 = GENERIC_READ, 2 = GENERIC_WRITE, 3 = GENERIC_READ\GENERIC_WRITE + dwCreationDisposition = OPEN_EXISTING
    /*0x6C*/DWORD file_creation_failed;
    /*0x70*/DWORD create_file_HANDLE;
    /*0x74*/DWORD unk74;
    /*0x78*/DWORD unk78;
    /*0x7C*/DWORD unk7C;
};


struct MUSIC_HEADER {
    /*0x00*/DWORD flags;
    /*0x04*/DWORD current_tune;
    /*0x08*/DWORD previous_tune;
    /*0x0C*/DWORD requested_tune;
    /*0x10*/DWORD unk10;
    /*0x14*/DWORD unk14;//volume?
};

struct MUSIC_FILE {
    FILE_STRUCT file;
    /*0x80*/DWORD unk80;//xanlib ptr ref??
    /*0x84*/DWORD flags;// DIGM_CDX_02_flag;//(BYTE)(flags<<8)don't interrupt until done
    /*0x88*/DWORD DIGM_CDX_01_flag;//continuous play
    /*0x8C*/DWORD DIGM_CDX_03_flag;//continuous play again ???
    ///*0x90*/DWORD unk90;
    ///*0x94*/DWORD DIGM_CDX_04_unk;
};


struct MUSIC_CLASS {
    MUSIC_HEADER header;
    MUSIC_FILE file[NUM_TUNES];
};


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

//this structure is very incomplete and likely larger than specified.
struct MOVIE_CLASS_01 {
    DWORD unk00;
    DWORD unk04;
    DWORD unk08;
    DWORD unk0C;
    DWORD unk10;
    DWORD unk14;
    DWORD unk18;
    DWORD unk1C;
    DWORD unk20;
    DWORD unk24;//*p_func(Arg1 BYTE *p_buff)
    DWORD unk28;
    DWORD unk2C;
    DWORD unk30;
    DWORD unk34;
    DWORD unk38;
    DWORD unk3C;
    DWORD unk40;
    DWORD unk44;
    DWORD unk48;
    DWORD unk4C;
    DWORD unk50;
    DWORD branch_max;//0x54
    DWORD unk58;
    DWORD unk5C;
    DWORD branch_current;//0x60
    DWORD unk64;
    DWORD unk68;
    DWORD unk6C;
    char* path;//0x70
    DWORD unk74;
    DWORD unk78;
    DWORD unk7C;
};


//this structure is very incomplete and likely larger than specified.
struct HUD_CLASS_01 {
    DWORD unk00;
    DWORD unk04;
    DWORD unk08;
    DWORD unk0C;
    DWORD unk10;
    DWORD unk14;
    DWORD unk18;
    DWORD unk1C;
    DWORD unk20;
    LONG hud_x;//x1
    LONG hud_y;//y1
    DWORD unk2C;
    DWORD unk30;
    DWORD unk34;
    DWORD unk38;
    DWORD unk3C;
    DWORD unk40;
    DWORD unk44;
    DWORD unk48;
    LONG comm_x;//x2
    LONG comm_y;//y2
    DWORD unk54;
    DWORD unk58;
    DWORD unk5C;

};

//this structure is very incomplete and likely larger than specified.
struct MOVIE_CLASS_INFLIGHT_01 {
    DWORD unk00;
    DWORD flags;//0x04
    DWORD timecode_start_of_file_30fps;//0x08
    DWORD timecode_duration_30fps;//0x0C
    DWORD video_frame_offset_15fps_neg;//0x10
    DWORD timecode_start_of_scene_30fps;//0x14
    DWORD unk18;
    char file_name[16];//0x1C
    DWORD unk2C;
    DWORD unk30;
    DWORD unk34;
    DWORD unk38;
    DWORD unk3C;
    DWORD unk40;
    DWORD unk44;
    DWORD unk48;
};

//this structure is very incomplete and likely larger than specified.
struct MOVIE_CLASS_INFLIGHT_02 {
    MOVIE_CLASS_01* p_movie_class;//0x00
    char* file_path;//0x04
    DWORD unk08;//0x08
    DWORD width;//0x0C
    DWORD height;//0x10
    LONG current_frame;//0x14
};


enum class SPACE_VIEW_TYPE : WORD {
    Cockpit = 0,
    CockLeft = 1,
    CockRight = 2,
    CockBack = 3,
    Chase = 4,
    Rotate = 5,
    CockHud = 6,
    UNK07 = 7,
    UNK08 = 8,
    NavMap = 9,
    UNK10 = 10,
    Track = 11,
};

//this structure is very incomplete and likely larger than specified.
struct CAMERA_CLASS_01 {
    DWORD unk00;//0x00
    DWORD unk04;//0x04
    DWORD unk08;//0x08
    SPACE_VIEW_TYPE view_type;//0x0C
    //WORD unk05;//0x10
};

void WC3W_Setup();

extern DWORD* p_wc3_virtual_alloc_mem_size;

extern char* p_wc3_szAppName;
extern HINSTANCE* pp_hinstWC3W;

extern bool* p_wc3_window_has_focus;
extern bool* p_wc3_is_windowed;
extern HWND* p_wc3_hWinMain;
extern LONG* p_wc3_main_surface_pitch;

extern DRAW_BUFFER** pp_wc3_db_game;
extern DRAW_BUFFER_MAIN** pp_wc3_db_game_main;

extern BITMAPINFO** pp_wc3_DIB_Bitmapinfo;
extern VOID** pp_wc3_DIB_vBits;

extern DWORD* p_wc3_gamma_val;

extern LARGE_INTEGER* p_wc3_frequency;
extern LARGE_INTEGER* p_wc3_space_time_max;


extern LONG* p_wc3_x_centre_cockpit;
extern LONG* p_wc3_y_centre_cockpit;
extern LONG* p_wc3_x_centre_rear;
extern LONG* p_wc3_y_centre_rear;
extern LONG* p_wc3_x_centre_hud;
extern LONG* p_wc3_y_centre_hud;

extern SPACE_VIEW_TYPE* p_wc3_view_current_dir;//will only be Cockpit, CockLeft, CockRight, CockBack or CockHud
extern SPACE_VIEW_TYPE* p_wc3_view_cockpit_or_hud;//will only be Cockpit or CockHud

extern CAMERA_CLASS_01* p_wc3_camera_01;

extern void** pp_wc3_player_obj_struct;

extern bool* p_wc3_is_ejecting;

extern BOOL* p_wc3_is_mouse_present;

extern WORD* p_wc3_mouse_button;
extern WORD* p_wc3_mouse_x;
extern WORD* p_wc3_mouse_y;

extern WORD* p_wc3_mouse_button_space;
extern WORD* p_wc3_mouse_x_space;
extern WORD* p_wc3_mouse_y_space;

extern LONG* p_wc3_mouse_centre_x;
extern LONG* p_wc3_mouse_centre_y;

extern LONG* p_wc3_joy_dead_zone;

extern DWORD* p_wc3_joy_buttons;
extern DWORD* p_wc3_joy_pov;

extern LONG* p_wc3_joy_move_x;
extern LONG* p_wc3_joy_move_y;
extern LONG* p_wc3_joy_move_r;

extern LONG* p_wc3_joy_move_x_256;
extern LONG* p_wc3_joy_move_y_256;

extern LONG* p_wc3_joy_x;
extern LONG* p_wc3_joy_y;

extern LONG* p_wc3_joy_throttle_pos;


extern CRITICAL_SECTION* p_wc3_movie_criticalsection;
extern void* p_wc3_movie_class;

extern LONG* p_wc3_movie_branch_list;
extern LONG* p_wc3_movie_branch_current_list_num;

extern bool* p_wc3_movie_halt_flag;

extern DWORD* p_wc3_movie_frame_count;

extern LONG* p_wc3_subtitles_enabled;
extern LONG* p_wc3_language_ref;

extern MOVIE_CLASS_INFLIGHT_01** pp_movie_class_inflight_01;
extern MOVIE_CLASS_INFLIGHT_02* p_movie_class_inflight_02;

extern LONG* p_wc3_ambient_music_volume;
extern LONG* p_wc3_music_volume_current;

extern BYTE* p_wc3_is_sound_enabled;
extern void* p_wc3_audio_class;

extern bool* p_wc3_movie_no_interlace;

//a reference to the current sound.
extern DWORD* p_wc3_inflight_audio_ref;
//not sure what this does, made use of when inserting "Inflight_Movie_Audio_Check" function.
extern BYTE* p_wc3_inflight_audio_unk01;
//holds a pointer to something relating to the speaking pilot for setting the colour of their targeting rect. 
extern void** pp_wc3_inflight_audio_ship_ptr_for_rect_colour;


//buffer rect structures used for drawing inflight movie frames, re-purposed to create a transparent rect in the cockpit/hud graphic for displaying HR movie's through.
extern DRAW_BUFFER_MAIN* p_wc3_inflight_draw_buff_main;
extern DRAW_BUFFER* p_wc3_inflight_draw_buff;

//return address when playing HR movies to skip over regular movie playback.
extern void* p_wc3_play_inflight_hr_movie_return_address;

extern const char* p_save_game_text_eng;
extern const char* p_save_game_text_ger;
extern const char* p_save_game_text_fre;

extern char** p_wc3_movie_branch_subtitle;

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

extern bool (*wc3_message_check_node_add)(bool(*)(HWND, UINT, WPARAM, LPARAM));
extern bool (*wc3_message_check_node_remove)(bool(*)(HWND, UINT, WPARAM, LPARAM));

extern void(__thiscall* wc3_file_init)(void*);
extern BOOL(__thiscall* wc3_file_load)(void*, char* path, DWORD dwDesiredAccess, BOOL halt_on_create_file_error, DWORD dwFlagsAndAttributes);
extern BOOL(__thiscall* wc3_file_close)(void*);
extern size_t(__thiscall* wc3_file_read)(void*, BYTE* buff, size_t len);
extern LONG(*wc3_find_file_in_tre)(char* pfile_name);
//pal_offset  colour (0-255) for filled rect, if pal_offset above 255 from-buffer is copied to to-buff.
extern void (*wc3_copy_rect)(DRAW_BUFFER_MAIN* p_fromBuff, LONG from_x, LONG from_y, DRAW_BUFFER_MAIN* p_toBuff, LONG to_x, LONG to_y, DWORD pal_offset);
extern LONG(__thiscall* wc3_play_audio_01)(void*, DWORD arg01, DWORD arg02, DWORD arg03, DWORD arg04);

extern void(__thiscall*wc3_set_music_volume)(void*, LONG level);

extern void*(*wc3_allocate_mem_main)(DWORD);
extern void(*wc3_deallocate_mem_main)(void*);

extern void(*wc3_error_message_box)(const char* format, ...);

extern void(*wc3_draw_text_to_buff)(DRAW_BUFFER* p_toBuff, DWORD x, DWORD y, DWORD unk1, char* text_buff, DWORD unk2);

extern LONG (*wc3_clear_buffer_colour)(DRAW_BUFFER_MAIN* p_Buff, BYTE pal_offset);
