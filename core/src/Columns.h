#pragma once
#include <string>
#include <vector>
#include "types.h"

class Column
{
private:
    std::string name;
    std::string type;

    // Validate data type
    bool isValidType(const std::string &dataType) const;

public:
    // Constructor
    Column(const std::string &name, const std::string &type);

    // Getters (const)
    std::string getName() const;
    std::string getType() const;

    // Setters
    void setName(const std::string &newName);
    void setType(const std::string &newType);

    // Utility
    bool isNumeric() const;
    bool isText() const;
    bool isBoolean() const;
};