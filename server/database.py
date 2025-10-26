import sqlite3

DB_PATH = "server/data.db"

def get_db():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

def insert_sample(device, timestamp, active_window):
    db = get_db()
    db.execute(
        "INSERT INTO samples (device, timestamp, active_window) VALUES (?, ?, ?)",
        (device, timestamp, active_window)
    )
    db.commit()
    db.close()
