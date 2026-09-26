#ifndef _CCUI_CORE_SCENES_WINDOW_HH
#define _CCUI_CORE_SCENES_WINDOW_HH

#include <cstdint>
#include <memory>
#include <utility>

#include <SDL3/SDL_video.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_properties.h>

#include "../event.hh"
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
      SDL_SetBooleanProperty(p, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);

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
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP: {
          mouse_press press {
            .button = static_cast<mouse_button>(ev.button.button),
            .down = ev.button.down,
            .x = ev.button.x,
            .y = ev.button.y,
          };

          set_hover(hover_within(_tree, press.x, press.y));
          dispatch_bubble(_tree, _hovered_id, press, *this);

          if (press.down && press.button == mouse_button::left) {
            auto hit = find_focusable(_tree, _hovered_id);
            if (hit.focus != 0) {
              set_focus(hit.focus);
            }
          }
          break;
        }
        case SDL_EVENT_MOUSE_WHEEL:
          set_hover(hover_within(_tree, ev.wheel.mouse_x, ev.wheel.mouse_y));
          dispatch_bubble(_tree, _hovered_id, mouse_wheel {
            .direction = static_cast<wheel_direction>(ev.wheel.direction),
            .x = ev.wheel.x,
            .y = ev.wheel.y,
          }, *this);
          break;
        case SDL_EVENT_MOUSE_MOTION:
          set_hover(hover_within(_tree, ev.motion.x, ev.motion.y));
          dispatch_bubble(_tree, _hovered_id, mouse_motion {
            .x = ev.motion.x,
            .y = ev.motion.y,
          }, *this);
          break;
        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
          set_hover(0);
          break;
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP: {
          key_press key {
            .key = ev.key.key,
            .mod = ev.key.mod,
            .down = ev.key.down,
            .repeat = ev.key.repeat,
          };

          auto disp = dispatch_to(_tree, _focused_id, key, *this);
          auto handled = _focused_id != 0 && disp;
          if (!handled && key.down && key.key == SDLK_TAB) {
            move_focus((key.mod & SDL_KMOD_SHIFT) != 0);
          } else if (!handled && key.down && key.key == SDLK_ESCAPE) {
            set_focus(0);
          }
          break;
        }
      }
      return true;
    }

    void frame() override {
      SDL_GL_MakeCurrent(_handle, _ctx);

      int pixel_width = 0;
      int pixel_height = 0;
      SDL_GetWindowSizeInPixels(_handle, &pixel_width, &pixel_height);
      _canvas->resize(pixel_width, pixel_height);
      _canvas->set_scale(SDL_GetWindowPixelDensity(_handle));

      int width = 0;
      int height = 0;
      SDL_GetWindowSize(_handle, &width, &height);
      auto w = static_cast<float>(width);
      auto h = static_cast<float>(height);

      rebuild(_tree, _focused_id, _hovered_id);
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

    void focuse(node_id node) noexcept override {
      set_focus(node);
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
        _tree(mount(std::move(root))) {
      auto range = link_focus(_tree);
      _focus_first = range.first;
      _focus_last = range.last;
    }

    void move_focus(bool backward) noexcept {
      auto link = find_focus_link(_tree, _focused_id);
      auto target = backward ? link.prev : link.next;
      if (target == 0) {
        target = backward ? _focus_last : _focus_first;
      }
      set_focus(target);
    }

    void set_focus(node_id id) noexcept {
      if (id == _focused_id) {
        return;
      }
      mark_context_dirty(_tree, _focused_id, id);
      _focused_id = id;
    }

    void set_hover(node_id id) noexcept {
      if (id == _hovered_id) {
        return;
      }
      mark_context_dirty(_tree, _hovered_id, id);
      _hovered_id = id;
    }

    SDL_Window* _handle = nullptr;
    SDL_GLContext _ctx = nullptr;

    std::optional<gfx::canvas> _canvas;

    node<mounted<S>> _tree;
    node_id _hovered_id = 0;
    node_id _focused_id = 0;
    node_id _focus_first = 0;
    node_id _focus_last = 0;
  };
}

#endif
