#include "timer_functions.h"

uint32_t timer_func_millis()
{
    return time_us_64() / 1000;
}