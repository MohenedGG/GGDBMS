#include "DatabasePersistence.h"
#include "SnapshotEncryptor.h"
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <utility>

namespace
{
    constexpr const char *kSnapshotMagic = "GGDBMS_SNAPSHOT_V1";

    bool writeQuoted(std::ostream &out, const std::string &value)
    {
        out << std::quoted(value) << '\n';
        return out.good();
    }

    bool readQuoted(std::istream &in, std::string &value)
    {
        return static_cast<bool>(in >> std::quoted(value));
    }

    bool writeSize(std::ostream &out, size_t value)
    {
        out << value << '\n';
        return out.good();
    }

    bool readSize(std::istream &in, size_t &value)
    {
        unsigned long long temp = 0;
        if (!(in >> temp))
        {
            return false;
        }

        value = static_cast<size_t>(temp);
        return true;
    }

    int toActionCode(ReferentialAction action)
    {
        return action == ReferentialAction::CASCADE ? 1 : 0;
    }

    bool fromActionCode(size_t code, ReferentialAction &action)
    {
        if (code == 0)
        {
            action = ReferentialAction::RESTRICT;
            return true;
        }

        if (code == 1)
        {
            action = ReferentialAction::CASCADE;
            return true;
        }

        return false;
    }

    struct PendingRow
    {
        std::string tableName;
        Rows row;
    };

    bool writeSnapshotText(const Database &database, std::ostream &out)
    {
        out << kSnapshotMagic << '\n';

        if (!writeQuoted(out, database.getName()))
        {
            return false;
        }

        const std::vector<Table> &tables = database.getTables();
        if (!writeSize(out, tables.size()))
        {
            return false;
        }

        for (const Table &table : tables)
        {
            if (!writeQuoted(out, table.getName()))
            {
                return false;
            }

            const std::vector<Column> &columns = table.getColumns();
            if (!writeSize(out, columns.size()))
            {
                return false;
            }

            for (const Column &column : columns)
            {
                if (!writeQuoted(out, column.getName()) || !writeQuoted(out, column.getType()))
                {
                    return false;
                }
            }

            out << (table.hasPrimaryKey() ? 1 : 0) << '\n';
            if (table.hasPrimaryKey() && !writeQuoted(out, table.getPrimaryKeyColumnName()))
            {
                return false;
            }

            const std::vector<std::string> &uniqueColumns = table.getUniqueColumns();
            if (!writeSize(out, uniqueColumns.size()))
            {
                return false;
            }
            for (const std::string &columnName : uniqueColumns)
            {
                if (!writeQuoted(out, columnName))
                {
                    return false;
                }
            }

            const std::vector<std::string> &notNullColumns = table.getNotNullColumns();
            if (!writeSize(out, notNullColumns.size()))
            {
                return false;
            }
            for (const std::string &columnName : notNullColumns)
            {
                if (!writeQuoted(out, columnName))
                {
                    return false;
                }
            }

            const std::vector<Rows> &rows = table.getRows();
            if (!writeSize(out, rows.size()))
            {
                return false;
            }

            for (const Rows &row : rows)
            {
                const std::vector<std::string> values = row.getValues();
                if (!writeSize(out, values.size()))
                {
                    return false;
                }

                for (const std::string &value : values)
                {
                    if (!writeQuoted(out, value))
                    {
                        return false;
                    }
                }
            }
        }

        const std::vector<ForeignKey> &foreignKeys = database.getForeignKeys();
        if (!writeSize(out, foreignKeys.size()))
        {
            return false;
        }

        for (const ForeignKey &foreignKey : foreignKeys)
        {
            if (!writeQuoted(out, foreignKey.childTableName) ||
                !writeQuoted(out, foreignKey.childColumnName) ||
                !writeQuoted(out, foreignKey.parentTableName) ||
                !writeQuoted(out, foreignKey.parentColumnName))
            {
                return false;
            }

            out << toActionCode(foreignKey.onDelete) << '\n';
            if (!out.good())
            {
                return false;
            }
        }

        return out.good();
    }

    bool readSnapshotText(std::istream &in, Database &database)
    {
        std::string magic;
        if (!(in >> magic) || magic != kSnapshotMagic)
        {
            return false;
        }

        std::string loadedDbName;
        if (!readQuoted(in, loadedDbName))
        {
            return false;
        }

        Database loadedDatabase(loadedDbName);

        size_t tableCount = 0;
        if (!readSize(in, tableCount))
        {
            return false;
        }

        std::vector<PendingRow> pendingRows;

        for (size_t i = 0; i < tableCount; ++i)
        {
            std::string tableName;
            if (!readQuoted(in, tableName))
            {
                return false;
            }

            if (!loadedDatabase.createTable(tableName))
            {
                return false;
            }

            Table *table = loadedDatabase.getTable(tableName);
            if (table == nullptr)
            {
                return false;
            }

            size_t columnCount = 0;
            if (!readSize(in, columnCount))
            {
                return false;
            }

            for (size_t c = 0; c < columnCount; ++c)
            {
                std::string columnName;
                std::string columnType;
                if (!readQuoted(in, columnName) || !readQuoted(in, columnType))
                {
                    return false;
                }

                if (!table->addColumn(Column(columnName, columnType)))
                {
                    return false;
                }
            }

            size_t hasPrimaryKeyCode = 0;
            if (!readSize(in, hasPrimaryKeyCode) || hasPrimaryKeyCode > 1)
            {
                return false;
            }

            if (hasPrimaryKeyCode == 1)
            {
                std::string primaryKeyName;
                if (!readQuoted(in, primaryKeyName) || !table->setPrimaryKey(primaryKeyName))
                {
                    return false;
                }
            }

            size_t uniqueCount = 0;
            if (!readSize(in, uniqueCount))
            {
                return false;
            }
            for (size_t u = 0; u < uniqueCount; ++u)
            {
                std::string uniqueColumnName;
                if (!readQuoted(in, uniqueColumnName))
                {
                    return false;
                }

                if (!table->hasUniqueConstraint(uniqueColumnName) && !table->addUniqueConstraint(uniqueColumnName))
                {
                    return false;
                }
            }

            size_t notNullCount = 0;
            if (!readSize(in, notNullCount))
            {
                return false;
            }
            for (size_t n = 0; n < notNullCount; ++n)
            {
                std::string notNullColumnName;
                if (!readQuoted(in, notNullColumnName))
                {
                    return false;
                }

                if (!table->hasNotNullConstraint(notNullColumnName) && !table->addNotNullConstraint(notNullColumnName))
                {
                    return false;
                }
            }

            size_t rowCount = 0;
            if (!readSize(in, rowCount))
            {
                return false;
            }

            for (size_t r = 0; r < rowCount; ++r)
            {
                size_t valueCount = 0;
                if (!readSize(in, valueCount))
                {
                    return false;
                }

                const size_t expectedColumnCount = table->columnCount();
                if (valueCount > expectedColumnCount)
                {
                    return false;
                }

                Rows row;
                for (size_t v = 0; v < valueCount; ++v)
                {
                    std::string value;
                    if (!readQuoted(in, value))
                    {
                        return false;
                    }
                    row.addValue(value);
                }

                // Backward compatibility: older snapshots may store rows shorter than column count.
                while (row.size() < expectedColumnCount)
                {
                    row.addValue("");
                }

                pendingRows.push_back(PendingRow{tableName, row});
            }
        }

        size_t foreignKeyCount = 0;
        if (!readSize(in, foreignKeyCount))
        {
            return false;
        }

        std::vector<ForeignKey> pendingForeignKeys;
        pendingForeignKeys.reserve(foreignKeyCount);

        for (size_t fkIndex = 0; fkIndex < foreignKeyCount; ++fkIndex)
        {
            ForeignKey foreignKey;
            if (!readQuoted(in, foreignKey.childTableName) ||
                !readQuoted(in, foreignKey.childColumnName) ||
                !readQuoted(in, foreignKey.parentTableName) ||
                !readQuoted(in, foreignKey.parentColumnName))
            {
                return false;
            }

            size_t actionCode = 0;
            if (!readSize(in, actionCode) || !fromActionCode(actionCode, foreignKey.onDelete))
            {
                return false;
            }

            pendingForeignKeys.push_back(foreignKey);
        }

        for (const ForeignKey &foreignKey : pendingForeignKeys)
        {
            if (!loadedDatabase.addForeignKey(foreignKey))
            {
                return false;
            }
        }

        while (!pendingRows.empty())
        {
            bool insertedAnyRow = false;

            for (auto it = pendingRows.begin(); it != pendingRows.end();)
            {
                if (loadedDatabase.insertRow(it->tableName, it->row))
                {
                    it = pendingRows.erase(it);
                    insertedAnyRow = true;
                }
                else
                {
                    ++it;
                }
            }

            if (!insertedAnyRow)
            {
                return false;
            }
        }

        database = std::move(loadedDatabase);
        return true;
    }
} // namespace

bool DatabasePersistence::saveToFile(const Database &database, const std::string &filePath)
{
    std::ostringstream plainSnapshot;
    if (!writeSnapshotText(database, plainSnapshot))
    {
        return false;
    }

    const std::string hexPayload = SnapshotEncryptor::encryptToHex(plainSnapshot.str());

    std::ofstream out(filePath, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!out.is_open())
    {
        return false;
    }

    out << SnapshotEncryptor::magicHeader() << '\n'
        << hexPayload;

    return out.good();
}

bool DatabasePersistence::loadFromFile(const std::string &filePath, Database &database)
{
    std::ifstream in(filePath, std::ios::binary);
    if (!in.is_open())
    {
        return false;
    }

    const std::string fileContents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (fileContents.empty())
    {
        return false;
    }

    const std::string encryptedPrefix = SnapshotEncryptor::magicHeader() + "\n";
    if (fileContents.compare(0, encryptedPrefix.size(), encryptedPrefix) == 0)
    {
        std::string hexPayload;
        hexPayload.reserve(fileContents.size() - encryptedPrefix.size());

        for (size_t i = encryptedPrefix.size(); i < fileContents.size(); ++i)
        {
            const unsigned char ch = static_cast<unsigned char>(fileContents[i]);
            if (!std::isspace(ch))
            {
                hexPayload.push_back(static_cast<char>(ch));
            }
        }

        std::string plainSnapshot;
        if (!SnapshotEncryptor::decryptFromHex(hexPayload, plainSnapshot))
        {
            return false;
        }

        std::istringstream decryptedInput(plainSnapshot);
        return readSnapshotText(decryptedInput, database);
    }

    std::istringstream plainInput(fileContents);
    return readSnapshotText(plainInput, database);
}
