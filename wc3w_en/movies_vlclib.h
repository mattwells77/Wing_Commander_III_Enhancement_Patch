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

#include "Display_DX11.h"
#include "vlcpp/vlc.hpp"


extern VLC::Instance vlc_instance;

//BOOL Create_Movie_Path(const char* mve_path, int branch, std::string* p_retPath);


struct MOVIE_STATE {
    LONG branch;
    LONG list_num;
    bool isPlaying;
    bool hasPlayed;
    bool isError;
};


class LibVlc_Movie {
public:
    LibVlc_Movie(std::string mve_path, LONG* branch_list, LONG branch_list_num);

    ~LibVlc_Movie() {
        Stop();
        if (surface)
            delete surface;
        surface = nullptr;
        if (next)
            delete next;
        Debug_Info("LibVlc_Movie: destroy: %s", path.c_str());
    };

    bool Play();
    bool Play(LARGE_INTEGER play_end_time_in_ticks);

    void Pause(bool pause) {
        if (isPlaying) {
            paused = pause;
            //using this function primarily when window is resizing or loosing focus.
            //need to use stop here instead of pause as this was causing crashes.
            if (pause) {
                position = mediaPlayer.position();
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
                mediaPlayer.stopAsync();
#else
                mediaPlayer.stop();
#endif
            }
            else {
                mediaPlayer.play();
                mediaPlayer.setPosition(position);
            }
            //mediaPlayer.setPause(pause);
        }
        if (next)
            next->Pause(pause);
        //isPlaying = false;
    };
    void SetMedia(std::string path);

    void Stop() {

#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
        mediaPlayer.stopAsync();
#else
        mediaPlayer.stop();
#endif
        isPlaying = false;

        Debug_Info("LibVlc_Movie: Stop: %s", path.c_str());
    };
    bool IsPlaying() const {
        return isPlaying;
    }
    bool IsPlaying(LONG* p_current_branch, LONG* p_current_branch_list_num) {

        if (hasPlayed && !isPlaying && next)
            return next->IsPlaying(p_current_branch, p_current_branch_list_num);

        if (p_current_branch)
            *p_current_branch = branch;
        if (p_current_branch_list_num)
            *p_current_branch_list_num = list_num;

        return isPlaying;
    }
    bool IsPlaying(MOVIE_STATE* p_movie_state) {

        if (hasPlayed && !isPlaying && next)
            return next->IsPlaying(p_movie_state);

        if (p_movie_state) {
            p_movie_state->branch = branch;
            p_movie_state->list_num = list_num;
            p_movie_state->isPlaying = isPlaying;
            p_movie_state->hasPlayed = hasPlayed;
            p_movie_state->isError = isError;
        }

        return isPlaying;
    }
    bool HasPlayed() const {
        return hasPlayed;
    }
    bool HasPlayed(LONG* p_current_branch, LONG* p_current_branch_list_num) {
        if (hasPlayed) {
            if (p_current_branch)
                *p_current_branch = branch;
            if (p_current_branch_list_num)
                *p_current_branch_list_num = list_num;
            if (next)
                return next->HasPlayed(p_current_branch, p_current_branch_list_num);
        }
        return hasPlayed;
    }
    bool IsError() {
        if (!isError && next)
            return next->IsError();
        return isError;
    };
    void SetScale() {
        if (surface) {
            surface->ScaleToScreen(SCALE_TYPE::fit);
            Debug_Info("LibVlc_Movie: SetScale: %s", path.c_str());
        }
        if (next)
            next->SetScale();
    }
protected:
private:
    LibVlc_Movie* next;
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
    VLC::MediaPlayer mediaPlayer = VLC::MediaPlayer(vlc_instance);
#else
    VLC::MediaPlayer mediaPlayer = VLC::MediaPlayer(vlc_instance);
#endif
    std::string path;
    LONG branch;
    LONG list_num;
    bool isPlaying;
    bool hasPlayed;
    bool isError;
    bool paused;
    bool initialised_for_play;
    float position;
    GEN_SURFACE* surface;

    bool InitialiseForPlay_Start();
    void InitialiseForPlay_End();
    void Initialise_Subtitles();


    void on_play() {
        Initialise_Subtitles();
        Debug_Info("LibVlc_Movie: On Play stopped: %s", path.c_str());

    };
    void on_stopped() {
        if (!paused) {
            isPlaying = false;
            //hasPlayed = true;
            Debug_Info("LibVlc_Movie: OnStop stopped: %s", path.c_str());
        }
    };
    void on_encountered_error() {
        isError = true;
        Debug_Info("LibVlc_Movie: Error Encountered !!!: %s", path.c_str());
    };
    void on_buffering(float percent) {
       // if (percent == 0.0f)
       //     Initialise_Subtitles();
        
        //if (percent == 100.0f && !initialised_for_play) {
       //     Debug_Info("LibVlc_Movie: Buffering %f: %s", percent, path.c_str());
        //    InitialiseForPlay_End();
        //}
    };
    void on_end_reached() {
        Debug_Info("LibVlc_Movie: end reached: %s", path.c_str());
        if (next)
            next->Play();
        isPlaying = false;
        hasPlayed = true;

    };
    void on_time_changed(libvlc_time_t time_ms);
    void* lock(void** planes) {
        if (!initialised_for_play) {
            Debug_Info("LibVlc_Movie: initialised_for_play ON FIRST LOCK %s", path.c_str());
            InitialiseForPlay_End();
        }
        //Debug_Info("lock: %s", path.c_str());
        BYTE* pSurface = nullptr;
        LONG pitch = 0;

        if (surface->Lock((VOID**)&pSurface, &pitch) != S_OK) {
            Debug_Info("LibVlc_Movie: LOCK FAILED %s", path.c_str());
            return nullptr;
        }
        *planes = (VOID**)pSurface;
        return nullptr;

    };
    void unlock(void* picture, void* const* planes) {
        //Debug_Info("unlock: %s", path.c_str());
        surface->Unlock();
        if (!initialised_for_play || !isPlaying)
            return;
        MovieRT_SetRenderTarget();
        surface->Display();
    };
    void display(void* picture) const {
        if (!initialised_for_play || !isPlaying)
            return;
        Display_Dx_Present(PRESENT_TYPE::movie);
    };
    uint32_t format(char* chroma, uint32_t* width, uint32_t* height, uint32_t* p_pitch, uint32_t* lines) {
        memcpy(chroma, "RV32", 4);
        Debug_Info("LibVlc_Movie: setVideoFormatCallbacks w:%d, h:%d", *width, *height);
        if (!surface || *width != surface->GetWidth() || *height < surface->GetHeight() || surface->GetPixelWidth() != 4) {
            if (surface)
                delete surface;
            surface = new GEN_SURFACE(*width, *height, 32);
            surface->ScaleToScreen(SCALE_TYPE::fit);
            Debug_Info("LibVlc_Movie: surface created: %s", path.c_str());
        }
        BYTE* pSurface = nullptr;
        LONG pitch = 0;
        if (surface->Lock((VOID**)&pSurface, &pitch) != S_OK)
            return 0;
        surface->Unlock();
        *width = surface->GetWidth();
        *height = surface->GetHeight();
        *p_pitch = pitch;
        *lines = *height;
        return 1;
    };
    void cleanup() {
        Debug_Info("LibVlc_Movie: cleanup: %s", path.c_str());
        if (surface)
            delete surface;
        surface = nullptr;
    };

};

BOOL Get_Movie_Name_From_Path(const char* mve_path, std::string* p_retPath);
