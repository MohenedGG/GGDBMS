#include "Columns.h"
#include <algorithm>

// Private helper method
bool Column::isValidType(const std::string &dataType) const
{
    static const std::vector<std::string> validTypes = {
        "INT", "FLOAT", "TEXT", "BOOL",
        "int", "float", "text", "bool"};

    return std::find(validTypes.begin(), validTypes.end(), dataType) != validTypes.end();
}

// Constructor
Column::Column(const std::string &name, const std::string &type)
    : name(name)
{
    if (isValidType(type))
    {
        this->type = type;
    }
    else
    {
        // Default type if invalid type is provided
        this->type = "TEXT";
    }
}

// Getters
std::string Column::getName() const
{
    return name;
}

std::string Column::getType() const
{
    return type;
}

// Setters
void Column::setName(const std::string &newName)
{
    if (!newName.empty())
    {
        name = newName;
    }
}

void Column::setType(const std::string &newType)
{
    if (isValidType(newType))
    {
        type = newType;
    }
}

// Utility methods
bool Column::isNumeric() const
{
    return (type == "INT" || type == "int" || type == "FLOAT" || type == "float");
}

bool Column::isText() const
{
    return (type == "TEXT" || type == "text");
}

bool Column::isBoolean() const
{
    return (type == "BOOL" || type == "bool");
}
