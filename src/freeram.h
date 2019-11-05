#ifndef _FREERAM_H_

#ifdef __arm__
#include "WProgram.h"
#include <stdlib.h>

// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#endif  // __arm__

// function from the sdFat library (SdFatUtil.cpp)
// licensed under GPL v3
// Full credit goes to William Greiman.
int freeRam() {
    #ifdef __arm__
        char top;
        return &top - reinterpret_cast<char*>(sbrk(0));
    #else
        return 0;
    #endif
}

#endif  // __FREERAM_H_
