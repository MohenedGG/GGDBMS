#pragma once
#include <string>
#include <vector>

class Rows
{
private:
    std::vector<std::string> values;

public:
    // Getters
    std::vector<std::string> getValues() const;
    const std::string& getValue(size_t index) const;
    const size_t& size() const;

    // Setters
    void setValues(const std::vector<std::string> &newValues);
    void setValue(size_t index, const std::string &value);

    // Add/Remove operations
    void addValue(const std::string &value);
    void removeValue(size_t index);
    void clearValues();

    // Check if empty
    bool isEmpty() const;
};