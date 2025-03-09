/*
The MIT License (MIT)
Copyright © 2025 Matt Wells

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

using namespace std;
using namespace winrt;
using namespace Windows::Gaming::Input;

JOYSTICKS Joysticks;
WC3_JOY_AXES wc3_joy_axes{};


WORD WC3_ACTIONS_KEYS[][2]{
	0x00, 0x00,		// None,
	0x00, 0x00,		// B1_Trigger,
	0x00, 0x00,		// B2_Modifier,
	0x00, 0x00,		// B3_Missile,
	0x00, 0x00,		// B4_Lock_Closest_Enemy_And_Match_Speed,

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
};


//__________________________________________________
bool Get_Joystick_Config_Path(wstring *p_ret_string) {
	if (!p_ret_string)
		return false;

	p_ret_string->assign(GetAppDataPath());
	if (!p_ret_string->empty())
		p_ret_string->append(L"\\");
	p_ret_string->append(JOYSTICK_CONFIG_PATH);
	
	Debug_Info_Joy("Get_Joystick_Config_Path: %S", p_ret_string->c_str());

	if (GetFileAttributes(p_ret_string->c_str()) == INVALID_FILE_ATTRIBUTES) {
		if (!CreateDirectory(p_ret_string->c_str(), nullptr)) {
			p_ret_string->clear();
			return false;
		}
	}
	return true;
}


//______________________________________________________
static void Simulate_Key_Pressed(WORD key_mod, WORD key) {
	//return;
	//Debug_Info("Simulate_Key_Pressed, key_mod:%d, key:%d", key_mod, key);
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
	if(JoyConfig_Refresh_CurrentAction(action, TRUE))
		return;

	switch (action) {
	case WC3_ACTIONS::None:
		return;
	case WC3_ACTIONS::B1_Trigger:
		*p_wc3_joy_buttons |= (1 << 0);
		return;
	case WC3_ACTIONS::B2_Modifier:
		*p_wc3_joy_buttons |= (1 << 1);
		return;
	case WC3_ACTIONS::B3_Missile:
		*p_wc3_joy_buttons |= (1 << 2);
		return;
	case WC3_ACTIONS::B4_Lock_Closest_Enemy_And_Match_Speed:
		*p_wc3_joy_buttons |= (1 << 3);
		return;

	case WC3_ACTIONS::Pitch_Down:
		wc3_joy_axes.y_neg = TRUE;
		return;
	case WC3_ACTIONS::Pitch_Up:
		wc3_joy_axes.y_pos = TRUE;
		return;
	case WC3_ACTIONS::Yaw_Left:
		wc3_joy_axes.x_neg = TRUE;
		return;
	case WC3_ACTIONS::Yaw_Right:
		wc3_joy_axes.x_pos = TRUE;
		return;

	case WC3_ACTIONS::Pitch_Down_Yaw_Left:
		wc3_joy_axes.y_neg = TRUE;
		wc3_joy_axes.x_neg = TRUE;
		return;
	case WC3_ACTIONS::Pitch_Down_Yaw_Right:
		wc3_joy_axes.y_neg = TRUE;
		wc3_joy_axes.x_pos = TRUE;
		return;
	case WC3_ACTIONS::Pitch_Up_Yaw_Left:
		wc3_joy_axes.y_pos = TRUE;
		wc3_joy_axes.x_neg = TRUE;
		return;
	case WC3_ACTIONS::Pitch_Up_Yaw_Right:
		wc3_joy_axes.y_pos = TRUE;
		wc3_joy_axes.x_pos = TRUE;
		return;

	case WC3_ACTIONS::Roll_Left:
		wc3_joy_axes.r_neg = TRUE;
		return;
	case  WC3_ACTIONS::Roll_Right:
		wc3_joy_axes.r_pos = TRUE;
		return;

	case WC3_ACTIONS::Double_Yaw_Pitch_Roll_Rates:
		wc3_joy_axes.button_mod = TRUE;
		return;
	default:
		break;
	}
	WORD key_mod = WC3_ACTIONS_KEYS[static_cast<int>(action)][0];
	WORD key = WC3_ACTIONS_KEYS[static_cast<int>(action)][1];

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
	if(JoyConfig_Refresh_CurrentAction(action, FALSE))
		return;

	switch (action) {
	case WC3_ACTIONS::None:
		return;
	case WC3_ACTIONS::B1_Trigger:
		*p_wc3_joy_buttons &= ~(1 << 0);
		return;
	case WC3_ACTIONS::B2_Modifier:
		*p_wc3_joy_buttons &= ~(1 << 1);
		return;
	case WC3_ACTIONS::B3_Missile:
		*p_wc3_joy_buttons &= ~(1 << 2);
		return;
	case WC3_ACTIONS::B4_Lock_Closest_Enemy_And_Match_Speed:
		*p_wc3_joy_buttons &= ~(1 << 3);
		return;

	case WC3_ACTIONS::Pitch_Down:
		wc3_joy_axes.y_neg = FALSE;
		return;
	case WC3_ACTIONS::Pitch_Up:
		wc3_joy_axes.y_pos = FALSE;
		return;
	case WC3_ACTIONS::Yaw_Left:
		wc3_joy_axes.x_neg = FALSE;
		return;
	case WC3_ACTIONS::Yaw_Right:
		wc3_joy_axes.x_pos = FALSE;
		return;

	case WC3_ACTIONS::Pitch_Down_Yaw_Left:
		wc3_joy_axes.y_neg = FALSE;
		wc3_joy_axes.x_neg = FALSE;
		return;
	case WC3_ACTIONS::Pitch_Down_Yaw_Right:
		wc3_joy_axes.y_neg = FALSE;
		wc3_joy_axes.x_pos = FALSE;
		return;
	case WC3_ACTIONS::Pitch_Up_Yaw_Left:
		wc3_joy_axes.y_pos = FALSE;
		wc3_joy_axes.x_neg = FALSE;
		return;
	case WC3_ACTIONS::Pitch_Up_Yaw_Right:
		wc3_joy_axes.y_pos = FALSE;
		wc3_joy_axes.x_pos = FALSE;
		return;

	case WC3_ACTIONS::Roll_Left:
		wc3_joy_axes.r_neg = FALSE;
		return;
	case  WC3_ACTIONS::Roll_Right:
		wc3_joy_axes.r_pos = FALSE;
		return;

	case WC3_ACTIONS::Double_Yaw_Pitch_Roll_Rates:
		wc3_joy_axes.button_mod = FALSE;
		return;
	default:
		break;
	}

	WORD key_mod = WC3_ACTIONS_KEYS[static_cast<int>(action)][0];
	WORD key = WC3_ACTIONS_KEYS[static_cast<int>(action)][1];

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


//////////////////////////ACTION_KEY/////////////////////////////////

//________________________________________
bool ACTION_KEY::SetButton(bool new_state) {

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


//////////////////////////ACTION_AXIS/////////////////////////////////

//__________________________________________
void ACTION_AXIS::Set_State(double axis_val) {

	if (calibrating) {
		if (axis_val < limits.min)
			limits.min = axis_val;
		if (axis_val > limits.max)
			limits.max = axis_val;
		//limits.size = limits.max - limits.min;
		//current_val = (axis_val - limits.min) / limits.size;
		if (rev_axis)
			axis_val = 1.0f - axis_val;

		//limits.centre = (axis_val - limits.min) / (limits.max - limits.min);

		current_val = axis_val;
		return;
	}
	if (rev_axis)
		axis_val = 1.0f - axis_val;

	if (axis_val > limits.max)
		current_val = limits.max;
	else if (axis_val < limits.min)
		current_val = limits.min;
	current_val = (axis_val - limits.min) / limits.span;
	
	double half_val = current_val / 2;

	if (is_centred) {
		if (axis_val > limits.centre_max)
			current_centred_val = limits.centre_max;
		else if (axis_val < limits.centre_min)
			current_centred_val = limits.centre_min;

		current_centred_val = (axis_val - limits.centre_min) / limits.centre_span;

		if (current_centred_val > limits.centre) {
			current_centred_val -= limits.centre;
			current_centred_val /= (limits.centre_max - limits.centre) * 2.0f;
		}
		else if (current_centred_val < limits.centre) {
			current_centred_val -= limits.centre;
			current_centred_val /= (limits.centre - limits.centre_min) * 2.0f;
		}
		else
			current_centred_val -= limits.centre;

	}

	bool new_state = false;

	switch (assiged_to) {
	case AXIS_TYPE::Yaw:// check if axis is assigned to Yaw.
		wc3_joy_axes.x += current_centred_val;
		break;
	case AXIS_TYPE::Pitch:// check if axis is assigned to Pitch
		wc3_joy_axes.y += current_centred_val;
		break;
	case AXIS_TYPE::Roll:// check if axis is assigned to Roll.
		wc3_joy_axes.r += current_centred_val;
		break;
		
	case AXIS_TYPE::Throttle:// check if axis is assigned to Throttle.
		wc3_joy_axes.t = current_val;
		break;
	case AXIS_TYPE::AsOneButton:// check if axis is assigned to imitate a single button press when pushed toward max.
		new_state = false;
		if (current_val >= 0.1)
			new_state = true;
		button_max.SetButton(new_state);
		break;
	case AXIS_TYPE::AsTwoButtons:// check if axis is assigned to imitate button presses at either end of the axis.
		new_state = false;
		if (current_centred_val > 0.1)
			new_state = true;
		button_max.SetButton(new_state);

		new_state = false;
		if (current_centred_val < -0.1)
			new_state = true;
		button_min.SetButton(new_state);
		break;

	case AXIS_TYPE::Yaw_Left:// check if axis is assigned to Yaw left.
		wc3_joy_axes.x -= half_val;
		break;
	case AXIS_TYPE::Yaw_Right:// check if axis is assigned to Yaw right.
		wc3_joy_axes.x += half_val;
		break;
	case AXIS_TYPE::Pitch_Up:// check if axis is assigned to Pitch up.
		wc3_joy_axes.y += half_val;
		break;
	case AXIS_TYPE::Pitch_Down:// check if axis is assigned to Pitch down.
		wc3_joy_axes.y -= half_val;
		break;
	case AXIS_TYPE::Roll_Left:// check if axis is assigned to Roll left.
		wc3_joy_axes.r += -half_val;
		break;
	case AXIS_TYPE::Roll_Right:// check if axis is assigned to Roll right.
		wc3_joy_axes.r += half_val;
		break;
	default:
		break;
	}
}


//_____________________________________
void ACTION_AXIS::Calibrate(BOOL state) {

	if (state) {
		limits.min = 1.0f;
		limits.max = 0.0f;
		limits.centre = 0.5f;
		calibrating = TRUE;
	}
	else {
		limits.span = limits.max - limits.min;

		//The original joystick function crops 1/64 from max and min values to account for hardware imprecision. Original range (-256 to 256) (-248 to 248). 
		limits.centre_max = limits.max - 0.015625;// 0.03125f;
		limits.centre_min = limits.min + 0.015625;// 0.03125f;

		limits.centre_span = limits.centre_max - limits.centre_min;
		calibrating = FALSE;
	}
};


//////////////////////////ACTION_SWITCH/////////////////////////////////

//_____________________________________________
int ACTION_SWITCH::Switch_Position(int new_pos) {

	if (!action || new_pos == current_position || new_pos >= num_positions)
		return current_position;
	//release current button
	if (action[current_position] != WC3_ACTIONS::None && current_position != 0)
		Simulate_Key_Release(action[current_position]);
	//press new button
	if (action[new_pos] != WC3_ACTIONS::None && new_pos != 0)
		Simulate_Key_Press(action[new_pos]);

	current_position = new_pos;
	return current_position;
};


//////////////////////////JOYSTICK/////////////////////////////////

//_______________________________________________________________________________________________________________________________________
JOYSTICK::JOYSTICK(winrt::Windows::Gaming::Input::RawGameController const& in_rawGameController) :rawGameController(in_rawGameController) {

	pid = rawGameController.HardwareProductId();
	vid = rawGameController.HardwareVendorId();
	NonRoamableId = rawGameController.NonRoamableId();

	DisplayName = rawGameController.DisplayName();
	//Debug_Info("NonRoamableId: %S", NonRoamableId.c_str());

	connected = true;
	enabled = true;

	num_buttons = rawGameController.ButtonCount();
	buttonArray = new bool[num_buttons];


	num_switches = rawGameController.SwitchCount();
	switchArray = new GameControllerSwitchPosition[num_switches];

	num_axes = rawGameController.AxisCount();
	axisArray = new double[num_axes];

	action_button = new ACTION_KEY[num_buttons]{};
	action_axis = new ACTION_AXIS[num_axes]{};

	GameControllerSwitchKind switchKind;
	int num_switch_positions = 0;
	action_switch = new ACTION_SWITCH[num_switches];
	for (int i = 0; i < num_switches; i++) {
		switchKind = rawGameController.GetSwitchKind(i);
		//switchKind enum: 0=2, 1=4, 2=8. shift 2 left by the enum val to get number of positions.
		num_switch_positions = 2 << static_cast<int>(switchKind);
		action_switch[i].Set_Num_Positions(num_switch_positions);
	}

	if (!Profile_Load())
		Profile_Save();

};


//_____________________
void JOYSTICK::Update() {

	if (!connected)
		return;
	if (!enabled)
		return;
	
	rawGameController.GetCurrentReading(array_view<bool>(buttonArray, num_buttons), array_view<GameControllerSwitchPosition>(switchArray, num_switches), array_view<double>(axisArray, num_axes));

	for (int i = 0; i < num_buttons; i++)
		action_button[i].SetButton(buttonArray[i]);
	for (int i = 0; i < num_switches; i++)
		action_switch[i].Switch_Position(static_cast<int>(switchArray[i]));
	for (int i = 0; i < num_axes; i++)
		action_axis[i].Set_State(axisArray[i]);
}


//__________________________________________________________________________________________________
bool JOYSTICK::Connect(winrt::Windows::Gaming::Input::RawGameController const& in_rawGameController) {

	if (!connected) {
		if (pid == in_rawGameController.HardwareProductId() && vid == in_rawGameController.HardwareVendorId() && NonRoamableId == rawGameController.NonRoamableId()) {
			rawGameController = in_rawGameController;
			Debug_Info_Joy("%S reconnected", rawGameController.DisplayName().c_str());
			//Debug_Info("%S prev NonRoamableId", NonRoamableId.c_str());
			//NonRoamableId = rawGameController.NonRoamableId();
			//Debug_Info("%S new NonRoamableId", NonRoamableId.c_str());
			connected = true;

			return true;
		}
	}
	return false;
};


//_____________________________________________________________________________________________________
bool JOYSTICK::DisConnect(winrt::Windows::Gaming::Input::RawGameController const& in_rawGameController) {

	if (rawGameController == in_rawGameController) {
		connected = false;
		Debug_Info_Joy("%S disconnected", rawGameController.DisplayName().c_str());
		return true;
	}
	return false;
};


//___________________________________________________
BOOL JOYSTICK::Profile_Load(const wchar_t* file_path) {

	FILE* fileCache = nullptr;
	DWORD version = 0;
	int i_data = 0;
	DWORD d_data = 0;

	if (_wfopen_s(&fileCache, file_path, L"rb") == 0) {
		fread(&version, sizeof(DWORD), 1, fileCache);
		/*
		fread(&d_data, sizeof(DWORD), 1, fileCache);

		wchar_t* pNonRoamableId = new wchar_t[d_data+1] {};
		fread(pNonRoamableId, sizeof(wchar_t), d_data, fileCache);
		hstring hNonRoamableId = pNonRoamableId;
		if (NonRoamableId != hNonRoamableId)
			MessageBox(nullptr, hNonRoamableId.c_str(), NonRoamableId.c_str(), 0);
		delete[]pNonRoamableId;
		*/
		if (version != JOYSTICK_PROFILE_VERSION) {
			fclose(fileCache);
			Debug_Info_Error("JOYSTICK::Profile_Load(), version mismatch, version:%d", version);
			return FALSE;
		}
		fread(&d_data, sizeof(DWORD), 1, fileCache);
		
		enabled = false;
		if (d_data & 1) 
			enabled = true;

		fread(&i_data, sizeof(i_data), 1, fileCache);
		if (i_data != num_axes) {
			fclose(fileCache);
			Debug_Info_Error("JOYSTICK::Profile_Load(), num_axes don't match:%d", version);
			return FALSE;
		}

		ACTION_AXIS* p_axis = nullptr;
		AXIS_LIMITS* p_limits = nullptr;
		ACTION_KEY* p_key = nullptr;
		ACTION_SWITCH* p_switch = nullptr;
		for (int i = 0; i < num_axes; i++) {
			p_axis = Get_Action_Axis(i);
			if (!p_axis) {
				Debug_Info_Error("JOYSTICK::Profile_Load() FAILED!: p_axis %d = null", i);
				fclose(fileCache);
				return FALSE;
			}
			p_limits = p_axis->Get_Axis_Limits();
			fread(&p_limits->min, sizeof(p_limits->min), 1, fileCache);
			fread(&p_limits->max, sizeof(p_limits->max), 1, fileCache);
			fread(&p_limits->span, sizeof(p_limits->span), 1, fileCache);
			fread(&p_limits->centre, sizeof(p_limits->centre), 1, fileCache);

			fread(&p_limits->centre_min, sizeof(p_limits->centre_min), 1, fileCache);
			fread(&p_limits->centre_max, sizeof(p_limits->centre_max), 1, fileCache);
			fread(&p_limits->centre_span, sizeof(p_limits->centre_span), 1, fileCache);


			fread(&i_data, sizeof(i_data), 1, fileCache);
			p_axis->Set_Axis_As(static_cast<AXIS_TYPE>(i_data));

			fread(&i_data, sizeof(i_data), 1, fileCache);
			p_axis->Set_Axis_Reversed(i_data);

			fread(&i_data, sizeof(i_data), 1, fileCache);
			p_axis->Set_Button_Action_Min(static_cast<WC3_ACTIONS>(i_data));

			fread(&i_data, sizeof(i_data), 1, fileCache);
			p_axis->Set_Button_Action_Max(static_cast<WC3_ACTIONS>(i_data));
		}

		fread(&i_data, sizeof(i_data), 1, fileCache);
		if (i_data != num_buttons) {
			fclose(fileCache);
			Debug_Info_Error("JOYSTICK::Profile_Load(), num_buttons don't match:%d", num_buttons);
			return FALSE;
		}
		for (int i = 0; i < num_buttons; i++) {
			p_key = Get_Action_Button(i);
			if (!p_key) {
				Debug_Info_Error("JOYSTICK::Profile_Load() FAILED!: p_key %d = null", i);
				fclose(fileCache);
				return FALSE;
			}
			fread(&i_data, sizeof(i_data), 1, fileCache);
			p_key->Set_Action(static_cast<WC3_ACTIONS>(i_data));
		}

		fread(&i_data, sizeof(i_data), 1, fileCache);
		if (i_data != num_switches) {
			fclose(fileCache);
			Debug_Info_Error("JOYSTICK::Profile_Load(), num_switches don't match:%d", num_buttons);
			return FALSE;
		}
		for (int i = 0; i < num_switches; i++) {
			p_switch = Get_Action_Switch(i);
			if (!p_switch) {
				Debug_Info_Error("JOYSTICK::Profile_Load() FAILED!: p_switch %d = null", i);
				fclose(fileCache);
				return FALSE;
			}

			int num_positions = 0;
			fread(&num_positions, sizeof(num_positions), 1, fileCache);
			for (int i = 0; i < num_positions; i++) {
				fread(&i_data, sizeof(i_data), 1, fileCache);
				p_switch->Set_Action(i, static_cast<WC3_ACTIONS>(i_data));
			}
		}

		fclose(fileCache);
	}
	else {
		Debug_Info_Error("JOYSTICK::Profile_Load(), _wfopen_s failed: %S", file_path);
		return FALSE;
	}
	return TRUE;
};


//___________________________
BOOL JOYSTICK::Profile_Load() {

	wstring path;
	if (!Get_Joystick_Config_Path(&path))
		return FALSE;

	wchar_t file_name[10 + 33 + 5]{};
	swprintf_s(file_name, 11, L"\\%04x%04x_", vid, pid);

	BYTE bHash[16];//convert the game path to hash data
	HashData((BYTE*)NonRoamableId.c_str(), NonRoamableId.size(), bHash, 16);
	wchar_t* bHashString = file_name + 10;

	for (int i = 0; i < 16; ++i)//convert the hash data to a string, this will be a unique folder name to store the config data in.
		swprintf_s(&bHashString[i * 2], 33 - i * 2, L"%02x", bHash[i]);

	swprintf_s(file_name + 10 + 32, 5, L".joy");

	path.append(file_name);
	Debug_Info_Joy("Profile_Load path: %S", path.c_str());
	return Profile_Load(path.c_str());
};


//___________________________________________________
BOOL JOYSTICK::Profile_Save(const wchar_t* file_path) {
	FILE* fileCache = nullptr;
	DWORD version = JOYSTICK_PROFILE_VERSION;
	int i_data = 10;
	DWORD dw_data = 0;

	if (_wfopen_s(&fileCache, file_path, L"wb") == 0) {
		ACTION_AXIS* p_axis = nullptr;
		AXIS_LIMITS* p_limits = nullptr;
		ACTION_KEY* p_key = nullptr;
		ACTION_SWITCH* p_switch = nullptr;

		fwrite(&version, sizeof(DWORD), 1, fileCache);
		//data = NonRoamableId.size();
		//fwrite(&data, sizeof(DWORD), 1, fileCache);
		//fwrite(NonRoamableId.c_str(), sizeof(wchar_t), NonRoamableId.size(), fileCache);
		dw_data = 0;
		if (enabled)
			dw_data |= 1;
		fwrite(&dw_data, sizeof(DWORD), 1, fileCache);

		fwrite(&num_axes, sizeof(num_axes), 1, fileCache);

		for (int i = 0; i < num_axes; i++) {
			p_axis = Get_Action_Axis(i);
			if (!p_axis) {
				Debug_Info_Error("JOYSTICK::Profile_Save() FAILED!: p_axis %d = null", i);
				fclose(fileCache);
				return FALSE;
			}
			p_limits = p_axis->Get_Axis_Limits();
			fwrite(&p_limits->min, sizeof(p_limits->min), 1, fileCache);
			fwrite(&p_limits->max, sizeof(p_limits->max), 1, fileCache);
			fwrite(&p_limits->span, sizeof(p_limits->span), 1, fileCache);
			fwrite(&p_limits->centre, sizeof(p_limits->centre), 1, fileCache);

			fwrite(&p_limits->centre_min, sizeof(p_limits->centre_min), 1, fileCache);
			fwrite(&p_limits->centre_max, sizeof(p_limits->centre_max), 1, fileCache);
			fwrite(&p_limits->centre_span, sizeof(p_limits->centre_span), 1, fileCache);

			i_data = static_cast<int>(p_axis->Get_Axis_As());
			fwrite(&i_data, sizeof(i_data), 1, fileCache);

			i_data = p_axis->Is_Axis_Reversed();
			fwrite(&i_data, sizeof(i_data), 1, fileCache);

			i_data = static_cast<int>(p_axis->Get_Button_Action_Min());
			fwrite(&i_data, sizeof(i_data), 1, fileCache);

			i_data = static_cast<int>(p_axis->Get_Button_Action_Max());
			fwrite(&i_data, sizeof(i_data), 1, fileCache);
		}

		fwrite(&num_buttons, sizeof(num_buttons), 1, fileCache);
		for (int i = 0; i < num_buttons; i++) {
			p_key = Get_Action_Button(i);
			i_data = static_cast<int>(p_key->GetAction());
			fwrite(&i_data, sizeof(i_data), 1, fileCache);
		}

		fwrite(&num_switches, sizeof(num_switches), 1, fileCache);
		for (int i = 0; i < num_switches; i++) {
			p_switch = Get_Action_Switch(i);
			if (!p_switch) {
				Debug_Info_Error("JOYSTICK::Profile_Save() FAILED!: p_switch %d = null", i);
				fclose(fileCache);
				return FALSE;
			}
			int num_positions = p_switch->Get_Num_Positions();
			fwrite(&num_positions, sizeof(num_positions), 1, fileCache);
			for (int i = 0; i < num_positions; i++) {
				i_data = static_cast<int>(p_switch->GetAction(i));
				fwrite(&i_data, sizeof(i_data), 1, fileCache);
			}
		}
		fclose(fileCache);
	}
	else {
		Debug_Info_Error("JOYSTICK::Profile_Save(), _wfopen_s failed: %S", file_path);
		return FALSE;
	}
	return TRUE;
};


//___________________________
BOOL JOYSTICK::Profile_Save() {

	wstring path;
	if (!Get_Joystick_Config_Path(&path))
		return FALSE;

	wchar_t file_name[10 + 33 + 5]{};
	swprintf_s(file_name, 11, L"\\%04x%04x_", vid, pid);

	BYTE bHash[16];//convert the game path to hash data
	HashData((BYTE*)NonRoamableId.c_str(), NonRoamableId.size(), bHash, 16);
	wchar_t* bHashString = file_name + 10;

	for (int i = 0; i < 16; ++i)//convert the hash data to a string, this will be a unique folder name to store the config data in.
		swprintf_s(&bHashString[i * 2], 33 - i * 2, L"%02x", bHash[i]);

	swprintf_s(file_name + 10 + 32, 5, L".joy");

	path.append(file_name);

	Debug_Info_Joy("Profile_Save path: %S", path.c_str());
	return Profile_Save(path.c_str());
};


//////////////////////////JOYSTICKS/////////////////////////////////

//_____________________
void JOYSTICKS::Setup() {
	if (setup)
		return;
	Set_Deadzone_Level(ConfigReadInt(L"MAIN", L"DEAD_ZONE", CONFIG_MAIN_DEAD_ZONE));

	winrt::init_apartment();
	//Debug_Info("JOYSTICKS setup");

	RawGameController::RawGameControllerAdded([this](Windows::Foundation::IInspectable const& /* sender */, RawGameController const& addedController) {
	
			concurrency::critical_section::scoped_lock s1{ controllerListLock };
			for (auto& joystick : joysticks) {
				if (joystick->Connect(addedController)) {
					Debug_Info_Joy("RawGameControllerAdded: controller reconnected");
					JoyConfig_Refresh_JoyList();
					return;
				}
			}
			JOYSTICK* joy = new JOYSTICK(addedController);
			joysticks.push_back(joy);
			Debug_Info_Joy("RawGameControllerAdded: done");
			JoyConfig_Refresh_JoyList();
		});

	RawGameController::RawGameControllerRemoved([this](winrt::Windows::Foundation::IInspectable const&, RawGameController const& removedController) {

			concurrency::critical_section::scoped_lock s2{ controllerListLock };
			for (auto& joystick : joysticks) {
				if (joystick->DisConnect(removedController)) {
					Debug_Info_Joy("RawGameControllerRemoved: Disconnected");
					JoyConfig_Refresh_JoyList();
					return;
				}
			}
			Debug_Info_Error("RawGameControllerRemoved: controller was not found in list!!!");
		});

	setup = true;
};


//______________________
void JOYSTICKS::Update() {

	Setup();

	concurrency::critical_section::scoped_lock s3{ controllerListLock };

	wc3_joy_axes.x = 0;
	wc3_joy_axes.y = 0;
	wc3_joy_axes.r = 0;
	wc3_joy_axes.t = -1.0f;

	for (auto& joysticks : joysticks)
		joysticks->Update();

	double button_val = 0.25f;
	if (wc3_joy_axes.button_mod)
		button_val *= 2;
	if (wc3_joy_axes.x_neg)
		wc3_joy_axes.x -= button_val;
	if (wc3_joy_axes.x_pos)
		wc3_joy_axes.x += button_val;

	if (wc3_joy_axes.y_neg)
		wc3_joy_axes.y -= button_val;
	if (wc3_joy_axes.y_pos)
		wc3_joy_axes.y += button_val;

	if (wc3_joy_axes.r_neg)
		wc3_joy_axes.r -= button_val;
	if (wc3_joy_axes.r_pos)
		wc3_joy_axes.r += button_val;


	if (wc3_joy_axes.x < -0.5f)
		wc3_joy_axes.x = -0.5f;
	else if (wc3_joy_axes.x > 0.5f)
		wc3_joy_axes.x = 0.5f;


	*p_wc3_joy_move_x = (LONG)(32 * wc3_joy_axes.x);
	if (*p_wc3_joy_move_x <= deadzone && *p_wc3_joy_move_x >= -deadzone)
		*p_wc3_joy_move_x = 0;


	if (wc3_joy_axes.y < -0.5f)
		wc3_joy_axes.y = -0.5f;
	else if (wc3_joy_axes.y > 0.5f)
		wc3_joy_axes.y = 0.5f;


	*p_wc3_joy_move_y = (LONG)(32 * wc3_joy_axes.y);
	if (*p_wc3_joy_move_y <= deadzone && *p_wc3_joy_move_y >= -deadzone)
		*p_wc3_joy_move_y = 0;


	if (wc3_joy_axes.r < -0.5f)
		wc3_joy_axes.r = -0.5f;
	else if (wc3_joy_axes.r > 0.5f)
		wc3_joy_axes.r = 0.5f;


	*p_wc3_joy_move_r = (LONG)(32 * wc3_joy_axes.r);
	if (*p_wc3_joy_move_r <= deadzone && *p_wc3_joy_move_r >= -deadzone)
		*p_wc3_joy_move_r = 0;


	if (wc3_joy_axes.t != -1.0f) {
		if (wc3_joy_axes.t < 0)
			wc3_joy_axes.t = 0;
		else if (wc3_joy_axes.t > 1.0f)
			wc3_joy_axes.t = 1.0f;

		*p_wc3_joy_throttle_pos = (LONG)(100 * wc3_joy_axes.t);
	}
	else
		*p_wc3_joy_throttle_pos = -1;
	//Debug_Info("Update done");
}


//____________________
BOOL JOYSTICKS::Save() {

	concurrency::critical_section::scoped_lock s7{ controllerListLock };
	BOOL all_good = TRUE;
	for (auto& joystick : joysticks) {
		if (!joystick->Profile_Save())
			all_good = FALSE;
	}

	ConfigWriteInt(L"MAIN", L"DEAD_ZONE", deadzone);

	return all_good;
}


//____________________
BOOL JOYSTICKS::Load() {

	concurrency::critical_section::scoped_lock s7{ controllerListLock };
	BOOL all_good = TRUE;
	for (auto& joystick : joysticks) {
		if (!joystick->Profile_Load())
			all_good = FALSE;
	}

	Set_Deadzone_Level(ConfigReadInt(L"MAIN", L"DEAD_ZONE", CONFIG_MAIN_DEAD_ZONE));
	return all_good;
}


//__________________________
void JOYSTICKS::Centre_All() {

	size_t num_joysticks = GetNumJoysticks();
	if (num_joysticks < 1)
		return;

	//int count = 0;
	JOYSTICK* joy = nullptr;
	int num_axes = 0;
	ACTION_AXIS* axis = nullptr;
	AXIS_TYPE type = AXIS_TYPE::None;

	for (UINT i = 0; i < num_joysticks; i++) {
		joy = GetJoy(i);
		if (joy && joy->IsEnabled()) {
			num_axes = joy->Axes(nullptr);
			for (int axis_num = 0; axis_num < num_axes; axis_num++) {
				axis = joy->Get_Action_Axis(axis_num);
				if (axis) {
					type = axis->Get_Axis_As();
					if (type == AXIS_TYPE::Pitch || type == AXIS_TYPE::Yaw || type == AXIS_TYPE::Roll) {
						axis->Centre();
						//count++;
					}
				}
			}
		}
	}
	//Debug_Info("Centre_All_Assiged_Axes: %d axes centred", count);

}


//______________________
static void Joy_Update() {

	Joysticks.Update();
}


//_________________________________________________
static void __declspec(naked) joy_update_main(void) {

	__asm {
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

/*
//___________________________
void Print_Scancode(int code) {
	Debug_Info("Key Scancode: %x", code);

}
//0048280C | .  8A9A 80214B00                MOV BL, BYTE PTR DS : [EDX + BYTE key_states[136]]
DWORD pp_key_states = 0x4B2180;
//___________________________________________________________________
static void __declspec(naked) print_scancode(void) {
	//insert the joystick message check function along with the main message check function.
	__asm {
		pushad
		push edx
		call Print_Scancode
		add esp, 0x4
		popad
		mov ebx, pp_key_states
		mov bl, byte ptr ds : [edx + ebx]
		ret
	}
}
*/

//___________________________
void Modifications_Joystick() {

	MemWrite8(0x407F90, 0x8B, 0xE9);
	FuncWrite32(0x407F91, 0x53042444, (DWORD)&joy_setup);

	MemWrite8(0x482A90, 0x83, 0xE9);
	FuncWrite32(0x482A91, 0xC93334EC, (DWORD)&joy_update_main);

	MemWrite8(0x482BA0, 0x83, 0xE9);
	FuncWrite32(0x482BA1, 0x05C634EC, (DWORD)&joy_update_buttons);

	//Update_Joystick_Movement function. Calls to this function are not needed, joystick movement is updated in "joy_update_main".
	MemWrite8(0x482DF0, 0x56, 0xC3);



	// get throttle value fixes-------------
	//skip over JOYCAPS.wCaps & JOYCAPS_HASZ
	MemWrite16(0x42932A, 0x05F6, 0x07EB);

	//skip over JOYCAPS.wZmax - JOYCAPS.wZmin
	MemWrite16(0x42933C, 0x0D8B, 0x0CEB);

	//skip over other maniputations
	MemWrite16(0x42934F, 0x052B, 0x11EB);
	//--------------------------------------
	

	//make the roll axis variable -------------
	// 
	//put the former dwRpos_sign value in edi this now contains the roll axis offset
	//8B3D D0244B00                MOV EDI, DWORD PTR DS : [dwRpos_sign]
	MemWrite8(0x429BF6, 0x83, 0x8B);
	MemWrite8(0x429BFC, 0x00, 0x90);
	//nop jmp
	MemWrite16(0x429BFD, 0x0D7D, 0x9090);

	//put the roll axis value into the player movement structure.
	//8978 14                      MOV DWORD PTR DS : [EAX + 14] , EDI; player roll offset
	MemWrite16(0x429C05, 0x40C7, 0x7889);
	MemWrite32(0x429C08, 0xFFFFFFF0, 0x90909090);
	//jmp this bit
	MemWrite8(0x429C25, 0x7E, 0xEB);
	//----------------------------------------
	

	//print scancodes
	//MemWrite16(0x48280C, 0x9A8A, 0xE890);
	//FuncWrite32(0x48280E, 0x4B2180, (DWORD)&print_scancode);

}
