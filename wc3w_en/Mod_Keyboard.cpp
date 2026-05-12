/*
The MIT License (MIT)
Copyright © 2026 Matt Wells

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

#include "input.h"
#include "input_config.h"
#include "wc3w.h"
#include "memwrite.h"

LONG vdu_comms_selected_line = -1;
BOOL vdu_comms_had_focus = FALSE;
BYTE vdu_comms_highlight[256]{ 0x0 };


//________________________________
static LONG VDU_Comms_Check_Keys() {

    LONG key = (LONG)*p_wc3_key_scancode;

    switch (key) {
    case 0x2E://[C] key pressed. cycle though comms list.
        if (vdu_comms_had_focus)
            vdu_comms_selected_line++;
        if (vdu_comms_selected_line >= *p_wc3_vdu_comms_list_size)
            vdu_comms_selected_line = 0;
        //Debug_Info("Num Comms: %d %d", comms_lst_current, num_options);
        break;

    case 1://   [esc] key pressed. return to previous list or exit.
        if (vdu_comms_selected_line > 0)
            vdu_comms_selected_line = 0;
        break;
        //number key pressed.
    case 2://   [1] key press
    case 3://   [2]
    case 4://   [3]
    case 5://   [4]
    case 6://   [5]
    case 7://   [6]
    case 8://   [7]
    case 9://   [8]
    case 10://  [9]
        if (vdu_comms_selected_line < 0)//exit if no item is currently selected(-1).
            break;
        if (key - 2 >= *p_wc3_vdu_comms_list_size)
            vdu_comms_selected_line = 0;
        break;
    case 0x1B://']' key pressed. select highlighted list item.
        if (vdu_comms_selected_line < 0)//exit if no item is currently selected(-1).
            break;
        key = vdu_comms_selected_line + 2;// add 2 to set a number key, as their codes start at '2' which equals the [1] key.
        if (vdu_comms_selected_line > 0)
            vdu_comms_selected_line = 0;
        break;
    case 0x1A://'[' key pressed. return to previous list or exit.
        key = 1;//[esc]
        if (vdu_comms_selected_line > 0)
            vdu_comms_selected_line = 0;
        break;
    default:
        break;
    }

    vdu_comms_had_focus = TRUE;
    return key;
}


//______________________________________________________
static void __declspec(naked) vdu_comms_check_keys(void) {

    __asm {

        push ebx
        push ecx
        push edi
        push esi
        push ebp

        call VDU_Comms_Check_Keys

        pop ebp
        pop esi
        pop edi
        pop ecx
        pop ebx

        mov edx, eax
        ret
    }
}


//___________________________________________________________________
static BYTE* VDU_Comms_Get_Pal_Highlight_Offsets(BYTE* p_pal_offsets) {

    static bool run_once = false;

    if (run_once)
        return vdu_comms_highlight;

    run_once = true;//run setup once
    //vdu_comms_highlight is an 256 byte array of pal offsets, with 0-254 filled with the same pal offset and 255 set to the value 255 as mask colour.
    //green
    memset(vdu_comms_highlight, 0x3B, sizeof(vdu_comms_highlight));
    vdu_comms_highlight[255] = 0xFF;

    return vdu_comms_highlight;
}


//____________________________________________________________________________________________________________________________________________
static void VDU_Comms_Draw_Menu_Text(LONG line_num, DRAW_BUFFER* p_toBuff, DWORD x, DWORD y, DWORD unk1, char* text_buff, BYTE* p_pal_offsets) {

    //highlight menu text if line is selected.
    if (line_num - 1 == vdu_comms_selected_line)
        p_pal_offsets = VDU_Comms_Get_Pal_Highlight_Offsets(p_pal_offsets);
    wc3_draw_text_to_buff(p_toBuff, x, y, unk1, text_buff, p_pal_offsets);
}


//__________________________________________________________
static void __declspec(naked) vdu_comms_draw_menu_text(void) {

    __asm {
        push[esp + 0x18]
        push[esp + 0x18]
        push[esp + 0x18]
        push[esp + 0x18]
        push[esp + 0x18]
        push[esp + 0x18]
        push ebx //current line num
        call VDU_Comms_Draw_Menu_Text
        add esp, 0x1C

        ret
    }
}


//______________________________________________________________
static void __declspec(naked) vdu_check_if_comms_had_focus(void) {

    __asm {
        mov edx, p_wc3_vdu_focus
        cmp byte ptr ds : [edx] , 0x4 //4 == comms
        je exit_func

        mov vdu_comms_had_focus, FALSE
        exit_func :
        //insert original code
        cmp byte ptr ds : [edx] , 0xFF
            ret
    }
}


//____________________________________________________
static void __declspec(naked) proccess_key_state(void) {

	__asm {
		mov cl, byte ptr ss : [esp + 0x18]//key transition state
		pushad
		push ecx
		push eax
		call Set_Key_State
		add esp, 0x8
		popad

		//insert original code
		mov cl, byte ptr ss : [esp + 0x14]//extended key flag
		cmp al, 0x2A//compare key scancode to Left Shift
		ret
	}
}


//__________________________________
static BYTE GUI_Alt_X_Message_Loop() {

	BYTE key = 0;
	//wait for alt+x keys to be released.
	do {
		wc3_update_input_states();
		key = Get_Pressed_Key();
	} while (key != 0);
	//wait for key press
	do {
		wc3_update_input_states();
		key = Get_Pressed_Key();
	} while (key == 0);

	//convert key to char. exit char depends on set language, Y for English.
	return MapVirtualKeyA(MapVirtualKeyA(key, MAPVK_VSC_TO_VK), MAPVK_VK_TO_CHAR);
}


//________________________________________________________
static void __declspec(naked) gui_alt_x_message_loop(void) {

	__asm {
		push ebp
		push ebx

		call GUI_Alt_X_Message_Loop

		pop ebx
		pop ebp
		ret
	}
}


//____________________________________________________________
static void __declspec(naked) replay_screen_check_yes_no(void) {

	__asm {
		mov eax, p_wc3_key_scancode
		mov eax, dword ptr ds : [eax]

		cmp eax, 0x15 //'Y'
		jne check_no
		mov eax, dword ptr ds : [esi + 0x14]
		jmp exit_func

		check_no :
		cmp eax, 0x31 //'N'
			jne exit_func
			mov eax, dword ptr ds : [esi + 0x18]

			exit_func :
			ret
	}
}

/*
//___________________________
void Print_Scancode(int code) {
	Debug_Info("Key Scancode: %x", code);

}


//________________________________________________
static void __declspec(naked) print_scancode(void) {
	//insert the joystick message check function along with the main message check function.
	__asm {
		pushad
		push edx
		call Print_Scancode
		add esp, 0x4
		popad
		mov ebx, p_wc3_keyboard_state_main
		mov bl, byte ptr ds : [edx + ebx]
		ret
	}
}
*/

//___________________________
void Modifications_Keyboard() {
	
    //-----scrollable comms------------------------------------------------
    MemWrite16(0x446BA8, 0x3D80, 0xE890);
    FuncWrite32(0x446BAA, 0x4B15BC, (DWORD)&vdu_check_if_comms_had_focus);
    MemWrite8(0x446BAE, 0xFF, 0x90);

    MemWrite16(0x44756E, 0x158B, 0xE890);
    FuncWrite32(0x447570, 0x4A9B80, (DWORD)&vdu_comms_check_keys);
    //0x2D == key 'C' - 1// allow for 'C' key to be used as menu selector
    MemWrite8(0x447579, 0x2D, 0x2C);

    FuncReplace32(0x41129E, 0x06413F, (DWORD)&vdu_comms_draw_menu_text);
    //---------------------------------------------------------------------

	//update key state
	MemWrite16(0x4827F8, 0x4C8A, 0xE890);
	FuncWrite32(0x4827FA, 0x2A3C1024, (DWORD)&proccess_key_state);
	
	//wait for yes no input in Alt+X message loop.
	FuncReplace32(0x412A0E, 0x07057E, (DWORD)&gui_alt_x_message_loop);

	//allow replay mission options to respond to 'Y'yes "replay" and 'N'no "continue" keys as well as the usual options
	//to improve controller support.
	MemWrite8(0x43008B, 0xA1, 0xE8);
	FuncWrite32(0x43008C, 0x4A9B80, (DWORD)&replay_screen_check_yes_no);

	//print scancodes
	//MemWrite16(0x48280C, 0x9A8A, 0xE890);
	//FuncWrite32(0x48280E, 0x4B2180, (DWORD)&print_scancode);
}