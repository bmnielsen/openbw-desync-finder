#include "FindDesyncsModule.h"
#include "rngdebug.h"

int main()
{
    auto replayFile = "/Users/bruce.nielsen/BW/desyncs/McRave-Iron-2.rep";

    std::vector<std::tuple<int, int, int, int>> results;
    for (int first = 15128; first <= 15128; first++)
    {
        framesToConsumeRng.clear();
        framesToConsumeRng.push_back(first);
        results.emplace_back(first, -1, -1, FindDesyncsModule::getDesyncFrame(replayFile));

        for (int second=first; second <= 0; second++)
        {
            framesToConsumeRng.clear();
            framesToConsumeRng.push_back(first);
            framesToConsumeRng.push_back(second);
            results.emplace_back(first, -1, -1, FindDesyncsModule::getDesyncFrame(replayFile));

            for (int third=second; third <= 0; third++)
            {
                framesToConsumeRng.clear();
                framesToConsumeRng.push_back(first);
                framesToConsumeRng.push_back(second);
                framesToConsumeRng.push_back(third);
                results.emplace_back(first, second, third, FindDesyncsModule::getDesyncFrame(replayFile));
            }
        }
    }

    int bestFrame = -1;
    int bestFirst = 0;
    int bestSecond = 0;
    int bestThird = 0;
    for (auto [first, second, third, result] : results)
    {
        if (result > bestFrame)
        {
            bestFrame = result;
            bestFirst = first;
            bestSecond = second;
            bestThird = third;
        }
    }

    framesToConsumeRng.clear();
    framesToConsumeRng.push_back(bestFirst);
    if (bestSecond != -1) framesToConsumeRng.push_back(bestSecond);
    if (bestThird != -1) framesToConsumeRng.push_back(bestThird);
    FindDesyncsModule::getDesyncFrame(replayFile);

    if (bestFrame == INT_MAX)
    {
        std::cout << "Best result is (" << bestFirst << "," << bestSecond << "," << bestThird << ") with no desync" << std::endl;
    }
    else
    {
        std::cout << "Best result is (" << bestFirst << "," << bestSecond << "," << bestThird << ") with desync frame " << bestFrame << std::endl;
    }
    std::cout << "Full results:" << std::endl;
    for (auto [first, second, third, result] : results)
    {
        std::cout << "(" << first << "," << second << "," << third << ") = " << result << std::endl;
    }

    return 0;
}
