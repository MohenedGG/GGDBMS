#include "Table.h"
#include <algorithm>

Table::Table()
    : name("")
{
}

Table::Table(const std::string &name)
    : name(name)
{
}

Table::~Table()
{
}

bool Table::isValidRowSize(const Rows &row) const
{
    return row.size() == this->cols.size();
}

const std::string &Table::getName() const
{
    return this->name;
}

void Table::setName(const std::string &name)
{
    if (!name.empty())
    {
        this->name = name;
    }
}

const std::vector<Column> &Table::getColumns() const
{
    return this->cols;
}

const std::vector<Rows> &Table::getRows() const
{
    return this->rows;
}

bool Table::addColumn(const Column &column)
{
    if (column.getName().empty())
    {
        return false;
    }

    const auto it = std::find_if(this->cols.begin(), this->cols.end(),
                                 [&column](const Column &existingColumn)
                                 {
                                     return existingColumn.getName() == column.getName();
                                 });

    if (it != this->cols.end())
    {
        return false;
    }

    this->cols.push_back(column);
    return true;
}

bool Table::addRow(const Rows &row)
{
    if (!isValidRowSize(row))
    {
        return false;
    }

    this->rows.push_back(row);
    return true;
}

bool Table::removeColumn(size_t index)
{
    if (index >= this->cols.size())
    {
        return false;
    }

    this->cols.erase(this->cols.begin() + index);

    // Keep rows aligned with columns after deleting a column.
    for (Rows &row : this->rows)
    {
        if (index < row.size())
        {
            row.removeValue(index);
        }
    }

    return true;
}

bool Table::removeRow(size_t index)
{
    if (index >= this->rows.size())
    {
        return false;
    }

    this->rows.erase(this->rows.begin() + index);
    return true;
}

void Table::clearRows()
{
    this->rows.clear();
}

void Table::clearTable()
{
    this->clearRows();
    this->cols.clear();
}

size_t Table::columnCount() const
{
    return this->cols.size();
}

size_t Table::rowCount() const
{
    return this->rows.size();
}
