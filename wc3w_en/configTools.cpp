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
#include "configTools.h"
#include "version.h"

using namespace std;


//_______________________________
static BOOL IsAppInProgramFiles() {

    static BOOL is_app_in_programfiles = FALSE;
    static bool app_in_programfiles_run_once = false;
    if(app_in_programfiles_run_once)
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

    ConfigWriteInt_InGame(L"MAIN", L"WINDOWED", CONFIG_MAIN_WINDOWED);
    ConfigWriteInt_InGame(L"MAIN", L"WIN_DATA", CONFIG_MAIN_WIN_DATA);

    ConfigWriteInt_InGame(L"MAIN", L"DEAD_ZONE", CONFIG_MAIN_DEAD_ZONE);
    ConfigWriteInt_InGame(L"MAIN", L"GAMMA_LEVEL", CONFIG_MAIN_GAMMA_LEVEL);
}


//________________________
static void ConfigCreate() {
    
    ConfigWriteInt(L"MAIN", L"UAC_AWARE", CONFIG_MAIN_UAC_AWARE);

    ConfigWriteInt(L"MAIN", L"ENABLE_CONTROLLER_ENHANCEMENTS", CONFIG_MAIN_ENABLE_CONTROLLER_ENHANCEMENTS);

    ConfigWriteInt(L"MAIN", L"ENABLE_LINEAR_UPSCALING_GUI", CONFIG_MAIN_ENABLE_LINEAR_UPSCALING_GUI);
    ConfigWriteInt(L"MAIN", L"ENABLE_LINEAR_UPSCALING_COCKPIT_HUD", CONFIG_MAIN_ENABLE_LINEAR_UPSCALING_COCKPIT_HUD);

    ConfigWriteInt(L"SPACE", L"COCKPIT_MAINTAIN_ASPECT_RATIO", CONFIG_SPACE_COCKPIT_MAINTAIN_ASPECT_RATIO);
    ConfigWriteInt(L"SPACE", L"SPACE_REFRESH_RATE_HZ", CONFIG_SPACE_SPACE_REFRESH_RATE_HZ);
    ConfigWriteInt(L"SPACE", L"NAV_SCREEN_KEY_RESPONSE_HZ", CONFIG_SPACE_NAV_SCREEN_KEY_RESPONSE_HZ);
    ConfigWriteInt(L"SPACE", L"LOD_LEVEL_DISTANCE_MODIFIER", CONFIG_SPACE_LOD_LEVEL_DISTANCE_MODIFIER);
    ConfigWriteInt(L"SPACE", L"ENABLE_ENHANCED_PLAYER_ROTATION_MATRIX", CONFIG_SPACE_ENABLE_ENHANCED_PLAYER_ROTATION_MATRIX);

    ConfigWriteInt(L"MOVIES", L"SHOW_ORIGINAL_MOVIES_INTERLACED", CONFIG_MOVIES_SHOW_ORIGINAL_INTERLACED);
    ConfigWriteString(L"MOVIES", L"PATH", CONFIG_MOVIES_PATH);
    ConfigWriteString(L"MOVIES", L"EXT", CONFIG_MOVIES_EXT);
    ConfigWriteInt(L"MOVIES", L"BRANCH_OFFSET_MS", CONFIG_MOVIES_BRANCH_OFFSET_MS);

    //ConfigWriteInt("MOVIES", "INFLIGHT_USE_AUDIO_FROM_FILE_IF_PRESENT", CONFIG_MOVIES_INFLIGHT_USE_AUDIO_FROM_FILE_IF_PRESENT);
    //ConfigWriteInt("MOVIES", "INFLIGHT_DISPLAY_ASPECT_TYPE", CONFIG_MOVIES_INFLIGHT_DISPLAY_ASPECT_TYPE);
    //ConfigWriteInt("MOVIES", "INFLIGHT_COCKPIT_BG_COLOUR_RGB", CONFIG_MOVIES_INFLIGHT_COCKPIT_BG_COLOUR_RGB);
    //ConfigWriteInt("MOVIES", "INFLIGHT_MONO_SHADER_ENABLE", CONFIG_INFLIGHT_MONO_SHADER_ENABLE);
    //ConfigWriteInt("MOVIES", "INFLIGHT_MONO_SHADER_COLOUR", CONFIG_INFLIGHT_MONO_SHADER_COLOUR);
    //ConfigWriteInt("MOVIES", "INFLIGHT_MONO_SHADER_BRIGHTNESS", CONFIG_INFLIGHT_MONO_SHADER_BRIGHTNESS);
    //ConfigWriteInt("MOVIES", "INFLIGHT_MONO_SHADER_CONTRAST", CONFIG_INFLIGHT_MONO_SHADER_CONTRAST);


    //ConfigWriteInt(L"DEBUG", L"ERRORS", 1);
    ConfigWriteInt(L"DEBUG", L"GENERAL", 0);
    ConfigWriteInt(L"DEBUG", L"FLIGHT", 0);
    ConfigWriteInt(L"DEBUG", L"MOVIE", 0);
    ConfigWriteInt(L"DEBUG", L"CONTROLLER", 0);
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

        if (GetFileAttributes(wConfigPath.c_str()) == INVALID_FILE_ATTRIBUTES) {

            ConfigCreate();
        }
    }
    return wConfigPath.c_str();
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
