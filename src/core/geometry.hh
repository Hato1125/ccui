#ifndef _CCUI_CORE_GEOMETRY_HH
#define _CCUI_CORE_GEOMETRY_HH

namespace ccui {
  struct extent {
    float width = 0.0f;
    float height = 0.0f;
  };

  struct point {
    float x = 0.0f;
    float y = 0.0f;
  };

  struct constraint {
    extent min;
    extent max;
  };
}

#endif
