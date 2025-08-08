#pragma once
#include "P7_Trace.h"

#define LOG(level, format, ...) Logger::trace->P7_DELIVER(0, EP7TRACE_LEVEL_##level, NULL, TM(format), __VA_ARGS__)

class Logger
{
public:
    static IP7_Trace *trace;
};
