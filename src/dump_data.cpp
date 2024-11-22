#include "BWAPI/GameImpl.h"
#include "DumpDataModule.h"

int main()
{
    auto replayFile = "/Users/bruce.nielsen/BW/desyncs/McRave-Iron-2.rep";

    BW::GameOwner gameOwner;
    BWAPI::BroodwarImpl_handle h(gameOwner.getGame());
    BWAPI::BroodwarImpl.bwgame.setMapFileName(replayFile);

    h->createSinglePlayerGame([&]()
                              {
                                  h->startGame();
                              });

    auto module = DumpDataModule();
    h->setAIModule(&module);

    while (!gameOwner.getGame().gameOver())
    {
        h->update();
        gameOwner.getGame().nextFrame();
    }
    module.onEnd(false);

    return 0;
}
