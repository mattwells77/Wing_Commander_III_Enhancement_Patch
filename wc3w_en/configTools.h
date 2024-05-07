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

#define CONFIG_SPACE_COCKPIT_MAINTAIN_ASPECT_RATIO	1
#define CONFIG_SPACE_SPACE_REFRESH_RATE_HZ			24
#define CONFIG_SPACE_NAV_SCREEN_KEY_RESPONCE_HZ		24

#define CONFIG_MOVIES_PATH				"movies\\"
#define CONFIG_MOVIES_EXT				"mp4"
#define CONFIG_MOVIES_BRANCH_OFFSET_MS	-210




UINT ConfigReadInt(const char* lpAppName, const char* lpKeyName, int nDefault);
BOOL ConfigWriteInt(const char* lpAppName, const char* lpKeyName, int intVal);

DWORD ConfigReadString(const char* lpAppName, const char* lpKeyName, const char* lpDefault, char* lpReturnedString, DWORD nSize);
BOOL ConfigWriteString(const char* lpAppName, const char* lpKeyName, const char* lpString);

BOOL ConfigReadWinData(const char* lpAppName, const char* lpKeyName, WINDOWPLACEMENT* pWinData);
BOOL ConfigWriteWinData(const char* lpAppName, const char* lpKeyName, WINDOWPLACEMENT* pWinData);

BOOL ConfigReadStruct(const char* lpszSection, const char* lpszKey, LPVOID lpStruct, UINT uSizeStruct);
BOOL ConfigWriteStruct(const char* lpszSection, const char* lpszKey, LPVOID lpStruct, UINT uSizeStruct);

void ConfigRefreshCache();
char* ConfigGetPath();
bool ConfigDestroy();
