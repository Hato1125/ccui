#include "widget.hh"
#include "scenes/scene.hh"

namespace ccui {
  void dispatch_context::focus() noexcept {
    owner.focuse(self);
  }
}
