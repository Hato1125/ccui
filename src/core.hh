#ifndef _CCUI_CORE_HH
#define _CCUI_CORE_HH

#include <tuple>
#include <utility>
#include <type_traits>

#include "type.hh"

namespace ccui {
  template <class T>
  concept stateless = requires(
    const T& t,
    const constraint& c,
    const point& o,
    const extent& e
  ) {
    { t.measure(c) } -> std::same_as<extent>;
    { t.layout(o) };
    { t.paint(o, e) };
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
  };


  template <class W>
  extent measure(node<W>& n, const constraint& c) {
    if constexpr (stateless<W>) {
      n.size = n.widget.measure(c);
    } else if constexpr (container<W>) {
      n.size = n.widget.measure(c);
    } else if constexpr (mounted_widget<W>) {
      n.size = measure(n.widget.tree,  c);
    }
    return n.size;
  }

  template <class W>
  void layout(node<W>& n, const point& o) {
    n.offset = o;

    if constexpr (stateless<W>) {
      n.widget.layout(o);
    } else if constexpr (container<W>) {
      n.widget.layout(o);
    } else if constexpr (mounted_widget<W>) {
      layout(n.widget.tree, o);
    }
  }

  template <class W>
  void paint(node<W>& n) {
    if constexpr (stateless<W>) {
      n.widget.paint(n.offset, n.size);
    } else if constexpr (container<W>) {
      n.widget.paint(n.offset, n.size);
    } else if constexpr (mounted_widget<W>) {
      paint(n.widget.tree);
    }
  }
}

#endif
