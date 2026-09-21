#ifndef _CCUI_CORE_SCENES_SCENE_HH
#define _CCUI_CORE_SCENES_SCENE_HH

#include <SDL3/SDL_events.h>

namespace ccui {
  class scene {
  public:
    virtual ~scene() = default;

    virtual bool handle_event(SDL_Event& ev) = 0;
    virtual void frame() = 0;
    virtual void destroy() = 0;

    [[nodiscard]] virtual bool has_id(const SDL_Event& ev) const noexcept = 0;
    [[nodiscard]] virtual bool has() const noexcept = 0;
  };
}

#endif
