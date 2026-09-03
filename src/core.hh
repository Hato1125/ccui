#ifndef _NEKO_CORE_HH
#define _NEKO_CORE_HH

#include <tuple>
#include <utility>
#include <type_traits>

namespace neko {
  template <class T>
  concept stateless = requires(const T& t) {
    { t.measure() };
    { t.layout() };
    { t.paint() };
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

  template <template <class...> class C, class... Cs>
    requires container<C<Cs...>>
  auto mount(C<Cs...> c);

  auto mount(stateless auto s) {
    return s;
  }

  auto mount(stateful auto s) {
    auto tree = mount(s.body());
    return mounted {
      std::move(s),
      std::move(tree)
    };
  }

  template <template <class...> class C, class... Cs>
    requires container<C<Cs...>>
  auto mount(C<Cs...> c) {
    return std::apply(
      [](auto... xs) {
        return C{ mount(std::move(xs))... };
      },
      std::move(c.children)
    );
  }

  template <stateful S>
  struct mounted {
    S self;
    decltype(mount(self.body())) tree;
  };

  auto measure(stateless auto& s) {
    s.measure();
  }

  auto layout(stateless auto& s) {
    s.layout();
  }

  auto paint(stateless auto& s) {
    s.paint();
  }

  auto measure(container auto& c) {
    template for (auto& ch : c.children) {
      measure(ch);
    }
  }

  auto layout(container auto& c) {
    template for (auto& ch : c.children) {
      layout(ch);
    }
  }

  auto paint(container auto& c) {
    template for (auto& ch : c.children) {
      paint(ch);
    }
  }

  template <stateful S>
  auto measure(mounted<S>& m) {
    measure(m.tree);
  }

  template <stateful S>
  auto layout(mounted<S>& m) {
    layout(m.tree);
  }

  template <stateful S>
  auto paint(mounted<S>& m) {
    paint(m.tree);
  }
}

#endif
