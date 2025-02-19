#include "core.h"

int time_to_str(char* str, double bytes)
{
    if(bytes < 1000) return sprintf(str, "%.2lfns", bytes);
    bytes *= 1e-3;
    if(bytes < 1000) return sprintf(str, "%.2lfμs", bytes);
    bytes *= 1e-3;
    if(bytes < 1000) return sprintf(str, "%.2lfms", bytes);
    bytes *= 1e-3;
    if(bytes < 1000) return sprintf(str, "%.2lfs", bytes);
    return 0;
}

int num_to_str(char* str, double bytes)
{
    if(bytes < 1000) return sprintf(str, "%.2lf", bytes);
    bytes *= 1e-3;
    if(bytes < 1000) return sprintf(str, "%.2lfK", bytes);
    bytes *= 1e-3;
    if(bytes < 1000) return sprintf(str, "%.2lfM", bytes);
    bytes *= 1e-3;
    if(bytes < 1000) return sprintf(str, "%.2lfG", bytes);
    return 0;
}
