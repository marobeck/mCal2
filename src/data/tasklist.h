#pragma once
#include "defs.h"
#include "DLL.h"
// Objects
#include "task.h"
#include "timeblock.h"

#include <string>
#include <ctime>
#include <cstdint>

class Tasklist
{
    void SortTimeBlocks();
    void SortTasks();
};