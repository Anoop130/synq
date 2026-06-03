#include "x11_utils.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <iostream>
#include <cstdlib>

// walks the X11 window tree upward from w until reaching a top level window.
// input:  d (Display ptr) open X11 display connection
//         w (Window) starting child window id
// output: Window, the top level ancestor of w, or w itself if the tree walk fails
Window getTopLevelParent(Display* d, Window w) {
    Window root, parent;
    Window* children = nullptr;
    unsigned int nchildren;

    while (true) {
        if (XQueryTree(d, w, &root, &parent, &children, &nchildren) == 0) {
            return w;
        }
        if (children) {
            XFree(children);
        }
        if (parent == root || parent == None) {
            return w;
        }
        w = parent;
    }
}

// reads the title of the currently focused X11 window.
// primary source: _NET_ACTIVE_WINDOW on the root window (set by the window manager).
// fallback: XGetInputFocus + tree walk if _NET_ACTIVE_WINDOW is unavailable.
// title fetch: tries _NET_WM_NAME first, then WM_NAME via XFetchName.
// input:  none
// output: std::string, the window title or a sentinel string if X11 is unavailable
std::string getActiveWindowTitle() {
    const char* display_var = std::getenv("DISPLAY");
    if (!display_var) {
        std::cerr << "[ERROR] DISPLAY environment variable not set\n";
        return "Unknown";
    }

    Display* d = XOpenDisplay(nullptr);
    if (!d) {
        std::cerr << "[ERROR] cannot open X11 display: " << display_var << "\n";
        return "Unknown";
    }

    Window root = DefaultRootWindow(d);
    Atom net_active = XInternAtom(d, "_NET_ACTIVE_WINDOW", False);

    Atom actual;
    int format;
    unsigned long nitems, bytes;
    unsigned char* data = nullptr;
    Window w = None;

    if (Success == XGetWindowProperty(
            d, root, net_active, 0, 1, False, XA_WINDOW,
            &actual, &format, &nitems, &bytes, &data)) {
        if (data) {
            w = *reinterpret_cast<Window*>(data);
            XFree(data);
        }
    }

    if (w == None || w == root) {
        int revert;
        XGetInputFocus(d, &w, &revert);
        if (w == None) {
            XCloseDisplay(d);
            return "No active window";
        }
        w = getTopLevelParent(d, w);
    }

    Atom prop = XInternAtom(d, "_NET_WM_NAME", False);
    data = nullptr;
    std::string title;

    if (Success == XGetWindowProperty(
            d, w, prop, 0, (~0L), False, AnyPropertyType,
            &actual, &format, &nitems, &bytes, &data)) {
        if (data && nitems > 0) {
            title = reinterpret_cast<char*>(data);
            XFree(data);
            XCloseDisplay(d);
            return title;
        }
        if (data) XFree(data);
    }

    char* name = nullptr;
    if (XFetchName(d, w, &name) > 0 && name) {
        title = name;
        XFree(name);
        XCloseDisplay(d);
        return title;
    }
    if (name) XFree(name);

    XCloseDisplay(d);
    return "Unnamed window";
}
