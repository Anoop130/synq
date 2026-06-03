# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Is

Synq is a lightweight activity tracking system. On Ubuntu/Linux desktops, the synq-gnome GNOME Shell extension tracks the active window and exposes a DBus API. A Python bridge collector subscribes to that API and POSTs samples to a Flask server. The server stores data in PostgreSQL (locally or via Supabase) and serves a dashboard.

## Coding Standards

These rules apply to all code, comments, and assistant responses in this repo. They are non-negotiable.

### Formatting
- NEVER use em dashes or hyphens in comments.
- NO long blocks of repeating characters (e.g. `//////` or `******`).
- Use ONLY standard ASCII characters. If an arrow is needed, use `-->`.
- No emojis anywhere (not in chat, not in code, not in comments).

### Interaction Protocol
- Do NOT blindly agree.
- No glazing, flattery, or sugarcoating.
- ALWAYS challenge ideas. If an approach is wrong, inefficient, or flawed, say so bluntly.
- Immediately follow any critique with a concrete, superior alternative.
- System and status messages (responses from the assistant) must begin with a single bracketed uppercase word followed by a space: `[REPORT]`, `[INFO]`, `[ERROR]`, `[WARN]`.

### Character and Language Constraints
- Use ONLY standard ASCII characters (letters, numbers, standard punctuation).
- NO special characters or unicode symbols. If an arrow is required, use `-->`.
- NO pronouns anywhere in code or comments (do not use I, you, your, we, our, they).

### Comments and Documentation
- All comments must start with a lowercase letter (unless the word is an acronym).
- Function documentation placement must match the language:
  - C++ and Rust: outside and immediately preceding the function signature.
  - Python: inside the function as a docstring.
- Every documented function block must contain:
  1. A concise description of the functionality.
  2. Specific inputs required, explicitly detailing data types and size/bounds.
  3. Exact outputs returned, explicitly detailing data types and size/bounds.

### Logging
- All log lines must start with a one-word bracketed tag followed by a space: `[INFO]`, `[ERROR]`, `[DEBUG]`, `[WARN]`.
- Write all highly detailed and verbose logs to a designated log file.
- Default console output: critical or actionable messages only.
- Gate verbose console output behind a flag or env var (e.g. `SYNQ_VERBOSE=1`).

### Version Control
- ALWAYS use Conventional Commits for commit messages (e.g. `feat(auth): add jwt validation`).
- ALWAYS use prefix-based branch names with underscores (e.g. `feat/user_authentication`).
- NEVER commit changes or create branches autonomously.
- Wait for explicit user authorization before executing any git command that modifies history or branch structure.

## Commands

### Server

```bash
# install dependencies
pip install -r requirements.txt

# run development server (from server/)
cd server && python3 app.py
# listens on http://localhost:5001
```

### Collector (GNOME extension + Python bridge)

```bash
# step 1: install the GNOME Shell extension (collectors/gnome submodule)
# enable it in GNOME Extensions or via gnome-extensions CLI:
gnome-extensions enable synq-gnome@rsim

# step 2: install bridge collector dependencies (from repo root)
bash collectors/synq-gnome/install.sh

# step 3: run the bridge collector
export SYNQ_ENDPOINT=http://localhost:5001
python3 collectors/synq-gnome/collector.py
```

### Docker

```bash
# quick start (checks Docker, starts all services)
./install.sh

# manual
docker-compose up -d    # start all services
docker-compose logs -f  # watch logs
docker-compose down     # stop
```

### Diagnostics

```bash
# profile server and collector resource usage
./benchmark.sh
```

## Architecture

Two independently deployed components communicate over HTTP:

**Collector (Ubuntu/Linux desktop)**:
- `collectors/gnome/` (git submodule, `git@github.com:Anoop130/synq-gnome.git`): GNOME Shell extension that tracks active window focus in real time, stores events locally in `~/.local/share/synq-gnome/events.jsonl`, and exposes a DBus API (`io.github.Synq.GnomeExtension`) with a `WindowChanged(title, timestamp)` signal.
- `collectors/synq-gnome/collector.py`: Python bridge that subscribes to the DBus signal and forwards each window-change event to the server via `POST /collect`. Reads `SYNQ_ENDPOINT` from the environment (defaults to `http://127.0.0.1:5001`). Manages device UUID at `~/.config/synq/device.conf`.

**Server (`server/`)**: Flask app:
- `app.py`: five routes: `POST /register`, `POST /collect`, `GET /devices`, `GET /view`, `GET /dashboard`
- `database.py`: dual backend abstraction; all DB calls branch on `BACKEND_TYPE` (`postgresql` or `supabase`). PostgreSQL uses a `SimpleConnectionPool` (1 to 10 connections); Supabase uses the supabase-py client. `parse_window_title()` splits raw window titles on ` - ` into `app_name` and `activity` fields.
- `config.py`: reads all configuration from environment variables; PostgreSQL settings have defaults matching docker-compose; Supabase credentials default to empty strings
- Dashboard is rendered as an inline HTML string in `app.py` (no template files)

**Database schema** (`server/schema_postgres.sql`):
- `devices(id UUID PK, device_name, device_type, first_seen, last_seen)`
- `samples(id BIGSERIAL PK, device_id FK-->devices, timestamp, active_window, app_name, activity, created_at)`

**Archived collector**: `archive/collectors/laptop/` -- original C++ X11 polling collector; see `archive/collectors/laptop/ARCHIVE.md` for the reason it was replaced.

## Configuration

Server reads from `server/.env` (copy from `server/.env.example`):

```env
BACKEND_TYPE=postgresql          # or supabase
POSTGRES_HOST=localhost
POSTGRES_PORT=5432
POSTGRES_DB=synq
POSTGRES_USER=synq_user
POSTGRES_PASSWORD=synq_password

# for Supabase backend:
SUPABASE_URL=https://project.supabase.co
SUPABASE_KEY=anon_key
```

Collector reads `SYNQ_ENDPOINT` from the environment.

Cloud deployment uses Render: `Procfile` starts the server, `runtime.txt` pins the Python version.

## Key Behaviors

- Device registration is an upsert: re-registering the same UUID updates `last_seen` only; `first_seen` is never overwritten (both PostgreSQL and Supabase paths)
- `get_dashboard_data` filters to today's 24-hour window only (midnight to midnight)
- For Supabase, `get_dashboard_data` paginates in 1000-row batches and aggregates in Python (no SQL `GROUP BY`)
- The bridge collector continues running if registration fails, using the locally cached UUID
- Port 5001 is the default; overridden by the `PORT` environment variable (for Render)
- The GNOME extension fires `WindowChanged` on focus change (event-driven), not on a fixed interval
- Window titles are split on ` - ` into `app_name` and `activity` fields at insert time
- The GNOME extension emits `__IDLE__` as a sentinel title on suspend/lock; the bridge collector ignores it
