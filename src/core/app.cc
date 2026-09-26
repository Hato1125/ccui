#include <cstdlib>
#include <vector>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>

#include "app.hh"

namespace ccui {
  int app::run(std::function<void(context&)> launching) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
      return EXIT_FAILURE;
    }

    _rebuild_event = SDL_RegisterEvents(1);
    if (_rebuild_event == 0) {
      return EXIT_FAILURE;
    }

    context cx(*this);

    if (launching) {
      launching(cx);
    }

    loop();

    return EXIT_SUCCESS;
  }

  void app::notify(bool& dirty) {
    SDL_Event ev {
      .user = {
        .type = _rebuild_event,
        .data1 = &dirty
      },
    };
    SDL_PushEvent(&ev);
  }

  void app::dispatch(SDL_Event& ev) {
    if (ev.type == _rebuild_event) {
      *static_cast<bool*>(ev.user.data1) = true;
      return;
    }

    for (const auto& scene : _scenes) {
      if (scene->has_id(ev)) {
        scene->handle_event(ev);
      }
    }

    std::erase_if(_scenes, [](const auto& scene) noexcept {
      return !scene->has();
    });
  }

  void app::loop() {
    SDL_Event ev;

    while (!_scenes.empty()) {
      if (!SDL_WaitEvent(&ev)) {
        return;
      }

      do {
        dispatch(ev);
      } while (SDL_PollEvent(&ev));

      for (const auto& scene : _scenes) {
        scene->frame();
      }
    }
  }
}
