#include "Rows.h"

// Getters
std::vector<std::string> Rows::getValues() const
{
    return this->values;
}

std::string Rows::getValue(size_t index) const
{
    if (index < this->values.size())
    {
        return this->values[index];
    }
    return std::string();
}

size_t Rows::size() const
{
    return this->values.size();
}

// Setters
void Rows::setValues(const std::vector<std::string> &values)
{
    this->values = values;
}

void Rows::setValue(size_t index, const std::string &value)
{
    if (index < this->values.size())
    {
        this->values[index] = value;
    }
}

void Rows::addValue(const std::string &value)
{
    this->values.push_back(value);
}

void Rows::removeValue(size_t index)
{
    if (index < this->values.size())
    {
        this->values.erase(this->values.begin() + index);
    }
}

void Rows::clearValues()
{
    this->values.clear();
}

bool Rows::isEmpty() const
{
    return this->values.empty();
}
