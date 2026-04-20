#pragma once
#include <string>
#include "Database.h"

class DatabasePersistence
{
public:
    static bool saveToFile(const Database &database, const std::string &filePath);
    static bool loadFromFile(const std::string &filePath, Database &database);
};
