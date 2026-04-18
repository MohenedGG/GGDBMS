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
const std::string& Column::getName() const
{
    return this->name;
}

const std::string& Column::getType() const
{
    return this->type;
}

// Setters
void Column::setName(const std::string &name)
{
    if (!name.empty())
    {
        this->name = name;
    }
}

void Column::setType(const std::string &type)
{
    if (isValidType(type))
    {
        this->type = type;
    }
}

// Utility methods
bool Column::isNumeric() const
{
    return (this->type == "INT" || this->type == "int" || this->type == "FLOAT" || this->type == "float");
}

bool Column::isText() const
{
    return (this->type == "TEXT" || this->type == "text");
}

bool Column::isBoolean() const
{
    return (this->type == "BOOL" || this->type == "bool");
}
