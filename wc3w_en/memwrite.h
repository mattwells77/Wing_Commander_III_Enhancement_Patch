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

void MemBlt8(BYTE *fromBuff, int subWidth, int subHeight, int fromWidth, BYTE *toBuff, int toWidth);
void MemBltMasked8(BYTE *fromBuff, int subWidth, int subHeight, int fromWidth, BYTE *toBuff, int toWidth);
void MemBlt8Stretch(BYTE *fromBuff, int subWidth, int subHeight, int fromWidth, BYTE *toBuff, int toWidth, int toHeight, bool ARatio, bool centred);

bool CompareCharArray_IgnoreCase(const char* msg1, const char* msg2, DWORD numChars);

#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

void MemWrite8__(const char* file, int line, DWORD address, BYTE expected, BYTE change_to);
#define MemWrite8( address, expected,  change_to) MemWrite8__( __FILENAME__, __LINE__, address, expected,  change_to)
void MemWrite16__(const char* file, int line, DWORD address, WORD expected, WORD change_to);
#define MemWrite16( address, expected,  change_to) MemWrite16__( __FILENAME__, __LINE__, address, expected,  change_to)
void MemWrite32__(const char* file, int line, DWORD address, DWORD expected, DWORD change_to);
#define MemWrite32( address, expected,  change_to) MemWrite32__( __FILENAME__, __LINE__, address, expected,  change_to)
void MemWriteString__(const char* file, int line, DWORD  address, unsigned char* expected, unsigned char* change_to, DWORD length);
#define MemWriteString( address, expected,  change_to, length) MemWriteString__( __FILENAME__, __LINE__, address, expected,  change_to, length)


void FuncWrite32__(const char* file, int line, DWORD address, DWORD expected, DWORD funcAddress);
#define FuncWrite32( address, expected,  change_to) FuncWrite32__( __FILENAME__, __LINE__, address, expected,  change_to)
void FuncReplace32__(const char* file, int line, DWORD address, DWORD expected, DWORD funcAddress);
#define FuncReplace32( address, expected,  change_to) FuncReplace32__( __FILENAME__, __LINE__, address, expected,  change_to)
