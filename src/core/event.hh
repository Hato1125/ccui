#ifndef _CCUI_CORE_EVENT_HH
#define _CCUI_CORE_EVENT_HH

#include <cstdint>
#include <tuple>

namespace ccui {
  enum class mouse_button {
    left = 1,
    middle,
    right,
    x1,
    x2,
  };

  enum class wheel_direction {
    normal,
    flipped,
  };

  struct mouse_press {
    mouse_button button;
    bool down;
    float x = 0.0f;
    float y = 0.0f;
  };

  struct mouse_wheel {
    wheel_direction direction;
    float x = 0.0f;
    float y = 0.0f;
  };

  struct mouse_motion {
    float x = 0.0f;
    float y = 0.0f;
  };

  struct key_press {
    std::uint32_t key = 0;
    std::uint16_t mod = 0;
    bool down = false;
    bool repeat = false;
  };

  using focus_events = std::tuple<key_press>;
}

#endif
