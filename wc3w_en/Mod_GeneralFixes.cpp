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
}
