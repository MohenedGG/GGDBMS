#include "Database.h"

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

    this->tables.erase(this->tables.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

bool Database::hasTable(const std::string &tableName) const
{
    return findTableIndex(tableName) != kNotFound;
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
    return this->getTable(tableName);
}

const std::vector<Table> &Database::getTables() const
{
    return this->tables;
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
    this->tables.clear();
}
