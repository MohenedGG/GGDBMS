#pragma once
#include <cstddef>
#include <string>
#include <vector>
#include "Table.h"

class Database
{
private:
    std::string name;
    std::vector<Table> tables;

    static constexpr size_t kNotFound = static_cast<size_t>(-1);
    size_t findTableIndex(const std::string &tableName) const;

public:
    Database();
    explicit Database(const std::string &name);
    ~Database();

    const std::string &getName() const;
    void setName(const std::string &newName);

    bool createTable(const std::string &tableName);
    bool createTable(const Table &table);
    bool dropTable(const std::string &tableName);

    bool hasTable(const std::string &tableName) const;

    Table *getTable(const std::string &tableName);
    const Table *getTable(const std::string &tableName) const;

    const std::vector<Table> &getTables() const;
    std::vector<std::string> listTableNames() const;

    size_t tableCount() const;
    void clearDatabase();
};
