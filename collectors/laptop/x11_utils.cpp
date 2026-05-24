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
// tries the legacy WM_NAME atom first, then falls back to the UTF8 _NET_WM_NAME atom.
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

    Window w;
    int revert;
    XGetInputFocus(d, &w, &revert);

    if (w == None) {
        XCloseDisplay(d);
        return "No active window";
    }

    w = getTopLevelParent(d, w);

    char* name = nullptr;
    int status = XFetchName(d, w, &name);

    if (status > 0 && name) {
        std::string title(name);
        XFree(name);
        XCloseDisplay(d);
        return title;
    }
    if (name) XFree(name);

    Atom utf8 = XInternAtom(d, "UTF8_STRING", True);
    Atom prop = XInternAtom(d, "_NET_WM_NAME", True);

    Atom actual;
    int format;
    unsigned long nitems, bytes;
    unsigned char* data = nullptr;
    std::string title = "Unnamed window";

    if (Success == XGetWindowProperty(
            d, w, prop, 0, (~0L), False, utf8,
            &actual, &format, &nitems, &bytes, &data)) {
        if (data) {
            title = reinterpret_cast<char*>(data);
            XFree(data);
        }
    }

    XCloseDisplay(d);
    return title;
}
