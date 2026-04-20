GGDB bridge-only quick start

This folder currently keeps only bridge files. Renderer UI files were removed
intentionally and can be added later.

1) Open terminal in frontend folder
2) Install dependencies:
   npm install

3) Build C++ engine executable:
   npm run build:engine

4) Optional smoke run for bridge process:
   npm start

Bridge files:
- ggdb_node_bridge.js (Node to C++ JSON CLI bridge)
- electron.main.js (IPC handler + process wiring)
- electron.preload.js (safe API exposure for future renderer)

Snapshot/runtime artifacts:
- ggdb.electron.snapshot (generated while running)

JSON CLI commands (engine):
- --json ping
- --json init <snapshotPath> <dbName>
- --json list_tables <snapshotPath>
- --json create_table <snapshotPath> <tableName>
- --json add_column <snapshotPath> <tableName> <columnName> <columnType>
- --json set_primary_key <snapshotPath> <tableName> <columnName>
- --json add_unique <snapshotPath> <tableName> <columnName>
- --json add_not_null <snapshotPath> <tableName> <columnName>
- --json add_foreign_key <snapshotPath> <childTable> <childColumn> <parentTable> <parentColumn> <RESTRICT|CASCADE>
- --json insert_row <snapshotPath> <tableName> <value1|value2|...>
- --json get_table <snapshotPath> <tableName>
