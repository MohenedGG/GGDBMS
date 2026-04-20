const { execFile } = require('node:child_process');
const { promisify } = require('node:util');

const execFileAsync = promisify(execFile);

class GGDBBridge {
  constructor(options = {}) {
    this.executablePath = options.executablePath || './main.exe';
    this.snapshotPath = options.snapshotPath || './ggdb.snapshot';
    this.cwd = options.cwd;
  }

  async run(command, ...args) {
    const cmdArgs = ['--json', command, ...args];
    const { stdout, stderr } = await execFileAsync(this.executablePath, cmdArgs, {
      cwd: this.cwd,
      windowsHide: true,
      maxBuffer: 1024 * 1024,
    });

    const lines = stdout
      .split(/\r?\n/)
      .map((line) => line.trim())
      .filter(Boolean);

    if (lines.length === 0) {
      throw new Error(`Empty response from engine. stderr: ${stderr || '(none)'}`);
    }

    let response;
    try {
      response = JSON.parse(lines[lines.length - 1]);
    } catch (error) {
      throw new Error(`Invalid JSON response: ${lines[lines.length - 1]}`);
    }

    if (!response.ok) {
      throw new Error(response.message || 'Engine command failed');
    }

    return response.data;
  }

  ping() {
    return this.run('ping');
  }

  initDatabase(dbName) {
    return this.run('init', this.snapshotPath, dbName);
  }

  listTables() {
    return this.run('list_tables', this.snapshotPath);
  }

  createTable(tableName) {
    return this.run('create_table', this.snapshotPath, tableName);
  }

  addColumn(tableName, columnName, columnType) {
    return this.run('add_column', this.snapshotPath, tableName, columnName, columnType);
  }

  setPrimaryKey(tableName, columnName) {
    return this.run('set_primary_key', this.snapshotPath, tableName, columnName);
  }

  addUnique(tableName, columnName) {
    return this.run('add_unique', this.snapshotPath, tableName, columnName);
  }

  addNotNull(tableName, columnName) {
    return this.run('add_not_null', this.snapshotPath, tableName, columnName);
  }

  addForeignKey(childTable, childColumn, parentTable, parentColumn, onDelete = 'RESTRICT') {
    return this.run(
      'add_foreign_key',
      this.snapshotPath,
      childTable,
      childColumn,
      parentTable,
      parentColumn,
      onDelete
    );
  }

  insertRow(tableName, values) {
    const payload = values.join('|');
    return this.run('insert_row', this.snapshotPath, tableName, payload);
  }

  getTable(tableName) {
    return this.run('get_table', this.snapshotPath, tableName);
  }
}

module.exports = { GGDBBridge };
