#include "Database.h"
#include <cctype>

namespace
{
    bool equalsIgnoreCase(const std::string &lhs, const std::string &rhs)
    {
        if (lhs.size() != rhs.size())
        {
            return false;
        }

        for (size_t i = 0; i < lhs.size(); ++i)
        {
            const unsigned char left = static_cast<unsigned char>(lhs[i]);
            const unsigned char right = static_cast<unsigned char>(rhs[i]);
            if (std::toupper(left) != std::toupper(right))
            {
                return false;
            }
        }

        return true;
    }
} // namespace

Database::Database()
    : name("")
{
}

Database::Database(const std::string &name)
    : name(name)
{
}

Database::~Database()
{
}

size_t Database::findTableIndex(const std::string &tableName) const
{
    for (size_t i = 0; i < this->tables.size(); ++i)
    {
        if (this->tables[i].getName() == tableName)
        {
            return i;
        }
    }

    return kNotFound;
}

size_t Database::findColumnIndex(const Table &table, const std::string &columnName) const
{
    const std::vector<Column> &columns = table.getColumns();
    for (size_t i = 0; i < columns.size(); ++i)
    {
        if (columns[i].getName() == columnName)
        {
            return i;
        }
    }

    return kNotFound;
}

bool Database::hasValueInColumn(const Table &table, size_t columnIndex, const std::string &value) const
{
    const std::vector<Rows> &rows = table.getRows();
    for (const Rows &row : rows)
    {
        const std::vector<std::string> values = row.getValues();
        if (columnIndex < values.size() && values[columnIndex] == value)
        {
            return true;
        }
    }

    return false;
}

bool Database::canInsertWithRelations(const std::string &tableName, const Rows &row) const
{
    const Table *childTable = getTable(tableName);
    if (childTable == nullptr)
    {
        return false;
    }

    const std::vector<std::string> rowValues = row.getValues();

    for (const ForeignKey &foreignKey : this->foreignKeys)
    {
        if (foreignKey.childTableName != tableName)
        {
            continue;
        }

        const size_t childColumnIndex = findColumnIndex(*childTable, foreignKey.childColumnName);
        if (childColumnIndex == kNotFound || childColumnIndex >= rowValues.size())
        {
            return false;
        }

        const std::string &childValue = rowValues[childColumnIndex];
        if (childValue.empty())
        {
            continue;
        }

        const Table *parentTable = getTable(foreignKey.parentTableName);
        if (parentTable == nullptr)
        {
            return false;
        }

        const size_t parentColumnIndex = findColumnIndex(*parentTable, foreignKey.parentColumnName);
        if (parentColumnIndex == kNotFound)
        {
            return false;
        }

        if (!hasValueInColumn(*parentTable, parentColumnIndex, childValue))
        {
            return false;
        }
    }

    return true;
}

const std::string &Database::getName() const
{
    return this->name;
}

void Database::setName(const std::string &newName)
{
    if (!newName.empty())
    {
        this->name = newName;
    }
}

bool Database::createTable(const std::string &tableName)
{
    if (tableName.empty() || hasTable(tableName))
    {
        return false;
    }

    this->tables.emplace_back(tableName);
    return true;
}

bool Database::createTable(const Table &table)
{
    if (table.getName().empty() || hasTable(table.getName()))
    {
        return false;
    }

    this->tables.push_back(table);
    return true;
}

bool Database::dropTable(const std::string &tableName)
{
    const size_t index = findTableIndex(tableName);
    if (index == kNotFound)
    {
        return false;
    }

    for (const ForeignKey &foreignKey : this->foreignKeys)
    {
        if (foreignKey.parentTableName == tableName || foreignKey.childTableName == tableName)
        {
            return false;
        }
    }

    this->tables.erase(this->tables.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

bool Database::insertRow(const std::string &tableName, const Rows &row)
{
    Table *table = getTable(tableName);
    if (table == nullptr)
    {
        return false;
    }

    if (!canInsertWithRelations(tableName, row))
    {
        return false;
    }

    return table->addRow(row);
}

bool Database::deleteRow(const std::string &tableName, size_t rowIndex)
{
    Table *parentTable = getTable(tableName);
    if (parentTable == nullptr || rowIndex >= parentTable->rowCount())
    {
        return false;
    }

    const std::vector<std::string> parentRowValues = parentTable->getRows()[rowIndex].getValues();

    for (const ForeignKey &foreignKey : this->foreignKeys)
    {
        if (foreignKey.parentTableName != tableName)
        {
            continue;
        }

        const size_t parentColumnIndex = findColumnIndex(*parentTable, foreignKey.parentColumnName);
        if (parentColumnIndex == kNotFound || parentColumnIndex >= parentRowValues.size())
        {
            return false;
        }

        const std::string parentValue = parentRowValues[parentColumnIndex];
        Table *childTable = getTable(foreignKey.childTableName);
        if (childTable == nullptr)
        {
            return false;
        }

        const size_t childColumnIndex = findColumnIndex(*childTable, foreignKey.childColumnName);
        if (childColumnIndex == kNotFound)
        {
            return false;
        }

        for (size_t i = childTable->rowCount(); i > 0; --i)
        {
            const size_t childRowIndex = i - 1;
            const std::vector<std::string> childRowValues = childTable->getRows()[childRowIndex].getValues();
            if (childColumnIndex >= childRowValues.size())
            {
                continue;
            }

            if (childRowValues[childColumnIndex] != parentValue)
            {
                continue;
            }

            if (foreignKey.onDelete == ReferentialAction::RESTRICT)
            {
                return false;
            }

            if (!deleteRow(foreignKey.childTableName, childRowIndex))
            {
                return false;
            }
        }
    }

    return parentTable->removeRow(rowIndex);
}

bool Database::hasTable(const std::string &tableName) const
{
    return findTableIndex(tableName) != kNotFound;
}

bool Database::addForeignKey(const ForeignKey &foreignKey)
{
    return addForeignKey(foreignKey.childTableName,
                         foreignKey.childColumnName,
                         foreignKey.parentTableName,
                         foreignKey.parentColumnName,
                         foreignKey.onDelete);
}

bool Database::addForeignKey(const std::string &childTableName,
                             const std::string &childColumnName,
                             const std::string &parentTableName,
                             const std::string &parentColumnName,
                             ReferentialAction onDelete)
{
    if (childTableName.empty() || childColumnName.empty() ||
        parentTableName.empty() || parentColumnName.empty())
    {
        return false;
    }

    const Table *childTable = getTable(childTableName);
    const Table *parentTable = getTable(parentTableName);
    if (childTable == nullptr || parentTable == nullptr)
    {
        return false;
    }

    const size_t childColumnIndex = findColumnIndex(*childTable, childColumnName);
    const size_t parentColumnIndex = findColumnIndex(*parentTable, parentColumnName);
    if (childColumnIndex == kNotFound || parentColumnIndex == kNotFound)
    {
        return false;
    }

    if (!parentTable->hasPrimaryKey() || parentTable->getPrimaryKeyColumnName() != parentColumnName)
    {
        return false;
    }

    const std::string &childType = childTable->getColumns()[childColumnIndex].getType();
    const std::string &parentType = parentTable->getColumns()[parentColumnIndex].getType();
    if (!equalsIgnoreCase(childType, parentType))
    {
        return false;
    }

    for (const ForeignKey &foreignKey : this->foreignKeys)
    {
        if (foreignKey.childTableName == childTableName &&
            foreignKey.childColumnName == childColumnName &&
            foreignKey.parentTableName == parentTableName &&
            foreignKey.parentColumnName == parentColumnName)
        {
            return false;
        }
    }

    this->foreignKeys.push_back(ForeignKey{childTableName, childColumnName, parentTableName, parentColumnName, onDelete});
    return true;
}

Table *Database::getTable(const std::string &tableName)
{
    const size_t index = findTableIndex(tableName);
    if (index == kNotFound)
    {
        return nullptr;
    }

    return &this->tables[index];
}

const Table *Database::getTable(const std::string &tableName) const
{
    const size_t index = findTableIndex(tableName);
    if (index == kNotFound)
    {
        return nullptr;
    }

    return &this->tables[index];
}

const std::vector<Table> &Database::getTables() const
{
    return this->tables;
}

const std::vector<ForeignKey> &Database::getForeignKeys() const
{
    return this->foreignKeys;
}

std::vector<std::string> Database::listTableNames() const
{
    std::vector<std::string> names;
    names.reserve(this->tables.size());

    for (const Table &table : this->tables)
    {
        names.push_back(table.getName());
    }

    return names;
}

size_t Database::tableCount() const
{
    return this->tables.size();
}

void Database::clearDatabase()
{
    this->foreignKeys.clear();
    this->tables.clear();
}
