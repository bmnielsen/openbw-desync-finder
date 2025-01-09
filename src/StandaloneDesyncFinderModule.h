#pragma once

#include "BWAPI.h"
#include "BWAPI/GameImpl.h"
#include <fstream>

#define LOGGING false

namespace
{
    bool shouldIgnore(BWAPI::UnitType type)
    {
        return type == BWAPI::UnitTypes::Zerg_Larva
               || type == BWAPI::UnitTypes::Zerg_Egg
               || type == BWAPI::UnitTypes::Protoss_Interceptor
               || type == BWAPI::UnitTypes::Protoss_Scarab
               || type == BWAPI::UnitTypes::Terran_Vulture_Spider_Mine
               || type.isAddon();
    }
}

class StandaloneDesyncFinderModule : public BWAPI::AIModule
{
public:
    void onStart() override
    {
        std::cout << BWAPI::Broodwar->mapPathName() << std::flush;
#if LOGGING
        std::cout << std::endl;
#endif

        BWAPI::Broodwar->setLocalSpeed(0);
        BWAPI::Broodwar->setLatCom(false);
    }

    void onFrame() override
    {
        // Check every 25 frames
        if (BWAPI::Broodwar->getFrameCount() < 50 || BWAPI::Broodwar->getFrameCount() % 25 != 0) return;

#if LOGGING
        if (BWAPI::Broodwar->getFrameCount() % 1000 == 0)
        {
            std::cout << "Processed " << BWAPI::Broodwar->getFrameCount() << " frames" << std::endl;
        }
#else
        if (BWAPI::Broodwar->getFrameCount() % 500 == 0)
        {
            std::cout << "\r" << BWAPI::Broodwar->mapPathName() << ": " << BWAPI::Broodwar->getFrameCount()
                      << " - " << unitsThatHaveNotBeenOrdered.size()
                      << "        " << std::flush;
        }
#endif

        // First detect any new units and add them to the list of units that haven't been ordered
        // We only consider units that can attack and aren't buildings
        // We also keep track of how many combat units and buildings each player has
        auto processUnits = [&](int playerId)
        {
            unsigned long combatUnits = 0;
            unsigned long buildings = 0;
            for (const auto &unit : BWAPI::Broodwar->getPlayer(playerId)->getUnits())
            {
                if (!unit->exists()) continue;
                if (shouldIgnore(unit->getType())) continue;

                if (unit->getType().isBuilding())
                {
                    buildings++;
                    continue;
                }

                if (!unit->isCompleted()) continue;
                if (unit->getType().groundWeapon() == BWAPI::WeaponTypes::None && unit->getType().airWeapon() == BWAPI::WeaponTypes::None) continue;

                combatUnits++;

                if (seenIDs.contains(unit->getID())) continue;
                seenIDs.insert(unit->getID());

                unitsThatHaveNotBeenOrdered.emplace_back(unit, playerId, BWAPI::Broodwar->getFrameCount());
#if LOGGING
                std::cout << BWAPI::Broodwar->getFrameCount() << ": Added " << unit->getID() << " (" << unit->getType() << ") @ "
                          << unit->getTilePosition() << std::endl;
#endif
            }
            return std::make_pair(combatUnits, buildings);
        };
        playerOneUnitCounts = processUnits(0);
        playerTwoUnitCounts = processUnits(1);

        // Next, remove any units from the list that have been ordered
        for (auto it = unitsThatHaveNotBeenOrdered.begin(); it != unitsThatHaveNotBeenOrdered.end(); )
        {
            const auto &unit = std::get<0>(*it);
            if (unit->getOrder() != BWAPI::Orders::None
                && unit->getOrder() != BWAPI::Orders::Nothing
                && unit->getOrder() != BWAPI::Orders::PickupIdle
                && unit->getOrder() != BWAPI::Orders::Guard
                && unit->getOrder() != BWAPI::Orders::PlayerGuard)
            {
#if LOGGING
                std::cout << BWAPI::Broodwar->getFrameCount() << ": Clearing " << unit->getID() << " (" << unit->getType() << ") @ "
                          << unit->getTilePosition() << "; order is " << unit->getOrder() << std::endl;
#endif

                it = unitsThatHaveNotBeenOrdered.erase(it);
            }
            else
            {
                it++;
            }
        }
    }

    void onEnd(bool) override
    {
        // Count the number of units for each player that haven't been ordered, ignoring any that completed in the last 500 frames
        unsigned long playerOneNotOrderedUnits = 0;
        unsigned long playerTwoNotOrderedUnits = 0;
        for (const auto &[unit, playerId, frame] : unitsThatHaveNotBeenOrdered)
        {
            if ((BWAPI::Broodwar->getFrameCount() - frame) < 500) continue;
            ((playerId == 0) ? playerOneNotOrderedUnits : playerTwoNotOrderedUnits)++;
        }
        unsigned long totalNotOrderedUnits = playerOneNotOrderedUnits + playerTwoNotOrderedUnits;

        auto outputResult = [&](const std::string &label)
        {
            std::cout << "\r" << BWAPI::Broodwar->mapPathName() << ": " << label
                      << ", p1 remaining (" << playerOneUnitCounts.first << "," << playerOneUnitCounts.second << ")"
                      << ", p2 remaining (" << playerTwoUnitCounts.first << "," << playerTwoUnitCounts.second << ")";
            if (playerOneNotOrderedUnits > 0) std::cout << ", p1 unordered " << playerOneNotOrderedUnits;
            if (playerTwoNotOrderedUnits > 0) std::cout << ", p2 unordered " << playerTwoNotOrderedUnits;
            std::cout << std::endl;
        };

        // If there were not many unordered units, it probably isn't a desync
        if (totalNotOrderedUnits < 5)
        {
            outputResult((totalNotOrderedUnits == 0) ? "OK - no unordered units" : "OK - few unordered units");
            return;
        }

        // If one of the players has no buildings left, the game ended via elimination and there can not have been a desync
        if (playerOneUnitCounts.second == 0 || playerTwoUnitCounts.second == 0)
        {
            outputResult("Probably OK - a player was eliminated");
            return;
        }

        // If one of the players had far more combat units than the other at the end of the game, it probably isn't a desync
        if (playerOneUnitCounts.first * 6 < playerTwoUnitCounts.first || playerTwoUnitCounts.first * 6 < playerOneUnitCounts.first)
        {
            outputResult(" Probably OK - one army is much larger, so surrender makes sense");
            return;
        }

        outputResult("DESYNC");
        std::cout << "Not ordered units: ";
        for (const auto &[unit, _, frame] : unitsThatHaveNotBeenOrdered)
        {
            if ((BWAPI::Broodwar->getFrameCount() - frame) < 500) continue;
            std::cout << "\n" << unit->getType() << " @ " << unit->getTilePosition() << " first seen at " << frame;
        }
    }

    static void checkForDesync(const std::string &replayFile)
    {
        BW::GameOwner gameOwner;
        BWAPI::BroodwarImpl_handle h(gameOwner.getGame());
        BWAPI::BroodwarImpl.bwgame.setMapFileName(replayFile);

        h->createSinglePlayerGame([&]()
                                  {
                                      h->startGame();
                                  });

        auto module = StandaloneDesyncFinderModule();
        h->setAIModule(&module);

        while (!gameOwner.getGame().gameOver())
        {
            h->update();
            gameOwner.getGame().nextFrame();
        }
        module.onEnd(false);
    }

private:
    std::unordered_set<int> seenIDs;
    std::vector<std::tuple<BWAPI::Unit, int, int>> unitsThatHaveNotBeenOrdered;
    std::pair<int, int> playerOneUnitCounts;
    std::pair<int, int> playerTwoUnitCounts;
};
