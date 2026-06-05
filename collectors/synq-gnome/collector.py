import os
import uuid
import requests
from datetime import datetime
from gi.repository import GLib, Gio

DBUS_NAME    = 'io.github.Synq.GnomeExtension'
DBUS_PATH    = '/io/github/Synq/GnomeExtension'
DBUS_IFACE   = 'io.github.Synq.GnomeExtension'
CONFIG_DIR   = os.path.expanduser('~/.config/synq')
DEVICE_FILE  = os.path.join(CONFIG_DIR, 'device.conf')
ENDPOINT     = os.environ.get('SYNQ_ENDPOINT', 'http://127.0.0.1:5001')
IDLE_SENTINEL = '__IDLE__'

def _read_device_id():
    """
    reads the device UUID from ~/.config/synq/device.conf.
    creates the file with a new uuid4 if it does not exist.

    inputs:
        none

    returns:
        str: UUID v4, exactly 36 chars
    """
    os.makedirs(CONFIG_DIR, exist_ok=True)
    if os.path.exists(DEVICE_FILE):
        with open(DEVICE_FILE) as f:
            for line in f:
                if line.startswith('device_id='):
                    return line.strip()[len('device_id='):]
    device_id = str(uuid.uuid4())
    _write_device_id(device_id)
    return device_id

def _write_device_id(device_id):
    """
    writes a device UUID to ~/.config/synq/device.conf in key=value format.

    inputs:
        device_id (str) UUID v4, exactly 36 chars

    returns:
        None
    """
    os.makedirs(CONFIG_DIR, exist_ok=True)
    with open(DEVICE_FILE, 'w') as f:
        f.write(f'device_id={device_id}\n')

def _register_device(device_id):
    """
    registers the device with the Synq server via POST /register.
    logs the result; does not raise on failure.

    inputs:
        device_id (str) UUID v4, exactly 36 chars

    returns:
        None
    """
    try:
        import socket
        name = socket.gethostname()
        r = requests.post(f'{ENDPOINT}/register', json={
            'device_id':   device_id,
            'device_name': name,
            'device_type': 'linux'
        }, timeout=5)
        if r.ok:
            print(f'[INFO] registered device {device_id}')
        else:
            print(f'[WARN] registration returned {r.status_code}, continuing with cached id')
    except Exception as e:
        print(f'[WARN] registration failed: {e}, continuing with cached id')

def _parse_iso(ts):
    """
    parses an ISO 8601 timestamp string into a timezone-aware datetime.
    handles the Z suffix by replacing it with +00:00 for fromisoformat compat.

    inputs:
        ts (str) ISO 8601 datetime string, unbounded length

    returns:
        datetime.datetime, timezone-aware
    """
    return datetime.fromisoformat(ts.replace('Z', '+00:00'))

def _post_sample(device_id, title, timestamp, duration_seconds):
    """
    posts one completed window session to the Synq server via POST /collect.

    inputs:
        device_id        (str) UUID v4, exactly 36 chars
        title            (str) raw window title, unbounded length
        timestamp        (str) ISO 8601 datetime string marking session start
        duration_seconds (int or None) session length in whole seconds; None if unknown

    returns:
        None
    """
    payload = {
        'device_id':     device_id,
        'timestamp':     timestamp,
        'active_window': title
    }
    if duration_seconds is not None:
        payload['duration_seconds'] = duration_seconds
    try:
        r = requests.post(f'{ENDPOINT}/collect', json=payload, timeout=5)
        if not r.ok:
            print(f'[ERROR] /collect returned {r.status_code}: {r.text[:120]}')
        elif os.environ.get('SYNQ_VERBOSE'):
            dur_str = f'{duration_seconds}s' if duration_seconds is not None else 'unknown duration'
            print(f'[DEBUG] collected: {title[:60]} ({dur_str})')
    except Exception as e:
        print(f'[ERROR] /collect failed: {e}')

def main():
    """
    entry point: reads device id, subscribes to the synq-gnome DBus signal,
    and forwards each completed window session to the Synq server.
    each sample is posted when the NEXT focus change fires so duration is known.

    inputs:
        none (reads SYNQ_ENDPOINT and SYNQ_VERBOSE from environment)

    returns:
        None (runs until interrupted)
    """
    device_id = _read_device_id()
    _register_device(device_id)

    bus = Gio.bus_get_sync(Gio.BusType.SESSION, None)

    prev_title = None
    prev_ts    = None

    def on_window_changed(conn, sender, path, iface, signal, params, *_):
        nonlocal prev_title, prev_ts
        title, timestamp = params.unpack()

        if title == IDLE_SENTINEL:
            if prev_title is not None:
                _post_sample(device_id, prev_title, prev_ts, None)
            prev_title = None
            prev_ts    = None
            return

        if prev_title is not None:
            try:
                duration_seconds = max(0, int(
                    (_parse_iso(timestamp) - _parse_iso(prev_ts)).total_seconds()
                ))
            except Exception:
                duration_seconds = None
            _post_sample(device_id, prev_title, prev_ts, duration_seconds)

        prev_title = title
        prev_ts    = timestamp

    sub_id = bus.signal_subscribe(
        None,
        DBUS_IFACE,
        'WindowChanged',
        DBUS_PATH,
        None,
        Gio.DBusSignalFlags.NONE,
        on_window_changed
    )

    print(f'[INFO] subscribed to {DBUS_NAME} WindowChanged signal')
    print(f'[INFO] forwarding to {ENDPOINT}')

    loop = GLib.MainLoop()
    try:
        loop.run()
    except KeyboardInterrupt:
        pass
    finally:
        bus.signal_unsubscribe(sub_id)
        print('[INFO] collector stopped')

if __name__ == '__main__':
    main()
