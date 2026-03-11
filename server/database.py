import sqlite3
import os

# Use environment variable if set (for tests), otherwise default to server.db
DB_PATH = os.environ.get("MCAL_SERVER_DB", "server.db")

def get_db() -> sqlite3.Connection:
    print(f"Connecting to database at {DB_PATH}")
    conn : sqlite3.Connection = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row

    # Get the directory of this file and construct the path to schema.sql
    current_dir = os.path.dirname(os.path.abspath(__file__))
    schema_path = os.path.join(current_dir, "schema.sql")
    
    with open(schema_path, "r") as f:
        schema = f.read()
        print(f"Applying schema from {schema_path}:\n{schema}")
    conn.executescript(schema)
    conn.commit()

    return conn