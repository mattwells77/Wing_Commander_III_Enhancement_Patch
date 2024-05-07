/*
The MIT License (MIT)
Copyright � 2024 Matt Wells

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the �Software�), to deal in the
Software without restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED �AS IS�, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
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


//_______________________________
void Modifications_GeneralFixes() {

    //Fix memory leak issues surrounding thread creation and destruction which can causes crashes.
    MemWrite32(0x41C47B, 0x41B700, (DWORD)&movie_thread_cycle_branches);

    MemWrite16(0x41C483, 0x15FF, 0xE890);
    FuncWrite32(0x41C485, 0x4B5320, (DWORD)&movie_thread_begin_fix);

    MemWrite16(0x41C646, 0x15FF, 0xE890);
    FuncWrite32(0x41C648, 0x4B531C, (DWORD)&movie_thread_end_fix);
}
