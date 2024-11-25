#include "GameImpl.h"

#include <BWAPI/Command.h>
#include <BWAPI/TechType.h>
#include <BWAPI/UnitType.h>

#include <BW/OrderTypes.h>

#include "../../../Debug.h"

namespace BWAPI
{
  //----------------------------------------- ADD TO COMMAND BUFFER ------------------------------------------
  void GameImpl::addToCommandBuffer(Command* command)
  {
    //executes latency compensation code and added it to the buffer
    command->execute(0);
    this->commandBuffer[this->commandBuffer.size() - 1].push_back(command);
  }
  //----------------------------------------- APPLY LATENCY COMPENSATION
  void GameImpl::applyLatencyCompensation()
  {
    //apply latency compensation
    while ((int)(this->commandBuffer.size()) > this->getLatency()+15)
    {
      for (unsigned int i = 0; i < this->commandBuffer.front().size(); ++i)
        delete this->commandBuffer.front()[i];
      this->commandBuffer.erase(this->commandBuffer.begin());
    }
    this->commandBuffer.push_back(std::vector<Command *>());
    for (unsigned int i = 0; i < this->commandBuffer.size(); ++i)
      for (unsigned int j = 0; j < this->commandBuffer[i].size(); ++j)
        this->commandBuffer[i][j]->execute(this->commandBuffer.size()-1-i);
  }

  //--------------------------------------------- EXECUTE COMMAND --------------------------------------------
  void GameImpl::executeCommand(UnitCommand command)
  {
    apmCounter.addNoSelect();

    UnitCommandType ct = command.type;
    bool queued = command.isQueued();
    if (ct == UnitCommandTypes::Attack_Move)
    {
      if ( command.unit && command.unit->getType() == UnitTypes::Zerg_Infested_Terran )
        bwgame.QueueCommand<BW::Orders::Attack>(command.x, command.y, Orders::Enum::Attack1, queued);
      else
        bwgame.QueueCommand<BW::Orders::Attack>(command.x, command.y, Orders::Enum::AttackMove, queued);
    }
    else if (ct == UnitCommandTypes::Attack_Unit)
    {
      UnitType ut      = command.unit ? command.unit->getType() : UnitTypes::None;
      if ( ut == UnitTypes::Protoss_Carrier || ut == UnitTypes::Hero_Gantrithor )
        bwgame.QueueCommand<BW::Orders::Attack>(command.target, Orders::Enum::CarrierAttack, queued);
      else if ( ut == UnitTypes::Protoss_Reaver || ut == UnitTypes::Hero_Warbringer )
        bwgame.QueueCommand<BW::Orders::Attack>(command.target, Orders::Enum::ReaverAttack, queued);
      else if ( ut.isBuilding() )
        bwgame.QueueCommand<BW::Orders::Attack>(command.target, Orders::Enum::TowerAttack, queued);
      else
        bwgame.QueueCommand<BW::Orders::Attack>(command.target, Orders::Enum::Attack1, queued);
    }
    else if (ct == UnitCommandTypes::Build)
    {
      UnitType extraType(command.extra);
      if ( command.unit && command.unit->getType() == BWAPI::UnitTypes::Zerg_Nydus_Canal &&
           extraType == UnitTypes::Zerg_Nydus_Canal )
        bwgame.QueueCommand<BW::Orders::MakeNydusExit>(command.x, command.y);
      else if ( extraType.isAddon() )
        bwgame.QueueCommand<BW::Orders::MakeAddon>(command.x, command.y, extraType);
      else
        bwgame.QueueCommand<BW::Orders::MakeBuilding>(command.x, command.y, extraType);
    }
    else if ( ct == UnitCommandTypes::Build_Addon && command.unit )
    {
      TilePosition target = command.unit->getTilePosition() + TilePosition(4, 1);
      bwgame.QueueCommand<BW::Orders::MakeAddon>(BW::TilePosition(target.makeValid()), command.getUnitType());
    }
    else if ( ct == UnitCommandTypes::Train )
    {
      UnitType type1(command.extra);
      switch ( command.unit ? command.unit->getType() : UnitTypes::None )
      {
      case UnitTypes::Enum::Zerg_Larva:
      case UnitTypes::Enum::Zerg_Mutalisk:
      case UnitTypes::Enum::Zerg_Hydralisk:
        bwgame.QueueCommand<BW::Orders::UnitMorph>(type1);
        break;
      case UnitTypes::Enum::Zerg_Hatchery:
      case UnitTypes::Enum::Zerg_Lair:
      case UnitTypes::Enum::Zerg_Spire:
      case UnitTypes::Enum::Zerg_Creep_Colony:
        bwgame.QueueCommand<BW::Orders::BuildingMorph>(type1);
        break;
      case UnitTypes::Enum::Protoss_Carrier:
      case UnitTypes::Enum::Hero_Gantrithor:
      case UnitTypes::Enum::Protoss_Reaver:
      case UnitTypes::Enum::Hero_Warbringer:
        bwgame.QueueCommand<BW::Orders::TrainFighter>();
        break;
      default:
        bwgame.QueueCommand<BW::Orders::TrainUnit>(type1);
        break;
      }
    }
    else if (ct == UnitCommandTypes::Morph)
    {
      UnitType type(command.extra);
      if ( type.isBuilding() )
        bwgame.QueueCommand<BW::Orders::BuildingMorph>(type);
      else
        bwgame.QueueCommand<BW::Orders::UnitMorph>(type);
    }
    else if (ct == UnitCommandTypes::Research)
      bwgame.QueueCommand<BW::Orders::Invent>(command.getTechType());
    else if (ct == UnitCommandTypes::Upgrade)
      bwgame.QueueCommand<BW::Orders::Upgrade>(command.getUpgradeType());
    else if (ct == UnitCommandTypes::Set_Rally_Position)
      bwgame.QueueCommand<BW::Orders::Attack>(command.x, command.y, Orders::Enum::RallyPointTile);
    else if (ct == UnitCommandTypes::Set_Rally_Unit)
      bwgame.QueueCommand<BW::Orders::Attack>(command.target, Orders::Enum::RallyPointUnit);
    else if (ct == UnitCommandTypes::Move)
      bwgame.QueueCommand<BW::Orders::Attack>(command.x, command.y, Orders::Enum::Move, queued);
    else if (ct == UnitCommandTypes::Patrol)
      bwgame.QueueCommand<BW::Orders::Attack>(command.x, command.y, Orders::Enum::Patrol, queued);
    else if (ct == UnitCommandTypes::Hold_Position)
      bwgame.QueueCommand<BW::Orders::HoldPosition>(queued);
    else if (ct == UnitCommandTypes::Stop)
    {
      switch ( command.unit ? command.unit->getType() : UnitTypes::None )
      {
      case UnitTypes::Enum::Protoss_Reaver:
      case UnitTypes::Enum::Hero_Warbringer:
        bwgame.QueueCommand<BW::Orders::ReaverStop>();
        break;
      case UnitTypes::Enum::Protoss_Carrier:
      case UnitTypes::Enum::Hero_Gantrithor:
        bwgame.QueueCommand<BW::Orders::CarrierStop>();
        break;
      default:
        bwgame.QueueCommand<BW::Orders::Stop>(queued);
        break;
      }
    }
    else if (ct == UnitCommandTypes::Follow)
      bwgame.QueueCommand<BW::Orders::Attack>(command.target, Orders::Enum::Follow, queued);
    else if (ct == UnitCommandTypes::Gather)
      bwgame.QueueCommand<BW::Orders::Attack>(command.target, Orders::Enum::Harvest1, queued);
    else if (ct == UnitCommandTypes::Return_Cargo)
      bwgame.QueueCommand<BW::Orders::ReturnCargo>(queued);
    else if (ct == UnitCommandTypes::Repair)
      bwgame.QueueCommand<BW::Orders::Attack>(command.target, Orders::Enum::Repair, queued);
    else if (ct == UnitCommandTypes::Burrow)
      bwgame.QueueCommand<BW::Orders::Burrow>();
    else if (ct == UnitCommandTypes::Unburrow)
      bwgame.QueueCommand<BW::Orders::Unburrow>();
    else if (ct == UnitCommandTypes::Cloak)
      bwgame.QueueCommand<BW::Orders::Cloak>();
    else if (ct == UnitCommandTypes::Decloak)
      bwgame.QueueCommand<BW::Orders::Decloak>();
    else if (ct == UnitCommandTypes::Siege)
      bwgame.QueueCommand<BW::Orders::Siege>();
    else if (ct == UnitCommandTypes::Unsiege)
      bwgame.QueueCommand<BW::Orders::Unsiege>();
    else if (ct == UnitCommandTypes::Lift)
      bwgame.QueueCommand<BW::Orders::Lift>();
    else if (ct == UnitCommandTypes::Land)
    {
      if (command.unit)
        bwgame.QueueCommand<BW::Orders::Land>(command.x, command.y, command.unit->getType());
    }
    else if (ct == UnitCommandTypes::Load)
    {
      BWAPI::UnitType thisType = command.unit ? command.unit->getType() : UnitTypes::None;
      if ( thisType == UnitTypes::Terran_Bunker )
        bwgame.QueueCommand<BW::Orders::Attack>(command.target, Orders::Enum::PickupBunker, queued);
      else if ( thisType == UnitTypes::Terran_Dropship || 
                thisType == UnitTypes::Protoss_Shuttle || 
                thisType == UnitTypes::Zerg_Overlord   ||
                thisType == UnitTypes::Hero_Yggdrasill )
        bwgame.QueueCommand<BW::Orders::Attack>(command.target, Orders::Enum::PickupTransport, queued);
      else if ( command.target->getType() == UnitTypes::Terran_Bunker   ||
                command.target->getType() == UnitTypes::Terran_Dropship ||
                command.target->getType() == UnitTypes::Protoss_Shuttle ||
                command.target->getType() == UnitTypes::Zerg_Overlord   ||
                command.target->getType() == UnitTypes::Hero_Yggdrasill )
        bwgame.QueueCommand<BW::Orders::RightClick>(command.target, queued);
    }
    else if (ct == UnitCommandTypes::Unload)
    {
      bwgame.QueueCommand<BW::Orders::UnloadUnit>(command.target);
    }
    else if (ct == UnitCommandTypes::Unload_All && command.unit)
    {
      if ( command.unit->getType() == UnitTypes::Terran_Bunker )
        bwgame.QueueCommand<BW::Orders::UnloadAll>();
      else
        bwgame.QueueCommand<BW::Orders::Attack>(command.unit->getPosition(), Orders::Enum::MoveUnload, queued);
    }
    else if (ct == UnitCommandTypes::Unload_All_Position)
    {
      bwgame.QueueCommand<BW::Orders::Attack>(command.x, command.y, Orders::Enum::MoveUnload, queued);
    }
    else if (ct == UnitCommandTypes::Right_Click_Position)
      bwgame.QueueCommand<BW::Orders::RightClick>(command.x, command.y, queued);
    else if (ct == UnitCommandTypes::Right_Click_Unit)
      bwgame.QueueCommand<BW::Orders::RightClick>(command.target, queued);
    else if (ct == UnitCommandTypes::Halt_Construction)
      bwgame.QueueCommand<BW::Orders::Stop>();
    else if (ct == UnitCommandTypes::Cancel_Construction)
      bwgame.QueueCommand<BW::Orders::CancelConstruction>();
    else if (ct == UnitCommandTypes::Cancel_Addon)
      bwgame.QueueCommand<BW::Orders::CancelAddon>();
    else if (ct == UnitCommandTypes::Cancel_Train || ct == UnitCommandTypes::Cancel_Train_Slot)
      bwgame.QueueCommand<BW::Orders::CancelTrain>(command.extra);
    else if (ct == UnitCommandTypes::Cancel_Morph)
    {
      if ( command.unit && command.unit->getType().isBuilding() )
        bwgame.QueueCommand<BW::Orders::CancelConstruction>();
      else
        bwgame.QueueCommand<BW::Orders::CancelUnitMorph>();
    }
    else if (ct == UnitCommandTypes::Cancel_Research)
      bwgame.QueueCommand<BW::Orders::CancelResearch>();
    else if (ct == UnitCommandTypes::Cancel_Upgrade)
      bwgame.QueueCommand<BW::Orders::CancelUpgrade>();
    else if (ct == UnitCommandTypes::Use_Tech)
    {
      TechType tech(command.extra);
      switch (tech)
      {
        case TechTypes::Enum::Stim_Packs:
          bwgame.QueueCommand<BW::Orders::UseStimPack>();
          break;
        case TechTypes::Enum::Tank_Siege_Mode:
          if ( command.unit && command.unit->isSieged() )
            bwgame.QueueCommand<BW::Orders::Unsiege>();
          else
            bwgame.QueueCommand<BW::Orders::Siege>();
          break;
        case TechTypes::Enum::Personnel_Cloaking:
        case TechTypes::Enum::Cloaking_Field:
          if ( command.unit && command.unit->isCloaked() )
            bwgame.QueueCommand<BW::Orders::Decloak>();
          else
            bwgame.QueueCommand<BW::Orders::Cloak>();
          break;
        case TechTypes::Enum::Burrowing:
          if ( command.unit && command.unit->isBurrowed() )
            bwgame.QueueCommand<BW::Orders::Unburrow>();
          else
            bwgame.QueueCommand<BW::Orders::Burrow>();
          break;
      }
    }
    else if (ct == UnitCommandTypes::Use_Tech_Position)
    {
      Order order = (command.getTechType() == TechTypes::Healing ? Orders::HealMove : command.getTechType().getOrder());
      bwgame.QueueCommand<BW::Orders::Attack>(command.x, command.y, order);
    }
    else if (ct == UnitCommandTypes::Use_Tech_Unit)
    {
      TechType tech(command.extra);
      if (tech == TechTypes::Archon_Warp)
        bwgame.QueueCommand<BW::Orders::MergeArchon>();
      else if (tech == TechTypes::Dark_Archon_Meld)
        bwgame.QueueCommand<BW::Orders::MergeDarkArchon>();
      else
        bwgame.QueueCommand<BW::Orders::Attack>(command.target, tech.getOrder());
    }
    else if ( ct == UnitCommandTypes::Place_COP && command.unit )
      bwgame.QueueCommand<BW::Orders::PlaceCOP>(command.x, command.y, command.unit->getType());
  }

}
