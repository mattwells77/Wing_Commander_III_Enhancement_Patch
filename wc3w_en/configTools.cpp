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

char* pConfigPath = nullptr;


// _________________________________________________
static DWORD GetConfigPath_NonUAC(char* pReturnPath) {
    DWORD buffSize = GetCurrentDirectoryA(0, nullptr);
    buffSize += 13;
    if (!pReturnPath)
        return buffSize;

    if (GetCurrentDirectoryA(buffSize, pReturnPath))
        strncat_s(pReturnPath, buffSize, "\\wc3w_en.ini", 13);

    return buffSize;
}

/*
//____________________________________________
DWORD UAC_GetAppDataPath(char* appDataPath) {

    char* pRoamingAppData = nullptr;
    SHGetKnownFolderPath(FOLDERID_RoamingAppData, SHGFP_TYPE_CURRENT, nullptr, &pRoamingAppData);

    DWORD appDatPathSize = 0;
    DWORD appPathSize = 0;

    while (pRoamingAppData[appPathSize] != '\0')
        appPathSize++;

    DWORD currentDirSize = GetCurrentDirectory(0, nullptr);//get the path size
    char* dirCurrent = new char[currentDirSize];
    GetCurrentDirectory(currentDirSize, dirCurrent);


    BYTE bHash[16];//convert the game path to hash data
    HashData((BYTE*)dirCurrent, currentDirSize, bHash, 16);
    char bHashString[33];

    for (int i = 0; i < 16; ++i)//convert the hash data to a string, this will be a unique folder name to store the config data in.
        swprintf_s(&bHashString[i * 2], 33 - i * 2, L"%02x", bHash[i]);

    appDatPathSize = 4 + appPathSize + 9 + 1 + 33;
    if (!appDataPath) {
        CoTaskMemFree(pRoamingAppData);
        return appDatPathSize;
    }

    ZeroMemory(appDataPath, appDatPathSize);
    strncat_s(appDataPath, appDatPathSize, "\\\\?\\", appPathSize);//uniPrependSize // to allow for strings longer than MAX_PATH
    strncat_s(appDataPath, appDatPathSize, pRoamingAppData, appPathSize);//appDatPathSize
    CoTaskMemFree(pRoamingAppData);
    strncat_s(appDataPath, appDatPathSize, "\\Fallout2", 9);

    if (GetFileAttributes(appDataPath) == INVALID_FILE_ATTRIBUTES) {
        if (!CreateDirectory(appDataPath, nullptr)) {
            appDataPath[0] = L'\0';
            return 0;
        }
    }

    strncat_s(appDataPath, appDatPathSize, "\\", 1);//BackSlashSize
    strncat_s(appDataPath, appDatPathSize, bHashString, 33);//hexFolderSize
    delete[] dirCurrent;
    if (GetFileAttributes(appDataPath) == INVALID_FILE_ATTRIBUTES)
        CreateDirectory(appDataPath, nullptr);

    return appDatPathSize;
}
*/

//________________________
static void ConfigCreate() {

    //To-Do - Lets ditch the UAC stuff for now, not sure anyone was using it anyway.

    /*LONG UAC_AWARE = ConfigReadInt(L"MAIN", L"UAC_AWARE", 1);

    if (UAC_AWARE && IsWindowsVistaOrGreater()) {
        DWORD buffSize = GetCurrentDirectory(0, nullptr);
        char* pGameFolderPath = new char[buffSize];
        GetCurrentDirectory(buffSize, pGameFolderPath);
        //insert current path for visual identification.
        WritePrivateProfileString(L"LOCATION", L"path", pGameFolderPath, pConfigPath);
        delete[] pGameFolderPath;
        pGameFolderPath = nullptr;

    }*/


    ConfigWriteInt("MAIN", "WINDOWED", CONFIG_MAIN_WINDOWED);
    ConfigWriteInt("MAIN", "WIN_DATA", CONFIG_MAIN_WIN_DATA);

    ConfigWriteInt("SPACE", "COCKPIT_MAINTAIN_ASPECT_RATIO", CONFIG_SPACE_COCKPIT_MAINTAIN_ASPECT_RATIO);
    ConfigWriteInt("SPACE", "SPACE_REFRESH_RATE_HZ", CONFIG_SPACE_SPACE_REFRESH_RATE_HZ);
    ConfigWriteInt("SPACE", "NAV_SCREEN_KEY_RESPONCE_HZ", CONFIG_SPACE_NAV_SCREEN_KEY_RESPONCE_HZ);

    ConfigWriteString("MOVIES", "PATH", CONFIG_MOVIES_PATH);
    ConfigWriteString("MOVIES", "EXT", CONFIG_MOVIES_EXT);
    ConfigWriteInt("MOVIES", "BRANCH_OFFSET_MS", CONFIG_MOVIES_BRANCH_OFFSET_MS);

    //ConfigWriteInt("MOVIES", "INFLIGHT_USE_AUDIO_FROM_FILE_IF_PRESENT", CONFIG_MOVIES_INFLIGHT_USE_AUDIO_FROM_FILE_IF_PRESENT);
    //ConfigWriteInt("MOVIES", "INFLIGHT_DISPLAY_ASPECT_TYPE", CONFIG_MOVIES_INFLIGHT_DISPLAY_ASPECT_TYPE);
    //ConfigWriteInt("MOVIES", "INFLIGHT_COCKPIT_BG_COLOUR_RGB", CONFIG_MOVIES_INFLIGHT_COCKPIT_BG_COLOUR_RGB);
    //ConfigWriteInt("MOVIES", "INFLIGHT_MONO_SHADER_ENABLE", CONFIG_INFLIGHT_MONO_SHADER_ENABLE);
    //ConfigWriteInt("MOVIES", "INFLIGHT_MONO_SHADER_COLOUR", CONFIG_INFLIGHT_MONO_SHADER_COLOUR);
    //ConfigWriteInt("MOVIES", "INFLIGHT_MONO_SHADER_BRIGHTNESS", CONFIG_INFLIGHT_MONO_SHADER_BRIGHTNESS);
    //ConfigWriteInt("MOVIES", "INFLIGHT_MONO_SHADER_CONTRAST", CONFIG_INFLIGHT_MONO_SHADER_CONTRAST);

    

}


//___________________________
static void ConfigPathSetup() {
    if (pConfigPath)
        return;

    DWORD pathSize = 0;

    pathSize = GetConfigPath_NonUAC(nullptr);
    if (pathSize) {
        pConfigPath = new char[pathSize];
        GetConfigPath_NonUAC(pConfigPath);
    }

    //To-Do - Lets ditch the UAC stuff for now, not sure anyone was using it anyway.

    /*LONG UAC_AWARE = ConfigReadInt(L"MAIN", L"UAC_AWARE", 1);

    if (UAC_AWARE && IsWindowsVistaOrGreater()) {
        delete[] pConfigPath;
        pConfigPath = nullptr;
        DWORD appDatPathSize = UAC_GetAppDataPath(nullptr);
        char* pAppDatPath = nullptr;
        if (appDatPathSize) {
            pAppDatPath = new char[appDatPathSize];
            appDatPathSize = UAC_GetAppDataPath(pAppDatPath);
        }

        pathSize = appDatPathSize + 12;
        pConfigPath = new char[pathSize];
        ZeroMemory(pConfigPath, pathSize);
        if (pAppDatPath)
            strncat_s(pConfigPath, pathSize, pAppDatPath, appDatPathSize);//appDatPathSize
        delete[] pAppDatPath;
        pAppDatPath = nullptr;
        strncat_s(pConfigPath, pathSize, "\\wc3w_en.ini", 13);
    }*/

    //create config file if not found.
    if (pConfigPath && GetFileAttributesA(pConfigPath) == INVALID_FILE_ATTRIBUTES)
        ConfigCreate();
}


//_______________________
void ConfigRefreshCache() {
    ConfigPathSetup();
    WritePrivateProfileStringA(nullptr, nullptr, nullptr, pConfigPath);
}


//______________________________________________________________________________________________________________________________
DWORD ConfigReadString(const char* lpAppName, const char* lpKeyName, const char* lpDefault, char* lpReturnedString, DWORD nSize) {
    ConfigPathSetup();
    return GetPrivateProfileStringA(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, pConfigPath);
}


//________________________________________________________________________________________
BOOL ConfigWriteString(const char* lpAppName, const char* lpKeyName, const char* lpString) {
    ConfigPathSetup();
    return WritePrivateProfileStringA(lpAppName, lpKeyName, lpString, pConfigPath);
}


//____________________________________________________________________________
UINT ConfigReadInt(const char* lpAppName, const char* lpKeyName, int nDefault) {
    ConfigPathSetup();
    return GetPrivateProfileIntA(lpAppName, lpKeyName, nDefault, pConfigPath);
}


//___________________________________________________________________________
BOOL ConfigWriteInt(const char* lpAppName, const char* lpKeyName, int intVal) {
    ConfigPathSetup();
    char lpString[64];
    sprintf_s(lpString, 64, "%d", intVal);
    return WritePrivateProfileStringA(lpAppName, lpKeyName, lpString, pConfigPath);
}


//_____________________________________________________________________________________________
BOOL ConfigReadWinData(const char* lpAppName, const char* lpKeyName, WINDOWPLACEMENT* pWinData) {
    ConfigPathSetup();
    pWinData->length = sizeof(WINDOWPLACEMENT);
    return GetPrivateProfileStructA(lpAppName, lpKeyName, pWinData, sizeof(WINDOWPLACEMENT), pConfigPath);
}


//______________________________________________________________________________________________
BOOL ConfigWriteWinData(const char* lpAppName, const char* lpKeyName, WINDOWPLACEMENT* pWinData) {
    ConfigPathSetup();
    return WritePrivateProfileStructA(lpAppName, lpKeyName, pWinData, sizeof(WINDOWPLACEMENT), pConfigPath);
}


//____________________________________________________________________________________________________
BOOL ConfigReadStruct(const char* lpszSection, const char* lpszKey, LPVOID lpStruct, UINT uSizeStruct) {
    ConfigPathSetup();
    return GetPrivateProfileStructA(lpszSection, lpszKey, lpStruct, uSizeStruct, pConfigPath);
}


//_____________________________________________________________________________________________________
BOOL ConfigWriteStruct(const char* lpszSection, const char* lpszKey, LPVOID lpStruct, UINT uSizeStruct) {
    ConfigPathSetup();
    return WritePrivateProfileStructA(lpszSection, lpszKey, lpStruct, uSizeStruct, pConfigPath);
}


//___________________
char* ConfigGetPath() {
    ConfigPathSetup();
   return pConfigPath;
}


//__________________
bool ConfigDestroy() {
    ConfigPathSetup();
    if (pConfigPath) {
        if (DeleteFileA(pConfigPath)) {//delete ini file
            char* pFileName = strstr(pConfigPath, "\\wc3w_en.ini");
            if (pFileName) {
                *pFileName ='\0';//remove file name from path
                RemoveDirectoryA(pConfigPath);// delete config folder if empty
            }
            delete[] pConfigPath;
            pConfigPath = nullptr;
            return true;
        }
    }
    return false;
}
