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
#include "libvlc_common.h"

//#include <iostream>
//#include <thread>
//#include <cstring>
//using namespace VLC;

/*const char* const vlc_options[] = {
    "--file-caching=300"//,
    //"--network-caching=150",
    //"--clock-jitter=0",
    //"--live-caching=150",
    //"--clock-synchro=0",
    //"-vvv",
    //"--drop-late-frames",
    //"--skip-frames"
     };*/
     //const char* const vlc_options[] = { "--freetype-font=Incised901 Lt BT" };

     //VLC::Instance vlc_instance = VLC::Instance(_countof(vlc_options), vlc_options);
VLC::Instance vlc_instance = VLC::Instance(0, nullptr);
