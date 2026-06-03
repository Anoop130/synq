# archived: C++ laptop collector

archived on 2026-06-03 as part of feat/gnome_submodule_collector.

reason: replaced by the synq-gnome GNOME Shell extension (collectors/gnome submodule)
combined with the Python DBus bridge (collectors/synq-gnome/collector.py).
the extension is event-driven and provides richer tracking (app breakdown, heatmap,
pause on suspend/lock) without requiring libcurl or libX11 as system dependencies.
the C++ binary required a compile step and polled at a fixed interval; the new setup
is purely Python and requires no build toolchain on the end device.

dead code not archived:
- proc_utils.cpp / proc_utils.h: process enumeration, never compiled, no callers
- collectors/laptop/collectors/: empty nested directory, stale submodule artifact
