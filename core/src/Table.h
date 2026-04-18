#pragma once
#include <string>
#include <vector>
#include "Columns.h"
#include "Rows.h"

class Table
{
private:
    std::string name;
    std::vector<Column> cols;
    std::vector<Rows> rows;

    bool isValidRowSize(const Rows &row) const;

public:
    Table();
    explicit Table(const std::string &name);
    ~Table();

    const std::string &getName() const;
    void setName(const std::string &newName);

    const std::vector<Column> &getColumns() const;
    const std::vector<Rows> &getRows() const;

    bool addColumn(const Column &column);
    bool addRow(const Rows &row);

    bool removeColumn(size_t index);
    bool removeRow(size_t index);

    void clearRows();
    void clearTable();

    size_t columnCount() const;
    size_t rowCount() const;
};