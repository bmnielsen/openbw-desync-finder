#ifdef _WIN32

#include <BWAPI.h>
#include <Windows.h>

#include "DumpDataModule.h"

extern "C" __declspec(dllexport) void gameInit(BWAPI::Game *game)
{
    BWAPI::BroodwarPtr = game;
}

extern "C" __declspec(dllexport) BWAPI::AIModule *newAIModule()
{
    return new DumpDataModule();
}

#endif
