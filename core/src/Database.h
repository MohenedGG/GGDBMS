#pragma once
#include <cstddef>
#include <string>
#include <vector>
#include "Table.h"
#include "types.h"

class Database
{
private:
    std::string name;
    std::vector<Table> tables;
    std::vector<ForeignKey> foreignKeys;

    static constexpr size_t kNotFound = static_cast<size_t>(-1);

    size_t findTableIndex(const std::string &tableName) const;
    size_t findColumnIndex(const Table &table, const std::string &columnName) const;
    bool hasValueInColumn(const Table &table, size_t columnIndex, const std::string &value) const;
    bool canInsertWithRelations(const std::string &tableName, const Rows &row) const;

public:
    Database();
    explicit Database(const std::string &name);
    ~Database();

    const std::string &getName() const;
    void setName(const std::string &newName);

    bool createTable(const std::string &tableName);
    bool createTable(const Table &table);
    bool dropTable(const std::string &tableName);
    bool insertRow(const std::string &tableName, const Rows &row);
    bool deleteRow(const std::string &tableName, size_t rowIndex);

    bool hasTable(const std::string &tableName) const;
    bool addForeignKey(const ForeignKey &foreignKey);
    bool addForeignKey(const std::string &childTableName,
                       const std::string &childColumnName,
                       const std::string &parentTableName,
                       const std::string &parentColumnName,
                       ReferentialAction onDelete = ReferentialAction::RESTRICT);

    Table *getTable(const std::string &tableName);
    const Table *getTable(const std::string &tableName) const;

    const std::vector<Table> &getTables() const;
    const std::vector<ForeignKey> &getForeignKeys() const;
    std::vector<std::string> listTableNames() const;

    size_t tableCount() const;
    void clearDatabase();
};
