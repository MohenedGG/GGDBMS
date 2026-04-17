#pragma once
#include <string>
#include <vector>

class Column
{
private:
    std::string name;
    std::string type;

public:
    Column(std::string name, std::string type);
    std::string getName();
    std::string getType();
};