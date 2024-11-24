#include "FindDesyncsModule.h"
#include "rngdebug.h"

int main()
{
    auto replayFile = "../files/McRave-Iron-2.rep";

    printRngCallsFromFrame = 23453;
    printRngCallsToFrame = 23458;

    std::vector<std::tuple<int, int, int, int, int, int, int>> results;
    auto addFrame = [](int frame, int rngIdx)
    {
        if (frame == 0) return;
        if (frame > 0) framesToConsumeRng.emplace_back(frame, rngIdx);
        if (frame < 0) framesToIgnoreRng.emplace_back(-frame, rngIdx);
    };
    auto check = [&](int first, int firstIdx, int second, int secondIdx, int third, int thirdIdx)
    {
        framesToConsumeRng.clear();
        framesToIgnoreRng.clear();
        addFrame(first, firstIdx);
        addFrame(second, secondIdx);
        addFrame(third, thirdIdx);
        auto result = FindDesyncsModule::getDesyncFrame(replayFile);
        results.emplace_back(first, firstIdx, second, secondIdx, third, thirdIdx, result);
        return result == INT_MAX;
    };
    for (int first = 15128; first <= 15128; first++)
    {
//        if (check(first, 0, 0, 0, 0, 0)) goto breakAll;
//        for (int second=23455; second <= 0; second++)
//        {
//            if (check(first, 0, second, 0)) goto breakAll;
//            for (int third=second; third <= 0; third++)
//            {
//                if (check(first, second, third)) goto breakAll;
//            }
//        }
        for (int secondIdx = 4; secondIdx <= 9; secondIdx++)
        {
            if (check(first, 0, 23455, secondIdx, 0, 0)) goto breakAll;
        }
//        for (int second = -23455; second >= -23456; second--)
//        {
//            for (int idx = 0; idx < 13; idx++)
//            {
//                if (check(first, 0, second, idx, 0, 0)) goto breakAll;
//            }
//        }
    }
    breakAll:;

    int bestFrame = -1;
    int bestFirst = 0;
    int bestFirstIdx = 0;
    int bestSecond = 0;
    int bestSecondIdx = 0;
    int bestThird = 0;
    int bestThirdIdx = 0;
    for (auto [first, firstIdx, second, secondIdx, third, thirdIdx, result] : results)
    {
        if (result > bestFrame)
        {
            bestFrame = result;
            bestFirst = first;
            bestFirstIdx = firstIdx;
            bestSecond = second;
            bestSecondIdx = secondIdx;
            bestThird = third;
            bestThirdIdx = thirdIdx;
        }
    }

//    if (results.size() > 3)
//    {
//        framesToConsumeRng.clear();
//        framesToIgnoreRng.clear();
//
//        framesToConsumeRng.push_back(bestFirst);
//        if (bestSecond != -1) framesToConsumeRng.push_back(bestSecond);
//        if (bestThird != -1) framesToConsumeRng.push_back(bestThird);
//        FindDesyncsModule::getDesyncFrame(replayFile);
//    }

    if (bestFrame == INT_MAX)
    {
        std::cout << "Best result is ("
                  << bestFirst << ":" << bestFirstIdx << ","
                  << bestSecond << ":" << bestSecondIdx << ","
                  << bestThird << ":" << bestThirdIdx << ") with no desync" << std::endl;
    }
    else
    {
        std::cout << "Best result is ("
                  << bestFirst << ":" << bestFirstIdx << ","
                  << bestSecond << ":" << bestSecondIdx << ","
                  << bestThird << ":" << bestThirdIdx << ") with desync frame " << bestFrame << std::endl;
    }
    std::cout << "Full results:" << std::endl;
    for (auto [first, firstIdx, second, secondIdx, third, thirdIdx, result] : results)
    {
        std::cout << "(" << first << ":" << firstIdx
                  << "," << second << ":" << secondIdx
                  << "," << third << ":" << thirdIdx << ") = " << result << std::endl;
    }

    return 0;
}
