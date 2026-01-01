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

#include "resource.h"
#include "joystick_config.h"
#include "joystick.h"
#include "wc3w.h"
#include "modifications.h"
#include "configTools.h"

using namespace winrt;
using namespace Windows::Gaming::Input;

HWND hWin_SaveAsPreset = nullptr;
HWND hWin_Config_Joy = nullptr;
HWND hWin_Config_Control = nullptr;
HWND hWin_Config_Mouse = nullptr;

BOOL joyList_Updated = 0;

HWND hWin_AxisCalibrate = nullptr;

#define GEN_TEXT_BUFF_COUNT	64
wchar_t general_string_buff[GEN_TEXT_BUFF_COUNT]{0};
wchar_t general_string_buff2[GEN_TEXT_BUFF_COUNT]{ 0 };

int current_JoySelected = -1;

int current_num_axes = 0;
double* current_axisArray = nullptr;

int current_num_buttons = 0;
bool* current_buttonArray = nullptr;

int current_num_switches = 0;
GameControllerSwitchPosition* current_switchArray;

const UINT WC3_ACTION_UID[] {
	IDS_NONE,
	IDS_ACTION001,
	IDS_ACTION002,
	IDS_ACTION003,
	IDS_ACTION004,
	IDS_ACTION005,
	IDS_ACTION006,
	IDS_ACTION007,
	IDS_ACTION008,
	IDS_ACTION009,
	IDS_ACTION010,
	IDS_ACTION011,
	IDS_ACTION012,
	IDS_ACTION013,
	IDS_ACTION014,
	IDS_ACTION015,
	IDS_ACTION016,
	IDS_ACTION017,
	IDS_ACTION018,
	IDS_ACTION019,
	IDS_ACTION020,
	IDS_ACTION021,
	IDS_ACTION022,
	IDS_ACTION023,
	IDS_ACTION024,
	IDS_ACTION025,
	IDS_ACTION026,
	IDS_ACTION027,
	IDS_ACTION028,
	IDS_ACTION029,
	IDS_ACTION030,
	IDS_ACTION031,
	IDS_ACTION032,
	IDS_ACTION033,
	IDS_ACTION034,
	IDS_ACTION035,
	IDS_ACTION036,
	IDS_ACTION037,
	IDS_ACTION038,
	IDS_ACTION039,
	IDS_ACTION040,
	IDS_ACTION041,
	IDS_ACTION042,
	IDS_ACTION043,
	IDS_ACTION044,
	IDS_ACTION045,
	IDS_ACTION046,
	IDS_ACTION047,
	IDS_ACTION048,
	IDS_ACTION049,
	IDS_ACTION050,
	IDS_ACTION051,
	IDS_ACTION052,
	IDS_ACTION053,
	IDS_ACTION054,
	IDS_ACTION055,
	IDS_ACTION056,
	IDS_ACTION057,
	IDS_ACTION058,
	IDS_ACTION059,
	IDS_ACTION060,
	IDS_ACTION061,
	IDS_ACTION062,
	IDS_ACTION063,
};

const UINT AXIS_TYPE_UID[]{
	IDS_NONE,
	IDS_AXIS_TYPE001,
	IDS_AXIS_TYPE002,
	IDS_AXIS_TYPE003,
	IDS_AXIS_TYPE004,
	IDS_AXIS_TYPE005,
	IDS_AXIS_TYPE006,
	IDS_AXIS_TYPE007,
	IDS_AXIS_TYPE008,
	IDS_AXIS_TYPE009,
	IDS_AXIS_TYPE010,
	IDS_AXIS_TYPE011,
	IDS_AXIS_TYPE012,

};

//
const UINT SWITCH_POS_UID[]{
	IDS_SWITCH_POS000,
	IDS_SWITCH_POS001,
	IDS_SWITCH_POS002,
	IDS_SWITCH_POS003,
	IDS_SWITCH_POS004,
	IDS_SWITCH_POS005,
	IDS_SWITCH_POS006,
	IDS_SWITCH_POS007,
	IDS_SWITCH_POS008,
};


//_____________________________________________________________________
BOOL JoyConfig_Refresh_CurrentAction(WC3_ACTIONS action, BOOL activate) {

	if (!hWin_Config_Joy)
		return FALSE;
	HWND hwnd_sub = GetDlgItem(hWin_Config_Joy, IDC_STATIC_CURRENT_ACTION);
	if (!hwnd_sub)
		return FALSE;
	
	UINT UID = WC3_ACTION_UID[static_cast<int>(WC3_ACTIONS::None)];
	if (activate)
		UID = WC3_ACTION_UID[static_cast<int>(action)];
	LoadString(phinstDLL, UID, general_string_buff, _countof(general_string_buff));
	SendMessage(hwnd_sub, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)general_string_buff);

	switch (action) {
	case WC3_ACTIONS::Pitch_Down:
		wc3_joy_axes.y_neg = activate;
		break;
	case WC3_ACTIONS::Pitch_Up:
		wc3_joy_axes.y_pos = activate;
		break;
	case WC3_ACTIONS::Yaw_Left:
		wc3_joy_axes.x_neg = activate;
		break;
	case WC3_ACTIONS::Yaw_Right:
		wc3_joy_axes.x_pos = activate;
		break;

	case WC3_ACTIONS::Pitch_Down_Yaw_Left:
		wc3_joy_axes.y_neg = activate;
		wc3_joy_axes.x_neg = activate;
		break;
	case WC3_ACTIONS::Pitch_Down_Yaw_Right:
		wc3_joy_axes.y_neg = activate;
		wc3_joy_axes.x_pos = activate;
		break;
	case WC3_ACTIONS::Pitch_Up_Yaw_Left:
		wc3_joy_axes.y_pos = activate;
		wc3_joy_axes.x_neg = activate;
		break;
	case WC3_ACTIONS::Pitch_Up_Yaw_Right:
		wc3_joy_axes.y_pos = activate;
		wc3_joy_axes.x_pos = activate;
		break;

	case WC3_ACTIONS::Roll_Left:
		wc3_joy_axes.r_neg = activate;
		break;
	case  WC3_ACTIONS::Roll_Right:
		wc3_joy_axes.r_pos = activate;
		break;
	case WC3_ACTIONS::Double_Yaw_Pitch_Roll_Rates:
		wc3_joy_axes.button_mod = activate;
		break;
	default:
		break;
	}
	return TRUE;
}


//________________________________________________________
static void JoyConfig_Refresh_Button_Display(HWND hwndDlg) {

	static bool pressed = false;
	static int button = 0;

	HWND hwnd_joy = GetDlgItem(hwndDlg, IDC_COMBO_JOY_SELECT);
	if (!hwnd_joy)
		return;
	HWND hwnd_button = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_BUTTONS);
	if (!hwnd_button)
		return;
	HWND hwnd_button_state = GetDlgItem(hwndDlg, IDC_STATIC_BUTTON_STATE);
	if (!hwnd_button_state)
		return;
	int joy_selected = (int)(SendMessage(hwnd_joy, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	JOYSTICK* p_joy_selected = Joysticks.GetJoy(joy_selected);
	if (!p_joy_selected)
		return;
	int button_selected = (int)(SendMessage(hwnd_button, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));

	ACTION_KEY* p_action_button = p_joy_selected->Get_Action_Button(button_selected);
	if (!p_action_button)
		return;

	if (button == button_selected && pressed == p_action_button->Is_Pressed())
		return;
	button = button_selected;
	pressed = p_action_button->Is_Pressed();
	
	int id = IDS_UNPRESSED;
	if (pressed)
		id = IDS_PRESSED;
	LoadString(phinstDLL, id, general_string_buff, _countof(general_string_buff));
	SendMessage(hwnd_button_state, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)general_string_buff);
}


//___________________________________________________________________
static void JoyConfig_Refresh_Buttons(HWND hwndDlg, BOOL joy_changed) {

	HWND hwnd_joy = GetDlgItem(hwndDlg, IDC_COMBO_JOY_SELECT);
	if (!hwnd_joy)
		return;
	HWND hwnd_button = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_BUTTONS);
	if (!hwnd_button)
		return;
	HWND hwnd_action = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_BUTTON_ACTION);
	if (!hwnd_action)
		return;
	HWND hwnd_button_state = GetDlgItem(hwndDlg, IDC_STATIC_BUTTON_STATE);
	if (!hwnd_button_state)
		return;

	int joy_selected = (int)(SendMessage(hwnd_joy, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	JOYSTICK* p_joy_selected = Joysticks.GetJoy(joy_selected);
	if (!p_joy_selected)
		return;

	if (joy_changed) {// setup button list if joystick has changed
		LoadString(phinstDLL, IDS_UNPRESSED, general_string_buff, _countof(general_string_buff));
		SendMessage(hwnd_button_state, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)general_string_buff);

		SendMessage(hwnd_button, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
		if (p_joy_selected->Buttons(nullptr) > 0) {
			EnableWindow(hwnd_button, TRUE);
			EnableWindow(hwnd_action, TRUE);
			wchar_t* msg = new wchar_t[12];
			LoadString(phinstDLL, IDS_BUTTON, general_string_buff, _countof(general_string_buff));
			for (int i = 0; i < p_joy_selected->Buttons(nullptr); i++) {
				swprintf_s(msg, 12, L"%s %d", general_string_buff, i);
				SendMessage(hwnd_button, CB_ADDSTRING, (WPARAM)0, (LPARAM)msg);
			}
			delete[] msg;
		}
		else {
			LoadString(phinstDLL, IDS_NONE, general_string_buff, _countof(general_string_buff));
			SendMessage(hwnd_button, CB_ADDSTRING, (WPARAM)0, (LPARAM)general_string_buff);
			EnableWindow(hwnd_button, FALSE);
			SendMessage(hwnd_action, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
			EnableWindow(hwnd_action, FALSE);
		}
		SendMessage(hwnd_button, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
	}

	int button_selected = (int)(SendMessage(hwnd_button, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	ACTION_KEY* p_action_button = p_joy_selected->Get_Action_Button(button_selected);
	if (p_action_button) {
		WC3_ACTIONS action = p_action_button->GetAction();
		SendMessage(hwnd_action, CB_SETCURSEL, (WPARAM)action, (LPARAM)0);
		return;
	}
}


//__________________________________________________
static void JoyConfig_Button_SetButton(HWND hwndDlg) {

	HWND hwnd_joy = GetDlgItem(hwndDlg, IDC_COMBO_JOY_SELECT);
	if (!hwnd_joy)
		return;
	HWND hwnd_selected_button = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_BUTTONS);
	if (!hwnd_selected_button)
		return;
	HWND hwnd_action = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_BUTTON_ACTION);
	if (!hwnd_action)
		return;

	int joy_selected = (int)(SendMessage(hwnd_joy, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	JOYSTICK* p_joy_selected = Joysticks.GetJoy(joy_selected);
	if (!p_joy_selected)
		return;

	int button_selected = (int)(SendMessage(hwnd_selected_button, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	ACTION_KEY* p_action_button = p_joy_selected->Get_Action_Button(button_selected);
	if (!p_action_button)
		return;
	WC3_ACTIONS action = (WC3_ACTIONS)(SendMessage(hwnd_action, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	p_action_button->Set_Action(action);

}


//________________________________________________________
static void JoyConfig_Refresh_Switch_Display(HWND hwndDlg) {

	static int current_position = false;
	static int current_switch = 0;

	HWND hwnd_joy = GetDlgItem(hwndDlg, IDC_COMBO_JOY_SELECT);
	if (!hwnd_joy)
		return;
	HWND hwnd_switch = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_SWITCHES);
	if (!hwnd_switch)
		return;
	HWND hwnd_button_state = GetDlgItem(hwndDlg, IDC_STATIC_SWITCH_STATE);
	if (!hwnd_button_state)
		return;
	int joy_selected = (int)(SendMessage(hwnd_joy, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	JOYSTICK* p_joy_selected = Joysticks.GetJoy(joy_selected);
	if (!p_joy_selected)
		return;

	int switch_selected = (int)(SendMessage(hwnd_switch, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	ACTION_SWITCH* p_action_switch = p_joy_selected->Get_Action_Switch(switch_selected);
	if (!p_action_switch)
		return;

	if (current_switch == switch_selected && current_position == p_action_switch->Get_Current_Position())
		return;
	current_switch = switch_selected;
	current_position = p_action_switch->Get_Current_Position();

	
	LoadString(phinstDLL, SWITCH_POS_UID[current_position], general_string_buff, _countof(general_string_buff));
	SendMessage(hwnd_button_state, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)general_string_buff);

}


//_________________________________________________________________________________________
static void JoyConfig_Refresh_Switches(HWND hwndDlg, BOOL joy_changed, BOOL switch_changed) {

	HWND hwnd_joy = GetDlgItem(hwndDlg, IDC_COMBO_JOY_SELECT);
	if (!hwnd_joy)
		return;
	HWND hwnd_switch = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_SWITCHES);
	if (!hwnd_switch)
		return;
	HWND hwnd_pos = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_SWITCH_POS);
	if (!hwnd_pos)
		return;
	HWND hwnd_action = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_SWITCH_ACTION);
	if (!hwnd_action)
		return;
	HWND hwnd_button_state = GetDlgItem(hwndDlg, IDC_STATIC_SWITCH_STATE);
	if (!hwnd_button_state)
		return;

	int joy_selected = (int)(SendMessage(hwnd_joy, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	JOYSTICK* p_joy_selected = Joysticks.GetJoy(joy_selected);
	if (!p_joy_selected)
		return;

	if (joy_changed) {
		LoadString(phinstDLL, SWITCH_POS_UID[0], general_string_buff, _countof(general_string_buff));
		SendMessage(hwnd_button_state, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)general_string_buff);
		// setup switches list if joystick has changed
		SendMessage(hwnd_switch, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
		if (p_joy_selected->Switches(nullptr) > 0) {
			EnableWindow(hwnd_switch, TRUE);
			wchar_t* msg = new wchar_t[12];
			LoadString(phinstDLL, IDS_SWITCH, general_string_buff, _countof(general_string_buff));
			for (int i = 0; i < p_joy_selected->Switches(nullptr); i++) {
				swprintf_s(msg, 12, L"%s %d", general_string_buff, i);
				SendMessage(hwnd_switch, CB_ADDSTRING, (WPARAM)0, (LPARAM)msg);
			}
			delete[] msg;
		}
		else {
			LoadString(phinstDLL, IDS_NONE, general_string_buff, _countof(general_string_buff));
			SendMessage(hwnd_switch, CB_ADDSTRING, (WPARAM)0, (LPARAM)general_string_buff);
			EnableWindow(hwnd_switch, FALSE);
		}
		SendMessage(hwnd_switch, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

	}
	if (joy_changed || switch_changed) {
		// setup switch positions list if joystick has changed
		SendMessage(hwnd_pos, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
		int switch_selected = (int)(SendMessage(hwnd_switch, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
		ACTION_SWITCH* p_action_switch = p_joy_selected->Get_Action_Switch(switch_selected);
		if (p_action_switch) {
			EnableWindow(hwnd_action, TRUE);
			EnableWindow(hwnd_pos, TRUE);
			int num_positions = p_action_switch->Get_Num_Positions();
			int step = 8 / (num_positions - 1);
			for (int i = 1; i < 9; i += step) {
				LoadString(phinstDLL, SWITCH_POS_UID[i], general_string_buff, _countof(general_string_buff));
				SendMessage(hwnd_pos, CB_ADDSTRING, (WPARAM)0, (LPARAM)general_string_buff);
				//SendMessage(hwnd_pos, CB_ADDSTRING, (WPARAM)0, (LPARAM)SWITCH_POSITION_TEXT[i]);
			}
			SendMessage(hwnd_pos, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
		}
		else {
			LoadString(phinstDLL, IDS_NONE, general_string_buff, _countof(general_string_buff));
			SendMessage(hwnd_pos, CB_ADDSTRING, (WPARAM)0, (LPARAM)general_string_buff);
			SendMessage(hwnd_pos, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
			EnableWindow(hwnd_pos, FALSE);
			SendMessage(hwnd_action, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
			EnableWindow(hwnd_action, FALSE);
		}
	}

	int switch_selected = (int)(SendMessage(hwnd_switch, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	ACTION_SWITCH* p_action_switch = p_joy_selected->Get_Action_Switch(switch_selected);
	if (p_action_switch) {
		int switch_position = (int)(SendMessage(hwnd_pos, CB_GETCURSEL, (WPARAM)0, (LPARAM)0)) + 1;

		WC3_ACTIONS action = p_action_switch->GetAction(switch_position);
		SendMessage(hwnd_action, CB_SETCURSEL, (WPARAM)action, (LPARAM)0);
		return;
	}
}


//__________________________________________________
static void JoyConfig_Switch_SetButton(HWND hwndDlg) {

	HWND hwnd_joy = GetDlgItem(hwndDlg, IDC_COMBO_JOY_SELECT);
	if (!hwnd_joy)
		return;
	HWND hwnd_selected_switch = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_SWITCHES);
	if (!hwnd_selected_switch)
		return;
	HWND hwnd_pos = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_SWITCH_POS);
	if (!hwnd_pos)
		return;
	HWND hwnd_action = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_SWITCH_ACTION);
	if (!hwnd_action)
		return;

	int joy_selected = (int)(SendMessage(hwnd_joy, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	JOYSTICK* p_joy_selected = Joysticks.GetJoy(joy_selected);
	if (!p_joy_selected)
		return;

	int switch_selected = (int)(SendMessage(hwnd_selected_switch, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	int switch_position = (int)(SendMessage(hwnd_pos, CB_GETCURSEL, (WPARAM)0, (LPARAM)0)) + 1;
	ACTION_SWITCH* p_action_switch = p_joy_selected->Get_Action_Switch(switch_selected);
	if (!p_action_switch)
		return;
	WC3_ACTIONS action = (WC3_ACTIONS)(SendMessage(hwnd_action, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	p_action_switch->Set_Action(switch_position, action);
}


//______________________________________________________
static void JoyConfig_Refresh_Axis_Display(HWND hwndDlg) {

	HWND hwnd_joy = GetDlgItem(hwndDlg, IDC_COMBO_JOY_SELECT);
	if (!hwnd_joy)
		return;
	HWND hwnd_axis = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_AXIS);
	if (!hwnd_axis)
		return;

	int joy_selected = (int)(SendMessage(hwnd_joy, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	JOYSTICK* p_joy_selected = Joysticks.GetJoy(joy_selected);
	if (!p_joy_selected)
		return;
	int axis_selected = (int)(SendMessage(hwnd_axis, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	ACTION_AXIS* p_action_axis = p_joy_selected->Get_Action_Axis(axis_selected);
	if (!p_action_axis)
		return;
	double val = p_action_axis->Get_Current_Val();

	HWND hwnd_sub = GetDlgItem(hwndDlg, IDC_STATIC_AXIS_BOX);
	RECT rc{};
	GetWindowRect(hwnd_sub, &rc); //get window rect of control relative to screen
	POINT pt = { rc.left, rc.top }; //new point object using rect x, y
	ScreenToClient(hwndDlg, &pt); //convert screen co-ords to client based points

	int width = rc.right - rc.left - 2 - 3;

	int i_val = (int)(val * width);
	hwnd_sub = GetDlgItem(hwndDlg, IDC_STATIC_AXIS_BAR);
	MoveWindow(hwnd_sub, pt.x + 1 + i_val, pt.y + 1, 3, 9, TRUE);
}


//_______________________________________________________________________________________
static void JoyConfig_Refresh_Axes(HWND hwndDlg, BOOL joy_changed, BOOL axis_type_change) {

	HWND hwnd_joy = GetDlgItem(hwndDlg, IDC_COMBO_JOY_SELECT);
	if (!hwnd_joy)
		return;
	HWND hwnd_axis = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_AXIS);
	if (!hwnd_axis)
		return;
	HWND hwnd_type = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_AXIS_TYPE);
	if (!hwnd_type)
		return;
	HWND hwnd_sign = GetDlgItem(hwndDlg, IDC_CHECK_SELECTED_AXIS_SIGN);
	if (!hwnd_sign)
		return;
	HWND hwnd_calibrate = GetDlgItem(hwndDlg, IDC_BUTTON_CALIBRATE_AXIS);
	if (!hwnd_calibrate)
		return;
	HWND hwnd_centre = GetDlgItem(hwndDlg, IDC_BUTTON_CENTRE_AXIS);
	if (!hwnd_centre)
		return;
	HWND hwnd_butt1 = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_AXIS_BUTTON1);
	if (!hwnd_butt1)
		return;
	HWND hwnd_butt2 = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_AXIS_BUTTON2);
	if (!hwnd_butt2)
		return;
	int joy_selected = (int)(SendMessage(hwnd_joy, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	JOYSTICK* p_joy_selected = Joysticks.GetJoy(joy_selected);
	if (!p_joy_selected)
		return;

	if (joy_changed) {// setup axes list if joystick has changed
		SendMessage(hwnd_axis, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
		if (p_joy_selected->Axes(nullptr) > 0) {
			EnableWindow(hwnd_axis, TRUE);
			wchar_t* msg = new wchar_t[12];
			LoadString(phinstDLL, IDS_AXIS, general_string_buff, _countof(general_string_buff));
			for (int i = 0; i < p_joy_selected->Axes(nullptr); i++) {
				swprintf_s(msg, 12, L"%s %d", general_string_buff, i);
				SendMessage(hwnd_axis, CB_ADDSTRING, (WPARAM)0, (LPARAM)msg);
			}
			delete[] msg;
		}
		else {
			LoadString(phinstDLL, IDS_NONE, general_string_buff, _countof(general_string_buff));
			SendMessage(hwnd_axis, CB_ADDSTRING, (WPARAM)0, (LPARAM)general_string_buff);
			EnableWindow(hwnd_axis, FALSE);
		}
		SendMessage(hwnd_axis, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
	}
	if (joy_changed || axis_type_change) {
		int axis_selected = (int)(SendMessage(hwnd_axis, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
		ACTION_AXIS* p_action_axis = p_joy_selected->Get_Action_Axis(axis_selected);
		if (p_action_axis) {
			EnableWindow(hwnd_calibrate, TRUE);
			EnableWindow(hwnd_centre, TRUE);
			EnableWindow(hwnd_sign, TRUE);

			EnableWindow(hwnd_type, TRUE);
			AXIS_TYPE axis_type = p_action_axis->Get_Axis_As();
			SendMessage(hwnd_type, CB_SETCURSEL, (WPARAM)axis_type, (LPARAM)0);


			DWORD checked = BST_UNCHECKED;
			if (p_action_axis->Is_Axis_Reversed() == TRUE)
				checked = BST_CHECKED;
			SendMessage(hwnd_sign, BM_SETCHECK, (WPARAM)checked, (LPARAM)0);

			BOOL is_button1 = FALSE;
			BOOL is_button2 = FALSE;
			if (axis_type == AXIS_TYPE::AsOneButton)
				is_button2 = TRUE;
			else if (axis_type == AXIS_TYPE::AsTwoButtons) {
				is_button1 = TRUE;
				is_button2 = TRUE;
			}

			SendMessage(hwnd_butt1, CB_SETCURSEL, (WPARAM)p_action_axis->Get_Button_Action_Min(), (LPARAM)0);
			EnableWindow(hwnd_butt1, is_button1);

			SendMessage(hwnd_butt2, CB_SETCURSEL, (WPARAM)p_action_axis->Get_Button_Action_Max(), (LPARAM)0);
			EnableWindow(hwnd_butt2, is_button2);

		}
		else {
			SendMessage(hwnd_type, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
			EnableWindow(hwnd_type, FALSE);
			SendMessage(hwnd_sign, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
			EnableWindow(hwnd_sign, FALSE);
			SendMessage(hwnd_butt1, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
			EnableWindow(hwnd_butt1, FALSE);
			SendMessage(hwnd_butt2, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
			EnableWindow(hwnd_butt2, FALSE);
			EnableWindow(hwnd_calibrate, FALSE);
			EnableWindow(hwnd_centre, FALSE);
		}
	}
}


//______________________________________________
static void JoyConfig_Axis_SetType(HWND hwndDlg) {

	HWND hwnd_joy = GetDlgItem(hwndDlg, IDC_COMBO_JOY_SELECT);
	if (!hwnd_joy)
		return;
	HWND hwnd_axis = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_AXIS);
	if (!hwnd_axis)
		return;
	HWND hwnd_type = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_AXIS_TYPE);
	if (!hwnd_type)
		return;

	int joy_selected = (int)(SendMessage(hwnd_joy, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	JOYSTICK* p_joy_selected = Joysticks.GetJoy(joy_selected);
	if (!p_joy_selected)
		return;

	int axis_selected = (int)(SendMessage(hwnd_axis, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	ACTION_AXIS* p_action_axis = p_joy_selected->Get_Action_Axis(axis_selected);
	if (!p_action_axis)
		return;
	int axis_type = (int)(SendMessage(hwnd_type, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	p_action_axis->Set_Axis_As((AXIS_TYPE)axis_type);
	JoyConfig_Refresh_Axes(hwndDlg, FALSE, TRUE);
}


//______________________________________________
static void JoyConfig_Axis_SetSign(HWND hwndDlg) {

	HWND hwnd_joy = GetDlgItem(hwndDlg, IDC_COMBO_JOY_SELECT);
	if (!hwnd_joy)
		return;
	HWND hwnd_axis = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_AXIS);
	if (!hwnd_axis)
		return;
	HWND hwnd_sign = GetDlgItem(hwndDlg, IDC_CHECK_SELECTED_AXIS_SIGN);
	if (!hwnd_sign)
		return;

	int joy_selected = (int)(SendMessage(hwnd_joy, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	JOYSTICK* p_joy_selected = Joysticks.GetJoy(joy_selected);
	if (!p_joy_selected)
		return;

	int axis_selected = (int)(SendMessage(hwnd_axis, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	ACTION_AXIS* p_action_axis = p_joy_selected->Get_Action_Axis(axis_selected);
	if (!p_action_axis)
		return;

	DWORD button_state = (int)(SendMessage(hwnd_sign, BM_GETCHECK, (WPARAM)0, (LPARAM)0));
	BOOL is_rev_axis = FALSE;
	if (button_state & BST_CHECKED)
		is_rev_axis = TRUE;

	p_action_axis->Set_Axis_Reversed(is_rev_axis);
}


//__________________________________________________________________
static void JoyConfig_Axis_SetButton(HWND hwndDlg, DWORD IDC_BUTTON) {

	HWND hwnd_joy = GetDlgItem(hwndDlg, IDC_COMBO_JOY_SELECT);
	if (!hwnd_joy)
		return;
	HWND hwnd_axis = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_AXIS);
	if (!hwnd_axis)
		return;
	HWND hwnd_button = GetDlgItem(hwndDlg, IDC_BUTTON);
	if (!hwnd_button)
		return;

	int joy_selected = (int)(SendMessage(hwnd_joy, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	JOYSTICK* p_joy_selected = Joysticks.GetJoy(joy_selected);
	if (!p_joy_selected)
		return;

	int axis_selected = (int)(SendMessage(hwnd_axis, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	ACTION_AXIS* p_action_axis = p_joy_selected->Get_Action_Axis(axis_selected);
	if (!p_action_axis)
		return;
	WC3_ACTIONS action = (WC3_ACTIONS)(SendMessage(hwnd_button, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	if (IDC_BUTTON == IDC_COMBO_SELECT_AXIS_BUTTON1)
		p_action_axis->Set_Button_Action_Min(action);
	else if (IDC_BUTTON == IDC_COMBO_SELECT_AXIS_BUTTON2)
		p_action_axis->Set_Button_Action_Max(action);
}


//_________________________________________________
static void JoyConfig_Refresh_Enabled(HWND hwndDlg) {

	HWND hwnd_joy = GetDlgItem(hwndDlg, IDC_COMBO_JOY_SELECT);
	if (!hwnd_joy)
		return;
	HWND hwnd_enabled = GetDlgItem(hwndDlg, IDC_CHECK_JOY_ENABLE);
	if (!hwnd_enabled)
		return;

	int joy_selected = (int)(SendMessage(hwnd_joy, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	JOYSTICK* p_joy_selected = Joysticks.GetJoy(joy_selected);
	if (!p_joy_selected)
		return;
	DWORD checked = BST_UNCHECKED;
	if (p_joy_selected->IsEnabled())
		checked = BST_CHECKED;
	//Debug_Info("JoyConfig_Refresh_Enabled, checked:%d, is_checked:%d", checked);

	SendMessage(hwnd_enabled, BM_SETCHECK, (WPARAM)checked, (LPARAM)0);
}


//________________________________________________
static void JoyConfig_Update_Enabled(HWND hwndDlg) {

	HWND hwnd_joy = GetDlgItem(hwndDlg, IDC_COMBO_JOY_SELECT);
	if (!hwnd_joy)
		return;
	HWND hwnd_enabled = GetDlgItem(hwndDlg, IDC_CHECK_JOY_ENABLE);
	if (!hwnd_enabled)
		return;

	int joy_selected = (int)(SendMessage(hwnd_joy, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	JOYSTICK* p_joy_selected = Joysticks.GetJoy(joy_selected);
	if (!p_joy_selected)
		return;

	DWORD checked = (int)(SendMessage(hwnd_enabled, BM_GETCHECK, (WPARAM)0, (LPARAM)0));
	bool is_checked = false;
	if (checked & BST_CHECKED)
		is_checked = true;
	//Debug_Info("JoyConfig_Update_Enabled, checked:%d, is_checked:%d", checked, is_checked);
	p_joy_selected->Enable(is_checked);
}


//____________________________________________
static bool JoyConfig_Preset_Set(HWND hwndDlg) {

	HWND hwnd_joy = GetDlgItem(hwndDlg, IDC_COMBO_JOY_SELECT);
	if (!hwnd_joy)
		return false;
	HWND hwnd_presets = GetDlgItem(hwndDlg, IDC_COMBO_JOY_PRESETS);
	if (!hwnd_presets)
		return false;
	int preset_selected = (int)(SendMessage(hwnd_presets, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	if (preset_selected == 0)
		return false;

	int joy_selected = (int)(SendMessage(hwnd_joy, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	JOYSTICK* p_joy_selected = Joysticks.GetJoy(joy_selected);
	if (!p_joy_selected)
		return false;

	std::wstring file_path;
	if (!Get_Joystick_Config_Path(&file_path))
		return false;

	file_path.append(L"\\presets\\");

	wchar_t vid_pid_name[10]{ 0 };
	swprintf_s(vid_pid_name, L"%04x%04x_", p_joy_selected->Get_VID(), p_joy_selected->Get_PID());

	file_path.append(vid_pid_name);
	
	SendMessage(hwnd_presets, CB_GETLBTEXT, (WPARAM)preset_selected, (LPARAM)general_string_buff);
	file_path.append(general_string_buff);
	file_path.append(L".joy");

	p_joy_selected->Profile_Load(file_path.c_str());

	SendMessage(hwnd_presets, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
	return true;
}


//____________________________________________________________________________________________________
static INT_PTR CALLBACK DialogProc_SaveAsPreset(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	static wchar_t* text_current = nullptr;
	static wchar_t* text_last_good = nullptr;
	static JOYSTICK* p_joy_selected = nullptr;

	switch (uMsg) {
	case WM_INITDIALOG: {
		////101 is the wcIII icon
		SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(*pp_hinstWC3W, MAKEINTRESOURCE(101)));

		HWND hwndParent = GetParent(hwndDlg);

		//set position to centre of parent window.
		RECT rc_Win{ 0,0,0,0 };
		GetWindowRect(hwndDlg, &rc_Win);
		RECT rcParent{ 0,0,0,0 };
		GetWindowRect(hwndParent, &rcParent);
		SetWindowPos(hwndDlg, nullptr, rcParent.left + ((rcParent.right - rcParent.left) - (rc_Win.right - rc_Win.left)) / 2, rc_Win.top + ((rcParent.bottom - rcParent.top) - (rc_Win.bottom - rc_Win.top)) / 2, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

		HWND hwnd_edit = GetDlgItem(hwndDlg, IDC_EDIT_PRESET_NAME);
		SendMessage(hwnd_edit, EM_SETLIMITTEXT, (WPARAM)GEN_TEXT_BUFF_COUNT, (LPARAM)0);
		text_current = new wchar_t[GEN_TEXT_BUFF_COUNT] {0};
		text_last_good = new wchar_t[GEN_TEXT_BUFF_COUNT] {0};

		p_joy_selected = (JOYSTICK*)lParam;

		SetFocus(hwnd_edit);
		return FALSE;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_EDIT_PRESET_NAME:
			switch (HIWORD(wParam)) {
			case EN_UPDATE: {
				wchar_t editExcludeChars[] = L"\\/:*?<>|";
				HWND hwnd_edit = GetDlgItem(hwndDlg, IDC_EDIT_PRESET_NAME);
				
				bool error = false;
				int text_length = SendMessage(hwnd_edit, WM_GETTEXT, (WPARAM)GEN_TEXT_BUFF_COUNT, (LPARAM)text_current);

				for (int indx = 0; indx < text_length; indx++) {
					wchar_t nChar = text_current[indx];
					for (int compare = 0; compare < _countof(editExcludeChars); compare++) {
						if (nChar == editExcludeChars[compare]) {
							error = true;
							continue;
						}
						if (error)
							continue;
					}
				}
				if (error) {
					DWORD start = 0 , end = 0;
					//get current char selection
					SendMessage(hwnd_edit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
					//Restore the last good text that was entered 
					SendMessage(hwnd_edit, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)text_last_good);
					//restore char selection
					SendMessage(hwnd_edit, EM_SETSEL, (WPARAM)start-1, (LPARAM)end-1);
					//beep
					MessageBeep(MB_OK);
				}
				else
					wcsncpy_s(text_last_good, GEN_TEXT_BUFF_COUNT, text_current, GEN_TEXT_BUFF_COUNT);

				return TRUE;
			}
			}
			break;
		case IDOK: {
			
			std::wstring file_path;
			if (!Get_Joystick_Config_Path(&file_path)) {
				DestroyWindow(hwndDlg);
				return TRUE;
			}

			file_path.append(L"\\presets");
			if (GetFileAttributes(file_path.c_str()) == INVALID_FILE_ATTRIBUTES) {
				if (!CreateDirectory(file_path.c_str(), nullptr)) {
					DestroyWindow(hwndDlg);
					return TRUE;
				}
			}
			file_path.append(L"\\");

			wchar_t vid_pid_name[10]{ 0 };
			swprintf_s(vid_pid_name, L"%04x%04x_", p_joy_selected->Get_VID(), p_joy_selected->Get_PID());

			file_path.append(vid_pid_name);
			file_path.append(text_last_good);
			file_path.append(L".joy");

			WIN32_FIND_DATA FindFileData{};
			HANDLE hFind = hFind = FindFirstFile(file_path.c_str(), &FindFileData);
			int save_ok = 0;
			if (hFind != INVALID_HANDLE_VALUE) {
				LoadString(phinstDLL, IDS_FILE_ALREADY_EXISTS, general_string_buff, _countof(general_string_buff));
				LoadString(phinstDLL, IDS_OVERWRITE, general_string_buff2, _countof(general_string_buff2));
				save_ok = MessageBox(hwndDlg, general_string_buff2, general_string_buff, MB_YESNO | MB_ICONQUESTION);
			}
			else {
				if (GetLastError() == ERROR_FILE_NOT_FOUND)
					save_ok = IDYES;
			}
			FindClose(hFind);

			if(save_ok == IDYES)
				p_joy_selected->Profile_Save(file_path.c_str());

			DestroyWindow(hwndDlg);
			return TRUE;
		}
		case IDCANCEL:
			DestroyWindow(hwndDlg);
			return TRUE;
		default:
			break;
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hwndDlg);
		return FALSE;
	case WM_DESTROY:
		if (text_current)
			delete[] text_current;
		text_current = nullptr;
		if (text_last_good)
			delete[] text_last_good;
		text_last_good = nullptr;

		hWin_SaveAsPreset = nullptr;
		return FALSE;
	}
	return FALSE;
}


//_____________________________________________
static bool JoyConfig_Preset_Save(HWND hwndDlg) {

	HWND hwnd_joy = GetDlgItem(hwndDlg, IDC_COMBO_JOY_SELECT);
	if (!hwnd_joy)
		return false;

	int joy_selected = (int)(SendMessage(hwnd_joy, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	JOYSTICK* p_joy_selected = Joysticks.GetJoy(joy_selected);
	if (!p_joy_selected)
		return false;

	hWin_SaveAsPreset = CreateDialogParam(phinstDLL, MAKEINTRESOURCE(IDD_DIALOG_SAVE_AS_PRESET), hwndDlg, (DLGPROC)DialogProc_SaveAsPreset, (LPARAM)p_joy_selected);
	ShowWindow(hWin_SaveAsPreset, SW_SHOW);
	while (hWin_SaveAsPreset != nullptr) {
		Sleep(16);
		MSG message;
		while (PeekMessage(&message, 0, 0, 0, true)) {
			if (!hWin_SaveAsPreset || !IsDialogMessage(hWin_SaveAsPreset, &message)) {
				TranslateMessage(&message);
				DispatchMessage(&message);
			}
		}
	}

	return true;
}
//_________________________________________________
static bool JoyConfig_Refresh_Presets(HWND hwndDlg) {

	HWND hwnd_joy = GetDlgItem(hwndDlg, IDC_COMBO_JOY_SELECT);
	if (!hwnd_joy)
		return false;
	HWND hwnd_presets = GetDlgItem(hwndDlg, IDC_COMBO_JOY_PRESETS);
	if (!hwnd_presets)
		return false;
	SendMessage(hwnd_presets, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);

	int joy_selected = (int)(SendMessage(hwnd_joy, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	JOYSTICK* p_joy_selected = Joysticks.GetJoy(joy_selected);
	if (!p_joy_selected)
		return false;
	
	std::wstring search_path;
	if(!Get_Joystick_Config_Path(&search_path))
		return false;

	search_path.append(L"\\presets");
	if (GetFileAttributes(search_path.c_str()) == INVALID_FILE_ATTRIBUTES)
			return false;
	
	search_path.append(L"\\");

	wchar_t vid_pid_name[10]{ 0 };
	swprintf_s(vid_pid_name, L"%04x%04x_", p_joy_selected->Get_VID(), p_joy_selected->Get_PID());

	std::wstring file_path = search_path;

	file_path.append(vid_pid_name);
	file_path.append(L"*.joy");

	WIN32_FIND_DATA FindFileData{};
	HANDLE hFind = hFind = FindFirstFile(file_path.c_str(), &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		FindClose(hFind);
		LoadString(phinstDLL, IDS_NONE, general_string_buff, _countof(general_string_buff));
		SendMessage(hwnd_presets, CB_ADDSTRING, (WPARAM)0, (LPARAM)general_string_buff);
		SendMessage(hwnd_presets, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
		EnableWindow(hwnd_presets, FALSE);
		return false;
	}
	EnableWindow(hwnd_presets, TRUE);
	LoadString(phinstDLL, IDS_SELECT_PRESET, general_string_buff, _countof(general_string_buff));
	SendMessage(hwnd_presets, CB_ADDSTRING, (WPARAM)0, (LPARAM)general_string_buff);
	SendMessage(hwnd_presets, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

	std::wstring name;
	do {
		name = &FindFileData.cFileName[9]; 
		name.at(name.find_last_of(L'.')) = L'\0';
		SendMessage(hwnd_presets, CB_ADDSTRING, (WPARAM)0, (LPARAM)name.c_str());
			
	} while (FindNextFile(hFind, &FindFileData));

	FindClose(hFind);
	return true;
}


//______________________________
void JoyConfig_Refresh_JoyList() {

	if (!hWin_Config_Joy)
		return;
	joyList_Updated++;
}


//_________________________________________________
static void JoyConfig_JoyList_Refresh(HWND hwndDlg) {
	if (!hwndDlg)
		return;
	if (joyList_Updated <= 0)
		return;

	current_JoySelected = -1;

	HWND hwnd_joy = GetDlgItem(hwndDlg, IDC_COMBO_JOY_SELECT);
	if (!hwnd_joy)
		return;

	SendMessage(hwnd_joy, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);

	size_t num_joysticks = Joysticks.GetNumJoysticks();

	if (num_joysticks < 1) {
		LoadString(phinstDLL, IDS_NO_CONTROLLERS_DETECTED, general_string_buff, _countof(general_string_buff));
		SendMessage(hwnd_joy, CB_ADDSTRING, (WPARAM)0, (LPARAM)general_string_buff);
	}
	else {
		JOYSTICK* joy = nullptr;
		for (UINT i = 0; i < num_joysticks; i++) {
			joy = Joysticks.GetJoy(i);
			if (!joy)
				return;
			SendMessage(hwnd_joy, CB_ADDSTRING, (WPARAM)0, (LPARAM)joy->Get_DisplayName().c_str());
		}
	}
	SendMessage(hwnd_joy, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);


	JoyConfig_Refresh_Presets(hwndDlg);
	JoyConfig_Refresh_Enabled(hwndDlg);
	JoyConfig_Refresh_Axes(hwndDlg, TRUE, TRUE);
	JoyConfig_Refresh_Buttons(hwndDlg, TRUE);
	JoyConfig_Refresh_Switches(hwndDlg, TRUE, TRUE);

	if (joyList_Updated > 0)
		joyList_Updated--;
}


//_____________________________________________________
//Brings a particular contoller state into focus, when it's corresponding button, axis or switch is manipulated.
static void Update_Controller_State_Focus(HWND hwndDlg) {

	HWND hwnd_joy = GetDlgItem(hwndDlg, IDC_COMBO_JOY_SELECT);
	if (!hwnd_joy)
		return;
	int joy_selected = (int)(SendMessage(hwnd_joy, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));

	JOYSTICK* p_joy_selected = Joysticks.GetJoy(joy_selected);
	if (!p_joy_selected)
		return;
	bool* buttons = nullptr;
	double* axes = nullptr;
	GameControllerSwitchPosition* switches = nullptr;
	int axis_changed = -1;
	int butt_changed = -1;
	int switch_changed = -1;

	if (current_JoySelected != joy_selected) {
		//reset current controller states when joy changed.
		current_JoySelected = joy_selected;

		current_num_axes = p_joy_selected->Axes(&axes);
		
		if (current_axisArray)
			delete[] current_axisArray;
		current_axisArray = nullptr;
		if (current_num_axes > 0) {
			current_axisArray = new double[current_num_axes];
			for (int i = 0; i < current_num_axes; i++)
				current_axisArray[i] = axes[i];
		}


		current_num_buttons = p_joy_selected->Buttons(&buttons);

		if (current_buttonArray)
			delete[] current_buttonArray;
		current_buttonArray = nullptr;
		if (current_num_buttons > 0) {
			current_buttonArray = new bool[current_num_buttons];
			for (int i = 0; i < current_num_buttons; i++)
				current_buttonArray[i] = buttons[i];
		}


		current_num_switches = p_joy_selected->Switches(&switches);

		if (current_switchArray)
			delete[] current_switchArray;
		current_switchArray = nullptr;
		if (current_num_switches > 0) {
			current_switchArray = new GameControllerSwitchPosition[current_num_switches];
			for (int i = 0; i < current_num_switches; i++)
				current_switchArray[i] = switches[i];
		}

	}
	else {
		//compare current controller states against new input.
		int num_axes = p_joy_selected->Axes(&axes);
		if (num_axes > 0 && num_axes == current_num_axes) {
			for (int i = 0; i < current_num_axes; i++) {
				if (current_axisArray[i] > axes[i] + 0.30f || current_axisArray[i] < axes[i] - 0.30f) {
					axis_changed = i;
					current_axisArray[i] = axes[i];
				}
			}
		}
		if (axis_changed >= 0) {
			//Debug_Info("Check_For_Changes - axis changed:%d", axis_changed);
			HWND hwnd_axis = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_AXIS);
			SendMessage(hwnd_axis, CB_SETCURSEL, (WPARAM)axis_changed, (LPARAM)0);
			SetFocus(hwnd_axis);
			JoyConfig_Refresh_Axes(hwndDlg, FALSE, TRUE);
		}
		
		
		int num_buttons = p_joy_selected->Buttons(&buttons);
		if (num_buttons > 0 && num_buttons == current_num_buttons) {
			for (int i = 0; i < current_num_buttons; i++) {
				if (current_buttonArray[i] != buttons[i])
					butt_changed = i;
				current_buttonArray[i] = buttons[i];
			}
		}
		if (butt_changed >= 0) {
			//Debug_Info("Check_For_Changes - butt changed:%d", butt_changed);
			HWND hwnd_selected_button = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_BUTTONS);
			SendMessage(hwnd_selected_button, CB_SETCURSEL, (WPARAM)butt_changed, (LPARAM)0);
			SetFocus(hwnd_selected_button);
			JoyConfig_Refresh_Buttons(hwndDlg, FALSE);
		}


		int num_switches = p_joy_selected->Switches(&switches);
		if (num_switches > 0 && num_switches == current_num_switches) {
			for (int i = 0; i < current_num_switches; i++) {
				if (current_switchArray[i] != switches[i])
					switch_changed = i;
				current_switchArray[i] = switches[i];
			}
		}
		if (switch_changed >= 0) {
			//Debug_Info("Check_For_Changes - switch changed:%d", switch_changed);
			HWND hwnd_selected_switch = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_SWITCHES);
			SendMessage(hwnd_selected_switch, CB_SETCURSEL, (WPARAM)switch_changed, (LPARAM)0);
			SetFocus(hwnd_selected_switch);
			JoyConfig_Refresh_Switches(hwndDlg, FALSE, TRUE);
		}
	}

}


//_________________________________________
static void JoyConfig_Refresh(HWND hwndDlg) {

	JoyConfig_JoyList_Refresh(hwndDlg);

	HWND hwnd_sub = nullptr;

	RECT rc{};
	hwnd_sub = GetDlgItem(hwndDlg, IDC_STATIC_XY_BOX);
	GetWindowRect(hwnd_sub, &rc); //get window rect of control relative to screen
	POINT pt = { rc.left, rc.top }; //new point object using rect x, y
	ScreenToClient(hwndDlg, &pt); //convert screen co-ords to client based points

	int width = rc.right - rc.left - 2 - 9;
	int height = rc.bottom - rc.top - 2 - 9;
	int pos_x = (int)((float)(*p_wc3_joy_move_x + 16) * ((float)width / 32.0f));
	int pos_y = (int)((float)(*p_wc3_joy_move_y + 16) * ((float)height / 32.0f));

	hwnd_sub = GetDlgItem(hwndDlg, IDC_STATIC_XY_CROSS);
	MoveWindow(hwnd_sub, pt.x + 1 + pos_x, pt.y + 1 + pos_y, 9, 9, TRUE);

	hwnd_sub = GetDlgItem(hwndDlg, IDC_STATIC_ROLL_BOX);
	GetWindowRect(hwnd_sub, &rc); //get window rect of control relative to screen
	pt = { rc.left, rc.top }; //new point object using rect x, y
	ScreenToClient(hwndDlg, &pt); //convert screen co-ords to client based points

	width = rc.right - rc.left - 2 - 3;
	pos_x = (int)((float)(*p_wc3_joy_move_r + 256) * ((float)width / 512.0f));
	hwnd_sub = GetDlgItem(hwndDlg, IDC_STATIC_ROLL_BAR);
	MoveWindow(hwnd_sub, pt.x + 1 + pos_x, pt.y + 1, 3, 9, TRUE);

	hwnd_sub = GetDlgItem(hwndDlg, IDC_STATIC_THROTTLE_BOX);
	GetWindowRect(hwnd_sub, &rc); //get window rect of control relative to screen
	pt = { rc.left, rc.top }; //new point object using rect x, y
	ScreenToClient(hwndDlg, &pt); //convert screen co-ords to client based points

	height = rc.bottom - rc.top - 2 - 3;
	pos_y = (int)((float)*p_wc3_joy_throttle_pos * ((float)height / 100.0f));
	hwnd_sub = GetDlgItem(hwndDlg, IDC_STATIC_THROTTLE_BAR);
	MoveWindow(hwnd_sub, pt.x + 1, pt.y + 1 + pos_y, 9, 3, TRUE);


	Update_Controller_State_Focus(hwndDlg);

	JoyConfig_Refresh_Axis_Display(hwndDlg);
	JoyConfig_Refresh_Button_Display(hwndDlg);
	JoyConfig_Refresh_Switch_Display(hwndDlg);
}


//_____________________________________________________________________________________
static void Axis_Calibration_Refresh_Axis_Display(HWND hwndDlg, int joystick, int axis) {

	JOYSTICK* p_joy_selected = Joysticks.GetJoy(joystick);
	if (!p_joy_selected) 
		return;
	
	ACTION_AXIS* p_action_axis = p_joy_selected->Get_Action_Axis(axis);
	if (!p_action_axis) 
		return;
	
	double val = p_action_axis->Get_Current_Val();

	HWND hwnd_sub = GetDlgItem(hwndDlg, IDC_STATIC_AXIS_BOX);
	RECT rc{};
	GetWindowRect(hwnd_sub, &rc); //get window rect of control relative to screen
	POINT pt = { rc.left, rc.top }; //new point object using rect x, y
	ScreenToClient(hwndDlg, &pt); //convert screen co-ords to client based points

	int width = rc.right - rc.left - 2 - 3;

	int i_val = (int)(val * width);
	hwnd_sub = GetDlgItem(hwndDlg, IDC_STATIC_AXIS_BAR);
	MoveWindow(hwnd_sub, pt.x + 1 + i_val, pt.y + 1, 3, 9, TRUE);
}


//_______________________________________________________________________________________________________
static INT_PTR CALLBACK DialogProc_AxisCalibration(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {
	case WM_INITDIALOG: {
		////101 is the wcIII icon
		SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(*pp_hinstWC3W, MAKEINTRESOURCE(101)));

		HWND hwndParent = GetParent(hwndDlg);

		//set position to centre of parent window.
		RECT rc_Win{ 0,0,0,0 };
		GetWindowRect(hwndDlg, &rc_Win);
		RECT rcParent{ 0,0,0,0 };
		GetWindowRect(hwndParent, &rcParent);
		SetWindowPos(hwndDlg, nullptr, rcParent.left + ((rcParent.right - rcParent.left) - (rc_Win.right - rc_Win.left)) / 2, rc_Win.top + ((rcParent.bottom - rcParent.top) - (rc_Win.bottom - rc_Win.top)) / 2, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

		return TRUE;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {

		case IDOK:
		case IDCANCEL: 
			DestroyWindow(hwndDlg);
			return TRUE;
		default:
			break;
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hwndDlg);
		return FALSE;
	case WM_DESTROY: 
		hWin_AxisCalibrate = nullptr;
		return FALSE;
	}
	
	return FALSE;
}

//________________________________________________
static void JoyConfig_Axis_Calibrate(HWND hwndDlg) {

	HWND hwnd_joy = GetDlgItem(hwndDlg, IDC_COMBO_JOY_SELECT);
	if (!hwnd_joy)
		return;
	HWND hwnd_axis = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_AXIS);
	if (!hwnd_axis)
		return;

	int calibrating_joy = (int)(SendMessage(hwnd_joy, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	JOYSTICK* p_joy_selected = Joysticks.GetJoy(calibrating_joy);
	if (!p_joy_selected) 
		return;
	int	calibrating_axis = (int)(SendMessage(hwnd_axis, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	ACTION_AXIS* p_action_axis = p_joy_selected->Get_Action_Axis(calibrating_axis);
	if (!p_action_axis)
		return;
	p_action_axis->Calibrate(TRUE);

	hWin_AxisCalibrate = CreateDialog(phinstDLL, MAKEINTRESOURCE(IDD_DIALOG_CALIBRATE), hwndDlg, (DLGPROC)DialogProc_AxisCalibration);
	ShowWindow(hWin_AxisCalibrate, SW_SHOW);
	while (hWin_AxisCalibrate != nullptr) {
		Sleep(16);
		MSG message;
		while (PeekMessage(&message, 0, 0, 0, true)) {
			if (!hWin_AxisCalibrate || !IsDialogMessage(hWin_AxisCalibrate, &message)) {
				TranslateMessage(&message);
				DispatchMessage(&message);
			}
		}
		Joysticks.Update();
		Axis_Calibration_Refresh_Axis_Display(hWin_AxisCalibrate, calibrating_joy, calibrating_axis);
	}

	//re-get p_joy_selected and p_action_axis incase joystick was lost during axis calibration.
	p_joy_selected = Joysticks.GetJoy(calibrating_joy);
	if (!p_joy_selected)
		return;
	p_action_axis = p_joy_selected->Get_Action_Axis(calibrating_axis);
	if (!p_action_axis)
		return;
	p_action_axis->Calibrate(FALSE);


}


//_____________________________________________
static void JoyConfig_Axis_Centre(HWND hwndDlg) {

	HWND hwnd_joy = GetDlgItem(hwndDlg, IDC_COMBO_JOY_SELECT);
	if (!hwnd_joy)
		return;
	HWND hwnd_axis = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_AXIS);
	if (!hwnd_axis)
		return;


	int joy_selected = (int)(SendMessage(hwnd_joy, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	JOYSTICK* p_joy_selected = Joysticks.GetJoy(joy_selected);
	if (!p_joy_selected)
		return;

	int axis_selected = (int)(SendMessage(hwnd_axis, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
	ACTION_AXIS* p_action_axis = p_joy_selected->Get_Action_Axis(axis_selected);
	if (!p_action_axis)
		return;

	p_action_axis->Centre();
}


//_________________________________________________________________________________________________
static INT_PTR CALLBACK DialogProc_JoyConfig(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	//static HWND hwndParent = nullptr;
	//static bool was_Deactivated = false;
	switch (uMsg) {
	case WM_INITDIALOG: {

		InitCommonControls();

		hWin_Config_Joy = hwndDlg;

		JoyConfig_Refresh_Presets(hwndDlg);
		JoyConfig_Refresh_Enabled(hwndDlg);

		JoyConfig_Refresh_Axes(hwndDlg, TRUE, TRUE);
		JoyConfig_Refresh_Buttons(hwndDlg, TRUE);
		JoyConfig_Refresh_Switches(hwndDlg, TRUE, TRUE);

		HWND hwnd_sub = nullptr;
		RECT rc{};
		POINT pt{};

		//set size of roll box
		hwnd_sub = GetDlgItem(hwndDlg, IDC_STATIC_ROLL_BOX);
		GetWindowRect(hwnd_sub, &rc);
		pt = { rc.left, rc.top };
		ScreenToClient(hwndDlg, &pt);
		MoveWindow(hwnd_sub, pt.x, pt.y, rc.right - rc.left, 9 + 2, TRUE);
		//set size of throttle box
		hwnd_sub = GetDlgItem(hwndDlg, IDC_STATIC_THROTTLE_BOX);
		GetWindowRect(hwnd_sub, &rc);
		pt = { rc.left, rc.top };
		ScreenToClient(hwndDlg, &pt);
		MoveWindow(hwnd_sub, pt.x, pt.y, 9 + 2, rc.bottom - rc.top, TRUE);

		//fill text for axis type combo
		hwnd_sub = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_AXIS_TYPE);

		//AXIS_TYPE_UID

		for (int i = 0; i < _countof(AXIS_TYPE_UID); i++) {
			LoadString(phinstDLL, AXIS_TYPE_UID[i], general_string_buff, _countof(general_string_buff));
			SendMessage(hwnd_sub, CB_ADDSTRING, (WPARAM)0, (LPARAM)general_string_buff);
		}
		SendMessage(hwnd_sub, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);


		HWND hwnd_axis_act1 = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_AXIS_BUTTON1);
		HWND hwnd_axis_act2 = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_AXIS_BUTTON2);
		HWND hwnd_butt_act = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_BUTTON_ACTION);
		HWND hwnd_switch_act = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_SWITCH_ACTION);

		//fill text for action combo's
		//LoadString(phinstDLL, IDS_NONE, general_string_buff, _countof(general_string_buff));
		for (int i = 0; i < _countof(WC3_ACTION_UID); i++) {
			//
			LoadString(phinstDLL, WC3_ACTION_UID[i], general_string_buff, _countof(general_string_buff));
			SendMessage(hwnd_axis_act1, CB_ADDSTRING, (WPARAM)0, (LPARAM)general_string_buff);
			SendMessage(hwnd_axis_act2, CB_ADDSTRING, (WPARAM)0, (LPARAM)general_string_buff);
			SendMessage(hwnd_butt_act, CB_ADDSTRING, (WPARAM)0, (LPARAM)general_string_buff);
			SendMessage(hwnd_switch_act, CB_ADDSTRING, (WPARAM)0, (LPARAM)general_string_buff);
		}
		//Debug_Info("_countof(WC3_ACTION_UID) %d", _countof(WC3_ACTION_UID));
		SendMessage(hwnd_axis_act1, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
		SendMessage(hwnd_axis_act2, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
		SendMessage(hwnd_butt_act, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
		SendMessage(hwnd_switch_act, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

		hwnd_sub = GetDlgItem(hwndDlg, IDC_COMBO_DEAD_ZONE);


		//wc axes have 16 degrees of movement from centre, mark deadzone levels as percentages for easier reading. 6.25% == 1/16 of axis from centre.
		SendMessage(hwnd_sub, CB_ADDSTRING, (WPARAM)0, (LPARAM)L"0%");
		SendMessage(hwnd_sub, CB_ADDSTRING, (WPARAM)0, (LPARAM)L"3.125%");
		SendMessage(hwnd_sub, CB_ADDSTRING, (WPARAM)0, (LPARAM)L"6.25%");
		SendMessage(hwnd_sub, CB_ADDSTRING, (WPARAM)0, (LPARAM)L"9.375%");
		SendMessage(hwnd_sub, CB_ADDSTRING, (WPARAM)0, (LPARAM)L"12.5%");
		SendMessage(hwnd_sub, CB_ADDSTRING, (WPARAM)0, (LPARAM)L"15.625%");
		SendMessage(hwnd_sub, CB_ADDSTRING, (WPARAM)0, (LPARAM)L"18.75%");
		SendMessage(hwnd_sub, CB_ADDSTRING, (WPARAM)0, (LPARAM)L"21.875%");
		SendMessage(hwnd_sub, CB_ADDSTRING, (WPARAM)0, (LPARAM)L"25%");
		SendMessage(hwnd_sub, CB_ADDSTRING, (WPARAM)0, (LPARAM)L"28.125%");
		SendMessage(hwnd_sub, CB_ADDSTRING, (WPARAM)0, (LPARAM)L"31.25%");

		SendMessage(hwnd_sub, CB_SETCURSEL, (WPARAM)Joysticks.Deadzone_Level(), (LPARAM)0);

		return TRUE;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_COMBO_JOY_SELECT:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				JoyConfig_Refresh_Presets(hwndDlg);
				JoyConfig_Refresh_Enabled(hwndDlg);
				JoyConfig_Refresh_Axes(hwndDlg, TRUE, TRUE);
				JoyConfig_Refresh_Buttons(hwndDlg, TRUE);
				JoyConfig_Refresh_Switches(hwndDlg, TRUE, TRUE);
			}
			return TRUE;
		case IDC_COMBO_JOY_PRESETS:
			if (HIWORD(wParam) == CBN_SELCHANGE)
				if (JoyConfig_Preset_Set(hwndDlg)) {
					JoyConfig_Refresh_Enabled(hwndDlg);
					JoyConfig_Refresh_Axes(hwndDlg, TRUE, TRUE);
					JoyConfig_Refresh_Buttons(hwndDlg, TRUE);
					JoyConfig_Refresh_Switches(hwndDlg, TRUE, TRUE);
				}
			return TRUE;
		case IDC_BUTTON_SAVE_PRESET:
			if (JoyConfig_Preset_Save(hwndDlg)) 
				JoyConfig_Refresh_Presets(hwndDlg);
			return TRUE;
		case IDC_CHECK_JOY_ENABLE:
			if (HIWORD(wParam) == BN_CLICKED)
				JoyConfig_Update_Enabled(hwndDlg);
			return TRUE;
			
		case IDC_COMBO_DEAD_ZONE:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				int deadzone = (int)(SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_DEAD_ZONE), CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
				Joysticks.Set_Deadzone_Level(deadzone);
			}
			return TRUE;
		case IDC_BUTTON_CENTRE_ALL:
			Joysticks.Centre_All();
			return TRUE;

		case IDC_COMBO_SELECT_AXIS:
			if (HIWORD(wParam) == CBN_SELCHANGE)
				JoyConfig_Refresh_Axes(hwndDlg, FALSE, TRUE);
			
			return TRUE;
		case IDC_COMBO_SELECT_AXIS_TYPE:
			if (HIWORD(wParam) == CBN_SELCHANGE)
				JoyConfig_Axis_SetType(hwndDlg);
			return TRUE;
		case IDC_CHECK_SELECTED_AXIS_SIGN:
			if (HIWORD(wParam) == BN_CLICKED)
				JoyConfig_Axis_SetSign(hwndDlg);
			return TRUE;
		case IDC_COMBO_SELECT_AXIS_BUTTON1:
		case IDC_COMBO_SELECT_AXIS_BUTTON2:
			if (HIWORD(wParam) == CBN_SELCHANGE)
				JoyConfig_Axis_SetButton(hwndDlg, LOWORD(wParam));
			return TRUE;
		case IDC_BUTTON_CALIBRATE_AXIS:
			JoyConfig_Axis_Calibrate(hwndDlg);
			return TRUE;
		case IDC_BUTTON_CENTRE_AXIS: {
			JoyConfig_Axis_Centre(hwndDlg);
			return TRUE;
		}

		case IDC_COMBO_SELECT_BUTTONS:
			if (HIWORD(wParam) == CBN_SELCHANGE)
				JoyConfig_Refresh_Buttons(hwndDlg, FALSE);
			return TRUE;

		case IDC_COMBO_SELECT_BUTTON_ACTION:
			if (HIWORD(wParam) == CBN_SELCHANGE)
				JoyConfig_Button_SetButton(hwndDlg);
			return TRUE;


		case IDC_COMBO_SELECT_SWITCHES:
			if (HIWORD(wParam) == CBN_SELCHANGE)
				JoyConfig_Refresh_Switches(hwndDlg, FALSE, TRUE);
			return TRUE;
		case IDC_COMBO_SELECT_SWITCH_POS:
			if (HIWORD(wParam) == CBN_SELCHANGE)
				JoyConfig_Refresh_Switches(hwndDlg, FALSE, FALSE);
			return TRUE;
		case IDC_COMBO_SELECT_SWITCH_ACTION:
			if (HIWORD(wParam) == CBN_SELCHANGE)
				JoyConfig_Switch_SetButton(hwndDlg);
			return TRUE;

		default:
			break;
		}
		break;
	case WM_DESTROY: {
		hWin_Config_Joy = nullptr;
		return FALSE;
	}
	default:
		return FALSE;
		break;
	}

	return TRUE;
}


//___________________________________________________________________________
BOOL JoyConfig_Refresh_CurrentAction_Mouse(WC3_ACTIONS action, BOOL activate) {

	if (!hWin_Config_Mouse)
		return FALSE;

	HWND hwnd_sub = GetDlgItem(hWin_Config_Mouse, IDC_STATIC_CURRENT_ACTION);
	if (!hwnd_sub)
		return FALSE;

	UINT UID = WC3_ACTION_UID[static_cast<int>(WC3_ACTIONS::None)];
	if (activate)
		UID = WC3_ACTION_UID[static_cast<int>(action)];
	LoadString(phinstDLL, UID, general_string_buff, _countof(general_string_buff));
	SendMessage(hwnd_sub, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)general_string_buff);
	
	return TRUE;
}


//____________________________________________________
static void JoyConfig_Refresh_Mouse_Display(HWND hwnd) {

	RedrawWindow(GetDlgItem(hwnd, IDC_STATIC_B1), nullptr, nullptr, RDW_INVALIDATE);
	RedrawWindow(GetDlgItem(hwnd, IDC_STATIC_B2), nullptr, nullptr, RDW_INVALIDATE);
	RedrawWindow(GetDlgItem(hwnd, IDC_STATIC_B3), nullptr, nullptr, RDW_INVALIDATE);
	RedrawWindow(GetDlgItem(hwnd, IDC_STATIC_B4), nullptr, nullptr, RDW_INVALIDATE);
	RedrawWindow(GetDlgItem(hwnd, IDC_STATIC_B5), nullptr, nullptr, RDW_INVALIDATE);
	RedrawWindow(GetDlgItem(hwnd, IDC_STATIC_SCROLL_UP), nullptr, nullptr, RDW_INVALIDATE);
	RedrawWindow(GetDlgItem(hwnd, IDC_STATIC_SCROLL_DN), nullptr, nullptr, RDW_INVALIDATE);
	RedrawWindow(GetDlgItem(hwnd, IDC_STATIC_SCROLL_LEFT), nullptr, nullptr, RDW_INVALIDATE);
	RedrawWindow(GetDlgItem(hwnd, IDC_STATIC_SCROLL_RIGHT), nullptr, nullptr, RDW_INVALIDATE);
}


//____________________________________________________________________________________________________
static INT_PTR CALLBACK DialogProc_Config_Mouse(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	
	static HBRUSH hbrush_colour_box = nullptr;
	static DWORD button_states = 0;

	switch (uMsg) {
	case WM_INITDIALOG: {
		hWin_Config_Mouse = hwndDlg;
		InitCommonControls();

		HWND hwnd_sub = nullptr;

		hwnd_sub = GetDlgItem(hwndDlg, IDC_COMBO_DEAD_ZONE);

		//wc axes have 16 degrees of movement from centre, mark deadzone levels as percentages for easier reading. 6.25% == 1/16 of axis from centre.
		SendMessage(hwnd_sub, CB_ADDSTRING, (WPARAM)0, (LPARAM)L"0%");
		SendMessage(hwnd_sub, CB_ADDSTRING, (WPARAM)0, (LPARAM)L"3.125%");
		SendMessage(hwnd_sub, CB_ADDSTRING, (WPARAM)0, (LPARAM)L"6.25%");
		SendMessage(hwnd_sub, CB_ADDSTRING, (WPARAM)0, (LPARAM)L"9.375%");
		SendMessage(hwnd_sub, CB_ADDSTRING, (WPARAM)0, (LPARAM)L"12.5%");
		SendMessage(hwnd_sub, CB_ADDSTRING, (WPARAM)0, (LPARAM)L"15.625%");
		SendMessage(hwnd_sub, CB_ADDSTRING, (WPARAM)0, (LPARAM)L"18.75%");
		SendMessage(hwnd_sub, CB_ADDSTRING, (WPARAM)0, (LPARAM)L"21.875%");
		SendMessage(hwnd_sub, CB_ADDSTRING, (WPARAM)0, (LPARAM)L"25%");
		SendMessage(hwnd_sub, CB_ADDSTRING, (WPARAM)0, (LPARAM)L"28.125%");
		SendMessage(hwnd_sub, CB_ADDSTRING, (WPARAM)0, (LPARAM)L"31.25%");

		SendMessage(hwnd_sub, CB_SETCURSEL, (WPARAM)Mouse.Deadzone_Level(), (LPARAM)0);
		

		//setup button selection combo.
		hwnd_sub = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_BUTTONS);
		wchar_t* msg = new wchar_t[12];
		LoadString(phinstDLL, IDS_BUTTON, general_string_buff, _countof(general_string_buff));
		for (int i = 0; i < NUM_MOUSE_BUTTONS; i++) {
			swprintf_s(msg, 12, L"%s %d", general_string_buff, i+1);
			SendMessage(hwnd_sub, CB_ADDSTRING, (WPARAM)0, (LPARAM)msg);
		}
		delete[] msg;
		SendMessage(hwnd_sub, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

		//fill action selection lists.
		HWND hwnd_button_actions = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_BUTTON_ACTION);
		HWND hwnd_wheel_up_actions = GetDlgItem(hwndDlg, IDC_COMBO_WHEEL_UP_ACTION);
		HWND hwnd_wheel_down_actions = GetDlgItem(hwndDlg, IDC_COMBO_WHEEL_DOWN_ACTION);
		HWND hwnd_wheel_left_actions = GetDlgItem(hwndDlg, IDC_COMBO_WHEEL_LEFT_ACTION);
		HWND hwnd_wheel_right_actions = GetDlgItem(hwndDlg, IDC_COMBO_WHEEL_RIGHT_ACTION);

		for (int i = 0; i < _countof(WC3_ACTION_UID); i++) {
			//
			LoadString(phinstDLL, WC3_ACTION_UID[i], general_string_buff, _countof(general_string_buff));
			SendMessage(hwnd_button_actions, CB_ADDSTRING, (WPARAM)0, (LPARAM)general_string_buff);
			SendMessage(hwnd_wheel_up_actions, CB_ADDSTRING, (WPARAM)0, (LPARAM)general_string_buff);
			SendMessage(hwnd_wheel_down_actions, CB_ADDSTRING, (WPARAM)0, (LPARAM)general_string_buff);
			SendMessage(hwnd_wheel_left_actions, CB_ADDSTRING, (WPARAM)0, (LPARAM)general_string_buff);
			SendMessage(hwnd_wheel_right_actions, CB_ADDSTRING, (WPARAM)0, (LPARAM)general_string_buff);
		}
		
		SendMessage(hwnd_button_actions, CB_SETCURSEL, (WPARAM)Mouse.GetAction_Button(0), (LPARAM)0);
		SendMessage(hwnd_wheel_up_actions, CB_SETCURSEL, (WPARAM)Mouse.GetAction_Wheel_Up(), (LPARAM)0);
		SendMessage(hwnd_wheel_down_actions, CB_SETCURSEL, (WPARAM)Mouse.GetAction_Wheel_Down(), (LPARAM)0);
		SendMessage(hwnd_wheel_left_actions, CB_SETCURSEL, (WPARAM)Mouse.GetAction_Wheel_Left(), (LPARAM)0);
		SendMessage(hwnd_wheel_right_actions, CB_SETCURSEL, (WPARAM)Mouse.GetAction_Wheel_Right(), (LPARAM)0);

		break;
	}
	case WM_CTLCOLORSTATIC: {
		//highlight pressed buttons.
		if (!hbrush_colour_box)
			hbrush_colour_box = CreateSolidBrush(RGB(128, 128, 128));

		if (((HWND)lParam == GetDlgItem(hwndDlg, IDC_STATIC_B1) && (button_states & (1 << 0))) ||
			((HWND)lParam == GetDlgItem(hwndDlg, IDC_STATIC_B2) && (button_states & (1 << 1))) ||
			((HWND)lParam == GetDlgItem(hwndDlg, IDC_STATIC_B3) && (button_states & (1 << 2))) ||
			((HWND)lParam == GetDlgItem(hwndDlg, IDC_STATIC_B4) && (button_states & (1 << 3))) ||
			((HWND)lParam == GetDlgItem(hwndDlg, IDC_STATIC_B5) && (button_states & (1 << 4))) ||
			((HWND)lParam == GetDlgItem(hwndDlg, IDC_STATIC_SCROLL_UP) && (button_states & (1 << 5))) ||
			((HWND)lParam == GetDlgItem(hwndDlg, IDC_STATIC_SCROLL_DN) && (button_states & (1 << 6))) ||
			((HWND)lParam == GetDlgItem(hwndDlg, IDC_STATIC_SCROLL_LEFT) && (button_states & (1 << 7))) ||
			((HWND)lParam == GetDlgItem(hwndDlg, IDC_STATIC_SCROLL_RIGHT) && (button_states & (1 << 8)))) {
			HDC hdcStatic = (HDC)wParam;
			SetTextColor(hdcStatic, RGB(255, 255, 255));
			SetBkColor(hdcStatic, RGB(128, 128, 128));
			return (INT_PTR)hbrush_colour_box;
		}
		return FALSE;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_COMBO_DEAD_ZONE:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				int deadzone = (int)(SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_DEAD_ZONE), CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
				Mouse.Set_Deadzone_Level(deadzone);
			}
			return TRUE;
		case IDC_COMBO_SELECT_BUTTONS:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				HWND hwnd_button = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_BUTTONS);
				int button_selected = (int)(SendMessage(hwnd_button, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
				HWND hwnd_actions = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_BUTTON_ACTION);
				SendMessage(hwnd_actions, CB_SETCURSEL, (WPARAM)Mouse.GetAction_Button(button_selected), (LPARAM)0);
			}
			return TRUE;
		case IDC_COMBO_SELECT_BUTTON_ACTION:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				HWND hwnd_button = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_BUTTONS);
				int button_selected = (int)(SendMessage(hwnd_button, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
				HWND hwnd_actions = GetDlgItem(hwndDlg, IDC_COMBO_SELECT_BUTTON_ACTION);
				int action_selected = (int)(SendMessage(hwnd_actions, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
				Mouse.SetAction_Button(button_selected, static_cast<WC3_ACTIONS>(action_selected));
			}
			return TRUE;
		case IDC_COMBO_WHEEL_UP_ACTION:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				HWND hwnd_action = GetDlgItem(hwndDlg, IDC_COMBO_WHEEL_UP_ACTION);
				int action_selected = (int)(SendMessage(hwnd_action, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
				Mouse.SetAction_Wheel_Up(static_cast<WC3_ACTIONS>(action_selected));
			}
			return TRUE;
		case IDC_COMBO_WHEEL_DOWN_ACTION:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				HWND hwnd_action = GetDlgItem(hwndDlg, IDC_COMBO_WHEEL_DOWN_ACTION);
				int action_selected = (int)(SendMessage(hwnd_action, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
				Mouse.SetAction_Wheel_Down(static_cast<WC3_ACTIONS>(action_selected));
			}
			return TRUE;
		case IDC_COMBO_WHEEL_LEFT_ACTION:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				HWND hwnd_action = GetDlgItem(hwndDlg, IDC_COMBO_WHEEL_LEFT_ACTION);
				int action_selected = (int)(SendMessage(hwnd_action, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
				Mouse.SetAction_Wheel_Left(static_cast<WC3_ACTIONS>(action_selected));
			}
			return TRUE;
		case IDC_COMBO_WHEEL_RIGHT_ACTION:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				HWND hwnd_action = GetDlgItem(hwndDlg, IDC_COMBO_WHEEL_RIGHT_ACTION);
				int action_selected = (int)(SendMessage(hwnd_action, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
				Mouse.SetAction_Wheel_Right(static_cast<WC3_ACTIONS>(action_selected));
			}
			return TRUE;
		default:
			break;
		}
		break;
	case WM_DESTROY: {
		if (hbrush_colour_box)
			DeleteObject(hbrush_colour_box);
		hbrush_colour_box = nullptr;

		hWin_Config_Mouse = nullptr;
		return FALSE;
	}
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP: {
		Mouse.Update_Buttons(wParam);

		int key_state = GET_KEYSTATE_WPARAM(wParam);
		if (key_state & MK_LBUTTON)
			button_states |= (1 << 0);
		else
			button_states &= ~(1 << 0);
		if (key_state & MK_RBUTTON)
			button_states |= (1 << 1);
		else
			button_states &= ~(1 << 1);
		if (key_state & MK_MBUTTON)
			button_states |= (1 << 2);
		else
			button_states &= ~(1 << 2);
		if (key_state & MK_XBUTTON1)
			button_states |= (1 << 3);
		else
			button_states &= ~(1 << 3);
		if (key_state & MK_XBUTTON2)
			button_states |= (1 << 4);
		else
			button_states &= ~(1 << 4);

		//clear scroll wheel states
		button_states &= ~(1 << 5);
		button_states &= ~(1 << 6);
		button_states &= ~(1 << 7);
		button_states &= ~(1 << 8);

		JoyConfig_Refresh_Mouse_Display(hwndDlg);
		break;
	}
	case WM_MOUSEWHEEL: {
		Mouse.Update_Wheel_Vertical(wParam);
		short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		if (zDelta > 0) {
			button_states |= (1 << 5);
			button_states &= ~(1 << 6);
		}
		else if (zDelta < 0) {
			button_states |= (1 << 6);
			button_states &= ~(1 << 5);
		}
		JoyConfig_Refresh_Mouse_Display(hwndDlg);
		break;
	}
	case WM_MOUSEHWHEEL: {
		Mouse.Update_Wheel_Horizontal(wParam);
		short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		if (zDelta > 0) {
			button_states |= (1 << 7);
			button_states &= ~(1 << 8);
		}
		else if (zDelta < 0) {
			button_states |= (1 << 8);
			button_states &= ~(1 << 7);
		}
		JoyConfig_Refresh_Mouse_Display(hwndDlg);
		break;
	}
	default:
		return FALSE;
		break;
	}
	return TRUE;
}


//______________________________________________________________________________________________________
static INT_PTR CALLBACK DialogProc_Config_Control(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static HWND hwndParent = nullptr;
	static bool was_Deactivated = false;
	
	switch (uMsg) {
	case WM_INITDIALOG: {
		hWin_Config_Control = hwndDlg;
		
		////101 is the wcIII icon
		SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(*pp_hinstWC3W, MAKEINTRESOURCE(101)));
		InitCommonControls();

		hwndParent = GetParent(hwndDlg);

		INITCOMMONCONTROLSEX iccex{ 0 };
		//initialize common controls.
		iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
		iccex.dwICC = ICC_TAB_CLASSES;
		InitCommonControlsEx(&iccex);

		TCITEM tie{ 0 };

		HWND hwndTab = GetDlgItem(hwndDlg, IDC_TAB1);
		//add a tab for each of the child dialog boxes.
		tie.mask = TCIF_TEXT | TCIF_IMAGE;
		tie.iImage = -1;

		LoadString(phinstDLL, IDS_TAB_JOYSTICK, general_string_buff, _countof(general_string_buff));
		tie.pszText = general_string_buff;
		TabCtrl_InsertItem(hwndTab, 0, &tie);

		LoadString(phinstDLL, IDS_TAB_MOUSE, general_string_buff, _countof(general_string_buff));
		tie.pszText = general_string_buff;
		TabCtrl_InsertItem(hwndTab, 1, &tie);


		//set position to centre of parent window.
		RECT rc_Win{ 0,0,0,0 };
		GetWindowRect(hwndDlg, &rc_Win);
		RECT rcParent{ 0,0,0,0 };
		GetWindowRect(hwndParent, &rcParent);
		SetWindowPos(hwndDlg, nullptr, rcParent.left + ((rcParent.right - rcParent.left) - (rc_Win.right - rc_Win.left)) / 2, rc_Win.top + ((rcParent.bottom - rcParent.top) - (rc_Win.bottom - rc_Win.top)) / 2, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

		//disable main window while this dialog is running.
		EnableWindow(hwndParent, FALSE);
		
		//create tab windows.
		hWin_Config_Joy = CreateDialogParam(phinstDLL, MAKEINTRESOURCE(IDD_DIALOG_CONFIG_JOY), hwndDlg, &DialogProc_JoyConfig, 0);
		hWin_Config_Mouse = CreateDialogParam(phinstDLL, MAKEINTRESOURCE(IDD_DIALOG_CONFIG_MOUSE), hwndDlg, &DialogProc_Config_Mouse, 0);

		//set the position of tab windows, adjusting for the height of the tabs.
		RECT rcTab;
		GetClientRect(hwndDlg, &rcTab);
		TabCtrl_AdjustRect(hwndTab, FALSE, &rcTab);

		SetWindowPos(hWin_Config_Joy, nullptr, rcTab.left, rcTab.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		SetWindowPos(hWin_Config_Mouse, nullptr, rcTab.left, rcTab.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

		//set initial focus tab.
		if (*p_wc3_controller_mouse == 1) {
			TabCtrl_SetCurFocus(hwndTab, 1);
			ShowWindow(hWin_Config_Joy, SW_HIDE);
			ShowWindow(hWin_Config_Mouse, SW_SHOW);
		}
		else {
			TabCtrl_SetCurFocus(hwndTab, 0);
			ShowWindow(hWin_Config_Joy, SW_SHOW);
			ShowWindow(hWin_Config_Mouse, SW_HIDE);
		}
		break;
	}
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case TCN_SELCHANGE: {
			HWND hwndTab = GetDlgItem(hwndDlg, IDC_TAB1);
			int tabNum = TabCtrl_GetCurSel(hwndTab);
			if (tabNum == 1) {
				ShowWindow(hWin_Config_Joy, SW_HIDE);
				ShowWindow(hWin_Config_Mouse, SW_SHOW);
			}
			else if (tabNum == 0) {
				ShowWindow(hWin_Config_Joy, SW_SHOW);
				ShowWindow(hWin_Config_Mouse, SW_HIDE);
			}
			break;
		}
		default:
			break;
		}
		break;
	case WM_MOVE: {
		HWND hwndTab = GetDlgItem(hwndDlg, IDC_TAB1);
		//move tab windows with the main window, adjusting for the height of the tabs.
		RECT rcTab;
		GetClientRect(hwndDlg, &rcTab);
		TabCtrl_AdjustRect(hwndTab, FALSE, &rcTab);

		SetWindowPos(hWin_Config_Joy, nullptr, rcTab.left, rcTab.top, 0,0, SWP_NOZORDER| SWP_NOSIZE);
		SetWindowPos(hWin_Config_Mouse, nullptr, rcTab.left, rcTab.top, 0,0, SWP_NOZORDER | SWP_NOSIZE);

		return TRUE;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK: {
			Joysticks.Save();
			Mouse.Save();
			EnableWindow(hwndParent, TRUE);
			DestroyWindow(hwndDlg);
			return FALSE;
		}
		case IDCANCEL: {
			Joysticks.Load();
			Mouse.Load();
			EnableWindow(hwndParent, TRUE);
			DestroyWindow(hwndDlg);
			return FALSE;
		}
		default:
			break;
		}
		break;
	case WM_CLOSE:
		Joysticks.Load();
		Mouse.Load();
		EnableWindow(hwndParent, TRUE);
		DestroyWindow(hwndDlg);
		return FALSE;
	case WM_DESTROY: {
		hWin_Config_Control = nullptr;
		if (was_Deactivated)
			Set_WindowActive_State(TRUE);
		return FALSE;
	}
	case WM_ACTIVATEAPP:
		//WM_ACTIVATEAPP:wParam==FALSE wont be received with the parent window disabled so Set_WindowActive_State(TRUE) on exit to return re-enable fullscreen window.
		if (wParam == FALSE)
			was_Deactivated = true;
		else
			was_Deactivated = false;

		return FALSE;
	default:
		return FALSE;
		break;
	}

	return TRUE;
}


//__________________________________________________________
static HWND JoyConfig_Create(HWND hwnd, HINSTANCE hinstance) {

	if (hWin_Config_Control)
		return hWin_Config_Control;

	HWND hwndDlg = CreateDialog(hinstance, MAKEINTRESOURCE(IDD_DIALOG_CONFIG_MAIN), hwnd, (DLGPROC)DialogProc_Config_Control);
	if (!hwndDlg)
		return hwndDlg;
	JoyConfig_Refresh_JoyList();

	ShowWindow(hwndDlg, SW_SHOW);

	return hwndDlg;
}


//___________________
BOOL JoyConfig_Main() {

	if (!JoyConfig_Create(*p_wc3_hWinMain, phinstDLL))
		return FALSE;

	ShowCursor(TRUE);
	while (hWin_Config_Control != nullptr) {
		Sleep(16);
		MSG message;
		while (PeekMessage(&message, 0, 0, 0, true)) {
			if (!hWin_Config_Control || !IsDialogMessage(hWin_Config_Control, &message)) {
				TranslateMessage(&message);
				DispatchMessage(&message);
			}
		}
		Joysticks.Update();
		if(hWin_Config_Joy)
		JoyConfig_Refresh(hWin_Config_Joy);
	}


	current_num_axes = 0;
	if (current_axisArray)
		delete[] current_axisArray;
	current_axisArray = nullptr;

	current_num_buttons = 0;
	if (current_buttonArray)
		delete[] current_buttonArray;
	current_buttonArray = nullptr;

	current_num_switches = 0;
	if (current_switchArray)
		delete[] current_switchArray;
	current_switchArray = nullptr;


	ShowCursor(FALSE);
	return TRUE;
}
