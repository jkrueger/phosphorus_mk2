#pragma once

#include "state.hpp"

namespace stream {
  static const uint32_t RAYS_PER_LANE = (2<<17)-1;

  struct node_ref_t {
    uint32_t offset : 28;
    uint32_t flags  : 4;
    float    d;
  };

  template<int WIDTH>
  struct lanes_t {
    uint32_t active[WIDTH][RAYS_PER_LANE];
    uint32_t num[WIDTH];

    inline lanes_t() {
      memset(num, 0, sizeof(uint32_t) * WIDTH);
    }

    inline void init(const active_t<>& a) {
      memcpy(active[0], a.index, sizeof(uint32_t) * a.num);
      num[0] = a.num;
    }
  };

  struct task_t {
    uint32_t offset;
    uint32_t num_rays;
    uint32_t lane: 8, flags: 8, prims: 16;
    uint32_t padding;
  
    inline bool is_leaf() const {
      return flags == 1;
    }
  };

  template<typename Node>
  inline void push(
    node_ref_t* stack
  , int32_t& top
  , const float* const dists
  , const Node* node
  , uint32_t idx)
  {
    stack[top].offset = node->offset[idx];
    stack[top].flags  = node->num[idx];
    stack[top].d      = dists[idx];
    ++top;
  }

  inline void push(
    task_t* stack
  , int32_t& top
  , uint16_t num)
  {
    stack[top].offset   = 0;
    stack[top].num_rays = num;
    stack[top].lane     = 0;
    stack[top].flags    = 0;
    stack[top].prims    = 0;

    ++top;
  }

  template<typename Node>
  inline void push(
    task_t* stack
  , int32_t& top
  , const Node& node
  , uint8_t  lane
  , uint16_t num)
  {
    stack[top].offset   = node.offset[lane];
    stack[top].num_rays = num;
    stack[top].lane     = lane;
    stack[top].flags    = node.flags[lane];
    stack[top].prims    = node.num[lane];

    ++top;
  }

  template<int WIDTH>
  inline void push(lanes_t<WIDTH>& lanes, uint8_t lane, uint32_t id) {
    auto& a = lanes.num[lane];
    lanes.active[lane][a++] = id;
  }

  template<int WIDTH>
  inline uint32_t* pop(lanes_t<WIDTH>& lanes, uint8_t lane, uint32_t num) {
    auto& a = lanes.num[lane];
    a -= num;

    return &(lanes.active[lane][a]);
  }
}
