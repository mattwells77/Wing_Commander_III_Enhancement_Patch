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

#include "joystick.h"
#include "joystick_config.h"
#include "wc3w.h"
#include "memwrite.h"
#include "configTools.h"


MOUSE Mouse;


WORD WC3_ACTIONS_KEYS_MOUSE[][2]{
	0x00, 0x00,		// None,
	0x00, 0x39,		// B1_Trigger,
	0x00, 0x00,		// B2_Modifier,
	0x00, 0x1C,		// B3_Missile,
	0x00, 0x15,		// B4_Lock_Closest_Enemy_And_Match_Speed,

	0x00, 0x48,		// Pitch_Down,
	0x00, 0x50,		// Pitch_Up,
	0x00, 0x4B,		// Yaw_Left,
	0x00, 0x4D,		// Yaw_Right,

	0x00, 0x47,		// Pitch Down, Yaw Left,
	0x00, 0x49,		// Pitch Down, Yaw Right,
	0x00, 0x4F,		// Pitch Up, Yaw Left,
	0x00, 0x51,		// Pitch Up, Yaw Right,

	0x00, 0x52,		// Roll_Left,
	0x00, 0x53,		// Roll_Right,
	0x00, 0x2A,		// Double_Yaw_Pich_Roll_Rates,

	0x00, 0x3A,		// Auto_slide
	0x00, 0x35,		// Toggle_Auto_slide
	0x00, 0x0D,		// Accelerate
	0x00, 0x0C,		// Decelerate
	0x00, 0x0E,		// Full_stop
	0x00, 0x2B,		// Full_speed
	//0x00, 0x15,		// Match_target_speed
	0x00, 0x0F,		// Afterburner
	0x00, 0x29,		// Toggle_Afterburner
	0x00, 0x1E,		// Autopilot
	0x00, 0x24,		// Jump
	0x1D, 0x2E,		// Cloak
	0x1D, 0x12,		// Eject
	0x38, 0x19,		// Pause
	//0x38, 0x2E,		// Calibrate_Joystick
	0x38, 0x18,		// Options_Menu
	0x00, 0x31,		// Nav_Map

	0x00, 0x14,		// Cycle_targets
	0x00, 0x13,		// Cycle_turrets
	0x00, 0x26,		// Lock_target
	0x1D, 0x1F,		// Toggle_Smart_Targeting
	0x00, 0x22,		// Cycle_guns
	0x00, 0x21,		// Full_guns
	0x1D, 0x22,		// Synchronize_guns
	0x1D, 0x1E,		// Toggle_Auto_Tracking
	0x00, 0x32,		// Config_Cycle_Missile
	0x00, 0x1B,		// Change_Missile__Increase_power_to_selected_component
	0x00, 0x1A,		// Select_Missile__Decrease_power_to_selected_component
	0x00, 0x30,		// Select_All_Missiles
	//0x00, 0x39,		// Fire_guns
	//0x00, 0x1C,		// Fire_missile
	0x00, 0x12,		// Drop_decoy

	0x00, 0x0B,		// Cycle_VDUs
	0x00, 0x1F,		// VDU_Shield
	0x00, 0x2E,		// VDU_Comms
	0x00, 0x20,		// VDU_Damage
	0x00, 0x11,		// VDU_Weapons
	0x00, 0x19,		// VDU_Power
	0x2A, 0x1B,		// Power_Set_Selected_Component_100
	0x2A, 0x1A,		// Power_Reset_Components_25
	0x1D, 0x1B,		// Lock_power_to_selected_component

	0x00, 0x3B,		// View_Front
	0x00, 0x3C,		// View_Left
	0x00, 0x3D,		// View_Righr
	0x00, 0x3E,		// View_Rear_Turret
	0x1D, 0x3E,		// View_Rear_Turret_VDU
	0x00, 0x3F,		// Camera_Chase
	0x00, 0x40,		// Camera_Object
	//0x00, 0x41,		// Tactical_view
	0x00, 0x42,		// Camera_Missile
	0x00, 0x43,		// Camera_Victim
	0x00, 0x44,		// Camera_Track

	0x1D, 0x2F,		// Disable_Video_In_Left_VDU

	0x38, 0x30,		// WingMan_Break_And_Attack,
	0x38, 0x21,		// WingMan_Form_On_Wing,
	0x38, 0x20,		// WingMan_Request_Status,
	0x38, 0x23,		// WingMan_Help_Me_Out,
	0x38, 0x1E,		// WingMan_Attack_My_Target,
	0x38, 0x14,		// Enemy_Taunt,
};


//__________________________________________________
static void Simulate_Key_Pressed(WC3_ACTIONS action) {
	if (JoyConfig_Refresh_CurrentAction_Mouse(action, TRUE))
		return;
	if (action == WC3_ACTIONS::None)
		return;

	WORD key_mod = WC3_ACTIONS_KEYS_MOUSE[static_cast<int>(action)][0];
	WORD key = WC3_ACTIONS_KEYS_MOUSE[static_cast<int>(action)][1];

	INPUT inputs[4] = {};
	ZeroMemory(inputs, sizeof(inputs));

	inputs[0].type = INPUT_KEYBOARD;
	//inputs[0].ki.wVk = key_mod;
	inputs[0].ki.wScan = key_mod;
	inputs[0].ki.dwFlags = KEYEVENTF_SCANCODE;

	inputs[1].type = INPUT_KEYBOARD;
	//inputs[1].ki.wVk = key;
	inputs[1].ki.wScan = key;
	inputs[1].ki.dwFlags = KEYEVENTF_SCANCODE;

	inputs[2].type = INPUT_KEYBOARD;
	//inputs[2].ki.wVk = key;
	inputs[2].ki.wScan = key;
	inputs[2].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;

	inputs[3].type = INPUT_KEYBOARD;
	//inputs[3].ki.wVk = key_mod;
	inputs[3].ki.wScan = key_mod;
	inputs[3].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;

	UINT num_inputs = 4;
	INPUT* p_input = inputs;
	if (key_mod == 0) {//don't proccess mod key if there is none.
		num_inputs = 2;
		p_input = &inputs[1];
	}

	UINT uSent = SendInput(num_inputs, p_input, sizeof(INPUT));
	if (uSent != num_inputs)
		Debug_Info_Error("Simulate_Key_Press - SendInput failed: 0x%x\n", HRESULT_FROM_WIN32(GetLastError()));
}


//________________________________________________
static void Simulate_Key_Press(WC3_ACTIONS action) {
	if (JoyConfig_Refresh_CurrentAction_Mouse(action, TRUE))
		return;

	switch (action) {
	case WC3_ACTIONS::None:
		return;
	case WC3_ACTIONS::B1_Trigger:
		mouse_state_space[0] |= (1 << 0);
		return;
	case WC3_ACTIONS::B2_Modifier:
		mouse_state_space[0] |= (1 << 1);
		return;
	default:
		break;
	}
	WORD key_mod = WC3_ACTIONS_KEYS_MOUSE[static_cast<int>(action)][0];
	WORD key = WC3_ACTIONS_KEYS_MOUSE[static_cast<int>(action)][1];

	//Debug_Info("Simulate_Key_Press, key_mod:%d, key:%d", key_mod, key);
	INPUT inputs[2] = { 0 };

	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wScan = key_mod;
	inputs[0].ki.dwFlags = KEYEVENTF_SCANCODE;

	inputs[1].type = INPUT_KEYBOARD;
	inputs[1].ki.wScan = key;
	inputs[1].ki.dwFlags = KEYEVENTF_SCANCODE;

	UINT num_inputs = 2;
	INPUT* p_input = inputs;
	if (key_mod == 0) {//don't proccess mod key if there is none.
		num_inputs = 1;
		p_input = &inputs[1];
	}

	UINT uSent = SendInput(num_inputs, p_input, sizeof(INPUT));
	if (uSent != num_inputs)
		Debug_Info_Error("Simulate_Key_Press - SendInput failed: 0x%x\n", HRESULT_FROM_WIN32(GetLastError()));
}


//__________________________________________________
static void Simulate_Key_Release(WC3_ACTIONS action) {
	if (JoyConfig_Refresh_CurrentAction_Mouse(action, FALSE))
		return;

	switch (action) {
	case WC3_ACTIONS::None:
		return;
	case WC3_ACTIONS::B1_Trigger:
		mouse_state_space[0] &= ~(1 << 0);
		return;
	case WC3_ACTIONS::B2_Modifier:
		mouse_state_space[0] &= ~(1 << 1);
		return;
	default:
		break;
	}

	WORD key_mod = WC3_ACTIONS_KEYS_MOUSE[static_cast<int>(action)][0];
	WORD key = WC3_ACTIONS_KEYS_MOUSE[static_cast<int>(action)][1];

	//Debug_Info("Simulate_Key_Release, key_mod:%d, key:%d", key_mod, key);
	INPUT inputs[2] = { 0 };

	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wScan = key;
	inputs[0].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;

	inputs[1].type = INPUT_KEYBOARD;
	inputs[1].ki.wScan = key_mod;
	inputs[1].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;

	UINT num_inputs = 2;
	INPUT* p_input = inputs;
	if (key_mod == 0)//don't proccess mod key if there is none.
		num_inputs = 1;

	UINT uSent = SendInput(num_inputs, p_input, sizeof(INPUT));
	if (uSent != num_inputs)
		Debug_Info_Error("Simulate_Key_Release - SendInput failed: 0x%x\n", HRESULT_FROM_WIN32(GetLastError()));
}



//////////////////////////ACTION_KEY_MOUSE//////////////////////
 
//______________________________________________
bool ACTION_KEY_MOUSE::SetButton(bool new_state) {

	if (new_state == true && pressed == false) {
		Simulate_Key_Press(button);
		pressed = true;
	}
	else if (new_state == false && pressed == true) {
		Simulate_Key_Release(button);
		pressed = false;
	}

	return pressed;
};
//________________________________________
void ACTION_KEY_MOUSE::SetButton_Instant() const {
		Simulate_Key_Pressed(button);
};



//////////////////////////MOUSE/////////////////////////////////

//_________________
void MOUSE::Setup() {
	if (setup)
		return;
	Load();
	setup = true;
}


//________________
void MOUSE::Load() {

	Set_Deadzone_Level(ConfigReadInt_InGame(L"MOUSE", L"DEAD_ZONE", CONFIG_MOUSE_DEAD_ZONE));

	action_key_button[0].SetAction(static_cast<WC3_ACTIONS>(ConfigReadInt_InGame(L"MOUSE", L"BUTTON_01", CONFIG_MOUSE_BUTTON_01)));
	action_key_button[1].SetAction(static_cast<WC3_ACTIONS>(ConfigReadInt_InGame(L"MOUSE", L"BUTTON_02", CONFIG_MOUSE_BUTTON_02)));
	action_key_button[2].SetAction(static_cast<WC3_ACTIONS>(ConfigReadInt_InGame(L"MOUSE", L"BUTTON_03", CONFIG_MOUSE_BUTTON_03)));
	action_key_button[3].SetAction(static_cast<WC3_ACTIONS>(ConfigReadInt_InGame(L"MOUSE", L"BUTTON_04", CONFIG_MOUSE_BUTTON_04)));
	action_key_button[4].SetAction(static_cast<WC3_ACTIONS>(ConfigReadInt_InGame(L"MOUSE", L"BUTTON_05", CONFIG_MOUSE_BUTTON_05)));
	
	action_key_wheel_v[0].SetAction(static_cast<WC3_ACTIONS>(ConfigReadInt_InGame(L"MOUSE", L"MOUSE_WHEEL_UP", CONFIG_MOUSE_WHEEL_UP)));
	action_key_wheel_v[1].SetAction(static_cast<WC3_ACTIONS>(ConfigReadInt_InGame(L"MOUSE", L"MOUSE_WHEEL_DOWN", CONFIG_MOUSE_WHEEL_DOWN)));

	action_key_wheel_h[0].SetAction(static_cast<WC3_ACTIONS>(ConfigReadInt_InGame(L"MOUSE", L"MOUSE_WHEEL_LEFT", CONFIG_MOUSE_WHEEL_LEFT)));
	action_key_wheel_h[1].SetAction(static_cast<WC3_ACTIONS>(ConfigReadInt_InGame(L"MOUSE", L"MOUSE_WHEEL_RIGHT", CONFIG_MOUSE_WHEEL_RIGHT)));
}


//________________
void MOUSE::Save() {

	ConfigWriteInt_InGame(L"MOUSE", L"DEAD_ZONE", Deadzone_Level());

	wchar_t button_name[12];
	for (int button = 0; button < NUM_MOUSE_BUTTONS; button++) {
		swprintf(button_name, _countof(button_name), L"BUTTON_%02d", button+1);
		ConfigWriteInt_InGame(L"MOUSE", button_name, static_cast<int>(action_key_button[button].GetAction()));
	}

	ConfigWriteInt_InGame(L"MOUSE", L"MOUSE_WHEEL_UP", static_cast<int>(action_key_wheel_v[0].GetAction()));
	ConfigWriteInt_InGame(L"MOUSE", L"MOUSE_WHEEL_DOWN", static_cast<int>(action_key_wheel_v[1].GetAction()));

	ConfigWriteInt_InGame(L"MOUSE", L"MOUSE_WHEEL_LEFT", static_cast<int>(action_key_wheel_h[0].GetAction()));
	ConfigWriteInt_InGame(L"MOUSE", L"MOUSE_WHEEL_RIGHT", static_cast<int>(action_key_wheel_h[1].GetAction()));
}


//_______________________________________
void MOUSE::Update_Buttons(WPARAM wParam) {

	Setup();

	int key_state = GET_KEYSTATE_WPARAM(wParam);
	if (key_state & MK_LBUTTON)
		action_key_button[0].SetButton(true);
	else
		action_key_button[0].SetButton(false);
	if (key_state & MK_RBUTTON)
		action_key_button[1].SetButton(true);
	else
		action_key_button[1].SetButton(false);
	if (key_state & MK_MBUTTON)
		action_key_button[2].SetButton(true);
	else
		action_key_button[2].SetButton(false);
	if (key_state & MK_XBUTTON1)
		action_key_button[3].SetButton(true);
	else
		action_key_button[3].SetButton(false);
	if (key_state & MK_XBUTTON2)
		action_key_button[4].SetButton(true);
	else
		action_key_button[4].SetButton(false);
}


//______________________________________________
void MOUSE::Update_Button(int button, bool state) {

	Setup();

	if (button < 0 || button >= NUM_MOUSE_BUTTONS)
		return;

	action_key_button[button].SetButton(state);

}


//______________________________________________
void MOUSE::Update_Wheel_Vertical(WPARAM wParam) {

	Setup();

	short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	if (zDelta > 0) 
		action_key_wheel_v[0].SetButton_Instant();
	else if (zDelta < 0) 
		action_key_wheel_v[1].SetButton_Instant();
}


//________________________________________________
void MOUSE::Update_Wheel_Horizontal(WPARAM wParam) {

	Setup();

	short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	if (zDelta > 0) 
		action_key_wheel_h[0].SetButton_Instant();
	else if (zDelta < 0)
		action_key_wheel_h[1].SetButton_Instant();
}


WC3_ACTIONS MOUSE::GetAction_Button(int button) {
	Setup();
	if (button < 0 || button > NUM_MOUSE_BUTTONS)
		return WC3_ACTIONS::None;
	return action_key_button[button].GetAction();
}


WC3_ACTIONS MOUSE::GetAction_Wheel_Up() {
	Setup();
	return action_key_wheel_v[0].GetAction();
}
WC3_ACTIONS MOUSE::GetAction_Wheel_Down() {
	Setup();
	return action_key_wheel_v[1].GetAction();
}
WC3_ACTIONS MOUSE::GetAction_Wheel_Left() {
	Setup();
	return action_key_wheel_h[0].GetAction();
}
WC3_ACTIONS MOUSE::GetAction_Wheel_Right() {
	Setup();
	return action_key_wheel_h[1].GetAction();
}


void MOUSE::SetAction_Button(int button, WC3_ACTIONS action) {
	if (button < 0 || button > NUM_MOUSE_BUTTONS)
		return ;
	action_key_button[button].SetAction(action);
}
void MOUSE::SetAction_Wheel_Up(WC3_ACTIONS action) {
	action_key_wheel_v[0].SetAction(action);
}
void MOUSE::SetAction_Wheel_Down(WC3_ACTIONS action) {
	action_key_wheel_v[1].SetAction(action);
}
void MOUSE::SetAction_Wheel_Left(WC3_ACTIONS action) {
	action_key_wheel_h[0].SetAction(action);
}
void MOUSE::SetAction_Wheel_Right(WC3_ACTIONS action) {
	action_key_wheel_h[1].SetAction(action);
}