#!/bin/bash
# Setup script for PostgreSQL database

set -e

# Default values (can be overridden by environment variables)
DB_NAME="${POSTGRES_DB:-synq}"
DB_USER="${POSTGRES_USER:-synq_user}"
DB_PASSWORD="${POSTGRES_PASSWORD:-synq_password}"

echo "Setting up PostgreSQL database for Synq..."

# Check if PostgreSQL is installed
if ! command -v psql &> /dev/null; then
    echo "PostgreSQL is not installed. Please install it first:"
    echo "  Ubuntu/Debian: sudo apt-get install postgresql postgresql-contrib"
    echo "  Fedora/RHEL: sudo dnf install postgresql-server postgresql-contrib"
    exit 1
fi

# Create database and user
sudo -u postgres psql <<EOF
-- Create user if not exists
DO \$\$
BEGIN
  IF NOT EXISTS (SELECT FROM pg_user WHERE usename = '$DB_USER') THEN
    CREATE USER $DB_USER WITH PASSWORD '$DB_PASSWORD';
  END IF;
END
\$\$;

-- Create database if not exists
SELECT 'CREATE DATABASE $DB_NAME OWNER $DB_USER'
WHERE NOT EXISTS (SELECT FROM pg_database WHERE datname = '$DB_NAME')\gexec

-- Grant privileges
GRANT ALL PRIVILEGES ON DATABASE $DB_NAME TO $DB_USER;
EOF

echo "Database created successfully!"
echo ""
echo "Now run the schema:"
echo "  psql -U $DB_USER -d $DB_NAME -f schema_postgres.sql"
echo ""
echo "Or using environment variable:"
echo "  PGPASSWORD=$DB_PASSWORD psql -U $DB_USER -h localhost -d $DB_NAME -f schema_postgres.sql"
