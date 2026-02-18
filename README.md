# Synq

A lightweight, privacy-focused activity tracking system for monitoring application usage across multiple devices.

## Overview

Synq tracks active applications and window titles across devices, providing a unified dashboard for analyzing time allocation and productivity patterns. The system consists of native collectors that capture window activity and a central server that aggregates and visualizes the data.

## Features

- **Multi-Device Support** - Track activity across unlimited devices with automatic device registration
- **Privacy-First Architecture** - Self-hosted or cloud deployment options with complete data control
- **Minimal Resource Usage** - Less than 0.5% CPU utilization and 5MB memory footprint per collector
- **Flexible Deployment** - Docker containers, PostgreSQL, or Supabase cloud backend
- **Real-Time Dashboard** - Web interface with per-device filtering and visual analytics
- **Open Source** - MIT licensed with full source code transparency

## Architecture

```
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│  Collector   │  │  Collector   │  │  Collector   │
│  (Device 1)  │  │  (Device 2)  │  │  (Device N)  │
└──────┬───────┘  └──────┬───────┘  └──────┬───────┘
       │                 │                 │
       └─────────────────┼─────────────────┘
                         ▼
                ┌─────────────────┐
                │  Flask Server   │
                │  REST API       │
                └────────┬────────┘
                         │
              ┌──────────┴──────────┐
              ▼                     ▼
      ┌───────────────┐     ┌──────────────┐
      │  PostgreSQL   │     │  Supabase    │
      │  (Local/Docker)│     │  (Cloud)     │
      └───────────────┘     └──────────────┘
```

## Quick Start

### Docker Deployment (Recommended)

```bash
git clone https://github.com/Anoop130/synq.git
cd synq
./install.sh
```

Access the dashboard at `http://localhost:5001/dashboard`

### Manual Installation

**Prerequisites:**
- Python 3.8+
- PostgreSQL 12+
- GCC (for collector compilation)

**Server Setup:**

```bash
# Install Python dependencies
pip install -r requirements.txt

# Initialize PostgreSQL database
PGPASSWORD=synq_password psql -U synq_user -h localhost -d synq -f server/schema_postgres.sql

# Start server
cd server
python3 app.py
```

**Collector Setup (Linux/X11):**

```bash
cd collectors/laptop
make
export SYNQ_ENDPOINT=http://localhost:5001
../../build/binaries/collector
```

## Configuration

### Server Configuration

Environment variables can be set in `server/.env`:

```env
# Backend type: postgresql or supabase
BACKEND_TYPE=postgresql

# PostgreSQL configuration
POSTGRES_HOST=localhost
POSTGRES_PORT=5432
POSTGRES_DB=synq
POSTGRES_USER=synq_user
POSTGRES_PASSWORD=synq_password

# Supabase configuration (alternative to PostgreSQL)
# BACKEND_TYPE=supabase
# SUPABASE_URL=https://project.supabase.co
# SUPABASE_KEY=anon_key
```

### Collector Configuration

The collector endpoint is configured via environment variable:

```bash
export SYNQ_ENDPOINT=http://server-address:5001
```

Device registration occurs automatically on first run, with the device ID stored in `~/.config/synq/device.conf`.

## API Reference

### Register Device

```http
POST /register
Content-Type: application/json

{
  "device_name": "workstation-01",
  "device_type": "linux"
}

Response: {"status": "ok", "device_id": "uuid"}
```

### Submit Activity Sample

```http
POST /collect
Content-Type: application/json

{
  "device_id": "uuid",
  "timestamp": "2026-02-17 14:30:00",
  "active_window": "application - window title"
}

Response: {"status": "ok", "message": "sample recorded"}
```

### List Registered Devices

```http
GET /devices

Response: [
  {
    "id": "uuid",
    "device_name": "workstation-01",
    "device_type": "linux",
    "first_seen": "2026-02-17T10:00:00",
    "last_seen": "2026-02-17T14:30:00"
  }
]
```

### View Activity Samples

```http
GET /view?device_id=uuid&limit=100

Response: [
  {
    "id": 1,
    "device_id": "uuid",
    "timestamp": "2026-02-17T14:30:00",
    "active_window": "application - window title",
    "created_at": "2026-02-17T14:30:00"
  }
]
```

### Dashboard

```http
GET /dashboard
GET /dashboard?device_id=uuid
```

Web interface displaying activity statistics with device filtering capabilities.

## Storage Requirements

**Per device with 5-second sampling interval:**

- 17,280 samples per day (~1.7 MB)
- 51 MB per month
- 612 MB per year

**Multi-device calculation:**
- 4 devices: ~200 MB per month, 2.4 GB per year

**Data retention strategy:**

Implement periodic cleanup to maintain storage limits:

```sql
DELETE FROM samples WHERE created_at < NOW() - INTERVAL '90 days';
```

## Performance Metrics

- **CPU Usage:** < 0.5% (collector)
- **Memory Usage:** 5-10 MB (collector)
- **Network Traffic:** ~500 bytes per sample
- **Database Performance:** Optimized for millions of samples with indexed queries

## Deployment Options

### Option 1: Docker with PostgreSQL

Self-contained deployment with Docker Compose includes:
- PostgreSQL 16 database
- Flask application server
- Persistent data volumes
- Automatic health checks

### Option 2: Supabase Cloud

Serverless PostgreSQL backend with:
- 500 MB free tier storage
- REST API auto-generation
- Built-in authentication (optional)
- Managed backups and scaling

### Option 3: Custom Cloud Deployment

Deploy to any infrastructure supporting:
- Python 3.8+ runtime
- PostgreSQL 12+ database
- Persistent storage for application state

## Development

### Building Collector

```bash
cd collectors/laptop
make clean
make
```

### Running Tests

```bash
# Test device registration
curl -X POST http://localhost:5001/register \
  -H "Content-Type: application/json" \
  -d '{"device_name": "test-device", "device_type": "linux"}'

# Test sample submission
curl -X POST http://localhost:5001/collect \
  -H "Content-Type: application/json" \
  -d '{"device_id": "device-uuid", "timestamp": "2026-02-17 12:00:00", "active_window": "test"}'
```

### Project Structure

```
synq/
├── collectors/
│   └── laptop/           # C++ collector for Linux/X11
│       ├── collector.cpp
│       ├── config_utils.cpp
│       ├── net_utils.cpp
│       ├── x11_utils.cpp
│       └── Makefile
├── server/
│   ├── app.py           # Flask application
│   ├── database.py      # Database abstraction layer
│   ├── config.py        # Configuration management
│   └── schema_postgres.sql
├── docker-compose.yml   # Docker deployment configuration
├── Dockerfile          # Server container definition
└── install.sh          # Automated installation script
```

## Platform Support

### Current Support

- **Linux** - X11 window manager (collector implemented)
- **Server** - Any platform supporting Python 3.8+ and PostgreSQL

### Planned Support

- Android collector application
- iOS collector application
- Wayland window manager support
- macOS collector (Accessibility API)
- Windows collector (Win32 API)

## Security Considerations

- Window titles may contain sensitive information
- Device IDs are randomly generated UUIDs with no personal identifiers
- Local storage uses unencrypted SQLite for device configuration
- Network communication uses HTTP (HTTPS recommended for production)
- Database access requires authentication credentials
- No telemetry or external data transmission beyond configured endpoints

## Troubleshooting

**Collector compilation errors:**
```bash
# Install required development libraries
sudo apt-get install build-essential libcurl4-openssl-dev libx11-dev
```

**Database connection failures:**
```bash
# Verify PostgreSQL is running
sudo systemctl status postgresql

# Check credentials match configuration
psql -U synq_user -h localhost -d synq
```

**Port conflicts:**
```bash
# Identify process using port 5001
sudo netstat -tlnp | grep 5001

# Modify server port in app.py or docker-compose.yml
```

## Contributing

Contributions are welcome. Please follow these guidelines:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/enhancement`)
3. Implement changes with appropriate tests
4. Ensure code follows existing style conventions
5. Submit pull request with detailed description

## License

MIT License - See LICENSE file for full text

## Technical Stack

- **Backend:** Flask 3.0 (Python)
- **Database:** PostgreSQL 16 / Supabase
- **Collector:** C++11, libcurl, X11
- **Deployment:** Docker, Docker Compose
- **Frontend:** HTML5, CSS3, Vanilla JavaScript

## Support

- **Issues:** https://github.com/Anoop130/synq/issues
- **Documentation:** https://github.com/Anoop130/synq/wiki
- **Discussions:** https://github.com/Anoop130/synq/discussions
