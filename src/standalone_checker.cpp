#include "StandaloneDesyncFinderModule.h"

#include <filesystem>

int main()
{
//    auto replayFile = "/Users/bruce.nielsen/BW/desyncs/HaoPan-BryanWeber.rep";
//
//    StandaloneDesyncFinderModule::checkForDesync(replayFile);

    auto directory = "/Users/bruce.nielsen/BW/replay_desync_testing";
    for (const auto &replayFile : std::filesystem::recursive_directory_iterator(directory))
    {
        StandaloneDesyncFinderModule::checkForDesync(replayFile.path());
    }

    return 0;
}
