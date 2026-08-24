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
#include "modifications.h"
#include "memwrite.h"
#include "configTools.h"
#include "wc3w.h"

BOOL thread_not_active = FALSE;


//______________________________________________________
static void __stdcall movie_thread_cycle_branches(void*) {
    while (thread_not_active == 0) {
        if (TryEnterCriticalSection(p_wc3_movie_criticalsection) != 0) {
            __asm {
                mov eax, p_wc3_movie_class
                mov eax, dword ptr ds : [eax]
                cmp dword ptr ds : [eax + 0xD4] , 0
                je skip_update
                mov ecx, eax
                call wc3_movie_update_positon
                skip_update :
            }
            LeaveCriticalSection(p_wc3_movie_criticalsection);
        }
        Sleep(0);
    }
    return;
}


//____________________________________________________________________________________________________________________________________________________________________________________
static HANDLE __stdcall  movie_thread_begin_fix( void* security, unsigned stack_size, unsigned(__stdcall* start_address)(void*), void* arglist, unsigned initflag, unsigned* thrdaddr) {
    thread_not_active = FALSE;
    return (HANDLE)_beginthreadex(security, stack_size, start_address, arglist, initflag, thrdaddr);
}


//____________________________________________________________________________
static BOOL __stdcall  movie_thread_end_fix(HANDLE hThread, DWORD  dwExitCode) {
    thread_not_active = TRUE;
    WaitForSingleObject(hThread, INFINITE);
    return TRUE;
}


//Look for and load files located in the "theGameDir"\\data folder in place of files located in the .tre archives.
//__________________________________________
static BOOL Load_Data_File(char* pfile_name) {

    //"..\\..\\" signifies that the file is located in a .tre archive.
    if (strncmp(pfile_name, "..\\..\\", 6) == 0) {

        DWORD file_attributes = GetFileAttributesA(pfile_name + 4);
        //check if the file exists under relative path \data
        if (file_attributes != INVALID_FILE_ATTRIBUTES && !(file_attributes & FILE_ATTRIBUTE_DIRECTORY) ) {
            size_t file_name_len = strlen(pfile_name) + 1;

            char* file_name_backup = new char[file_name_len + 1];
            strncpy_s(file_name_backup, file_name_len, pfile_name, file_name_len);
            //change the path removing the path intro leaving .\data\"etc"
            strncpy_s(pfile_name, file_name_len, file_name_backup + 4, file_name_len - 4);
            delete[] file_name_backup;
            Debug_Info("Load_Data_File File FOUND: %s", pfile_name);
        } 
    }
    //Debug_Info("Load_Data_File: %s", pfile_name);
    return wc3_find_file_in_tre(pfile_name);
}


//________________________________________________
static void __declspec(naked) load_data_file(void) {

    __asm {
        mov ebx, [esp + 0x4]//pointer to file name in file_class

        push ebp
        push esi

        push ebx
        call Load_Data_File
        add esp, 0x4

        pop esi
        pop ebp

        ret
    }
}


/*
static void Print_Closed_Handle(BOOL close_good, void* p_this_class) {
    Debug_Info("Print_Closed_Handle: %s, close_flag_good_zero:%d", p_this_class, close_good);
}


void* p_close_file_handle = (void*)0x485280;
//______________________________________________________
static void __declspec(naked) close_file_handle(void) {

    __asm {
        push eax
        call p_close_file_handle
        add esp, 0x4

        pushad
        push esi
        push eax
        call Print_Closed_Handle
        add esp, 0x8
        popad
        ret
    }
}
*/

//Fixed a code error on a call to the "VirtualProtect" function, where the "lpflOldProtect" parameter was set to NULL when it should point to a place to store the previous access protection value.
//______________________________
static void VirtualProtect_Fix() {
    DWORD oldProtect;
    VirtualProtect((LPVOID)0x476080, 0x47D7AE - 0x476080, PAGE_EXECUTE_READWRITE, &oldProtect);
}


//____________________________________________________
static void __declspec(naked) virtualprotect_fix(void) {

    __asm {
        pushad
        call VirtualProtect_Fix
        popad
        ret
    }
}


//_____________________________________________________________
//Check if an alterable file exists in either the Application folder or UAC data folder. 
static DWORD __stdcall GetFileAttributes_UAC(LPCSTR lpFileName) {
    const char* pos = StrStrIA(lpFileName, ".WSG");//check if saved game file.
    if(!pos)
        pos = StrStrIA(lpFileName, "SFOSAVED.DAT");//check if settings file.
    if (pos) {
        //Debug_Info("GetFileAttributes_UAC: %s", lpFileName);
        std::wstring path = GetAppDataPath();
        if (!path.empty()) {
            path.append(L"\\");
            DWORD attributes = INVALID_FILE_ATTRIBUTES;
            size_t num_bytes = 0;
            wchar_t* wchar_buff = new wchar_t[13] {0};
            if (mbstowcs_s(&num_bytes, wchar_buff, 13, lpFileName, 13) == 0) {
                path.append(wchar_buff);
                attributes = GetFileAttributes(path.c_str());
                //Copy the "WSG_NDX.WSG" saved game names file to the UAC data folder if it does not exist.
                if (attributes == INVALID_FILE_ATTRIBUTES && wcsstr(wchar_buff, L"WSG_NDX.WSG")) {
                    if (CopyFile(wchar_buff, path.c_str(), TRUE))
                        attributes = GetFileAttributes(path.c_str());
                }
            }
            delete[] wchar_buff;
            if (attributes != INVALID_FILE_ATTRIBUTES)
                return attributes;
        }
    }
    return GetFileAttributesA(lpFileName);
}
void* p_get_file_attributes_uac = &GetFileAttributes_UAC;


//____________________________________________________________________________________________________________________________________________________________________________________________________________________________
//Create\Open an alterable file for editing, from the UAC data folder first if present or depending on the DesiredAccess.
static HANDLE __stdcall CreateFile_UAC(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {

    const char* pos = StrStrIA(lpFileName, ".WSG");//check if saved game file.
    if (!pos)
        pos = StrStrIA(lpFileName, "SFOSAVED.DAT");//check if settings file.
    if (pos) {
        //Debug_Info("CreateFile_UAC: %s, acc:%X", lpFileName, dwDesiredAccess);
        std::wstring path = GetAppDataPath();
        if (!path.empty()) {
            path.append(L"\\");
            HANDLE handle = INVALID_HANDLE_VALUE;
            size_t num_bytes = 0;
            wchar_t* wchar_buff = new wchar_t[13] {0};
            if (mbstowcs_s(&num_bytes, wchar_buff, 13, lpFileName, 13) == 0) {
                path.append(wchar_buff);
                handle = CreateFile(path.c_str(), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
            }
            delete[] wchar_buff;
            if (handle != INVALID_HANDLE_VALUE)
                return handle;
        }
    }
    return CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}
void* p_create_file_uac = &CreateFile_UAC;


//_____________________________________________________
//This function is only called for deleting the temp file "00000102.WSG". Which only exists during missions.
static BOOL __stdcall DeleteFile_UAC(LPCSTR lpFileName) {
    const char* pos = StrStrIA(lpFileName, "00000102.wsg");
    if (pos) {
        //Debug_Info("DeleteFile_UAC: %s", lpFileName);
        std::wstring path = GetAppDataPath();
        if (!path.empty()) {
            path.append(L"\\");
            BOOL retVal = FALSE;
            size_t num_bytes = 0;
            wchar_t* wchar_buff = new wchar_t[13] {0};
            if (mbstowcs_s(&num_bytes, wchar_buff, 13, lpFileName, 13) == 0) {
                path.append(wchar_buff);
                retVal = DeleteFile(path.c_str());
            }
            delete[] wchar_buff;
            if (retVal)
                return retVal;
        }
    }
    return DeleteFileA(lpFileName);
}
void* p_delete_file_uac = &DeleteFile_UAC;


//________________________________
//Rebuilds the save game name list file if it does not exist. Adding detected saved games from UAC appdata and the Application folder.
static BOOL Build_SaveNames_File() {

    bool isUAC = false;

    std::wstring path = GetAppDataPath();
    if (!path.empty()) {
        path.append(L"\\");
        isUAC = true;
    }
    size_t path_length = path.length();

    path.append(L"WSG_NDX.WSG");
    HANDLE h_name_file = CreateFile(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (h_name_file == INVALID_HANDLE_VALUE)
        return FALSE;
    DWORD num_bytes_written = 0;
    DWORD dw_dat = 0x4D524F46;//FORM text
    WriteFile(h_name_file, &dw_dat, 4, &num_bytes_written, nullptr);
    dw_dat = _byteswap_ulong(0x0BE6);//form size (switch endianness)
    WriteFile(h_name_file, &dw_dat, 4, &num_bytes_written, nullptr);
    dw_dat = 0x45564153;//SAVE text
    WriteFile(h_name_file, &dw_dat, 4, &num_bytes_written, nullptr);
    dw_dat = 0x4F464E49;//INFO text
    WriteFile(h_name_file, &dw_dat, 4, &num_bytes_written, nullptr);
    dw_dat = _byteswap_ulong(0x04);//info size (switch endianness)
    WriteFile(h_name_file, &dw_dat, 4, &num_bytes_written, nullptr);
    dw_dat = 0x06;//info data
    WriteFile(h_name_file, &dw_dat, 4, &num_bytes_written, nullptr);

    //create saved game names using chosen language. english format "SAVE GAME %d."
    const char* save_game_text = p_save_game_text_eng;
    if (*p_wc3_language_ref == 1)
        save_game_text = p_save_game_text_ger;
    if (*p_wc3_language_ref == 2)
        save_game_text = p_save_game_text_fre;

    char game_title[22]{ 0 };
    wchar_t w_game_file_name[16]{ 0 };

    path.resize(path_length);
    path.append(L"00000000.WSG");
    
    //search for previously saved games.
    for (DWORD i = 0; i <= 100; i++) { //valid save names go from 0 to 100.
        dw_dat = i;
        WriteFile(h_name_file, &dw_dat, 4, &num_bytes_written, nullptr);
        dw_dat = _byteswap_ulong(21);// max save game title length, minus the ending null char. (switch endianness)
        WriteFile(h_name_file, &dw_dat, 4, &num_bytes_written, nullptr);

        memset(game_title, '\0', 22);

        swprintf_s(w_game_file_name, L"%08d.WSG", i);


        path.replace(path_length, 12, w_game_file_name);

        //check the app data path for this save, and if not found and UCA enabled also check the app Application dir.
        if (GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES)
            sprintf_s(game_title, save_game_text, i);
        else if (isUAC) {
            if (GetFileAttributes(w_game_file_name) != INVALID_FILE_ATTRIBUTES)
                sprintf_s(game_title, save_game_text, i);
        }

        WriteFile(h_name_file, game_title, 22, &num_bytes_written, nullptr);
    }

    CloseHandle(h_name_file);
    h_name_file = INVALID_HANDLE_VALUE;
    return TRUE;
}


//_______________________________________________________
static void __declspec(naked) build_save_names_file(void) {

    __asm {

        push ebx
        push edx
        push ecx
        push esi
        push edi
        push ebp

        call Build_SaveNames_File

        pop ebp
        pop edi
        pop esi
        pop ecx
        pop edx
        pop ebx

        ret
    }
}



/*
ACM Tail
    DWORD Identifier;//0x00  = ACM!
    DWORD data_size;//0x04
    WORD  sample_rate;//0x08 (in hertz)
    WORD  bits_per_sample;//0x0A
    WORD  num_channels;//0x0C, 1 channel == 0, 

audio_format_struct, seems to be similar to wav format structure.
    WORD  audio_format?//0x00
    WORD  num_channels?//0x02
    DWORD sample_rate;//0x04 (in hertz)
    DWORD bytes_per_sec?//0x08
    WORD  bytes_per_block?//0x0C
    WORD  bits_per_sample;// 0x0E
    WORD  unk_10;
*/
struct AUDIO_DATA_STRUCT {
    DWORD unk_00;
    void* p_data_ptr;//0x04
    DWORD unk_08;
    DWORD unk_0C;//set to 0
    DWORD unk_10;
    DWORD data_size;//0x14
    DWORD flags;//0x18
    DWORD unk_1C;
};


//_______________________________________________
static BYTE* Get_Wav_Data_Chunk(BYTE* p_wav_data) {

    DWORD code = *(DWORD*)p_wav_data;
    if (code != 0x46464952)// FOURCC [RIFF]
        return nullptr;

    p_wav_data += 4;
    DWORD file_size = *(DWORD*)p_wav_data;
    LONGLONG remaining_size = (LONGLONG)file_size;

    p_wav_data += 4;
    remaining_size -= 4;

    code = *(DWORD*)p_wav_data;
    if (code != 0x45564157) {// FOURCC [WAVE]
        Debug_Info("Get_Wav_Data_Chunk: Wav RIFF has No WAVE");
        return nullptr;
    }

    p_wav_data += 4;
    remaining_size -= 4;

    DWORD sec_size = 0;

    while (remaining_size > 0) {
        code = *(DWORD*)p_wav_data;
        p_wav_data += 4;
        remaining_size -= 4;
        sec_size = *(DWORD*)p_wav_data;
        if (code == 0x61746164)// FOURCC [data]
            break;

        p_wav_data += 4;
        remaining_size -= 4;
        p_wav_data += sec_size;
        remaining_size -= sec_size;
    }
    if (code == 0x61746164) // FOURCC [data]
        return p_wav_data;

    Debug_Info("Get_Wav_Data_Chunk: data chunk NOT found");
    return nullptr;

}


//_________________________________________________
static BYTE* Get_Wav_Format_Chunk(BYTE* p_wav_data) {

    DWORD code = *(DWORD*)p_wav_data;
    if (code != 0x46464952)// FOURCC [RIFF]
        return nullptr;

    p_wav_data += 4;
    DWORD file_size = *(DWORD*)p_wav_data;
    LONGLONG remaining_size = (LONGLONG)file_size;

    p_wav_data += 4;
    remaining_size -= 4;

    code = *(DWORD*)p_wav_data;
    if (code != 0x45564157) {// FOURCC [WAVE]
        Debug_Info("Get_Wav_Format_Chunk: Wav RIFF has No WAVE");
        return nullptr;
    }

    p_wav_data += 4;
    remaining_size -= 4;

    DWORD sec_size = 0;

    while (remaining_size > 0) {
        code = *(DWORD*)p_wav_data;
        p_wav_data += 4;
        remaining_size -= 4;
        sec_size = *(DWORD*)p_wav_data;
        if (code == 0x20746D66)// FOURCC [fmt ]
            break;

        p_wav_data += 4;
        remaining_size -= 4;
        p_wav_data += sec_size;
        remaining_size -= sec_size;
    }
    if (code == 0x20746D66) // FOURCC [fmt ]
        return p_wav_data;

    Debug_Info("Get_Wav_Format_Chunk: data chunk NOT found");
    return nullptr;

}


//____________________________________________________________________________________________________________________
static void Check_And_Set_Wav_Audio_Data(void* file_struct, AUDIO_DATA_STRUCT* data_struct, void* audio_format_struct) {

    //DWORD file_size = ((DWORD*)file_struct)[0];
    void* p_file_data = (void*)((DWORD*)file_struct)[5];

    if (((DWORD*)p_file_data)[0] == 0x46464952) { // FOURCC [RIFF]

        BYTE* p_wave_data = Get_Wav_Data_Chunk((BYTE*)p_file_data);
        if (p_wave_data) {
            data_struct->data_size = *(DWORD*)p_wave_data;
            data_struct->p_data_ptr = p_wave_data + 4;
            //Debug_Info("Check_And_Set_Wav_Audio_Data: Wav data size:%X, off:%X", data_struct->data_size, data_struct->p_data_ptr);

            //Fix for the second sound(ref 0x15) in the German version of the file "VICD2.IFF. Check if ACM tail is present within the wav data and subtract it from the data size.
            DWORD check_ACM = *(DWORD*)((BYTE*)data_struct->p_data_ptr + data_struct->data_size - 0x22);
            if (check_ACM == 0x214D4341) {//ACM!
                data_struct->data_size -= 0x22;
                Debug_Info("Check_And_Set_Wav_Audio_Data: ACM Tail DETECTED in Wav data re-sized:%X", data_struct->data_size);
            }
        }
        else
            wc3_error_message_box("WAV data NOT found: 0x%x", ((DWORD*)file_struct)[1]);
        
        //copy the wav format structure.
        BYTE* p_wave_format = Get_Wav_Format_Chunk((BYTE*)p_file_data);
        if (p_wave_format)
            memcpy(audio_format_struct, p_wave_format + 4, ((DWORD*)p_wave_format)[0]);
        else
            wc3_error_message_box("WAV format NOT found: 0x%x", ((DWORD*)file_struct)[1]);
        //Debug_Info("sound has WAV format - sample rate:%dHz, bits per sample:%d, num channels:%d", ((DWORD*)audio_format_struct)[1], ((WORD*)audio_format_struct)[7], ((WORD*)audio_format_struct)[1]);
    }
    else if (((DWORD*)p_file_data)[0] == 0x61657243 && ((DWORD*)p_file_data)[1] == 0x65766974 && ((DWORD*)p_file_data)[2] == 0x696F5620 &&
        ((DWORD*)p_file_data)[3] == 0x46206563 && ((DWORD*)p_file_data)[4] == 0x1A656C69) {//"Creative Voice File(0x1A)"
        
        data_struct->data_size = (*(DWORD*)((BYTE*)p_file_data + 0x1B)) & 0x00FFFFFF;///data size 3 bytes, mask out 4th byte, Frequency divisor 
        data_struct->data_size -= 2;//subtract Frequency divisor and codec flag from data size.
        data_struct->p_data_ptr = (BYTE*)p_file_data + 0x20;
        //Debug_Info("Check_And_Set_Wav_Audio_Data: VOC data size:%X, off:%X", data_struct->data_size, data_struct->p_data_ptr);

        //file_data ptr's
        WORD* p_version = (WORD*)((BYTE*)p_file_data + 0x16);
        WORD* p_check = (WORD*)((BYTE*)p_file_data + 0x18);
        if(~*p_version + 0x1234 != *p_check)
            wc3_error_message_box("Creative Voice File VOC version checksum failed: version:%X, check:%X, 0x%x", *p_version, *p_check,  ((DWORD*)file_struct)[1]);

        //file_data ptr's
        BYTE* p_codec = ((BYTE*)p_file_data + 0x1D);
        BYTE* p_freq_div = ((BYTE*)p_file_data + 0x1E);

        if(*p_codec && *p_codec !=4)//if codec not 0(8bit PCM) or 4(16bit PCM)
            wc3_error_message_box("Creative Voice File VOC codec not PCM: 0x%x", ((DWORD*)file_struct)[1]);
        
        //audio_format_struct ptr's
        WORD* p_audio_format = (WORD*)((BYTE*)audio_format_struct + 0x0);
        WORD* p_num_channels = (WORD*)((BYTE*)audio_format_struct + 0x2);
        DWORD* p_sample_rate = (DWORD*)((BYTE*)audio_format_struct + 0x4);
        DWORD* p_bytes_per_sec = (DWORD*)((BYTE*)audio_format_struct + 0x8);
        WORD* p_bytes_per_block = (WORD*)((BYTE*)audio_format_struct + 0xC);
        WORD* p_bits_per_sample = (WORD*)((BYTE*)audio_format_struct + 0xE);

        *p_audio_format = 1;
        *p_num_channels = 1;
        *p_sample_rate = 1000000 / (256 - *p_freq_div);
        *p_bits_per_sample = 8;
        if (*p_codec == 4)
            *p_bits_per_sample = 16;
        *p_bytes_per_block = *p_num_channels * *p_bits_per_sample / 8;
        *p_bytes_per_sec = *p_sample_rate * *p_bytes_per_block;
    }
    else
        wc3_error_message_box("Neither ACM! tail, VOC or WAV header info were found: 0x%x", ((DWORD*)file_struct)[1]);
    //Debug_Info("audio_format_struct %d, %d, %d, %d, %d, %d", ((WORD*)audio_format_struct)[0], ((WORD*)audio_format_struct)[1], ((DWORD*)audio_format_struct)[1], ((DWORD*)audio_format_struct)[2], ((WORD*)audio_format_struct)[6], ((WORD*)audio_format_struct)[7]);
}


//______________________________________________________________
static void __declspec(naked) check_and_set_wav_audio_data(void) {

    __asm {
        pushad

        push edx
        push esi
        push ebp
        call Check_And_Set_Wav_Audio_Data
        add esp, 0x0C

        popad

        ret
    }
}


//____________________________________________________
static void __declspec(naked) check_audio_format(void) {
    //[ebp] holds the data size.
    //ebx holds the expected data size, minus the ACM tail.
    //[ebp+0x14] holds the data pointer.
    //edi holds the expected pointer within the data to the ACM tail.
    
    __asm {
        xor eax, eax
        mov edx, dword ptr ss:[ebp+0x14]
        cmp dword ptr ds:[edx], 0x46464952//// FOURCC [RIFF]
        jne check_voc

        mov eax, -1// return -1 if not an ACM.
        ret


        check_voc:
        cmp dword ptr ds : [edx + 0x00] , 0x61657243//"Crea" (voc header first 4 bytes of "Creative Voice File")
        jne check_acm
        cmp dword ptr ds : [edx + 0x04] , 0x65766974//"tive" (voc header next 4 bytes of "Creative Voice File")
        jne check_acm
        cmp dword ptr ds : [edx + 0x08] , 0x696F5620//" Voi" (voc header next 4 bytes of "Creative Voice File")
        jne check_acm
        cmp dword ptr ds : [edx + 0x0C] , 0x46206563//"ce F" (voc header next 4 bytes of "Creative Voice File")
        jne check_acm
        cmp dword ptr ds : [edx + 0x10] , 0x1A656C69//"ile(0x1A)" (voc header next 4 bytes of "Creative Voice File")
        jne check_acm

        mov eax, -1// return -1 if not an ACM.
        ret


        check_acm:
        cmp dword ptr ds:[edi], 0x214D4341//// FOURCC [ACM!]
        je is_acm

        //fix for the third sound(ref 0x1A) in the German version of the file "VICD2.IFF". The header position is off by one.  
        //retry the acm check after subtracting the pointer position by one.
        sub edi, 1
        sub ebx, 1
        cmp dword ptr ds:[edi], 0x214D4341//// FOURCC [ACM!]
        je is_acm

        mov eax, -1// return -1 if not an ACM.

        is_acm:
        ret
    }
}


//________________________________________________
static void Modify_Object_LOD_Distance(DWORD* LOD) {

    static int lod_modifier = 100;
    static bool run_once = false;
    if (!run_once) {
        lod_modifier = ConfigReadInt(L"SPACE", L"LOD_LEVEL_DISTANCE_MODIFIER", CONFIG_SPACE_LOD_LEVEL_DISTANCE_MODIFIER);
        if (lod_modifier != 0 && lod_modifier < 100)
            lod_modifier = 100;

        run_once = true;
        Debug_Info("LOD_LEVEL_DISTANCE_MODIFIER SET AT: %d%%", lod_modifier);
    }
    if (*LOD <= 7)//Ignore values 7 or less. LOD dist 0-7 used by afterburner effect animation.
        return;

    if (lod_modifier == 0)
        *LOD = 0;
    else
        *LOD = *LOD * lod_modifier / 100;
    //Debug_Info("Modify_Object_LOD dist:%d", *LOD);
}


//________________________________________________________
static void __declspec(naked) modify_object_lod_dist(void) {

    __asm {
        pushad
        mov ecx, ebx
        add ecx, 0x30
        push ecx
        call Modify_Object_LOD_Distance
        add esp, 0x4
        popad
        //re-insert original code
        mov eax, dword ptr ds:[eax + 0x90]
        ret
    }
}


//__________________________________________________________
static LONG MULTI_ARG1_BY_256_DIV_ARG2(LONG arg1, LONG arg2) {
    LONGLONG val = (LONGLONG)arg1 << 8;
    return LONG(val / arg2);
}


//__________________________________________________________
static LONG MULTI_ARG1_BY_ARG2_DIV_256(LONG arg1, LONG arg2) {
    LONGLONG val = (LONGLONG)arg1 * arg2;
    return LONG(val >> 8);
}


//______________________________________________________________________
static LONG MULTI_ARG1_BY_ARG2_DIV_ARG3(LONG arg1, LONG arg2, LONG arg3) {
    return LONG((LONGLONG)arg1 * arg2 / arg3);
}


//_________________________________________________
static void Debug_Info_WC3(const char* format, ...) {
    __Debug_Info(DEBUG_INFO_ERROR, format);
}

/*
//_______________________________________________________________________________________________________________
static void Display_Debug_Info_1(DRAW_BUFFER_MAIN* p_toBuff, DWORD x, DWORD y, DWORD unk1, char* text_buff, BYTE* p_pal_offsets) {

    wc3_draw_text_to_buff(p_toBuff, x, y, unk1, text_buff, p_pal_offsets);
    y += 10;

    sprintf_s(text_buff, 240, "yaw axis: %d", *p_wc3_joy_move_x_256);
    y += 10;
    wc3_draw_text_to_buff(p_toBuff, x, y, unk1, text_buff, p_pal_offsets);
    sprintf_s(text_buff, 240, "pitch axis: %d", *p_wc3_joy_move_y_256);
    y += 10;
    wc3_draw_text_to_buff(p_toBuff, x, y, unk1, text_buff, p_pal_offsets);
    sprintf_s(text_buff, 240, "roll axis: %d", *p_wc3_joy_move_r);
    y += 10;
    wc3_draw_text_to_buff(p_toBuff, x, y, unk1, text_buff, p_pal_offsets);
}
*/

/*
//_____________________________________________
static void Proccess_Object(DWORD** func_array) {
    static int count = 0;
    Debug_Info("Proccess_Object: %d, func:%X", count, func_array[1]);

    count++;
}


//________________________________
static void Proccess_Object_Pass() {
    static int count = 0;
    Debug_Info("Proccess_Object PASSED: %d", count);

    count++;
}


//__________________________________________________
static void __declspec(naked) processes_object(void) {

    __asm {
        mov ebx, dword ptr ds : [eax]

        pushad
        push ebx
        call Proccess_Object
        add esp, 0x4
        popad


        mov ecx, eax
        call dword ptr ds : [ebx + 0x4]

        //pushad
        //call Proccess_Object_Pass
        //popad

        ret

    }
}
*/


//______________________________________
static DWORD Set_VirtualAlloc_Mem_Size() {

    static bool run_once = false;
    if (!run_once) {
        DWORD vmem_size = ConfigReadInt(L"MAIN", L"VIRTUAL_MEM_SIZE", CONFIG_MAIN_VIRTUAL_MEM_SIZE);
        if (vmem_size > *p_wc3_virtual_alloc_mem_size)
            *p_wc3_virtual_alloc_mem_size = vmem_size;

        run_once = true;
        Debug_Info("Virtual Mem Allocated: %d bytes", *p_wc3_virtual_alloc_mem_size);
    }

    return *p_wc3_virtual_alloc_mem_size;
}


//____________________________________________________________
static void __declspec(naked) set_virtual_alloc_mem_size(void) {

    __asm {
        push edx
        push ebx
        push ecx
        push edi
        push esi
        push ebp

        call Set_VirtualAlloc_Mem_Size

        pop ebp
        pop esi
        pop edi
        pop ecx
        pop ebx
        pop edx

        ret
    }
}


//_________________________________________________
static LONG Num_Watchers_Overide(LONG num_watchers) {

    static int num_watchers_overide = 500;
    static bool run_once = false;
    if (!run_once) {
        num_watchers_overide = ConfigReadInt(L"MAIN", L"NUM_WATCHERS_OVERIDE", CONFIG_MAIN_NUM_WATCHERS_OVERIDE);
        if (num_watchers_overide < 500)
            num_watchers_overide = 500;

        run_once = true;
        Debug_Info("Max Number Of Watches Overide Value: %d", num_watchers_overide);
    }

    if (num_watchers < num_watchers_overide) {
        num_watchers = num_watchers_overide;
        Debug_Info("Max Number Of Watches Set At: %d", num_watchers);
    }
    return num_watchers;
}


//______________________________________________________
static void __declspec(naked) num_watchers_overide(void) {

    __asm {
        push edx
        push ebx
        push ecx
        push edi
        push esi
        push ebp

        push eax
        call Num_Watchers_Overide
        add esp, 0x4

        pop ebp
        pop esi
        pop edi
        pop ecx
        pop ebx
        pop edx

        //re-insert original code
        mov dword ptr ds : [ecx + 0x4], eax
        mov esi, ecx

        ret
    }
}


//____________________________________________________________________________________________________________________________________
static void Display_Alt_X_Msg_Room_Scene_ID(DRAW_BUFFER_MAIN* p_toBuff, DWORD x, DWORD y, DWORD unk1, char* text_buff, BYTE* p_pal_offsets) {

    sprintf_s(text_buff, 240, "Room: %d, Scene: %d", *p_wc3_current_room_id, *p_wc3_current_scene_id);
    wc3_draw_text_to_buff(p_toBuff, x, y, unk1, text_buff, p_pal_offsets);
}


//_______________________________________________________
void Modifications_Replace_Alt_X_Msg_With_Room_Scene_ID() {

    FuncReplace32(0x4129FA, 0x0629E3, (DWORD)&Display_Alt_X_Msg_Room_Scene_ID);
}


//_______________________________________
static void Set_Subtitles_Flag(LONG flag) {

    *p_wc3_subtitles_enabled = flag;
    ConfigWriteInt_InGame(L"MAIN", L"ENABLE_SUBTITLES", *p_wc3_subtitles_enabled);
}


//____________________________________________________
static void __declspec(naked) set_subtitles_flag(void) {

    __asm {
        pushad
        push eax
        call Set_Subtitles_Flag
        add esp, 0x4
        popad
        ret
    }
}


//____________________________________
static void Set_Language_Ref(LONG ref) {

    *p_wc3_language_ref = ref;
    ConfigWriteInt_InGame(L"MAIN", L"LANGUAGE_REF", *p_wc3_language_ref);
}


//____________________________________________________
static void __declspec(naked) set_language_ref_1(void) {

    __asm {
        pushad
        push eax
        call Set_Language_Ref
        add esp, 0x4
        popad
        ret
    }
}


//____________________________________________________
static void __declspec(naked) set_language_ref_2(void) {

    __asm {
        pushad
        push 2
        call Set_Language_Ref
        add esp, 0x4
        popad
        ret
    }
}


//____________________________________________________
static void __declspec(naked) set_language_ref_3(void) {

    __asm {
        pushad
        push ebx
        call Set_Language_Ref
        add esp, 0x4
        popad
        ret
    }
}


DWORD vmem_start = 0;
DWORD vmem_end = 0;
//_____________________________________________________________________________________________________________________________
static LPVOID __stdcall VirtualAlloc_Game_Resources(LPVOID lpAddress, SIZE_T dwSize, DWORD  flAllocationType, DWORD  flProtect) {

    LPVOID base_address = VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);
    vmem_start = (DWORD)base_address;
    vmem_end = vmem_start + dwSize;

    return base_address;
}
void* p_virtual_alloc_game_resources = &VirtualAlloc_Game_Resources;


//_____________________________________________
//static void print_texture_error(DWORD mem_addr) {
//
//    Debug_Info_Error("BAD_Texture_Addr: %X", mem_addr);
//}


//______________________________________________________
static void __declspec(naked) test_texture_address(void) {

    __asm {
        add eax, edx// add tex_mem_ptr(EAX) and offset(EDX)
        cmp eax, vmem_start
        jb mem_out_of_bounds
        cmp eax, vmem_end
        jb sample_texture

        mem_out_of_bounds :
        //pushad
        //push eax
        //call print_texture_error
        //add esp, 0x4
        //popad
        mov al, 0xFF// set pixel to 255(mask colour) don't draw. 
        jmp end_func

        sample_texture :
        mov al, byte ptr ds : [eax]

        end_func :
        //original code
        add ebp, ebx
        ret
    }
}


//________________________________________________________
static void __declspec(naked) test_texture_address_2(void) {

    __asm {
        //original code
        adc ecx, dword ptr ds : [0x4A7590]

        add eax, edx// add tex_mem_ptr(EAX) and offset(EDX)
        cmp eax, vmem_start
        jb mem_out_of_bounds
        cmp eax, vmem_end
        jb sample_texture

        mem_out_of_bounds :
        //pushad
        //push eax
        //call print_texture_error
        //add esp, 0x4
        //popad
        mov al, 0xFF// set pixel to 255(mask colour) don't draw. 
        ret

        sample_texture :
        mov al, byte ptr ds : [eax]
        ret
    }
}


BOOL is_random_crash_report_activated = FALSE;
BOOL flag_draw_3d_func_addr = FALSE;
BOOL flag_draw_3d_func_num = FALSE;
BOOL flag_draw_3d_func_arg3 = FALSE;

BOOL flag_draw_3d_func_global1 = FALSE;
BOOL flag_draw_3d_func_global2 = FALSE;

BOOL flag_draw_3d_func_arg5 = FALSE;
BOOL flag_draw_3d_func_arg6 = FALSE;
BOOL flag_draw_3d_func_arg7 = FALSE;
BOOL flag_draw_3d_func_arg8 = FALSE;


//________________________
void Random_Crash_Report() {
    if (!is_random_crash_report_activated)
        return;
    Debug_Info_Error("");
    Debug_Info_Error("///////////// Random Crash Report /////////////");
    Debug_Info_Error("Processing function addr: 0x%X, function num: %d, Arg3 - num vertices to proccess?: %d", flag_draw_3d_func_addr, flag_draw_3d_func_num, flag_draw_3d_func_arg3);
    Debug_Info_Error("Arg5: %d, Arg6: %d, Arg7: %d, Arg8: %d", flag_draw_3d_func_arg5, flag_draw_3d_func_arg6, flag_draw_3d_func_arg7, flag_draw_3d_func_arg8);
    Debug_Info_Error("global1: %d, global2: %d", flag_draw_3d_func_global1, flag_draw_3d_func_global2);
}


//____________________________________________________
static void __declspec(naked) check_3d_draw_func(void) {

    __asm {

        push eax

        mov eax, dword ptr ds : [ebp + 0x10]
        mov flag_draw_3d_func_arg3, eax

        mov eax, dword ptr ds : [ebp + 0x18]
        mov flag_draw_3d_func_arg5, eax
        mov eax, dword ptr ds : [ebp + 0x1C]
        mov flag_draw_3d_func_arg6, eax
        mov eax, dword ptr ds : [ebp + 0x30]
        mov flag_draw_3d_func_arg7, eax
        mov eax, dword ptr ds : [ebp + 0x24]
        mov flag_draw_3d_func_arg8, eax

        mov eax, 0x4A74D0
        mov eax, dword ptr ds : [eax]
        mov flag_draw_3d_func_global1, eax
        mov eax, 0x4A74D4
        mov eax, dword ptr ds : [eax]
        mov flag_draw_3d_func_global2, eax

        pop eax

        mov flag_draw_3d_func_addr, eax
        mov flag_draw_3d_func_num, ebx
        cmp eax, 0
        je exit_func

        call eax

        exit_func:
        mov flag_draw_3d_func_addr, 0
        mov flag_draw_3d_func_num, 0
        ret
    }
}

//_____________________________________
void Modifications_Random_Crash_Check() {
    is_random_crash_report_activated = TRUE;

    MemWrite16(0x47D7A1, 0xF883, 0x9090);
    MemWrite8(0x47D7A3, 0x00, 0xE8);
    FuncWrite32(0x47D7A4, 0xD0FF0274, (DWORD)&check_3d_draw_func);
}


//_______________________________
void Modifications_GeneralFixes() {

    //Fix memory leak issues surrounding thread creation and destruction which can causes crashes.
    MemWrite32(0x41C47B, 0x41B700, (DWORD)&movie_thread_cycle_branches);

    MemWrite16(0x41C483, 0x15FF, 0xE890);
    FuncWrite32(0x41C485, 0x4B5320, (DWORD)&movie_thread_begin_fix);

    MemWrite16(0x41C646, 0x15FF, 0xE890);
    FuncWrite32(0x41C648, 0x4B531C, (DWORD)&movie_thread_end_fix);

    //Load files in place of files located in .tre archives.
    FuncReplace32(0x483A83, 0x1229, (DWORD)&load_data_file);
    //check if files are being closed.
    //FuncReplace32(0x483B6A, 0x1712, (DWORD)&close_file_handle);


    //Fixed a code error on a call to the "VirtualProtect" function, where the "lpflOldProtect" parameter was set to NULL when it should point to a place to store the previous access protection value.
    FuncReplace32(0x404FF1, 0x060B, (DWORD)&virtualprotect_fix);



    //-----------------------UAC-Patch---------------------------
    //Alter the save location of files to the RoamingAppData folder. To allow the game to work without admin privileges when installed under ProgramFiles and to seperate game data between different Windows users.
    MemWrite32(0x485198, 0x4B53BC, (DWORD)&p_get_file_attributes_uac);

    MemWrite32(0x485254, 0x4B53B4, (DWORD)&p_create_file_uac);

    MemWrite32(0x49AC57, 0x4B5370, (DWORD)&p_delete_file_uac);

    MemWrite16(0x4098A0, 0xEC81, 0xE990);
    FuncWrite32(0x4098A2, 0x02B4, (DWORD)&build_save_names_file);
    //------------------------------------------------------------



    //--------German-Audio-Fix-And-WAV-Format-Support-------------
    FuncReplace32(0x438BF7, 0x058285, (DWORD)&check_audio_format);
    //skip the No ACM tail error box.
    MemWrite8(0x438C0D, 0xE8, 0x90);
    MemWrite32(0x438C0E, 0x03769E, 0x90909090);
    //check and set WAV audio data or reestablish error box if not.
    MemWrite16(0x438C84, 0xC766, 0xE890);
    FuncWrite32(0x438C86, 0x00080E42, (DWORD)&check_and_set_wav_audio_data);
    //skip forced 16bit audio format selection.
    MemWrite8(0x438C91, 0x74, 0xEB);
    //------------------------------------------------------------


    MemWrite16(0x465712, 0x808B, 0xE890);
    FuncWrite32(0x465714, 0x90, (DWORD)&modify_object_lod_dist);


    //-----Replacement integer math function-------------------------------
    //originals causing crashes when imul/idiv were overflowing.
    MemWrite16(0x46EABC, 0x8B55, 0xE990);
    FuncWrite32(0x46EABE, 0x08458BEC, (DWORD)&MULTI_ARG1_BY_256_DIV_ARG2);

    MemWrite16(0x46EACF, 0x8B55, 0xE990);
    FuncWrite32(0x46EAD1, 0x08458BEC, (DWORD)&MULTI_ARG1_BY_ARG2_DIV_256);

    MemWrite16(0x46EADE, 0x8B55, 0xE990);
    FuncWrite32(0x46EAE0, 0x08458BEC, (DWORD)&MULTI_ARG1_BY_ARG2_DIV_ARG3);
    //---------------------------------------------------------------------


    //-----Debugging---------------------------------------------
    //For adding debug info to inflight debug overlay. 
    //FuncReplace32(0x420C1F, 0x0547BE, (DWORD)&Display_Debug_Info_1);
 
    //hijack WC3 Debug info
    MemWrite8(0x491000, 0x56, 0xE9);
    FuncWrite32(0x491001, 0x85606857, (DWORD)&Debug_Info_WC3);

    //changed key combo for space debug overlay from "ALT+D" to "CTRL+D".
    MemWrite8(0x4501E1, 0x03, 0x0C);
    //Remove the need to need for mitchell mode to enable to display space debug overlay "CTRL+D". 
    MemWrite16(0x4501EF, 0x840F, 0x9090);
    MemWrite32(0x4501F1, 0x0332, 0x90909090);
    //Prevent the general space overlay from also being displayed when pressing "CTRL+D".
    MemWrite8(0x450205, 0xA2, 0x90);
    MemWrite32(0x450206, 0x4A271C, 0x90909090);
    //___________________________________________________________


    //00486CA2 | .  8B18 | MOV EBX, DWORD PTR DS : [EAX]
    //00486CA4 | .  8BC8 | MOV ECX, EAX
    //00486CA6 | .FF53 04 | CALL DWORD PTR DS : [EBX + 4]
    //MemWrite8(0x486CA2, 0x8B, 0xE8);
    //FuncWrite32(0x486CA3, 0xFFC88B18, (DWORD)&processes_object);
    //MemWrite16(0x486CA7, 0x0453, 0x9090);

    //Increase the allocated general memory size.
    MemWrite8(0x480972, 0xA1, 0xE8);
    FuncWrite32(0x480973, 0x49F6D8, (DWORD)&set_virtual_alloc_mem_size);

    //Increase the max number of watchers at a nav point. (max number of active ships and turrets)
    MemWrite8(0x48C9E5, 0x89, 0xE8);
    FuncWrite32(0x48C9E6, 0xF18B0441, (DWORD)&num_watchers_overide);

    //Update language ref stored in ini.-------------------------
    //update language from loading game.
    MemWrite8(0x4092D7, 0xA3, 0xE8);
    FuncWrite32(0x4092D8, 0x4A9720, (DWORD)&set_language_ref_1);

    //update language when changing in game options.
    MemWrite8(0x41661C, 0xA3, 0xE8);
    FuncWrite32(0x41661D, 0x4A9720, (DWORD)&set_language_ref_1);

    MemWrite16(0x416667, 0x05C7, 0xE890);
    FuncWrite32(0x416669, 0x4A9720, (DWORD)&set_language_ref_2);
    MemWrite32(0x41666D, 0x02, 0x90909090);

    MemWrite16(0x4166BE, 0x1D89, 0xE890);
    FuncWrite32(0x4166C0, 0x4A9720, (DWORD)&set_language_ref_3);
    //-----------------------------------------------------------

    //Update subtitle flag stored in ini.------------------------
    //update subtitle ref when loading game.
    MemWrite8(0x4092CD, 0xA3, 0xE8);
    FuncWrite32(0x4092CE, 0x4A0FDC, (DWORD)&set_subtitles_flag);

    //update subtitle ref when changing in game options.
    MemWrite8(0x41660A, 0xA3, 0xE8);
    FuncWrite32(0x41660B, 0x4A0FDC, (DWORD)&set_subtitles_flag);
    //-----------------------------------------------------------


    //---------------random-space-crash-fix--texture-sampler-fix------------------
    MemWrite32(0x480986, 0x4B5344, (DWORD)&p_virtual_alloc_game_resources);

    // poly draw func 01: texture highlight, large near
    // 8 or greater
    //0
    MemWrite8(0x478528, 0x8A, 0xE8);
    FuncWrite32(0x478529, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x47852D, 0x4A7590, 0x90909090);
    //1
    MemWrite8(0x47855A, 0x8A, 0xE8);
    FuncWrite32(0x47855B, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x47855F, 0x4A7590, 0x90909090);
    //2
    MemWrite8(0x47858D, 0x8A, 0xE8);
    FuncWrite32(0x47858E, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x478592, 0x4A7590, 0x90909090);
    //3
    MemWrite8(0x4785C0, 0x8A, 0xE8);
    FuncWrite32(0x4785C1, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x4785C5, 0x4A7590, 0x90909090);
    //4
    MemWrite8(0x4785F3, 0x8A, 0xE8);
    FuncWrite32(0x4785F4, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x4785F8, 0x4A7590, 0x90909090);
    //5
    MemWrite8(0x478626, 0x8A, 0xE8);
    FuncWrite32(0x478627, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x47862B, 0x4A7590, 0x90909090);
    //6
    MemWrite8(0x478659, 0x8A, 0xE8);
    FuncWrite32(0x47865A, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x47865E, 0x4A7590, 0x90909090);
    //7
    MemWrite8(0x47868C, 0x8A, 0xE8);
    FuncWrite32(0x47868D, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x478691, 0x4A7590, 0x90909090);
    // less than 8
    //0
    MemWrite8(0x4786DC, 0x8A, 0xE8);
    FuncWrite32(0x4786DD, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x4786E1, 0x4A7590, 0x90909090);
    //1
    MemWrite8(0x47871A, 0x8A, 0xE8);
    FuncWrite32(0x47871B, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x47871F, 0x4A7590, 0x90909090);
    //2
    MemWrite8(0x478759, 0x8A, 0xE8);
    FuncWrite32(0x47875A, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x47875E, 0x4A7590, 0x90909090);
    //3
    MemWrite8(0x478798, 0x8A, 0xE8);
    FuncWrite32(0x478799, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x47879D, 0x4A7590, 0x90909090);
    //4
    MemWrite8(0x4787D7, 0x8A, 0xE8);
    FuncWrite32(0x4787D8, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x4787DC, 0x4A7590, 0x90909090);
    //5
    MemWrite8(0x478812, 0x8A, 0xE8);
    FuncWrite32(0x478813, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x478817, 0x4A7590, 0x90909090);
    //6
    MemWrite8(0x47884D, 0x8A, 0xE8);
    FuncWrite32(0x47884E, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x478852, 0x4A7590, 0x90909090);

    // poly draw func 02: texture, large near
    // 8 or greater
    //0
    MemWrite8(0x478FB7, 0x03, 0xE8);
    FuncWrite32(0x478FB8, 0x02048AEB, (DWORD)&test_texture_address);
    //1
    MemWrite8(0x478FDB, 0x03, 0xE8);
    FuncWrite32(0x478FDC, 0x02048AEB, (DWORD)&test_texture_address);
    //2
    MemWrite8(0x479000, 0x03, 0xE8);
    FuncWrite32(0x479001, 0x02048AEB, (DWORD)&test_texture_address);
    //3
    MemWrite8(0x479025, 0x03, 0xE8);
    FuncWrite32(0x479026, 0x02048AEB, (DWORD)&test_texture_address);
    //4
    MemWrite8(0x47904A, 0x03, 0xE8);
    FuncWrite32(0x47904B, 0x02048AEB, (DWORD)&test_texture_address);
    //5
    MemWrite8(0x47906F, 0x03, 0xE8);
    FuncWrite32(0x479070, 0x02048AEB, (DWORD)&test_texture_address);
    //6
    MemWrite8(0x479094, 0x03, 0xE8);
    FuncWrite32(0x479095, 0x02048AEB, (DWORD)&test_texture_address);
    //7
    MemWrite8(0x4790B9, 0x03, 0xE8);
    FuncWrite32(0x4790BA, 0x02048AEB, (DWORD)&test_texture_address);
    // less than 8
    //0
    MemWrite8(0x4790FB, 0x03, 0xE8);
    FuncWrite32(0x4790FC, 0x02048AEB, (DWORD)&test_texture_address);
    //1
    MemWrite8(0x47912B, 0x03, 0xE8);
    FuncWrite32(0x47912C, 0x02048AEB, (DWORD)&test_texture_address);
    //2
    MemWrite8(0x47915C, 0x03, 0xE8);
    FuncWrite32(0x47915D, 0x02048AEB, (DWORD)&test_texture_address);
    //3
    MemWrite8(0x47918D, 0x03, 0xE8);
    FuncWrite32(0x47918E, 0x02048AEB, (DWORD)&test_texture_address);
    //4
    MemWrite8(0x4791BA, 0x03, 0xE8);
    FuncWrite32(0x4791BB, 0x02048AEB, (DWORD)&test_texture_address);
    //5
    MemWrite8(0x4791E7, 0x03, 0xE8);
    FuncWrite32(0x4791E8, 0x02048AEB, (DWORD)&test_texture_address);
    //6
    MemWrite8(0x479214, 0x03, 0xE8);
    FuncWrite32(0x479215, 0x02048AEB, (DWORD)&test_texture_address);

    // poly draw func 03: texture highlight
    // 8 or greater
    //0
    MemWrite8(0x479BA2, 0x8A, 0xE8);
    FuncWrite32(0x479BA3, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x479BA7, 0x4A7590, 0x90909090);
    //1
    MemWrite8(0x479BD4, 0x8A, 0xE8);
    FuncWrite32(0x479BD5, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x479BD9, 0x4A7590, 0x90909090);
    //2
    MemWrite8(0x479C07, 0x8A, 0xE8);
    FuncWrite32(0x479C08, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x479C0C, 0x4A7590, 0x90909090);
    //3
    MemWrite8(0x479C3A, 0x8A, 0xE8);
    FuncWrite32(0x479C3B, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x479C3F, 0x4A7590, 0x90909090);
    //4
    MemWrite8(0x479C6D, 0x8A, 0xE8);
    FuncWrite32(0x479C6E, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x479C72, 0x4A7590, 0x90909090);
    //5
    MemWrite8(0x479CA0, 0x8A, 0xE8);
    FuncWrite32(0x479CA1, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x479CA5, 0x4A7590, 0x90909090);
    //6
    MemWrite8(0x479CD3, 0x8A, 0xE8);
    FuncWrite32(0x479CD4, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x479CD8, 0x4A7590, 0x90909090);
    //7
    MemWrite8(0x479D06, 0x8A, 0xE8);
    FuncWrite32(0x479D07, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x479D0B, 0x4A7590, 0x90909090);
    // less than 8
    //0
    MemWrite8(0x479D56, 0x8A, 0xE8);
    FuncWrite32(0x479D57, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x479D5B, 0x4A7590, 0x90909090);
    //1
    MemWrite8(0x479D94, 0x8A, 0xE8);
    FuncWrite32(0x479D95, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x479D99, 0x4A7590, 0x90909090);
    //2
    MemWrite8(0x479DD3, 0x8A, 0xE8);
    FuncWrite32(0x479DD4, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x479DD8, 0x4A7590, 0x90909090);
    //3
    MemWrite8(0x479E12, 0x8A, 0xE8);
    FuncWrite32(0x479E13, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x479E17, 0x4A7590, 0x90909090);
    //4
    MemWrite8(0x479E51, 0x8A, 0xE8);
    FuncWrite32(0x479E52, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x479E56, 0x4A7590, 0x90909090);
    //5
    MemWrite8(0x479E8C, 0x8A, 0xE8);
    FuncWrite32(0x479E8D, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x479E91, 0x4A7590, 0x90909090);
    //6
    MemWrite8(0x479EC7, 0x8A, 0xE8);
    FuncWrite32(0x479EC8, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x479ECC, 0x4A7590, 0x90909090);

    // texture, large near
    // 8 or greater
    //0
    MemWrite8(0x47A85B, 0x03, 0xE8);
    FuncWrite32(0x47A85C, 0x02048AEB, (DWORD)&test_texture_address);
    //1
    MemWrite8(0x47A87F, 0x03, 0xE8);
    FuncWrite32(0x47A880, 0x02048AEB, (DWORD)&test_texture_address);
    //2
    MemWrite8(0x47A8A4, 0x03, 0xE8);
    FuncWrite32(0x47A8A5, 0x02048AEB, (DWORD)&test_texture_address);
    //3
    MemWrite8(0x47A8C9, 0x03, 0xE8);
    FuncWrite32(0x47A8CA, 0x02048AEB, (DWORD)&test_texture_address);
    //4
    MemWrite8(0x47A8EE, 0x03, 0xE8);
    FuncWrite32(0x47A8EF, 0x02048AEB, (DWORD)&test_texture_address);
    //5
    MemWrite8(0x47A913, 0x03, 0xE8);
    FuncWrite32(0x47A914, 0x02048AEB, (DWORD)&test_texture_address);
    //6
    MemWrite8(0x47A938, 0x03, 0xE8);
    FuncWrite32(0x47A939, 0x02048AEB, (DWORD)&test_texture_address);
    //7
    MemWrite8(0x47A95D, 0x03, 0xE8);
    FuncWrite32(0x47A95E, 0x02048AEB, (DWORD)&test_texture_address);
    // less than 8
    //0
    MemWrite8(0x47A99F, 0x03, 0xE8);
    FuncWrite32(0x47A9A0, 0x02048AEB, (DWORD)&test_texture_address);
    //1
    MemWrite8(0x47A9CF, 0x03, 0xE8);
    FuncWrite32(0x47A9D0, 0x02048AEB, (DWORD)&test_texture_address);
    //2
    MemWrite8(0x47AA00, 0x03, 0xE8);
    FuncWrite32(0x47AA01, 0x02048AEB, (DWORD)&test_texture_address);
    //3
    MemWrite8(0x47AA31, 0x03, 0xE8);
    FuncWrite32(0x47AA32, 0x02048AEB, (DWORD)&test_texture_address);
    //4
    MemWrite8(0x47AA5E, 0x03, 0xE8);
    FuncWrite32(0x47AA5F, 0x02048AEB, (DWORD)&test_texture_address);
    //5
    MemWrite8(0x47AA8B, 0x03, 0xE8);
    FuncWrite32(0x47AA8C, 0x02048AEB, (DWORD)&test_texture_address);
    //6
    MemWrite8(0x47AAB8, 0x03, 0xE8);
    FuncWrite32(0x47AAB9, 0x02048AEB, (DWORD)&test_texture_address);


    // poly draw func 04: texture
    // 8 or greater
    //0
    MemWrite8(0x47B0F9, 0x03, 0xE8);
    FuncWrite32(0x47B0FA, 0x02048AEB, (DWORD)&test_texture_address);
    //1
    MemWrite8(0x47B119, 0x03, 0xE8);
    FuncWrite32(0x47B11A, 0x02048AEB, (DWORD)&test_texture_address);
    //2
    MemWrite8(0x47B13A, 0x03, 0xE8);
    FuncWrite32(0x47B13B, 0x02048AEB, (DWORD)&test_texture_address);
    //3
    MemWrite8(0x47B15B, 0x03, 0xE8);
    FuncWrite32(0x47B15C, 0x02048AEB, (DWORD)&test_texture_address);
    //4
    MemWrite8(0x47B17C, 0x03, 0xE8);
    FuncWrite32(0x47B17D, 0x02048AEB, (DWORD)&test_texture_address);
    //5
    MemWrite8(0x47B19D, 0x03, 0xE8);
    FuncWrite32(0x47B19E, 0x02048AEB, (DWORD)&test_texture_address);
    //6
    MemWrite8(0x47B1BE, 0x03, 0xE8);
    FuncWrite32(0x47B1BF, 0x02048AEB, (DWORD)&test_texture_address);
    //7
    MemWrite8(0x47B1DF, 0x03, 0xE8);
    FuncWrite32(0x47B1E0, 0x02048AEB, (DWORD)&test_texture_address);
    // less than 8
    //0
    MemWrite8(0x47B21D, 0x03, 0xE8);
    FuncWrite32(0x47B21E, 0x02048AEB, (DWORD)&test_texture_address);
    //1
    MemWrite8(0x47B249, 0x03, 0xE8);
    FuncWrite32(0x47B24A, 0x02048AEB, (DWORD)&test_texture_address);
    //2
    MemWrite8(0x47B276, 0x03, 0xE8);
    FuncWrite32(0x47B277, 0x02048AEB, (DWORD)&test_texture_address);
    //3
    MemWrite8(0x47B2A3, 0x03, 0xE8);
    FuncWrite32(0x47B2A4, 0x02048AEB, (DWORD)&test_texture_address);
    //4
    MemWrite8(0x47B2CC, 0x03, 0xE8);
    FuncWrite32(0x47B2CD, 0x02048AEB, (DWORD)&test_texture_address);
    //5
    MemWrite8(0x47B2F5, 0x03, 0xE8);
    FuncWrite32(0x47B2F6, 0x02048AEB, (DWORD)&test_texture_address);
    //6
    MemWrite8(0x47B31E, 0x03, 0xE8);
    FuncWrite32(0x47B31F, 0x02048AEB, (DWORD)&test_texture_address);


    // poly draw func 05: texture highlight
    // 8 or greater
    //0
    MemWrite8(0x47B85F, 0x8A, 0xE8);
    FuncWrite32(0x47B860, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x47B864, 0x4A7590, 0x90909090);
    //1
    MemWrite8(0x47B88D, 0x8A, 0xE8);
    FuncWrite32(0x47B88E, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x47B892, 0x4A7590, 0x90909090);
    //2
    MemWrite8(0x47B8BC, 0x8A, 0xE8);
    FuncWrite32(0x47B8BD, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x47B8C1, 0x4A7590, 0x90909090);
    //3
    MemWrite8(0x47B8EB, 0x8A, 0xE8);
    FuncWrite32(0x47B8EC, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x47B8F0, 0x4A7590, 0x90909090);
    //4
    MemWrite8(0x47B91A, 0x8A, 0xE8);
    FuncWrite32(0x47B91B, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x47B91F, 0x4A7590, 0x90909090);
    //5
    MemWrite8(0x47B949, 0x8A, 0xE8);
    FuncWrite32(0x47B94A, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x47B94E, 0x4A7590, 0x90909090);
    //6
    MemWrite8(0x47B978, 0x8A, 0xE8);
    FuncWrite32(0x47B979, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x47B97D, 0x4A7590, 0x90909090);
    //7
    MemWrite8(0x47B9A7, 0x8A, 0xE8);
    FuncWrite32(0x47B9A8, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x47B9AC, 0x4A7590, 0x90909090);
    // less than 8
    //0
    MemWrite8(0x47B9F3, 0x8A, 0xE8);
    FuncWrite32(0x47B9F4, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x47B9F8, 0x4A7590, 0x90909090);
    //1
    MemWrite8(0x47BA2D, 0x8A, 0xE8);
    FuncWrite32(0x47BA2E, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x47BA32, 0x4A7590, 0x90909090);
    //2
    MemWrite8(0x47BA68, 0x8A, 0xE8);
    FuncWrite32(0x47BA69, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x47BA6D, 0x4A7590, 0x90909090);
    //3
    MemWrite8(0x47BAA3, 0x8A, 0xE8);
    FuncWrite32(0x47BAA4, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x47BAA8, 0x4A7590, 0x90909090);
    //4
    MemWrite8(0x47BADE, 0x8A, 0xE8);
    FuncWrite32(0x47BADF, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x47BAE3, 0x4A7590, 0x90909090);
    //5
    MemWrite8(0x47BB15, 0x8A, 0xE8);
    FuncWrite32(0x47BB16, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x47BB1A, 0x4A7590, 0x90909090);
    //6
    MemWrite8(0x47BB4C, 0x8A, 0xE8);
    FuncWrite32(0x47BB4D, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x47BB51, 0x4A7590, 0x90909090);
    //----------------------------------------------------------------------------
}
