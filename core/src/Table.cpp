#include "Table.h"
#include <algorithm>

Table::Table()
    : name(""), primaryKeyColumnName("")
{
}

Table::Table(const std::string &name)
    : name(name), primaryKeyColumnName("")
{
}

Table::~Table()
{
}

bool Table::isValidRowSize(const Rows &row) const
{
    return row.size() == this->cols.size();
}

size_t Table::findColumnIndex(const std::string &columnName) const
{
    for (size_t i = 0; i < this->cols.size(); ++i)
    {
        if (this->cols[i].getName() == columnName)
        {
            return i;
        }
    }

    return this->cols.size();
}

bool Table::isPrimaryKeyValueUnique(const Rows &row) const
{
    if (!hasPrimaryKey())
    {
        return true;
    }

    const size_t pkIndex = getPrimaryKeyColumnIndex();
    if (pkIndex >= row.size())
    {
        return false;
    }

    const std::string pkValue = row.getValue(pkIndex);
    if (pkValue.empty())
    {
        return false;
    }

    for (const Rows &existingRow : this->rows)
    {
        if (pkIndex < existingRow.size() && existingRow.getValue(pkIndex) == pkValue)
        {
            return false;
        }
    }

    return true;
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

bool Table::setPrimaryKey(const std::string &columnName)
{
    const size_t pkIndex = findColumnIndex(columnName);
    if (pkIndex >= this->cols.size())
    {
        return false;
    }

    for (const Rows &row : this->rows)
    {
        if (pkIndex >= row.size() || row.getValue(pkIndex).empty())
        {
            return false;
        }
    }

    for (size_t i = 0; i < this->rows.size(); ++i)
    {
        for (size_t j = i + 1; j < this->rows.size(); ++j)
        {
            if (this->rows[i].getValue(pkIndex) == this->rows[j].getValue(pkIndex))
            {
                return false;
            }
        }
    }

    this->primaryKeyColumnName = columnName;
    return true;
}

bool Table::hasPrimaryKey() const
{
    return !this->primaryKeyColumnName.empty();
}

const std::string &Table::getPrimaryKeyColumnName() const
{
    return this->primaryKeyColumnName;
}

size_t Table::getPrimaryKeyColumnIndex() const
{
    if (!hasPrimaryKey())
    {
        return this->cols.size();
    }

    return findColumnIndex(this->primaryKeyColumnName);
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

    if (!isPrimaryKeyValueUnique(row))
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

    if (hasPrimaryKey())
    {
        const size_t pkIndex = findColumnIndex(this->primaryKeyColumnName);
        if (pkIndex >= this->cols.size())
        {
            this->primaryKeyColumnName.clear();
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
    this->primaryKeyColumnName.clear();
}

size_t Table::columnCount() const
{
    return this->cols.size();
}

size_t Table::rowCount() const
{
    return this->rows.size();
}
