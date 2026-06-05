# Synq

Lightweight activity tracking for Ubuntu/GNOME desktops. Tracks the active window in real time and shows a daily time breakdown on a local dashboard.

## How It Works

```
GNOME Shell extension
  (active window focus)
        |
        | DBus signal: WindowChanged(title, timestamp)
        v
  Python bridge collector
  (collectors/synq-gnome/collector.py)
        |
        | POST /collect  (duration measured between focus changes)
        v
    Flask server
    (server/app.py)
        |
        v
    PostgreSQL
    (local Docker  or  Supabase cloud)
        |
        v
  Dashboard: http://localhost:5001/dashboard
```

The GNOME extension fires a `WindowChanged` signal on every focus change (event-driven, not polled). The Python bridge buffers the previous window, calculates how long it was active, and POSTs the completed session to the server. The server parses window titles into `app_name` and `activity` fields, stores them in PostgreSQL, and serves an aggregated dashboard.

## Components

| Path | What it is |
|---|---|
| `collectors/gnome/` | GNOME Shell extension (git submodule). Tracks active window, exposes DBus API `io.github.Synq.GnomeExtension` |
| `collectors/synq-gnome/collector.py` | Python bridge. Subscribes to the DBus signal, calculates durations, POSTs to server |
| `server/app.py` | Flask API. Five routes: `/register`, `/collect`, `/devices`, `/view`, `/dashboard` |
| `server/database.py` | Dual-backend DB layer: PostgreSQL (local/Docker) or Supabase (cloud) |
| `server/schema_postgres.sql` | PostgreSQL schema. Auto-applied by Docker on first run |
| `server/migrate_v2.sql` | Migration for existing databases -- adds `app_name`, `activity`, `duration_seconds` columns |

## Quick Start (local, Ubuntu)

### Prerequisites

- Ubuntu with GNOME Shell (Wayland or X11)
- Docker and docker-compose
- Git

### Install

```bash
git clone --recurse-submodules https://github.com/Anoop130/synq.git
cd synq
./install.sh
```

`install.sh` does the following in order:

1. Initializes the `collectors/gnome` git submodule
2. Starts the Flask server + PostgreSQL via Docker Compose
3. Copies the GNOME extension to `~/.local/share/gnome-shell/extensions/`
4. Installs Python bridge dependencies (`python3-gi`, `dbus-python`, `requests`)
5. Enables the extension via `gnome-extensions enable synq-gnome@rsim`
6. On X11: reloads GNOME Shell in place. On Wayland: prompts you to log out and back in.
7. Prints the command to start the collector bridge.

### After install

On Wayland, log out and back in first so GNOME Shell loads the extension.

Then start the collector:

```bash
export SYNQ_ENDPOINT=http://localhost:5001
python3 collectors/synq-gnome/collector.py
```

Open the dashboard: http://localhost:5001/dashboard

## Deployment Options

### Option 1: Local Docker (default, recommended)

The `./install.sh` flow above. Everything runs on your machine. Dashboard at `http://localhost:5001/dashboard`.

Server management:

```bash
docker-compose logs -f      # stream logs
docker-compose down         # stop
docker-compose restart      # restart
docker-compose up -d        # start (after a stop)
```

### Option 2: Cloud (Render + Supabase)

Host the server on Render free tier with Supabase as the database. Useful for tracking across multiple machines or accessing the dashboard remotely.

**Supabase setup:**

1. Create a project at https://supabase.com
2. Open the SQL Editor and run the contents of `server/schema_postgres.sql`
3. Copy the Project URL and anon key from Settings --> API

**Render setup:**

1. Push this repo to GitHub
2. Create a new Web Service on Render pointing to the repo
3. Set these environment variables in Render:

```
BACKEND_TYPE=supabase
SUPABASE_URL=https://<your-project>.supabase.co
SUPABASE_KEY=<anon-key>
```

4. Render picks up the `Procfile` and runs the server automatically.

**Collector pointing to cloud:**

```bash
export SYNQ_ENDPOINT=https://<your-render-app>.onrender.com
python3 collectors/synq-gnome/collector.py
```

## Migrating an Existing Database

If you have an existing PostgreSQL database from before the `app_name`/`activity`/`duration_seconds` columns were added, apply the migration:

```bash
# Docker
docker exec -i $(docker-compose ps -q postgres) \
    psql -U synq_user -d synq < server/migrate_v2.sql

# Manual PostgreSQL
psql -U synq_user -d synq -f server/migrate_v2.sql
```

## Project Structure

```
synq/
|-- collectors/
|   |-- gnome/                      # git submodule: GNOME Shell extension
|   |   `-- synq-gnome@rsim/        # extension directory installed to ~/.local/share/gnome-shell/extensions/
|   `-- synq-gnome/                 # Python bridge collector
|       |-- collector.py            # DBus subscriber, duration calculation, HTTP POST
|       |-- install.sh              # installs system and Python dependencies
|       `-- requirements.txt        # requests, PyGObject
|-- server/
|   |-- app.py                      # Flask routes
|   |-- database.py                 # PostgreSQL and Supabase backends
|   |-- config.py                   # environment variable configuration
|   |-- schema_postgres.sql         # base schema (auto-applied by Docker)
|   |-- migrate_v2.sql              # migration for existing databases
|   `-- .env.example                # environment variable template
|-- archive/
|   `-- collectors/laptop/          # archived C++ X11 collector (replaced)
|-- docker-compose.yml              # PostgreSQL + Flask server
|-- Dockerfile                      # Flask server container
|-- Procfile                        # Render deployment entry point
|-- runtime.txt                     # Python version pin for Render
|-- install.sh                      # full onboarding script
`-- requirements.txt                # Python dependencies for the server
```

## Configuration

Server reads from `server/.env` (copy from `server/.env.example`):

```env
# local Docker (default)
BACKEND_TYPE=postgresql
POSTGRES_HOST=localhost
POSTGRES_PORT=5432
POSTGRES_DB=synq
POSTGRES_USER=synq_user
POSTGRES_PASSWORD=synq_password

# Supabase cloud
BACKEND_TYPE=supabase
SUPABASE_URL=https://project.supabase.co
SUPABASE_KEY=anon_key
```

Collector reads one environment variable:

```bash
export SYNQ_ENDPOINT=http://localhost:5001   # or your Render URL
```

Device UUID is stored automatically at `~/.config/synq/device.conf`.

## API Reference

### POST /register

Register a device. Returns a UUID that the collector stores locally.

```json
{ "device_name": "my-laptop", "device_type": "linux" }
```

Response: `{"status": "ok", "device_id": "<uuid>"}`

### POST /collect

Submit one activity sample.

```json
{
  "device_id": "<uuid>",
  "timestamp": "2026-06-05 14:30:00",
  "active_window": "main.py - synq - Visual Studio Code",
  "duration_seconds": 42
}
```

Response: `{"status": "ok", "message": "sample recorded"}`

`duration_seconds` is optional. Legacy rows without it fall back to a 5-second estimate in dashboard aggregation.

### GET /devices

Returns all registered devices ordered by `last_seen` descending.

### GET /view

Returns recent samples. Optional query params: `device_id`, `limit` (default 10).

### GET /dashboard

Renders the HTML dashboard. Optional query param: `device_id` to filter to one device.

## Development

### Run server locally (no Docker)

```bash
# create and activate virtualenv
python3 -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt

# configure
cp server/.env.example server/.env
# edit server/.env with your PostgreSQL credentials

# run
cd server && python3 app.py
```

### Database operations

```bash
# connect to Docker PostgreSQL
docker exec -it $(docker-compose ps -q postgres) psql -U synq_user -d synq

# view recent samples
SELECT app_name, activity, duration_seconds FROM samples ORDER BY id DESC LIMIT 20;

# today's time per app
SELECT app_name, SUM(COALESCE(duration_seconds, 5)) AS seconds
FROM samples
WHERE timestamp >= CURRENT_DATE
GROUP BY app_name
ORDER BY seconds DESC;

# delete data older than 90 days
DELETE FROM samples WHERE created_at < NOW() - INTERVAL '90 days';
```

### Test endpoints manually

```bash
# register a device
curl -s -X POST http://localhost:5001/register \
  -H "Content-Type: application/json" \
  -d '{"device_name": "test", "device_type": "linux"}' | python3 -m json.tool

# submit a sample
curl -s -X POST http://localhost:5001/collect \
  -H "Content-Type: application/json" \
  -d '{"device_id": "<uuid>", "timestamp": "2026-06-05 12:00:00",
       "active_window": "test - Visual Studio Code", "duration_seconds": 10}' \
  | python3 -m json.tool
```

## Troubleshooting

**Extension not receiving events after install:**
On Wayland, a full log-out and log-in is required after installing or updating the GNOME extension. Disable/enable alone is not sufficient on Wayland.

**Collector exits immediately with DBus error:**
The GNOME extension must be enabled and GNOME Shell must have loaded it. Verify in GNOME Extensions that `synq-gnome@rsim` shows as ON.

**Server returns 404 on /collect:**
The device is not registered. The collector registers on startup -- check collector logs for registration errors. If the server was unreachable at startup, restart the collector.

**Docker PostgreSQL port conflict:**
Another PostgreSQL instance may be using port 5432. Either stop it or change the host port in `docker-compose.yml`.

**Dashboard shows no data:**
Confirm the collector is running and the `SYNQ_ENDPOINT` points to the correct server. Check `docker-compose logs -f` for incoming POST requests.

## License

MIT License. Copyright (c) 2026 Anoop130.
