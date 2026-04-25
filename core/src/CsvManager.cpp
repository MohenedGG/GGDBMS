#include "CsvManager.h"
#include <fstream>

bool CsvManager::valueExistsInColumn(const Table &table, size_t columnIndex, const std::string &value)
{
    const std::vector<Rows> &rows = table.getRows();
    for (const Rows &existingRow : rows)
    {
        if (columnIndex < existingRow.size() && existingRow.getValue(columnIndex) == value)
        {
            return true;
        }
    }

    return false;
}

std::string CsvManager::diagnoseInsertFailure(const Database &db, const std::string &tableName, const Rows &row)
{
    const Table *table = db.getTable(tableName);
    if (table == nullptr)
    {
        return "Table not found.";
    }

    const std::vector<Column> &columns = table->getColumns();
    if (row.size() != columns.size())
    {
        return "Column/value count mismatch. Expected " + std::to_string(columns.size()) + " values but got " + std::to_string(row.size()) + ".";
    }

    if (table->hasPrimaryKey())
    {
        const std::string pkName = table->getPrimaryKeyColumnName();
        const size_t pkIndex = table->getPrimaryKeyColumnIndex();
        if (pkIndex >= columns.size() || pkIndex >= row.size())
        {
            return "Primary key column is invalid.";
        }

        const std::string pkValue = row.getValue(pkIndex);
        if (pkValue.empty())
        {
            return "Primary key '" + pkName + "' cannot be empty.";
        }

        const std::vector<Rows> &existingRows = table->getRows();
        for (const Rows &existingRow : existingRows)
        {
            if (pkIndex < existingRow.size() && existingRow.getValue(pkIndex) == pkValue)
            {
                return "Primary key constraint failed on column '" + pkName + "'. Value '" + pkValue + "' already exists.";
            }
        }
    }

    const std::vector<std::string> &notNullColumns = table->getNotNullColumns();
    for (const std::string &columnName : notNullColumns)
    {
        const std::vector<Column> &tableColumns = table->getColumns();
        for (size_t i = 0; i < tableColumns.size(); ++i)
        {
            if (tableColumns[i].getName() == columnName)
            {
                if (i >= columns.size() || i >= row.size() || row.getValue(i).empty())
                {
                    return "NOT NULL constraint failed on column '" + columnName + "'.";
                }
                break;
            }
        }
    }

    const std::vector<std::string> &uniqueColumns = table->getUniqueColumns();
    for (const std::string &columnName : uniqueColumns)
    {
        size_t columnIndex = columns.size();
        for (size_t i = 0; i < columns.size(); ++i)
        {
            if (columns[i].getName() == columnName)
            {
                columnIndex = i;
                break;
            }
        }

        if (columnIndex >= columns.size() || columnIndex >= row.size())
        {
            return "UNIQUE constraint is invalid for column '" + columnName + "'.";
        }

        const std::string newValue = row.getValue(columnIndex);
        const std::vector<Rows> &existingRows = table->getRows();
        for (const Rows &existingRow : existingRows)
        {
            if (columnIndex < existingRow.size() && existingRow.getValue(columnIndex) == newValue)
            {
                return "UNIQUE constraint failed on column '" + columnName + "'. Value '" + newValue + "' already exists.";
            }
        }
    }

    const std::vector<ForeignKey> &foreignKeys = db.getForeignKeys();
    for (const ForeignKey &foreignKey : foreignKeys)
    {
        if (foreignKey.childTableName != tableName)
        {
            continue;
        }

        size_t childColumnIndex = columns.size();
        for (size_t i = 0; i < columns.size(); ++i)
        {
            if (columns[i].getName() == foreignKey.childColumnName)
            {
                childColumnIndex = i;
                break;
            }
        }

        if (childColumnIndex >= columns.size() || childColumnIndex >= row.size())
        {
            return "Foreign key child column is invalid: '" + foreignKey.childTableName + "." + foreignKey.childColumnName + "'.";
        }

        const std::string childValue = row.getValue(childColumnIndex);
        if (childValue.empty())
        {
            continue;
        }

        const Table *parentTable = db.getTable(foreignKey.parentTableName);
        if (parentTable == nullptr)
        {
            return "Foreign key parent table not found: '" + foreignKey.parentTableName + "'.";
        }

        size_t parentColumnIndex = parentTable->getColumns().size();
        const std::vector<Column> &parentColumns = parentTable->getColumns();
        for (size_t i = 0; i < parentColumns.size(); ++i)
        {
            if (parentColumns[i].getName() == foreignKey.parentColumnName)
            {
                parentColumnIndex = i;
                break;
            }
        }

        if (parentColumnIndex >= parentTable->getColumns().size())
        {
            return "Foreign key parent column is invalid: '" + foreignKey.parentTableName + "." + foreignKey.parentColumnName + "'.";
        }

        if (!valueExistsInColumn(*parentTable, parentColumnIndex, childValue))
        {
            return "Foreign key constraint failed: '" + foreignKey.childTableName + "." + foreignKey.childColumnName + "' value '" + childValue +
                   "' does not exist in '" + foreignKey.parentTableName + "." + foreignKey.parentColumnName + "'.";
        }
    }

    return "Failed to insert row.";
}

std::string CsvManager::escapeCsv(const std::string &value)
{
    bool shouldQuote = false;
    std::string escaped;
    escaped.reserve(value.size() + 4);

    for (char ch : value)
    {
        if (ch == '"')
        {
            escaped += "\"\"";
            shouldQuote = true;
            continue;
        }

        if (ch == ',' || ch == '\n' || ch == '\r')
        {
            shouldQuote = true;
        }

        escaped.push_back(ch);
    }

    if (!shouldQuote)
    {
        return escaped;
    }

    return std::string("\"") + escaped + "\"";
}

bool CsvManager::parseCsvLine(const std::string &line, std::vector<std::string> &fields)
{
    fields.clear();

    std::string current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i)
    {
        const char ch = line[i];

        if (inQuotes)
        {
            if (ch == '"')
            {
                if (i + 1 < line.size() && line[i + 1] == '"')
                {
                    current.push_back('"');
                    ++i;
                }
                else
                {
                    inQuotes = false;
                }
                continue;
            }

            current.push_back(ch);
            continue;
        }

        if (ch == '"')
        {
            inQuotes = true;
        }
        else if (ch == ',')
        {
            fields.push_back(current);
            current.clear();
        }
        else
        {
            current.push_back(ch);
        }
    }

    if (inQuotes)
    {
        return false;
    }

    fields.push_back(current);
    return true;
}

bool CsvManager::readCsvRecordsFromFile(const std::string &csvFilePath,
                                        std::vector<std::vector<std::string>> &records,
                                        std::string &errorMessage)
{
    std::ifstream input(csvFilePath);
    if (!input.is_open())
    {
        errorMessage = "Failed to open CSV file.";
        return false;
    }

    records.clear();

    std::string line;
    size_t lineNumber = 0;
    while (std::getline(input, line))
    {
        ++lineNumber;

        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if (line.empty())
        {
            continue;
        }

        std::vector<std::string> fields;
        if (!parseCsvLine(line, fields))
        {
            errorMessage = "Invalid CSV format at line " + std::to_string(lineNumber) + ".";
            return false;
        }

        records.push_back(fields);
    }

    if (!input.eof() && input.fail())
    {
        errorMessage = "Failed while reading CSV file.";
        return false;
    }

    return true;
}

bool CsvManager::csvHeaderMatchesColumns(const Table &table, const std::vector<std::string> &rowValues)
{
    const std::vector<Column> &columns = table.getColumns();
    if (rowValues.size() != columns.size())
    {
        return false;
    }

    for (size_t i = 0; i < columns.size(); ++i)
    {
        if (columns[i].getName() != rowValues[i])
        {
            return false;
        }
    }

    return true;
}

bool CsvManager::exportTable(const Table &table, const std::string &csvFilePath, std::string &errorMessage)
{
    std::ofstream output(csvFilePath, std::ios::out | std::ios::trunc);
    if (!output.is_open())
    {
        errorMessage = "Failed to open CSV output file.";
        return false;
    }

    const std::vector<Column> &columns = table.getColumns();
    for (size_t i = 0; i < columns.size(); ++i)
    {
        output << escapeCsv(columns[i].getName());
        if (i + 1 < columns.size())
        {
            output << ',';
        }
    }
    output << '\n';

    const std::vector<Rows> &rows = table.getRows();
    for (const Rows &row : rows)
    {
        for (size_t i = 0; i < columns.size(); ++i)
        {
            const std::string value = i < row.size() ? row.getValue(i) : "";
            output << escapeCsv(value);
            if (i + 1 < columns.size())
            {
                output << ',';
            }
        }
        output << '\n';
    }

    if (!output.good())
    {
        errorMessage = "Failed while writing CSV file.";
        return false;
    }

    return true;
}

bool CsvManager::importTable(Database &db,
                             const std::string &tableName,
                             const std::string &csvFilePath,
                             size_t &importedRows,
                             std::string &errorMessage)
{
    importedRows = 0;

    Table *table = db.getTable(tableName);
    if (table == nullptr)
    {
        errorMessage = "Table not found.";
        return false;
    }

    const std::vector<Column> &columns = table->getColumns();
    if (columns.empty())
    {
        errorMessage = "Table has no columns.";
        return false;
    }

    std::vector<std::vector<std::string>> records;
    if (!readCsvRecordsFromFile(csvFilePath, records, errorMessage))
    {
        return false;
    }

    if (records.empty())
    {
        errorMessage = "CSV file is empty.";
        return false;
    }

    size_t startIndex = 0;
    if (csvHeaderMatchesColumns(*table, records[0]))
    {
        startIndex = 1;
    }

    for (size_t index = startIndex; index < records.size(); ++index)
    {
        const std::vector<std::string> &record = records[index];
        if (record.size() != columns.size())
        {
            errorMessage = "CSV row " + std::to_string(index + 1) + " has " + std::to_string(record.size()) +
                           " values but table expects " + std::to_string(columns.size()) + ".";
            return false;
        }

        Rows row;
        for (const std::string &value : record)
        {
            row.addValue(value);
        }

        if (!db.insertRow(tableName, row))
        {
            errorMessage = "CSV row " + std::to_string(index + 1) + " failed: " + CsvManager::diagnoseInsertFailure(db, tableName, row);
            return false;
        }

        ++importedRows;
    }

    return true;
}
