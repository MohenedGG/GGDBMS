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

bool Table::areUniqueConstraintsSatisfied(const Rows &row) const
{
    for (const std::string &columnName : this->uniqueColumnNames)
    {
        const size_t uniqueIndex = findColumnIndex(columnName);
        if (uniqueIndex >= this->cols.size() || uniqueIndex >= row.size())
        {
            return false;
        }

        const std::string uniqueValue = row.getValue(uniqueIndex);

        for (const Rows &existingRow : this->rows)
        {
            if (uniqueIndex < existingRow.size() && existingRow.getValue(uniqueIndex) == uniqueValue)
            {
                return false;
            }
        }
    }

    return true;
}

bool Table::areNotNullConstraintsSatisfied(const Rows &row) const
{
    for (const std::string &columnName : this->notNullColumnNames)
    {
        const size_t notNullIndex = findColumnIndex(columnName);
        if (notNullIndex >= this->cols.size() || notNullIndex >= row.size())
        {
            return false;
        }

        if (row.getValue(notNullIndex).empty())
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

    if (hasPrimaryKey() && this->primaryKeyColumnName != columnName)
    {
        this->uniqueColumnNames.erase(
            std::remove(this->uniqueColumnNames.begin(), this->uniqueColumnNames.end(), this->primaryKeyColumnName),
            this->uniqueColumnNames.end());

        this->notNullColumnNames.erase(
            std::remove(this->notNullColumnNames.begin(), this->notNullColumnNames.end(), this->primaryKeyColumnName),
            this->notNullColumnNames.end());
    }

    this->primaryKeyColumnName = columnName;

    if (!hasUniqueConstraint(columnName))
    {
        this->uniqueColumnNames.push_back(columnName);
    }

    if (!hasNotNullConstraint(columnName))
    {
        this->notNullColumnNames.push_back(columnName);
    }

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

bool Table::addUniqueConstraint(const std::string &columnName)
{
    const size_t uniqueIndex = findColumnIndex(columnName);
    if (uniqueIndex >= this->cols.size())
    {
        return false;
    }

    if (hasUniqueConstraint(columnName))
    {
        return false;
    }

    for (size_t i = 0; i < this->rows.size(); ++i)
    {
        for (size_t j = i + 1; j < this->rows.size(); ++j)
        {
            if (this->rows[i].getValue(uniqueIndex) == this->rows[j].getValue(uniqueIndex))
            {
                return false;
            }
        }
    }

    this->uniqueColumnNames.push_back(columnName);
    return true;
}

bool Table::hasUniqueConstraint(const std::string &columnName) const
{
    return std::find(this->uniqueColumnNames.begin(), this->uniqueColumnNames.end(), columnName) != this->uniqueColumnNames.end();
}

const std::vector<std::string> &Table::getUniqueColumns() const
{
    return this->uniqueColumnNames;
}

bool Table::addNotNullConstraint(const std::string &columnName)
{
    const size_t notNullIndex = findColumnIndex(columnName);
    if (notNullIndex >= this->cols.size())
    {
        return false;
    }

    if (hasNotNullConstraint(columnName))
    {
        return false;
    }

    for (const Rows &row : this->rows)
    {
        if (notNullIndex >= row.size() || row.getValue(notNullIndex).empty())
        {
            return false;
        }
    }

    this->notNullColumnNames.push_back(columnName);
    return true;
}

bool Table::hasNotNullConstraint(const std::string &columnName) const
{
    return std::find(this->notNullColumnNames.begin(), this->notNullColumnNames.end(), columnName) != this->notNullColumnNames.end();
}

const std::vector<std::string> &Table::getNotNullColumns() const
{
    return this->notNullColumnNames;
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

    if (!areUniqueConstraintsSatisfied(row))
    {
        return false;
    }

    if (!areNotNullConstraintsSatisfied(row))
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

    const std::string removedColumnName = this->cols[index].getName();

    this->cols.erase(this->cols.begin() + index);

    // Keep rows aligned with columns after deleting a column.
    for (Rows &row : this->rows)
    {
        if (index < row.size())
        {
            row.removeValue(index);
        }
    }

    if (hasPrimaryKey() && this->primaryKeyColumnName == removedColumnName)
    {
        this->primaryKeyColumnName.clear();
    }

    this->uniqueColumnNames.erase(
        std::remove(this->uniqueColumnNames.begin(), this->uniqueColumnNames.end(), removedColumnName),
        this->uniqueColumnNames.end());

    this->notNullColumnNames.erase(
        std::remove(this->notNullColumnNames.begin(), this->notNullColumnNames.end(), removedColumnName),
        this->notNullColumnNames.end());

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
    this->uniqueColumnNames.clear();
    this->notNullColumnNames.clear();
}

size_t Table::columnCount() const
{
    return this->cols.size();
}

size_t Table::rowCount() const
{
    return this->rows.size();
}
