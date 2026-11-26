// Calibration tables as non-owning multidimensional views (mdspan).
//
//   std::mdspan (C++23, GCC 14+)  ->  Kokkos reference mdspan (C++17)  ->  tiny built-in fallback
//
// Storage stays plain arrays (they map 1:1 onto TunerStudio pages); code only touches views.
// Index with cell(view, i, j): std::mdspan uses operator[], Kokkos in C++17 uses operator().
#pragma once
#include <cstddef>
#include <cstdint>

#if __has_include(<mdspan>) && __cplusplus > 202002L
  #include <mdspan>
  namespace tbl { using std::mdspan; using std::extents; }
  #define TBL_BRACKETS 1
#elif __has_include(<mdspan/mdspan.hpp>)
  #include <mdspan/mdspan.hpp>
  namespace tbl { using Kokkos::mdspan; using Kokkos::extents; }
  #define TBL_BRACKETS 0
#else
  // Minimal stand-in: static extents, layout_right, rank 1 or 2. Replace by uploading Kokkos mdspan.
  namespace tbl
  {
      template <class I, std::size_t... E> struct extents
      {
          static constexpr std::size_t rank() { return sizeof...(E); }
          static constexpr std::size_t extent(std::size_t r) { constexpr std::size_t e[] = { E... }; return e[r]; }
      };
      template <class T, class Ext> class mdspan
      {
          T* p_;
      public:
          using element_type = T;
          using extents_type = Ext;
          constexpr explicit mdspan(T* p) : p_(p) {}
          static constexpr std::size_t rank() { return Ext::rank(); }
          static constexpr std::size_t extent(std::size_t r) { return Ext::extent(r); }
          constexpr T* data_handle() const { return p_; }
          constexpr T& operator()(std::size_t i) const { return p_[i]; }
          constexpr T& operator()(std::size_t i, std::size_t j) const { return p_[i * Ext::extent(1) + j]; }
      };
  }
  #define TBL_BRACKETS 0
#endif

namespace tbl
{
    template <class V> constexpr auto& cell(const V& v, std::size_t i)                { return v.data_handle()[i]; }
    template <class V> constexpr auto& cell(const V& v, std::size_t i, std::size_t j)
    {
#if TBL_BRACKETS
        return v[i, j];
#else
        return v(i, j);
#endif
    }

    template <class T, std::size_t N>              using curve = mdspan<T, extents<std::size_t, N>>;
    template <class T, std::size_t R, std::size_t C> using map2d = mdspan<T, extents<std::size_t, R, C>>;

    // Position of x on an ascending axis: index of the lower bin and fraction (0..1) to the next.
    struct Pos { std::size_t i; float f; };
    template <class Axis> Pos locate(const Axis& axis, float x)
    {
        const std::size_t n = axis.extent(0);
        if (x <= float(cell(axis, 0)))     return { 0, 0.0f };
        if (x >= float(cell(axis, n - 1))) return { n - 2, 1.0f };
        std::size_t i = 0;
        while (i + 2 < n && x >= float(cell(axis, i + 1))) i++;
        float lo = float(cell(axis, i)), hi = float(cell(axis, i + 1));
        return { i, hi > lo ? (x - lo) / (hi - lo) : 0.0f };
    }

    template <class Curve, class Axis> float interp(const Curve& c, const Axis& axis, float x, float scale = 1.0f)
    {
        Pos p = locate(axis, x);
        return (float(cell(c, p.i)) * (1 - p.f) + float(cell(c, p.i + 1)) * p.f) * scale;
    }

    // Bilinear lookup. Table rows follow the y axis (e.g. MAP), columns the x axis (e.g. rpm).
    template <class Map, class AxisY, class AxisX>
    float interp(const Map& m, const AxisY& ay, const AxisX& ax, float y, float x, float scale = 1.0f)
    {
        Pos py = locate(ay, y), px = locate(ax, x);
        float a = float(cell(m, py.i, px.i)),     b = float(cell(m, py.i, px.i + 1));
        float c = float(cell(m, py.i + 1, px.i)), d = float(cell(m, py.i + 1, px.i + 1));
        return ((a * (1 - px.f) + b * px.f) * (1 - py.f) + (c * (1 - px.f) + d * px.f) * py.f) * scale;
    }
}
