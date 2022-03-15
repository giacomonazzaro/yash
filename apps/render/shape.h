#pragma once
#include <yocto/yocto_bvh.h>

namespace yash {
using namespace yocto;

template <typename Shape>
vec3f eval_position(const Shape& shape, int element, const vec2f& uv) {
  if (shape.num_points() != 0) {
    auto& point = shape.points(element);
    return shape.positions(point);
  } else if (shape.num_lines() != 0) {
    auto& line = shape.lines(element);
    return interpolate_line(
        shape.positions(line.x), shape.positions(line.y), uv.x);
  } else if (shape.num_triangles() != 0) {
    auto& triangle = shape.triangles(element);
    return interpolate_triangle(shape.positions(triangle.x),
        shape.positions(triangle.y), shape.positions(triangle.z), uv);
  } else if (shape.num_quads() != 0) {
    auto& quad = shape.quads(element);
    return interpolate_quad(shape.positions(quad.x), shape.positions(quad.y),
        shape.positions(quad.z), shape.positions(quad.w), uv);
  } else {
    return {0, 0, 0};
  }
}

template <typename Shape>
bool intersect_shape(const bvh_data& bvh, const Shape& shape, const ray3f& ray_,
    int& element, vec2f& uv, float& distance, bool find_any) {
  // #ifdef YOCTO_EMBREE
  //   // call Embree if needed
  //   if (bvh.embree_bvh) {
  //     return intersect_embree_bvh(
  //         bvh, shape, ray_, element, uv, distance, find_any);
  //   }
  // #endif

  // check empty
  if (bvh.nodes.empty()) return false;

  // node stack
  auto node_stack        = array<int, 128>{};
  auto node_cur          = 0;
  node_stack[node_cur++] = 0;

  // shared variables
  auto hit = false;

  // copy ray to modify it
  auto ray = ray_;

  // prepare ray for fast queries
  auto ray_dinv  = vec3f{1 / ray.d.x, 1 / ray.d.y, 1 / ray.d.z};
  auto ray_dsign = vec3i{(ray_dinv.x < 0) ? 1 : 0, (ray_dinv.y < 0) ? 1 : 0,
      (ray_dinv.z < 0) ? 1 : 0};

  // walking stack
  while (node_cur != 0) {
    // grab node
    auto& node = bvh.nodes[node_stack[--node_cur]];

    // intersect bbox
    // if (!intersect_bbox(ray, ray_dinv, ray_dsign, node.bbox)) continue;
    if (!intersect_bbox(ray, ray_dinv, node.bbox)) continue;

    // intersect node, switching based on node type
    // for each type, iterate over the the primitive list
    if (node.internal) {
      // for internal nodes, attempts to proceed along the
      // split axis from smallest to largest nodes
      if (ray_dsign[node.axis] != 0) {
        node_stack[node_cur++] = node.start + 0;
        node_stack[node_cur++] = node.start + 1;
      } else {
        node_stack[node_cur++] = node.start + 1;
        node_stack[node_cur++] = node.start + 0;
      }
    } else if (shape.num_points() != 0) {
      for (auto idx = node.start; idx < node.start + node.num; idx++) {
        auto& p = shape.points(bvh.primitives[idx]);
        if (intersect_point(
                ray, shape.positions(p), shape.radius(p), uv, distance)) {
          hit      = true;
          element  = bvh.primitives[idx];
          ray.tmax = distance;
        }
      }
    } else if (shape.num_lines() != 0) {
      for (auto idx = node.start; idx < node.start + node.num; idx++) {
        auto& l = shape.lines(bvh.primitives[idx]);
        if (intersect_line(ray, shape.positions(l.x), shape.positions(l.y),
                shape.radius(l.x), shape.radius(l.y), uv, distance)) {
          hit      = true;
          element  = bvh.primitives[idx];
          ray.tmax = distance;
        }
      }
    } else if (shape.num_triangles() != 0) {
      for (auto idx = node.start; idx < node.start + node.num; idx++) {
        auto& t = shape.triangles(bvh.primitives[idx]);
        if (intersect_triangle(ray, shape.positions(t.x), shape.positions(t.y),
                shape.positions(t.z), uv, distance)) {
          hit      = true;
          element  = bvh.primitives[idx];
          ray.tmax = distance;
        }
      }
    } else if (shape.num_quads() != 0) {
      for (auto idx = node.start; idx < node.start + node.num; idx++) {
        auto& q = shape.quads(bvh.primitives[idx]);
        if (intersect_quad(ray, shape.positions(q.x), shape.positions(q.y),
                shape.positions(q.z), shape.positions(q.w), uv, distance)) {
          hit      = true;
          element  = bvh.primitives[idx];
          ray.tmax = distance;
        }
      }
    }

    // check for early exit
    if (find_any && hit) return hit;
  }

  return hit;
}

template <typename Shape>
bvh_intersection intersect_shape(const bvh_data& bvh, const Shape& shape,
    const ray3f& ray, bool find_any = false, bool non_rigid_frames = true) {
  auto intersection = bvh_intersection{};
  intersection.hit  = intersect_shape(bvh, shape, ray, intersection.element,
      intersection.uv, intersection.distance, find_any);
  return intersection;
}

template <typename Shape>
bvh_data make_shape_bvh(const Shape& shape, bool highquality, bool embree) {
  // embree
  // #ifdef YOCTO_EMBREE
  //   if (embree) return make_embree_bvh(shape, highquality);
  // #endif

  // bvh
  auto bvh = bvh_data{};

  // build primitives
  auto bboxes = vector<bbox3f>{};
  if (shape.num_points() != 0) {
    bboxes = vector<bbox3f>(shape.num_points());
    for (auto idx = 0; idx < shape.num_points(); idx++) {
      auto& point = shape.points(idx);
      bboxes[idx] = point_bounds(shape.positions(point), shape.radius(point));
    }
  } else if (shape.num_lines() != 0) {
    bboxes = vector<bbox3f>(shape.num_lines());
    for (auto idx = 0; idx < shape.num_lines(); idx++) {
      auto& line  = shape.lines(idx);
      bboxes[idx] = line_bounds(shape.positions(line.x),
          shape.positions(line.y), shape.radius(line.x), shape.radius(line.y));
    }
  } else if (shape.num_triangles() != 0) {
    bboxes = vector<bbox3f>(shape.num_triangles());
    for (auto idx = 0; idx < shape.num_triangles(); idx++) {
      auto& triangle = shape.triangles(idx);
      bboxes[idx]    = triangle_bounds(shape.positions(triangle.x),
          shape.positions(triangle.y), shape.positions(triangle.z));
    }
  } else if (shape.num_quads() != 0) {
    bboxes = vector<bbox3f>(shape.num_quads());
    for (auto idx = 0; idx < shape.num_quads(); idx++) {
      auto& quad  = shape.quads(idx);
      bboxes[idx] = quad_bounds(shape.positions(quad.x),
          shape.positions(quad.y), shape.positions(quad.z),
          shape.positions(quad.w));
    }
  }

  // build nodes
  build_bvh(bvh, bboxes, highquality);

  // done
  return bvh;
}

}  // namespace yash
