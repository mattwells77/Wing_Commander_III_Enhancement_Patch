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
#include "configTools.h"

MOUSE Mouse;

bool mouse_double_click_left = false;


//_____________________________
void Check_Mouse_Double_Click() {

	static LONGLONG doubleclick_time = (LONGLONG)GetDoubleClickTime() * (*p_wc3_frequency).QuadPart / 1000LL;//ms to ticks
	static LONG doubleclick_width = GetSystemMetrics(SM_CXDOUBLECLK);
	static LONG doubleclick_height = GetSystemMetrics(SM_CYDOUBLECLK);

	static bool left_click_was_down = 0;
	static LARGE_INTEGER left_click_time = { 0 };
	static LONG left_click_left_x = 0;
	static LONG left_click_left_y = 0;


	if (*p_wc3_mouse_button & 0x1) {

		left_click_was_down = true;

		if (left_click_time.QuadPart > 0) {
			LARGE_INTEGER time = { 0 };
			QueryPerformanceCounter(&time);
			if (time.QuadPart <= left_click_time.QuadPart) {
				LONG x = (LONG)*p_wc3_mouse_x;
				LONG y = (LONG)*p_wc3_mouse_y;
				//Get_Mouse_Position(&x, &y);
		
				if (abs(x - left_click_left_x) > doubleclick_width || abs(y - left_click_left_y) > doubleclick_height)
					left_click_time.QuadPart = 0;
				else {
					mouse_double_click_left = true;
					*p_wc3_movie_halt_flag = true;
					//Debug_Info("MOUSE DOUBLE CLICK");
				}
			}
			else
				left_click_time.QuadPart = 0;
		}
	}
	else {
		if (left_click_time.QuadPart > 0) {
			LARGE_INTEGER time = { 0 };
			QueryPerformanceCounter(&time);
			if (time.QuadPart > left_click_time.QuadPart)
				left_click_time.QuadPart = 0;
		}
		else if (left_click_was_down) {
			QueryPerformanceCounter(&left_click_time);
			left_click_time.QuadPart += doubleclick_time;
			left_click_left_x = (LONG)*p_wc3_mouse_x;
			left_click_left_y = (LONG)*p_wc3_mouse_y;
			//Get_Mouse_Position(&left_click_left_x, &left_click_left_y);
		}
		mouse_double_click_left = false;
		*p_wc3_movie_halt_flag = false;
		left_click_was_down = false;
	}
}


//////////////////////////ACTION_KEY_MOUSE//////////////////////
 
//______________________________________________
bool ACTION_KEY_MOUSE::SetButton(bool new_state) {

	PROFILE_TYPE profile_type = current_pro_type;
	if (current_pro_type == PROFILE_TYPE::Space && current_pro_type_map != PROFILE_TYPE::Space)
		profile_type = current_pro_type_map;

	if (new_state == true && pressed == false) {
		active_profile = profile_type;//ensure this button is bound to the same profile untill it is released.
		Simulate_Key_Press(button[static_cast<int>(active_profile)]);
		pressed = true;
	}
	else if (new_state == false && pressed == true) {
		Simulate_Key_Release(button[static_cast<int>(active_profile)]);
		pressed = false;
	}
	return pressed;
};


//________________________________________________________
void ACTION_KEY_MOUSE::SetButton_Instant(LONG duration_ms) const {

	PROFILE_TYPE profile_type = current_pro_type;
	if (current_pro_type == PROFILE_TYPE::Space && current_pro_type_map != PROFILE_TYPE::Space)
		profile_type = current_pro_type_map;

	Simulate_Key_Pressed(button[static_cast<int>(profile_type)], duration_ms);
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

	PROFILE_TYPE saved_pro_type = current_pro_type;
	wchar_t profile_name[16];

	for (int i = 0; i < NUM_JOY_PROFILES; i++) {
		switch (i) {
		case 0:
			current_pro_type = PROFILE_TYPE::GUI;
			swprintf(profile_name, _countof(profile_name), L"MOUSE_GUI");
			action_key_button[0].SetAction(static_cast<WC3_ACTIONS>(ConfigReadInt_InGame(profile_name, L"BUTTON_01", static_cast<int>(WC3_ACTIONS::B1_Select))));
			action_key_button[1].SetAction(static_cast<WC3_ACTIONS>(ConfigReadInt_InGame(profile_name, L"BUTTON_02", static_cast<int>(WC3_ACTIONS::B2_Cycle_Hotspots))));
			break;
		case 1:
			current_pro_type = PROFILE_TYPE::NAV;
			swprintf(profile_name, _countof(profile_name), L"MOUSE_NAV");
			action_key_button[0].SetAction(static_cast<WC3_ACTIONS>(ConfigReadInt_InGame(profile_name, L"BUTTON_01", static_cast<int>(WC3_ACTIONS::NAV_Cycle_Points))));
			action_key_button[1].SetAction(static_cast<WC3_ACTIONS>(ConfigReadInt_InGame(profile_name, L"BUTTON_02", static_cast<int>(WC3_ACTIONS::B2_Modifier))));
			break;
		case 2:
			current_pro_type = PROFILE_TYPE::Space;
			swprintf(profile_name, _countof(profile_name), L"MOUSE_SPACE");
			action_key_button[0].SetAction(static_cast<WC3_ACTIONS>(ConfigReadInt_InGame(profile_name, L"BUTTON_01", static_cast<int>(WC3_ACTIONS::B1_Fire_Guns))));
			action_key_button[1].SetAction(static_cast<WC3_ACTIONS>(ConfigReadInt_InGame(profile_name, L"BUTTON_02", static_cast<int>(WC3_ACTIONS::B2_Modifier))));
			break;
		default:
			current_pro_type = static_cast<PROFILE_TYPE>(i);
			swprintf(profile_name, _countof(profile_name), L"MOUSE_REMAP_%02d", i - 1);
			action_key_button[0].SetAction(static_cast<WC3_ACTIONS>(ConfigReadInt_InGame(profile_name, L"BUTTON_01", static_cast<int>(WC3_ACTIONS::None))));
			action_key_button[1].SetAction(static_cast<WC3_ACTIONS>(ConfigReadInt_InGame(profile_name, L"BUTTON_02", static_cast<int>(WC3_ACTIONS::None))));
			break;
		}

		action_key_button[2].SetAction(static_cast<WC3_ACTIONS>(ConfigReadInt_InGame(profile_name, L"BUTTON_03", static_cast<int>(WC3_ACTIONS::None))));
		action_key_button[3].SetAction(static_cast<WC3_ACTIONS>(ConfigReadInt_InGame(profile_name, L"BUTTON_04", static_cast<int>(WC3_ACTIONS::None))));
		action_key_button[4].SetAction(static_cast<WC3_ACTIONS>(ConfigReadInt_InGame(profile_name, L"BUTTON_05", static_cast<int>(WC3_ACTIONS::None))));

		action_key_wheel_v[0].SetAction(static_cast<WC3_ACTIONS>(ConfigReadInt_InGame(profile_name, L"MOUSE_WHEEL_UP", static_cast<int>(WC3_ACTIONS::None))));
		action_key_wheel_v[1].SetAction(static_cast<WC3_ACTIONS>(ConfigReadInt_InGame(profile_name, L"MOUSE_WHEEL_DOWN", static_cast<int>(WC3_ACTIONS::None))));

		action_key_wheel_h[0].SetAction(static_cast<WC3_ACTIONS>(ConfigReadInt_InGame(profile_name, L"MOUSE_WHEEL_LEFT", static_cast<int>(WC3_ACTIONS::None))));
		action_key_wheel_h[1].SetAction(static_cast<WC3_ACTIONS>(ConfigReadInt_InGame(profile_name, L"MOUSE_WHEEL_RIGHT", static_cast<int>(WC3_ACTIONS::None))));
	}

	current_pro_type = saved_pro_type;
}


//________________
void MOUSE::Save() {

	ConfigWriteInt_InGame(L"MOUSE", L"DEAD_ZONE", Deadzone_Level());

	PROFILE_TYPE saved_pro_type = current_pro_type;
	wchar_t button_name[12];
	wchar_t profile_name[16];


	for (int i = 0; i < NUM_JOY_PROFILES; i++) {
		switch (i) {
		case 0:
			current_pro_type = PROFILE_TYPE::GUI;
			swprintf(profile_name, _countof(profile_name), L"MOUSE_GUI");
			break;
		case 1:
			current_pro_type = PROFILE_TYPE::NAV;
			swprintf(profile_name, _countof(profile_name), L"MOUSE_NAV");
			break;
		case 2:
			current_pro_type = PROFILE_TYPE::Space;
			swprintf(profile_name, _countof(profile_name), L"MOUSE_SPACE");
			break;
		default:
			current_pro_type = static_cast<PROFILE_TYPE>(i);
			swprintf(profile_name, _countof(profile_name), L"MOUSE_REMAP_%02d", i - 1);
			break;
		}

		for (int button = 0; button < NUM_MOUSE_BUTTONS; button++) {
			swprintf(button_name, _countof(button_name), L"BUTTON_%02d", button + 1);
			ConfigWriteInt_InGame(profile_name, button_name, static_cast<int>(action_key_button[button].GetAction()));
		}

		ConfigWriteInt_InGame(profile_name, L"MOUSE_WHEEL_UP", static_cast<int>(action_key_wheel_v[0].GetAction()));
		ConfigWriteInt_InGame(profile_name, L"MOUSE_WHEEL_DOWN", static_cast<int>(action_key_wheel_v[1].GetAction()));

		ConfigWriteInt_InGame(profile_name, L"MOUSE_WHEEL_LEFT", static_cast<int>(action_key_wheel_h[0].GetAction()));
		ConfigWriteInt_InGame(profile_name, L"MOUSE_WHEEL_RIGHT", static_cast<int>(action_key_wheel_h[1].GetAction()));

	}

	current_pro_type = saved_pro_type;
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
	Debug_Info_Joy("Update_Wheel_Vertical: %d", zDelta);

	float fzDelta = (float)zDelta / 120 * 60;

	if (zDelta > 0)
		action_key_wheel_v[0].SetButton_Instant((LONG)fzDelta);


	else if (zDelta < 0)
		action_key_wheel_v[1].SetButton_Instant(-(LONG)fzDelta);
}


//________________________________________________
void MOUSE::Update_Wheel_Horizontal(WPARAM wParam) {

	Setup();

	short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	Debug_Info_Joy("Update_Wheel_Horizontal: %d", zDelta);

	float fzDelta = (float)zDelta / 120 * 60;

	if (zDelta > 0)
		action_key_wheel_h[0].SetButton_Instant((LONG)fzDelta);
	else if (zDelta < 0)
		action_key_wheel_h[1].SetButton_Instant(-(LONG)fzDelta);
}


//_____________________________________________
WC3_ACTIONS MOUSE::GetAction_Button(int button) {

	Setup();

	if (button < 0 || button > NUM_MOUSE_BUTTONS)
		return WC3_ACTIONS::None;
	return action_key_button[button].GetAction();
}


//_____________________________________
WC3_ACTIONS MOUSE::GetAction_Wheel_Up() {

	Setup();

	return action_key_wheel_v[0].GetAction();
}


//_______________________________________
WC3_ACTIONS MOUSE::GetAction_Wheel_Down() {

	Setup();

	return action_key_wheel_v[1].GetAction();
}


//_______________________________________
WC3_ACTIONS MOUSE::GetAction_Wheel_Left() {

	Setup();

	return action_key_wheel_h[0].GetAction();
}


//________________________________________
WC3_ACTIONS MOUSE::GetAction_Wheel_Right() {

	Setup();

	return action_key_wheel_h[1].GetAction();
}


//__________________________________________________________
void MOUSE::SetAction_Button(int button, WC3_ACTIONS action) {

	Setup();

	if (button < 0 || button > NUM_MOUSE_BUTTONS)
		return ;
	action_key_button[button].SetAction(action);
}


//________________________________________________
void MOUSE::SetAction_Wheel_Up(WC3_ACTIONS action) {

	Setup();

	action_key_wheel_v[0].SetAction(action);
}


//__________________________________________________
void MOUSE::SetAction_Wheel_Down(WC3_ACTIONS action) {

	Setup();

	action_key_wheel_v[1].SetAction(action);
}


//__________________________________________________
void MOUSE::SetAction_Wheel_Left(WC3_ACTIONS action) {

	Setup();

	action_key_wheel_h[0].SetAction(action);
}


//___________________________________________________
void MOUSE::SetAction_Wheel_Right(WC3_ACTIONS action) {

	Setup();

	action_key_wheel_h[1].SetAction(action);
}
