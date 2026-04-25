GGDB Electron quick start

1) Open terminal in frontend folder
2) Install dependencies:
   npm install

3) Build C++ engine executable:
   npm run build:engine

4) Start desktop UI:
   npm start

UI features:
- Full table viewer (all tables with rows and columns)
- Click table/column/row to auto-fill edit controls
- Schema operations (create/drop table, add/remove column)
- Constraints (primary key, unique, not-null)
- Foreign keys (RESTRICT/CASCADE)
- Row operations (insert, replace selected row, delete selected row)
- Snapshot controls (set path, open file, save as, refresh)

Key files:
- ggdb_node_bridge.js
- electron.main.js
- electron.preload.js
- electron.renderer.html
- electron.renderer.js

JSON CLI commands (engine):
- --json ping
- --json init <snapshotPath> <dbName>
- --json list_tables <snapshotPath>
- --json create_table <snapshotPath> <tableName>
- --json drop_table <snapshotPath> <tableName>
- --json add_column <snapshotPath> <tableName> <columnName> <columnType>
- --json remove_column <snapshotPath> <tableName> <columnName>
- --json set_primary_key <snapshotPath> <tableName> <columnName>
- --json add_unique <snapshotPath> <tableName> <columnName>
- --json add_not_null <snapshotPath> <tableName> <columnName>
- --json add_foreign_key <snapshotPath> <childTable> <childColumn> <parentTable> <parentColumn> <RESTRICT|CASCADE>
- --json insert_row <snapshotPath> <tableName> <value1|value2|...>
- --json delete_row <snapshotPath> <tableName> <rowIndex>
- --json get_table <snapshotPath> <tableName>
