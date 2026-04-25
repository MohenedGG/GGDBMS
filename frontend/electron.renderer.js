const state = {
    tables: [],
    foreignKeys: [],
    selectedTable: "",
    selectedColumn: "",
    selectedRowIndex: null,
};

const byId = (id) => document.getElementById(id);

function setStatus(message, isError = false) {
    const status = byId("status");
    status.textContent = message;
    status.style.color = isError ? "#ff8da2" : "#dce8ff";
}

function setLog(label, payload, isError = false) {
    const log = byId("log");
    const details =
        typeof payload === "string"
            ? payload
            : JSON.stringify(payload, null, 2);
    log.textContent = `[${new Date().toLocaleTimeString()}] ${label}\n${details}`;
    log.style.color = isError ? "#ff8da2" : "#bcd6ff";
}

function normalizeValuesInput(raw) {
    const separator = raw.includes("|") ? "|" : ",";
    return raw.split(separator).map((item) => item.trim());
}

function getTableByName(tableName) {
    return state.tables.find((table) => table.name === tableName);
}

function getOutgoingRelations(tableName) {
    return state.foreignKeys.filter((fk) => fk.childTable === tableName);
}

function getIncomingRelations(tableName) {
    return state.foreignKeys.filter((fk) => fk.parentTable === tableName);
}

function relationLabel(fk, direction) {
    if (direction === "outgoing") {
        return `${fk.childColumn} -> ${fk.parentTable}.${fk.parentColumn} (${fk.onDelete})`;
    }

    return `${fk.childTable}.${fk.childColumn} -> ${fk.parentColumn} (${fk.onDelete})`;
}

function renderRelationChips(relations, direction) {
    if (!relations.length) {
        return '<span class="meta">none</span>';
    }

    return relations
        .map(
            (fk) =>
                `<span class="relation-chip ${direction === "incoming" ? "incoming" : ""}">${relationLabel(fk, direction)}</span>`,
        )
        .join("");
}

function renderRelationsMatrix() {
    const wrap = byId("relationsWrap");

    if (state.tables.length === 0) {
        wrap.innerHTML = '<div class="empty">No relations yet.</div>';
        return;
    }

    const rows = state.tables
        .map((table) => {
            const outgoing = getOutgoingRelations(table.name);
            const incoming = getIncomingRelations(table.name);

            return `
                <tr>
                    <td>${table.name}</td>
                    <td class="relations-cell">${renderRelationChips(outgoing, "outgoing")}</td>
                    <td class="relations-cell">${renderRelationChips(incoming, "incoming")}</td>
                </tr>
            `;
        })
        .join("");

    wrap.innerHTML = `
        <table class="relations-table">
            <thead>
                <tr>
                    <th>Table</th>
                    <th>Outgoing Relations</th>
                    <th>Incoming Relations</th>
                </tr>
            </thead>
            <tbody>${rows}</tbody>
        </table>
    `;
}

function updateSelectionChips() {
    byId("selectedTableChip").textContent =
        `Table: ${state.selectedTable || "-"}`;
    byId("selectedColumnChip").textContent =
        `Column: ${state.selectedColumn || "-"}`;
    byId("selectedRowChip").textContent =
        state.selectedRowIndex === null
            ? "Row: -"
            : `Row: ${state.selectedRowIndex}`;
}

function clearRowSelection() {
    state.selectedRowIndex = null;
    byId("selectedRowIndex").value = "";
    updateSelectionChips();
}

function renderTablePicker() {
    const picker = byId("tablePicker");
    if (!picker) {
        return;
    }

    picker.innerHTML = "";

    if (state.tables.length === 0) {
        const option = document.createElement("option");
        option.value = "";
        option.textContent = "No tables available";
        picker.appendChild(option);
        picker.disabled = true;
        return;
    }

    picker.disabled = false;
    for (const table of state.tables) {
        const option = document.createElement("option");
        option.value = table.name;
        option.textContent = table.name;
        picker.appendChild(option);
    }

    if (state.selectedTable && getTableByName(state.selectedTable)) {
        picker.value = state.selectedTable;
    }
}

function selectTable(tableName) {
    state.selectedTable = tableName;
    byId("tableName").value = tableName;
    byId("insertTable").value = tableName;
    byId("csvTable").value = tableName; // Keep CSV table selection in sync
    byId("fkChildTable").value = tableName;
    const tablePicker = byId("tablePicker");
    if (tablePicker) {
        tablePicker.value = tableName;
    }
    clearRowSelection();
    updateSelectionChips();
}

function selectColumn(tableName, columnName, columnType) {
    state.selectedTable = tableName;
    state.selectedColumn = columnName;
    byId("tableName").value = tableName;
    byId("insertTable").value = tableName;
    byId("columnName").value = columnName;
    byId("columnType").value = (columnType || "TEXT").toUpperCase();
    activateLeftTab("schema");
    updateSelectionChips();
}

function selectRow(tableName, rowIndex, rowValues) {
    state.selectedTable = tableName;
    state.selectedRowIndex = rowIndex;
    byId("tableName").value = tableName;
    byId("insertTable").value = tableName;
    byId("selectedRowIndex").value = String(rowIndex);
    byId("insertValues").value = rowValues.join(",");
    activateLeftTab("rows");
    updateSelectionChips();
}

function renderTables() {
    const container = byId("tablesGrid");
    const activeSummary = byId("activeTableSummary");
    container.innerHTML = "";

    if (state.tables.length === 0) {
        container.innerHTML =
            '<div class="empty">No tables found in this snapshot.</div>';
        byId("meta").textContent = "0 tables";
        activeSummary.textContent =
            "Select a table to view its data and constraints.";
        return;
    }

    const totalRows = state.tables.reduce(
        (sum, table) => sum + table.rows.length,
        0,
    );
    const activeTable = getTableByName(state.selectedTable) || state.tables[0];

    if (activeTable.name !== state.selectedTable) {
        selectTable(activeTable.name);
    }

    const uniqueText = activeTable.unique.length
        ? activeTable.unique.join(", ")
        : "none";
    const notNullText = activeTable.notNull.length
        ? activeTable.notNull.join(", ")
        : "none";
    const outgoing = getOutgoingRelations(activeTable.name);
    const incoming = getIncomingRelations(activeTable.name);

    activeSummary.textContent = `${activeTable.name} | ${activeTable.columns.length} columns | ${activeTable.rows.length} rows | PK: ${activeTable.primaryKey || "none"}`;

    const card = document.createElement("article");
    card.className = "table-card";

    card.innerHTML = `
                <div class="table-top">
                    <div class="table-name" data-action="select-table" data-table="${activeTable.name}">${activeTable.name}</div>
                    <div class="meta">${activeTable.columns.length} columns / ${activeTable.rows.length} rows</div>
                </div>
                <div class="table-constraints">
                    <div class="constraint pk">PK: ${activeTable.primaryKey || "none"}</div>
                    <div class="constraint">UNIQUE: ${uniqueText}</div>
                    <div class="constraint">NOT NULL: ${notNullText}</div>
                </div>
                <div style="margin-top:8px;overflow:auto;">
                    <table>
                        <thead>
                            <tr>
                                <th>#</th>
                                ${activeTable.columns
                                    .map(
                                        (column) =>
                                            `<th class="col-head ${
                                                state.selectedTable ===
                                                    activeTable.name &&
                                                state.selectedColumn ===
                                                    column.name
                                                    ? "is-selected"
                                                    : ""
                                            }" data-action="select-column" data-table="${activeTable.name}" data-column="${column.name}" data-type="${column.type}">${column.name}<br /><span style="font-size:10px;color:#86a8df;">${column.type}</span></th>`,
                                    )
                                    .join("")}
                            </tr>
                        </thead>
                        <tbody>
                            ${activeTable.rows
                                .map(
                                    (row, rowIndex) => `
                                                <tr class="row-click ${
                                                    state.selectedTable ===
                                                        activeTable.name &&
                                                    state.selectedRowIndex ===
                                                        rowIndex
                                                        ? "is-selected"
                                                        : ""
                                                }" data-action="select-row" data-table="${activeTable.name}" data-row-index="${rowIndex}">
                                                    <td>${rowIndex}</td>
                                                    ${row
                                                        .map(
                                                            (value) =>
                                                                `<td>${String(value)}</td>`,
                                                        )
                                                        .join("")}
                                                </tr>
                                            `,
                                )
                                .join("")}
                        </tbody>
                    </table>
                </div>
                <div style="margin-top:8px;">
                        <div class="meta" style="margin-bottom:4px;">Relations with other tables</div>
                        <div class="relations-cell"><strong>Outgoing:</strong> ${renderRelationChips(outgoing, "outgoing")}</div>
                        <div class="relations-cell" style="margin-top:4px;"><strong>Incoming:</strong> ${renderRelationChips(incoming, "incoming")}</div>
                </div>
        `;

    container.appendChild(card);

    byId("meta").textContent =
        `${state.tables.length} tables / ${totalRows} rows`;
}

async function refreshAll() {
    const [tablesResult, relationsResult] = await Promise.all([
        window.ggdb.listTables(),
        window.ggdb.listForeignKeys(),
    ]);

    const tables = [];

    for (const tableName of tablesResult.tables || []) {
        const table = await window.ggdb.getTable(tableName);
        tables.push(table);
    }

    state.tables = tables;
    state.foreignKeys = relationsResult.foreignKeys || [];

    if (state.tables.length === 0) {
        state.selectedTable = "";
        state.selectedColumn = "";
        clearRowSelection();
    } else if (!state.selectedTable || !getTableByName(state.selectedTable)) {
        selectTable(state.tables[0].name);
    }

    renderTablePicker();
    renderRelationsMatrix();
    renderTables();
    updateSelectionChips();
}

async function runAction(label, fn, refresh = true) {
    setStatus(`Running: ${label}`);
    try {
        const result = await fn();
        setLog(label, result || {});
        if (refresh) {
            await refreshAll();
        }
        setStatus(`Done: ${label}`);
        return result;
    } catch (error) {
        setStatus(`Failed: ${label} (${error.message || String(error)})`, true);
        setLog(label, { error: error.message || String(error) }, true);
        return null;
    }
}

function getActiveTableName() {
    return byId("tableName").value.trim() || state.selectedTable;
}

function getActiveColumnName() {
    return byId("columnName").value.trim() || state.selectedColumn;
}

function getActiveRowTableName() {
    return byId("insertTable").value.trim() || state.selectedTable;
}

function getActiveCsvTableName() {
    return byId("csvTable").value.trim() || state.selectedTable;
}

function requireValue(value, message) {
    if (!value) {
        throw new Error(message);
    }
}

function activateLeftTab(tabName) {
    const tabs = document.querySelectorAll(".left-tab");
    const panels = document.querySelectorAll("[data-left-panel]");

    tabs.forEach((tab) => {
        tab.classList.toggle("is-active", tab.dataset.leftTab === tabName);
    });

    panels.forEach((panel) => {
        panel.classList.toggle(
            "is-hidden",
            panel.dataset.leftPanel !== tabName,
        );
    });
}

function bindLeftTabs() {
    const tabsWrap = byId("leftTabs");
    if (!tabsWrap) {
        return;
    }

    tabsWrap.addEventListener("click", (event) => {
        const tabButton = event.target.closest("[data-left-tab]");
        if (!tabButton) {
            return;
        }

        activateLeftTab(tabButton.dataset.leftTab);
    });

    activateLeftTab("database");
}

function bindButtons() {
    byId("tablePicker").addEventListener("change", (event) => {
        const tableName = event.target.value;
        if (!tableName) {
            return;
        }

        selectTable(tableName);
        renderTables();
    });

    byId("btnPing").addEventListener("click", () =>
        runAction("Ping", () => window.ggdb.ping(), false),
    );

    byId("btnSetSnapshot").addEventListener("click", () =>
        runAction(
            "Set Snapshot Path",
            async () => {
                const snapshotPath = byId("snapshotPath").value.trim();
                requireValue(snapshotPath, "Snapshot path is required.");
                const result = await window.ggdb.setSnapshotPath(snapshotPath);
                byId("snapshotPath").value = result.path;
                return result;
            },
            false,
        ),
    );

    byId("btnOpenSnapshot").addEventListener("click", async () => {
        const result = await runAction(
            "Open Snapshot File",
            () => window.ggdb.openSnapshotDialog(),
            false,
        );

        if (result && !result.canceled) {
            byId("snapshotPath").value = result.path;
            await runAction("Load Snapshot", () => refreshAll(), false);
        }
    });

    byId("btnSaveSnapshot").addEventListener("click", () =>
        runAction(
            "Save Snapshot As",
            () => window.ggdb.saveSnapshotAsDialog(),
            false,
        ),
    );

    byId("btnInit").addEventListener("click", () =>
        runAction("Initialize Database", async () => {
            const dbName = byId("dbName").value.trim() || "GGDBDesktop";
            return window.ggdb.initDatabase(dbName);
        }),
    );

    byId("btnRefreshAll").addEventListener("click", () =>
        runAction("Refresh Tables", () => refreshAll(), false),
    );

    byId("btnCreateTable").addEventListener("click", () =>
        runAction("Create Table", async () => {
            const tableName = getActiveTableName();
            requireValue(tableName, "Table name is required.");
            return window.ggdb.createTable(tableName);
        }),
    );

    byId("btnDropTable").addEventListener("click", () =>
        runAction("Drop Table", async () => {
            const tableName = getActiveTableName();
            requireValue(tableName, "Select table to drop.");
            return window.ggdb.dropTable(tableName);
        }),
    );

    byId("btnAddColumn").addEventListener("click", () =>
        runAction("Add Column", async () => {
            const tableName = getActiveTableName();
            const columnName = byId("columnName").value.trim();
            const columnType = byId("columnType").value.trim() || "TEXT";
            requireValue(tableName, "Table is required.");
            requireValue(columnName, "Column name is required.");
            return window.ggdb.addColumn(tableName, columnName, columnType);
        }),
    );

    byId("btnRemoveColumn").addEventListener("click", () =>
        runAction("Remove Column", async () => {
            const tableName = getActiveTableName();
            const columnName = getActiveColumnName();
            requireValue(tableName, "Table is required.");
            requireValue(columnName, "Column is required.");
            return window.ggdb.removeColumn(tableName, columnName);
        }),
    );

    byId("btnSetPk").addEventListener("click", () =>
        runAction("Set Primary Key", async () => {
            const tableName = getActiveTableName();
            const columnName = getActiveColumnName();
            requireValue(tableName, "Table is required.");
            requireValue(columnName, "Column is required.");
            return window.ggdb.setPrimaryKey(tableName, columnName);
        }),
    );

    byId("btnAddUnique").addEventListener("click", () =>
        runAction("Add Unique", async () => {
            const tableName = getActiveTableName();
            const columnName = getActiveColumnName();
            requireValue(tableName, "Table is required.");
            requireValue(columnName, "Column is required.");
            return window.ggdb.addUnique(tableName, columnName);
        }),
    );

    byId("btnAddNotNull").addEventListener("click", () =>
        runAction("Add Not Null", async () => {
            const tableName = getActiveTableName();
            const columnName = getActiveColumnName();
            requireValue(tableName, "Table is required.");
            requireValue(columnName, "Column is required.");
            return window.ggdb.addNotNull(tableName, columnName);
        }),
    );

    byId("btnAddFk").addEventListener("click", () =>
        runAction("Add Foreign Key", async () => {
            const childTable = byId("fkChildTable").value.trim();
            const childColumn = byId("fkChildColumn").value.trim();
            const parentTable = byId("fkParentTable").value.trim();
            const parentColumn = byId("fkParentColumn").value.trim();
            const onDelete = byId("fkOnDelete").value.trim() || "RESTRICT";
            requireValue(childTable, "Child table is required.");
            requireValue(childColumn, "Child column is required.");
            requireValue(parentTable, "Parent table is required.");
            requireValue(parentColumn, "Parent column is required.");
            return window.ggdb.addForeignKey(
                childTable,
                childColumn,
                parentTable,
                parentColumn,
                onDelete,
            );
        }),
    );

    byId("btnInsertRow").addEventListener("click", () =>
        runAction("Insert Row", async () => {
            const tableName = getActiveRowTableName();
            requireValue(tableName, "Table is required.");
            const values = normalizeValuesInput(byId("insertValues").value);
            return window.ggdb.insertRow(tableName, values);
        }),
    );

    byId("btnReplaceRow").addEventListener("click", () =>
        runAction("Replace Selected Row", async () => {
            const tableName = getActiveRowTableName();
            requireValue(tableName, "Table is required.");
            if (state.selectedRowIndex === null) {
                throw new Error("Select a row first.");
            }
            const values = normalizeValuesInput(byId("insertValues").value);
            return window.ggdb.replaceRow(
                tableName,
                state.selectedRowIndex,
                values,
            );
        }),
    );

    byId("btnDeleteRow").addEventListener("click", () =>
        runAction("Delete Selected Row", async () => {
            const tableName = getActiveRowTableName();
            requireValue(tableName, "Table is required.");
            if (state.selectedRowIndex === null) {
                throw new Error("Select a row first.");
            }
            return window.ggdb.deleteRow(tableName, state.selectedRowIndex);
        }),
    );

    byId("btnImportCsv").addEventListener("click", () =>
        runAction("Import CSV", async () => {
            const tableName = getActiveCsvTableName();
            requireValue(tableName, "Table is required.");
            const result = await window.ggdb.importCsvDialog(tableName);
            if (result && !result.canceled) {
                byId("csvTable").value = tableName;
            }
            return result;
        }),
    );

    byId("btnExportCsv").addEventListener("click", () =>
        runAction(
            "Export CSV",
            async () => {
                const tableName = getActiveCsvTableName();
                requireValue(tableName, "Table is required.");
                const result = await window.ggdb.exportCsvDialog(tableName);
                if (result && !result.canceled) {
                    byId("csvTable").value = tableName;
                }
                return result;
            },
            false,
        ),
    );

    byId("tablesGrid").addEventListener("click", (event) => {
        const tableNode = event.target.closest("[data-action='select-table']");
        if (tableNode) {
            selectTable(tableNode.dataset.table);
            renderTables();
            return;
        }

        const columnNode = event.target.closest(
            "[data-action='select-column']",
        );
        if (columnNode) {
            selectColumn(
                columnNode.dataset.table,
                columnNode.dataset.column,
                columnNode.dataset.type,
            );
            renderTables();
            return;
        }

        const rowNode = event.target.closest("[data-action='select-row']");
        if (rowNode) {
            const tableName = rowNode.dataset.table;
            const rowIndex = Number(rowNode.dataset.rowIndex);
            const table = getTableByName(tableName);
            if (table && table.rows[rowIndex]) {
                selectRow(tableName, rowIndex, table.rows[rowIndex]);
                renderTables();
            }
        }
    });
}

async function boot() {
    bindLeftTabs();
    bindButtons();
    updateSelectionChips();

    try {
        const snapshot = await window.ggdb.getSnapshotPath();
        byId("snapshotPath").value = snapshot.path;
    } catch (error) {
        setStatus(
            `Snapshot path load failed: ${error.message || String(error)}`,
            true,
        );
    }

    try {
        await refreshAll();
        setStatus("Ready.");
    } catch (_error) {
        setStatus("No snapshot loaded yet. Initialize DB or open a file.");
    }
}

boot();
