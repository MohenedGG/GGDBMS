#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "./core/src/Database.h"
#include "./core/src/DatabasePersistence.h"

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

    if (command == "list_tables")
    {
        const vector<string> names = db.listTableNames();
        printJsonResult(true, "Tables loaded.", "{\"tables\":" + toJsonStringArray(names) + "}");
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
        if (!actionOk || !db.addForeignKey(argv[4], argv[5], argv[6], argv[7], action))
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
            printJsonResult(false, "Failed to insert row.");
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

    printJsonResult(true, "Command executed.");
    return 0;
}

static void printTable(const Table &table)
{
    cout << "\nTable: " << table.getName() << endl;
    if (table.hasPrimaryKey())
    {
        cout << "Primary Key: " << table.getPrimaryKeyColumnName() << endl;
    }
    else
    {
        cout << "Primary Key: (none)" << endl;
    }

    cout << "Unique Columns: ";
    const vector<string> &uniqueColumns = table.getUniqueColumns();
    if (uniqueColumns.empty())
    {
        cout << "(none)";
    }
    else
    {
        for (size_t i = 0; i < uniqueColumns.size(); ++i)
        {
            cout << uniqueColumns[i];
            if (i + 1 < uniqueColumns.size())
            {
                cout << ", ";
            }
        }
    }
    cout << endl;

    cout << "Not Null Columns: ";
    const vector<string> &notNullColumns = table.getNotNullColumns();
    if (notNullColumns.empty())
    {
        cout << "(none)";
    }
    else
    {
        for (size_t i = 0; i < notNullColumns.size(); ++i)
        {
            cout << notNullColumns[i];
            if (i + 1 < notNullColumns.size())
            {
                cout << ", ";
            }
        }
    }
    cout << endl;

    cout << "Columns:" << endl;
    for (const Column &col : table.getColumns())
    {
        cout << "  - " << col.getName() << " (" << col.getType() << ")" << endl;
    }

    cout << "Rows:" << endl;
    const vector<Rows> &rows = table.getRows();
    if (rows.empty())
    {
        cout << "  (empty)" << endl;
        return;
    }

    for (size_t i = 0; i < rows.size(); ++i)
    {
        cout << "  [" << i << "] ";
        const vector<string> values = rows[i].getValues();
        for (size_t j = 0; j < values.size(); ++j)
        {
            cout << values[j];
            if (j + 1 < values.size())
            {
                cout << " | ";
            }
        }
        cout << endl;
    }
}

static int runDemoMode()
{
    Database db("TestDB");
    const string snapshotPath = "ggdb.snapshot";

    db.createTable("Users");
    db.createTable("Orders");

    Table *usersTable = db.getTable("Users");
    if (usersTable)
    {
        usersTable->addColumn(Column("id", "INT"));
        usersTable->addColumn(Column("name", "TEXT"));
        usersTable->addColumn(Column("email", "TEXT"));
        cout << "Set Users PK(id): " << (usersTable->setPrimaryKey("id") ? "OK" : "FAILED") << endl;
        cout << "Set Users UNIQUE(email): " << (usersTable->addUniqueConstraint("email") ? "OK" : "FAILED") << endl;
        cout << "Set Users NOT NULL(name): " << (usersTable->addNotNullConstraint("name") ? "OK" : "FAILED") << endl;
        cout << "Set Users NOT NULL(email): " << (usersTable->addNotNullConstraint("email") ? "OK" : "FAILED") << endl;
    }

    Table *ordersTable = db.getTable("Orders");
    if (ordersTable)
    {
        ordersTable->addColumn(Column("id", "INT"));
        ordersTable->addColumn(Column("user_id", "INT"));
        ordersTable->addColumn(Column("amount", "FLOAT"));
        cout << "Set Orders PK(id): " << (ordersTable->setPrimaryKey("id") ? "OK" : "FAILED") << endl;
        cout << "Set Orders NOT NULL(user_id): " << (ordersTable->addNotNullConstraint("user_id") ? "OK" : "FAILED") << endl;
        cout << "Set Orders NOT NULL(amount): " << (ordersTable->addNotNullConstraint("amount") ? "OK" : "FAILED") << endl;
    }

    db.addForeignKey("Orders", "user_id", "Users", "id", ReferentialAction::CASCADE);

    Rows user1;
    user1.addValue("1");
    user1.addValue("Ali");
    user1.addValue("ali@mail.com");

    Rows user2;
    user2.addValue("2");
    user2.addValue("Sara");
    user2.addValue("sara@mail.com");

    Rows user3DuplicateEmail;
    user3DuplicateEmail.addValue("3");
    user3DuplicateEmail.addValue("Noor");
    user3DuplicateEmail.addValue("sara@mail.com");

    Rows user4EmptyName;
    user4EmptyName.addValue("4");
    user4EmptyName.addValue("");
    user4EmptyName.addValue("no_name@mail.com");

    cout << "Insert user1: " << (db.insertRow("Users", user1) ? "OK" : "FAILED") << endl;
    cout << "Insert duplicate user1 (PK test): " << (db.insertRow("Users", user1) ? "OK" : "FAILED") << endl;
    cout << "Insert user2: " << (db.insertRow("Users", user2) ? "OK" : "FAILED") << endl;
    cout << "Insert user3 duplicate email (UNIQUE test): " << (db.insertRow("Users", user3DuplicateEmail) ? "OK" : "FAILED") << endl;
    cout << "Insert user4 empty name (NOT NULL test): " << (db.insertRow("Users", user4EmptyName) ? "OK" : "FAILED") << endl;

    Rows order1;
    order1.addValue("101");
    order1.addValue("1");
    order1.addValue("250.75");

    Rows order2;
    order2.addValue("102");
    order2.addValue("2");
    order2.addValue("100.25");

    Rows invalidOrder;
    invalidOrder.addValue("103");
    invalidOrder.addValue("99");
    invalidOrder.addValue("60.00");

    Rows invalidOrderEmptyUserId;
    invalidOrderEmptyUserId.addValue("104");
    invalidOrderEmptyUserId.addValue("");
    invalidOrderEmptyUserId.addValue("44.00");

    cout << "Insert order1: " << (db.insertRow("Orders", order1) ? "OK" : "FAILED") << endl;
    cout << "Insert order2: " << (db.insertRow("Orders", order2) ? "OK" : "FAILED") << endl;
    cout << "Insert invalidOrder (FK test): " << (db.insertRow("Orders", invalidOrder) ? "OK" : "FAILED") << endl;
    cout << "Insert invalidOrderEmptyUserId (NOT NULL test): " << (db.insertRow("Orders", invalidOrderEmptyUserId) ? "OK" : "FAILED") << endl;

    cout << "Database '" << db.getName() << "' created with tables: ";
    for (const string &tableName : db.listTableNames())
    {
        cout << tableName << " ";
    }
    cout << endl;

    for (const Table &table : db.getTables())
    {
        printTable(table);
    }

    cout << "\nDelete Users row[0] with CASCADE FK: "
         << (db.deleteRow("Users", 0) ? "OK" : "FAILED") << endl;

    cout << "\nState after delete:" << endl;
    for (const Table &table : db.getTables())
    {
        printTable(table);
    }

    cout << "\nSave snapshot: " << (DatabasePersistence::saveToFile(db, snapshotPath) ? "OK" : "FAILED") << endl;

    Database loadedDb;
    cout << "Load snapshot: " << (DatabasePersistence::loadFromFile(snapshotPath, loadedDb) ? "OK" : "FAILED") << endl;

    cout << "\nLoaded database name: " << loadedDb.getName() << endl;
    for (const Table &table : loadedDb.getTables())
    {
        printTable(table);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc > 1 && string(argv[1]) == "--json")
    {
        return runJsonMode(argc, argv);
    }

    return runDemoMode();
}