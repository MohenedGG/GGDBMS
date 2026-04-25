const { contextBridge, ipcRenderer } = require("electron");

function normalizeIpcErrorMessage(error) {
    const raw =
        error && typeof error.message === "string"
            ? error.message
            : String(error || "Unknown error.");

    const prefix = "Error invoking remote method 'ggdb:call': Error: ";
    if (raw.startsWith(prefix)) {
        return raw.slice(prefix.length).trim();
    }

    const nestedErrorIndex = raw.lastIndexOf("Error: ");
    if (nestedErrorIndex !== -1) {
        return raw.slice(nestedErrorIndex + "Error: ".length).trim();
    }

    return raw;
}

async function call(method, ...args) {
    try {
        return await ipcRenderer.invoke("ggdb:call", method, args);
    } catch (error) {
        throw new Error(normalizeIpcErrorMessage(error));
    }
}

contextBridge.exposeInMainWorld("ggdb", {
    ping: () => call("ping"),
    getSnapshotPath: () => call("getSnapshotPath"),
    setSnapshotPath: (snapshotPath) => call("setSnapshotPath", snapshotPath),
    openSnapshotDialog: () => call("openSnapshotDialog"),
    saveSnapshotAsDialog: () => call("saveSnapshotAsDialog"),
    exportCsvDialog: (tableName) => call("exportCsvDialog", tableName),
    importCsvDialog: (tableName) => call("importCsvDialog", tableName),
    initDatabase: (dbName) => call("initDatabase", dbName),
    listTables: () => call("listTables"),
    listForeignKeys: () => call("listForeignKeys"),
    createTable: (tableName) => call("createTable", tableName),
    dropTable: (tableName) => call("dropTable", tableName),
    addColumn: (tableName, columnName, columnType) =>
        call("addColumn", tableName, columnName, columnType),
    removeColumn: (tableName, columnName) =>
        call("removeColumn", tableName, columnName),
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
    deleteRow: (tableName, rowIndex) => call("deleteRow", tableName, rowIndex),
    replaceRow: (tableName, rowIndex, values) =>
        call("replaceRow", tableName, rowIndex, values),
    getTable: (tableName) => call("getTable", tableName),
});
