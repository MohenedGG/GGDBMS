#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>
#include "./src/Database.h"
#include "./src/DatabasePersistence.h"
#include "./src/CsvManager.h"

using namespace std;

static string escapeJson(const string &value)
{
    string escaped;
    escaped.reserve(value.size() + 8);

    for (char ch : value)
    {
        switch (ch)
        {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped += ch;
            break;
        }
    }

    return escaped;
}

static string toJsonString(const string &value)
{
    return string("\"") + escapeJson(value) + "\"";
}

static string toJsonStringArray(const vector<string> &values)
{
    string out = "[";
    for (size_t i = 0; i < values.size(); ++i)
    {
        out += toJsonString(values[i]);
        if (i + 1 < values.size())
        {
            out += ",";
        }
    }
    out += "]";
    return out;
}

static string tableToJson(const Table &table)
{
    string json = "{";
    json += "\"name\":" + toJsonString(table.getName()) + ",";
    json += "\"primaryKey\":" + toJsonString(table.hasPrimaryKey() ? table.getPrimaryKeyColumnName() : "") + ",";
    json += "\"unique\":" + toJsonStringArray(table.getUniqueColumns()) + ",";
    json += "\"notNull\":" + toJsonStringArray(table.getNotNullColumns()) + ",";

    json += "\"columns\":[";
    const vector<Column> &columns = table.getColumns();
    for (size_t i = 0; i < columns.size(); ++i)
    {
        json += "{";
        json += "\"name\":" + toJsonString(columns[i].getName()) + ",";
        json += "\"type\":" + toJsonString(columns[i].getType());
        json += "}";
        if (i + 1 < columns.size())
        {
            json += ",";
        }
    }
    json += "],";

    json += "\"rows\":[";
    const vector<Rows> &rows = table.getRows();
    for (size_t i = 0; i < rows.size(); ++i)
    {
        json += toJsonStringArray(rows[i].getValues());
        if (i + 1 < rows.size())
        {
            json += ",";
        }
    }
    json += "]";
    json += "}";

    return json;
}

static void printJsonResult(bool ok, const string &message, const string &dataJson = "{}")
{
    cout << "{\"ok\":" << (ok ? "true" : "false")
         << ",\"message\":" << toJsonString(message)
         << ",\"data\":" << dataJson << "}" << endl;
}

static vector<string> splitPipeSeparated(const string &joinedValues)
{
    vector<string> values;
    string current;

    for (char ch : joinedValues)
    {
        if (ch == '|')
        {
            values.push_back(current);
            current.clear();
        }
        else
        {
            current.push_back(ch);
        }
    }

    values.push_back(current);
    return values;
}

static ReferentialAction parseAction(const string &actionText, bool &ok)
{
    string normalized = actionText;
    transform(normalized.begin(), normalized.end(), normalized.begin(),
              [](unsigned char c)
              { return static_cast<char>(toupper(c)); });

    if (normalized == "CASCADE")
    {
        ok = true;
        return ReferentialAction::CASCADE;
    }

    if (normalized == "RESTRICT")
    {
        ok = true;
        return ReferentialAction::RESTRICT;
    }

    ok = false;
    return ReferentialAction::RESTRICT;
}

static bool parseSizeValue(const string &text, size_t &value)
{
    if (text.empty())
    {
        return false;
    }

    try
    {
        size_t parsedChars = 0;
        const unsigned long long parsed = stoull(text, &parsedChars);
        if (parsedChars != text.size())
        {
            return false;
        }

        value = static_cast<size_t>(parsed);
        return true;
    }
    catch (const invalid_argument &)
    {
        return false;
    }
    catch (const out_of_range &)
    {
        return false;
    }
}

static bool equalsIgnoreCaseText(const string &lhs, const string &rhs)
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }

    for (size_t i = 0; i < lhs.size(); ++i)
    {
        const unsigned char left = static_cast<unsigned char>(lhs[i]);
        const unsigned char right = static_cast<unsigned char>(rhs[i]);
        if (toupper(left) != toupper(right))
        {
            return false;
        }
    }

    return true;
}

static size_t findColumnIndexByName(const Table &table, const string &columnName)
{
    const vector<Column> &columns = table.getColumns();
    for (size_t i = 0; i < columns.size(); ++i)
    {
        if (columns[i].getName() == columnName)
        {
            return i;
        }
    }

    return columns.size();
}

static string referentialActionToText(ReferentialAction action)
{
    return action == ReferentialAction::CASCADE ? "CASCADE" : "RESTRICT";
}

static bool valueExistsInColumn(const Table &table, size_t columnIndex, const string &value)
{
    const vector<Rows> &rows = table.getRows();
    for (const Rows &existingRow : rows)
    {
        if (columnIndex < existingRow.size() && existingRow.getValue(columnIndex) == value)
        {
            return true;
        }
    }

    return false;
}

static string diagnoseInsertFailure(const Database &db, const string &tableName, const Rows &row)
{
    const Table *table = db.getTable(tableName);
    if (table == nullptr)
    {
        return "Table not found.";
    }

    const vector<Column> &columns = table->getColumns();
    if (row.size() != columns.size())
    {
        return "Column/value count mismatch. Expected " + to_string(columns.size()) + " values but got " + to_string(row.size()) + ".";
    }

    if (table->hasPrimaryKey())
    {
        const string pkName = table->getPrimaryKeyColumnName();
        const size_t pkIndex = findColumnIndexByName(*table, pkName);
        if (pkIndex >= columns.size() || pkIndex >= row.size())
        {
            return "Primary key column is invalid.";
        }

        const string pkValue = row.getValue(pkIndex);
        if (pkValue.empty())
        {
            return "Primary key '" + pkName + "' cannot be empty.";
        }

        const vector<Rows> &existingRows = table->getRows();
        for (const Rows &existingRow : existingRows)
        {
            if (pkIndex < existingRow.size() && existingRow.getValue(pkIndex) == pkValue)
            {
                return "Primary key constraint failed on column '" + pkName + "'. Value '" + pkValue + "' already exists.";
            }
        }
    }

    const vector<string> &notNullColumns = table->getNotNullColumns();
    for (const string &columnName : notNullColumns)
    {
        const size_t columnIndex = findColumnIndexByName(*table, columnName);
        if (columnIndex >= columns.size() || columnIndex >= row.size() || row.getValue(columnIndex).empty())
        {
            return "NOT NULL constraint failed on column '" + columnName + "'.";
        }
    }

    const vector<string> &uniqueColumns = table->getUniqueColumns();
    for (const string &columnName : uniqueColumns)
    {
        const size_t columnIndex = findColumnIndexByName(*table, columnName);
        if (columnIndex >= columns.size() || columnIndex >= row.size())
        {
            return "UNIQUE constraint is invalid for column '" + columnName + "'.";
        }

        const string newValue = row.getValue(columnIndex);
        const vector<Rows> &existingRows = table->getRows();
        for (const Rows &existingRow : existingRows)
        {
            if (columnIndex < existingRow.size() && existingRow.getValue(columnIndex) == newValue)
            {
                return "UNIQUE constraint failed on column '" + columnName + "'. Value '" + newValue + "' already exists.";
            }
        }
    }

    const vector<ForeignKey> &foreignKeys = db.getForeignKeys();
    for (const ForeignKey &foreignKey : foreignKeys)
    {
        if (foreignKey.childTableName != tableName)
        {
            continue;
        }

        const size_t childColumnIndex = findColumnIndexByName(*table, foreignKey.childColumnName);
        if (childColumnIndex >= columns.size() || childColumnIndex >= row.size())
        {
            return "Foreign key child column is invalid: '" + foreignKey.childTableName + "." + foreignKey.childColumnName + "'.";
        }

        const string childValue = row.getValue(childColumnIndex);
        if (childValue.empty())
        {
            continue;
        }

        const Table *parentTable = db.getTable(foreignKey.parentTableName);
        if (parentTable == nullptr)
        {
            return "Foreign key parent table not found: '" + foreignKey.parentTableName + "'.";
        }

        const size_t parentColumnIndex = findColumnIndexByName(*parentTable, foreignKey.parentColumnName);
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

static int runJsonMode(int argc, char *argv[])
{
    if (argc < 3)
    {
        printJsonResult(false, "Missing command. Usage: --json <command> ...");
        return 1;
    }

    const string command = argv[2];

    if (command == "ping")
    {
        printJsonResult(true, "pong");
        return 0;
    }

    if (command == "init")
    {
        if (argc < 5)
        {
            printJsonResult(false, "Usage: --json init <snapshotPath> <dbName>");
            return 1;
        }

        const string snapshotPath = argv[3];
        const string dbName = argv[4];

        Database db(dbName);
        if (!DatabasePersistence::saveToFile(db, snapshotPath))
        {
            printJsonResult(false, "Failed to create snapshot file.");
            return 1;
        }

        printJsonResult(true, "Database initialized.", "{\"databaseName\":" + toJsonString(dbName) + "}");
        return 0;
    }

    if (argc < 4)
    {
        printJsonResult(false, "Missing snapshot path.");
        return 1;
    }

    const string snapshotPath = argv[3];
    Database db;
    if (!DatabasePersistence::loadFromFile(snapshotPath, db))
    {
        printJsonResult(false, "Failed to load snapshot.");
        return 1;
    }

    bool mutated = false;
    string successMessage = "Command executed.";
    string successDataJson = "{}";

    if (command == "list_tables")
    {
        const vector<string> names = db.listTableNames();
        printJsonResult(true, "Tables loaded.", "{\"tables\":" + toJsonStringArray(names) + "}");
        return 0;
    }

    if (command == "list_foreign_keys")
    {
        const vector<ForeignKey> &foreignKeys = db.getForeignKeys();
        string json = "{\"foreignKeys\":[";

        for (size_t i = 0; i < foreignKeys.size(); ++i)
        {
            const ForeignKey &fk = foreignKeys[i];
            json += "{";
            json += "\"childTable\":" + toJsonString(fk.childTableName) + ",";
            json += "\"childColumn\":" + toJsonString(fk.childColumnName) + ",";
            json += "\"parentTable\":" + toJsonString(fk.parentTableName) + ",";
            json += "\"parentColumn\":" + toJsonString(fk.parentColumnName) + ",";
            json += "\"onDelete\":" + toJsonString(referentialActionToText(fk.onDelete));
            json += "}";

            if (i + 1 < foreignKeys.size())
            {
                json += ",";
            }
        }

        json += "]}";
        printJsonResult(true, "Foreign keys loaded.", json);
        return 0;
    }

    if (command == "create_table")
    {
        if (argc < 5)
        {
            printJsonResult(false, "Usage: --json create_table <snapshotPath> <tableName>");
            return 1;
        }

        if (!db.createTable(argv[4]))
        {
            printJsonResult(false, "Failed to create table.");
            return 1;
        }
        mutated = true;
    }
    else if (command == "drop_table")
    {
        if (argc < 5)
        {
            printJsonResult(false, "Usage: --json drop_table <snapshotPath> <tableName>");
            return 1;
        }

        if (!db.dropTable(argv[4]))
        {
            printJsonResult(false, "Failed to drop table.");
            return 1;
        }
        mutated = true;
    }
    else if (command == "add_column")
    {
        if (argc < 7)
        {
            printJsonResult(false, "Usage: --json add_column <snapshotPath> <tableName> <columnName> <columnType>");
            return 1;
        }

        Table *table = db.getTable(argv[4]);
        if (table == nullptr || !table->addColumn(Column(argv[5], argv[6])))
        {
            printJsonResult(false, "Failed to add column.");
            return 1;
        }
        mutated = true;
    }
    else if (command == "remove_column")
    {
        if (argc < 6)
        {
            printJsonResult(false, "Usage: --json remove_column <snapshotPath> <tableName> <columnName>");
            return 1;
        }

        Table *table = db.getTable(argv[4]);
        if (table == nullptr)
        {
            printJsonResult(false, "Table not found.");
            return 1;
        }

        const string columnName = argv[5];
        const vector<Column> &columns = table->getColumns();
        size_t columnIndex = columns.size();

        for (size_t i = 0; i < columns.size(); ++i)
        {
            if (columns[i].getName() == columnName)
            {
                columnIndex = i;
                break;
            }
        }

        if (columnIndex >= columns.size() || !table->removeColumn(columnIndex))
        {
            printJsonResult(false, "Failed to remove column.");
            return 1;
        }
        mutated = true;
    }
    else if (command == "set_primary_key")
    {
        if (argc < 6)
        {
            printJsonResult(false, "Usage: --json set_primary_key <snapshotPath> <tableName> <columnName>");
            return 1;
        }

        Table *table = db.getTable(argv[4]);
        if (table == nullptr || !table->setPrimaryKey(argv[5]))
        {
            printJsonResult(false, "Failed to set primary key.");
            return 1;
        }
        mutated = true;
    }
    else if (command == "add_unique")
    {
        if (argc < 6)
        {
            printJsonResult(false, "Usage: --json add_unique <snapshotPath> <tableName> <columnName>");
            return 1;
        }

        Table *table = db.getTable(argv[4]);
        if (table == nullptr || !table->addUniqueConstraint(argv[5]))
        {
            printJsonResult(false, "Failed to add unique constraint.");
            return 1;
        }
        mutated = true;
    }
    else if (command == "add_not_null")
    {
        if (argc < 6)
        {
            printJsonResult(false, "Usage: --json add_not_null <snapshotPath> <tableName> <columnName>");
            return 1;
        }

        Table *table = db.getTable(argv[4]);
        if (table == nullptr || !table->addNotNullConstraint(argv[5]))
        {
            printJsonResult(false, "Failed to add not-null constraint.");
            return 1;
        }
        mutated = true;
    }
    else if (command == "add_foreign_key")
    {
        if (argc < 9)
        {
            printJsonResult(false, "Usage: --json add_foreign_key <snapshotPath> <childTable> <childColumn> <parentTable> <parentColumn> <RESTRICT|CASCADE>");
            return 1;
        }

        bool actionOk = false;
        const ReferentialAction action = parseAction(argv[8], actionOk);
        if (!actionOk)
        {
            printJsonResult(false, "Invalid onDelete action. Use RESTRICT or CASCADE.");
            return 1;
        }

        const string childTableName = argv[4];
        const string childColumnName = argv[5];
        const string parentTableName = argv[6];
        const string parentColumnName = argv[7];

        const Table *childTable = db.getTable(childTableName);
        if (childTable == nullptr)
        {
            printJsonResult(false, "Child table not found.");
            return 1;
        }

        const Table *parentTable = db.getTable(parentTableName);
        if (parentTable == nullptr)
        {
            printJsonResult(false, "Parent table not found.");
            return 1;
        }

        const size_t childColumnIndex = findColumnIndexByName(*childTable, childColumnName);
        if (childColumnIndex >= childTable->getColumns().size())
        {
            printJsonResult(false, "Child column not found.");
            return 1;
        }

        const size_t parentColumnIndex = findColumnIndexByName(*parentTable, parentColumnName);
        if (parentColumnIndex >= parentTable->getColumns().size())
        {
            printJsonResult(false, "Parent column not found.");
            return 1;
        }

        if (!parentTable->hasPrimaryKey() || parentTable->getPrimaryKeyColumnName() != parentColumnName)
        {
            printJsonResult(false, "Parent column must be the parent table primary key.");
            return 1;
        }

        const string &childType = childTable->getColumns()[childColumnIndex].getType();
        const string &parentType = parentTable->getColumns()[parentColumnIndex].getType();
        if (!equalsIgnoreCaseText(childType, parentType))
        {
            printJsonResult(false, "Child/parent column types must match.");
            return 1;
        }

        const vector<ForeignKey> &existingForeignKeys = db.getForeignKeys();
        for (const ForeignKey &foreignKey : existingForeignKeys)
        {
            if (foreignKey.childTableName == childTableName &&
                foreignKey.childColumnName == childColumnName &&
                foreignKey.parentTableName == parentTableName &&
                foreignKey.parentColumnName == parentColumnName)
            {
                printJsonResult(false, "Foreign key already exists.");
                return 1;
            }
        }

        if (!db.addForeignKey(childTableName, childColumnName, parentTableName, parentColumnName, action))
        {
            printJsonResult(false, "Failed to add foreign key.");
            return 1;
        }
        mutated = true;
    }
    else if (command == "insert_row")
    {
        if (argc < 6)
        {
            printJsonResult(false, "Usage: --json insert_row <snapshotPath> <tableName> <value1|value2|...>");
            return 1;
        }

        Rows row;
        const vector<string> values = splitPipeSeparated(argv[5]);
        for (const string &value : values)
        {
            row.addValue(value);
        }

        if (!db.insertRow(argv[4], row))
        {
            printJsonResult(false, diagnoseInsertFailure(db, argv[4], row));
            return 1;
        }
        mutated = true;
    }
    else if (command == "delete_row")
    {
        if (argc < 6)
        {
            printJsonResult(false, "Usage: --json delete_row <snapshotPath> <tableName> <rowIndex>");
            return 1;
        }

        size_t rowIndex = 0;
        if (!parseSizeValue(argv[5], rowIndex) || !db.deleteRow(argv[4], rowIndex))
        {
            printJsonResult(false, "Failed to delete row.");
            return 1;
        }
        mutated = true;
    }
    else if (command == "get_table")
    {
        if (argc < 5)
        {
            printJsonResult(false, "Usage: --json get_table <snapshotPath> <tableName>");
            return 1;
        }

        const Table *table = db.getTable(argv[4]);
        if (table == nullptr)
        {
            printJsonResult(false, "Table not found.");
            return 1;
        }

        printJsonResult(true, "Table loaded.", tableToJson(*table));
        return 0;
    }
    else if (command == "export_csv")
    {
        if (argc < 6)
        {
            printJsonResult(false, "Usage: --json export_csv <snapshotPath> <tableName> <csvFilePath>");
            return 1;
        }

        const Table *table = db.getTable(argv[4]);
        if (table == nullptr)
        {
            printJsonResult(false, "Table not found.");
            return 1;
        }

        string errorMessage;
        if (!CsvManager::exportTable(*table, argv[5], errorMessage))
        {
            printJsonResult(false, errorMessage);
            return 1;
        }

        printJsonResult(true, "CSV exported.", "{\"path\":" + toJsonString(argv[5]) + "}");
        return 0;
    }
    else if (command == "import_csv")
    {
        if (argc < 6)
        {
            printJsonResult(false, "Usage: --json import_csv <snapshotPath> <tableName> <csvFilePath>");
            return 1;
        }

        size_t importedRows = 0;
        string errorMessage;
        if (!CsvManager::importTable(db, argv[4], argv[5], importedRows, errorMessage))
        {
            printJsonResult(false, errorMessage);
            return 1;
        }

        mutated = true;
        successMessage = "CSV imported.";
        successDataJson = "{\"importedRows\":" + to_string(importedRows) + "}";
    }
    else
    {
        printJsonResult(false, "Unknown command.");
        return 1;
    }

    if (mutated && !DatabasePersistence::saveToFile(db, snapshotPath))
    {
        printJsonResult(false, "Failed to persist changes.");
        return 1;
    }

    printJsonResult(true, successMessage, successDataJson);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc > 1 && string(argv[1]) == "--json")
    {
        return runJsonMode(argc, argv);
    }

    printJsonResult(false, "Production mode requires JSON commands. Usage: --json <command> ...");
    return 1;
}