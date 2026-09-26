#include "gfx/canvas.hh"

namespace ccui::gfx {
  namespace {
    bool retarget(
      tvg::GlCanvas* tvg_canvas,
      void* ctx,
      std::uint32_t width,
      std::uint32_t height
    ) {
      tvg_canvas->sync();

      return tvg_canvas->target(
        nullptr,
        nullptr,
        ctx,
        0,
        width,
        height,
        tvg::ColorSpace::ABGR8888S
      ) == tvg::Result::Success;
    }
  }

  std::optional<canvas> canvas::create(
    void* gl_context,
    std::uint32_t width,
    std::uint32_t height
  ) {
    if (!gl_context || width == 0 || height == 0) {
      return std::nullopt;
    }

    if (tvg::Initializer::init(0) != tvg::Result::Success) {
      return std::nullopt;
    }

    auto* tvg_canvas = tvg::GlCanvas::gen();
    if (!tvg_canvas) {
      tvg::Initializer::term();
      return std::nullopt;
    }

    if (!retarget(tvg_canvas, gl_context, width, height)) {
      delete tvg_canvas;
      tvg::Initializer::term();
      return std::nullopt;
    }

    return canvas {
      tvg_canvas,
      gl_context,
      width,
      height,
    };
  }

  canvas::canvas(
    tvg::GlCanvas* tvg,
    void* ctx,
    std::uint32_t width,
    std::uint32_t height
  ) noexcept
    : _tvg(tvg), _ctx(ctx), _width(width), _height(height) {}

  canvas::canvas(canvas&& other) noexcept
    : _tvg(other._tvg),
      _ctx(other._ctx),
      _width(other._width),
      _height(other._height),
      _scale(other._scale) {
    other._tvg = nullptr;
  }

  canvas& canvas::operator=(canvas&& other) noexcept {
    if (this == &other) {
      return *this;
    }

    this->~canvas();

    _tvg = other._tvg;
    _ctx = other._ctx;
    _width = other._width;
    _height = other._height;
    _scale = other._scale;
    other._tvg = nullptr;

    return *this;
  }

  canvas::~canvas() {
    if (!_tvg) {
      return;
    }

    delete _tvg;
    _tvg = nullptr;

    tvg::Initializer::term();
  }

  bool canvas::resize(std::uint32_t width, std::uint32_t height) {
    if (width == 0 || height == 0) {
      return false;
    }
    if (width == _width && height == _height) {
      return true;
    }

    if (!retarget(_tvg, _ctx, width, height)) {
      return false;
    }

    _width = width;
    _height = height;
    return true;
  }

  void canvas::set_scale(float factor) noexcept {
    _scale = factor;
  }

  bool canvas::begin() {
    return _tvg->remove() == tvg::Result::Success;
  }

  bool canvas::end() {
    auto ok = _tvg->update() == tvg::Result::Success
      && _tvg->draw(true) == tvg::Result::Success
      && _tvg->sync() == tvg::Result::Success;

    return ok;
  }

  void canvas::rect(
    float x,
    float y,
    float w,
    float h,
    float radius,
    color c
  ) {
    auto* shape = tvg::Shape::gen();
    shape->appendRect(x, y, w, h, radius, radius);
    shape->fill(c.r, c.g, c.b, c.a);
    shape->scale(_scale);

    _tvg->add(shape);
  }

  void canvas::circle(
    float cx,
    float cy,
    float radius,
    color c
  ) {
    auto* shape = tvg::Shape::gen();
    shape->appendCircle(cx, cy, radius, radius);
    shape->fill(c.r, c.g, c.b, c.a);
    shape->scale(_scale);

    _tvg->add(shape);
  }

  void canvas::triangle(
    float x1,
    float y1,
    float x2,
    float y2,
    float x3,
    float y3,
    color c
  ) {
    auto* shape = tvg::Shape::gen();
    shape->moveTo(x1, y1);
    shape->lineTo(x2, y2);
    shape->lineTo(x3, y3);
    shape->close();
    shape->fill(c.r, c.g, c.b, c.a);
    shape->scale(_scale);

    _tvg->add(shape);
  }
}
