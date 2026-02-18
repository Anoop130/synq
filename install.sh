#!/bin/bash
set -e

echo "  Synq - Multi-Device Activity Tracker  "
echo "========================================="
echo ""

# Check if Docker is installed
if ! command -v docker &> /dev/null; then
    echo "{$red} Docker is not installed."
    echo ""
    echo "Please install Docker first:"
    echo "  Ubuntu/Debian: sudo apt-get install docker.io docker-compose"
    echo "  macOS: https://docs.docker.com/desktop/mac/install/"
    echo "  Other: https://docs.docker.com/get-docker/"
    exit 1
fi

# Check if docker-compose is installed
if ! command -v docker-compose &> /dev/null; then
    echo "{$red} docker-compose is not installed."
    echo ""
    echo "Please install docker-compose:"
    echo "  sudo apt-get install docker-compose"
    exit 1
fi

echo " Docker and docker-compose are installed"
echo ""

# Check if docker is running
if ! docker ps &> /dev/null; then
    echo "{$red} Docker is not running or you don't have permission."
    echo ""
    echo "Please start Docker or add your user to the docker group:"
    echo "  sudo usermod -aG docker $USER"
    echo "  Then log out and log back in."
    exit 1
fi

echo "Docker is running"
echo ""

# Stop existing containers if any
echo "Stopping existing Synq containers (if any)..."
docker-compose down 2>/dev/null || true

# Build and start containers
echo "Building and starting Synq..."
docker-compose up -d --build

# Wait for services to be healthy
echo ""
echo "Waiting for services to start..."
sleep 5

# Check if services are running
if docker-compose ps | grep -q "Up"; then
    echo ""
    echo "========================================="
    echo "Synq is now running!"
    echo "========================================="
    echo ""
    echo "Dashboard: http://localhost:5001/dashboard"
    echo "API:       http://localhost:5001"
    echo ""
    echo "To connect devices:"
    echo "   1. Install the collector on your device"
    echo "   2. Configure endpoint: http://$(hostname -I | awk '{print $1}'):5001"
    echo ""
    echo " Useful commands:"
    echo "   View logs:    docker-compose logs -f"
    echo "   Stop Synq:    docker-compose down"
    echo "   Restart:      docker-compose restart"
    echo ""
else
    echo ""
    echo "Failed to start Synq"
    echo ""
    echo "Check logs with: docker-compose logs"
    exit 1
fi
