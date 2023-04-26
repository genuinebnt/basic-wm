extern "C" {
#include <X11/Xlib.h>
}
#include <memory>

class WindowManager {
public:
  static ::std::unique_ptr<WindowManager> Create();

  ~WindowManager();

  void Run();

private:
  WindowManager(Display* display);

  Display* display_;

  const Window root_;
};