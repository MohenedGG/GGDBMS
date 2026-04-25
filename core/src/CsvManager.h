#pragma once
#include <string>
#include "Database.h"

class CsvManager
{
private:
    static std::string escapeCsv(const std::string &value);
    static bool parseCsvLine(const std::string &line, std::vector<std::string> &fields);
    static bool readCsvRecordsFromFile(const std::string &csvFilePath,
                                       std::vector<std::vector<std::string>> &records,
                                       std::string &errorMessage);
    static bool csvHeaderMatchesColumns(const Table &table, const std::vector<std::string> &rowValues);
    static bool valueExistsInColumn(const Table &table, size_t columnIndex, const std::string &value);
    static std::string diagnoseInsertFailure(const Database &db, const std::string &tableName, const Rows &row);

public:
    static bool exportTable(const Table &table, const std::string &csvFilePath, std::string &errorMessage);
    static bool importTable(Database &db,
                            const std::string &tableName,
                            const std::string &csvFilePath,
                            size_t &importedRows,
                            std::string &errorMessage);
};
