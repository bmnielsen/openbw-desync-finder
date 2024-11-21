#pragma once

#include "BWAPI.h"
#include <span>

class BulletData
{
public:
    int id;

    int typeIndex;
    int positionX;
    int positionY;
    int velocityX;
    int velocityY;
    int angle;

    BulletData(int id,
             int typeIndex,
             int positionX,
             int positionY,
             int velocityX,
             int velocityY,
             int angle)
            : id(id)
            , typeIndex(typeIndex)
            , positionX(positionX)
            , positionY(positionY)
            , velocityX(velocityX)
            , velocityY(velocityY)
            , angle(angle)
    {
    }

    explicit BulletData(BWAPI::Bullet bullet)
        : id(bullet->getID())
        , typeIndex((int)bullet->getType())
        , positionX(bullet->getPosition().x)
        , positionY(bullet->getPosition().y)
        , velocityX((int)std::round(bullet->getVelocityX() * 1000.0))
        , velocityY((int)std::round(bullet->getVelocityY() * 1000.0))
        , angle((int)std::round(bullet->getAngle() * 1000.0))
    {
    }

    bool shouldSkip() const
    {
        return false;
    }

    int delta(const BulletData &other) const
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

        return result;
    }

    std::string differences(const BulletData &other) const
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

        return builder.str();
    }

    void outputToCSV(std::ofstream &file) const
    {
        file << id << ";"
             << typeIndex << ";"
             << positionX << ";"
             << positionY << ";"
             << velocityX << ";"
             << velocityY << ";"
             << angle;
    }

    static void outputCSVHeader(std::ofstream &file)
    {
        file << "ID;TypeIdx;PosX;PosY;vX;vY;Angle";
    }

    static bool parseCSVLineAndEmplace(const std::span<std::string> &line,
                                       std::list<BulletData> &bulletData,
                                       int lineNumber)
    {
        if (line.size() < 7)
        {
            std::cout << "ERROR: Not enough fields"
                      << " at line " << lineNumber << std::endl;
            return false;
        }

        bulletData.emplace_back(
                std::stoi(line[0]),
                std::stoi(line[1]),
                std::stoi(line[2]),
                std::stoi(line[3]),
                std::stoi(line[4]),
                std::stoi(line[5]),
                std::stoi(line[6]));

        return true;
    }

    static bool splitByPlayer()
    {
        return false;
    }

    friend std::ostream &operator<<(std::ostream &os, const BulletData &bulletData)
    {
        os << "\n" << "id=" << bulletData.id
           << "\n" << "type=" << (BWAPI::BulletType)bulletData.typeIndex
           << "\n" << "positionX=" << bulletData.positionX
           << "\n" << "positionY=" << bulletData.positionY
           << "\n" << "velocityX=" << bulletData.velocityX
           << "\n" << "velocityY=" << bulletData.velocityY
           << "\n" << "angle=" << bulletData.angle;
        return os;
    }
};
