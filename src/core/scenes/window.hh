#ifndef _CCUI_CORE_SCENES_WINDOW_HH
#define _CCUI_CORE_SCENES_WINDOW_HH

#include <cstdint>
#include <memory>
#include <utility>

#include <SDL3/SDL_video.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_properties.h>

#include "scene.hh"
#include "core/widget.hh"
#include "core/traverse.hh"
#include "gfx/canvas.hh"

namespace ccui {
  struct window_opts {
    const char* title = nullptr;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
  };

  template <stateful S>
  class window : public scene {
  public:
    [[nodiscard]] static std::unique_ptr<window> create(window_opts opts, S root) {
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

      auto p = SDL_CreateProperties();
      SDL_SetStringProperty(p, SDL_PROP_WINDOW_CREATE_TITLE_STRING, opts.title);
      SDL_SetNumberProperty(p, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, opts.width);
      SDL_SetNumberProperty(p, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, opts.height);
      SDL_SetBooleanProperty(p, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);

      auto* handle = SDL_CreateWindowWithProperties(p);
      SDL_DestroyProperties(p);

      if (!handle) {
        return nullptr;
      }

      auto ctx = SDL_GL_CreateContext(handle);
      if (!ctx) {
        SDL_DestroyWindow(handle);
        return nullptr;
      }

      SDL_GL_MakeCurrent(handle, ctx);

      int width = 0;
      int height = 0;
      SDL_GetWindowSizeInPixels(handle, &width, &height);

      auto canvas = gfx::canvas::create(
        ctx,
        static_cast<std::uint32_t>(width),
        static_cast<std::uint32_t>(height)
      );

      if (!canvas) {
        SDL_GL_DestroyContext(ctx);
        SDL_DestroyWindow(handle);
        return nullptr;
      }

      return std::unique_ptr<window>(
        new window(handle, ctx, std::move(*canvas), std::move(root))
      );
    }

    ~window() override {
      destroy();
    }

    window(const window&) = delete;
    window& operator=(const window&) = delete;

    bool handle_event(SDL_Event& ev) override {
      switch (ev.type) {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
          destroy();
          break;
      }
      return true;
    }

    void frame() override {
      SDL_GL_MakeCurrent(_handle, _ctx);

      int width = 0;
      int height = 0;
      SDL_GetWindowSizeInPixels(_handle, &width, &height);
      auto w = static_cast<float>(width);
      auto h = static_cast<float>(height);

      _canvas->resize(w, h);

      rebuild(_tree);
      measure(_tree, {
        .min = { 0.0f, 0.0f },
        .max = { w, h },
      });
      layout(_tree, { 0.0f, 0.0f });
      _canvas->begin();
      paint(_tree, *_canvas);
      _canvas->end();

      SDL_GL_SwapWindow(_handle);
    }

    void destroy() override {
      _canvas.reset();

      if (_ctx) {
        SDL_GL_DestroyContext(_ctx);
        _ctx = nullptr;
      }
      if (_handle) {
        SDL_DestroyWindow(_handle);
        _handle = nullptr;
      }
    }

    [[nodiscard]] bool has_id(const SDL_Event& ev) const noexcept override {
      return SDL_GetWindowFromEvent(&ev) == _handle;
    }

    [[nodiscard]] bool has() const noexcept override {
      return _handle;
    }

  private:
    window(
      SDL_Window* handle,
      SDL_GLContext ctx,
      gfx::canvas canvas,
      S root
    )
      : _handle(handle),
        _ctx(ctx),
        _canvas(std::move(canvas)),
        _tree(mount(std::move(root))) {}

    SDL_Window* _handle = nullptr;
    SDL_GLContext _ctx = nullptr;

    std::optional<gfx::canvas> _canvas;

    node<mounted<S>> _tree;
  };
}

#endif
