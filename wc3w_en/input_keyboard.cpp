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

#include "configTools.h"
#include "wc3w.h"
#include "input.h"

using namespace std;

BYTE keyboard_state[256]{ 0 };
BYTE keyboard_state_once[256]{ 0 };


BYTE WC3_ACTIONS_KEYS[][2]{
	0x00, 0x00,		// None,
	0x39, 0x00,		// Fire_guns,
	0x00, 0x00,		// B2_Modifier,
	0x1C, 0x00,		// Fire_missile,
	0x15, 0x00,		// B4_Lock_Closest_Enemy_And_Match_Speed,

	0x48, 0x00, 	// Pitch_Down,
	0x50, 0x00,		// Pitch_Up,
	0x4B, 0x00,		// Yaw_Left,
	0x4D, 0x00,		// Yaw_Right,

	0x47, 0x00,		// Pitch Down, Yaw Left,
	0x49, 0x00,		// Pitch Down, Yaw Right,
	0x4F, 0x00,		// Pitch Up, Yaw Left,
	0x51, 0x00,		// Pitch Up, Yaw Right,

	0x52, 0x00,		// Roll_Left,
	0x53, 0x00,		// Roll_Right,
	0x2A, 0x00,		// Double_Yaw_Pich_Roll_Rates,

	0x3A, 0x00,		// Auto_slide
	0x35, 0x00,		// Toggle_Auto_slide
	0x0D, 0x00,		// Accelerate
	0x0C, 0x00,		// Decelerate
	0x0E, 0x00,		// Full_stop
	0x2B, 0x00,		// Full_speed
	0x0F, 0x00,		// Afterburner
	0x29, 0x00,		// Toggle_Afterburner
	0x1E, 0x00,		// Autopilot
	0x24, 0x00,		// Jump
	0x2E, 0x1D,		// Cloak
	0x12, 0x1D,		// Eject
	0x19, 0x38,		// Pause
	//0x2E, 0x38,		// Calibrate_Joystick
	0x18, 0x38,		// Options_Menu
	0x31, 0x00,		// Nav_Map

	0x14, 0x00,		// Cycle_targets
	0x13, 0x00,		// Cycle_turrets
	0x26, 0x00,		// Lock_target
	0x1F, 0x1D,		// Toggle_Smart_Targeting
	0x22, 0x00,		// Cycle_guns
	0x21, 0x00,		// Full_guns
	0x22, 0x1D,		// Synchronize_guns
	0x1E, 0x1D,		// Toggle_Auto_Tracking
	0x32, 0x00,		// Config_Cycle_Missile
	0x1B, 0x00,		// Change_Missile__Increase_power_to_selected_component
	0x1A, 0x00,		// Select_Missile__Decrease_power_to_selected_component
	0x30, 0x00,		// Select_All_Missiles
	0x12, 0x00,		// Drop_decoy

	0x0B, 0x00,		// Cycle_VDUs
	0x1F, 0x00,		// VDU_Shield
	0x2E, 0x00,		// VDU_Comms
	0x20, 0x00,		// VDU_Damage
	0x11, 0x00,		// VDU_Weapons
	0x19, 0x00,		// VDU_Power
	0x1B, 0x2A,		// Power_Set_Selected_Component_100
	0x1A, 0x2A,		// Power_Reset_Components_25
	0x1B, 0x1D,		// Lock_power_to_selected_component

	0x3B, 0x00,		// View_Front
	0x3C, 0x00,		// View_Left
	0x3D, 0x00,		// View_Righr
	0x3E, 0x00,		// View_Rear_Turret
	0x3E, 0x1D,		// View_Rear_Turret_VDU
	0x3F, 0x3F,		// Camera_Chase
	0x40, 0x00,		// Camera_Object
	//0x41, 0x00,		// Tactical_view
	0x42, 0x00,		// Camera_Missile
	0x43, 0x00,		// Camera_Victim
	0x44, 0x00,		// Camera_Track

	0x2F, 0x1D,		// Disable_Video_In_Left_VDU

	0x30, 0x38,		// WingMan_Break_And_Attack,
	0x21, 0x38,		// WingMan_Form_On_Wing,
	0x20, 0x38,		// WingMan_Request_Status,
	0x23, 0x38,		// WingMan_Help_Me_Out,
	0x1E, 0x38,		// WingMan_Attack_My_Target,
	0x14, 0x38,		// Enemy_Taunt,

	0x00, 0x00,		// Remap1
	0x00, 0x00,		// Remap2
	0x00, 0x00,		// Remap2

	0x1C, 0x00,		// Select,
	0x0F, 0x00,		// Cycle_Hotspots,
	0x0F, 0x2A,		// Cycle_Hotspots_Reverse,

	0x4B, 0x4B,		// Gamma_Increase,
	0x4D, 0x4D,		// Gamma_Decrease,
	
	0x32, 0x1D,		// Toggle_Sound_Effects,
	0x48, 0x1D,		// Sound_Effects_Volume_Increase,
	0x50, 0x1D,		// Sound_Effects_Volume_Decrease,

	0x32, 0x38,		// Toggle_Music,
	0x48, 0x38,		// Music_Volume_Increase,
	0x50, 0x38,		// Music_Volume_Decrease,

	0x48, 0x00,		// Save_Load_Up,
	0x50, 0x00,		// Save_Load_Down,
	0x49, 0x00,		// Save_Load_Page_Up,
	0x51, 0x00,		// Save_Load_Page_Down,

	0x2D, 0x38,		// Exit_Game,
	0x15, 0x00,		// Exit_Game_Yes,
	0x31, 0x00,		// Exit_Game_No,

	0x1A, 0x00,		// Zoom_In,
	0x1B, 0x00,		// Zoom_Out,
	0x2E, 0x00,		// NAV_Centre_View,
	0x1F, 0x00,		// NAV_Toggle_Starfield,
	0x22, 0x00,		// NAV_Toggle_Grid,
	0x30, 0x00,		// NAV_Toggle_Background,
	0x31, 0x00,		// NAV_Cycle_Points,
	0x19, 0x00,		// NAV_Cycle_Points_Reverse,

	0x01, 0x00,		// Escape,
};


//______________________________________
void Set_Key_State(BYTE key, BYTE state) {

	Debug_Info("Set_Key_State %X, %X", key, state);
	keyboard_state[key] = state;
	keyboard_state_once[key] = state;
}


//_____________________
void Clear_Key_States() {
	memset(keyboard_state, 0, sizeof(keyboard_state));
	memset(keyboard_state_once, 0, sizeof(keyboard_state_once));

	memset(p_wc3_keyboard_state_main, 0, 136);
}


//_____________________________________________________________
bool Get_Key_State(BYTE key, BYTE mod_key_flags, BYTE run_once) {

	bool ret_val = false;
	//mod_key_flags &= 0xF7;// ignore flag 0x8

	BYTE mod_keys = 0;
	mod_keys |= keyboard_state[0x2A];//L Shift
	mod_keys |= keyboard_state[0x36];// R Shift
	mod_keys |= (keyboard_state[0x1D] << 1);// Ctrl
	mod_keys |= (keyboard_state[0x38] << 2);// Alt

	if (run_once & 0x10) {
		if (keyboard_state_once[key] != 0)
			ret_val |= true;
		keyboard_state_once[key] = 0;
	}
	else {
		if (keyboard_state[key] != 0)
			ret_val |= true;
	}

	//always allow these actions to pass whether a mod key is down or not. 
	if (key == WC3_ACTIONS_KEYS[static_cast<int>(WC3_ACTIONS::B1_Fire_Guns)][0] ||
		key == WC3_ACTIONS_KEYS[static_cast<int>(WC3_ACTIONS::Fire_Missile)][0] ||
		key == WC3_ACTIONS_KEYS[static_cast<int>(WC3_ACTIONS::Afterburner)][0])
		return ret_val;

	//if mod keys don't match expected mod keys but mod keys expected or if there are mod keys but no mod keys expected.
	if ((!(mod_keys & mod_key_flags) && mod_key_flags) || (mod_keys && !mod_key_flags)) 
		return false;
	
	return ret_val;
}


//____________________
BYTE Get_Pressed_Key() {

	for (int i = 1; i < sizeof(keyboard_state); i++) {
		if (keyboard_state[i])
			return i;
	}
	return 0;
}
