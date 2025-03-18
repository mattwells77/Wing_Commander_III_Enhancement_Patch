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

#pragma once

//defaults
#define CONFIG_MAIN_WINDOWED	0
#define CONFIG_MAIN_WIN_DATA	0
#define CONFIG_MAIN_UAC_AWARE	1

#define CONFIG_MAIN_DEAD_ZONE	1


#define CONFIG_SPACE_COCKPIT_MAINTAIN_ASPECT_RATIO	1
#define CONFIG_SPACE_SPACE_REFRESH_RATE_HZ			24
#define CONFIG_SPACE_NAV_SCREEN_KEY_RESPONSE_HZ		24


#define CONFIG_SHOW_ORIGINAL_MOVIES_INTERLACED		0

#define CONFIG_MOVIES_PATH							L"movies\\"
#define CONFIG_MOVIES_EXT							L"mp4"
#define CONFIG_MOVIES_BRANCH_OFFSET_MS				-210

#define CONFIG_MOVIES_INFLIGHT_USE_AUDIO_FROM_FILE_IF_PRESENT	1
//0 = fill, 1 = fit
#define CONFIG_MOVIES_INFLIGHT_DISPLAY_ASPECT_TYPE				1

#define CONFIG_MOVIES_INFLIGHT_COCKPIT_BG_COLOUR_RGB			0x00202020

#define CONFIG_INFLIGHT_MONO_SHADER_ENABLE						0
#define CONFIG_INFLIGHT_MONO_SHADER_COLOUR						0x34FF4E
#define CONFIG_INFLIGHT_MONO_SHADER_BRIGHTNESS					100
#define CONFIG_INFLIGHT_MONO_SHADER_CONTRAST					100



UINT ConfigReadInt(const wchar_t* lpAppName, const wchar_t* lpKeyName, int nDefault);
BOOL ConfigWriteInt(const wchar_t* lpAppName, const wchar_t* lpKeyName, int intVal);

DWORD ConfigReadString(const wchar_t* lpAppName, const wchar_t* lpKeyName, const wchar_t* lpDefault, wchar_t* lpReturnedString, DWORD nSize);
BOOL ConfigWriteString(const wchar_t* lpAppName, const wchar_t* lpKeyName, const wchar_t* lpString);

BOOL ConfigReadWinData(const wchar_t* lpAppName, const wchar_t* lpKeyName, WINDOWPLACEMENT* pWinData);
BOOL ConfigWriteWinData(const wchar_t* lpAppName, const wchar_t* lpKeyName, WINDOWPLACEMENT* pWinData);

BOOL ConfigReadStruct(const wchar_t* lpszSection, const wchar_t* lpszKey, LPVOID lpStruct, UINT uSizeStruct);
BOOL ConfigWriteStruct(const wchar_t* lpszSection, const wchar_t* lpszKey, LPVOID lpStruct, UINT uSizeStruct);

void ConfigRefreshCache();


//BOOL GetAppDataPath(std::wstring* p_ret_path);

const wchar_t* GetAppPath();
const wchar_t* GetAppDataPath();