#ifndef _CCUI_CORE_EVENT_HH
#define _CCUI_CORE_EVENT_HH

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
}

#endif
