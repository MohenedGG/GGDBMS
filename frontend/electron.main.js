const path = require("node:path");
const { app, BrowserWindow, ipcMain } = require("electron");
const { GGDBBridge } = require("./ggdb_node_bridge");

const appRoot = __dirname;
const engineRoot = path.join(appRoot, "..");
const bridge = new GGDBBridge({
    executablePath: path.join(engineRoot, "main.exe"),
    snapshotPath: path.join(appRoot, "ggdb.electron.snapshot"),
    cwd: engineRoot,
});

function createWindow() {
    const mainWindow = new BrowserWindow({
                width: 620,
                height: 280,
                backgroundColor: "#1f2933",
        webPreferences: {
            contextIsolation: true,
            nodeIntegration: false,
            preload: path.join(appRoot, "electron.preload.js"),
        },
    });

        const placeholder = `
        <!doctype html>
        <html lang="en">
            <head>
                <meta charset="UTF-8" />
                <meta name="viewport" content="width=device-width, initial-scale=1.0" />
                <title>GGDB Bridge</title>
                <style>
                    body {
                        margin: 0;
                        min-height: 100vh;
                        display: grid;
                        place-items: center;
                        background: #111827;
                        color: #e5e7eb;
                        font-family: "Segoe UI", sans-serif;
                    }

                    .card {
                        border: 1px solid #374151;
                        border-radius: 12px;
                        padding: 20px;
                        max-width: 460px;
                        background: #1f2937;
                    }

                    h1 {
                        margin: 0 0 10px;
                        font-size: 20px;
                    }

                    p {
                        margin: 0;
                        color: #cbd5e1;
                        line-height: 1.5;
                    }
                </style>
            </head>
            <body>
                <div class="card">
                    <h1>GGDB bridge is ready</h1>
                    <p>
                        Renderer UI is intentionally removed for now. Keep working in bridge
                        files, then add your interface later.
                    </p>
                </div>
            </body>
        </html>`;

        mainWindow.loadURL(
                `data:text/html;charset=UTF-8,${encodeURIComponent(placeholder)}`,
        );
}

ipcMain.handle("ggdb:call", async (_event, method, args = []) => {
    if (typeof bridge[method] !== "function") {
        throw new Error(`Unknown bridge method: ${method}`);
    }

    return bridge[method](...args);
});

app.whenReady().then(() => {
    createWindow();

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
