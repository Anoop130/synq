import psycopg2
import psycopg2.pool
import psycopg2.extras
from contextlib import contextmanager
from datetime import datetime, timedelta
from config import (
    BACKEND_TYPE, POSTGRES_HOST, POSTGRES_PORT, POSTGRES_DB,
    POSTGRES_USER, POSTGRES_PASSWORD, SUPABASE_URL, SUPABASE_KEY
)

if BACKEND_TYPE == "supabase":
    try:
        from supabase import create_client, Client
        supabase_client: Client = create_client(SUPABASE_URL, SUPABASE_KEY)
    except ImportError:
        print("[WARN] supabase-py not installed, falling back to postgresql mode")
        BACKEND_TYPE = "postgresql"

_pg_pool = None

def _get_pool():
    """
    returns the module level PostgreSQL connection pool, creating it on first call.

    inputs:
        none

    returns:
        psycopg2.pool.SimpleConnectionPool with 1 to 10 connections
    """
    global _pg_pool
    if _pg_pool is None:
        _pg_pool = psycopg2.pool.SimpleConnectionPool(
            1, 10,
            host=POSTGRES_HOST,
            port=POSTGRES_PORT,
            database=POSTGRES_DB,
            user=POSTGRES_USER,
            password=POSTGRES_PASSWORD
        )
    return _pg_pool

@contextmanager
def get_db():
    """
    context manager that lends a PostgreSQL connection from the pool.
    commits on clean exit, rolls back on exception, and always returns the connection.

    inputs:
        none

    returns:
        psycopg2 connection object scoped to the with block
    """
    conn = _get_pool().getconn()
    try:
        yield conn
        conn.commit()
    except Exception:
        conn.rollback()
        raise
    finally:
        _get_pool().putconn(conn)

def register_device(device_id, device_name, device_type):
    """
    inserts a new device or updates its mutable fields if it already exists.
    first_seen is set on insert only and never overwritten on conflict.

    inputs:
        device_id   (str) UUID v4 format, max 36 chars
        device_name (str) human readable label, max 255 chars
        device_type (str) platform identifier such as linux, max 50 chars

    returns:
        str: device_id echoed back
    """
    if BACKEND_TYPE == "supabase":
        try:
            now = datetime.now().isoformat()
            try:
                supabase_client.table("devices").insert({
                    "id": device_id,
                    "device_name": device_name,
                    "device_type": device_type,
                    "first_seen": now,
                    "last_seen": now
                }).execute()
            except Exception:
                supabase_client.table("devices").update({
                    "device_name": device_name,
                    "device_type": device_type,
                    "last_seen": now
                }).eq("id", device_id).execute()
            return device_id
        except Exception as e:
            print(f"[ERROR] supabase register_device: {e}")
            return device_id
    else:
        with get_db() as db:
            cursor = db.cursor()
            cursor.execute("""
                INSERT INTO devices (id, device_name, device_type, first_seen, last_seen)
                VALUES (%s, %s, %s, %s, %s)
                ON CONFLICT (id) DO UPDATE SET
                    device_name = EXCLUDED.device_name,
                    device_type = EXCLUDED.device_type,
                    last_seen   = EXCLUDED.last_seen
            """, (device_id, device_name, device_type, datetime.now(), datetime.now()))
            cursor.close()
        return device_id

def get_device(device_id):
    """
    fetches a single device record by its UUID.

    inputs:
        device_id (str) UUID v4 format, max 36 chars

    returns:
        dict with device fields, or None if no matching record exists
    """
    if BACKEND_TYPE == "supabase":
        try:
            result = supabase_client.table("devices").select("*").eq("id", device_id).execute()
            return result.data[0] if result.data else None
        except Exception as e:
            print(f"[ERROR] supabase get_device: {e}")
            return None
    else:
        with get_db() as db:
            cursor = db.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
            cursor.execute("SELECT * FROM devices WHERE id = %s", (device_id,))
            device = cursor.fetchone()
            cursor.close()
        return dict(device) if device else None

def get_all_devices():
    """
    fetches all registered devices ordered by most recently seen first.

    inputs:
        none

    returns:
        list of dicts, each representing one device row; empty list on error
    """
    if BACKEND_TYPE == "supabase":
        try:
            result = supabase_client.table("devices").select("*").order("last_seen", desc=True).execute()
            return result.data
        except Exception as e:
            print(f"[ERROR] supabase get_all_devices: {e}")
            return []
    else:
        with get_db() as db:
            cursor = db.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
            cursor.execute("SELECT * FROM devices ORDER BY last_seen DESC")
            devices = cursor.fetchall()
            cursor.close()
        return [dict(d) for d in devices]

def insert_sample(device_id, timestamp, active_window):
    """
    records one activity sample and updates the device last_seen timestamp.

    inputs:
        device_id     (str) UUID v4 format, max 36 chars, must already exist in devices
        timestamp     (str) ISO 8601 datetime string e.g. 2026-05-24 12:00:00
        active_window (str) window title, unbounded length

    returns:
        None
    """
    if BACKEND_TYPE == "supabase":
        try:
            supabase_client.table("samples").insert({
                "device_id": device_id,
                "timestamp": timestamp,
                "active_window": active_window
            }).execute()
            supabase_client.table("devices").update({
                "last_seen": datetime.now().isoformat()
            }).eq("id", device_id).execute()
        except Exception as e:
            print(f"[ERROR] supabase insert_sample: {e}")
    else:
        with get_db() as db:
            cursor = db.cursor()
            cursor.execute(
                "INSERT INTO samples (device_id, timestamp, active_window) VALUES (%s, %s, %s)",
                (device_id, timestamp, active_window)
            )
            cursor.execute(
                "UPDATE devices SET last_seen = %s WHERE id = %s",
                (datetime.now(), device_id)
            )
            cursor.close()

def get_samples(device_id=None, limit=10):
    """
    fetches recent activity samples in reverse insertion order.

    inputs:
        device_id (str or None) UUID v4, max 36 chars; pass None to fetch across all devices
        limit     (int) max number of rows to return, default 10, min 1

    returns:
        list of dicts, each representing one sample row; empty list on error
    """
    if BACKEND_TYPE == "supabase":
        try:
            query = supabase_client.table("samples").select("*")
            if device_id:
                query = query.eq("device_id", device_id)
            result = query.order("id", desc=True).limit(limit).execute()
            return result.data
        except Exception as e:
            print(f"[ERROR] supabase get_samples: {e}")
            return []
    else:
        with get_db() as db:
            cursor = db.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
            if device_id:
                cursor.execute(
                    "SELECT * FROM samples WHERE device_id = %s ORDER BY id DESC LIMIT %s",
                    (device_id, limit)
                )
            else:
                cursor.execute("SELECT * FROM samples ORDER BY id DESC LIMIT %s", (limit,))
            rows = cursor.fetchall()
            cursor.close()
        return [dict(row) for row in rows]

def get_dashboard_data(device_id=None):
    """
    aggregates sample counts grouped by window title for today's 24 hour window.

    inputs:
        device_id (str or None) UUID v4, max 36 chars; pass None to aggregate all devices

    returns:
        list of dicts with keys active_window (str) and samples (int),
        sorted by samples descending; empty list on error
    """
    day_start = datetime.now().replace(hour=0, minute=0, second=0, microsecond=0)
    day_end   = day_start + timedelta(days=1)

    if BACKEND_TYPE == "supabase":
        try:
            batch_size = 1000
            offset = 0
            all_rows = []

            while True:
                query = (
                    supabase_client.table("samples")
                    .select("active_window")
                    .gte("timestamp", day_start.isoformat())
                    .lt("timestamp", day_end.isoformat())
                    .order("id", desc=False)
                    .range(offset, offset + batch_size - 1)
                )
                if device_id:
                    query = query.eq("device_id", device_id)

                result = query.execute()
                batch = result.data or []
                if not batch:
                    break

                all_rows.extend(batch)
                if len(batch) < batch_size:
                    break

                offset += batch_size

            from collections import Counter
            counts = Counter(row.get("active_window") for row in all_rows)
            return [{"active_window": window, "samples": count}
                    for window, count in counts.most_common()]
        except Exception as e:
            print(f"[ERROR] supabase get_dashboard_data: {e}")
            return []
    else:
        with get_db() as db:
            cursor = db.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
            if device_id:
                cursor.execute("""
                    SELECT active_window, COUNT(*) as samples
                    FROM samples
                    WHERE device_id = %s
                      AND timestamp >= %s
                      AND timestamp < %s
                    GROUP BY active_window
                    ORDER BY samples DESC
                """, (device_id, day_start, day_end))
            else:
                cursor.execute("""
                    SELECT active_window, COUNT(*) as samples
                    FROM samples
                    WHERE timestamp >= %s
                      AND timestamp < %s
                    GROUP BY active_window
                    ORDER BY samples DESC
                """, (day_start, day_end))
            rows = cursor.fetchall()
            cursor.close()
        return [dict(row) for row in rows]
