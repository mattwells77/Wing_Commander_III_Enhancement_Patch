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
#include "memwrite.h"

//______________________________________________________________________________________________
void FuncWrite32__(const char* file, int line, DWORD address, DWORD expected, DWORD funcAddress) {

    funcAddress = funcAddress - (address + 4);

    DWORD oldProtect;
    VirtualProtect((LPVOID)address, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
    if (CompareMem_DWORD(file, line, (DWORD*)address, expected))
        *(DWORD*)address = funcAddress;
    VirtualProtect((LPVOID)address, 4, oldProtect, &oldProtect);
}


//________________________________________________________________________________________________
void FuncReplace32__(const char* file, int line, DWORD address, DWORD expected, DWORD funcAddress) {

    expected = expected + (address + 4);
    funcAddress = funcAddress - (address + 4);
    expected = expected - (address + 4);

    DWORD oldProtect;
    VirtualProtect((LPVOID)address, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
    if (CompareMem_DWORD(file, line, (DWORD*)address, expected))
        *(DWORD*)address = funcAddress;
    VirtualProtect((LPVOID)address, 4, oldProtect, &oldProtect);
}


//________________________________________________________________________________________
void MemWrite8__(const char* file, int line, DWORD address, BYTE expected, BYTE change_to) {

    DWORD oldProtect;
    VirtualProtect((LPVOID)address, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    if (CompareMem_BYTE(file, line, (BYTE*)address, expected))
        *(BYTE*)address = change_to;
    VirtualProtect((LPVOID)address, 1, oldProtect, &oldProtect);
}


//_________________________________________________________________________________________
void MemWrite16__(const char* file, int line, DWORD address, WORD expected, WORD change_to) {

    DWORD oldProtect;
    VirtualProtect((LPVOID)address, 2, PAGE_EXECUTE_READWRITE, &oldProtect);
    if (CompareMem_WORD(file, line, (WORD*)address, expected))
        *(WORD*)address = change_to;
    VirtualProtect((LPVOID)address, 2, oldProtect, &oldProtect);
}


//___________________________________________________________________________________________
void MemWrite32__(const char* file, int line, DWORD address, DWORD expected, DWORD change_to) {

    DWORD oldProtect;
    VirtualProtect((LPVOID)address, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
    if (CompareMem_DWORD(file, line, (DWORD*)address, expected))
        *(DWORD*)address = change_to;
    VirtualProtect((LPVOID)address, 4, oldProtect, &oldProtect);
}


//_______________________________________________________________________________________________________________________________
void MemWriteString__(const char* file, int line, DWORD address, unsigned char* expected, unsigned char* change_to, DWORD length) {

    if (memcmp((void*)address, expected, length) != 0)
        Error_RecordMemMisMatch(file, line, address, expected, (unsigned char*)address, length);
    else {
        DWORD oldProtect;
        VirtualProtect((LPVOID)address, length + 1, PAGE_EXECUTE_READWRITE, &oldProtect);
        memcpy((void*)address, change_to, length);
        VirtualProtect((LPVOID)address, length + 1, oldProtect, &oldProtect);
    }
}


//_________________________________________________________________________________________________
void MemBlt8(BYTE* fromBuff, int subWidth, int subHeight, int fromWidth, BYTE* toBuff, int toWidth) {

    for (int h = 0; h < subHeight; h++) {
        memcpy(toBuff, fromBuff, subWidth);
        fromBuff += fromWidth;
        toBuff += toWidth;
    }
}


//_______________________________________________________________________________________________________
void MemBltMasked8(BYTE* fromBuff, int subWidth, int subHeight, int fromWidth, BYTE* toBuff, int toWidth) {

    for (int h = 0; h < subHeight; h++) {
        for (int w = 0; w < subWidth; w++) {
            if (fromBuff[w] != 0)
                toBuff[w] = fromBuff[w];
        }
        fromBuff += fromWidth;
        toBuff += toWidth;
    }
}


//_________________________________________________________________________________________________________________________________________________
void MemBlt8Stretch(BYTE* fromBuff, int subWidth, int subHeight, int fromWidth, BYTE* toBuff, int toWidth, int toHeight, bool ARatio, bool centred) {

    float toWidthAR = (float)toWidth, toHeightAR = (float)toHeight;

    if (ARatio) {
        float imageRO = (float)subWidth / subHeight;
        float winRO = (float)toWidth / toHeight;

        if (winRO > imageRO) {
            toWidthAR = toHeightAR / subHeight * subWidth;
            if (centred)
                toBuff += (toWidth - (int)toWidthAR) / 2;
        }
        else if (winRO < imageRO) {
            toHeightAR = toWidthAR / subWidth * subHeight;
            if (centred)
                toBuff += ((toHeight - (int)toHeightAR) / 2) * toWidth;
        }
    }

    float pWidth = subWidth / toWidthAR;
    float pHeight = subHeight / toHeightAR;

    float fx = 0, fy = 0;
    int fyMul = 0;

    for (int ty = 0; ty < (int)toHeightAR; ty++) {
        fx = 0;
        for (int tx = 0; tx < (int)toWidthAR; tx++) {//draw stretched line
            toBuff[tx] = fromBuff[fyMul + (int)fx];
            fx += pWidth;
            if (fx >= subWidth)
                fx = (float)subWidth - 1;
        }
        fy += pHeight;
        if ((int)fy >= subHeight)
            fy = (float)subHeight - 1;
        fyMul = (int)fy * fromWidth;
        toBuff += toWidth;
    }
}


//__________________________________________________________________________________
bool CompareCharArray_IgnoreCase(const char* msg1, const char* msg2, DWORD numChars) {
    if (msg1 == nullptr || msg2 == nullptr)
        return false;
    for (DWORD num = 0; num < numChars; num++) {
        if (msg1[num] == '\0' && msg2[num] != '\0')
            return false;
        if (msg2[num] == '\0' && msg1[num] != '\0')
            return false;
        if (tolower(msg1[num]) != tolower(msg2[num]))
            return false;
    }
    return true;
}
