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
    const char* pos = strstr(lpFileName, ".WSG");//check if saved game file.
    if(!pos)
        pos = strstr(lpFileName, "SFOSAVED.DAT");//check if settings file.
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

    const char* pos = strstr(lpFileName, ".WSG");//check if saved game file.
    if (!pos)
        pos = strstr(lpFileName, "SFOSAVED.DAT");//check if settings file.
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
    const char* pos = strstr(lpFileName, "00000102.wsg");
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
}
