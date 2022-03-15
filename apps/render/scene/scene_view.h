#pragma once

#include <yocto/yocto_scene.h>

#include "view.h"

namespace yash {
using namespace yocto;

struct Shape_View {
  // enum struct shape_type { points, lines, triangles, quads };
  // shape_type type;
  // union {
  view<int>   _points    = {};
  view<vec2i> _lines     = {};
  view<vec3i> _triangles = {};
  view<vec4i> _quads     = {};
  // };

  // vertex data
  view<vec3f> _positions = {};
  view<vec3f> _normals   = {};
  view<vec2f> _texcoords = {};
  view<vec4f> _colors    = {};
  view<float> _radius    = {};
  view<vec4f> _tangents  = {};

  const int&   points(size_t i) const { return _points[i]; }
  const vec2i& lines(size_t i) const { return _lines[i]; }
  const vec3i& triangles(size_t i) const { return _triangles[i]; }
  const vec4i& quads(size_t i) const { return _quads[i]; }
  const vec3f& positions(size_t i) const { return _positions[i]; }
  const vec3f& normals(size_t i) const { return _normals[i]; }
  const vec2f& texcoords(size_t i) const { return _texcoords[i]; }
  const vec4f& colors(size_t i) const { return _colors[i]; }
  const float& radius(size_t i) const { return _radius[i]; }
  const vec4f& tangents(size_t i) const { return _tangents[i]; }
  int&         points(size_t i) { return _points[i]; }
  vec2i&       lines(size_t i) { return _lines[i]; }
  vec3i&       triangles(size_t i) { return _triangles[i]; }
  vec4i&       quads(size_t i) { return _quads[i]; }
  vec3f&       positions(size_t i) { return _positions[i]; }
  vec3f&       normals(size_t i) { return _normals[i]; }
  vec2f&       texcoords(size_t i) { return _texcoords[i]; }
  vec4f&       colors(size_t i) { return _colors[i]; }
  float&       radius(size_t i) { return _radius[i]; }
  vec4f&       tangents(size_t i) { return _tangents[i]; }
  size_t       num_points() const { return _points.size(); }
  size_t       num_lines() const { return _lines.size(); }
  size_t       num_triangles() const { return _triangles.size(); }
  size_t       num_quads() const { return _quads.size(); }
  size_t       num_positions() const { return _positions.size(); }
  size_t       num_normals() const { return _normals.size(); }
  size_t       num_texcoords() const { return _texcoords.size(); }
  size_t       num_colors() const { return _colors.size(); }
  size_t       num_radius() const { return _radius.size(); }
  size_t       num_tangents() const { return _tangents.size(); }
};

inline Shape_View make_shape_view(const shape_data& shape) {
  auto shape_view       = Shape_View{};
  shape_view._points    = shape.points;
  shape_view._lines     = shape.lines;
  shape_view._triangles = shape.triangles;
  shape_view._quads     = shape.quads;
  shape_view._positions = shape.positions;
  shape_view._normals   = shape.normals;
  shape_view._texcoords = shape.texcoords;
  shape_view._colors    = shape.colors;
  shape_view._radius    = shape.radius;
  shape_view._tangents  = shape.tangents;
  return shape_view;
}

struct Texture_View {
  int  width  = 0;
  int  height = 0;
  bool linear = false;
  // bool hdr    = false;

  // union {
  view<vec4f> pixelsf = {};
  view<vec4b> pixelsb = {};
  // };
  // Texture_View() {}
};

inline Texture_View make_texture_view(const texture_data& texture) {
  auto texture_view    = Texture_View{};
  texture_view.width   = texture.width;
  texture_view.height  = texture.height;
  texture_view.linear  = texture.linear;
  texture_view.pixelsf = texture.pixelsf;
  texture_view.pixelsb = texture.pixelsb;
  return texture_view;
}

struct Subdiv_View {
  // subdivision data
  int  subdivisions = 0;
  bool catmullclark = true;
  bool smooth       = true;

  // displacement data
  float displacement     = 0;
  int   displacement_tex = invalidid;

  // shape reference
  int shape = invalidid;

  // face-varying primitives
  view<vec4i> quadspos      = {};
  view<vec4i> quadsnorm     = {};
  view<vec4i> quadstexcoord = {};

  // vertex data
  view<vec3f> positions = {};
  view<vec3f> normals   = {};
  view<vec2f> texcoords = {};
};

inline Subdiv_View make_subdiv_view(const subdiv_data& subdiv) {
  auto subdiv_view             = Subdiv_View{};
  subdiv_view.subdivisions     = subdiv.subdivisions;
  subdiv_view.catmullclark     = subdiv.catmullclark;
  subdiv_view.smooth           = subdiv.smooth;
  subdiv_view.displacement     = subdiv.displacement;
  subdiv_view.displacement_tex = subdiv.displacement_tex;
  subdiv_view.shape            = subdiv.shape;
  subdiv_view.quadspos         = subdiv.quadspos;
  subdiv_view.quadsnorm        = subdiv.quadsnorm;
  subdiv_view.quadstexcoord    = subdiv.quadstexcoord;
  subdiv_view.positions        = subdiv.positions;
  subdiv_view.normals          = subdiv.normals;
  subdiv_view.texcoords        = subdiv.texcoords;
  return subdiv_view;
}

struct Scene_View {
  // scene elements
  hash_map<size_t, const camera_data*>      _cameras      = {};
  hash_map<size_t, const instance_data*>    _instances    = {};
  hash_map<size_t, const environment_data*> _environments = {};
  hash_map<size_t, Shape_View>              _shapes       = {};
  hash_map<size_t, Texture_View>            _textures     = {};
  hash_map<size_t, const material_data*>    _materials    = {};
  hash_map<size_t, Subdiv_View>             _subdivs      = {};

  const camera_data&   cameras(size_t i) const { return *_cameras.at(i); }
  const instance_data& instances(size_t i) const { return *_instances.at(i); }
  const environment_data& environments(size_t i) const {
    return *_environments.at(i);
  }
  const Shape_View&    shapes(size_t i) const { return _shapes.at(i); }
  const Texture_View&  textures(size_t i) const { return _textures.at(i); }
  const material_data& materials(size_t i) const { return *_materials.at(i); }
  const Subdiv_View&   subdivs(size_t i) const { return _subdivs.at(i); }

  const size_t num_cameras() const { return _cameras.size(); }
  const size_t num_instances() const { return _instances.size(); }
  const size_t num_environments() const { return _environments.size(); }
  const size_t num_shapes() const { return _shapes.size(); }
  const size_t num_textures() const { return _textures.size(); }
  const size_t num_materials() const { return _materials.size(); }
  const size_t num_subdivs() const { return _subdivs.size(); }

  // names (this will be cleanup significantly later)
  vector<string> camera_names      = {};
  vector<string> texture_names     = {};
  vector<string> material_names    = {};
  vector<string> shape_names       = {};
  vector<string> instance_names    = {};
  vector<string> environment_names = {};
  vector<string> subdiv_names      = {};
};

}  // namespace yash
