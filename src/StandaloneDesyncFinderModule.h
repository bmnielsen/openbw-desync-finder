#pragma once

#include "BWAPI.h"
#include "BWAPI/GameImpl.h"
#include <fstream>

#define LOGGING false

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

#if !LOGGING
        if (BWAPI::Broodwar->getFrameCount() % 500 == 0)
        {
            std::cout << "\r" << BWAPI::Broodwar->mapPathName() << ": " << BWAPI::Broodwar->getFrameCount()
                      << " - " << unitsThatHaveNotBeenOrdered.size()
                      << "        " << std::flush;
        }
#endif

        // First detect any new units and add them to the list of units that haven't been ordered
        // We only consider units that can attack and aren't buildings
        auto addNewUnits = [&](const BWAPI::Unitset &units)
        {
            for (const auto &unit : units)
            {
                if (!unit->exists()) continue;
                if (!unit->isCompleted()) continue;
                if (unit->getType() == BWAPI::UnitTypes::Zerg_Larva) continue;
                if (unit->getType() == BWAPI::UnitTypes::Zerg_Egg) continue;
                if (unit->getType() == BWAPI::UnitTypes::Protoss_Interceptor) continue;
                if (unit->getType() == BWAPI::UnitTypes::Protoss_Scarab) continue;
                if (unit->getType() == BWAPI::UnitTypes::Terran_Vulture_Spider_Mine) continue;
                if (unit->getType().isBuilding()) continue;
                if (unit->getType().groundWeapon() == BWAPI::WeaponTypes::None && unit->getType().airWeapon() == BWAPI::WeaponTypes::None) continue;

                if (seenIDs.contains(unit->getID())) continue;
                seenIDs.insert(unit->getID());

                unitsThatHaveNotBeenOrdered.emplace_back(unit, BWAPI::Broodwar->getFrameCount());
#if LOGGING
                std::cout << BWAPI::Broodwar->getFrameCount() << ": Added " << unit->getID() << " (" << unit->getType() << ")" << std::endl;
#endif
            }
        };
        addNewUnits(BWAPI::Broodwar->getPlayer(0)->getUnits());
        addNewUnits(BWAPI::Broodwar->getPlayer(1)->getUnits());

        // Next, remove any units from the list that have been ordered
        for (auto it = unitsThatHaveNotBeenOrdered.begin(); it != unitsThatHaveNotBeenOrdered.end(); )
        {
            if (it->first->getOrder() != BWAPI::Orders::None
                && it->first->getOrder() != BWAPI::Orders::Nothing
                && it->first->getOrder() != BWAPI::Orders::PickupIdle
                && it->first->getOrder() != BWAPI::Orders::Guard
                && it->first->getOrder() != BWAPI::Orders::PlayerGuard)
            {
#if LOGGING
                std::cout << BWAPI::Broodwar->getFrameCount() << ": Clearing " << it->first->getID()
                          << "; order is " << it->first->getOrder() << std::endl;
#endif

                it = unitsThatHaveNotBeenOrdered.erase(it);
            }
            else
            {
                it++;
            }
        }

//        if (quit) return;
//        if (consecutiveDesyncFrames > 10)
//        {
//            std::cout << "DESYNC" << std::flush;
//            BWAPI::Broodwar->leaveGame();
//            quit = true;
//        }
//
//        if (BWAPI::Broodwar->getFrameCount() > 1 && BWAPI::Broodwar->getFrameCount() % 1000 == 0)
//        {
//            std::cout << "." << std::flush;
//        }
//
//        if (BWAPI::Broodwar->getFrameCount() > 1 && (consecutiveDesyncFrames > 0 || (BWAPI::Broodwar->getFrameCount() % 500 == 0)))
//        {
//            unsigned long unitsOnGuard = 0;
//            unsigned long totalUnits = 0;
//            auto addPlayerData = [&](BWAPI::Player player)
//            {
//                for (const auto &unit : player->getUnits())
//                {
//                    if (!unit->exists()) continue;
//                    if (!unit->getType().canAttack()) continue;
//                    totalUnits++;
//                    if (unit->getOrder() == BWAPI::Orders::PlayerGuard) unitsOnGuard++;
//                }
//            };
//            addPlayerData(BWAPI::Broodwar->getPlayer(1));
//            addPlayerData(BWAPI::Broodwar->getPlayer(2));
//
//            double percentOnGuard = 0.0;
//            if (totalUnits > 0)
//            {
//                percentOnGuard = (double)unitsOnGuard / (double)totalUnits;
//            }
//            if (percentOnGuard > 0.2)
//            {
//                consecutiveDesyncFrames++;
//            }
//            else
//            {
//                consecutiveDesyncFrames = 0;
//            }
//        }
    }

    void onEnd(bool) override
    {
        if (unitsThatHaveNotBeenOrdered.size() < 10)
        {
            std::cout << "\r" << BWAPI::Broodwar->mapPathName() << ": OK" << std::endl;
        }
        else
        {
            std::cout << "\r" << BWAPI::Broodwar->mapPathName() << ": DESYNC" << std::endl;
            std::cout << "Not ordered units: ";
            for (const auto &[unit, frame] : unitsThatHaveNotBeenOrdered)
            {
                std::cout << "\n" << unit->getType() << " @ " << unit->getTilePosition() << " first seen at " << frame;
            }
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
    std::vector<std::pair<BWAPI::Unit, int>> unitsThatHaveNotBeenOrdered;
};
