#ifndef _CCUI_CORE_SCENES_APP_HH
#define _CCUI_CORE_SCENES_APP_HH

#include <cstdint>
#include <utility>
#include <vector>
#include <memory>
#include <functional>

#include <SDL3/SDL_events.h>

#include "widget.hh"
#include "scenes/scene.hh"

namespace ccui {
  class context;

  class app {
  public:
    int run(std::function<void(context&)> launching);

  private:
    friend class context;

    std::vector<std::unique_ptr<scene>> _scenes;
    std::uint32_t _rebuild_event = 0;

    template <template <stateful> class Sc, class... Args>
    void open(Args&&... args) {
      using S = std::remove_cvref_t<decltype((args, ...))>;
      if (auto s = Sc<S>::create(std::forward<Args>(args)...)) {
        _scenes.push_back(std::move(s));
      }
    }

    void notify(bool& dirty);
    void dispatch(SDL_Event& ev);
    void loop();
  };

  class context {
  public:
    template <template <stateful> class Sc, class... Args>
    void open(Args&&... args) {
      _app.open<Sc>(std::forward<Args>(args)...);
    }

    template <stateful S>
    void notify(mounted<S>& m) {
      _app.notify(m.dirty);
    }

  private:
    friend class app;

    context(app& app) : _app(app) {}

    app& _app;
  };
}

#endif
