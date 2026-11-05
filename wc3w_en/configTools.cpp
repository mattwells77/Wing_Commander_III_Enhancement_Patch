/*
The MIT License (MIT)
Copyright © 2024-2026 Matt Wells

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
#include "version.h"
#include "input.h"
#include "libvlc_Music.h"

#include <iomanip>
using namespace std;

wstring movies_path_override;
wstring movies_ext_override;


//________________________
static BOOL Build_Config() {
    wofstream config_stream;

    wstring wConfigPath;
    wConfigPath.assign(GetAppPath());

    wConfigPath.append(L"\\");
    wConfigPath.append(VER_PRODUCTNAME_STR);
    wConfigPath.append(L".ini");

    config_stream.open(wConfigPath.c_str());
    if (!config_stream.is_open())
        return FALSE;

    config_stream << "; " << VER_FILE_DESCRIPTION_STR << endl;
    config_stream << "; Check " << VER_PRODUCTNAME_STR << "_readme.txt for further information regarding the below settings." << endl << endl;

    config_stream << endl << "[MAIN]" << endl;
    config_stream << "VERSION=" << VER_FILE_VERSION_STR << endl;
    config_stream << "UAC_AWARE=" << CONFIG_MAIN_UAC_AWARE << endl;
    config_stream << "ENABLE_DPI_AWARENESS=" << CONFIG_MAIN_ENABLE_DPI_AWARENESS << endl;

    config_stream << "ENABLE_CONTROLLER_ENHANCEMENTS=" << CONFIG_MAIN_ENABLE_CONTROLLER_ENHANCEMENTS << endl;
    config_stream << "ENABLE_MUSIC_ENHANCEMENTS=" << CONFIG_MAIN_ENABLE_MUSIC_ENHANCEMENTS << endl;

    config_stream << "ENABLE_SUBTITLES=" << CONFIG_MAIN_ENABLE_SUBTITLES << endl;
    config_stream << "LANGUAGE_REF=" << CONFIG_MAIN_LANGUAGE_REF << endl;

    config_stream << "ENABLE_ESC_KEY_TO_EXIT_GAME=" << CONFIG_MAIN_ENABLE_ESC_KEY_TO_EXIT_GAME << endl;
    
    config_stream << "DEAD_ZONE=" << CONFIG_MAIN_DEAD_ZONE << endl;
    config_stream << "WINDOWED=" << CONFIG_MAIN_WINDOWED << endl;

    config_stream << "ENABLE_LINEAR_UPSCALING_GUI=" << CONFIG_MAIN_ENABLE_LINEAR_UPSCALING_GUI << endl;
    config_stream << "ENABLE_LINEAR_UPSCALING_COCKPIT_HUD=" << CONFIG_MAIN_ENABLE_LINEAR_UPSCALING_COCKPIT_HUD << endl;

    config_stream << "VIRTUAL_MEM_SIZE=" << CONFIG_MAIN_VIRTUAL_MEM_SIZE << endl;
    config_stream << "NUM_WATCHERS_OVERIDE=" << CONFIG_MAIN_NUM_WATCHERS_OVERIDE << endl;

    config_stream << endl << "[SPACE]" << endl;
    config_stream << "USE_ORIGINAL_ASPECT_RATIO=" << CONFIG_SPACE_USE_ORIGINAL_ASPECT_RATIO << endl;
    config_stream << "COCKPIT_MAINTAIN_ASPECT_RATIO=" << CONFIG_SPACE_COCKPIT_MAINTAIN_ASPECT_RATIO << endl;
    config_stream << "LOD_LEVEL_DISTANCE_MODIFIER=" << CONFIG_SPACE_LOD_LEVEL_DISTANCE_MODIFIER << endl;
    config_stream << "SPACE_REFRESH_RATE_HZ=" << CONFIG_SPACE_SPACE_REFRESH_RATE_HZ << endl;
    config_stream << "NAV_SCREEN_KEY_RESPONSE_HZ=" << CONFIG_SPACE_NAV_SCREEN_KEY_RESPONSE_HZ << endl;
    config_stream << "ENABLE_SPACE_COLOUR_CHANGE=" << CONFIG_SPACE_ENABLE_SPACE_COLOUR_CHANGE << endl;
    config_stream << "SPACE_COLOUR_PALETTE_OFFSET=" << CONFIG_SPACE_COLOUR_PALETTE_OFFSET << endl;
    config_stream << "IS_SPACE_SCALED=" << CONFIG_SPACE_IS_SPACE_SCALED << endl;
    config_stream << "SCALED_SPACE_HEIGHT=" << CONFIG_SPACE_SCALED_SPACE_HEIGHT << endl;
    config_stream << "ENABLE_ENHANCED_PLAYER_ROTATION_MATRIX=" << CONFIG_SPACE_ENABLE_ENHANCED_PLAYER_ROTATION_MATRIX << endl;
    //config_stream << "ENABLE_REPLAY_LAST_MISSION=" << CONFIG_SPACE_ENABLE_REPLAY_LAST_MISSION << endl;

    config_stream << endl << "[MOVIES]" << endl;
    config_stream << "SHOW_ORIGINAL_MOVIES_INTERLACED=" << CONFIG_MOVIES_SHOW_ORIGINAL_INTERLACED << endl;
    config_stream << "ENABLE_ORIGINAL_MOVIES_LIMITED_SCALING=" << CONFIG_MOVIES_ENABLE_ORIGINAL_MOVIES_LIMITED_SCALING << endl;
    config_stream << "ENABLE_ORIGINAL_MOVIES_LINEAR_UPSCALING=" << CONFIG_MOVIES_ENABLE_ORIGINAL_MOVIES_LINEAR_UPSCALING << endl;

    config_stream << "PATH=" << CONFIG_MOVIES_PATH << endl;
    config_stream << "EXT=" << CONFIG_MOVIES_EXT << endl;
    config_stream << "BRANCH_OFFSET_MS=" << CONFIG_MOVIES_BRANCH_OFFSET_MS << endl;

    config_stream << endl << "[MOUSE]" << endl;
    config_stream << "DEAD_ZONE=" << CONFIG_MOUSE_DEAD_ZONE << endl;
    config_stream << "AXIS_LIMIT_PERCENTAGE=" << CONFIG_MOUSE_AXIS_LIMIT_PERCENTAGE << endl;
    config_stream << "INVERT_Y_AXIS=" << CONFIG_MOUSE_INVERT_Y_AXIS << endl;

    for (int i = 0; i < NUM_JOY_PROFILES; i++) {
        switch (i) {
        case static_cast<int>(PROFILE_TYPE::GUI):
            config_stream << endl << "[MOUSE_GUI]" << endl;
            config_stream << "BUTTON_01=" << static_cast<int>(WC3_ACTIONS::B1_Select) << endl;
            config_stream << "BUTTON_02=" << static_cast<int>(WC3_ACTIONS::B2_Cycle_Hotspots) << endl;
            break;
        case static_cast<int>(PROFILE_TYPE::NAV):
            config_stream << endl << "[MOUSE_NAV]" << endl;
            config_stream << "BUTTON_01=" << static_cast<int>(WC3_ACTIONS::NAV_Cycle_Points) << endl;
            config_stream << "BUTTON_02=" << static_cast<int>(WC3_ACTIONS::B2_Modifier) << endl;
            break;
        case static_cast<int>(PROFILE_TYPE::Space):
            config_stream << endl << "[MOUSE_SPACE]" << endl;
            config_stream << "BUTTON_01=" << static_cast<int>(WC3_ACTIONS::B1_Fire_Guns) << endl;
            config_stream << "BUTTON_02=" << static_cast<int>(WC3_ACTIONS::B2_Modifier) << endl;
            break;
        case static_cast<int>(PROFILE_TYPE::ReMap_1):
        case static_cast<int>(PROFILE_TYPE::ReMap_2):
        case static_cast<int>(PROFILE_TYPE::ReMap_3):
            config_stream << endl << "[MOUSE_REMAP_" << setfill(L'0') << setw(2) << i - static_cast<int>(PROFILE_TYPE::ReMap_1) + 1 << "]" << endl;
            config_stream << "BUTTON_01=" << static_cast<int>(WC3_ACTIONS::None) << endl;
            config_stream << "BUTTON_02=" << static_cast<int>(WC3_ACTIONS::None) << endl;
            break;
        default:
            continue;
        }
        config_stream << "BUTTON_03=" << static_cast<int>(WC3_ACTIONS::None) << endl;
        config_stream << "BUTTON_04=" << static_cast<int>(WC3_ACTIONS::None) << endl;
        config_stream << "BUTTON_05=" << static_cast<int>(WC3_ACTIONS::None) << endl;
        config_stream << "MOUSE_WHEEL_UP=" << static_cast<int>(WC3_ACTIONS::None) << endl;
        config_stream << "MOUSE_WHEEL_DOWN=" << static_cast<int>(WC3_ACTIONS::None) << endl;
        config_stream << "MOUSE_WHEEL_LEFT=" << static_cast<int>(WC3_ACTIONS::None) << endl;
        config_stream << "MOUSE_WHEEL_RIGHT=" << static_cast<int>(WC3_ACTIONS::None) << endl;
    }

    config_stream << endl << "[DEBUG]" << endl;
    //config_stream << "ERRORS=" << 1 << endl;
    config_stream << "GENERAL=" << 0 << endl;
    config_stream << "FLIGHT=" << 0 << endl;
    config_stream << "MOVIE=" << 0 << endl;
    config_stream << "MUSIC=" << 0 << endl;
    config_stream << "CONTROLLER=" << 0 << endl;
    config_stream << "REPLACE_ALT_X_MSG_WITH_ROOM_SCENE_ID=" << CONFIG_DEBUG_REPLACE_ALT_X_MSG_WITH_ROOM_SCENE_ID << endl;

    config_stream << endl;
    config_stream.close();

    return TRUE;
}


//__________________________________
static BOOL Build_Alt_Music_Config() {
    ofstream config_stream;

    config_stream.open(Get_Alt_Music_ConfigPath());
    if (!config_stream.is_open())
        return FALSE;


    config_stream << "; " << "Edit this file to setup alternative tunes to play in game." << endl;
    config_stream << "; " << "\"[xx]\" the original track number." << endl;
    config_stream << "; " << "\"path=\" the path to the track you wish to play in place of the original." << endl;
    config_stream << "; " << "\"max_volume=\" here you can adjust how loudly it plays to better match other in game sounds.Range 0 - 100, default(100)." << endl << endl;
    
    const char* file_names[] = {
        "",
        "mission1",
        "mission2",
        "mission3",
        "mission4",
        "combat1",
        "combat3",
        "combat4",
        "onradar",
        "nofile",
        "majortra",
        "majorvic",
        "debriefu",
        "debriefs",
        "eject",
        "blownup",
        "barracks",
        "bridge",
        "flightco",
        "flightde",
        "lift",
        "recroom1",
    };

    const char* orignal_file_name_list[] = {
        file_names[1],
        file_names[4],
        file_names[2],
        file_names[3],
        file_names[4],
        file_names[2],

        file_names[8],

        file_names[5],
        file_names[6],
        file_names[7],

        file_names[9],

        file_names[10],
        file_names[10],
        file_names[11],
        file_names[11],

        file_names[12],
        file_names[13],

        file_names[14],
        file_names[15],

        file_names[0],
        file_names[0],
        file_names[0],
        file_names[0],
        file_names[0],

        file_names[16],
        file_names[17],
        file_names[18],
        file_names[19],
        file_names[20],
        file_names[21],

        file_names[0],
        file_names[0],
        file_names[0],
        file_names[0],
    };

    
    const char* discriptions[] = {
        "Mission 1",
        "Mission 2",
        "Mission 3",
        "Mission 4",
        "Next To Last Mission",
        "Finale Mission",
        "Enemy Sighted",
        "Combat No Damage",
        "Combat Damaged",
        "Combat DeathBed",
        "Fire Missile",
        "Minor Tragety",
        "Major Tragety",
        "Minor Victory",
        "Major Victory",
        "Debrief Unsuccessful",
        "Debrief Successful",
        "Ejection",
        "Dirge",
        "",
        "",
        "",
        "",
        "",
        "Berths",
        "Bridge",
        "Flight Control",
        "Flight Deck",
        "Lift",
        "Rec Room",
        "",
        "",
        "",
        ""
    };

    for (int i = 0; i < 34; i++) {
        //write section name.
        config_stream << "[" << setfill('0') << setw(2) << i << "]";
        
        //write tune comments if any.
        if (orignal_file_name_list[i][0] || discriptions[i][0])
            config_stream << " ;";
         config_stream << discriptions[i];

        if (orignal_file_name_list[i][0])
            config_stream << " \"" << orignal_file_name_list[i]<< ".wav" << "\"";
        config_stream << endl;

        //for Rec Room, write different paths depending on the CD number.
        if (i == 29) {
            for (int cd = 2; cd < 6; cd++)
                config_stream << "path_cd" << cd << "=" << endl;
        }
        else
            config_stream << "path=" << endl;

        config_stream << "max_volume=100" << endl;
    }

    config_stream.close();
    return TRUE;
}


//_____________________________
static BOOL Is_Config_Updated() {

    wchar_t wchar_buff[_countof(VER_FILE_VERSION_STR)];
    ConfigReadString(L"MAIN", L"VERSION", L"", wchar_buff, MAX_PATH);
    if (wcsncmp(wchar_buff, VER_FILE_VERSION_STR, _countof(VER_FILE_VERSION_STR)) == 0)
        return TRUE;
    return FALSE;
}


//_________________________
static BOOL Update_Config() {

    if (Is_Config_Updated()) 
        return TRUE;
    
    wchar_t* wchar_buff = new wchar_t[MAX_PATH] {0};
    UINT val = 0;

    ConfigWriteString(L"MAIN", L"VERSION", VER_FILE_VERSION_STR);

    val = ConfigReadInt(L"MAIN", L"UAC_AWARE", CONFIG_MAIN_UAC_AWARE);
    ConfigWriteInt(L"MAIN", L"UAC_AWARE", val);

    val = ConfigReadInt(L"MAIN", L"ENABLE_DPI_AWARENESS", CONFIG_MAIN_ENABLE_DPI_AWARENESS);
    ConfigWriteInt(L"MAIN", L"ENABLE_DPI_AWARENESS", val);

    val = ConfigReadInt(L"MAIN", L"ENABLE_CONTROLLER_ENHANCEMENTS", CONFIG_MAIN_ENABLE_CONTROLLER_ENHANCEMENTS);
    ConfigWriteInt(L"MAIN", L"ENABLE_CONTROLLER_ENHANCEMENTS", val);
    val = ConfigReadInt(L"MAIN", L"ENABLE_MUSIC_ENHANCEMENTS", CONFIG_MAIN_ENABLE_MUSIC_ENHANCEMENTS);
    ConfigWriteInt(L"MAIN", L"ENABLE_MUSIC_ENHANCEMENTS", val);

    val = ConfigReadInt(L"MAIN", L"ENABLE_LINEAR_UPSCALING_GUI", CONFIG_MAIN_ENABLE_LINEAR_UPSCALING_GUI);
    ConfigWriteInt(L"MAIN", L"ENABLE_LINEAR_UPSCALING_GUI", val);
    val = ConfigReadInt(L"MAIN", L"ENABLE_LINEAR_UPSCALING_COCKPIT_HUD", CONFIG_MAIN_ENABLE_LINEAR_UPSCALING_COCKPIT_HUD);
    ConfigWriteInt(L"MAIN", L"ENABLE_LINEAR_UPSCALING_COCKPIT_HUD", val);

    val = ConfigReadInt(L"MAIN", L"VIRTUAL_MEM_SIZE", CONFIG_MAIN_VIRTUAL_MEM_SIZE);
    ConfigWriteInt(L"MAIN", L"VIRTUAL_MEM_SIZE", val);
    val = ConfigReadInt(L"MAIN", L"NUM_WATCHERS_OVERIDE", CONFIG_MAIN_NUM_WATCHERS_OVERIDE);
    ConfigWriteInt(L"MAIN", L"NUM_WATCHERS_OVERIDE", val);

    val = ConfigReadInt(L"MAIN", L"WINDOWED", CONFIG_MAIN_WINDOWED);
    ConfigWriteInt(L"MAIN", L"WINDOWED", val);
    val = ConfigReadInt(L"MAIN", L"DEAD_ZONE", CONFIG_MAIN_DEAD_ZONE);
    ConfigWriteInt(L"MAIN", L"DEAD_ZONE", val);

    val = ConfigReadInt(L"MAIN", L"ENABLE_SUBTITLES", CONFIG_MAIN_ENABLE_SUBTITLES);
    ConfigWriteInt(L"MAIN", L"ENABLE_SUBTITLES", val);
    val = ConfigReadInt(L"MAIN", L"LANGUAGE_REF", CONFIG_MAIN_LANGUAGE_REF);
    ConfigWriteInt(L"MAIN", L"LANGUAGE_REF", val);

    val = ConfigReadInt(L"MAIN", L"ENABLE_ESC_KEY_TO_EXIT_GAME", CONFIG_MAIN_ENABLE_ESC_KEY_TO_EXIT_GAME);
    ConfigWriteInt(L"MAIN", L"ENABLE_ESC_KEY_TO_EXIT_GAME", val);


    val = ConfigReadInt(L"SPACE", L"COCKPIT_MAINTAIN_ASPECT_RATIO", CONFIG_SPACE_COCKPIT_MAINTAIN_ASPECT_RATIO);
    ConfigWriteInt(L"SPACE", L"COCKPIT_MAINTAIN_ASPECT_RATIO", val);
    val = ConfigReadInt(L"SPACE", L"SPACE_REFRESH_RATE_HZ", CONFIG_SPACE_SPACE_REFRESH_RATE_HZ);
    ConfigWriteInt(L"SPACE", L"SPACE_REFRESH_RATE_HZ", val);
    val = ConfigReadInt(L"SPACE", L"NAV_SCREEN_KEY_RESPONSE_HZ", CONFIG_SPACE_NAV_SCREEN_KEY_RESPONSE_HZ);
    ConfigWriteInt(L"SPACE", L"NAV_SCREEN_KEY_RESPONSE_HZ", val);
    val = ConfigReadInt(L"SPACE", L"LOD_LEVEL_DISTANCE_MODIFIER", CONFIG_SPACE_LOD_LEVEL_DISTANCE_MODIFIER);
    ConfigWriteInt(L"SPACE", L"LOD_LEVEL_DISTANCE_MODIFIER", val);
    val = ConfigReadInt(L"SPACE", L"ENABLE_ENHANCED_PLAYER_ROTATION_MATRIX", CONFIG_SPACE_ENABLE_ENHANCED_PLAYER_ROTATION_MATRIX);
    ConfigWriteInt(L"SPACE", L"ENABLE_ENHANCED_PLAYER_ROTATION_MATRIX", val);
    val = ConfigReadInt(L"SPACE", L"ENABLE_SPACE_COLOUR_CHANGE", CONFIG_SPACE_ENABLE_SPACE_COLOUR_CHANGE);
    ConfigWriteInt(L"SPACE", L"ENABLE_SPACE_COLOUR_CHANGE", val);
    val = ConfigReadInt(L"SPACE", L"SPACE_COLOUR_PALETTE_OFFSET", CONFIG_SPACE_COLOUR_PALETTE_OFFSET);
    ConfigWriteInt(L"SPACE", L"SPACE_COLOUR_PALETTE_OFFSET", val);
    val = ConfigReadInt(L"SPACE", L"USE_ORIGINAL_ASPECT_RATIO", CONFIG_SPACE_USE_ORIGINAL_ASPECT_RATIO);
    ConfigWriteInt(L"SPACE", L"USE_ORIGINAL_ASPECT_RATIO", val);
    val = ConfigReadInt(L"SPACE", L"IS_SPACE_SCALED", CONFIG_SPACE_IS_SPACE_SCALED);
    ConfigWriteInt(L"SPACE", L"IS_SPACE_SCALED", val);
    val = ConfigReadInt(L"SPACE", L"SCALED_SPACE_HEIGHT", CONFIG_SPACE_SCALED_SPACE_HEIGHT);
    ConfigWriteInt(L"SPACE", L"SCALED_SPACE_HEIGHT", val);
    //val = ConfigReadInt(L"SPACE", L"ENABLE_REPLAY_LAST_MISSION", CONFIG_SPACE_ENABLE_REPLAY_LAST_MISSION);
    //ConfigWriteInt(L"SPACE", L"ENABLE_REPLAY_LAST_MISSION", val);


    val = ConfigReadInt(L"MOVIES", L"SHOW_ORIGINAL_MOVIES_INTERLACED", CONFIG_MOVIES_SHOW_ORIGINAL_INTERLACED);
    ConfigWriteInt(L"MOVIES", L"SHOW_ORIGINAL_MOVIES_INTERLACED", val);
    val = ConfigReadInt(L"MOVIES", L"ENABLE_ORIGINAL_MOVIES_LIMITED_SCALING", CONFIG_MOVIES_ENABLE_ORIGINAL_MOVIES_LIMITED_SCALING);
    ConfigWriteInt(L"MOVIES", L"ENABLE_ORIGINAL_MOVIES_LIMITED_SCALING", val);
    val = ConfigReadInt(L"MOVIES", L"ENABLE_ORIGINAL_MOVIES_LINEAR_UPSCALING", CONFIG_MOVIES_ENABLE_ORIGINAL_MOVIES_LINEAR_UPSCALING);
    ConfigWriteInt(L"MOVIES", L"ENABLE_ORIGINAL_MOVIES_LINEAR_UPSCALING", val);


    ConfigReadString(L"MOVIES", L"PATH", CONFIG_MOVIES_PATH, wchar_buff, MAX_PATH);
    ConfigWriteString(L"MOVIES", L"PATH", wchar_buff);
    ConfigReadString(L"MOVIES", L"EXT", CONFIG_MOVIES_EXT, wchar_buff, MAX_PATH);
    ConfigWriteString(L"MOVIES", L"EXT", wchar_buff);

    val = ConfigReadInt(L"MOVIES", L"BRANCH_OFFSET_MS", CONFIG_MOVIES_BRANCH_OFFSET_MS);
    ConfigWriteInt(L"MOVIES", L"BRANCH_OFFSET_MS", val);


    val = ConfigReadInt(L"MOUSE", L"DEAD_ZONE", CONFIG_MOUSE_DEAD_ZONE);
    ConfigWriteInt(L"MOUSE", L"DEAD_ZONE", val);
    val = ConfigReadInt(L"MOUSE", L"AXIS_LIMIT_PERCENTAGE", CONFIG_MOUSE_AXIS_LIMIT_PERCENTAGE);
    ConfigWriteInt(L"MOUSE", L"AXIS_LIMIT_PERCENTAGE", val);
    val = ConfigReadInt(L"MOUSE", L"INVERT_Y_AXIS", CONFIG_MOUSE_INVERT_Y_AXIS);
    ConfigWriteInt(L"MOUSE", L"INVERT_Y_AXIS", val);

    wchar_t profile_name[16];
    for (int i = 0; i < NUM_JOY_PROFILES; i++) {
        switch (i) {
        case static_cast<int>(PROFILE_TYPE::GUI):
            swprintf(profile_name, _countof(profile_name), L"MOUSE_GUI");
            val = ConfigReadInt(profile_name, L"BUTTON_01", static_cast<int>(WC3_ACTIONS::B1_Select));
            ConfigWriteInt(profile_name, L"BUTTON_01", val);
            val = ConfigReadInt(profile_name, L"BUTTON_02", static_cast<int>(WC3_ACTIONS::B2_Cycle_Hotspots));
            ConfigWriteInt(profile_name, L"BUTTON_02", val);
            break;
        case static_cast<int>(PROFILE_TYPE::NAV):
            swprintf(profile_name, _countof(profile_name), L"MOUSE_NAV");
            val = ConfigReadInt(profile_name, L"BUTTON_01", static_cast<int>(WC3_ACTIONS::NAV_Cycle_Points));
            ConfigWriteInt(profile_name, L"BUTTON_01", static_cast<int>(WC3_ACTIONS::NAV_Cycle_Points));
            val = ConfigReadInt(profile_name, L"BUTTON_02", static_cast<int>(WC3_ACTIONS::B2_Modifier));
            ConfigWriteInt(profile_name, L"BUTTON_02", val);
            break;
        case static_cast<int>(PROFILE_TYPE::Space):
            swprintf(profile_name, _countof(profile_name), L"MOUSE_SPACE");
            val = ConfigReadInt(profile_name, L"BUTTON_01", static_cast<int>(WC3_ACTIONS::B1_Fire_Guns));
            ConfigWriteInt(profile_name, L"BUTTON_01",val);
            val = ConfigReadInt(profile_name, L"BUTTON_02", static_cast<int>(WC3_ACTIONS::B2_Modifier));
            ConfigWriteInt(profile_name, L"BUTTON_02", val);
            break;
        case static_cast<int>(PROFILE_TYPE::ReMap_1):
        case static_cast<int>(PROFILE_TYPE::ReMap_2):
        case static_cast<int>(PROFILE_TYPE::ReMap_3):
            swprintf(profile_name, _countof(profile_name), L"MOUSE_REMAP_%02d", i - static_cast<int>(PROFILE_TYPE::ReMap_1) + 1);
            val = ConfigReadInt(profile_name, L"BUTTON_01", static_cast<int>(WC3_ACTIONS::None));
            ConfigWriteInt(profile_name, L"BUTTON_01", val);
            val = ConfigReadInt(profile_name, L"BUTTON_02", static_cast<int>(WC3_ACTIONS::None));
            ConfigWriteInt(profile_name, L"BUTTON_02", val);
            break;
        default:
            continue;
        }
        val = ConfigReadInt(profile_name, L"BUTTON_03", static_cast<int>(WC3_ACTIONS::None));
        ConfigWriteInt(profile_name, L"BUTTON_03", val);
        val = ConfigReadInt(profile_name, L"BUTTON_04", static_cast<int>(WC3_ACTIONS::None));
        ConfigWriteInt(profile_name, L"BUTTON_04", val);
        val = ConfigReadInt(profile_name, L"BUTTON_05", static_cast<int>(WC3_ACTIONS::None));
        ConfigWriteInt(profile_name, L"BUTTON_05", val);
        val = ConfigReadInt(profile_name, L"MOUSE_WHEEL_UP", static_cast<int>(WC3_ACTIONS::None));
        ConfigWriteInt(profile_name, L"MOUSE_WHEEL_UP", val);
        val = ConfigReadInt(profile_name, L"MOUSE_WHEEL_DOWN", static_cast<int>(WC3_ACTIONS::None));
        ConfigWriteInt(profile_name, L"MOUSE_WHEEL_DOWN", val);
        val = ConfigReadInt(profile_name, L"MOUSE_WHEEL_LEFT", static_cast<int>(WC3_ACTIONS::None));
        ConfigWriteInt(profile_name, L"MOUSE_WHEEL_LEFT", val);
        val = ConfigReadInt(profile_name, L"MOUSE_WHEEL_RIGHT", static_cast<int>(WC3_ACTIONS::None));
        ConfigWriteInt(profile_name, L"MOUSE_WHEEL_RIGHT", val);
    }


    //ConfigWriteInt(L"DEBUG", L"ERRORS", 1);
    val = ConfigReadInt(L"DEBUG", L"GENERAL", 0);
    ConfigWriteInt(L"DEBUG", L"GENERAL", val);
    val = ConfigReadInt(L"DEBUG", L"FLIGHT", 0);
    ConfigWriteInt(L"DEBUG", L"FLIGHT", val);
    val = ConfigReadInt(L"DEBUG", L"MOVIE", 0);
    ConfigWriteInt(L"DEBUG", L"MOVIE", val);
    val = ConfigReadInt(L"DEBUG", L"MUSIC", 0);
    ConfigWriteInt(L"DEBUG", L"MUSIC", val);
    val = ConfigReadInt(L"DEBUG", L"CONTROLLER", 0);
    ConfigWriteInt(L"DEBUG", L"CONTROLLER", val);
    val = ConfigReadInt(L"DEBUG", L"REPLACE_ALT_X_MSG_WITH_ROOM_SCENE_ID", CONFIG_DEBUG_REPLACE_ALT_X_MSG_WITH_ROOM_SCENE_ID);
    ConfigWriteInt(L"DEBUG", L"REPLACE_ALT_X_MSG_WITH_ROOM_SCENE_ID", val);

    delete[]wchar_buff;
    return TRUE;
}


//______________________________
static void Admin_Build_Config() {
    
    wchar_t exeName[MAX_PATH];
    GetModuleFileName(nullptr, exeName, MAX_PATH);

    SHELLEXECUTEINFO shExInfo = { 0 };
    shExInfo.cbSize = sizeof(shExInfo);
    shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    shExInfo.hwnd = 0;
    shExInfo.lpVerb = L"runas";                 // Launch as Administrator, User Account Control (UAC) will prompt the user for consent.
    shExInfo.lpFile = exeName;
    shExInfo.lpParameters = L"/build_config";
    shExInfo.lpDirectory = 0;
    shExInfo.nShow = SW_SHOW;
    shExInfo.hInstApp = 0;

    Debug_Info_Error("Upgrading UAC privileges to Build/Update config files for: %S.", exeName);

    if (ShellExecuteEx(&shExInfo) && shExInfo.hProcess != nullptr) {
        WaitForSingleObject(shExInfo.hProcess, INFINITE);
        CloseHandle(shExInfo.hProcess);
    }
}


//_______________________________
static BOOL IsAppInProgramFiles() {

    static BOOL is_app_in_programfiles = FALSE;
    static bool app_in_programfiles_run_once = false;
    if (app_in_programfiles_run_once)
        return is_app_in_programfiles;

    wstring current_path = GetAppPath();

    wchar_t* p_program_files_path = nullptr;
    SHGetKnownFolderPath(FOLDERID_ProgramFiles, SHGFP_TYPE_CURRENT, nullptr, &p_program_files_path);

    size_t loc = current_path.find(p_program_files_path);
    CoTaskMemFree(p_program_files_path);

    if (loc == string::npos)
        is_app_in_programfiles = FALSE;
    else
        is_app_in_programfiles = TRUE;

    app_in_programfiles_run_once = true;
    return is_app_in_programfiles;
}


//_________________________
const wchar_t* GetAppPath() {
    static wstring wAppPath;
    static bool app_path_set = false;

    if (!app_path_set) {
        DWORD currentDirSize = GetCurrentDirectory(0, nullptr);//get the path size
        wchar_t* dirCurrent = new wchar_t[currentDirSize];
        GetCurrentDirectory(currentDirSize, dirCurrent);
        wAppPath.assign(L"\\\\?\\");
        wAppPath.append(dirCurrent);
        delete[] dirCurrent;
        app_path_set = true;
    }
    return wAppPath.c_str();
}


//_____________________________
const wchar_t* GetAppDataPath() {
    static wstring wAppDataPath;
    static bool app_data_path_set = false;

    if (!app_data_path_set) {
        //This setting is only valid in the wc3w_en.ini file located in the wc3 app folder.
        wstring local_ini = GetAppPath();
        local_ini.append(L"\\");
        local_ini.append(VER_PRODUCTNAME_STR);
        local_ini.append(L".ini");
        LONG UAC_AWARE = GetPrivateProfileInt(L"MAIN", L"UAC_AWARE", CONFIG_MAIN_UAC_AWARE, local_ini.c_str());

        if (UAC_AWARE == 2 || (UAC_AWARE == 1 && IsAppInProgramFiles())) {
            wchar_t* pRoamingAppData = nullptr;
            SHGetKnownFolderPath(FOLDERID_RoamingAppData, SHGFP_TYPE_CURRENT, nullptr, &pRoamingAppData);
            wAppDataPath.assign(L"\\\\?\\");// to allow for strings longer than MAX_PATH

            wAppDataPath.append(pRoamingAppData);
            CoTaskMemFree(pRoamingAppData);

            wAppDataPath.append(L"\\");
            wAppDataPath.append(VER_PRODUCTNAME_STR);

            if (GetFileAttributes(wAppDataPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
                if (!CreateDirectory(wAppDataPath.c_str(), nullptr)) {
                    wAppDataPath.clear();
                    //Debug_Info_Error("AppDataPath: folder creation FAILED!: %S", wAppDataPath.c_str());
                }
            }
        }
        else
            wAppDataPath.clear();
        app_data_path_set = true;
        //Debug_Info("AppDataPath: %S", wAppDataPath.c_str());
    }

    return wAppDataPath.c_str();
}


//_______________________________
static void ConfigCreate_InGame() {

    //ConfigWriteInt_InGame(L"MAIN", L"WINDOWED", CONFIG_MAIN_WINDOWED);
    ConfigWriteInt_InGame(L"MAIN", L"WIN_DATA", CONFIG_MAIN_WIN_DATA);

    //ConfigWriteInt_InGame(L"MAIN", L"DEAD_ZONE", CONFIG_MAIN_DEAD_ZONE);
    ConfigWriteInt_InGame(L"MAIN", L"GAMMA_LEVEL", CONFIG_MAIN_GAMMA_LEVEL);
}


//___________________________________________
static const wchar_t* Get_ConfigPath_InGame() {

    static bool config_path_set = false;
    static wstring wConfigPath;
    if (!config_path_set) {
        config_path_set = true;

        wConfigPath.assign(GetAppDataPath());
        bool is_app_folder = false;
        if (wConfigPath.empty()) {
            is_app_folder = true;
            wConfigPath.assign(GetAppPath());
        }

        wConfigPath.append(L"\\");
        wConfigPath.append(VER_PRODUCTNAME_STR);
        wConfigPath.append(L"_ingame");
        wConfigPath.append(L".ini");

        if (GetFileAttributes(wConfigPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
            if (!is_app_folder) { //if no ini file exists on the UAC path, first attempt to copy the ini from the local path.
                wstring local_ini = GetAppPath();
                local_ini.append(L"\\");
                local_ini.append(VER_PRODUCTNAME_STR);
                local_ini.append(L"_ingame");
                local_ini.append(L".ini");
                if (GetFileAttributes(local_ini.c_str()) != INVALID_FILE_ATTRIBUTES && CopyFile(local_ini.c_str(), wConfigPath.c_str(), TRUE))
                    return wConfigPath.c_str();
            }
            ConfigCreate_InGame();
        }
    }
    return wConfigPath.c_str();
}


//____________________________________
static const wchar_t* Get_ConfigPath() {

    static bool config_path_set = false;
    static wstring wConfigPath;
    if (!config_path_set) {
        config_path_set = true;

        wConfigPath.assign(GetAppPath());

        wConfigPath.append(L"\\");
        wConfigPath.append(VER_PRODUCTNAME_STR);
        wConfigPath.append(L".ini");
    }
    return wConfigPath.c_str();
}


//____________________________________
static void Check_Config_Files_Exist() {

    if (GetFileAttributes(Get_ConfigPath()) == INVALID_FILE_ATTRIBUTES) {
        Debug_Info_Error("Building: %S.ini.", VER_PRODUCTNAME_STR);
        if (IsAppInProgramFiles())
            Admin_Build_Config();
        else
            Build_Config();
    }
    else if (!Is_Config_Updated()) {
        Debug_Info_Error("Updating: %S.ini", VER_PRODUCTNAME_STR);
        if (IsAppInProgramFiles())
            Admin_Build_Config();
        else
            Update_Config();
    }
    if (GetFileAttributesA(Get_Alt_Music_ConfigPath()) == INVALID_FILE_ATTRIBUTES) {
        Debug_Info_Error("Building: %S_alt_music.ini.", VER_PRODUCTNAME_STR);
        if (IsAppInProgramFiles())
            Admin_Build_Config();
        else
            Build_Alt_Music_Config();
    }
}


//_________________________________
void Check_Command_Line_Overrides() {

    LPWSTR* szArglist;
    int nArgs;

    szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    if (szArglist == nullptr) {
        Debug_Info_Error("CommandLineToArgvW failed.");
        return;
    }
    
    for (int i = 1; i < nArgs; i++) {
        //Debug_Info("%d: %S", i, szArglist[i]);
        if (wcsncmp(szArglist[i], L"/movies_path=", 13) == 0)
            movies_path_override.assign(szArglist[i] + 13);
        else if (wcsncmp(szArglist[i], L"/movies_ext=", 12) == 0)
            movies_ext_override.assign(szArglist[i] + 12);
        else if (wcsncmp(szArglist[i], L"/build_config", 13) == 0) {
            if (GetFileAttributes(Get_ConfigPath()) == INVALID_FILE_ATTRIBUTES)
                Build_Config();
            else
                Update_Config();
            if (GetFileAttributesA(Get_Alt_Music_ConfigPath()) == INVALID_FILE_ATTRIBUTES)
                Build_Alt_Music_Config();
            wc3_exit_game(0);
        }
    }
    LocalFree(szArglist);

    Check_Config_Files_Exist();
}


//_______________________
void ConfigRefreshCache() {
    WritePrivateProfileString(nullptr, nullptr, nullptr, Get_ConfigPath());
}


//__________________________________________________________________________________________________________________________________________
DWORD ConfigReadString(const wchar_t* lpAppName, const wchar_t* lpKeyName, const wchar_t* lpDefault, wchar_t* lpReturnedString, DWORD nSize) {
    return GetPrivateProfileString(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, Get_ConfigPath());
}


//_________________________________________________________________________________________________
BOOL ConfigWriteString(const wchar_t* lpAppName, const wchar_t* lpKeyName, const wchar_t* lpString) {
    return WritePrivateProfileString(lpAppName, lpKeyName, lpString, Get_ConfigPath());
}


//__________________________________________________________________________________
UINT ConfigReadInt(const wchar_t* lpAppName, const wchar_t* lpKeyName, int nDefault) {
    return GetPrivateProfileInt(lpAppName, lpKeyName, nDefault, Get_ConfigPath());
}


//_________________________________________________________________________________________
UINT ConfigReadInt_InGame(const wchar_t* lpAppName, const wchar_t* lpKeyName, int nDefault) {
    
    nDefault = ConfigReadInt(lpAppName, lpKeyName, nDefault);// check for defaults in main ini file;
    return GetPrivateProfileInt(lpAppName, lpKeyName, nDefault, Get_ConfigPath_InGame());
}


//_________________________________________________________________________________
BOOL ConfigWriteInt(const wchar_t* lpAppName, const wchar_t* lpKeyName, int intVal) {
    wchar_t lpString[64]{0};
    swprintf_s(lpString, 64, L"%d", intVal);
    return WritePrivateProfileString(lpAppName, lpKeyName, lpString, Get_ConfigPath());
}


//________________________________________________________________________________________
BOOL ConfigWriteInt_InGame(const wchar_t* lpAppName, const wchar_t* lpKeyName, int intVal) {
    wchar_t lpString[64]{ 0 };
    swprintf_s(lpString, 64, L"%d", intVal);
    return WritePrivateProfileString(lpAppName, lpKeyName, lpString, Get_ConfigPath_InGame());
}


//___________________________________________________________________________________________________
BOOL ConfigReadWinData(const wchar_t* lpAppName, const wchar_t* lpKeyName, WINDOWPLACEMENT* pWinData) {
    pWinData->length = sizeof(WINDOWPLACEMENT);
    return GetPrivateProfileStruct(lpAppName, lpKeyName, pWinData, sizeof(WINDOWPLACEMENT), Get_ConfigPath_InGame());
}


//____________________________________________________________________________________________________
BOOL ConfigWriteWinData(const wchar_t* lpAppName, const wchar_t* lpKeyName, WINDOWPLACEMENT* pWinData) {
    return WritePrivateProfileStruct(lpAppName, lpKeyName, pWinData, sizeof(WINDOWPLACEMENT), Get_ConfigPath_InGame());
}


//__________________________________________________________________________________________________________
BOOL ConfigReadStruct(const wchar_t* lpszSection, const wchar_t* lpszKey, LPVOID lpStruct, UINT uSizeStruct) {
    return GetPrivateProfileStruct(lpszSection, lpszKey, lpStruct, uSizeStruct, Get_ConfigPath());
}


//___________________________________________________________________________________________________________
BOOL ConfigWriteStruct(const wchar_t* lpszSection, const wchar_t* lpszKey, LPVOID lpStruct, UINT uSizeStruct) {
    return WritePrivateProfileStruct(lpszSection, lpszKey, lpStruct, uSizeStruct, Get_ConfigPath());
}
