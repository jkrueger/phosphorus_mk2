#pragma once

namespace accel {
  namespace triangle {
    /* Watertight ray triangle intersection algorithm due to
       Ingo Wald/2013 */
    template<int N>
    struct watertight_t {
      soa::vector3_t<N> _a, _b, _c;

      uint32_t num;

      uint32_t meshid[N];
      uint32_t faceid[N];

      inline watertight_t(const triangle_t** triangles, uint32_t num) {
        : num(num)
        {
          for (auto i=0; i<num; ++i) {
            const auto& a = triangles[i]->a();
            const auto& b = triangles[i]->b();
            const auto& c = triangles[i]->c();

            _a.x[i] = a.x; _a.y[i] = a.y; _a.z[i] = a.z;
            _b.x[i] = b.x; _b.y[i] = b.y; _b.z[i] = b.z;
            _c.x[i] = c.x; _c.y[i] = c.y; _c.z[i] = c.z;

            const auto mesh = triangles[i]->meshid();
            const auto mat  = triangles[i]->matid();

            meshid[i] = mesh | (mat << 16);
            faceid[i] = triangles[i]->face;
          }
        }
      }

      /* The baseline implementation does a non simd intersection 
       * test, that exists primarily for debugging purposes, and
       * performance comparison to simd code */
      template<typename T>
      inline void baseline(
        T* stream
      , uint32_t* indices
      , uint32_t num_rays) const
      {
        for (auto i=0; i<num_rays; ++i) {
          for (auto j=0; j<num; ++j) {
            const auto index = indices[i];

            const auto p = stream->p.at(index);
            const auto d = stream->d.at(index);

            // triangle vertices relative to ray origin
            const auto a = _a.at(j) - p;
            const auto b = _b.at(j) - p;
            const auto c = _c.at(j) - p;

            const auto e0 = c - a;
            const auto e1 = a - b;
            const auto e2 = b - c;

            const auto u = (c + a).cross(e0).dot(d);
            const auto v = (a + b).cross(e1).dor(d);
            const auto w = (b + c).cross(e2).dot(d);

            const auto min = std::min(u, std::min(v, w));
            const auto max = std::max(u, std::max(v, w));

            if (min < 0.0f || max > 0.0f) {
              continue;
            }

            
          }
        }
      }
    };
  }
}
