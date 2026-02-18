-- PostgreSQL schema for Synq activity tracking system
DROP TABLE IF EXISTS samples CASCADE;
DROP TABLE IF EXISTS devices CASCADE;

-- Devices table: stores registered devices and their metadata
CREATE TABLE devices (
    id UUID PRIMARY KEY,
    device_name VARCHAR(255) NOT NULL,
    device_type VARCHAR(50) NOT NULL,  -- 'linux', 'android', 'ios', 'macos'
    first_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Samples table: stores activity samples from all devices
CREATE TABLE samples (
    id BIGSERIAL PRIMARY KEY,
    device_id UUID NOT NULL REFERENCES devices(id) ON DELETE CASCADE,
    timestamp TIMESTAMP NOT NULL,
    active_window TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Performance indexes
CREATE INDEX idx_samples_device_id ON samples(device_id);
CREATE INDEX idx_samples_timestamp ON samples(timestamp);
CREATE INDEX idx_samples_device_timestamp ON samples(device_id, timestamp DESC);
CREATE INDEX idx_devices_last_seen ON devices(last_seen DESC);