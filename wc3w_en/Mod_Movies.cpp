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

#include "modifications.h"
#include "Display_DX11.h"
#include "libvlc_Movies.h"
#include "input_config.h"
#include "wc3w.h"
#include "memwrite.h"
#include "configTools.h"


LibVlc_Movie* pMovie_vlc = nullptr;


//_____________________________________________________________________________________
static BOOL DrawVideoFrame(VIDframe* vidFrame, RGBQUAD* tBuff, UINT tWidth, DWORD flag) {

    static SCALE_TYPE scale_type = SCALE_TYPE::fit;
    static bool linear_upscaling = false;
    static bool run_once = false;
    if (!run_once) {
        run_once = true;
        if (ConfigReadInt(L"MOVIES", L"ENABLE_ORIGINAL_MOVIES_LIMITED_SCALING", CONFIG_MOVIES_ENABLE_ORIGINAL_MOVIES_LIMITED_SCALING))
            scale_type = SCALE_TYPE::fit_best;
        if (ConfigReadInt(L"MOVIES", L"ENABLE_ORIGINAL_MOVIES_LINEAR_UPSCALING", CONFIG_MOVIES_ENABLE_ORIGINAL_MOVIES_LINEAR_UPSCALING))
            linear_upscaling = true;
    }

    DWORD height = vidFrame->height;
    DWORD width = vidFrame->width;

    if (!*p_wc3_movie_no_interlace) {
        height += height;
        width += width;
    }

    if (!surface_movieXAN || width != surface_movieXAN->GetWidth() || height != surface_movieXAN->GetHeight()) {
        if (surface_movieXAN)
            delete surface_movieXAN;
        surface_movieXAN = new DrawSurface8_RT(0, 0, width, height, 32, 0x00000000, false, 0);
        surface_movieXAN->ScaleTo((float)clientWidth, (float)clientHeight, scale_type);
        if (!linear_upscaling)
            surface_movieXAN->Set_Default_SamplerState(pd3dPS_SamplerState_Point);
        Debug_Info("surface_movieXAN created");
    }
    //Debug_Info("%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X", vidFrame->unknown00, vidFrame->unknown04, vidFrame->unknown08, vidFrame->width, vidFrame->height, vidFrame->unknown14, vidFrame->bitFlag, vidFrame->unknown1C, vidFrame->unknown20, vidFrame->unknown24);

    BYTE* pSurface = nullptr;
    LONG pitch = 0;

    if (surface_movieXAN->Lock((VOID**)&pSurface, &pitch) != S_OK)
        return FALSE;

    BYTE* fBuff = vidFrame->buff;

    for (UINT y = 0; y < height; y++) {
        if (*p_wc3_movie_no_interlace || y % 2) {
            UINT x2 = 0;
            for (UINT x = 0; x < vidFrame->width; x++) {
                pSurface[x2] = fBuff[x];
                if (*p_wc3_movie_no_interlace)
                    x2++;
                else {
                    pSurface[x2 + 1] = fBuff[x];
                    x2 += 2;
                }
            }
            fBuff += vidFrame->width;
        }
        pSurface += pitch;
    }

    surface_movieXAN->Unlock();

    return TRUE;
}


//__________________________________
static void UnlockShowMovieSurface() {

    if (surface_gui == nullptr)
        return;
    surface_gui->Unlock();
    Display_Dx_Present(PRESENT_TYPE::movie);
}


//______________________________________________________________________________________________________
static BOOL Play_Movie_Sequence(void* p_wc3_movie_class, void* p_sig_movie_class, DWORD sig_movie_flags) {

    char* mve_path = (char*)((DWORD*)p_wc3_movie_class)[28];
    //Debug_Info("Play_Movie_Loop:  sig_movie_flags:%X", sig_movie_flags);
    Debug_Info_Movie("Play_Movie_Sequence: main_path: %s", mve_path);
    Debug_Info_Movie("Play_Movie_Sequence: current_list_num: %d", *p_wc3_movie_branch_current_list_num);
    Debug_Info_Movie("Play_Movie_Sequence: first branch: %d", *p_wc3_movie_branch_list);
    //Debug_Info("max branches:%d", ((LONG*)p_wc3_movie_class)[21]);

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
            //wc3_movie_update_joystick_double_click_exit();
            exit_flag = *p_wc3_movie_halt_flag;// wc3_movie_exit();
            if (Get_Key_State(0x1, 0, 0))
                exit_flag |= TRUE;
            if (!pMovie_vlc->IsPlaying(&movie_state)) {
                *p_wc3_movie_branch_current_list_num = movie_state.list_num;
                if (!movie_state.hasPlayed) {
                    Debug_Info_Error("Play_Movie_Sequence: ended BAD, branch:%d, listnum:%d", movie_state.branch, movie_state.list_num);
                    //if branch failed to play, shift to the current branch position so the rest of the movie can be played out using the original player.
                    if (wc3_movie_set_position(p_wc3_movie_class, p_wc3_movie_branch_list[movie_state.list_num]) == FALSE)
                        Debug_Info_Error("Play_Movie_Sequence: wc3_movie_set_position Failed, branch:%d", p_wc3_movie_branch_list[movie_state.list_num]);
                    else
                        play_successfull = FALSE;//Only set false if wc3_movie_set_position succeeds. 
                }
                else
                    Debug_Info_Movie("Play_Movie_Sequence: ended OK, branch:%d, listnum:%d", movie_state.branch, movie_state.list_num);

                exit_flag = TRUE;
            }
        }
        pMovie_vlc->Stop();
    }

    //if alternate movie failed to play, continue movie using original player.
    if (!play_successfull) {
        delete pMovie_vlc;
        pMovie_vlc = nullptr;
        play_successfull = wc3_sig_movie_play_sequence(p_sig_movie_class, sig_movie_flags);
    }
    else
        *p_wc3_movie_frame_count += 1;//this global needs to be set to evoke the movie fade out function.

    Sleep(150);//add a small delay to reduce unintended button clicks after ending a movie by double-clicking.

    Debug_Info_Movie("Play_Movie_Sequence: Done");
    return play_successfull;
}


//_____________________________________________________
static void __declspec(naked) play_movie_sequence(void) {

    __asm {
        push ebx
        push ebp

        push[esp + 0xC]//sig movie flags? 0x7FFFFFFF
        push ecx
        push ebp
        call Play_Movie_Sequence
        add esp, 0xC

        pop ebp
        pop ebx

        ret 0x4
    }
}


//__________________________
static void Movie_Fade_Out() {

    LARGE_INTEGER thisTime = { 0LL };
    LARGE_INTEGER nextTime = { 0LL };
    LARGE_INTEGER update_offset{ 0LL };
    update_offset.QuadPart = p_wc3_frequency->QuadPart / 32;
    QueryPerformanceCounter(&thisTime);
    nextTime.QuadPart = thisTime.QuadPart + update_offset.QuadPart;

    int count = 0;

    while (count < 16) {
        QueryPerformanceCounter(&thisTime);
        if (thisTime.QuadPart >= nextTime.QuadPart) {
            nextTime.QuadPart = thisTime.QuadPart + update_offset.QuadPart;
            count++;
            Set_Movie_Fade_Level(count);
            Display_Dx_Present();
        }
    }

    //clear movie buffers before resetting fade level.
    if (pMovie_vlc) {
        DrawSurface* surface = pMovie_vlc->Get_Currently_Playing_Surface();
        if (surface)
            surface->Clear_Texture(0);
    }
    if (surface_movieXAN)
        surface_movieXAN->Clear_Texture(0);
    //set level to 0 to end fade out.
    Set_Movie_Fade_Level(0);
    Display_Dx_Present();
}


//________________________________________________
static void __declspec(naked) movie_fade_out(void) {

    __asm {
        pushad
        call Movie_Fade_Out
        popad

        ret
    }
}


//________________________________________________
static BOOL Play_HD_Movie_Sequence(char* mve_path) {

    Debug_Info_Movie("Play_HD_Movie_Sequence: main_path: %s", mve_path);
    Debug_Info_Movie("Play_HD_Movie_Sequence: current_list_num: %d", *p_wc3_movie_branch_current_list_num);
    Debug_Info_Movie("Play_HD_Movie_Sequence: first branch: %d", *p_wc3_movie_branch_list);

    if (pMovie_vlc)
        delete pMovie_vlc;
    std::string movie_name;
    Get_Movie_Name_From_Path(mve_path, &movie_name);
    pMovie_vlc = new LibVlc_Movie(movie_name, p_wc3_movie_branch_list, *p_wc3_movie_branch_current_list_num);

    if (pMovie_vlc->IsError()) {
        delete pMovie_vlc;
        pMovie_vlc = nullptr;
        Debug_Info("Play_HD_Movie_Sequence: Failed");
        return FALSE;
    }

    BOOL exit_flag = FALSE;
    BOOL play_successfull = FALSE;
    MOVIE_STATE movie_state{ 0 };

    if (pMovie_vlc->Play()) {
        play_successfull = TRUE;
        while (!exit_flag) {
            wc3_translate_messages_keys();
            //wc3_movie_update_joystick_double_click_exit();
            exit_flag = *p_wc3_movie_halt_flag;// wc3_movie_exit();
            if (Get_Key_State(0x1, 0, 0))
                exit_flag |= TRUE;
            if (!pMovie_vlc->IsPlaying(&movie_state)) {
                *p_wc3_movie_branch_current_list_num = movie_state.list_num;
                if (!movie_state.hasPlayed) {
                    Debug_Info_Movie("Play_HD_Movie_Sequence: ended BAD, branch:%d, listnum:%d", movie_state.branch, movie_state.list_num);
                    play_successfull = FALSE;
                }
                else
                    Debug_Info_Movie("Play_HD_Movie_Sequence: ended OK, branch:%d, listnum:%d", movie_state.branch, movie_state.list_num);
                exit_flag = TRUE;
            }
        }
        pMovie_vlc->Stop();
    }

    Sleep(150);//add a small delay to reduce unintended button clicks after ending a movie by double-clicking.

    Debug_Info_Movie("Play_HD_Movie_Sequence: Done:%d", play_successfull);
    return play_successfull;
}


//__________________________________________________
static void Set_Conversation_Decision_Text_Colours() {

    static BYTE text_colour[]{ 255, 255, 255, 80, 80, 80 };
    Palette_Update(text_colour, 252, 2);
}


//______________________________________________________
static BOOL Play_HD_Movie(char* mve_path, BYTE fade_out) {

    DXGI_RATIONAL refreshRate{};
    refreshRate.Denominator = 1;
    refreshRate.Numerator = 3;
    p_wc3_movie_click_time->QuadPart = p_wc3_frequency->QuadPart * refreshRate.Denominator / refreshRate.Numerator;
    *p_wc3_movie_frame_count = 0;
    wc3_message_check_node_add(wc3_movie_messages);

    if (surface_gui)
        surface_gui->Clear_Texture(0x00);

    //set colour values used by dialogue choice text.
    Set_Conversation_Decision_Text_Colours();

    Debug_Info_Movie("Play_HD_Movie");
    BOOL play_successfull = TRUE;
    BOOL exit_flag = FALSE;
    while (!exit_flag) {
        wc3_translate_messages_keys();
        //wc3_movie_update_joystick_double_click_exit();
        exit_flag = *p_wc3_movie_halt_flag;// wc3_movie_exit();
        if (Get_Key_State(0x1, 0, 0))
            exit_flag |= TRUE;
        if (Play_HD_Movie_Sequence(mve_path)) {
            //wc3_draw_movie_frame();

            wc3_handle_movie(0);

            *p_wc3_movie_branch_current_list_num = 0;
            if (p_wc3_movie_branch_list[*p_wc3_movie_branch_current_list_num] == -1)
                exit_flag = TRUE;
        }
        else
            exit_flag = TRUE, play_successfull = FALSE;
    }

    wc3_message_check_node_remove(wc3_movie_messages);

    if (surface_gui)
        surface_gui->Clear_Texture(0);
    if (fade_out && play_successfull)
        Movie_Fade_Out();

    Debug_Info_Movie("Play_HD_Movie: Done");
    return play_successfull;
}


//_______________________________________________
static void __declspec(naked) play_hd_movie(void) {

    __asm {
        push ebx
        push ecx
        push edx
        push edi
        push esi
        push ebp

        mov edx, dword ptr ss : [esp + 0x30]//fade_out flag
        mov ecx, dword ptr ss : [esp + 0x20]//path
        push edx
        push ecx
        call Play_HD_Movie
        add esp, 0x8

        pop ebp
        pop esi
        pop edi
        pop edx
        pop ecx
        pop ebx

        cmp eax, FALSE
        je hd_movie_error
        //hd movie played without error.
        add esp, 0x04 //ditch ret address for this function.
        //The next address on the stack is the ret address for the regular movie play back function.
        ret

        hd_movie_error :

        // hd movie had errors, return to wc3 play_movie function to play regular movie.
        pop ecx  //store ret address for this function
            sub esp, 0x17C//start prologue code for return to regular movie play back function.
            push ecx //re-insert address for this function

            ret
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


// The below function makes use of data that has it's origin in inflight profile iff files. These are located on the path "\DATA\PROFILE\" within the games ".tre" files.
// Internally they contain an inflight profile form labelled "PROF" and within that a form labelled "RADI" which contains the communication data.
// The relevant sections are:
//
// "FMV " section.
// format structure for eace listed mve file:
// BYTE     ref;                //number reference within the MSGx sections
// BYTE     flag;               //?
// DWORD    tc_start_of_file;   //SMPTE time-code 30fps for the start of file.
// char     file_name[13];      //mve movie file name
//
// "MSGS", "MSGG" and "MSGF" sections for each language.
// format structure for each listed scene:
// BYTE     sond_ref;           //reference to the played audio in the "SOND" section.
// BYTE     fmv_ref;            //reference to a movie in the FMV " section.
// DWORD    tc_start_30fps;     //SMPTE time-code 30fps for the start of scene.
// DWORD    tc_length_30fps;    //SMPTE time-code 30fps for the duration of scene.
// DWORD    neg_offset_15fps;   //subtracted from the video position offset, 15fps.
// char     text[variable];     //subtitle for the particular language.
//
//___________________________________________________________
static BOOL Play_Inflight_Movie(HUD_CLASS_01* p_hud_class_01) {

    if (*pp_movie_class_inflight_01) {

        if (p_hud_class_01->hud_y + p_hud_class_01->comm_y + (LONG)p_movie_class_inflight_02->height > (*pp_wc3_db_game_main)->rc.bottom - (*pp_wc3_db_game_main)->rc.top + 1) {
            Debug_Info_Movie("Play_Inflight_Movie: Movie being drawn beyond screen height hud_y:%d, comm_y:%d, vid_height:%d, scrn_height:%d", p_hud_class_01->hud_y, p_hud_class_01->comm_y, (LONG)p_movie_class_inflight_02->height, (*pp_wc3_db_game_main)->rc.bottom - (*pp_wc3_db_game_main)->rc.top + 1);
            LONG y_diff = (p_hud_class_01->hud_y + p_hud_class_01->comm_y + (LONG)p_movie_class_inflight_02->height) - ((*pp_wc3_db_game_main)->rc.bottom - (*pp_wc3_db_game_main)->rc.top + 1);
            if (p_hud_class_01->comm_y >= y_diff)
                p_hud_class_01->comm_y -= y_diff;
            Debug_Info_Movie("Play_Inflight_Movie: comm_y adjusted:%d", p_hud_class_01->comm_y);
        }

        static LARGE_INTEGER inflight_audio_play_start_time{ 0 };
        static LARGE_INTEGER inflight_audio_play_start_offset{ 0 };

        if (!(*p_wc3_inflight_draw_buff).buff && !pMovie_vlc_Inflight) {
            Debug_Info_Movie("Play_Inflight_Movie: timecodes: tc_start_of_file:%d, tc_start_of_scene:%d, tc_duration/appendix:%d, scene_video_neg_frame_offset:%d", (*pp_movie_class_inflight_01)->timecode_start_of_file_30fps, (*pp_movie_class_inflight_01)->timecode_start_of_scene_30fps, (*pp_movie_class_inflight_01)->timecode_duration_30fps, (*pp_movie_class_inflight_01)->video_frame_offset_15fps_neg);

            //Get the offset with in video file by subtracting the start_of_scene from the start_of_file, value returned is frames at 30fps.
            //This is on occasion used by pilot heads to jump to different scenes but more often then not they reuse the same footage with at different durations to match the audio. 
            LONG video_start_frame = Get_Num_Frames_Between_Timecodes_30fps((*pp_movie_class_inflight_01)->timecode_start_of_file_30fps, (*pp_movie_class_inflight_01)->timecode_start_of_scene_30fps);
            Debug_Info_Movie("Play_Inflight_Movie: Video start offset frames:%d", video_start_frame);
            if (video_start_frame < 0)
                video_start_frame = 0;

            //Supposed to be subtracted from video_start_frame. Makes more sense to me to add it to audio offset. A value of 3 is used by many Pilot Heads, otherwise this is usually zero.
            LONG audio_start_frame = (*pp_movie_class_inflight_01)->video_frame_offset_15fps_neg * 2;

            //inflight_audio_play_start_offset.QuadPart = 0;
            //If the start frame offset is small, delay audio start instead of moving the video position, as libvlc doesn't shift position well.
            //Two of Rollins Victory communications have a small offset like this, Others Victory communications start a zero.
            if (video_start_frame <= 10) {
                audio_start_frame += video_start_frame;
                video_start_frame = 0;
                Debug_Info_Movie("Play_Inflight_Movie: Fixed Video start offset frames:%d", video_start_frame);
                Debug_Info_Movie("Play_Inflight_Movie: Fixed Audio start offset frames:%d", audio_start_frame);
            }

            inflight_audio_play_start_offset.QuadPart = static_cast<long long>(audio_start_frame) * p_wc3_frequency->QuadPart / 30;

            RECT rc_dest{ p_hud_class_01->hud_x + p_hud_class_01->comm_x, p_hud_class_01->hud_y + p_hud_class_01->comm_y,  p_hud_class_01->hud_x + p_hud_class_01->comm_x + (LONG)p_movie_class_inflight_02->width - 1, p_hud_class_01->hud_y + p_hud_class_01->comm_y + (LONG)p_movie_class_inflight_02->height - 1 };
            //Debug_Info_Movie("size:%d,%d,%d,%d", rc_dest.left, rc_dest.top, rc_dest.right, rc_dest.bottom);

            //iff files modified to play movies divided into scenes DON'T INCLUDE an extension in their file name. 
            //if the movie file name has an extension, DON'T ADD a letter appendix by setting "appendix_offset = -1".
            char* ext = strrchr((*pp_movie_class_inflight_01)->file_name, '.');

            if (ext) {
                DWORD length_frames = Get_Num_Frames_Between_Timecodes_30fps(0, (*pp_movie_class_inflight_01)->timecode_duration_30fps);
                pMovie_vlc_Inflight = new LibVlc_MovieInflight((*pp_movie_class_inflight_01)->file_name, &rc_dest, video_start_frame, length_frames);
            }
            else {
                //appendix offsets for individual scene files are stored in "timecode_duration_30fps" 0-26 for letter code.
                DWORD appendix = (*pp_movie_class_inflight_01)->timecode_duration_30fps;
                pMovie_vlc_Inflight = new LibVlc_MovieInflight((*pp_movie_class_inflight_01)->file_name, &rc_dest, appendix);
            }

            if (!pMovie_vlc_Inflight->Play()) {
                delete pMovie_vlc_Inflight;
                pMovie_vlc_Inflight = nullptr;
                Debug_Info_Movie("LibVlc_MovieInflight play failed");
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
            //setting timecode_start_of_scene_30fps = 1 in order allow the audio delay as timecode_start_of_scene_30fps is used as a flag to start the audio when == 0;
            (*pp_movie_class_inflight_01)->timecode_start_of_scene_30fps = 1;
        }

        if (!pMovie_vlc_Inflight) {
            //Debug_Info("LibVlc_MovieInflight !pMovie_vlc_Inflight failed");
            return FALSE;
        }

        if (!pMovie_vlc_Inflight->Check_Play_Time())
            return TRUE;

        // timecode_start_of_scene_30fps is used as a flag to initiate audio playback.
        // setting this here once playback is initialised and audio start time reached.
        if ((*pp_movie_class_inflight_01)->timecode_start_of_scene_30fps != 0) {
            LARGE_INTEGER inflight_video_play_time{};
            QueryPerformanceCounter(&inflight_video_play_time);
            if (inflight_audio_play_start_time.QuadPart == 0)
                inflight_audio_play_start_time.QuadPart = inflight_video_play_time.QuadPart + inflight_audio_play_start_offset.QuadPart;
            if (inflight_audio_play_start_time.QuadPart < inflight_video_play_time.QuadPart)
                (*pp_movie_class_inflight_01)->timecode_start_of_scene_30fps = 0;
        }

        //check if video display dimensions have changed and update if necessary.
        RECT rc_dest{ p_hud_class_01->hud_x + p_hud_class_01->comm_x, p_hud_class_01->hud_y + p_hud_class_01->comm_y,  p_hud_class_01->hud_x + p_hud_class_01->comm_x + (LONG)p_movie_class_inflight_02->width - 1, p_hud_class_01->hud_y + p_hud_class_01->comm_y + (LONG)p_movie_class_inflight_02->height - 1 };
        pMovie_vlc_Inflight->Update_Display_Dimensions(&rc_dest);

        //set the movie class pointer to null to signal to wc3 that the movie has ended.
        if (pMovie_vlc_Inflight && pMovie_vlc_Inflight->HasPlayed()) {
            *pp_movie_class_inflight_01 = nullptr;
            Debug_Info_Movie("Play_Inflight_Movie: Movie Finished");
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
            Debug_Info_Movie("Inflight_Movie_Unload HasAudio - volume returned to normal");
            wc3_set_music_volume(p_wc3_audio_class, *p_wc3_ambient_music_volume);
        }
        //this needs to be set to null to remove the highlight colour from the talking ships target rect.
        *pp_wc3_inflight_audio_ship_ptr_for_rect_colour = nullptr;
        delete pMovie_vlc_Inflight;
        pMovie_vlc_Inflight = nullptr;
        Debug_Info_Movie("Inflight_Movie_Unload done");
    }
}


//_______________________________________________________
static void __declspec(naked) inflight_movie_unload(void) {

    __asm {
        pushad

        call Inflight_Movie_Unload

        popad

        mov eax, p_wc3_inflight_draw_buff
        cmp dword ptr ds : [eax] , ebx
        ret
    }
}


//______________________________________
static LONG Inflight_Movie_Audio_Check() {
    //check if the hd movie has audio, and if so lower the volume of the background music while playing.
    if (*p_wc3_inflight_audio_ref == 0 && pMovie_vlc_Inflight && pMovie_vlc_Inflight->HasAudio()) {
        Debug_Info_Movie("Inflight_Movie_Check_Audio HasAudio - volume lowered");
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


//_________________________________________________________________________________
static void Movie_Clear_Choice_Background(BYTE* fBuff, DWORD subY, DWORD subHeight) {
    surface_gui->Clear_Texture(0x00);
}


//______________________________________________________________________________________________________________________________
static void Movie_Draw_Choice_Text(DRAW_BUFFER_MAIN* p_toBuff, LONG x, LONG y, DWORD unk1, char* text_buff, BYTE* p_pal_offsets) {

    if (surface_gui == nullptr)
        return;

    LONG surface_width = (LONG)surface_gui->GetWidth();
    LONG surface_height = (LONG)surface_gui->GetHeight();

    bool is_top = false;
    if (y < 240)
        is_top = true;

    LONG movie_height = 0;
    if (pMovie_vlc) {
        DrawSurface* surface = pMovie_vlc->Get_Currently_Playing_Surface();
        movie_height = (LONG)surface->GetScaledHeight();
    }
    else if (surface_movieXAN)
        movie_height = (LONG)surface_movieXAN->GetScaledHeight();

    LONG text_y = 0;
    LONG text_height = 18;
    LONG black_bar_height = (LONG)(((float)surface_height / clientHeight) * ((clientHeight - movie_height) / 2));

    if (black_bar_height >= text_height) {
        text_y = (clientHeight - movie_height) / 4;
        text_y = (surface_height * text_y) / clientHeight;

        if (is_top)//draw text in the black area above the movie if there is room.
            text_y -= text_height / 2;
        else//draw text in the black area under the movie if there is room.
            text_y = surface_height - text_y - text_height / 2;
    }
    else {
        if (is_top)//otherwise draw text over the movie at the top rather than overlapping the black bar.
            text_y = black_bar_height;
        else//otherwise draw text over the movie at the bottom rather than overlapping the black bar.
            text_y = surface_height - black_bar_height - text_height;
    }

    if (text_y < 0)
        text_y = 0;
    else if (text_y > surface_height - text_height)
        text_y = surface_height - text_height;

    BYTE* pSurface = nullptr;
    LONG surface_pitch = 0;

    if (surface_gui->Lock((VOID**)&pSurface, &surface_pitch) != S_OK)
        return;
    
    DRAW_BUFFER db{
        pSurface,
        surface_pitch - 1, surface_height - 1,0,0
    };
    DRAW_BUFFER_MAIN dbm{
        &db,
        0,0,surface_width - 1,surface_height - 1
    };

    wc3_draw_text_to_buff(&dbm, x, text_y, unk1, text_buff, p_pal_offsets);
    surface_gui->Unlock();
}


//____________________________________________________________________________________________________________________________________
static void Movie_Draw_Choice_Text_Top(DRAW_BUFFER_MAIN* p_toBuff, DWORD x, DWORD y, DWORD unk1, char* text_buff, BYTE* p_pal_offsets) {
    
    if (surface_gui)//to clear choice text if resizing during conversation. 
        surface_gui->Clear_Texture(0x0);
    Movie_Draw_Choice_Text(p_toBuff, x, y, unk1, text_buff, p_pal_offsets);
}


//_______________________________________________________________________________________________________________________________________
static void Movie_Draw_Choice_Text_Bottom(DRAW_BUFFER_MAIN* p_toBuff, DWORD x, DWORD y, DWORD unk1, char* text_buff, BYTE* p_pal_offsets) {

    Movie_Draw_Choice_Text(p_toBuff, x, y, unk1, text_buff, p_pal_offsets);
    Display_Dx_Present(PRESENT_TYPE::movie);
}


//__________________________________________________________________________________________________________________________________________
static void Movie_Draw_Choice_Text_Highlight(DRAW_BUFFER_MAIN* p_toBuff, DWORD x, DWORD y, DWORD unk1, char* text_buff, BYTE* p_pal_offsets) {

    Movie_Draw_Choice_Text(p_toBuff, x, y, unk1, text_buff, p_pal_offsets);
    Display_Dx_Present(PRESENT_TYPE::movie);
}


bool subtitle_has_2_lines = false;
bool subtitle_2nd_line_check = false;


//___________________________________________________________________________________________________________________________________________
static void Movie_Draw_Subtitle_Text_Line_Two(DRAW_BUFFER_MAIN* p_toBuff, DWORD x, DWORD y, DWORD unk1, char* text_buff, BYTE* p_pal_offsets) {
    
    wc3_draw_text_to_buff(p_toBuff, x, y, unk1, text_buff, p_pal_offsets);
    
    //set flag if a second line of txt is draw to subtitle buffer.
    subtitle_2nd_line_check = true;
}


//___________________________________________________________________________________________________________________________________________
static void Movie_Draw_Subtitle_Text_Line_One(DRAW_BUFFER_MAIN* p_toBuff, DWORD x, DWORD y, DWORD unk1, char* text_buff, BYTE* p_pal_offsets) {
    
    wc3_draw_text_to_buff(p_toBuff, x, y, unk1, text_buff, p_pal_offsets);

    if (subtitle_2nd_line_check) {
        subtitle_has_2_lines = true;
        subtitle_2nd_line_check = false;
    }
    else
        subtitle_has_2_lines = false;
}


//_________________________________________________________________________________
static void Movie_Draw_Subtitle_Buffer(BYTE* to_Buff, BYTE* from_Buff, LONG height) {
    if (!surface_movieXAN)
        return;

    if (surface_gui == nullptr)
        return;

    //to clear choice text if resizing during conversation. 
    surface_gui->Clear_Texture(0x0);

    LONG movie_height = (LONG)surface_movieXAN->GetScaledHeight();
    LONG surface_width = (LONG)surface_gui->GetWidth();
    LONG surface_height = (LONG)surface_gui->GetHeight();

    LONG text_y = 0;

    // reduce height of buffer if subtitle has only one line.
    if (!subtitle_has_2_lines)
        height = 18;

    LONG black_bar_height = (LONG)(((float)surface_height / clientHeight) * ((clientHeight - movie_height) / 2));

    if (black_bar_height >= height) {
        text_y = (clientHeight - movie_height) / 4;
        text_y = (surface_height * text_y) / clientHeight;
        //draw text in the black area under the movie if there is room.
        text_y = surface_height - text_y - height / 2;
    }
    else {
        //otherwise draw text over the movie at the bottom rather than overlapping the black bar.
        text_y = surface_height - black_bar_height - height;
    }

    if (text_y < 0)
        text_y = 0;
    else if (text_y > surface_height - height)
        text_y = surface_height - height;

    to_Buff += text_y * *p_wc3_main_surface_pitch;

    for (LONG y = 0; y < height; y++) {
        for (LONG x = 0; x < 640; x++)
            to_Buff[x] = from_Buff[x];

        from_Buff += 640;
        to_Buff += *p_wc3_main_surface_pitch;
    }
}


//_________________________
void Modifications_Movies() {
    
    //replace direct draw - draw movie frame func
    MemWrite8(0x444850, 0x83, 0xE9);
    FuncWrite32(0x444851, 0x565304EC, (DWORD)&DrawVideoFrame);

    //Keep 320 movie width - prevent scale to 640
    MemWrite32(0x41C3E7, 0x01, 0x0);

    //in draw movie func, jump over direct draw stuff
    MemWrite8(0x41B9AD, 0x74, 0xEB);

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

    //clear conversation choice text buffers
    MemWrite8(0x4147D3, 0xE8, 0x90);
    MemWrite32(0x4147D4, 0xFFFF10B8, 0x90909090);
    FuncReplace32(0x4147EC, 0xFFFF10A0, (DWORD)&Movie_Clear_Choice_Background);

    MemWrite8(0x414A3F, 0xE8, 0x90);
    MemWrite32(0x414A40, 0xFFFF0E4C, 0x90909090);
    FuncReplace32(0x414A58, 0xFFFF0E34, (DWORD)&Movie_Clear_Choice_Background);

    //draw conversation text
    FuncReplace32(0x414B35, 0x0608A8, (DWORD)&Movie_Draw_Choice_Text_Top);
    FuncReplace32(0x414B75, 0x060868, (DWORD)&Movie_Draw_Choice_Text_Bottom);
    FuncReplace32(0x414CA1, 0x06073C, (DWORD)&Movie_Draw_Choice_Text_Highlight);
    //jump redundant blits after drawing choice text.
    MemWrite16(0x414CAF, 0x4B6A, 0x2AEB);//JMP SHORT 00414CDB

    //draw original movie subtitles
    FuncReplace32(0x446937, 0x02EAA6, (DWORD)&Movie_Draw_Subtitle_Text_Line_Two);
    FuncReplace32(0x446959, 0x02EA84, (DWORD)&Movie_Draw_Subtitle_Text_Line_One);

    MemWrite8(0x41B9A2, 0x52, 0x51);//PUSH ECX, un-altered to buffer pointer.
    FuncReplace32(0x41B9A4, 0x02D8, (DWORD)&Movie_Draw_Subtitle_Buffer);

    // HD Movies-----------------------------------------------
    FuncReplace32(0x414940, 0x765C, (DWORD)&Set_Conversation_Decision_Text_Colours);
    //jump over other  
    MemWrite16(0x414944, 0xE850, 0x17EB);//JMP SHORT 0041495D
    MemWrite32(0x414946, 0xFFFFCAB6, 0x90909090);

    //Light and dark offsets set to match those in wc4 for convenience.
    //Set conversation decision text colour palette offset: Light
    MemWrite8(0x414974, 0xA0, 0xB8);
    MemWrite32(0x414975, 0x4AA74E, 0xFC);
    //Set conversation decision text colour palette offset: Dark
    MemWrite16(0x414981, 0x0D8A, 0xB990);
    MemWrite32(0x414983, 0x4AA84E, 0xFD);

    //Attempt to play HD movie
    MemWrite16(0x41C240, 0xEC81, 0xE890);
    FuncWrite32(0x41C242, 0x017C, (DWORD)&play_hd_movie);

    //play alternate hires movies
    FuncReplace32(0x41C59E, 0x03A1FE, (DWORD)&play_movie_sequence);

    //fade out effect for movies
    FuncReplace32(0x41C622, 0x057A, (DWORD)&movie_fade_out);

    MemWrite16(0x422EA5, 0x3D83, 0xE890);
    FuncWrite32(0x422EA7, 0x4AB318, (DWORD)&play_inflight_movie);
    MemWrite8(0x422EAB, 0x00, 0x90);

    MemWrite16(0x423085, 0x1D39, 0xE890);
    FuncWrite32(0x423087, 0x4AB318, (DWORD)&inflight_movie_unload);

    MemWrite16(0x433731, 0x3D80, 0xE890);
    FuncWrite32(0x433733, 0x4A3338, (DWORD)&inflight_movie_audio_check);
    MemWrite8(0x433737, 0x00, 0x90);
    //---------------------------------------------------------
}