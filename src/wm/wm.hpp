extern "C" {
#include <X11/Xlib.h>
}
#include <memory>
#include <unordered_map>

class WindowManager {
public:
  static ::std::unique_ptr<WindowManager> Create();

  ~WindowManager();

  void Run();

private:
  WindowManager(Display *display);

  Display *display_;

  const Window root_;

  static inline bool wm_detected_ = false;

  std::unordered_map<Window, Window> clients_;

  static int OnXError(Display *display, XErrorEvent *e);

  static int OnWMDectected(Display *display, XErrorEvent *e);

  void OnCreateNotify(const XCreateWindowEvent &e);

  void OnDestroyNotify(const XDestroyWindowEvent &e);

  void OnConfigureRequest(const XConfigureRequestEvent &e);

  void OnConfigureNotify(const XConfigureEvent &e);

  void OnMapRequest(const XMapRequestEvent &e);

  void OnMapNotify(const XMapEvent &e);

  void OnReparentNotify(const XReparentEvent &e);

  void Frame(Window w);
};