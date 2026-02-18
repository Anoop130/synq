# Synq

A lightweight, privacy-focused activity tracking system for monitoring application usage across multiple devices.

## Overview

Synq tracks active applications and window titles across devices, providing a unified dashboard for analyzing time allocation and productivity patterns. The system consists of native collectors that capture window activity and a central server that aggregates and visualizes the data through PostgreSQL and Supabase infrastructure.

## Features

- **Multi-Device Support** - Unlimited device tracking with automatic registration
- **Privacy-First Architecture** - Self-hosted or cloud deployment with complete data control
- **Minimal Resource Usage** - Less than 0.5% CPU and 5MB memory footprint per collector
- **Cloud-Native Backend** - PostgreSQL with Supabase and Render free tier deployment
- **Real-Time Dashboard** - Web interface with device filtering and visual analytics
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
              ┌─────────────────────┐
              │   Render (Flask)    │
              │   Free Tier         │
              └──────────┬──────────┘
                         ▼
              ┌─────────────────────┐
              │  Supabase (Postgres)│
              │   Free Tier         │
              └─────────────────────┘
```

---

## For End Users

### Installation

#### Linux (Ubuntu/Debian)

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y build-essential libcurl4-openssl-dev libx11-dev git

# Clone repository
git clone https://github.com/Anoop130/synq.git
cd synq/collectors/laptop

# Compile collector
make

# Configure cloud endpoint
export SYNQ_ENDPOINT=https://synq-ypow.onrender.com

# Run collector
../../build/binaries/collector
```

The collector will automatically register the device and begin tracking activity.

#### Run on Startup (Linux)

To enable automatic startup, create a systemd service:

```bash
sudo tee /etc/systemd/system/synq-collector.service > /dev/null << 'EOF'
[Unit]
Description=Synq Activity Collector
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=$USER
Environment="SYNQ_ENDPOINT=https://synq-ypow.onrender.com"
ExecStart=/home/$USER/synq/build/binaries/collector
WorkingDirectory=/home/$USER/synq
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF

# Enable and start service
sudo systemctl daemon-reload
sudo systemctl enable synq-collector
sudo systemctl start synq-collector
```

Verify the service is running:
```bash
sudo systemctl status synq-collector
```

### Viewing Activity Data

Access the dashboard at:
```
https://synq-ypow.onrender.com/dashboard
```

The dashboard displays:
- Activity breakdown by application
- Time spent per application
- Device filtering capabilities
- Visual progress indicators

### Managing the Collector

```bash
# Check collector status
sudo systemctl status synq-collector

# View logs
sudo journalctl -u synq-collector -f

# Stop collector
sudo systemctl stop synq-collector

# Restart collector
sudo systemctl restart synq-collector

# Disable auto-start
sudo systemctl disable synq-collector
```

### Uninstalling

```bash
# Stop and disable service
sudo systemctl stop synq-collector
sudo systemctl disable synq-collector
sudo rm /etc/systemd/system/synq-collector.service
sudo systemctl daemon-reload

# Remove files
rm -rf ~/synq
rm -rf ~/.config/synq
```

---

## For Developers

### Prerequisites

- Python 3.8 or higher
- PostgreSQL 12 or higher
- GCC with C++11 support
- Git

### Local Development Setup

#### 1. Clone Repository

```bash
git clone https://github.com/Anoop130/synq.git
cd synq
```

#### 2. Install Python Dependencies

```bash
pip install -r requirements.txt
```

#### 3. Configure PostgreSQL

```bash
# Create database and user
sudo -u postgres psql << EOF
CREATE USER synq_user WITH PASSWORD 'synq_password';
CREATE DATABASE synq OWNER synq_user;
GRANT ALL PRIVILEGES ON DATABASE synq TO synq_user;
\q
EOF

# Initialize schema
PGPASSWORD=synq_password psql -U synq_user -h localhost -d synq -f server/schema_postgres.sql
```

#### 4. Configure Environment

Create `server/.env`:

```env
BACKEND_TYPE=postgresql
POSTGRES_HOST=localhost
POSTGRES_PORT=5432
POSTGRES_DB=synq
POSTGRES_USER=synq_user
POSTGRES_PASSWORD=synq_password
```

For Supabase backend (cloud development):

```env
BACKEND_TYPE=supabase
SUPABASE_URL=https://project.supabase.co
SUPABASE_KEY=anon_key
```

#### 5. Start Development Server

```bash
cd server
python3 app.py
```

Server will be available at `http://localhost:5001`

#### 6. Build and Test Collector

```bash
cd collectors/laptop

# Compile
make

# Configure local endpoint
export SYNQ_ENDPOINT=http://localhost:5001

# Run collector
../../build/binaries/collector
```

### Project Structure

```
synq/
├── collectors/
│   └── laptop/              # Linux X11 collector
│       ├── collector.cpp    # Main collection loop
│       ├── config_utils.cpp # Device ID and configuration management
│       ├── net_utils.cpp    # HTTP client and registration
│       ├── x11_utils.cpp    # X11 window title capture
│       ├── proc_utils.cpp   # Process enumeration
│       └── Makefile         # Build configuration
├── server/
│   ├── app.py              # Flask REST API
│   ├── database.py         # Database abstraction layer
│   ├── config.py           # Configuration management
│   ├── schema_postgres.sql # PostgreSQL schema
│   └── .env.example        # Environment template
├── docker-compose.yml      # Docker deployment
├── Dockerfile              # Container definition
├── Procfile               # Render deployment
└── requirements.txt       # Python dependencies
```

### API Reference

#### Register Device

```http
POST /register
Content-Type: application/json

{
  "device_name": "workstation-01",
  "device_type": "linux"
}

Response: {"status": "ok", "device_id": "uuid"}
```

#### Submit Activity Sample

```http
POST /collect
Content-Type: application/json

{
  "device_id": "uuid",
  "timestamp": "2026-02-18 14:30:00",
  "active_window": "application - window title"
}

Response: {"status": "ok", "message": "sample recorded"}
```

#### List Devices

```http
GET /devices

Response: [{"id": "uuid", "device_name": "...", "device_type": "...", ...}]
```

#### View Samples

```http
GET /view?device_id=uuid&limit=100
```

#### Dashboard

```http
GET /dashboard
GET /dashboard?device_id=uuid
```

### Development Workflow

#### Running Tests

```bash
# Test device registration
curl -X POST http://localhost:5001/register \
  -H "Content-Type: application/json" \
  -d '{"device_name": "test-device", "device_type": "linux"}'

# Test sample submission
curl -X POST http://localhost:5001/collect \
  -H "Content-Type: application/json" \
  -d '{"device_id": "device-uuid", "timestamp": "2026-02-18 12:00:00", "active_window": "test"}'
```

#### Building Collector

```bash
cd collectors/laptop

# Clean build
make clean

# Debug build
make CXXFLAGS="-Wall -g -O0"

# Production build
make
```

#### Database Operations

```bash
# View all devices
PGPASSWORD=synq_password psql -U synq_user -h localhost -d synq -c "SELECT * FROM devices;"

# View recent samples
PGPASSWORD=synq_password psql -U synq_user -h localhost -d synq -c "SELECT * FROM samples ORDER BY id DESC LIMIT 10;"

# Clean old data (90-day retention)
PGPASSWORD=synq_password psql -U synq_user -h localhost -d synq -c "DELETE FROM samples WHERE created_at < NOW() - INTERVAL '90 days';"
```

### Deployment

#### Docker Deployment (Local)

```bash
# Start services
docker-compose up -d

# View logs
docker-compose logs -f

# Stop services
docker-compose down
```

#### Render Deployment (Cloud)

1. Push code to GitHub
2. Create Render account at https://render.com
3. Create new Web Service from repository
4. Configure environment variables:
   - `BACKEND_TYPE=supabase`
   - `SUPABASE_URL=<project-url>`
   - `SUPABASE_KEY=<anon-key>`
5. Deploy

#### Supabase Setup

1. Create account at https://supabase.com
2. Create new project
3. Run schema from `server/schema_postgres.sql` in SQL Editor
4. Copy Project URL and anon key from Settings → API

### Contributing

Contributions are welcome. Development process:

1. Fork the repository
2. Create feature branch (`git checkout -b feature/enhancement`)
3. Implement changes with appropriate tests
4. Ensure code follows existing conventions
5. Submit pull request with detailed description

---

## Technical Specifications

### Storage Requirements

**Per device with 5-second sampling interval:**
- 17,280 samples per day (~1.7 MB)
- 51 MB per month
- 612 MB per year

**Recommended retention:** 90 days per device (~150 MB)

### Performance Metrics

- **CPU Usage:** < 0.5% per collector
- **Memory Usage:** 5-10 MB per collector
- **Network Traffic:** ~500 bytes per sample
- **Database Performance:** Indexed queries supporting millions of samples

### Platform Support

**Current:**
- Linux (X11 window manager)
- Server: Any platform with Python 3.8+ and PostgreSQL

**Planned:**
- Android collector
- iOS collector  
- Wayland window manager support
- macOS collector (Accessibility API)
- Windows collector (Win32 API)

### Technology Stack

- **Backend:** Flask 3.0 (Python)
- **Database:** PostgreSQL 16 / Supabase
- **Collector:** C++11, libcurl, X11
- **Deployment:** Docker, Render, Supabase
- **Frontend:** HTML5, CSS3, JavaScript

## Configuration

### Server Configuration

Environment variables (set in `server/.env` or deployment platform):

```env
# Backend type
BACKEND_TYPE=postgresql              # or 'supabase'

# PostgreSQL (local/Docker)
POSTGRES_HOST=localhost
POSTGRES_PORT=5432
POSTGRES_DB=synq
POSTGRES_USER=synq_user
POSTGRES_PASSWORD=synq_password

# Supabase (cloud)
SUPABASE_URL=https://project.supabase.co
SUPABASE_KEY=anon_key
```

### Collector Configuration

Set via environment variable:
```bash
export SYNQ_ENDPOINT=http://server-address:5001
```

Device identification is managed automatically in `~/.config/synq/device.conf`

## Security Considerations

- Window titles may contain sensitive information
- Device IDs are randomly generated UUIDs
- Local device configuration stored in plaintext at `~/.config/synq/device.conf`
- HTTP communication (HTTPS recommended for production deployments)
- Database requires authentication credentials
- No telemetry or external data transmission beyond configured endpoints

## Troubleshooting

### Collector Issues

**Compilation errors:**
```bash
sudo apt-get install build-essential libcurl4-openssl-dev libx11-dev
```

**Connection failures:**
- Verify server endpoint is accessible
- Check network connectivity
- Ensure firewall allows outbound HTTP/HTTPS

**X11 window capture failures:**
- Confirm X11 display is available: `echo $DISPLAY`
- Wayland users may need XWayland compatibility layer

### Server Issues

**Database connection errors:**
```bash
# Verify PostgreSQL is running
sudo systemctl status postgresql

# Test connection
psql -U synq_user -h localhost -d synq
```

**Port conflicts:**
```bash
# Identify process using port
sudo netstat -tlnp | grep 5001
```

**Supabase connection issues:**
- Verify SUPABASE_URL and SUPABASE_KEY are correct
- Check Supabase project is active
- Ensure Row Level Security policies allow operations

### Service Management Issues

**Systemd service not starting:**
```bash
# Check service logs
sudo journalctl -u synq-collector -n 50

# Verify file paths in service definition
sudo systemctl cat synq-collector
```

## Data Management

### Retention Policy

Implement automated cleanup to maintain storage limits:

```sql
DELETE FROM samples WHERE created_at < NOW() - INTERVAL '90 days';
```

Schedule via cron:
```bash
# Daily cleanup at 3 AM
0 3 * * * PGPASSWORD=password psql -U user -h host -d synq -c "DELETE FROM samples WHERE created_at < NOW() - INTERVAL '90 days';"
```

### Backup and Export

```bash
# Export device data
curl https://synq-ypow.onrender.com/devices > devices_backup.json

# Export samples
curl https://synq-ypow.onrender.com/view?limit=10000 > samples_backup.json
```

### Database Backup (PostgreSQL)

```bash
# Local PostgreSQL
pg_dump -U synq_user synq > synq_backup.sql

# Supabase
# Use Supabase dashboard → Database → Backups
```

## Deployment Options

### Option 1: Cloud (Render + Supabase) - Recommended for End Users

**Cost:** $0/month  
**Uptime:** Auto-wake on request  
**Setup time:** 15 minutes

Deployment endpoint: `https://synq-ypow.onrender.com`

### Option 2: Docker (Self-Hosted)

**Cost:** $0 (hardware only)  
**Uptime:** 24/7  
**Setup time:** 5 minutes

```bash
git clone https://github.com/Anoop130/synq.git
cd synq
./install.sh
```

Access at `http://localhost:5001/dashboard`

### Option 3: Manual PostgreSQL (Advanced)

**Cost:** $0  
**Uptime:** Manual management  
**Setup time:** 30 minutes

Requires PostgreSQL server administration and manual service configuration.

## License

MIT License

Copyright (c) 2026 Anoop130

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Technical Stack

- **Backend:** Flask 3.0 (Python)
- **Database:** PostgreSQL 16 / Supabase
- **Collector:** C++11, libcurl, X11
- **Deployment:** Docker Compose, Render
- **Cloud:** Supabase (PostgreSQL), Render (Flask)

## Support & Resources

- **Repository:** https://github.com/Anoop130/synq
- **Issues:** https://github.com/Anoop130/synq/issues
- **Dashboard:** https://synq-ypow.onrender.com/dashboard
- **API Endpoint:** https://synq-ypow.onrender.com

## Roadmap

- [x] Multi-device support with automatic registration
- [x] PostgreSQL backend with indexing
- [x] Supabase cloud integration
- [x] Docker containerization
- [x] Render cloud deployment
- [x] Web dashboard with device filtering
- [ ] Android collector application
- [ ] iOS collector application
- [ ] User authentication system
- [ ] Wayland window manager support
- [ ] macOS collector (Accessibility API)
- [ ] Windows collector (Win32 API)
- [ ] Advanced analytics and insights
- [ ] Data export functionality (CSV/JSON)
- [ ] Mobile dashboard application

---

**Built with PostgreSQL, Flask, and C++**
