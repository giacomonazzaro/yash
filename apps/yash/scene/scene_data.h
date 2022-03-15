#pragma once

#include <yocto/yocto_scene.h>

namespace yash {
using namespace yocto;

struct Shape_Data {
  // enum struct shape_type { points, lines, triangles, quads };
  // shape_type type;
  // union {
  vector<int>   _points    = {};
  vector<vec2i> _lines     = {};
  vector<vec3i> _triangles = {};
  vector<vec4i> _quads     = {};
  // };

  // vertex data
  vector<vec3f> _positions = {};
  vector<vec3f> _normals   = {};
  vector<vec2f> _texcoords = {};
  vector<vec4f> _colors    = {};
  vector<float> _radius    = {};
  vector<vec4f> _tangents  = {};

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

struct Texture_Data {
  int  width  = 0;
  int  height = 0;
  bool linear = false;
  // bool hdr    = false;

  // union {
  vector<vec4f> pixelsf = {};
  vector<vec4b> pixelsb = {};
  // };
};

struct Subdiv_Data {
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
  vector<vec4i> quadspos      = {};
  vector<vec4i> quadsnorm     = {};
  vector<vec4i> quadstexcoord = {};

  // vertex data
  vector<vec3f> positions = {};
  vector<vec3f> normals   = {};
  vector<vec2f> texcoords = {};
};

struct Scene_Data {
  // scene elements
  vector<camera_data>      _cameras      = {};
  vector<instance_data>    _instances    = {};
  vector<environment_data> _environments = {};
  vector<Shape_View>       _shapes       = {};
  vector<Texture_View>     _textures     = {};
  vector<material_data>    _materials    = {};
  vector<Subdiv_View>      _subdivs      = {};

  const camera_data&      cameras(size_t i) const { return *_cameras[i]; }
  const instance_data&    instances(size_t i) const { return *_instances[i]; }
  const environment_data& environments(size_t i) const {
    return *_environments[i];
  }
  const Shape_View&    shapes(size_t i) const { return _shapes[i]; }
  const Texture_View&  textures(size_t i) const { return _textures[i]; }
  const material_data& materials(size_t i) const { return *_materials[i]; }
  const Subdiv_View&   subdivs(size_t i) const { return _subdivs[i]; }

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
