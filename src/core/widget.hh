#ifndef _CCUI_CORE_WIDGET_HH
#define _CCUI_CORE_WIDGET_HH

#include <tuple>
#include <ranges>
#include <utility>
#include <type_traits>

#include "core/geometry.hh"
#include "gfx/canvas.hh"

namespace ccui {
  template <class T>
  concept stateless = requires(
    const T& t,
    const constraint& c,
    const point& o,
    const extent& e,
    gfx::canvas& cv
  ) {
    { t.measure(c) } -> std::same_as<extent>;
    { t.layout(o) };
    { t.paint(o, e, cv) };
  };

  template <class T>
  concept stateful = requires(const T& t) {
    { t.body() };
  };

  template <class T>
  concept container = requires(const T& t) {
    t.children;

    typename std::tuple_size<
      std::remove_cvref_t<decltype(t.children)>
    >::type;
  };

  template <class T, class E>
  concept dispatchable = requires(const T& t, const E& ev) {
    { t.dispatch(ev) } -> std::same_as<bool>;
  };

  template <stateful S>
  struct mounted;

  template <class>
  inline constexpr bool is_mounted = false;

  template <stateful S>
  inline constexpr bool is_mounted<mounted<S>> = true;

  template <class W>
  concept mounted_widget = is_mounted<std::remove_cvref_t<W>>;

  template <class W>
  struct node {
    W widget;
    extent size;
    point offset;
  };

  template <template <class...> class C, class... Cs>
    requires container<C<Cs...>>
  auto mount(C<Cs...> c);

  auto mount(stateless auto s) {
    return node{s};
  }

  auto mount(stateful auto s) {
    auto tree = mount(s.body());
    return node {
      mounted {
        std::move(s),
        std::move(tree)
      }
    };
  }

  template <template <class...> class C, class... Cs>
    requires container<C<Cs...>>
  auto mount(C<Cs...> c) {
    return node {
      std::apply(
        [](auto... xs) {
          return C{ mount(std::move(xs))... };
        },
        std::move(c.children)
      )
    };
  }

  template <stateful S>
  struct mounted {
    S self;
    decltype(mount(self.body())) tree;
    bool dirty;
  };

  template <class W, class V>
  void apply(node<W>& n, V&& next) {
    if constexpr (container<W>) {
      constexpr auto count =
        std::tuple_size_v<std::remove_cvref_t<decltype(next.children)>>;

      template for (constexpr auto i : std::views::iota(0uz, count)) {
        apply(
          std::get<i>(n.widget.children),
          std::get<i>(next.children)
        );
      }
    } else if constexpr (stateless<W>) {
      n.widget = std::forward<V>(next);
    }
  }

  template <class W>
  void rebuild(node<W>& n) {
    if constexpr (mounted_widget<W>) {
      if (n.widget.dirty) {
        apply(
          n.widget.tree,
          n.widget.self.body()
        );
        n.widget.dirty = false;
      }
      rebuild(n.widget.tree);
    } else if constexpr (container<W>) {
      template for (auto& child : n.widget.children) {
        rebuild(child);
      }
    }
  }

  template <class W, class E>
  bool dispatch(node<W>& n, const E& ev) {
    if constexpr (mounted_widget<W>) {
      return dispatch(n.widget.tree, ev);
    } else if constexpr (dispatchable<W, E>) {
      return n.widget.dispatch(ev);
    }

    return false;
  }
}

#endif
