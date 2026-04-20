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
    std::string primaryKeyColumnName;
    std::vector<std::string> uniqueColumnNames;
    std::vector<std::string> notNullColumnNames;

    bool isValidRowSize(const Rows &row) const;
    size_t findColumnIndex(const std::string &columnName) const;
    bool isPrimaryKeyValueUnique(const Rows &row) const;
    bool areUniqueConstraintsSatisfied(const Rows &row) const;
    bool areNotNullConstraintsSatisfied(const Rows &row) const;

public:
    Table();
    explicit Table(const std::string &name);
    ~Table();

    const std::string &getName() const;
    void setName(const std::string &newName);

    const std::vector<Column> &getColumns() const;
    const std::vector<Rows> &getRows() const;

    bool setPrimaryKey(const std::string &columnName);
    bool hasPrimaryKey() const;
    const std::string &getPrimaryKeyColumnName() const;
    size_t getPrimaryKeyColumnIndex() const;

    bool addUniqueConstraint(const std::string &columnName);
    bool hasUniqueConstraint(const std::string &columnName) const;
    const std::vector<std::string> &getUniqueColumns() const;

    bool addNotNullConstraint(const std::string &columnName);
    bool hasNotNullConstraint(const std::string &columnName) const;
    const std::vector<std::string> &getNotNullColumns() const;

    bool addColumn(const Column &column);
    bool addRow(const Rows &row);

    bool removeColumn(size_t index);
    bool removeRow(size_t index);

    void clearRows();
    void clearTable();

    size_t columnCount() const;
    size_t rowCount() const;
};