#pragma once
#include <string>
#include <vector>
#include <Columns.h>
#include <Rows.h>

class Table
{
private:
    std::string name;
    std::vector<Column> cols;
    std::vector<Rows> rows;

public:
    Table();
    ~Table();
};