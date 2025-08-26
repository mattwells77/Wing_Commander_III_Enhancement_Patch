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


//____________________________________________________________________________________________________________________
static void Check_And_Set_Wav_Audio_Data(void* file_struct, AUDIO_DATA_STRUCT* data_struct, void* audio_format_struct) {

    //DWORD file_size = ((DWORD*)file_struct)[0];
    void* p_file_data = (void*)((DWORD*)file_struct)[5];

    if (((DWORD*)p_file_data)[0] == 0x46464952) { //RIFF

        data_struct->p_data_ptr = (BYTE*)p_file_data + 44;
        //copy the wav format structure.
        memcpy(audio_format_struct, (BYTE*)p_file_data + 20, 16);

        //Debug_Info("sound has WAV format - sample rate:%dHz, bits per sample:%d, num channels:%d", ((DWORD*)audio_format_struct)[1], ((WORD*)audio_format_struct)[7], ((WORD*)audio_format_struct)[1]);
    }
    else if (((DWORD*)p_file_data)[0] == 0x61657243 && ((DWORD*)p_file_data)[1] == 0x65766974 && ((DWORD*)p_file_data)[2] == 0x696F5620 &&
        ((DWORD*)p_file_data)[3] == 0x46206563 && ((DWORD*)p_file_data)[4] == 0x1A656C69) {//"Creative Voice File(0x1A)"
        
        data_struct->p_data_ptr = (BYTE*)p_file_data + 0x20;
        
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
        cmp dword ptr ds:[edx], 0x46464952//RIFF
        jne check_voc

        //sub ebx, 0x2C   //adjust data size to exclude size of wav header
        mov ebx, dword ptr ds : [edx + 0x28]
        mov eax, -1     //set format as NOT an ACM, but still check is ACM tail is present to fix the second sound(ref 0x15) in the German version of the file "VICD2.IFF". Which has both a WAV header and ACM tail.

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

        mov ebx, dword ptr ds : [edx + 0x1B]//data size 3 bytes
        and ebx, 0x00FFFFFF//mask out 4th byte, Frequency divisor 
        sub ebx, 0x2//subtract Frequency divisor and codec flag from data size.

        mov eax, -1

        check_acm:
        cmp dword ptr ds:[edi], 0x214D4341//ACM!
        je is_acm

        //fix for the third sound(ref 0x1A) in the German version of the file "VICD2.IFF". The header position is off by one.  
        //retry the acm check after subtracting the pointer position by one.
        sub edi, 1
        sub ebx, 1
        cmp dword ptr ds:[edi], 0x214D4341//ACM!
        je is_acm

        add ebx, 0x23   //add the size of the ACM tail back to the data size, as it was not found.
        mov eax, -1     //set format as NOT an ACM.

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
}
