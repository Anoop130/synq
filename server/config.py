import os

# Backend type: postgresql (Docker local) or supabase (cloud)
BACKEND_TYPE = os.getenv("BACKEND_TYPE", "postgresql")

# PostgreSQL configuration (local or Docker deployment)
POSTGRES_HOST = os.getenv("POSTGRES_HOST", "localhost")
POSTGRES_PORT = os.getenv("POSTGRES_PORT", "5432")
POSTGRES_DB = os.getenv("POSTGRES_DB", "synq")
POSTGRES_USER = os.getenv("POSTGRES_USER", "synq_user")
POSTGRES_PASSWORD = os.getenv("POSTGRES_PASSWORD", "synq_password")

# Supabase configuration (cloud deployment option)
SUPABASE_URL = os.getenv("SUPABASE_URL", "")
SUPABASE_KEY = os.getenv("SUPABASE_KEY", "")  # Public anon key for client access