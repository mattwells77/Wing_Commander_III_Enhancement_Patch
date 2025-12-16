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

#include "pch.h"

#include "libvlc_Music.h"
#include "configTools.h"
#include "wc3w.h"
#include "version.h"


LibVlc_Music* p_Music_Player = nullptr;


//___________________________________________
static const char* Get_Alt_Music_ConfigPath() {

    static bool config_path_set = false;
    static std::string alt_music_config_path;
    if (!config_path_set) {
        config_path_set = true;

        char* pAltMusicPath = new char[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, pAltMusicPath);
        alt_music_config_path.append(pAltMusicPath);
        alt_music_config_path.append("\\");
        delete[] pAltMusicPath;

        size_t len = _countof(VER_PRODUCTNAME_STR);
        char* s_product_name = new char[len];
        size_t num_bytes = 0;
        wcstombs_s(&num_bytes, s_product_name, len, VER_PRODUCTNAME_STR, _TRUNCATE);
        alt_music_config_path.append(s_product_name);
        delete[] s_product_name;
        alt_music_config_path.append("_alt_music.ini");

    }
    return alt_music_config_path.c_str();
}


//_________________________________________________________
static std::string Get_Alt_Tune_Path(const char* tune_name) {

    char* pAltTunePath = new char[MAX_PATH];
    GetPrivateProfileStringA(tune_name, "path", nullptr, pAltTunePath, MAX_PATH, Get_Alt_Music_ConfigPath());
    std::string s_alt_tune_path = pAltTunePath;
    delete[] pAltTunePath;

    return s_alt_tune_path;
}


//________________________________________________________
static LONG Get_Alt_Tune_Max_Volume(const char* tune_name) {

    return GetPrivateProfileIntA(tune_name, "max_volume", 100, Get_Alt_Music_ConfigPath());
}


//_________________________________________________
static LONG Calulate_Volume(LONG vol, LONG max_vol) {
    //adjust the volume level of the tune in relation to the range of the original game volume.
    LONG volume = max_vol * vol / MUSIC_VOLUME_ORI_MAX;
    if (volume < 0)
        volume = 0;
    else if (volume > 100)
        volume = 100;
    return volume;
}


//___________________________________________________________________
static int media_open_cb(void* opaque, void** datap, uint64_t* sizep) {
    Debug_Info_Music("LibVlc_Music: media_open_cb");

    TUNE_DATA* tune_data = static_cast<TUNE_DATA*>(opaque);
    if (!tune_data) {
        Debug_Info_Error("LibVlc_Music: media_open_cb tune_data null");
        return 1;
    }
    Debug_Info_Music("LibVlc_Music: media_open_cb buffer size: %d", tune_data->len);
    tune_data->pos = 0;
    *datap = tune_data;
    *sizep = (uint64_t)tune_data->len;
    return 0;
};


//________________________________________________________________________
static ssize_t media_read_cb(void* opaque, unsigned char* buf, size_t len) {
    Debug_Info_Music("LibVlc_Music: media_read_cb");

    TUNE_DATA* tune_data = static_cast<TUNE_DATA*>(opaque);
    if (!tune_data)
        return 0;

    if (len + tune_data->pos > tune_data->len)
        len = tune_data->len - tune_data->pos;

    if (tune_data->pos >= tune_data->len)
        return 0;

    memcpy(buf, tune_data->data + tune_data->pos, len);
    tune_data->pos += len;
    Debug_Info_Music("LibVlc_Music: media_read_cb bytes read:%d, bytes remaining: %d", len, tune_data->len - tune_data->pos);
    return len;

};


//______________________________________
static void media_close_cb(void* opaque) {
    Debug_Info_Music("LibVlc_Music: media_close_cb");

    TUNE_DATA* tune_data = static_cast<TUNE_DATA*>(opaque);
    if (tune_data)
        tune_data->pos = 0;
};


//_____________________________________________________
static int media_seek_cb(void* opaque, uint64_t offset) {
    Debug_Info_Music("LibVlc_Music: media_seek_cb");

    TUNE_DATA* tune_data = static_cast<TUNE_DATA*>(opaque);
    if (!tune_data)
        return 0;
    if (offset < (uint64_t)tune_data->pos)
        tune_data->pos = (size_t)offset;
    else
        tune_data->pos = 0;
    Debug_Info_Music("LibVlc_Music: media_seek_cb new pos: %d", tune_data->pos);
    return 0;
}


//_______________________________________________________
LibVlc_Music::LibVlc_Music(MUSIC_CLASS* p_in_music_class) {
    
    for (int i = 0; i < NUM_TUNES; i++)
        tune[i] = nullptr;

    p_music_class = p_in_music_class;
    Debug_Info_Music("LibVlc_Music current:%d", p_music_class->header.current_tune);

    using namespace std::placeholders; // for `_1`

    mediaPlayer.eventManager().onPlaying(std::bind(&LibVlc_Music::on_play, this));
    mediaPlayer.eventManager().onStopped(std::bind(&LibVlc_Music::on_stopped, this));

    mediaPlayer.eventManager().onEncounteredError(std::bind(&LibVlc_Music::on_encountered_error, this));

    //mediaPlayer.eventManager().onTimeChanged(std::bind(&LibVlc_Music::on_time_changed, this, _1));
    //mediaPlayer.eventManager().onBuffering(std::bind(&LibVlc_Music::on_buffering, this, _1));

#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
    is_stop_set = false;
    mediaPlayer.eventManager().onStopping(std::bind(&LibVlc_Movie::on_end_reached, this));
#else
    mediaPlayer.eventManager().onEndReached(std::bind(&LibVlc_Music::on_end_reached, this));
#endif
    current_tune = p_music_class->header.current_tune;
    paused = false;
    position = 0;
    is_playing = false;
};


//______________________________
bool LibVlc_Music::Update_Tune() {
    
    static LONG last_volume = 0;

    if (*p_wc3_music_volume_current != last_volume) {
        if (current_tune < NUM_TUNES) {
            Debug_Info_Music("LibVlc_Music: Update_Tune volume: %d", *p_wc3_music_volume_current);
            mediaPlayer.setVolume(Calulate_Volume(*p_wc3_music_volume_current*10, tune[current_tune]->max_volume));
            last_volume = *p_wc3_music_volume_current;
        }
    }

    p_music_class->header.current_tune = p_music_class->header.requested_tune;
    if (current_tune == p_music_class->header.current_tune)
        return 1;

    Debug_Info_Music("LibVlc_Music: Update_Tune current:%d", p_music_class->header.current_tune);
    if (p_music_class->header.current_tune >= NUM_TUNES) {
        mediaPlayer.stop();
        Debug_Info_Music("LibVlc_Music: Update_Tune - stop current:%d", current_tune);
        return false;
    }

    // if current tune is valid and set to be uninterupted than continue playing until done.
    if (current_tune < NUM_TUNES) {
        MUSIC_FILE* music_file = &p_music_class->file[current_tune];

        if (is_playing && (BYTE)(music_file->flags<<8))
            return is_playing;
    }

    // set and play next tune.
    current_tune = p_music_class->header.current_tune;
    MUSIC_FILE* music_file = &p_music_class->file[current_tune];


    std::string alt_tune = Get_Alt_Tune_Path(tune[current_tune]->name);

    libvlc_media_t* m_vlcMedia = nullptr;
    if (GetFileAttributesA(alt_tune.c_str()) != INVALID_FILE_ATTRIBUTES) {
        Debug_Info_Music("LibVlc_Music: Update_Tune Alt New media: %s", alt_tune.c_str());
        m_vlcMedia = libvlc_media_new_path(vlc_instance, alt_tune.c_str());
        // set the max volume.
        tune[current_tune]->max_volume = Get_Alt_Tune_Max_Volume(tune[current_tune]->name);
    }
    else
        m_vlcMedia = libvlc_media_new_callbacks(vlc_instance, media_open_cb, media_read_cb, media_seek_cb, media_close_cb, tune[current_tune]);

    if (m_vlcMedia == nullptr)
        Debug_Info_Error("LibVlc_Music: Update_Tune, Media creation FAILED");
    libvlc_media_player_set_media(mediaPlayer, m_vlcMedia);

    Debug_Info_Music("LibVlc_Music: Update_Tune volume New media: %d", *p_wc3_music_volume_current);
    mediaPlayer.setVolume(Calulate_Volume(*p_wc3_music_volume_current*10, tune[current_tune]->max_volume));

    return  mediaPlayer.play();
}


//_____________________________________________________________________
bool LibVlc_Music::Load_Tune_Data(int tune_num, MUSIC_FILE* music_file) {
    Debug_Info_Music("LibVlc_Music: Load_Tune_Data tune: %d", tune_num);
    Debug_Info_Music("LibVlc_Music: Load_Tune_Data tune: %s, pos1: %d, pos2: %d, size: %d", music_file->file.path, music_file->file.file_pos1, music_file->file.file_pos2, music_file->file.file_size);
    if (tune[tune_num])
        delete tune[tune_num];
    tune[tune_num] = new TUNE_DATA(music_file->file.file_size);
    char* name_start = strrchr(music_file->file.path, '\\');

    if (name_start) {
        char* tune_name = new char [80] {0};
        name_start++;
        int i = 0;
        while (i<80 && name_start[i] != '\0' && name_start[i] != '.') {
            tune_name[i] = name_start[i];
            i++;
        }
        Debug_Info_Music("LibVlc_Music: Load_Tune_Data tune name: %s", tune_name);
        size_t name_len = strlen(tune_name);
        if (name_len) {
            tune[tune_num]->name = new char[name_len + 1];
            strncpy_s(tune[tune_num]->name, name_len + 1, tune_name, name_len + 1);
        }
        delete[]tune_name;
    }
    size_t bytes_read = 0;
    if (bytes_read = wc3_file_read(music_file, tune[tune_num]->data, tune[tune_num]->len) == tune[tune_num]->len) {
        WORD* p_audio_format = (WORD*)(tune[tune_num]->data + 0x14);
        if (tune[tune_num]->data)
            Debug_Info_Music("LibVlc_Music: Load_Tune_Data tune: %d, audio format: %X", tune_num, *p_audio_format);

        //Debug_Info_Music("LibVlc_Music: Load_Tune_Data GOOD Data: %s, pos1: %d, pos2: %d, size: %d, bytes_read: %d", tune[tune_num]->data, music_file->file.file_pos1, music_file->file.file_pos2, music_file->file.file_size, bytes_read);
        return true;
    }

    Debug_Info_Error("LibVlc_Music: Load_Tune_Data FAILED tune: %d", tune_num);
    return false;
}


