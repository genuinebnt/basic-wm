#include "wm.hpp"
#include <glog/logging.h>

using ::std::unique_ptr;

unique_ptr<WindowManager> WindowManager::Create() {
  Display *display = XOpenDisplay(nullptr);
  if (display == nullptr) {
    LOG(ERROR) << "Failed to open display " << XDisplayName(nullptr);
    return nullptr;
  }

  return unique_ptr<WindowManager>(new WindowManager(display));
}

WindowManager::WindowManager(Display *display)
    : display_(CHECK_NOTNULL(display)), root_(DefaultRootWindow(display_)) {}

WindowManager::~WindowManager() { XCloseDisplay(display_); }

void WindowManager::Run() {
  XSetErrorHandler(&WindowManager::OnWMDectected);
  XSelectInput(display_, root_,
               SubstructureRedirectMask | SubstructureNotifyMask);
  XSync(display_, false);

  if (wm_detected_) {
    LOG(ERROR) << "Detected another window manager running on display "
               << XDisplayString(display_);
    return;
  }

  XSetErrorHandler(&WindowManager::OnXError);

  XGrabServer(display_);

  Window returned_root, returned_parent;
  Window *top_level_windows;
  unsigned int num_top_level_windows;
  CHECK(XQueryTree(display_, root_, &returned_root, &returned_parent,
                   &top_level_windows, &num_top_level_windows));
  CHECK_EQ(returned_root, root_);

  for (unsigned int i = 0; i < num_top_level_windows; ++i) {
    Frame(top_level_windows[i], true);
  }

  XFree(top_level_windows);
  XUngrabServer(display_);

  for (;;) {
    XEvent e;
    XNextEvent(display_, &e);

    switch (e.type) {
    case CreateNotify:
      OnCreateNotify(e.xcreatewindow);
      break;
    case DestroyNotify:
      OnDestroyNotify(e.xdestroywindow);
      break;
    case ReparentNotify:
      OnReparentNotify(e.xreparent);
      break;
    case ConfigureRequest:
      OnConfigureRequest(e.xconfigurerequest);
      break;
    case ConfigureNotify:
      OnConfigureNotify(e.xconfigure);
      break;
    case MapRequest:
      OnMapRequest(e.xmaprequest);
      break;
    case MapNotify:
      OnMapNotify(e.xmap);
    case UnmapNotify:
      OnUnmapNotify(e.xunmap);
      break;
    default:
      LOG(WARNING) << "Ignored event";
    }
  }
}

void WindowManager::OnCreateNotify(const XCreateWindowEvent &e) {}

void WindowManager::OnDestroyNotify(const XDestroyWindowEvent &e) {}

void WindowManager::OnConfigureRequest(const XConfigureRequestEvent &e) {
  XWindowChanges changes;
  changes.x = e.x;
  changes.y = e.y;
  changes.width = e.width;
  changes.height = e.height;
  changes.border_width = e.border_width;
  changes.sibling = e.above;
  changes.stack_mode = e.detail;

  if (clients_.count(e.window)) {
    const Window frame = clients_[e.window];
    XConfigureWindow(display_, frame, e.value_mask, &changes);
    LOG(INFO) << "Resize [" << frame << "] to width: " << e.width
              << " and height: " << e.height;
  }

  XConfigureWindow(display_, root_, e.value_mask, &changes);
  LOG(INFO) << "Resize " << e.window << " to width: " << e.width
            << " and height: " << e.height;
}

void WindowManager::OnConfigureNotify(const XConfigureEvent &e) {}

void WindowManager::OnMapRequest(const XMapRequestEvent &e) {
  Frame(e.window, false);

  XMapWindow(display_, e.window);
}

void WindowManager::Frame(Window w, bool was_created_before_window_manager) {
  const unsigned int BORDER_WIDTH = 3;
  const unsigned long BORDER_COLOR = 0xff0000;
  const unsigned long BG_COLOR = 0x0000ff;

  XWindowAttributes x_window_attrs;
  XGetWindowAttributes(display_, w, &x_window_attrs);

  if (was_created_before_window_manager) {
    if (x_window_attrs.override_redirect ||
        x_window_attrs.map_state != IsViewable) {
      return;
    }
  }

  const Window frame = XCreateSimpleWindow(
      display_, root_, x_window_attrs.x, x_window_attrs.y, x_window_attrs.width,
      x_window_attrs.height, BORDER_WIDTH, BORDER_COLOR, BG_COLOR);

  XSelectInput(display_, frame,
               SubstructureRedirectMask | SubstructureNotifyMask);

  XAddToSaveSet(display_, w);
  XReparentWindow(display_, w, frame, 0, 0);
  XMapWindow(display_, frame);

  clients_[w] = frame;

  LOG(INFO) << "Framed window " << w << " [" << frame << "]";
}

void WindowManager::OnMapNotify(const XMapEvent &e) {}

void WindowManager::OnUnmapNotify(const XUnmapEvent &e) {
  if (!clients_.count(e.window)) {
    LOG(INFO) << "Ignore unmap notify for non-client window " << e.window;
    return;
  }

  if (e.event == root_) {
    LOG(INFO) << "Ignore UnmapNotify for reparented pre-existing window "
              << e.window;
    return;
  }

  Unframe(e.window);
}

void WindowManager::Unframe(Window w) {
  const Window frame = clients_[w];

  XUnmapWindow(display_, frame);
  XReparentWindow(display_, w, root_, 0, 0);
  XRemoveFromSaveSet(display_, w);
  XDestroyWindow(display_, frame);

  clients_.erase(w);

  LOG(INFO) << "Unframed Window " << w << " [" << frame << "]";
}

void WindowManager::OnReparentNotify(const XReparentEvent &e) {}

int WindowManager::OnWMDectected(Display *display, XErrorEvent *e) {
  CHECK_EQ(static_cast<int>(e->error_code), BadAccess);
  wm_detected_ = true;

  return 0;
}

int WindowManager::OnXError(Display *display, XErrorEvent *e) {
  const int MAX_ERROR_TEXT_LENGTH = 1024;
  char error_text[MAX_ERROR_TEXT_LENGTH];
  XGetErrorText(display, e->error_code, error_text, sizeof(error_text));
  LOG(ERROR) << static_cast<int>(e->error_code) << ":" << error_text;

  return 0;
}