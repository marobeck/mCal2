PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS server_state (
    id INTEGER PRIMARY KEY CHECK (id = 1),
    global_version INTEGER NOT NULL
);

INSERT OR IGNORE INTO server_state (id, global_version)
VALUES (1, 0);

-- Database mirror
CREATE TABLE IF NOT EXISTS timeblocks (
    uuid TEXT PRIMARY KEY,
    status INTEGER NOT NULL,
    name TEXT NOT NULL,
    description TEXT,
    day_frequency INTEGER NOT NULL,
    duration INTEGER NOT NULL,
    start INTEGER,
    day_start INTEGER,
    completed_datetime INTEGER
);

CREATE TABLE IF NOT EXISTS tasks(
    uuid TEXT PRIMARY KEY,
    timeblock_uuid TEXT NOT NULL,
    name TEXT NOT NULL,
    description TEXT,
    due_date INTEGER,
    priority INTEGER NOT NULL,
    scope INTEGER NOT NULL,
    status INTEGER NOT NULL,
    goal_spec INTEGER NOT NULL,
    completed_datetime INTEGER,
    FOREIGN KEY(timeblock_uuid) REFERENCES timeblocks(uuid) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS habit_entries(
    task_uuid TEXT NOT NULL,
    date TEXT NOT NULL,
    PRIMARY KEY(task_uuid, date),
    FOREIGN KEY(task_uuid) REFERENCES tasks(uuid) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS entry_links(
    parent_uuid TEXT NOT NULL,
    child_uuid TEXT NOT NULL,
    link_type INTEGER NOT NULL,
    PRIMARY KEY(parent_uuid, child_uuid),
    FOREIGN KEY(parent_uuid) REFERENCES tasks(uuid) ON DELETE CASCADE,
    FOREIGN KEY(child_uuid) REFERENCES tasks(uuid) ON DELETE CASCADE
);

-- Ledger tables to track changes to timeblocks, habit entries, and entry links
CREATE TABLE IF NOT EXISTS timeblocks_ledger (
    uuid TEXT PRIMARY KEY,
    server_version INTEGER NOT NULL,
    modified_at INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS tasks_ledger (
    uuid TEXT PRIMARY KEY,
    server_version INTEGER NOT NULL,
    modified_at INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS habit_entries_ledger (
    task_uuid TEXT NOT NULL,
    date TEXT NOT NULL,
    server_version INTEGER NOT NULL,
    modified_at INTEGER NOT NULL,
    PRIMARY KEY(task_uuid, date)
);

CREATE TABLE IF NOT EXISTS entry_links_ledger (
    parent_uuid TEXT NOT NULL,
    child_uuid TEXT NOT NULL,
    server_version INTEGER NOT NULL,
    modified_at INTEGER NOT NULL,
    PRIMARY KEY(parent_uuid, child_uuid)
);