#include "x11_utils.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <iostream>

// Helper function to get the top-level parent window
Window getTopLevelParent(Display* d, Window w) {
    Window root, parent;
    Window* children = nullptr;
    unsigned int nchildren;
    
    while (true) {
        if (XQueryTree(d, w, &root, &parent, &children, &nchildren) == 0) {
            return w;  // Query failed, return current window
        }
        
        if (children) {
            XFree(children);
        }
        
        // If parent is root or doesn't exist, we've reached the top
        if (parent == root || parent == None) {
            return w;
        }
        
        // Move up to parent
        w = parent;
    }
}

std::string getActiveWindowTitle() {

    XInitThreads();

    // check DISPLAY variablle is set 
    const char* display_var = std::getenv("DISPLAY");
    if (!display_var) {
        std::cerr << "[X11] DISPLAY environment variable not set\n";
        return "Unknown";
    }
    std::cerr << "[X11] DISPLAY=" << display_var << "\n";

    Display* d = XOpenDisplay(nullptr);
    if (!d) {
        std::cerr << "[X11] Cannot open display: " << display_var << "\n";
        return "Unknown";
    }

    Window w;
    int revert;
    XGetInputFocus(d, &w, &revert);
    
    if (w == None) {
        XCloseDisplay(d);
        return "No active window";
    }
    
    // Get the top-level parent window (terminals and some apps focus child widgets)
    w = getTopLevelParent(d, w);

    // Try legacy WM_NAME
    char* name = nullptr;
    int status = XFetchName(d, w, &name);
    
    if (status > 0 && name) {
        std::string title(name);
        XFree(name);
        XCloseDisplay(d);
        return title;
    }
    if (name) XFree(name);  // Free if allocated but status was 0

    // Fallback: modern UTF8 _NET_WM_NAME
    Atom utf8  = XInternAtom(d, "UTF8_STRING", True);
    Atom prop  = XInternAtom(d, "_NET_WM_NAME", True);
    
    Atom actual; int format;
    unsigned long nitems, bytes; unsigned char* data = nullptr;
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
