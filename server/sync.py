from .database import get_db

TABLES = {
    "timeblocks": {"pk": ["uuid"], "ledger": "timeblocks_ledger"},
    "tasks": {"pk": ["uuid"], "ledger": "tasks_ledger"},
    "habit_entries": {"pk": ["task_uuid", "date"], "ledger": "habit_entries_ledger"},
    "entry_links": {
        "pk": ["parent_uuid", "child_uuid"],
        "ledger": "entry_links_ledger",
    },
}


def get_global_version(conn):
    return conn.execute(
        "SELECT global_version FROM server_state WHERE id = 1"
    ).fetchone()[0]


def increment_global_version(conn):
    conn.execute(
        "UPDATE server_state SET global_version = global_version + 1 WHERE id = 1"
    )
    return get_global_version(conn)


def apply_entry(conn, table, data):
    config = TABLES[table]
    pk_fields = config["pk"]
    ledger_table = config["ledger"]

    where_clause = " AND ".join([f"{k}=?" for k in pk_fields])
    pk_values = [data[k] for k in pk_fields]

    # --- Check existing modified_at to prevent applying stale updates ---

    existing = conn.execute(
        f"SELECT modified_at FROM {ledger_table} WHERE {where_clause}", pk_values
    ).fetchone()

    if existing and existing["modified_at"] >= data["modified_at"]:
        return False  # ignore older update

    # --- Check if this is a deletion of a non-existent record ---

    if data.get("deleted_at"):
        existing = conn.execute(
            f"SELECT 1 FROM {table} WHERE {where_clause}", pk_values
        ).fetchone()
        if not existing:
            return False  # ignore deletion of non-existent record

    # --- Apply update ---

    # Split incoming data's parameters between main table and ledger
    main_data = {}
    for k, v in data.items():
        if k not in {"modified_at", "deleted_at"}:
            main_data[k] = v

    print(f"Applying to table {table} with main_data {main_data} and ledger data modified_at={data['modified_at']} deleted_at={data.get('deleted_at')}")

    # Delete or update main table record
    if data.get("deleted_at"):
        conn.execute(f"DELETE FROM {table} WHERE {where_clause}", pk_values)
    else:
        # Replace into main table
        columns = ", ".join(main_data.keys())
        placeholders = ", ".join(["?"] * len(main_data))
        conn.execute(
            f"REPLACE INTO {table} ({columns}) VALUES ({placeholders})", list(main_data.values())
        )

    # Increment version
    new_version = increment_global_version(conn)

    # Update ledger
    ledger_columns = pk_fields + ["server_version", "modified_at", "deleted"]
    ledger_values = pk_values + [new_version, data["modified_at"], 1 if data.get("deleted_at") else 0]

    ledger_placeholders = ", ".join(["?"] * len(ledger_columns))
    conn.execute(
        f"""
        REPLACE INTO {ledger_table}
        ({", ".join(ledger_columns)})
        VALUES ({ledger_placeholders})
        """,
        ledger_values,
    )

    return True


def collect_deltas(conn, last_version):
    results = []

    for table, config in TABLES.items():
        ledger = config["ledger"]
        pk_fields = config["pk"]

        # Collect both active and deleted records from ledger
        ledger_rows = conn.execute(
            f"""
            SELECT l.*, t.*
            FROM {ledger} l
            LEFT JOIN {table} t
            ON {" AND ".join([f"l.{k}=t.{k}" for k in pk_fields])}
            WHERE l.server_version > ?
            """,
            (last_version,),
        ).fetchall()

        for row in ledger_rows:
            row_dict = dict(row)
            data = {}

            # Add primary key fields
            for pk in pk_fields:
                data[pk] = row_dict[pk]

            # Add metadata
            data["modified_at"] = row_dict["modified_at"]
            data["server_version"] = row_dict["server_version"]

            # Check if this is a deletion
            if row_dict["deleted"]:
                data["deleted"] = True
            else:
                data["deleted"] = False
                # Add all other fields from main table
                for k, v in row_dict.items():
                    if k not in pk_fields and k not in {"modified_at", "server_version", "deleted"}:
                        data[k] = v

            results.append({"table": table, "data": data})

    print(f"Collected {len(results)} deltas since version {last_version} -> {get_global_version(conn)}")
    return results


def process_sync(request):
    conn = get_db()
    try:
        conn.execute("BEGIN")

        print(f"Processing sync for client {request.client_id} with last_server_version {request.last_server_version}")
        print(f"Received {len(request.entries)} entries from client")

        for entry in request.entries:
            print(f"Applying entry for table `{entry.table}` with data {entry.data}")
            apply_entry(conn, entry.table, entry.data)

        new_version = get_global_version(conn)
        deltas = collect_deltas(conn, request.last_server_version)

        print(f"Sync processed for client {request.client_id}. New version: {new_version}")
        for delta in deltas:
            print(f"Delta to send: table `{delta['table']}` with data {delta['data']}")

        conn.commit()

        return new_version, deltas

    except Exception as e:
        conn.rollback()
        raise e

    finally:
        conn.close()
