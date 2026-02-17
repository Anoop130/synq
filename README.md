# Synq

**Synq** is a lightweight activity tracking system that monitors and logs your active applications and workflows. It consists of a C++ collector that captures system activity data and a Python Flask server that aggregates and visualizes your time usage patterns.

## Features

- **Real-time Activity Tracking** - Captures active window titles and process information at configurable intervals
- **Cross-platform Support** - Built for Linux systems with X11 window manager support
- **Local and Remote Logging** - Dual logging to local files and remote server database
- **Visual Dashboard** - Web-based interface to visualize time spent across different applications
- **REST API** - JSON endpoints for programmatic access to collected data
- **Minimal Overhead** - Lightweight C++ collector with efficient sampling

## Architecture

```
┌─────────────────┐
│  C++ Collector  │  → Samples every 5s
│   (collectors)  │  → Captures active window, process list
└────────┬────────┘
         │ HTTP POST
         ▼
┌─────────────────┐
│  Flask Server   │  → Stores in SQLite
│    (server)     │  → Provides REST API & Dashboard
└─────────────────┘
```

## Prerequisites

### System Requirements

- **Operating System**: Linux with X11
- **C++ Compiler**: g++ with C++11 support
- **Python**: Python 3.8 or higher

### Dependencies

**C++ Collector:**
- libcurl (`libcurl4-openssl-dev`)
- sqlite3 (`libsqlite3-dev`)
- X11 development libraries (`libx11-dev`)

**Python Server:**
- Flask 3.0+

### Installation

#### Ubuntu/Debian

```bash
# Install C++ dependencies
sudo apt-get update
sudo apt-get install -y build-essential libcurl4-openssl-dev libsqlite3-dev libx11-dev

# Install Python dependencies
sudo apt-get install -y python3 python3-pip
pip3 install flask
```

#### Fedora/RHEL

```bash
# Install C++ dependencies
sudo dnf install -y gcc-c++ libcurl-devel sqlite-devel libX11-devel

# Install Python dependencies
sudo dnf install -y python3 python3-pip
pip3 install flask
```

## Quick Start

### 1. Clone the Repository

```bash
git clone <repository-url>
cd synq
```

### 2. Build the Collector

```bash
# Create build directories
mkdir -p build/binaries build/logs

# Compile the collector
cd collectors/laptop
make
cd ../..
```

The compiled binary will be located at `build/binaries/collector`.

### 3. Initialize the Database

```bash
# Create and initialize the SQLite database
sqlite3 server/data.db < server/schema.sql
```

### 4. Start the Server

```bash
cd server
python3 app.py
```

The server will start on `http://0.0.0.0:5000`.

### 5. Run the Collector

In a new terminal:

```bash
cd synq
./build/binaries/collector
```

The collector will begin sampling your active window every 5 seconds and sending data to the server.

## Usage

### Web Interface

Once the server is running and collecting data, access the dashboard:

```
http://localhost:5000/dashboard
```

The dashboard displays:
- Time spent in each application
- Percentage breakdown of your activity
- Visual progress bars for easy comparison
- Date range for the data displayed

### API Endpoints

#### POST /collect

Submit a sample to the server.

**Request:**
```bash
curl -X POST http://localhost:5000/collect \
  -H "Content-Type: application/json" \
  -d '{
    "device": "laptop",
    "timestamp": "2026-02-16 18:30:00",
    "active_window": "Terminal",
    "process_count": 485
  }'
```

**Response:**
```json
{
  "status": "ok",
  "message": "sample recorded"
}
```

#### GET /view

Retrieve the 10 most recent samples.

**Request:**
```bash
curl http://localhost:5000/view
```

**Response:**
```json
[
  {
    "id": 1,
    "device": "laptop",
    "timestamp": "2026-02-16 18:30:00",
    "active_window": "Terminal"
  }
]
```

#### GET /dashboard

View the visual dashboard in your browser.

## Configuration

### Collector Configuration

Edit `collectors/laptop/config.ini` to adjust the sampling interval:

```ini
interval=5
```

The interval is in seconds. Default is 5 seconds.

### Server Configuration

Modify `server/app.py` to change the server host or port:

```python
if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
```

## Project Structure

```
synq/
├── collectors/
│   └── laptop/
│       ├── collector.cpp       # Main collector application
│       ├── x11_utils.cpp       # X11 window title capture
│       ├── proc_utils.cpp      # Process enumeration utilities
│       ├── net_utils.cpp       # HTTP client for server communication
│       ├── config_utils.cpp    # Configuration file parser
│       ├── Makefile            # Build configuration
│       └── config.ini          # Runtime configuration
├── server/
│   ├── app.py                  # Flask application and routes
│   ├── database.py             # SQLite database interface
│   └── schema.sql              # Database schema definition
├── build/
│   ├── binaries/               # Compiled executables
│   └── logs/                   # Local log files
└── README.md                   # This file
```

## Development

### Building in Debug Mode

```bash
cd collectors/laptop
make clean
CXX=g++ CXXFLAGS="-Wall -g -O0" make
```

### Testing Network Functionality

A standalone network test utility is provided:

```bash
cd collectors/laptop
g++ -Wall -O2 -o ../../build/binaries/test_net test_net.cpp net_utils.cpp -lcurl
../../build/binaries/test_net
```

### Database Schema

The SQLite database uses a simple schema:

```sql
CREATE TABLE IF NOT EXISTS samples (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    device TEXT,
    timestamp TEXT,
    active_window TEXT
);
```

## Troubleshooting

### Collector doesn't start

**Issue**: Cannot open log file
```
Could not open local log file!
```

**Solution**: Ensure the build directories exist:
```bash
mkdir -p build/binaries build/logs
```

### Server returns 500 errors

**Issue**: Cannot connect to database

**Solution**: Initialize the database:
```bash
sqlite3 server/data.db < server/schema.sql
```

### X11 window capture fails

**Issue**: Cannot get active window

**Solution**: Ensure you're running on an X11 display:
```bash
echo $DISPLAY  # Should output something like :0 or :1
```

For Wayland users, you may need to run under XWayland or modify the collector to use Wayland protocols.

### Port 5000 already in use

**Solution**: Kill existing Flask processes or change the port:
```bash
pkill -f "python3 app.py"
# Or change the port in server/app.py
```

## Security Considerations

- The collector captures active window titles, which may contain sensitive information
- Data is stored locally in an unencrypted SQLite database
- The Flask development server is not suitable for production use
- Consider using HTTPS and authentication for production deployments

## Performance

- **CPU Usage**: < 0.5% on modern systems
- **Memory Usage**: ~5-10 MB for the collector
- **Network**: ~500 bytes per sample (every 5 seconds by default)
- **Storage**: ~50 KB per day of continuous operation

## Roadmap

- [ ] Support for Wayland window managers
- [ ] macOS support via Accessibility APIs
- [ ] Windows support via Win32 APIs
- [ ] Export data to CSV/JSON
- [ ] Time range filtering in dashboard
- [ ] Multiple device support
- [ ] Encryption for sensitive data
- [ ] Production-ready server deployment guide


## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Acknowledgments

Built with:
- [Flask](https://flask.palletsprojects.com/) - Web framework
- [libcurl](https://curl.se/libcurl/) - HTTP client library
- [SQLite](https://www.sqlite.org/) - Embedded database
