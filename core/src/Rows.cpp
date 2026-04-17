#include "Rows.h"

// Getters
std::vector<std::string> Rows::getValues() const
{
    return values;
}

std::string Rows::getValue(size_t index) const
{
    if (index < values.size())
    {
        return values[index];
    }
    return "";
}

size_t Rows::size() const
{
    return values.size();
}

// Setters
void Rows::setValues(const std::vector<std::string> &newValues)
{
    values = newValues;
}

void Rows::setValue(size_t index, const std::string &value)
{
    if (index < values.size())
    {
        values[index] = value;
    }
}

void Rows::addValue(const std::string &value)
{
    values.push_back(value);
}

void Rows::removeValue(size_t index)
{
    if (index < values.size())
    {
        values.erase(values.begin() + index);
    }
}

void Rows::clearValues()
{
    values.clear();
}

bool Rows::isEmpty() const
{
    return values.empty();
}
