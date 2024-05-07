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
#include "errors.h"
#include "memwrite.h"

using namespace std;

#define CHECK_ERRORS
#define PRINT_DEBUG_ERRORS
#define PRINT_DEBUG_INFO

char msg_buff[260];


#ifdef CHECK_ERRORS

ofstream errors_stream;


//____________________________
static bool Is_Errors_Stream() {
    if (!errors_stream.is_open())
        errors_stream.open("wc3w_en.log");
    if (errors_stream.is_open())
        return true;
    return false;
}


//______________________________________
void Debug_Info(const char* format, ...) {
#ifdef PRINT_DEBUG_INFO
    va_list args;
    va_start(args, format);
    vsprintf_s(msg_buff, format, args);
    va_end(args);

    if (Is_Errors_Stream())
        errors_stream << msg_buff << endl;
#endif // PRINT_DEBUG_INFO
}


//_________________________________________________________________________________________________________________________________________________________________
void Error_RecordMemMisMatch(const char* file, int line, DWORD inOffset, const unsigned char* in_expectedData, const unsigned char* in_found_data, size_t inLength) {

    ostringstream  outString;
    outString << "offset:  0x" << uppercase << hex << inOffset << endl;
    outString << "Expected:";
    for (unsigned int e = 0; e < inLength; e++) {
        outString.width(2);
        outString.fill('0');
        outString << hex << (int)in_expectedData[e] << " ";
    }
    outString << endl;
    outString << "Found:   ";
    for (unsigned int e = 0; e < inLength; e++) {
        outString.width(2);
        outString.fill('0');
        outString << hex << (int)in_found_data[e] << " ";
    }
    outString << endl;

    if (Is_Errors_Stream())
        errors_stream << "Memory Mismatch Detected in " << file << " " << "on line " << line << endl << outString.str() << endl;
}

#endif // CHECK_ERRORS


//_________________________________________________________________________
bool CompareMem_DWORD(const char* file, int line, DWORD* addr, DWORD value) {
#ifdef CHECK_ERRORS
    if (*addr != value) {
        Error_RecordMemMisMatch(file, line, (DWORD)addr, (unsigned char*)&value, (unsigned char*)addr, sizeof(DWORD));
        return false;
    }
#endif // CHECK_ERRORS
    return true;
}


//______________________________________________________________________
bool CompareMem_WORD(const char* file, int line, WORD* addr, WORD value) {
#ifdef CHECK_ERRORS
    if (*addr != value) {
        Error_RecordMemMisMatch(file, line, (DWORD)addr, (unsigned char*)&value, (unsigned char*)addr, sizeof(WORD));
        return false;
    }
#endif // CHECK_ERRORS
    return true;
}


//______________________________________________________________________
bool CompareMem_BYTE(const char* file, int line, BYTE* addr, BYTE value) {
#ifdef CHECK_ERRORS
    if (*addr != value) {
        Error_RecordMemMisMatch(file, line, (DWORD)addr, (unsigned char*)&value, (unsigned char*)addr, sizeof(BYTE));
        return false;
    }
#endif // CHECK_ERRORS
    return true;
}
