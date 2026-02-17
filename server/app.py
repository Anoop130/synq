from flask import Flask, request, jsonify, render_template_string
from database import insert_sample, get_db

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
    db = get_db()
    rows = db.execute("SELECT * FROM samples ORDER BY id DESC LIMIT 10").fetchall()
    db.close()
    return jsonify([dict(row) for row in rows])


@app.route("/dashboard")
def dashboard():
    db = get_db()
    rows = db.execute("""
        SELECT active_window, COUNT(*) as samples
        FROM samples
        GROUP BY active_window
        ORDER BY samples DESC
    """).fetchall()
    db.close()

    total = sum(r["samples"] for r in rows)
    interval = 5  # polling interval in seconds

    data = []
    for r in rows:
        seconds = r["samples"] * interval
        hrs = seconds // 3600
        mins = (seconds % 3600) // 60
        pct = (r["samples"] / total) * 100 if total else 0
        data.append({
            "app": r["active_window"] or "Unknown",
            "hours": int(hrs),
            "minutes": int(mins),
            "pct": round(pct, 2)
        })

    html = """
    <html>
    <head>
        <title>Synq Dashboard</title>
        <style>
            body { background:#0d1117; color:#d1d5db; font-family: monospace; max-width:700px; margin:auto; padding:2em; }
            h2 { color:#fff; margin-bottom:1em; }
            .bar { display:inline-block; height:12px; background:#58a6ff; }
            .bar-bg { background:#2f343d; width:200px; display:inline-block; margin:0 1em; vertical-align:middle; }
            .row { margin:6px 0; }
            .pct { color:#9ca3af; }
        </style>
    </head>
    <body>
        <h2>From: {{ start }} – To: {{ end }}</h2>
        {% for d in data %}
        <div class="row">
            {{ "%-20s" % d.app }} {{ "%2d hrs %2d mins" % (d.hours, d.minutes) }}
            <div class="bar-bg"><div class="bar" style="width:{{ d.pct*2 }}px"></div></div>
            <span class="pct">{{ "%.2f" % d.pct }} %</span>
        </div>
        {% endfor %}
    </body>
    </html>
    """

    from datetime import datetime, timedelta
    end = datetime.now()
    start = end - timedelta(days=7)

    return render_template_string(html, data=data,
                                  start=start.strftime("%d %B %Y"),
                                  end=end.strftime("%d %B %Y"))



if __name__ == "__main__":
    print("Starting server on http://0.0.0.0:5000")
    app.run(host="0.0.0.0", port=5000)
