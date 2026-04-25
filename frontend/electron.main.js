const path = require("node:path");
const fs = require("node:fs/promises");
const { app, BrowserWindow, ipcMain, dialog } = require("electron");
const { GGDBBridge } = require("./ggdb_node_bridge");

const appRoot = __dirname;
const engineRoot = path.join(appRoot, "..");
const settingsFilePath = path.join(appRoot, ".ggdb-ui-settings.json");
const bridge = new GGDBBridge({
    executablePath: path.join(engineRoot, "main.exe"),
    snapshotPath: path.join(appRoot, "ggdb.electron.snapshot"),
    cwd: engineRoot,
});

async function loadSavedSnapshotPath() {
    try {
        const raw = await fs.readFile(settingsFilePath, "utf8");
        const parsed = JSON.parse(raw);
        if (
            parsed &&
            typeof parsed.snapshotPath === "string" &&
            parsed.snapshotPath.trim() !== ""
        ) {
            return parsed.snapshotPath.trim();
        }
    } catch (_error) {
        // Ignore missing/corrupt settings file and keep default path.
    }

    return null;
}

async function saveSnapshotPath(snapshotPath) {
    try {
        await fs.writeFile(
            settingsFilePath,
            JSON.stringify({ snapshotPath }, null, 2),
            "utf8",
        );
    } catch (_error) {
        // Ignore settings write failures to keep app functional.
    }
}

function createWindow() {
    const mainWindow = new BrowserWindow({
        width: 1420,
        height: 860,
        minWidth: 1080,
        minHeight: 700,
        backgroundColor: "#1f2933",
        title: "GGDBMS",
        icon: path.join(appRoot, "logo.png"),
        autoHideMenuBar: true,
        webPreferences: {
            contextIsolation: true,
            nodeIntegration: false,
            preload: path.join(appRoot, "electron.preload.js"),
        },
    });

    mainWindow.loadFile(path.join(appRoot, "electron.renderer.html"));
}

ipcMain.handle("ggdb:call", async (_event, method, args = []) => {
    if (method === "getSnapshotPath") {
        return { path: bridge.getSnapshotPath() };
    }

    if (method === "setSnapshotPath") {
        const updatedPath = bridge.setSnapshotPath(args[0]);
        await saveSnapshotPath(updatedPath);
        return { path: updatedPath };
    }

    if (method === "openSnapshotDialog") {
        const result = await dialog.showOpenDialog({
            title: "Open GGDB Snapshot",
            properties: ["openFile"],
            filters: [
                {
                    name: "GGDB Snapshot",
                    extensions: ["snapshot", "ggdb", "db"],
                },
                { name: "All Files", extensions: ["*"] },
            ],
        });

        if (result.canceled || result.filePaths.length === 0) {
            return { canceled: true };
        }

        const selectedPath = result.filePaths[0];
        bridge.setSnapshotPath(selectedPath);
        await saveSnapshotPath(selectedPath);

        // Validate quickly so broken files fail immediately.
        await bridge.listTables();

        return { canceled: false, path: selectedPath };
    }

    if (method === "saveSnapshotAsDialog") {
        const currentSnapshotPath = bridge.getSnapshotPath();
        const result = await dialog.showSaveDialog({
            title: "Save GGDB Snapshot As",
            defaultPath: currentSnapshotPath,
            filters: [
                {
                    name: "GGDB Snapshot",
                    extensions: ["snapshot", "ggdb", "db"],
                },
                { name: "All Files", extensions: ["*"] },
            ],
        });

        if (result.canceled || !result.filePath) {
            return { canceled: true };
        }

        await fs.copyFile(currentSnapshotPath, result.filePath);
        return { canceled: false, path: result.filePath };
    }

    if (method === "exportCsvDialog") {
        const tableName = typeof args[0] === "string" ? args[0].trim() : "";
        if (!tableName) {
            throw new Error("Table name is required for CSV export.");
        }

        const defaultCsvPath = path.join(
            path.dirname(bridge.getSnapshotPath()),
            `${tableName}.csv`,
        );

        const result = await dialog.showSaveDialog({
            title: `Export Table '${tableName}' to CSV`,
            defaultPath: defaultCsvPath,
            filters: [
                { name: "CSV Files", extensions: ["csv"] },
                { name: "All Files", extensions: ["*"] },
            ],
        });

        if (result.canceled || !result.filePath) {
            return { canceled: true };
        }

        await bridge.exportCsv(tableName, result.filePath);
        return { canceled: false, path: result.filePath };
    }

    if (method === "importCsvDialog") {
        const tableName = typeof args[0] === "string" ? args[0].trim() : "";
        if (!tableName) {
            throw new Error("Table name is required for CSV import.");
        }

        const result = await dialog.showOpenDialog({
            title: `Import CSV into '${tableName}'`,
            properties: ["openFile"],
            filters: [
                { name: "CSV Files", extensions: ["csv"] },
                { name: "All Files", extensions: ["*"] },
            ],
        });

        if (result.canceled || result.filePaths.length === 0) {
            return { canceled: true };
        }

        const csvPath = result.filePaths[0];
        const importResult = await bridge.importCsv(tableName, csvPath);
        return {
            canceled: false,
            path: csvPath,
            ...importResult,
        };
    }

    if (typeof bridge[method] !== "function") {
        throw new Error(`Unknown bridge method: ${method}`);
    }

    return bridge[method](...args);
});

app.whenReady().then(() => {
    loadSavedSnapshotPath().then((savedPath) => {
        if (savedPath) {
            bridge.setSnapshotPath(savedPath);
        }

        createWindow();
    });

    app.on("activate", () => {
        if (BrowserWindow.getAllWindows().length === 0) {
            createWindow();
        }
    });
});

app.on("window-all-closed", () => {
    if (process.platform !== "darwin") {
        app.quit();
    }
});
