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


//______________________
static void Joy_Update() {

	Check_Simulated_Key_For_Release();

	if (controller_enhancements_enabled)
		Joysticks.Update();
	else {
		//using original joystick fuctions.
		wc3_update_joystick();
		wc3_proccess_joystick_data();
		//add joy button to mouse button for double-click check.
		static bool left_click = false;
		if (current_pro_type == PROFILE_TYPE::GUI && *p_wc3_joy_buttons & 1) {
			*p_wc3_mouse_button |= 1;
			left_click = true;
			//clear joy button 1 so as not register as clicked after double-click in GUI. 
			*p_wc3_joy_buttons &= ~1;
		}
		else if (left_click) {
			*p_wc3_mouse_button &= ~1;
			left_click = false;
		}
	}

	Check_Mouse_Double_Click();
}

/*
//_________________________________________________
static void __declspec(naked) joy_update_main(void) {

	__asm {
		pushad
		call Joy_Update
		popad
		ret
	}
}*/

//_________________________________________________
static void __declspec(naked) joy_update_main(void) {

	__asm {
		mov eax, p_wc3_key_pressed_character_code
		mov byte ptr ds : [eax] , bl

		pushad
		call Joy_Update
		popad
		ret
	}
}


//____________________________________________________
static void __declspec(naked) joy_update_buttons(void) {

	__asm {
		pushad
		call Joy_Update
		popad
		ret
	}
}


//___________________________________
static void Joystick_Setup(LONG flag) {

	//Debug_Info("Joystick_Setup - flag:%d", flag);
	if (flag == -1)
		JoyConfig_Main();

	if (!controller_enhancements_enabled)
		wc3_setup_joystick(flag);
}


//___________________________________________
static void __declspec(naked) joy_setup(void) {

	__asm {
		mov eax, dword ptr ss : [esp + 0x4]
		push ebx
		push ecx
		push edx
		push edi
		push esi
		push ebp

		push eax
		call Joystick_Setup
		add esp, 0x4

		pop ebp
		pop esi
		pop edi
		pop edx
		pop ecx
		pop ebx

		ret 0x4
	}
}


//______________________________________________
static void Joystick_Setup_Alt_O_Menu(LONG flag) {
	
	PROFILE_TYPE saved_pro_type = current_pro_type;
	current_pro_type = PROFILE_TYPE::Space;

	Joystick_Setup(flag);

	current_pro_type = saved_pro_type;
}


//______________________________________________________
static void __declspec(naked) joy_setup_alt_o_menu(void) {

	__asm {
		mov eax, dword ptr ss : [esp + 0x4]
		push ebx
		push ecx
		push edx
		push edi
		push esi
		push ebp

		push eax
		call Joystick_Setup_Alt_O_Menu
		add esp, 0x4

		pop ebp
		pop esi
		pop edi
		pop edx
		pop ecx
		pop ebx

		ret 0x4
	}
}


//___________________________________________________
static void __declspec(naked) joy_roll_variable(void) {
	//p_wc3_joy_move_r was originaly a direction flag -1 to 1.
	//now holds the full range of motion on the roll axis.
	__asm {
		mov ecx, dword ptr ds : [esi + 0x650]
		mov edi, p_wc3_joy_move_r
		cmp dword ptr ds : [edi] , 0
		je exit_func
		mov edi, dword ptr ds : [edi]
		mov dword ptr ds : [ecx + 0x14] , edi//insert roll axis motion into the player movement structure.
		exit_func :
		ret
	}
}


//__________________________________________
void Modifications_Controller_Enhancements() {

	controller_enhancements_enabled = true;

	// get throttle value fixes-------------
	//skip over JOYCAPS.wCaps & JOYCAPS_HASZ
	MemWrite16(0x42932A, 0x05F6, 0x07EB);

	//skip over JOYCAPS.wZmax - JOYCAPS.wZmin
	MemWrite16(0x42933C, 0x0D8B, 0x0CEB);

	//skip over other maniputations
	MemWrite16(0x42934F, 0x052B, 0x11EB);
	//--------------------------------------

	//make the roll axis variable -------------
	MemWrite16(0x429C41, 0x8E8B, 0xE890);
	FuncWrite32(0x429C43, 0x0650, (DWORD)&joy_roll_variable);
	//----------------------------------------
}


//___________________________
void Modifications_Joystick() {

	FuncReplace32(0x404705, 0x00003887, (DWORD)&joy_setup);
	FuncReplace32(0x4082C8, 0xFFFFFCC4, (DWORD)&joy_setup);
	FuncReplace32(0x41288C, 0xFFFF5700, (DWORD)&joy_setup);
	FuncReplace32(0x435A17, 0xFFFD2575, (DWORD)&joy_setup);
	FuncReplace32(0x438948, 0xFFFCF644, (DWORD)&joy_setup_alt_o_menu);
	FuncReplace32(0x450345, 0xFFFB7C47, (DWORD)&joy_setup);
	FuncReplace32(0x45F4A6, 0xFFFA8AE6, (DWORD)&joy_setup);

	//update controllers before checking window messages.
	MemWrite16(0x482FC9, 0x1D88, 0xE890);
	FuncWrite32(0x482FCB, 0x4A7E2C, (DWORD)&joy_update_main);

	//disable calls to update joystick function. This is now done in "joy_update_main". 
	MemWrite8(0x4083B0, 0xE9, 0xC3);
	MemWrite32(0x4083B1, 0x07A6DB, 0x90909090);

	MemWrite8(0x481CA1, 0xE8, 0x90);
	MemWrite32(0x481CA2, 0x0DEA, 0x90909090);

	//disable calls to proccess joystick data function. This is now done in "joy_update_main".  
	MemWrite8(0x408409, 0xE8, 0x90);
	MemWrite32(0x40840A, 0x07A9E2, 0x90909090);

	MemWrite8(0x481CA8, 0xE8, 0x90);
	MemWrite32(0x481CA9, 0x1143, 0x90909090);

	//disable calls to update joystick buttons, for movies. This is now done in "joy_update_main".  
	MemWrite8(0x482BA0, 0x83, 0xC3);
	MemWrite16(0x482BA1, 0x34EC, 0x9090);

	//disable double-click check for movies. This is now done in "Check_Mouse_Double_Click".
	MemWrite8(0x41BCE0, 0x83, 0xC3);
	MemWrite16(0x41BCE1, 0x10EC, 0x9090);
}
