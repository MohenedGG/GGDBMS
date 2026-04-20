const { contextBridge, ipcRenderer } = require("electron");

async function call(method, ...args) {
    return ipcRenderer.invoke("ggdb:call", method, args);
}

contextBridge.exposeInMainWorld("ggdb", {
    ping: () => call("ping"),
    initDatabase: (dbName) => call("initDatabase", dbName),
    listTables: () => call("listTables"),
    createTable: (tableName) => call("createTable", tableName),
    addColumn: (tableName, columnName, columnType) =>
        call("addColumn", tableName, columnName, columnType),
    setPrimaryKey: (tableName, columnName) =>
        call("setPrimaryKey", tableName, columnName),
    addUnique: (tableName, columnName) =>
        call("addUnique", tableName, columnName),
    addNotNull: (tableName, columnName) =>
        call("addNotNull", tableName, columnName),
    addForeignKey: (
        childTable,
        childColumn,
        parentTable,
        parentColumn,
        onDelete,
    ) =>
        call(
            "addForeignKey",
            childTable,
            childColumn,
            parentTable,
            parentColumn,
            onDelete,
        ),
    insertRow: (tableName, values) => call("insertRow", tableName, values),
    getTable: (tableName) => call("getTable", tableName),
});
