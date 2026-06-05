-- v2 migration: adds app_name, activity, and duration_seconds columns to samples.
-- safe to run on an existing database; IF NOT EXISTS guards prevent errors on re-run.
-- for Supabase: paste into the Supabase SQL editor and run there.
ALTER TABLE samples ADD COLUMN IF NOT EXISTS app_name VARCHAR(255);
ALTER TABLE samples ADD COLUMN IF NOT EXISTS activity TEXT;
ALTER TABLE samples ADD COLUMN IF NOT EXISTS duration_seconds INTEGER;
CREATE INDEX IF NOT EXISTS idx_samples_app_name ON samples(app_name);
