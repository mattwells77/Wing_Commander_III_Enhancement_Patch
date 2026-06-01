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


//__________________________________________________
static std::string Get_Alt_Tune_Path(DWORD tune_num) {

    char c_section[3]{0};
    sprintf_s(c_section, "%02d", tune_num);

    char c_key[9]{ 0 };
    if (tune_num == 29)//recroom1
        sprintf_s(c_key, "path_cd%d", *p_wc3_current_cd_num + 1);
    else
        sprintf_s(c_key, "path");
    
    Debug_Info_Music("Get_Alt_Tune_Path - INI: section: %s key: %s", c_section, c_key);

    char* pAltTunePath = new char[MAX_PATH];
    GetPrivateProfileStringA(c_section, c_key, nullptr, pAltTunePath, MAX_PATH, Get_Alt_Music_ConfigPath());
    std::string s_alt_tune_path = pAltTunePath;
    delete[] pAltTunePath;

    return s_alt_tune_path;
}


//_________________________________________________
static LONG Get_Alt_Tune_Max_Volume(DWORD tune_num) {

    char c_section[3]{ 0 };
    sprintf_s(c_section, "%02d", tune_num);

    return GetPrivateProfileIntA(c_section, "max_volume", 100, Get_Alt_Music_ConfigPath());
}


//_________________________________________________
static LONG Calulate_Volume(LONG vol, LONG max_vol) {
    //adjust the volume level of the tune in relation to the range of the original game volume.
    LONG volume = max_vol * vol / MUSIC_VOLUME_ORI_MAX;
    if (volume < 0)
        volume = 0;
    else if (volume > 100)
        volume = 100;
    
    //fix volume to better match that of the original.
    double fvol_mod = volume * 0.01f;
    fvol_mod = sin(fvol_mod * 1.57079632679489661923);// PI/2
    volume = (LONG)(fvol_mod * 100);

    Debug_Info_Music("Calulate_Volume volume: %d %f", volume, fvol_mod);
    return volume;
}


//___________________________________________________________________
static int media_open_cb(void* opaque, void** datap, uint64_t* sizep) {
    //Debug_Info_Music("LibVlc_Music: media_open_cb");

    TUNE_DATA* tune_data = static_cast<TUNE_DATA*>(opaque);
    if (!tune_data) {
        Debug_Info_Error("LibVlc_Music: media_open_cb tune_data null");
        return 1;
    }
    Debug_Info_Music("LibVlc_Music: media_open_cb: %s buffer size: %d", tune_data->name, tune_data->len);
    tune_data->pos = 0;
    *datap = tune_data;
    *sizep = (uint64_t)tune_data->len;
    return 0;
};


//________________________________________________________________________
static ssize_t media_read_cb(void* opaque, unsigned char* buf, size_t len) {
    //Debug_Info_Music("LibVlc_Music: media_read_cb");

    TUNE_DATA* tune_data = static_cast<TUNE_DATA*>(opaque);
    if (!tune_data)
        return 0;

    if (len + tune_data->pos > tune_data->len)
        len = tune_data->len - tune_data->pos;

    if (tune_data->pos >= tune_data->len)
        return 0;

    memcpy(buf, tune_data->data + tune_data->pos, len);
    tune_data->pos += len;
    //Debug_Info_Music("LibVlc_Music: media_read_cb bytes read:%d, bytes remaining: %d", len, tune_data->len - tune_data->pos);
    return len;

};


//______________________________________
static void media_close_cb(void* opaque) {
    //Debug_Info_Music("LibVlc_Music: media_close_cb");

    TUNE_DATA* tune_data = static_cast<TUNE_DATA*>(opaque);
    if (!tune_data)
        return;
    tune_data->pos = 0;
    Debug_Info_Music("LibVlc_Music: media_close_cb: %s", tune_data->name);
};


//_____________________________________________________
static int media_seek_cb(void* opaque, uint64_t offset) {
    

    TUNE_DATA* tune_data = static_cast<TUNE_DATA*>(opaque);
    if (!tune_data)
        return 0;
    //Debug_Info_Music("LibVlc_Music: media_seek_cb pos: %d, offset: %d", tune_data->pos, (size_t)offset);
    if (offset < (uint64_t)tune_data->len)
        tune_data->pos = (size_t)offset;
    else
        tune_data->pos = 0;
    //Debug_Info_Music("LibVlc_Music: media_seek_cb new pos: %d", tune_data->pos);
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
    last_volume = -1;
};


//________________________________
bool LibVlc_Music::Update_Volume() {
    
    if (is_playing  && *p_wc3_music_volume_current != last_volume) {
        if (current_tune < NUM_TUNES) {
            last_volume = *p_wc3_music_volume_current;
            Debug_Info_Music("LibVlc_Music: Update_Tune volume: %d", last_volume);
            return mediaPlayer.setVolume(Calulate_Volume(last_volume * 10, tune[current_tune]->max_volume));
        }
    }
    return false;
}


//______________________________
bool LibVlc_Music::Update_Tune() {
    
    if (paused)
        return true;

    Update_Volume();

    if (is_playing && current_tune == p_music_class->header.requested_tune)
        return is_playing;
    
    if (p_music_class->header.requested_tune >= NUM_TUNES) {
        if (is_playing) {
            Debug_Info_Music("LibVlc_Music: Update_Tune - Stop tune:%d", current_tune);
            Stop();
        }
        return false;
    }

    if (tune[p_music_class->header.requested_tune] == nullptr) {
        Debug_Info_Music("LibVlc_Music: Update_Tune - Failed to play tune: %d, tune NOT loaded", p_music_class->header.requested_tune);
        p_music_class->header.requested_tune = -1;
        return is_playing;
    }

    if (current_tune < NUM_TUNES) {
        if (is_playing) {
            // if current tune is valid and set to be uninterrupted and the next tune is uninterruptible, than continue playing until done.
            MUSIC_FILE* music_file = &p_music_class->file[current_tune];
            MUSIC_FILE* music_file_next = &p_music_class->file[p_music_class->header.requested_tune];
            if (!(music_file->flags & MUSIC_INTERUPT_TUNE) && (music_file_next->flags & MUSIC_INTERUPT_TUNE)) {
                return is_playing;
            }

            // update tune position before it's replaced by the next tune if it is interruptable.
            if (music_file->flags & MUSIC_INTERUPT_TUNE) {
                tune[current_tune]->position = mediaPlayer.position();
                p_music_class->header.previous_tune = current_tune;
            }
        }
    }
    
    // set and play next tune.
    p_music_class->header.current_tune = p_music_class->header.requested_tune;
    current_tune = p_music_class->header.current_tune;

    std::string alt_tune = Get_Alt_Tune_Path(current_tune);
 
    libvlc_media_t* m_vlcMedia = nullptr;
    if (GetFileAttributesA(alt_tune.c_str()) != INVALID_FILE_ATTRIBUTES) {
        Debug_Info_Music("LibVlc_Music: Update_Tune Alt New media: %s", alt_tune.c_str());
        m_vlcMedia = libvlc_media_new_path(vlc_instance, alt_tune.c_str());
        // set the max volume.
        tune[current_tune]->max_volume = Get_Alt_Tune_Max_Volume(current_tune);
    }
    else
        m_vlcMedia = libvlc_media_new_callbacks(vlc_instance, media_open_cb, media_read_cb, media_seek_cb, media_close_cb, tune[current_tune]);

    if (m_vlcMedia) {
        libvlc_media_player_set_media(mediaPlayer, m_vlcMedia);
        libvlc_media_release(m_vlcMedia);
    }
    else
        Debug_Info_Error("LibVlc_Music: Update_Tune, Media creation FAILED");

    mediaPlayer.play();

    Update_Volume();
    mediaPlayer.setPosition(tune[current_tune]->position);
    Debug_Info_Music("LibVlc_Music: Update_Tune - Play tune: %d, Setting position: %f", current_tune, tune[current_tune]->position);
    return is_playing;
}


#define XAN_SECTION_SIZE  1471
// Details for proccessing Xan encoded wav files were obtained from here(https://wiki.multimedia.cx/index.php/Xan_DPCM).
//______________________________________________________________________________________________________
static uint32_t Convert_Xan_Data(int num_channels, uint8_t* in_buff, int16_t* out_buff, uint32_t length) {

    uint32_t count_section = 0;
    uint32_t count_bytes_out = 0;
    int predictor[2] = { 0, 0 };
    int shifter[2] = { 4, 4 };
    int is_stereo = num_channels - 1;

    uint32_t section_length = (XAN_SECTION_SIZE - 2) * num_channels;
    uint32_t last_non_zero_byte = 0;

    for (int channel = 0; channel < num_channels; channel++) {
        predictor[channel] = *(int16_t*)in_buff;
        in_buff += 2;
    }

    int channel = 0;
    for (uint32_t i = 0; i < length; i++) {

        if (count_section == section_length) {
            for (int channel = 0; channel < num_channels; channel++) {
                predictor[channel] = *(int16_t*)in_buff;
                in_buff += 2;
                i += 2;
            }
            count_section = 0;
            shifter[0] = 4;
            shifter[1] = 4;
        }

        int8_t byte = (int8_t)*in_buff;
        if (byte != 0)
            last_non_zero_byte = count_bytes_out;

        int shift_val = byte & 3;

        if (shift_val == 3)
            shifter[channel]++;
        else
            shifter[channel] -= (2 * shift_val);

        // the shift value may not go below 0, saturate 
        if (shifter[channel] < 0)
            shifter[channel] = 0;
        else if (shifter[channel] > 15) {
            Debug_Info_Music("Convert_Xan_Data shift should not go over 15!!!, byte count:%d", count_section);
            shifter[channel] = 15;
        }

        int diff = (byte & ~3) << 8;
        diff >>= shifter[channel];

        predictor[channel] += diff;

        if (predictor[channel] < -32768)
            predictor[channel] = -32768;
        else if (predictor[channel] > 32767)
            predictor[channel] = 32767;
        
        *out_buff = predictor[channel];

        out_buff++;
        in_buff++;
        count_section++;
        count_bytes_out++;
        //toggle channel 
        channel = is_stereo - channel;
    }
    //Debug_Info_Music("Convert_Xan_Data, last_non_zero_byte:%d", last_non_zero_byte);
    return last_non_zero_byte * num_channels;
}


//__________________________________________________
static BOOL Convert_Xan_To_PCM_WAVE(TUNE_DATA* tune) {

    DWORD code = *(DWORD*)(tune->data);
    if (code != 0x46464952)// FOURCC [RIFF]
        return FALSE;

    DWORD file_size = *(DWORD*)(tune->data + 4);

    code = *(DWORD*)(tune->data + 8);
    if (code != 0x45564157) {// FOURCC [WAVE]
        Debug_Info_Error("Convert_Xan_To_PCM_WAVE: Wav RIFF has No WAVE");
        return FALSE;
    }

    BYTE* wav_format = nullptr;
    BYTE* wav_data = nullptr;
    DWORD wav_data_size = 0;

    BYTE* file_data = (tune->data + 12);
    LONGLONG remaining_size = (LONGLONG)(file_size - 4);

    DWORD chunk_size = 0;

    while (remaining_size > 0) {
        code = *(DWORD*)file_data;
        file_data += 4;
        remaining_size -= 4;
        chunk_size = *(DWORD*)file_data;
        if (code == 0x20746D66) {  // FOURCC [fmt ]
            wav_format = file_data + 4;
            //Debug_Info("Convert_Xan_To_PCM_WAVE: Found FOURCC [fmt ] size:%d", chunk_size);
        }
        else if (code == 0x61746164) {// FOURCC [data]
            wav_data = file_data + 4;
            wav_data_size = chunk_size;
            //Debug_Info("Convert_Xan_To_PCM_WAVE: Found FOURCC [data] size:%d", wav_data_size);
        }
        if (wav_format && wav_data)
            break;
        file_data += 4;
        remaining_size -= 4;
        file_data += chunk_size;
        remaining_size -= chunk_size;
    }
    if (!wav_format || !wav_data) {
        Debug_Info_Error("Convert_Xan_To_PCM_WAVE: NO format or NO data");
        return FALSE;
    }

    if (*(WORD*)wav_format != 0x594A) {//XAN audio
        //Debug_Info("Convert_Xan_To_PCM_WAVE: NOT XAN audio");
        return FALSE;
    }

    int16_t num_channels = *(int16_t*)(wav_format +0x02);

    DWORD section_length = (XAN_SECTION_SIZE - 2) * num_channels;

    DWORD un_xaned_size = wav_data_size / section_length;
    un_xaned_size = wav_data_size - un_xaned_size * 4;
    Debug_Info_Music("Convert_Xan_To_PCM_WAVE: audio_data_size: %d, un_xaned_size: %d", wav_data_size, un_xaned_size);

    DWORD num_samples = wav_data_size;
    DWORD frequency = *(DWORD*)(wav_format + 0x04);
    WORD bitsPerSample = *(WORD*)(wav_format + 0x0E);
    WORD bytePerBloc = num_channels * bitsPerSample / 8;
    DWORD bytePerSec = frequency * bytePerBloc;

    Debug_Info_Music("Convert_Xan_To_PCM_WAVE: num_Channels: %d, num_samples: %d, frequency: %d, bitsPerSample: %d, bytePerBloc: %d ,  bytePerSec: %d", num_channels, num_samples, frequency, bitsPerSample, bytePerBloc, bytePerSec);

    size_t out_wav_data_size = un_xaned_size * 2;
    size_t out_file_size = 12 + 24 + 8 + out_wav_data_size;

    BYTE* out_file_data = new BYTE[out_file_size]{ 0 };
    *(DWORD*)out_file_data = 0x46464952;                // FOURCC [RIFF]
    *(DWORD*)(out_file_data + 4) = out_file_size - 8;   // file size
    *(DWORD*)(out_file_data + 8) = 0x45564157;          // FOURCC [WAVE]

    *(DWORD*)(out_file_data + 12) = 0x20746D66;         // FOURCC [fmt ]
    *(DWORD*)(out_file_data + 16) = 16;                 // size of format section
    *(WORD*)(out_file_data + 20) = 1;                   // audio_format PCM
    *(WORD*)(out_file_data + 22) = num_channels;        // number of channels
    *(DWORD*)(out_file_data + 24) = frequency;          // sample_rate
    *(DWORD*)(out_file_data + 28) = bytePerSec;         // bytePerSec
    *(WORD*)(out_file_data + 32) = bytePerBloc;         // bytePerBloc
    *(WORD*)(out_file_data + 34) = 16;                  // bits_per_sample

    *(DWORD*)(out_file_data + 36) = 0x61746164;         // FOURCC [data];
    *(DWORD*)(out_file_data + 40) = out_wav_data_size;  // out_wav_data_size

    DWORD data_count = Convert_Xan_Data(num_channels, wav_data, (int16_t*)(out_file_data + 44), num_samples);
    Debug_Info_Music("Convert_Xan_To_PCM_WAVE: BlocSize: %d data_count: %d, junk bytes: %d", out_wav_data_size, data_count, out_wav_data_size - data_count);
    
    //mark remainder of xan section(if any) with a junk chunk.
    if (out_wav_data_size > data_count && out_wav_data_size - data_count >= 8) {
        *(DWORD*)(out_file_data + 40) = data_count;   // out_wav_data_size
        *(DWORD*)(out_file_data + 44 + data_count) = 0x4B4E554A;// FOURCC [JUNK];
        *(DWORD*)(out_file_data + 44 + data_count + 4) = out_wav_data_size - data_count - 8;// junk_size;
        Debug_Info_Music("Convert_Xan_To_PCM_WAVE: Creating junk section at end of wav, BlocSize:%d", out_wav_data_size - data_count - 8);
    }
    delete[]tune->data;
    tune->data = out_file_data;
    tune->len = out_file_size;
    return TRUE;

    
    /*FILE* file = nullptr;
    fopen_s(&file, tune->name, "wb");
    if (file) {
        fwrite(out_file_data, out_file_size, 1, file);
        fclose(file);
    }
    return TRUE;*/
}


//_____________________________________________________________________
bool LibVlc_Music::Load_Tune_Data(int tune_num, MUSIC_FILE* music_file) {
    Debug_Info_Music("LibVlc_Music: Load_Tune_Data tune_num: %d", tune_num);
    Debug_Info_Music("LibVlc_Music: Load_Tune_Data path: %s, pos1: %d, pos2: %d, size: %d", music_file->file.path, music_file->file.file_pos1, music_file->file.file_pos2, music_file->file.file_size);
    Debug_Info_Music("LibVlc_Music: Load_Tune_Data unk80: %X, flags1: %X, flags2: %X, flags3: %X", music_file->unk80, music_file->flags, music_file->DIGM_CDX_01_flag, music_file->DIGM_CDX_03_flag);
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
       
        if (tune[tune_num]->data) {
            WORD* p_audio_format = (WORD*)(tune[tune_num]->data + 0x14);
            Debug_Info_Music("LibVlc_Music: Load_Tune_Data tune: %d, audio format: %X", tune_num, *p_audio_format);
            Convert_Xan_To_PCM_WAVE(tune[tune_num]);
        }
        //Debug_Info_Music("LibVlc_Music: Load_Tune_Data GOOD Data: %s, pos1: %d, pos2: %d, size: %d, bytes_read: %d", tune[tune_num]->data, music_file->file.file_pos1, music_file->file.file_pos2, music_file->file.file_size, bytes_read);
        return true;
    }

    Debug_Info_Error("LibVlc_Music: Load_Tune_Data FAILED tune: %d", tune_num);
    return false;
}


