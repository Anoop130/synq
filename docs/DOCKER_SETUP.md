# Docker Setup Guide

This guide will help you set up Synq using Docker for local self-hosted deployment.

## Prerequisites

- Docker (version 20.10 or higher)
- Docker Compose (version 2.0 or higher)
- Git

## Installation

### 1. Install Docker

#### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install docker.io docker-compose
sudo usermod -aG docker $USER
# Log out and log back in for group changes to take effect
```

#### macOS

Download and install [Docker Desktop for Mac](https://docs.docker.com/desktop/mac/install/)

#### Windows

Download and install [Docker Desktop for Windows](https://docs.docker.com/desktop/windows/install/)

### 2. Clone Synq

```bash
git clone https://github.com/Anoop130/synq.git
cd synq
```

### 3. Run the Installer

```bash
./install.sh
```

The installer will:
- Check if Docker is installed and running
- Build the Synq server image
- Start PostgreSQL and Synq containers
- Show you the dashboard URL and configuration instructions

## What Gets Installed

The Docker setup includes:

- **PostgreSQL 16** - Database server
- **Synq Server** - Flask application
- **Persistent Volume** - Data persists across container restarts

## Accessing Synq

Once installed, you can access:

- **Dashboard**: http://localhost:5001/dashboard
- **API**: http://localhost:5001

To access from other devices on your network:
```
http://YOUR_IP_ADDRESS:5001
```

Find your IP with: `hostname -I | awk '{print $1}'`

## Managing Synq

### View Logs

```bash
docker-compose logs -f
```

### Stop Synq

```bash
docker-compose down
```

### Restart Synq

```bash
docker-compose restart
```

### Update Synq

```bash
git pull
docker-compose down
docker-compose up -d --build
```

### Backup Database

```bash
# Backup
docker-compose exec postgres pg_dump -U synq_user synq > synq_backup.sql

# Restore
cat synq_backup.sql | docker-compose exec -T postgres psql -U synq_user synq
```

## Configuration

### Change Port

Edit `docker-compose.yml`:

```yaml
services:
  synq-server:
    ports:
      - "8080:5001"  # Change 8080 to your preferred port
```

Then restart:
```bash
docker-compose down && docker-compose up -d
```

### Database Credentials

To change database credentials, edit `docker-compose.yml`:

```yaml
services:
  postgres:
    environment:
      POSTGRES_USER: your_user
      POSTGRES_PASSWORD: your_password
      POSTGRES_DB: your_db
  
  synq-server:
    environment:
      POSTGRES_USER: your_user
      POSTGRES_PASSWORD: your_password
      POSTGRES_DB: your_db
```

## Setting Up Collectors

### Linux Collector

```bash
# On the machine you want to track
cd synq/collectors/laptop
make

# Configure endpoint
export SYNQ_ENDPOINT=http://YOUR_SERVER_IP:5001

# Run collector
../../build/binaries/collector
```

The collector will:
1. Auto-generate a device ID
2. Register with the server
3. Start collecting data

### Run Collector as Service

Create `/etc/systemd/system/synq-collector.service`:

```ini
[Unit]
Description=Synq Activity Collector
After=network.target

[Service]
Type=simple
User=your_username
Environment="SYNQ_ENDPOINT=http://YOUR_SERVER_IP:5001"
ExecStart=/path/to/synq/build/binaries/collector
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl daemon-reload
sudo systemctl enable synq-collector
sudo systemctl start synq-collector
```

## Troubleshooting

### Port Already in Use

If port 5001 is already in use:

```bash
# Find what's using the port
sudo netstat -tlnp | grep 5001

# Change Synq's port in docker-compose.yml
```

### Container Won't Start

```bash
# Check logs
docker-compose logs

# Common issues:
# 1. Port conflict - change port in docker-compose.yml
# 2. Permission denied - ensure user is in docker group
# 3. Out of disk space - clean up with: docker system prune
```

### Database Connection Failed

```bash
# Restart PostgreSQL container
docker-compose restart postgres

# Wait for it to be healthy
docker-compose ps
```

### Can't Access from Other Devices

1. Check firewall:
```bash
sudo ufw allow 5001/tcp
```

2. Ensure server is binding to all interfaces (0.0.0.0) - already configured in docker-compose.yml

3. Find your IP:
```bash
hostname -I
```

## Uninstalling

To completely remove Synq:

```bash
cd synq
docker-compose down -v  # -v removes volumes (deletes data!)
cd ..
rm -rf synq
```

## Next Steps

- [Configure collectors](MULTI_DEVICE.md)
- [View dashboard](http://localhost:5001/dashboard)
- [API documentation](../README.md#api-endpoints)
