/**
 * @File Memory.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/20
 * @Brief This file is part of Bee.
 */

#include <mimalloc.h>
#include <mimalloc-override.h>
#ifdef _WIN32
#include <mimalloc-new-delete.h>
#pragma comment(linker, "/include:mi_version")
#endif

#include "Memory/Memory.hpp"

void bee::MemoryReset()
{
    mi_version();
    mi_stats_reset();
}

void bee::MemoryPrintStats()
{
    mi_stats_print(nullptr);
}