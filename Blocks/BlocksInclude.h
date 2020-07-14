/*********************************************************
*
* BlocksInclude.h
* IMBlocks
*
* Sean Hickey
* 2020
*
**********************************************************/

#include <stdio.h>
#include <math.h>

u32 SizeForFormatString(const char *formatStr, va_list args) {
    int size = vsnprintf(NULL, 0, formatStr, args);
    return size + 1; // Since size doesn't include space for terminating \0
}

void FormatString(char *buffer, const char *formatStr, va_list args) {
    vsprintf(buffer, formatStr, args);
}

f32 Ceil(f32 num) {
    return ceilf(num);
}
