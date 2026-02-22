import psycopg2
import psycopg2.extras
from datetime import datetime, timedelta
from config import (
    BACKEND_TYPE, POSTGRES_HOST, POSTGRES_PORT, POSTGRES_DB,
    POSTGRES_USER, POSTGRES_PASSWORD, SUPABASE_URL, SUPABASE_KEY
)

# Conditional import for Supabase
if BACKEND_TYPE == "supabase":
    try:
        from supabase import create_client, Client
        supabase_client: Client = create_client(SUPABASE_URL, SUPABASE_KEY)
    except ImportError:
        print("Warning: supabase-py not installed. Install with: pip install supabase")
        print("Falling back to PostgreSQL mode")
        BACKEND_TYPE = "postgresql"

def get_db():
    """Get database connection based on backend type"""
    if BACKEND_TYPE == "supabase":
        # Return None for Supabase - we use the client directly
        return None
    else:
        # PostgreSQL (Docker local or direct connection)
        conn = psycopg2.connect(
            host=POSTGRES_HOST,
            port=POSTGRES_PORT,
            database=POSTGRES_DB,
            user=POSTGRES_USER,
            password=POSTGRES_PASSWORD
        )
        return conn

def register_device(device_id, device_name, device_type):
    """Register or update a device"""
    if BACKEND_TYPE == "supabase":
        try:
            data = {
                "id": device_id,
                "device_name": device_name,
                "device_type": device_type,
                "first_seen": datetime.now().isoformat(),
                "last_seen": datetime.now().isoformat()
            }
            supabase_client.table("devices").upsert(data).execute()
            return device_id
        except Exception as e:
            print(f"Supabase error: {e}")
            return device_id
    else:
        db = get_db()
        cursor = db.cursor()
        
        cursor.execute("""
            INSERT INTO devices (id, device_name, device_type, first_seen, last_seen)
            VALUES (%s, %s, %s, %s, %s)
            ON CONFLICT (id) DO UPDATE SET
                device_name = EXCLUDED.device_name,
                device_type = EXCLUDED.device_type,
                last_seen = EXCLUDED.last_seen
        """, (device_id, device_name, device_type, datetime.now(), datetime.now()))
        
        db.commit()
        cursor.close()
        db.close()
        return device_id

def get_device(device_id):
    """Get device information"""
    if BACKEND_TYPE == "supabase":
        try:
            result = supabase_client.table("devices").select("*").eq("id", device_id).execute()
            return result.data[0] if result.data else None
        except Exception as e:
            print(f"Supabase error: {e}")
            return None
    else:
        db = get_db()
        cursor = db.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
        
        cursor.execute("SELECT * FROM devices WHERE id = %s", (device_id,))
        device = cursor.fetchone()
        
        cursor.close()
        db.close()
        return dict(device) if device else None

def get_all_devices():
    """Get all registered devices"""
    if BACKEND_TYPE == "supabase":
        try:
            result = supabase_client.table("devices").select("*").order("last_seen", desc=True).execute()
            return result.data
        except Exception as e:
            print(f"Supabase error: {e}")
            return []
    else:
        db = get_db()
        cursor = db.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
        
        cursor.execute("SELECT * FROM devices ORDER BY last_seen DESC")
        devices = cursor.fetchall()
        
        cursor.close()
        db.close()
        return [dict(d) for d in devices]

def insert_sample(device_id, timestamp, active_window):
    """Insert a sample and update device last_seen"""
    if BACKEND_TYPE == "supabase":
        try:
            # Insert sample
            sample_data = {
                "device_id": device_id,
                "timestamp": timestamp,
                "active_window": active_window
            }
            supabase_client.table("samples").insert(sample_data).execute()
            
            # Update device last_seen
            supabase_client.table("devices").update({"last_seen": datetime.now().isoformat()}).eq("id", device_id).execute()
        except Exception as e:
            print(f"Supabase error: {e}")
    else:
        db = get_db()
        cursor = db.cursor()
        
        # Insert sample
        cursor.execute(
            "INSERT INTO samples (device_id, timestamp, active_window) VALUES (%s, %s, %s)",
            (device_id, timestamp, active_window)
        )
        
        # Update device last_seen
        cursor.execute(
            "UPDATE devices SET last_seen = %s WHERE id = %s",
            (datetime.now(), device_id)
        )
        
        db.commit()
        cursor.close()
        db.close()

def get_samples(device_id=None, limit=10):
    """Get recent samples, optionally filtered by device"""
    if BACKEND_TYPE == "supabase":
        try:
            query = supabase_client.table("samples").select("*")
            if device_id:
                query = query.eq("device_id", device_id)
            result = query.order("id", desc=True).limit(limit).execute()
            return result.data
        except Exception as e:
            print(f"Supabase error: {e}")
            return []
    else:
        db = get_db()
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
        db.close()
        
        return [dict(row) for row in rows]

def get_dashboard_data(device_id=None):
    """Get aggregated data for dashboard"""
    day_start = datetime.now().replace(hour=0, minute=0, second=0, microsecond=0)
    day_end = day_start + timedelta(days=1)

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
            print(f"Supabase error: {e}")
            return []
    else:
        db = get_db()
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
        db.close()
        
        return [dict(row) for row in rows]
