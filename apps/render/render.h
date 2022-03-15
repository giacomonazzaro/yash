#pragma once
#include <yocto/yocto_scene.h>
#include <yocto/yocto_shading.h>
#include <yocto/yocto_trace.h>

#include "scene/shape.h"

// -----------------------------------------------------------------------------
// USING DIRECTIVES
// -----------------------------------------------------------------------------
namespace yash {

// using directives
using std::unique_ptr;
using namespace std::string_literals;
using yocto::abs;
using yocto::acos;
using yocto::atan2;
using yocto::cos;
using yocto::fmod;
using yocto::sin;
using yocto::sqrt;
using namespace yocto;

}  // namespace yash

// -----------------------------------------------------------------------------
// TEXTURE PROPERTIES
// -----------------------------------------------------------------------------
namespace yash {

template <typename Texture>
vec4f lookup_texture(
    const Texture& texture, int i, int j, bool as_linear = false) {
  auto color = vec4f{0, 0, 0, 0};
  if (!texture.pixelsf.empty()) {
    color = texture.pixelsf[j * texture.width + i];
  } else {
    color = byte_to_float(texture.pixelsb[j * texture.width + i]);
  }
  if (as_linear && !texture.linear) {
    return srgb_to_rgb(color);
  } else {
    return color;
  }
}

// Evaluates an image at a point `uv`.
template <typename Texture>
vec4f eval_texture(const Texture& texture, const vec2f& uv,
    bool as_linear = false, bool no_interpolation = false,
    bool clamp_to_edge = false) {
  if (texture.width == 0 || texture.height == 0) return {0, 0, 0, 0};

  // get texture width/height
  auto size = vec2i{texture.width, texture.height};

  // get coordinates normalized for tiling
  auto s = 0.0f, t = 0.0f;
  if (clamp_to_edge) {
    s = clamp(uv.x, 0.0f, 1.0f) * size.x;
    t = clamp(uv.y, 0.0f, 1.0f) * size.y;
  } else {
    s = yocto::fmod(uv.x, 1.0f) * size.x;
    if (s < 0) s += size.x;
    t = yocto::fmod(uv.y, 1.0f) * size.y;
    if (t < 0) t += size.y;
  }

  // get image coordinates and residuals
  auto i = clamp((int)s, 0, size.x - 1), j = clamp((int)t, 0, size.y - 1);
  auto ii = (i + 1) % size.x, jj = (j + 1) % size.y;
  auto u = s - i, v = t - j;

  // handle interpolation
  if (no_interpolation) {
    return lookup_texture(texture, i, j, as_linear);
  } else {
    return lookup_texture(texture, i, j, as_linear) * (1 - u) * (1 - v) +
           lookup_texture(texture, i, jj, as_linear) * (1 - u) * v +
           lookup_texture(texture, ii, j, as_linear) * u * (1 - v) +
           lookup_texture(texture, ii, jj, as_linear) * u * v;
  }
}

template <typename Scene>
vec4f eval_texture(const Scene& scene, int texture, const vec2f& uv,
    bool ldr_as_linear = false, bool no_interpolation = false,
    bool clamp_to_edge = false) {
  if (texture == invalidid) return {1, 1, 1, 1};
  return eval_texture(
      scene.textures(texture), uv, ldr_as_linear, no_interpolation);
}
}  // namespace yash

// -----------------------------------------------------------------------------
// MATERIAL PROPERTIES
// -----------------------------------------------------------------------------
namespace yash {

// Evaluate material
template <typename Scene>
material_point eval_material(const Scene& scene, const material_data& material,
    const vec2f& texcoord, const vec4f& color_shp) {
  // evaluate textures
  auto emission_tex = eval_texture(
      scene, material.emission_tex, texcoord, true);
  auto color_tex     = eval_texture(scene, material.color_tex, texcoord, true);
  auto roughness_tex = eval_texture(
      scene, material.roughness_tex, texcoord, false);
  auto scattering_tex = eval_texture(
      scene, material.scattering_tex, texcoord, true);

  // material point
  auto point         = material_point{};
  point.type         = material.type;
  point.emission     = material.emission * xyz(emission_tex);
  point.color        = material.color * xyz(color_tex) * xyz(color_shp);
  point.opacity      = material.opacity * color_tex.w * color_shp.w;
  point.metallic     = material.metallic * roughness_tex.z;
  point.roughness    = material.roughness * roughness_tex.y;
  point.roughness    = point.roughness * point.roughness;
  point.ior          = material.ior;
  point.scattering   = material.scattering * xyz(scattering_tex);
  point.scanisotropy = material.scanisotropy;
  point.trdepth      = material.trdepth;

  // volume density
  if (material.type == material_type::refractive ||
      material.type == material_type::volumetric ||
      material.type == material_type::subsurface) {
    point.density = -log(clamp(point.color, 0.0001f, 1.0f)) / point.trdepth;
  } else {
    point.density = {0, 0, 0};
  }

  // fix roughness
  if (point.type == material_type::matte ||
      point.type == material_type::gltfpbr ||
      point.type == material_type::glossy) {
    point.roughness = clamp(point.roughness, min_roughness, 1.0f);
  }

  return point;
}

}  // namespace yash

// -----------------------------------------------------------------------------
// INSTANCE PROPERTIES
// -----------------------------------------------------------------------------
namespace yash {

// Eval position
template <typename Scene>
vec3f eval_position(const Scene& scene, const instance_data& instance,
    int element, const vec2f& uv) {
  auto shape = scene.shapes(instance.shape);
  if (shape.num_triangles() != 0) {
    auto t = shape.triangles(element);
    return transform_point(
        instance.frame, interpolate_triangle(shape.positions(t.x),
                            shape.positions(t.y), shape.positions(t.z), uv));
  } else if (shape.num_quads() != 0) {
    auto q = shape.quads(element);
    return transform_point(instance.frame,
        interpolate_quad(shape.positions(q.x), shape.positions(q.y),
            shape.positions(q.z), shape.positions(q.w), uv));
  } else if (shape.num_lines() != 0) {
    auto l = shape.lines(element);
    return transform_point(instance.frame,
        interpolate_line(shape.positions(l.x), shape.positions(l.y), uv.x));
  } else if (shape.num_points() != 0) {
    return transform_point(
        instance.frame, shape.positions(shape.points(element)));
  } else {
    return {0, 0, 0};
  }
}

// Shape element normal.
template <typename Scene>
vec3f eval_element_normal(
    const Scene& scene, const instance_data& instance, int element) {
  auto shape = scene.shapes(instance.shape);
  if (shape.num_triangles() != 0) {
    auto t = shape.triangles(element);
    return transform_normal(
        instance.frame, triangle_normal(shape.positions(t.x),
                            shape.positions(t.y), shape.positions(t.z)));
  } else if (shape.num_quads() != 0) {
    auto q = shape.quads(element);
    return transform_normal(
        instance.frame, quad_normal(shape.positions(q.x), shape.positions(q.y),
                            shape.positions(q.z), shape.positions(q.w)));
  } else if (shape.num_lines() != 0) {
    auto l = shape.lines(element);
    return transform_normal(instance.frame,
        line_tangent(shape.positions(l.x), shape.positions(l.y)));
  } else if (shape.num_points() != 0) {
    return {0, 0, 1};
  } else {
    return {0, 0, 0};
  }
}

// Eval normal
template <typename Scene>
vec3f eval_normal(const Scene& scene, const instance_data& instance,
    int element, const vec2f& uv) {
  auto shape = scene.shapes(instance.shape);
  if (shape.num_normals() == 0)
    return eval_element_normal(scene, instance, element);
  if (shape.num_triangles() != 0) {
    auto t = shape.triangles(element);
    return transform_normal(
        instance.frame, normalize(interpolate_triangle(shape.normals(t.x),
                            shape.normals(t.y), shape.normals(t.z), uv)));
  } else if (shape.num_quads() != 0) {
    auto q = shape.quads(element);
    return transform_normal(instance.frame,
        normalize(interpolate_quad(shape.normals(q.x), shape.normals(q.y),
            shape.normals(q.z), shape.normals(q.w), uv)));
  } else if (shape.num_lines() != 0) {
    auto l = shape.lines(element);
    return transform_normal(instance.frame,
        normalize(
            interpolate_line(shape.normals(l.x), shape.normals(l.y), uv.x)));
  } else if (shape.num_points() != 0) {
    return transform_normal(
        instance.frame, normalize(shape.normals(shape.points(element))));
  } else {
    return {0, 0, 0};
  }
}

// Eval texcoord
template <typename Scene>
vec2f eval_texcoord(const Scene& scene, const instance_data& instance,
    int element, const vec2f& uv) {
  auto shape = scene.shapes(instance.shape);
  if (shape.num_texcoords() == 0) return uv;
  if (shape.num_triangles() != 0) {
    auto t = shape.triangles(element);
    return interpolate_triangle(
        shape.texcoords(t.x), shape.texcoords(t.y), shape.texcoords(t.z), uv);
  } else if (shape.num_quads() != 0) {
    auto q = shape.quads(element);
    return interpolate_quad(shape.texcoords(q.x), shape.texcoords(q.y),
        shape.texcoords(q.z), shape.texcoords(q.w), uv);
  } else if (shape.num_lines() != 0) {
    auto l = shape.lines(element);
    return interpolate_line(shape.texcoords(l.x), shape.texcoords(l.y), uv.x);
  } else if (shape.num_points() != 0) {
    return shape.texcoords(shape.points(element));
  } else {
    return zero2f;
  }
}

#if 0
// Shape element normal.
inline pair<vec3f, vec3f> eval_tangents(
    const trace_shape& shape, int element, const vec2f& uv) {
  if (!shape.triangles.empty()) {
    auto t = shape.triangles[element];
    if (shape.texcoords.empty()) {
      return triangle_tangents_fromuv(shape.positions[t.x],
          shape.positions[t.y], shape.positions[t.z], {0, 0}, {1, 0}, {0, 1});
    } else {
      return triangle_tangents_fromuv(shape.positions[t.x],
          shape.positions[t.y], shape.positions[t.z], shape.texcoords[t.x],
          shape.texcoords[t.y], shape.texcoords[t.z]);
    }
  } else if (!shape.quads.empty()) {
    auto q = shape.quads[element];
    if (shape.texcoords.empty()) {
      return quad_tangents_fromuv(shape.positions[q.x], shape.positions[q.y],
          shape.positions[q.z], shape.positions[q.w], {0, 0}, {1, 0}, {0, 1},
          {1, 1}, uv);
    } else {
      return quad_tangents_fromuv(shape.positions[q.x], shape.positions[q.y],
          shape.positions[q.z], shape.positions[q.w], shape.texcoords[q.x],
          shape.texcoords[q.y], shape.texcoords[q.z], shape.texcoords[q.w],
          uv);
    }
  } else {
    return {{0,0,0}, {0,0,0}};
  }
}
#endif

// Shape element normal.
template <typename Scene>
pair<vec3f, vec3f> eval_element_tangents(
    const Scene& scene, const instance_data& instance, int element) {
  auto shape = scene.shapes(instance.shape);
  if (shape.num_triangles() != 0 && shape.num_texcoords() != 0) {
    auto t        = shape.triangles(element);
    auto [tu, tv] = triangle_tangents_fromuv(shape.positions(t.x),
        shape.positions(t.y), shape.positions(t.z), shape.texcoords(t.x),
        shape.texcoords(t.y), shape.texcoords(t.z));
    return {transform_direction(instance.frame, tu),
        transform_direction(instance.frame, tv)};
  } else if (shape.num_quads() != 0 && shape.num_texcoords() != 0) {
    auto q        = shape.quads(element);
    auto [tu, tv] = quad_tangents_fromuv(shape.positions(q.x),
        shape.positions(q.y), shape.positions(q.z), shape.positions(q.w),
        shape.texcoords(q.x), shape.texcoords(q.y), shape.texcoords(q.z),
        shape.texcoords(q.w), {0, 0});
    return {transform_direction(instance.frame, tu),
        transform_direction(instance.frame, tv)};
  } else {
    return {};
  }
}

template <typename Scene>
vec3f eval_normalmap(const Scene& scene, const instance_data& instance,
    int element, const vec2f& uv) {
  auto  shape    = scene.shapes(instance.shape);
  auto& material = scene.materials(instance.material);
  // apply normal mapping
  auto normal   = eval_normal(scene, instance, element, uv);
  auto texcoord = eval_texcoord(scene, instance, element, uv);
  if (material.normal_tex != invalidid &&
      (shape.num_triangles() != 0 || shape.num_quads() != 0)) {
    auto normal_tex = scene.textures(material.normal_tex);
    auto normalmap  = -1 + 2 * xyz(eval_texture(normal_tex, texcoord, false));
    auto [tu, tv]   = eval_element_tangents(scene, instance, element);
    auto frame      = frame3f{tu, tv, normal, {0, 0, 0}};
    frame.x         = orthonormalize(frame.x, frame.z);
    frame.y         = normalize(cross(frame.z, frame.x));
    auto flip_v     = dot(frame.y, tv) < 0;
    normalmap.y *= flip_v ? 1 : -1;  // flip vertical axis
    normal = transform_normal(frame, normalmap);
  }
  return normal;
}

// Eval shading position
template <typename Scene>
vec3f eval_shading_position(const Scene& scene, const instance_data& instance,
    int element, const vec2f& uv, const vec3f& outgoing) {
  auto shape = scene.shapes(instance.shape);
  if (shape.num_triangles() != 0 || shape.num_quads() != 0) {
    return eval_position(scene, instance, element, uv);
  } else if (shape.num_lines() != 0) {
    return eval_position(scene, instance, element, uv);
  } else if (shape.num_points() != 0) {
    return eval_position(shape, element, uv);
  } else {
    return {0, 0, 0};
  }
}

// Eval shading normal
template <typename Scene>
vec3f eval_shading_normal(const Scene& scene, const instance_data& instance,
    int element, const vec2f& uv, const vec3f& outgoing) {
  auto shape    = scene.shapes(instance.shape);
  auto material = scene.materials(instance.material);
  if (shape.num_triangles() != 0 || shape.num_quads() != 0) {
    auto normal = eval_normal(scene, instance, element, uv);
    if (material.normal_tex != invalidid) {
      normal = eval_normalmap(scene, instance, element, uv);
    }
    if (material.type == material_type::refractive) return normal;
    return dot(normal, outgoing) >= 0 ? normal : -normal;
  } else if (shape.num_lines() != 0) {
    auto normal = eval_normal(scene, instance, element, uv);
    return orthonormalize(outgoing, normal);
  } else if (shape.num_points() != 0) {
    // HACK: sphere
    if (true) {
      return transform_direction(instance.frame,
          vec3f{cos(2 * pif * uv.x) * sin(pif * uv.y),
              sin(2 * pif * uv.x) * sin(pif * uv.y), cos(pif * uv.y)});
    } else {
      return outgoing;
    }
  } else {
    return {0, 0, 0};
  }
}

// Eval color
template <typename Scene>
vec4f eval_color(const Scene& scene, const instance_data& instance, int element,
    const vec2f& uv) {
  auto shape = scene.shapes(instance.shape);
  if (shape.num_colors() == 0) return {1, 1, 1, 1};
  if (shape.num_triangles() != 0) {
    auto t = shape.triangles(element);
    return interpolate_triangle(
        shape.colors(t.x), shape.colors(t.y), shape.colors(t.z), uv);
  } else if (shape.num_quads() != 0) {
    auto q = shape.quads(element);
    return interpolate_quad(shape.colors(q.x), shape.colors(q.y),
        shape.colors(q.z), shape.colors(q.w), uv);
  } else if (shape.num_lines() != 0) {
    auto l = shape.lines(element);
    return interpolate_line(shape.colors(l.x), shape.colors(l.y), uv.x);
  } else if (shape.num_points() != 0) {
    return shape.colors(shape.points(element));
  } else {
    return {0, 0, 0, 0};
  }
}

// Evaluate material
template <typename Scene>
material_point eval_material(const Scene& scene, const instance_data& instance,
    int element, const vec2f& uv) {
  auto& material = scene.materials(instance.material);
  auto  texcoord = eval_texcoord(scene, instance, element, uv);

  // evaluate textures
  auto emission_tex = eval_texture(
      scene, material.emission_tex, texcoord, true);
  auto color_shp     = eval_color(scene, instance, element, uv);
  auto color_tex     = eval_texture(scene, material.color_tex, texcoord, true);
  auto roughness_tex = eval_texture(
      scene, material.roughness_tex, texcoord, false);
  auto scattering_tex = eval_texture(
      scene, material.scattering_tex, texcoord, true);

  // material point
  auto point         = material_point{};
  point.type         = material.type;
  point.emission     = material.emission * xyz(emission_tex);
  point.color        = material.color * xyz(color_tex) * xyz(color_shp);
  point.opacity      = material.opacity * color_tex.w * color_shp.w;
  point.metallic     = material.metallic * roughness_tex.z;
  point.roughness    = material.roughness * roughness_tex.y;
  point.roughness    = point.roughness * point.roughness;
  point.ior          = material.ior;
  point.scattering   = material.scattering * xyz(scattering_tex);
  point.scanisotropy = material.scanisotropy;
  point.trdepth      = material.trdepth;

  // volume density
  if (material.type == material_type::refractive ||
      material.type == material_type::volumetric ||
      material.type == material_type::subsurface) {
    point.density = -log(clamp(point.color, 0.0001f, 1.0f)) / point.trdepth;
  } else {
    point.density = {0, 0, 0};
  }

  // fix roughness
  if (point.type == material_type::matte ||
      point.type == material_type::gltfpbr ||
      point.type == material_type::glossy) {
    point.roughness = clamp(point.roughness, min_roughness, 1.0f);
  } else if (material.type == material_type::volumetric) {
    point.roughness = 0;
  } else {
    if (point.roughness < min_roughness) point.roughness = 0;
  }

  return point;
}

// check if an instance is volumetric
template <typename Scene>
bool is_volumetric(const Scene& scene, const instance_data& instance) {
  return is_volumetric(scene.materials(instance.material));
}

}  // namespace yash

// -----------------------------------------------------------------------------
// ENVIRONMENT PROPERTIES
// -----------------------------------------------------------------------------
namespace yash {

// Evaluate environment color.
template <typename Scene>
vec3f eval_environment(const Scene& scene, const environment_data& environment,
    const vec3f& direction) {
  auto wl       = transform_direction(inverse(environment.frame), direction);
  auto texcoord = vec2f{
      atan2(wl.z, wl.x) / (2 * pif), acos(clamp(wl.y, -1.0f, 1.0f)) / pif};
  if (texcoord.x < 0) texcoord.x += 1;
  return environment.emission *
         xyz(eval_texture(scene, environment.emission_tex, texcoord));
}

// Evaluate all environment color.
template <typename Scene>
vec3f eval_environment(const Scene& scene, const vec3f& direction) {
  auto emission = vec3f{0, 0, 0};
  //  for (auto& environment : scene.environments) {
  // TODO(giacomo): implement range loops
  for (auto i = 0; i < scene.num_environments(); i++) {
    emission += eval_environment(scene, scene.environments(i), direction);
  }
  return emission;
}

struct bvh_scene : bvh_data {};
// vector<instance_data> scene_instances = {};
// vector<Shape_View>    scene_shapes    = {};
//   const Scene* scene = nullptr;
//   bvh_scene(const Scene* s) : scene(s) {}
// const Shape& scene_shapes(size_t i) const {
//   return scene->shapes
// }
// };

template <typename Scene>
bvh_scene make_scene_bvh(
    const Scene& scene, bool highquality, bool embree, bool noparallel) {
  // embree
  // #ifdef YOCTO_EMBREE
  //   if (embree) return make_embree_bvh(scene, highquality, noparallel);
  // #endif

  // bvh
  auto bvh = bvh_scene{};

  // build shape bvh
  bvh.shapes.resize(scene.shapes().size());
  if (noparallel) {
    for (auto idx = (size_t)0; idx < scene.shapes().size(); idx++) {
      bvh.shapes[idx] = make_shape_bvh(scene.shapes(idx), highquality, embree);
    }
  } else {
    parallel_for(scene.shapes().size(), [&](size_t idx) {
      bvh.shapes[idx] = make_shape_bvh(scene.shapes(idx), highquality, embree);
    });
  }

  // instance bboxes
  auto bboxes = vector<bbox3f>(scene.instances().size());
  for (auto idx = 0; idx < bboxes.size(); idx++) {
    auto& instance = scene.instances(idx);
    auto& sbvh     = bvh.shapes[instance.shape];
    bboxes[idx]    = sbvh.nodes.empty()
                         ? invalidb3f
                         : transform_bbox(instance.frame, sbvh.nodes[0].bbox);
  }

  // build nodes
  build_bvh(bvh, bboxes, highquality);

  //  bvh.scene_instances.resize(scene.instances().size());
  //  for (int i = 0; i < bvh.scene_instances.size(); i++) {
  //    bvh.scene_instances[i] = scene.instances(i);
  //  }
  //  bvh.scene_shapes.resize(scene.shapes().size());
  //  for (int i = 0; i < bvh.scene_shapes.size(); i++) {
  //    bvh.scene_shapes[i] = scene.shapes(i);
  //  }

  // done
  return bvh;
}

template <typename Scene>
bvh_scene make_bvh(const Scene& scene, const trace_params& params) {
  return make_scene_bvh(
      scene, params.highqualitybvh, params.embreebvh, params.noparallel);
}

template <typename Scene>
bool intersect_scene(const bvh_scene& bvh, const Scene& scene,
    const ray3f& ray_, int& instance, int& element, vec2f& uv, float& distance,
    bool find_any, bool non_rigid_frames) {
  // #ifdef YOCTO_EMBREE
  //   // call Embree if needed
  //   if (bvh.embree_bvh) {
  //     return intersect_embree_bvh(
  //         bvh, scene, ray_, instance, element, uv, distance, find_any);
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
    } else {
      for (auto idx = node.start; idx < node.start + node.num; idx++) {
        auto& instance_ = scene.instances(bvh.primitives[idx]);
        auto  inv_ray   = transform_ray(
            inverse(instance_.frame, non_rigid_frames), ray);
        if (intersect_shape(bvh.shapes[instance_.shape],
                scene.shapes(instance_.shape), inv_ray, element, uv, distance,
                find_any)) {
          hit      = true;
          instance = bvh.primitives[idx];
          ray.tmax = distance;
        }
      }
    }

    // check for early exit
    if (find_any && hit) return hit;
  }

  return hit;
}

// Intersect ray with a bvh.
template <typename Scene>
bool intersect_scene(const bvh_scene& bvh, const Scene& scene, int instance_,
    const ray3f& ray, int& element, vec2f& uv, float& distance, bool find_any,
    bool non_rigid_frames) {
  auto& instance = scene.instances(instance_);
  auto  inv_ray = transform_ray(inverse(instance.frame, non_rigid_frames), ray);
  return intersect_shape(bvh.shapes[instance.shape],
      scene.shapes(instance.shape), inv_ray, element, uv, distance, find_any);
}
template <typename Scene>
bvh_intersection intersect_scene(const bvh_scene& bvh, const Scene& scene,
    const ray3f& ray, bool find_any = true, bool non_rigid_frames = false) {
  auto intersection = bvh_intersection{};
  intersection.hit  = intersect_scene(bvh, scene, ray, intersection.instance,
      intersection.element, intersection.uv, intersection.distance, find_any,
      non_rigid_frames);
  return intersection;
}
template <typename Scene>
bvh_intersection intersect_scene(const bvh_scene& bvh, const Scene& scene,
    int instance, const ray3f& ray, bool find_any = true,
    bool non_rigid_frames = false) {
  auto intersection     = bvh_intersection{};
  intersection.hit      = intersect_scene(bvh, scene, instance, ray,
      intersection.element, intersection.uv, intersection.distance, find_any,
      non_rigid_frames);
  intersection.instance = instance;
  return intersection;
}

template <typename Scene>
vec3f eval_position(const Scene& scene, const bvh_intersection& intersection) {
  return eval_position(scene, scene.instances(intersection.instance),
      intersection.element, intersection.uv);
}
template <typename Scene>
vec3f eval_normal(const Scene& scene, const bvh_intersection& intersection) {
  return eval_normal(scene, scene.instances(intersection.instance),
      intersection.element, intersection.uv);
}
template <typename Scene>
vec3f eval_element_normal(
    const Scene& scene, const bvh_intersection& intersection) {
  return eval_element_normal(
      scene, scene.instances(intersection.instance), intersection.element);
}
template <typename Scene>
vec3f eval_shading_position(const Scene& scene,
    const bvh_intersection& intersection, const vec3f& outgoing) {
  return eval_shading_position(scene, scene.instances(intersection.instance),
      intersection.element, intersection.uv, outgoing);
}
template <typename Scene>
vec3f eval_shading_normal(const Scene& scene,
    const bvh_intersection& intersection, const vec3f& outgoing) {
  return eval_shading_normal(scene, scene.instances(intersection.instance),
      intersection.element, intersection.uv, outgoing);
}
template <typename Scene>
vec2f eval_texcoord(const Scene& scene, const bvh_intersection& intersection) {
  return eval_texcoord(scene, scene.instances(intersection.instance),
      intersection.element, intersection.uv);
}
template <typename Scene>
material_point eval_material(
    const Scene& scene, const bvh_intersection& intersection) {
  return eval_material(scene, scene.instances(intersection.instance),
      intersection.element, intersection.uv);
}
template <typename Scene>
bool is_volumetric(const Scene& scene, const bvh_intersection& intersection) {
  return is_volumetric(scene, scene.instances(intersection.instance));
}
inline vec3f eval_emission(const material_point& material, const vec3f& normal,
    const vec3f& outgoing) {
  return dot(normal, outgoing) >= 0 ? material.emission : vec3f{0, 0, 0};
}
// Picks a direction based on the BRDF
inline vec3f sample_bsdfcos(const material_point& material, const vec3f& normal,
    const vec3f& outgoing, float rnl, const vec2f& rn) {
  if (material.roughness == 0) return {0, 0, 0};

  if (material.type == material_type::matte) {
    return sample_matte(material.color, normal, outgoing, rn);
  } else if (material.type == material_type::glossy) {
    return sample_glossy(material.color, material.ior, material.roughness,
        normal, outgoing, rnl, rn);
  } else if (material.type == material_type::reflective) {
    return sample_reflective(
        material.color, material.roughness, normal, outgoing, rn);
  } else if (material.type == material_type::transparent) {
    return sample_transparent(material.color, material.ior, material.roughness,
        normal, outgoing, rnl, rn);
  } else if (material.type == material_type::refractive) {
    return sample_refractive(material.color, material.ior, material.roughness,
        normal, outgoing, rnl, rn);
  } else if (material.type == material_type::subsurface) {
    return sample_refractive(material.color, material.ior, material.roughness,
        normal, outgoing, rnl, rn);
  } else if (material.type == material_type::gltfpbr) {
    return sample_gltfpbr(material.color, material.ior, material.roughness,
        material.metallic, normal, outgoing, rnl, rn);
  } else {
    return {0, 0, 0};
  }
}


inline trace_light& add_light(trace_lights& lights) {
  return lights.lights.emplace_back();
}

template <typename Scene>
trace_lights make_lights(const Scene& scene, const trace_params& params) {
  auto lights = trace_lights{};

  for (auto handle = 0; handle < scene.num_instances(); handle++) {
    auto& instance = scene.instances(handle);
    auto& material = scene.materials(instance.material);
    if (material.emission == vec3f{0, 0, 0}) continue;
    auto shape = scene.shapes(instance.shape);
    if (shape.num_triangles() == 0 && shape.num_quads() == 0) continue;
    auto& light       = add_light(lights);
    light.instance    = handle;
    light.environment = invalidid;
    if (shape.num_triangles() != 0) {
      light.elements_cdf = vector<float>(shape.num_triangles());
      for (auto idx = 0; idx < light.elements_cdf.size(); idx++) {
        auto& t                 = shape.triangles(idx);
        light.elements_cdf[idx] = triangle_area(
            shape.positions(t.x), shape.positions(t.y), shape.positions(t.z));
        if (idx != 0) light.elements_cdf[idx] += light.elements_cdf[idx - 1];
      }
    }
    if (shape.num_quads() != 0) {
      light.elements_cdf = vector<float>(shape.num_quads());
      for (auto idx = 0; idx < light.elements_cdf.size(); idx++) {
        auto& t                 = shape.quads(idx);
        light.elements_cdf[idx] = quad_area(shape.positions(t.x),
            shape.positions(t.y), shape.positions(t.z), shape.positions(t.w));
        if (idx != 0) light.elements_cdf[idx] += light.elements_cdf[idx - 1];
      }
    }
  }
  for (auto handle = 0; handle < scene.num_environments(); handle++) {
    auto& environment = scene.environments(handle);
    if (environment.emission == vec3f{0, 0, 0}) continue;
    auto& light       = add_light(lights);
    light.instance    = invalidid;
    light.environment = handle;
    if (environment.emission_tex != invalidid) {
      auto texture       = scene.textures(environment.emission_tex);
      light.elements_cdf = vector<float>(texture.width * texture.height);
      for (auto idx = 0; idx < light.elements_cdf.size(); idx++) {
        auto ij    = vec2i{idx % texture.width, idx / texture.width};
        auto th    = (ij.y + 0.5f) * pif / texture.height;
        auto value = lookup_texture(texture, ij.x, ij.y);
        light.elements_cdf[idx] = max(value) * sin(th);
        if (idx != 0) light.elements_cdf[idx] += light.elements_cdf[idx - 1];
      }
    }
  }

  // handle progress
  return lights;
}

struct trace_result {
  vec3f radiance = {0, 0, 0};
  bool  hit      = false;
  vec3f albedo   = {0, 0, 0};
  vec3f normal   = {0, 0, 0};
};

vec3f eval_scattering(const material_point& material, const vec3f& outgoing,
    const vec3f& incoming) {
  if (material.density == vec3f{0, 0, 0}) return {0, 0, 0};
  return material.scattering * material.density *
         eval_phasefunction(material.scanisotropy, outgoing, incoming);
}

vec3f sample_scattering(const material_point& material, const vec3f& outgoing,
    float rnl, const vec2f& rn) {
  if (material.density == vec3f{0, 0, 0}) return {0, 0, 0};
  return sample_phasefunction(material.scanisotropy, outgoing, rn);
}

float sample_scattering_pdf(const material_point& material,
    const vec3f& outgoing, const vec3f& incoming) {
  if (material.density == vec3f{0, 0, 0}) return 0;
  return sample_phasefunction_pdf(material.scanisotropy, outgoing, incoming);
}

ray3f sample_camera(const camera_data& camera, const vec2i& ij,
    const vec2i& image_size, const vec2f& puv, const vec2f& luv, bool tent) {
  if (!tent) {
    auto uv = vec2f{
        (ij.x + puv.x) / image_size.x, (ij.y + puv.y) / image_size.y};
    return eval_camera(camera, uv, sample_disk(luv));
  } else {
    const auto width  = 2.0f;
    const auto offset = 0.5f;
    auto       fuv =
        width *
            vec2f{
                puv.x < 0.5f ? sqrt(2 * puv.x) - 1 : 1 - sqrt(2 - 2 * puv.x),
                puv.y < 0.5f ? sqrt(2 * puv.y) - 1 : 1 - sqrt(2 - 2 * puv.y),
            } +
        offset;
    auto uv = vec2f{
        (ij.x + fuv.x) / image_size.x, (ij.y + fuv.y) / image_size.y};
    return eval_camera(camera, uv, sample_disk(luv));
  }
}

template <typename Scene>
vec3f sample_lights(const Scene& scene, const trace_lights& lights,
    const vec3f& position, float rl, float rel, const vec2f& ruv) {
  auto  light_id = sample_uniform((int)lights.lights.size(), rl);
  auto& light    = lights.lights[light_id];
  if (light.instance != invalidid) {
    auto& instance  = scene.instances(light.instance);
    auto  shape     = scene.shapes(instance.shape);
    auto  element   = sample_discrete(light.elements_cdf, rel);
    auto  uv        = (shape.num_triangles() == 0) ? sample_triangle(ruv) : ruv;
    auto  lposition = eval_position(scene, instance, element, uv);
    return normalize(lposition - position);
  } else if (light.environment != invalidid) {
    auto& environment = scene.environments(light.environment);
    if (environment.emission_tex != invalidid) {
      auto emission_tex = scene.textures(environment.emission_tex);
      auto idx          = sample_discrete(light.elements_cdf, rel);
      auto uv = vec2f{((idx % emission_tex.width) + 0.5f) / emission_tex.width,
          ((idx / emission_tex.width) + 0.5f) / emission_tex.height};
      return transform_direction(environment.frame,
          {cos(uv.x * 2 * pif) * sin(uv.y * pif), cos(uv.y * pif),
              sin(uv.x * 2 * pif) * sin(uv.y * pif)});
    } else {
      return sample_sphere(ruv);
    }
  } else {
    return {0, 0, 0};
  }
}

// Evaluates/sample the BRDF scaled by the cosine of the incoming direction.
inline vec3f eval_bsdfcos(const material_point& material, const vec3f& normal,
    const vec3f& outgoing, const vec3f& incoming) {
  if (material.roughness == 0) return {0, 0, 0};

  if (material.type == material_type::matte) {
    return eval_matte(material.color, normal, outgoing, incoming);
  } else if (material.type == material_type::glossy) {
    return eval_glossy(material.color, material.ior, material.roughness, normal,
        outgoing, incoming);
  } else if (material.type == material_type::reflective) {
    return eval_reflective(
        material.color, material.roughness, normal, outgoing, incoming);
  } else if (material.type == material_type::transparent) {
    return eval_transparent(material.color, material.ior, material.roughness,
        normal, outgoing, incoming);
  } else if (material.type == material_type::refractive) {
    return eval_refractive(material.color, material.ior, material.roughness,
        normal, outgoing, incoming);
  } else if (material.type == material_type::subsurface) {
    return eval_refractive(material.color, material.ior, material.roughness,
        normal, outgoing, incoming);
  } else if (material.type == material_type::gltfpbr) {
    return eval_gltfpbr(material.color, material.ior, material.roughness,
        material.metallic, normal, outgoing, incoming);
  } else {
    return {0, 0, 0};
  }
}

inline vec3f eval_delta(const material_point& material, const vec3f& normal,
    const vec3f& outgoing, const vec3f& incoming) {
  if (material.roughness != 0) return {0, 0, 0};

  if (material.type == material_type::reflective) {
    return eval_reflective(material.color, normal, outgoing, incoming);
  } else if (material.type == material_type::transparent) {
    return eval_transparent(
        material.color, material.ior, normal, outgoing, incoming);
  } else if (material.type == material_type::refractive) {
    return eval_refractive(
        material.color, material.ior, normal, outgoing, incoming);
  } else if (material.type == material_type::volumetric) {
    return eval_passthrough(material.color, normal, outgoing, incoming);
  } else {
    return {0, 0, 0};
  }
}

inline vec3f sample_delta(const material_point& material, const vec3f& normal,
    const vec3f& outgoing, float rnl) {
  if (material.roughness != 0) return {0, 0, 0};

  if (material.type == material_type::reflective) {
    return sample_reflective(material.color, normal, outgoing);
  } else if (material.type == material_type::transparent) {
    return sample_transparent(
        material.color, material.ior, normal, outgoing, rnl);
  } else if (material.type == material_type::refractive) {
    return sample_refractive(
        material.color, material.ior, normal, outgoing, rnl);
  } else if (material.type == material_type::volumetric) {
    return sample_passthrough(material.color, normal, outgoing);
  } else {
    return {0, 0, 0};
  }
}

// Compute the weight for sampling the BRDF
inline float sample_bsdfcos_pdf(const material_point& material,
    const vec3f& normal, const vec3f& outgoing, const vec3f& incoming) {
  if (material.roughness == 0) return 0;

  if (material.type == material_type::matte) {
    return sample_matte_pdf(material.color, normal, outgoing, incoming);
  } else if (material.type == material_type::glossy) {
    return sample_glossy_pdf(material.color, material.ior, material.roughness,
        normal, outgoing, incoming);
  } else if (material.type == material_type::reflective) {
    return sample_reflective_pdf(
        material.color, material.roughness, normal, outgoing, incoming);
  } else if (material.type == material_type::transparent) {
    return sample_tranparent_pdf(material.color, material.ior,
        material.roughness, normal, outgoing, incoming);
  } else if (material.type == material_type::refractive) {
    return sample_refractive_pdf(material.color, material.ior,
        material.roughness, normal, outgoing, incoming);
  } else if (material.type == material_type::subsurface) {
    return sample_refractive_pdf(material.color, material.ior,
        material.roughness, normal, outgoing, incoming);
  } else if (material.type == material_type::gltfpbr) {
    return sample_gltfpbr_pdf(material.color, material.ior, material.roughness,
        material.metallic, normal, outgoing, incoming);
  } else {
    return 0;
  }
}

inline float sample_delta_pdf(const material_point& material,
    const vec3f& normal, const vec3f& outgoing, const vec3f& incoming) {
  if (material.roughness != 0) return 0;

  if (material.type == material_type::reflective) {
    return sample_reflective_pdf(material.color, normal, outgoing, incoming);
  } else if (material.type == material_type::transparent) {
    return sample_tranparent_pdf(
        material.color, material.ior, normal, outgoing, incoming);
  } else if (material.type == material_type::refractive) {
    return sample_refractive_pdf(
        material.color, material.ior, normal, outgoing, incoming);
  } else if (material.type == material_type::volumetric) {
    return sample_passthrough_pdf(material.color, normal, outgoing, incoming);
  } else {
    return 0;
  }
}

// Sample lights pdf
template <typename Scene>
inline float sample_lights_pdf(const Scene& scene, const bvh_scene& bvh,
    const trace_lights& lights, const vec3f& position, const vec3f& direction) {
  auto pdf = 0.0f;
  for (auto& light : lights.lights) {
    if (light.instance != invalidid) {
      auto& instance = scene.instances(light.instance);
      // check all intersection
      auto lpdf          = 0.0f;
      auto next_position = position;
      for (auto bounce = 0; bounce < 100; bounce++) {
        auto intersection = intersect_scene(
            bvh, scene, light.instance, {next_position, direction});
        if (!intersection.hit) break;
        // accumulate pdf
        auto lposition = eval_position(
            scene, instance, intersection.element, intersection.uv);
        auto lnormal = eval_element_normal(
            scene, instance, intersection.element);
        // prob triangle * area triangle = area triangle mesh
        auto area = light.elements_cdf.back();
        lpdf += distance_squared(lposition, position) /
                (abs(dot(lnormal, direction)) * area);
        // continue
        next_position = lposition + direction * 1e-3f;
      }
      pdf += lpdf;
    } else if (light.environment != invalidid) {
      auto& environment = scene.environments(light.environment);
      if (environment.emission_tex != invalidid) {
        auto& emission_tex = scene.textures(environment.emission_tex);
        auto  wl = transform_direction(inverse(environment.frame), direction);
        auto  texcoord = vec2f{atan2(wl.z, wl.x) / (2 * pif),
            acos(clamp(wl.y, -1.0f, 1.0f)) / pif};
        if (texcoord.x < 0) texcoord.x += 1;
        auto i = clamp(
            (int)(texcoord.x * emission_tex.width), 0, emission_tex.width - 1);
        auto j    = clamp((int)(texcoord.y * emission_tex.height), 0,
            emission_tex.height - 1);
        auto prob = sample_discrete_pdf(
                        light.elements_cdf, j * emission_tex.width + i) /
                    light.elements_cdf.back();
        auto angle = (2 * pif / emission_tex.width) *
                     (pif / emission_tex.height) *
                     sin(pif * (j + 0.5f) / emission_tex.height);
        pdf += prob / angle;
      } else {
        pdf += 1 / (4 * pif);
      }
    }
  }
  pdf *= sample_uniform_pdf((int)lights.lights.size());
  return pdf;
}

// Init a sequence of random number generators.
template <typename Scene>
trace_state make_state(const Scene& scene, const trace_params& params) {
  auto& camera = scene.cameras(params.camera);
  auto  state  = trace_state{};
  if (camera.aspect >= 1) {
    state.width  = params.resolution;
    state.height = (int)round(params.resolution / camera.aspect);
  } else {
    state.height = params.resolution;
    state.width  = (int)round(params.resolution * camera.aspect);
  }
  state.samples = 0;
  state.image.assign(state.width * state.height, {0, 0, 0, 0});
  state.albedo.assign(state.width * state.height, {0, 0, 0});
  state.normal.assign(state.width * state.height, {0, 0, 0});
  state.hits.assign(state.width * state.height, 0);
  state.rngs.assign(state.width * state.height, {});
  auto rng_ = make_rng(1301081);
  for (auto& rng : state.rngs) {
    rng = make_rng(params.seed, rand1i(rng_, 1 << 31) / 2 + 1);
  }
  return state;
}

template <typename Scene>
trace_result trace_path(const Scene& scene, const bvh_scene& bvh,
    const trace_lights& lights, const ray3f& ray_, rng_state& rng,
    const trace_params& params) {
  // initialize
  auto radiance      = vec3f{0, 0, 0};
  auto weight        = vec3f{1, 1, 1};
  auto ray           = ray_;
  auto volume_stack  = vector<material_point>{};
  auto max_roughness = 0.0f;
  auto hit           = false;
  auto hit_albedo    = vec3f{0, 0, 0};
  auto hit_normal    = vec3f{0, 0, 0};
  auto opbounce      = 0;

  // trace  path
  for (auto bounce = 0; bounce < params.bounces; bounce++) {
    // intersect next point
    auto intersection = intersect_scene(bvh, scene, ray);
    if (!intersection.hit) {
      if (bounce > 0 || !params.envhidden)
        radiance += weight * eval_environment(scene, ray.d);
      break;
    }

    // handle transmission if inside a volume
    auto in_volume = false;
    if (!volume_stack.empty()) {
      auto& vsdf     = volume_stack.back();
      auto  distance = sample_transmittance(
          vsdf.density, intersection.distance, rand1f(rng), rand1f(rng));
      weight *= eval_transmittance(vsdf.density, distance) /
                sample_transmittance_pdf(
                    vsdf.density, distance, intersection.distance);
      in_volume             = distance < intersection.distance;
      intersection.distance = distance;
    }

    // switch between surface and volume
    if (!in_volume) {
      // prepare shading point
      auto outgoing = -ray.d;
      auto position = eval_shading_position(scene, intersection, outgoing);
      auto normal   = eval_shading_normal(scene, intersection, outgoing);
      auto material = eval_material(scene, intersection);

      // correct roughness
      if (params.nocaustics) {
        max_roughness      = max(material.roughness, max_roughness);
        material.roughness = max_roughness;
      }

      // handle opacity
      if (material.opacity < 1 && rand1f(rng) >= material.opacity) {
        if (opbounce++ > 128) break;
        ray = {position + ray.d * 1e-2f, ray.d};
        bounce -= 1;
        continue;
      }

      // set hit variables
      if (bounce == 0) {
        hit        = true;
        hit_albedo = material.color;
        hit_normal = normal;
      }

      // accumulate emission
      radiance += weight * eval_emission(material, normal, outgoing);

      // next direction
      auto incoming = vec3f{0, 0, 0};
      if (!is_delta(material)) {
        if (rand1f(rng) < 0.5f) {
          incoming = sample_bsdfcos(
              material, normal, outgoing, rand1f(rng), rand2f(rng));
        } else {
          incoming = sample_lights(
              scene, lights, position, rand1f(rng), rand1f(rng), rand2f(rng));
        }
        if (incoming == vec3f{0, 0, 0}) break;
        weight *=
            eval_bsdfcos(material, normal, outgoing, incoming) /
            (0.5f * sample_bsdfcos_pdf(material, normal, outgoing, incoming) +
                0.5f *
                    sample_lights_pdf(scene, bvh, lights, position, incoming));
      } else {
        incoming = sample_delta(material, normal, outgoing, rand1f(rng));
        weight *= eval_delta(material, normal, outgoing, incoming) /
                  sample_delta_pdf(material, normal, outgoing, incoming);
      }

      // update volume stack
      if (is_volumetric(scene, intersection) &&
          dot(normal, outgoing) * dot(normal, incoming) < 0) {
        if (volume_stack.empty()) {
          auto material = eval_material(scene, intersection);
          volume_stack.push_back(material);
        } else {
          volume_stack.pop_back();
        }
      }

      // setup next iteration
      ray = {position, incoming};
    } else {
      // prepare shading point
      auto  outgoing = -ray.d;
      auto  position = ray.o + ray.d * intersection.distance;
      auto& vsdf     = volume_stack.back();

      // accumulate emission
      // radiance += weight * eval_volemission(emission, outgoing);

      // next direction
      auto incoming = vec3f{0, 0, 0};
      if (rand1f(rng) < 0.5f) {
        incoming = sample_scattering(vsdf, outgoing, rand1f(rng), rand2f(rng));
      } else {
        incoming = sample_lights(
            scene, lights, position, rand1f(rng), rand1f(rng), rand2f(rng));
      }
      if (incoming == vec3f{0, 0, 0}) break;
      weight *=
          eval_scattering(vsdf, outgoing, incoming) /
          (0.5f * sample_scattering_pdf(vsdf, outgoing, incoming) +
              0.5f * sample_lights_pdf(scene, bvh, lights, position, incoming));

      // setup next iteration
      ray = {position, incoming};
    }

    // check weight
    if (weight == vec3f{0, 0, 0} || !isfinite(weight)) break;

    // russian roulette
    if (bounce > 3) {
      auto rr_prob = min((float)0.99, max(weight));
      if (rand1f(rng) >= rr_prob) break;
      weight *= 1 / rr_prob;
    }
  }

  return {radiance, hit, hit_albedo, hit_normal};
}

template <typename Scene>
void trace_sample(trace_state& state, const Scene& scene, const bvh_scene& bvh,
    const trace_lights& lights, int i, int j, const trace_params& params) {
  auto& camera = scene.cameras(params.camera);
  // auto  sampler = get_trace_sampler_func(params);
  auto idx = state.width * j + i;
  auto ray = sample_camera(camera, {i, j}, {state.width, state.height},
      rand2f(state.rngs[idx]), rand2f(state.rngs[idx]), params.tentfilter);
  // auto [radiance, hit, albedo, normal] = sampler(
  // scene, bvh, lights, ray, state.rngs[idx], params);
  auto [radiance, hit, albedo, normal] = trace_path(
      scene, bvh, lights, ray, state.rngs[idx], params);
  if (!isfinite(radiance)) radiance = {0, 0, 0};
  if (max(radiance) > params.clamp)
    radiance = radiance * (params.clamp / max(radiance));
  if (hit) {
    state.image[idx] += {radiance.x, radiance.y, radiance.z, 1};
    state.albedo[idx] += albedo;
    state.normal[idx] += normal;
    state.hits[idx] += 1;
  } else if (!params.envhidden && scene.num_environments() == 0) {
    state.image[idx] += {radiance.x, radiance.y, radiance.z, 1};
    state.albedo[idx] += {1, 1, 1};
    state.normal[idx] += -ray.d;
    state.hits[idx] += 1;
  }
}

template <typename Scene>
void trace_samples(trace_state& state, const Scene& scene, const bvh_scene& bvh,
    const trace_lights& lights, const trace_params& params) {
  if (state.samples >= params.samples) return;
  if (params.noparallel) {
    for (auto j = 0; j < state.height; j++) {
      for (auto i = 0; i < state.width; i++) {
        trace_sample(state, scene, bvh, lights, i, j, params);
      }
    }
  } else {
    parallel_for(state.width, state.height, [&](int i, int j) {
      trace_sample(state, scene, bvh, lights, i, j, params);
    });
  }
  state.samples += 1;
}

}  // namespace yash
