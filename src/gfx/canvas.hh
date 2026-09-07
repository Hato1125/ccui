#ifndef _CCUI_GFX_CANVAS_HH
#define _CCUI_GFX_CANVAS_HH

#include <cstdint>
#include <optional>

#include <thorvg.h>

namespace ccui::gfx {
  struct color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;
  };

  class canvas {
  public:
    [[nodiscard]] static std::optional<canvas> create(
      void* gl_context,
      std::uint32_t width,
      std::uint32_t height
    );

    ~canvas();

    canvas(canvas&& other) noexcept;
    canvas& operator=(canvas&& other) noexcept;

    canvas(const canvas&) = delete;
    canvas& operator=(const canvas&) = delete;

    bool resize(std::uint32_t width, std::uint32_t height);

    bool begin();
    bool end();

    void rect(
      float x,
      float y,
      float w,
      float h,
      float radius,
      color c
    );

    void circle(
      float cx,
      float cy,
      float radius,
      color c
    );

    void triangle(
      float x1,
      float y1,
      float x2,
      float y2,
      float x3,
      float y3,
      color c
    );

  private:
    canvas(
      tvg::GlCanvas* tvg,
      void* ctx,
      std::uint32_t width,
      std::uint32_t height
    ) noexcept;

    tvg::GlCanvas* _tvg = nullptr;
    void* _ctx = nullptr;
    std::uint32_t _width = 0;
    std::uint32_t _height = 0;
  };
}

#endif
