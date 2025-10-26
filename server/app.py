from flask import Flask, request, jsonify
from database import insert_sample

app = Flask(__name__)

@app.route("/collect", methods=["POST"])
def collect():
    data = request.get_json(force=True)
    if not data:
        return jsonify({"error": "No JSON received"}), 400

    device = data.get("device", "unknown")
    timestamp = data.get("timestamp", "")
    active_window = data.get("active_window", "")

    insert_sample(device, timestamp, active_window)
    return jsonify({"status": "ok", "message": "sample recorded"}), 200

@app.route("/view", methods=["GET"])
def view():
    import sqlite3
    conn = sqlite3.connect("server/data.db")
    conn.row_factory = sqlite3.Row
    rows = conn.execute("SELECT * FROM samples ORDER BY id DESC LIMIT 10").fetchall()
    conn.close()
    return jsonify([dict(row) for row in rows])

if __name__ == "__main__":
    print("Starting server on http://0.0.0.0:5000")
    app.run(host="0.0.0.0", port=5000)
