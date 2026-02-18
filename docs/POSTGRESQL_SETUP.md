# PostgreSQL Setup Guide

This guide will help you set up PostgreSQL for Synq metrics storage.

## Why PostgreSQL?

PostgreSQL offers several advantages over SQLite:
- **Better concurrency** - Multiple collectors can write simultaneously
- **Advanced indexing** - Faster queries on large datasets
- **Scalability** - Handle millions of samples efficiently
- **Remote access** - Centralized database for multiple devices
- **Advanced features** - Window functions, CTEs, better aggregations

## Installation

### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install postgresql postgresql-contrib
```

### Fedora/RHEL

```bash
sudo dnf install postgresql-server postgresql-contrib
sudo postgresql-setup --initdb
sudo systemctl start postgresql
sudo systemctl enable postgresql
```

### macOS

```bash
brew install postgresql@15
brew services start postgresql@15
```

## Setup

### 1. Install Python Dependencies

```bash
cd server
pip install -r requirements.txt
```

### 2. Create Database and User

**Option A: Using the setup script (Linux)**

```bash
cd server
chmod +x setup_postgres.sh
./setup_postgres.sh
```

**Option B: Manual setup**

```bash
# Switch to postgres user
sudo -u postgres psql

# In PostgreSQL prompt:
CREATE USER synq_user WITH PASSWORD 'synq_password';
CREATE DATABASE synq OWNER synq_user;
GRANT ALL PRIVILEGES ON DATABASE synq TO synq_user;
\q
```

### 3. Initialize Schema

```bash
cd server
PGPASSWORD=synq_password psql -U synq_user -h localhost -d synq -f schema_postgres.sql
```

### 4. Configure Environment

Copy the example environment file:

```bash
cd server
cp .env.example .env
```

Edit `.env` with your database credentials:

```bash
DB_TYPE=postgresql
POSTGRES_HOST=localhost
POSTGRES_PORT=5432
POSTGRES_DB=synq
POSTGRES_USER=synq_user
POSTGRES_PASSWORD=synq_password
```

### 5. Start the Server

```bash
cd server
python3 app.py
```

## Configuration Options

### Using Environment Variables

You can set database configuration via environment variables:

```bash
export DB_TYPE=postgresql
export POSTGRES_HOST=localhost
export POSTGRES_PORT=5432
export POSTGRES_DB=synq
export POSTGRES_USER=synq_user
export POSTGRES_PASSWORD=synq_password

python3 app.py
```

### Fallback to SQLite

If you want to temporarily use SQLite:

```bash
export DB_TYPE=sqlite
python3 app.py
```

## Remote PostgreSQL Setup

To use a remote PostgreSQL server:

1. Update `postgresql.conf` on the server:
   ```
   listen_addresses = '*'
   ```

2. Update `pg_hba.conf` to allow remote connections:
   ```
   host    synq    synq_user    0.0.0.0/0    md5
   ```

3. Restart PostgreSQL:
   ```bash
   sudo systemctl restart postgresql
   ```

4. Update your `.env` file with the remote host:
   ```
   POSTGRES_HOST=your-server-ip
   ```

## Verification

Test the connection:

```bash
psql -U synq_user -h localhost -d synq -c "SELECT COUNT(*) FROM samples;"
```

Check if data is being collected:

```bash
curl http://localhost:5000/view
```

## Troubleshooting

### Connection refused

**Issue**: `psql: error: connection to server on socket failed`

**Solution**: Ensure PostgreSQL is running:
```bash
sudo systemctl status postgresql
sudo systemctl start postgresql
```

### Authentication failed

**Issue**: `FATAL: password authentication failed for user "synq_user"`

**Solution**: Verify credentials in `.env` match the database user

### Permission denied

**Issue**: `ERROR: permission denied for table samples`

**Solution**: Grant proper permissions:
```bash
sudo -u postgres psql -d synq
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO synq_user;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA public TO synq_user;
```

## Migration from SQLite

To migrate existing SQLite data to PostgreSQL:

```bash
# Export from SQLite
sqlite3 data.db "SELECT * FROM samples;" > samples.csv

# Import to PostgreSQL
psql -U synq_user -d synq -c "\COPY samples(device, timestamp, active_window) FROM 'samples.csv' WITH CSV HEADER;"
```

## Performance Tuning

For better performance with large datasets:

```sql
-- Analyze tables for query optimization
ANALYZE samples;

-- Add additional indexes if needed
CREATE INDEX idx_samples_timestamp_device ON samples(timestamp, device);
CREATE INDEX idx_samples_created_at ON samples(created_at);
```

## Backup

Regular backups:

```bash
# Backup
pg_dump -U synq_user -h localhost synq > synq_backup.sql

# Restore
psql -U synq_user -h localhost synq < synq_backup.sql
```
