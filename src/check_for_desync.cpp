#include "FindDesyncsModule.h"

int main()
{
    auto replayFile = "/Users/bruce.nielsen/BW/desyncs/HaoPan-Stardust.rep";

    auto desyncFrame = FindDesyncsModule::getDesyncFrame(replayFile);
    if (desyncFrame == INT_MAX)
    {
        std::cout << "No desync found" << std::endl;
    }
    else
    {
        std::cout << "Desync observed at frame " << desyncFrame << std::endl;
    }

    return 0;
}
