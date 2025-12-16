/*
The MIT License (MIT)
Copyright © 2025 Matt Wells

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

#include "libvlc_common.h"
#include "Display_DX11.h"
#include "wc3w.h"

//the original max game volume.
#define MUSIC_VOLUME_ORI_MAX        100
//adjusted max volume of the original tunes when played with vlc.
#define MUSIC_VOLUME_VLC_MAX        100
//adjusted volume subtraction for comms when played with vlc.
//#define MUSIC_VOLUME_VLC_TALK_SUB   4

class TUNE_DATA {
public:
    BYTE* data;
    size_t len;
    size_t pos;
    char* name;
    LONG max_volume;
    TUNE_DATA(size_t size) {
        data = new BYTE[size];
        len = size;
        pos = 0;
        name = nullptr;
        max_volume = MUSIC_VOLUME_VLC_MAX;
    }
    ~TUNE_DATA() {
        if (data)
            delete[] data;
        data = nullptr;
        if (name)
            delete[] name;
        name = nullptr;
    }
};


//int media_open_cb(void* opaque, void** datap, uint64_t* sizep);
//ssize_t media_read_cb(void* opaque, unsigned char* buf, size_t len);
//void media_close_cb(void* opaque);

//int media_open_cb(void* opaque, void** datap, uint64_t* sizep);
//ssize_t media_read_cb(void* opaque, unsigned char* buf, size_t len);
//void media_close_cb(void* opaque);


class LibVlc_Music {
public:
    LibVlc_Music(MUSIC_CLASS* p_in_music_class);

    ~LibVlc_Music() {
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
        is_stop_set = true;
        mediaPlayer.stopAsync();
#else
        mediaPlayer.stop();
#endif
        for (int i = 0; i < NUM_TUNES; i++) {
            if (tune[i])
                delete tune[i];
            tune[i] = nullptr;
        }
        Debug_Info_Music("~LibVlc_Music: DONE");
    };

    bool Update_Tune();
    void Pause(bool pause) {
        if (is_playing) {
            paused = pause;
            //using this function primarily when window is resizing or loosing focus.
            //need to use stop here instead of pause as this was causing crashes.
            if (pause) {
                position = mediaPlayer.position();
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
                mediaPlayer.stopAsync();
                while (is_vlc_playing)//ensure vlc is done with surface. 
                    Sleep(0);
#else
                mediaPlayer.stop();
#endif
            }
            else {
                mediaPlayer.play();
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
                mediaPlayer.setPosition(position, false);
#else
                mediaPlayer.setPosition(position);
#endif
            }
        }
    };

    void Stop() {
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
        is_stop_set = true;
        mediaPlayer.stopAsync();
#else
        mediaPlayer.stop();
#endif
    };
    bool IsPlaying() const {
        return is_playing;
    };

    bool Load_Tune_Data(int tune_num, MUSIC_FILE* file);
    MUSIC_CLASS* Get_Music_Class() { return p_music_class; };
protected:
private:


#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
    VLC::MediaPlayer mediaPlayer = VLC::MediaPlayer(vlc_instance);
#else
    VLC::MediaPlayer mediaPlayer = VLC::MediaPlayer(vlc_instance);
#endif

    MUSIC_CLASS* p_music_class;
    DWORD current_tune;
    bool paused;
    float position;
    bool is_playing;
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
    bool is_stop_set;//when stopping after being deliberatly stopped rather than reaching the end of the movie. 
#endif
    TUNE_DATA* tune[NUM_TUNES];

    void on_play() {
        Debug_Info_Music("LibVlc_Music: on_play Play started");
        is_playing = true;
    };
    void on_stopped() {
        if (!paused) {
            Debug_Info_Music("LibVlc_Music: on_stopped Play stopped");
            p_music_class->header.current_tune = -1;
            p_music_class->header.requested_tune = -1;
            current_tune = -1;
            is_playing = false;
        }
    };
    void on_encountered_error() {
        Debug_Info_Error("LibVlc_Music: Error Encountered !!! %s", libvlc_errmsg());
    };

    void on_end_reached() {
        Debug_Info_Music("LibVlc_Music: end reached");
    };

    void on_time_changed(libvlc_time_t time_ms) {
        Debug_Info_Music("LibVlc_Music: on time changed %d", (int)time_ms);
    };
};

extern LibVlc_Music* p_Music_Player;