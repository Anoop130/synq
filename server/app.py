from flask import Flask, request, jsonify, render_template_string
from database import register_device, get_device, get_all_devices, insert_sample, get_samples, get_dashboard_data
import uuid

app = Flask(__name__)

@app.route("/register", methods=["POST"])
def register():
    """
    registers a device, generating a UUID if one is not supplied.

    inputs:
        JSON body with optional device_id (str, UUID v4, max 36 chars),
        optional device_name (str, max 255 chars),
        optional device_type (str, max 50 chars)

    returns:
        JSON with status (str) and device_id (str), HTTP 200 on success or 400 on bad input
    """
    data = request.get_json(force=True)
    if not data:
        return jsonify({"error": "No JSON received"}), 400

    device_id   = data.get("device_id") or str(uuid.uuid4())
    device_name = data.get("device_name", "Unknown Device")
    device_type = data.get("device_type", "unknown")

    register_device(device_id, device_name, device_type)
    return jsonify({"status": "ok", "device_id": device_id}), 200

@app.route("/collect", methods=["POST"])
def collect():
    """
    records one activity sample for a registered device.

    inputs:
        JSON body with device_id (str, UUID v4, max 36 chars, required),
        timestamp (str, ISO 8601, e.g. 2026-05-24 12:00:00),
        active_window (str, window title, unbounded length)

    returns:
        JSON with status and message, HTTP 200 on success, 400/404 on error
    """
    data = request.get_json(force=True)
    if not data:
        return jsonify({"error": "No JSON received"}), 400

    device_id = data.get("device_id")
    if not device_id:
        return jsonify({"error": "device_id required"}), 400

    timestamp     = data.get("timestamp", "")
    active_window = data.get("active_window", "")

    # verify device exists before inserting sample
    device = get_device(device_id)
    if not device:
        return jsonify({"error": "Device not registered"}), 404

    insert_sample(device_id, timestamp, active_window)
    return jsonify({"status": "ok", "message": "sample recorded"}), 200

@app.route("/devices", methods=["GET"])
def list_devices():
    """
    returns all registered devices ordered by most recently seen.

    inputs:
        none

    returns:
        JSON array of device objects, HTTP 200
    """
    devices = get_all_devices()
    return jsonify(devices)

@app.route("/view", methods=["GET"])
def view():
    """
    returns recent activity samples, optionally filtered to one device.

    inputs:
        query param device_id (str, UUID v4, optional),
        query param limit (int, default 10, min 1)

    returns:
        JSON array of sample objects, HTTP 200
    """
    device_id = request.args.get("device_id")
    limit     = int(request.args.get("limit", 10))
    rows      = get_samples(device_id=device_id, limit=limit)
    return jsonify(rows)

@app.route("/dashboard")
def dashboard():
    """
    renders the HTML activity dashboard for today's 24 hour window.

    inputs:
        query param device_id (str, UUID v4, optional filter)

    returns:
        HTML string, HTTP 200
    """
    device_id = request.args.get("device_id")
    devices   = get_all_devices()
    rows      = get_dashboard_data(device_id=device_id)

    total    = sum(r["samples"] for r in rows)
    interval = 5  # polling interval in seconds

    data = []
    for r in rows:
        seconds = r["samples"] * interval
        hrs     = seconds // 3600
        mins    = (seconds % 3600) // 60
        pct     = (r["samples"] / total) * 100 if total else 0
        data.append({
            "app":     r["active_window"] or "Unknown",
            "hours":   int(hrs),
            "minutes": int(mins),
            "pct":     round(pct, 2)
        })

    # build device selector options
    device_options = ""
    for d in devices:
        selected = "selected" if device_id == str(d["id"]) else ""
        device_options += f'<option value="{d["id"]}" {selected}>{d["device_name"]}</option>'

    html = f"""
    <html>
    <head>
        <title>Synq Dashboard</title>
        <style>
            body {{ background:#0d1117; color:#d1d5db; font-family: monospace; max-width:900px; margin:auto; padding:2em; }}
            h2 {{ color:#fff; margin-bottom:1em; }}
            .controls {{ margin-bottom:2em; }}
            select {{ padding:8px; font-size:14px; background:#21262d; color:#d1d5db; border:1px solid #30363d; border-radius:6px; }}
            .bar {{ display:inline-block; height:12px; background:#58a6ff; }}
            .bar-bg {{ background:#2f343d; width:200px; display:inline-block; margin:0 1em; vertical-align:middle; }}
            .row {{ margin:6px 0; }}
            .pct {{ color:#9ca3af; }}
        </style>
        <script>
            function filterDevice() {{
                const deviceId = document.getElementById('deviceSelect').value;
                if (deviceId === 'all') {{
                    window.location.href = '/dashboard';
                }} else {{
                    window.location.href = '/dashboard?device_id=' + deviceId;
                }}
            }}
        </script>
    </head>
    <body>
        <h2>Synq Dashboard</h2>
        <div class="controls">
            <label for="deviceSelect">Device: </label>
            <select id="deviceSelect" onchange="filterDevice()">
                <option value="all" {"selected" if not device_id else ""}>All Devices</option>
                {device_options}
            </select>
        </div>
        {"".join([f'''
        <div class="row">
            {d["app"][:30]:30s} {d["hours"]:2d} hrs {d["minutes"]:2d} mins
            <div class="bar-bg"><div class="bar" style="width:{d["pct"]*2}px"></div></div>
            <span class="pct">{d["pct"]:.2f} %</span>
        </div>
        ''' for d in data])}
    </body>
    </html>
    """

    return html

if __name__ == "__main__":
    import os
    port = int(os.environ.get("PORT", 5001))
    print(f"[INFO] starting server on http://0.0.0.0:{port}")
    app.run(host="0.0.0.0", port=port)
