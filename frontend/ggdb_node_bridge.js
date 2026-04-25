const { execFile } = require("node:child_process");
const { promisify } = require("node:util");

const execFileAsync = promisify(execFile);

function parseEngineResponse(stdoutText) {
    const lines = String(stdoutText || "")
        .split(/\r?\n/)
        .map((line) => line.trim())
        .filter(Boolean);

    if (lines.length === 0) {
        return null;
    }

    try {
        return JSON.parse(lines[lines.length - 1]);
    } catch (_error) {
        return null;
    }
}

class GGDBBridge {
    constructor(options = {}) {
        this.executablePath = options.executablePath || "../main.exe";
        this.snapshotPath = options.snapshotPath || "./ggdb.snapshot";
        this.cwd = options.cwd;
    }

    getSnapshotPath() {
        return this.snapshotPath;
    }

    setSnapshotPath(newSnapshotPath) {
        if (
            typeof newSnapshotPath === "string" &&
            newSnapshotPath.trim() !== ""
        ) {
            this.snapshotPath = newSnapshotPath.trim();
        }
        return this.snapshotPath;
    }

    async run(command, ...args) {
        const cmdArgs = ["--json", command, ...args];
        let stdout = "";
        let stderr = "";
        let response = null;

        try {
            const result = await execFileAsync(this.executablePath, cmdArgs, {
                cwd: this.cwd,
                windowsHide: true,
                maxBuffer: 1024 * 1024,
            });

            stdout = result.stdout || "";
            stderr = result.stderr || "";
            response = parseEngineResponse(stdout);
        } catch (error) {
            stdout = error.stdout || "";
            stderr = error.stderr || "";
            response = parseEngineResponse(stdout);

            if (response && typeof response.message === "string") {
                throw new Error(response.message);
            }

            throw new Error(
                `Engine command failed: ${command}. stderr: ${stderr || "(none)"}`,
            );
        }

        if (!response) {
            throw new Error(
                `Invalid or empty JSON response. stderr: ${stderr || "(none)"}`,
            );
        }

        if (!response.ok) {
            throw new Error(response.message || "Engine command failed");
        }

        return response.data;
    }

    ping() {
        return this.run("ping");
    }

    initDatabase(dbName) {
        return this.run("init", this.snapshotPath, dbName);
    }

    listTables() {
        return this.run("list_tables", this.snapshotPath);
    }

    listForeignKeys() {
        return this.run("list_foreign_keys", this.snapshotPath);
    }

    createTable(tableName) {
        return this.run("create_table", this.snapshotPath, tableName);
    }

    dropTable(tableName) {
        return this.run("drop_table", this.snapshotPath, tableName);
    }

    addColumn(tableName, columnName, columnType) {
        return this.run(
            "add_column",
            this.snapshotPath,
            tableName,
            columnName,
            columnType,
        );
    }

    removeColumn(tableName, columnName) {
        return this.run(
            "remove_column",
            this.snapshotPath,
            tableName,
            columnName,
        );
    }

    setPrimaryKey(tableName, columnName) {
        return this.run(
            "set_primary_key",
            this.snapshotPath,
            tableName,
            columnName,
        );
    }

    addUnique(tableName, columnName) {
        return this.run("add_unique", this.snapshotPath, tableName, columnName);
    }

    addNotNull(tableName, columnName) {
        return this.run(
            "add_not_null",
            this.snapshotPath,
            tableName,
            columnName,
        );
    }

    addForeignKey(
        childTable,
        childColumn,
        parentTable,
        parentColumn,
        onDelete = "RESTRICT",
    ) {
        return this.run(
            "add_foreign_key",
            this.snapshotPath,
            childTable,
            childColumn,
            parentTable,
            parentColumn,
            onDelete,
        );
    }

    insertRow(tableName, values) {
        const payload = values.join("|");
        return this.run("insert_row", this.snapshotPath, tableName, payload);
    }

    deleteRow(tableName, rowIndex) {
        return this.run(
            "delete_row",
            this.snapshotPath,
            tableName,
            String(rowIndex),
        );
    }

    async replaceRow(tableName, rowIndex, values) {
        await this.deleteRow(tableName, rowIndex);
        return this.insertRow(tableName, values);
    }

    getTable(tableName) {
        return this.run("get_table", this.snapshotPath, tableName);
    }

    exportCsv(tableName, csvFilePath) {
        return this.run(
            "export_csv",
            this.snapshotPath,
            tableName,
            csvFilePath,
        );
    }

    importCsv(tableName, csvFilePath) {
        return this.run(
            "import_csv",
            this.snapshotPath,
            tableName,
            csvFilePath,
        );
    }
}

module.exports = { GGDBBridge };
