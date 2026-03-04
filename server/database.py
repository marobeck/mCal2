import sqlite3

DB_PATH = "data/server.db"

def get_db() -> sqlite3.Connection:
    conn : sqlite3.Connection = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    conn.execute("PRAGMA foreign_keys = ON;")
    return conn