import sqlite3
import os

# Get the directory where this file is located
DB_DIR = os.path.dirname(os.path.abspath(__file__))
DB_PATH = os.path.join(DB_DIR, "data.db")

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
