CREATE TABLE IF NOT EXISTS samples (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    device TEXT,
    timestamp TEXT,
    active_window TEXT
);

ALTER TABLE samples ADD COLUMN duration INTEGER DEFAULT 5;
