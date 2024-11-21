#pragma once

#include "BWAPI.h"
#include <span>

class UnitData
{
public:
    int id;
    int typeIndex;
    int orderIndex;
    int orderTimer;
    int positionX;
    int positionY;
    int velocityX;
    int velocityY;
    int angle;
    bool isBurrowed;
    int hitPoints;
    int shields;
    int cooldown;

    UnitData(int id,
             int typeIndex,
             int orderIndex,
             int orderTimer,
             int positionX,
             int positionY,
             int velocityX,
             int velocityY,
             int angle,
             bool isBurrowed,
             int hitPoints,
             int shields,
             int cooldown)
            : id(id)
            , typeIndex(typeIndex)
            , orderIndex(orderIndex)
            , orderTimer(orderTimer)
            , positionX(positionX)
            , positionY(positionY)
            , velocityX(velocityX)
            , velocityY(velocityY)
            , angle(angle)
            , isBurrowed(isBurrowed)
            , hitPoints(hitPoints)
            , shields(shields)
            , cooldown(cooldown)
    {
    }

    explicit UnitData(BWAPI::Unit unit)
        : id(unit->getID())
        , typeIndex((int)unit->getType())
        , orderIndex((int)unit->getOrder())
        , orderTimer(unit->getOrderTimer())
        , positionX(unit->getPosition().x)
        , positionY(unit->getPosition().y)
        , velocityX((int)std::round(unit->getVelocityX() * 1000.0))
        , velocityY((int)std::round(unit->getVelocityY() * 1000.0))
        , angle((int)std::round(unit->getAngle() * 1000.0))
        , isBurrowed(unit->isBurrowed())
        , hitPoints(unit->getHitPoints())
        , shields(unit->getShields())
        , cooldown((unit->getAirWeaponCooldown() > unit->getGroundWeaponCooldown()) ? unit->getAirWeaponCooldown() : unit->getGroundWeaponCooldown())
    {
    }

    bool shouldSkip() const
    {
        return false;
    }

    int delta(const UnitData &other) const
    {
        int result = 0;
        auto add = [&result](int first, int second)
        {
            result += std::abs(first - second);
        };

        add(positionX, other.positionX);
        add(positionY, other.positionY);
        add(velocityX, other.velocityX);
        add(velocityY, other.velocityY);
        add(angle, other.angle);
        add(isBurrowed ? 32 : 0, other.isBurrowed ? 32 : 0);
        add(hitPoints, other.hitPoints);
        add(shields, other.shields);
        add(cooldown, other.cooldown);

        return result;
    }

    std::string differences(const UnitData &other) const
    {
        std::ostringstream builder;
        std::string sep;

        auto add = [&builder, &sep](const std::string &label, auto first, auto second)
        {
            builder << sep << label << ": " << first << "!=" << second;
            sep = ", ";
        };

        if (positionX != other.positionX) add("positionX", positionX, other.positionX);
        if (positionY != other.positionY) add("positionY", positionY, other.positionY);
        if (velocityX != other.velocityX) add("velocityX", velocityX, other.velocityX);
        if (velocityY != other.velocityY) add("velocityY", velocityY, other.velocityY);
        if (angle != other.angle) add("angle", angle, other.angle);
        if (isBurrowed != other.isBurrowed) add("isBurrowed", isBurrowed, other.isBurrowed);
        if (hitPoints != other.hitPoints) add("hitPoints", hitPoints, other.hitPoints);
        if (shields != other.shields) add("shields", shields, other.shields);
        if (cooldown != other.cooldown) add("cooldown", cooldown, other.cooldown);

        return builder.str();
    }

    void outputToCSV(std::ofstream &file) const
    {
        file << id << ";"
             << typeIndex << ";"
             << orderIndex << ";"
             << orderTimer << ";"
             << positionX << ";"
             << positionY << ";"
             << velocityX << ";"
             << velocityY << ";"
             << angle << ";"
             << (int)isBurrowed << ";"
             << hitPoints << ";"
             << shields << ";"
             << cooldown;
    }

    static void outputCSVHeader(std::ofstream &file)
    {
        file << "ID;TypeIdx;OrderIdx;OrderTmr;PosX;PosY;vX;vY;Angle;Burrowed;HP;Shields;Cooldown";
    }

    static bool parseCSVLineAndEmplace(const std::span<std::string> &line,
                                       std::list<UnitData> &unitData,
                                       int lineNumber)
    {
        if (line.size() < 13)
        {
            std::cout << "ERROR: Not enough fields"
                      << " at line " << lineNumber << std::endl;
            return false;
        }

        unitData.emplace_back(
                std::stoi(line[0]),
                std::stoi(line[1]),
                std::stoi(line[2]),
                std::stoi(line[3]),
                std::stoi(line[4]),
                std::stoi(line[5]),
                std::stoi(line[6]),
                std::stoi(line[7]),
                std::stoi(line[8]),
                std::stoi(line[9]) != 0,
                std::stoi(line[10]),
                std::stoi(line[11]),
                std::stoi(line[12]));

        return true;
    }

    static bool splitByPlayer()
    {
        return true;
    }

    friend std::ostream &operator<<(std::ostream &os, const UnitData &unitData)
    {
        os << "\n" << "id=" << unitData.id
           << "\n" << "type=" << (BWAPI::UnitType)unitData.typeIndex
           << "\n" << "order=" << (BWAPI::Order)unitData.orderIndex
           << "\n" << "orderTimer=" << unitData.orderTimer
           << "\n" << "positionX=" << unitData.positionX
           << "\n" << "positionY=" << unitData.positionY
           << "\n" << "velocityX=" << unitData.velocityX
           << "\n" << "velocityY=" << unitData.velocityY
           << "\n" << "angle=" << unitData.angle
           << "\n" << "isBurrowed=" << (unitData.isBurrowed ? "true" : "false")
           << "\n" << "hitPoints=" << unitData.hitPoints
           << "\n" << "shields=" << unitData.shields
           << "\n" << "cooldown=" << unitData.cooldown;
        return os;
    }
};
