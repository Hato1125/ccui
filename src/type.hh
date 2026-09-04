#ifndef _NEKO_TYPE_HH
#define _NEKO_TYPE_HH

namespace neko {
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
