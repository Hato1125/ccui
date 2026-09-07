#ifndef _CCUI_CORE_TRAVERSE_HH
#define _CCUI_CORE_TRAVERSE_HH

#include "core/geometry.hh"
#include "core/widget.hh"

namespace ccui {
  template <class W>
  extent measure(node<W>& n, const constraint& c) {
    if constexpr (stateless<W>) {
      n.size = n.widget.measure(c);
    } else if constexpr (container<W>) {
      n.size = n.widget.measure(c);
    } else if constexpr (mounted_widget<W>) {
      n.size = measure(n.widget.tree, c);
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
