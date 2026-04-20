#pragma once
#include <string>

enum class DataTypes
{
    INT,
    FLOAT,
    TEXT,
    BOOL
};

enum class SQLKeywords
{
    SELECT,
    FROM,
    WHERE,
    INSERT,
    INTO,
    VALUES,
    UPDATE,
    SET,
    DELETE
};

enum class NoSQLKeywords
{
    FIND,
    INSERT,
    UPDATE,
    DELETE
};

enum class CashKeywords
{
    GET,
    SET,
    DELETE
};

enum class LogicalOperators
{
    AND,
    OR,
    NOT
};

enum class ComparisonOperators
{
    EQUALS,
    NOT_EQUALS,
    GREATER_THAN,
    LESS_THAN,
    GREATER_THAN_OR_EQUALS,
    LESS_THAN_OR_EQUALS
};

enum class ArithmeticOperators
{
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE
};

enum class ReferentialAction
{
    RESTRICT,
    CASCADE
};

struct ForeignKey
{
    std::string childTableName;
    std::string childColumnName;
    std::string parentTableName;
    std::string parentColumnName;
    ReferentialAction onDelete = ReferentialAction::RESTRICT;
};