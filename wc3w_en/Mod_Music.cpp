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
#include "modifications.h"
#include "memwrite.h"
#include "configTools.h"
#include "wc3w.h"


//_________________________________________________
static DWORD Music_Player(MUSIC_CLASS* music_class) {
    //This function runs in a seperate thread.

    if (p_Music_Player)
        delete p_Music_Player;

    p_Music_Player = new LibVlc_Music(music_class);

    //set flags than wait for music data to load.
    music_class->header.flags |= 0x21;
    while (!(music_class->header.flags & 0xC0)) {
        Sleep(64);
        Debug_Info_Music("Music_Player Init: Waiting...");
    }
    Debug_Info_Music("Music_Player Init: DONE");

    //periodically check for tune changes and update.
    while (!(music_class->header.flags & 0x80)) {
        Sleep(64);
        p_Music_Player->Update_Tune();
    }

    if (p_Music_Player->IsPlaying()) {
        p_Music_Player->Stop();
    }

    music_class->header.flags &= 0xFFFFFFFE;

    if (p_Music_Player)
        delete p_Music_Player;
    p_Music_Player = nullptr;

    Debug_Info_Music("Music_Player DONE");
    return 0;
}


//___________________________________________________
static void __declspec(naked) music_thread_play(void) {

    __asm {
        push ebx
        push edi
        push esi
        push ebp

        push ecx
        call Music_Player
        add esp, 0x4

        pop ebp
        pop esi
        pop edi
        pop ebx

        ret
    }
}


//__________________________________________________________________________________________________________________________________________________
static BOOL Read_Music_Data(LONG tune_num, void* This, char* path, DWORD dwDesiredAccess, BOOL halt_on_create_file_error, BOOL dwFlagsAndAttributes) {
    Debug_Info_Music("Read_Music_Data flags %d, %d, %d", dwDesiredAccess, halt_on_create_file_error, dwFlagsAndAttributes);
    Debug_Info_Music("Read_Music_Data: tune num: %d", tune_num);
    Debug_Info_Music("Read_Music_Data: tune path: %s", path);

    BOOL is_file_loaded = wc3_file_load(This, path, dwDesiredAccess, halt_on_create_file_error, dwFlagsAndAttributes);
    if (is_file_loaded == TRUE) {
        MUSIC_FILE* music_file = static_cast<MUSIC_FILE*>(This);
        if (p_Music_Player)
            p_Music_Player->Load_Tune_Data(tune_num, music_file);
        //Debug_Info_Music("Read_Music_Data: continuous play: %d, dont_interrupt_tune: %d, ?: %d, ?: %d", music_file->DIGM_CDX_01_flag, music_file->dont_interrupt_tune, music_file->DIGM_CDX_03_flag, music_file->DIGM_CDX_04_unk);
        Debug_Info_Music("Read_Music_Data: continuous play: %d, dont_interrupt_tune: %d, ?: %d", music_file->DIGM_CDX_03_flag, (BYTE)(music_file->flags >> 8), music_file->DIGM_CDX_01_flag);
    }
    return is_file_loaded;
}


//_________________________________________________
static void __declspec(naked) read_music_data(void) {

    __asm {
        push ebx
        push edi
        push esi
        push ebp

        push[esp + 0x20] //unknown_flag2
        push[esp + 0x20] //unknown_flag1
        push[esp + 0x20] //print_error_flag
        push[esp + 0x20] //path
        push ecx        //this file ptr
        //lea eax, [esp + 0x9C]
        //push eax        //tune name
        push edi        //tune num
        call Read_Music_Data
        add esp, 0x18

        pop ebp
        pop esi
        pop edi
        pop ebx

        ret 0x10
    }
}


//_____________________________________________________
static void __declspec(naked) put_tune_num_in_edi(void) {

    __asm {
        mov edi, eax // store tune num in edi for "Read_Music_Data" function.
        //re-insert original code
        lea eax, [eax * 0x8 + eax]
        mov esi, dword ptr ds:[esp + 0x14]
        ret
    }
}


//________________________
void Modifications_Music() {

    //replace original xanlib music playback thread function.
    MemWrite8(0x444E20, 0x83, 0xE9);
    FuncWrite32(0x444E21, 0x565364EC, (DWORD)&music_thread_play);
 

    //initialization: store tune num in edi for "read_music_data" function.
    MemWrite8(0x444C09, 0x8D, 0xE8);
    FuncWrite32(0x444C0A, 0x748BC004, (DWORD)&put_tune_num_in_edi);
    MemWrite16(0x444C0E, 0x1024, 0x9090);

    //initialization: read music wav file data into buffers.
    FuncReplace32(0x444C60, 0x03ED9C, (DWORD)&read_music_data);
    //initialization: jump over xanlib stuff.
    MemWrite16(0x444C64, 0xC085, 0x7FEB);//JMP SHORT 00444CE5


    //music starting value 10, should probably be 10
//    MemWrite32(0x4A3A8C, 0x0A, 10);
    //music volume lowered for comms, original 4.
    //adjusted for vlc for a similar effect to 4.
//    MemWrite8(0x433782, 0x4, MUSIC_VOLUME_VLC_TALK_SUB);
}
