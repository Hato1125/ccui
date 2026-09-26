#ifndef _CCUI_CORE_WIDGET_HH
#define _CCUI_CORE_WIDGET_HH

#include <tuple>
#include <ranges>
#include <utility>
#include <cstdint>
#include <type_traits>

#include "core/event.hh"
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

  using node_id = std::uint32_t;

  struct build_context {
    node_id self = 0;
    node_id last = 0;
    node_id focused = 0;

    [[nodiscard]] bool focused_within() const noexcept {
      return self <= focused && focused <= last;
    }
  };

  template <class T>
  concept context_aware = requires(const T& t, const build_context& cx) {
    { t.body(cx) };
  };

  template <class T>
  concept stateful = context_aware<T> || requires(const T& t) {
    { t.body() };
  };

  template <stateful S>
  auto build_body(const S& s, const build_context& cx) {
    if constexpr (context_aware<S>) {
      return s.body(cx);
    } else {
      return s.body();
    }
  }

  template <class T>
  concept container = requires(const T& t) {
    t.children;

    typename std::tuple_size<
      std::remove_cvref_t<decltype(t.children)>
    >::type;
  };

  class scene;
  struct dispatch_context {
    scene& owner;
    node_id self;

    void focus() noexcept;
  };

  template <class T, class E>
  concept dispatchable = requires(
    const T& t,
    const E& ev,
    dispatch_context& ctx
  ) {
    { t.dispatch(ev, ctx) } -> std::same_as<bool>;
  };

  template <class W, class Events>
  inline constexpr bool dispatchable_any = false;

  template <class W, class... Es>
  inline constexpr bool dispatchable_any<W, std::tuple<Es...>> =
    (dispatchable<W, Es> || ...);

  template <class W>
  concept focusable = dispatchable_any<W, focus_events>;

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
    node_id id = 0;
    node_id next = 0;
    node_id prev = 0;
    point offset;
    extent size;
    W widget;
  };

  inline node_id next_node_id = 0;

  template <template <class...> class C, class... Cs>
    requires container<C<Cs...>>
  auto mount(C<Cs...> c);

  auto mount(stateless auto s) {
    next_node_id++;

    return node {
      .id = next_node_id,
      .widget = s,
    };
  }

  auto mount(stateful auto s) {
    auto id = ++next_node_id;
    auto tree = mount(build_body(s, { .self = id }));
    return node {
      .id = id,
      .widget = mounted {
        std::move(s),
        std::move(tree),
        next_node_id,
      }
    };
  }

  template <template <class...> class C, class... Cs>
    requires container<C<Cs...>>
  auto mount(C<Cs...> c) {
    next_node_id++;

    return node {
      .id = next_node_id,
      .widget = std::apply(
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
    decltype(mount(build_body(self, {}))) tree;
    node_id last = 0;
    bool dirty = false;
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
  void rebuild(node<W>& n, node_id focused = 0) {
    if constexpr (mounted_widget<W>) {
      if (n.widget.dirty) {
        apply(
          n.widget.tree,
          build_body(n.widget.self, {
            .self = n.id,
            .last = n.widget.last,
            .focused = focused,
          })
        );
        n.widget.dirty = false;
      }
      rebuild(n.widget.tree, focused);
    } else if constexpr (container<W>) {
      template for (auto& child : n.widget.children) {
        rebuild(child, focused);
      }
    }
  }

  template <class W>
  void mark_focus_dirty(node<W>& n, node_id from, node_id to) {
    if constexpr (mounted_widget<W>) {
      auto contains = [&](node_id id) {
        return n.id <= id && id <= n.widget.last;
      };
      if (!contains(from) && !contains(to)) {
        return;
      }

      using S = decltype(n.widget.self);
      if constexpr (context_aware<S>) {
        n.widget.dirty = true;
      }
      mark_focus_dirty(n.widget.tree, from, to);
    } else if constexpr (container<W>) {
      template for (auto& child : n.widget.children) {
        mark_focus_dirty(child, from, to);
      }
    }
  }

  template <class W, class E>
  bool dispatch(node<W>& n, const E& ev, scene& owner) {
    if constexpr (mounted_widget<W>) {
      return dispatch(n.widget.tree, ev, owner);
    } else if constexpr (dispatchable<W, E>) {
      return n.widget.dispatch(ev, { owner, n.id });
    }

    return false;
  }

  template <class W, class E>
  bool dispatch_to(node<W>& n, node_id target, const E& ev, scene& owner) {
    if (n.id == target) {
      if constexpr (dispatchable<W, E>) {
        return n.widget.dispatch(ev, { owner, n.id });
      }
      return false;
    }

    if constexpr (mounted_widget<W>) {
      return dispatch_to(n.widget.tree, target, ev, owner);
    } else if constexpr (container<W>) {
      template for (auto& child : n.widget.children) {
        if (dispatch_to(child, target, ev, owner)) {
          return true;
        }
      }
    }

    return false;
  }

  struct focus_chain {
    node_id first = 0;
    node_id* first_prev = nullptr;

    node_id last = 0;
    node_id* last_next = nullptr;
  };

  template <class W>
  void link_focus(node<W>& n, focus_chain& chain) {
    if constexpr (focusable<W>) {
      if (chain.last_next) {
        *chain.last_next = n.id;
        n.prev = chain.last;
      } else {
        chain.first = n.id;
        chain.first_prev = &n.prev;
      }
      chain.last = n.id;
      chain.last_next = &n.next;
    }

    if constexpr (mounted_widget<W>) {
      link_focus(n.widget.tree, chain);
    } else if constexpr (container<W>) {
      template for (auto& child : n.widget.children) {
        link_focus(child, chain);
      }
    }
  }

  struct focus_range {
    node_id first = 0;
    node_id last = 0;
  };

  template <class W>
  focus_range link_focus(node<W>& root) {
    focus_chain chain;
    link_focus(root, chain);

    if (chain.last_next) {
      *chain.last_next = chain.first;
      *chain.first_prev = chain.last;
    }
    return { chain.first, chain.last };
  }

  struct focus_link {
    node_id next = 0;
    node_id prev = 0;
  };

  template <class W>
  focus_link find_focus_link(node<W>& n, node_id target) {
    if (n.id == target) {
      return { n.next, n.prev };
    }

    if constexpr (mounted_widget<W>) {
      return find_focus_link(n.widget.tree, target);
    } else if constexpr (container<W>) {
      template for (auto& child : n.widget.children) {
        auto link = find_focus_link(child, target);
        if (link.next != 0) {
          return link;
        }
      }
    }

    return {};
  }
}

#endif
