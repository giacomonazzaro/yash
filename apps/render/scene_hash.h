#pragma once
#include "hash_tree.h"
//
#include <yocto/yocto_scene.h>

#include "scene.h"
#include "scene_view.h"

namespace yash {
using namespace yocto;

inline Hash_Node* add_shape_node(
    Hash_Node* parent, const shape_data& shape, Data_Table& data, size_t id) {
  auto node = add_node(parent, id);
  add_leaf_node(node, shape.points, data);
  add_leaf_node(node, shape.lines, data);
  add_leaf_node(node, shape.triangles, data);
  add_leaf_node(node, shape.quads, data);
  add_leaf_node(node, shape.positions, data);
  add_leaf_node(node, shape.normals, data);
  add_leaf_node(node, shape.texcoords, data);
  add_leaf_node(node, shape.colors, data);
  add_leaf_node(node, shape.radius, data);
  add_leaf_node(node, shape.tangents, data);
  return node;
}

inline Shape_View make_shape_view(
    const Hash_Node* node, const Data_Table& data) {
  auto shape_view       = Shape_View{};
  shape_view._points    = data.get_view<int>(node->children[0]->hash);
  shape_view._lines     = data.get_view<vec2i>(node->children[1]->hash);
  shape_view._triangles = data.get_view<vec3i>(node->children[2]->hash);
  shape_view._quads     = data.get_view<vec4i>(node->children[3]->hash);
  shape_view._positions = data.get_view<vec3f>(node->children[4]->hash);
  shape_view._normals   = data.get_view<vec3f>(node->children[5]->hash);
  shape_view._texcoords = data.get_view<vec2f>(node->children[6]->hash);
  shape_view._colors    = data.get_view<vec4f>(node->children[7]->hash);
  shape_view._radius    = data.get_view<float>(node->children[8]->hash);
  shape_view._tangents  = data.get_view<vec4f>(node->children[9]->hash);
  return shape_view;
}

inline Hash_Node* add_texture_node(Hash_Node* parent,
    const texture_data& texture, Data_Table& data, size_t id) {
  auto node = add_node(parent, id);
  add_leaf_node(node, texture.pixelsf, data);
  add_leaf_node(node, texture.pixelsb, data);
  add_leaf_node<texture_data>(node, texture, data);  // pixels copied too...
  return node;
}
inline Texture_View make_texture_view(
    const Hash_Node* node, const Data_Table& data) {
  auto texture_view    = Texture_View{};
  texture_view.pixelsf = data.get_view<vec4f>(node->children[0]->hash);
  if (data.get_view<vec4b>(node->children[1]->hash).size()) {
    texture_view.pixelsb = data.get_view<vec4b>(node->children[1]->hash);
    //    texture_view.hdr     = false;
  } else {
    //    texture_view.hdr = true;
    texture_view.pixelsb = data.get_view<vec4b>(node->children[0]->hash);
  }
  auto& info          = data.get<texture_data>(node->children[2]->hash);
  texture_view.width  = info.width;
  texture_view.height = info.height;
  texture_view.linear = info.linear;

  return texture_view;
}

inline Subdiv_View make_subdiv_view(
    const Hash_Node* node, const Data_Table& data) {
  auto subdiv             = Subdiv_View{};
  subdiv.quadspos         = data.get_view<vec4i>(node->children[0]->hash);
  subdiv.quadsnorm        = data.get_view<vec4i>(node->children[1]->hash);
  subdiv.quadstexcoord    = data.get_view<vec4i>(node->children[2]->hash);
  subdiv.positions        = data.get_view<vec3f>(node->children[3]->hash);
  subdiv.normals          = data.get_view<vec3f>(node->children[4]->hash);
  subdiv.texcoords        = data.get_view<vec2f>(node->children[5]->hash);
  auto info               = data.get<subdiv_data>(node->children[6]->hash);
  subdiv.subdivisions     = info.subdivisions;
  subdiv.catmullclark     = info.catmullclark;
  subdiv.smooth           = info.smooth;
  subdiv.displacement     = info.displacement;
  subdiv.displacement_tex = info.displacement_tex;
  return subdiv;
};
inline Hash_Node* add_subdiv_node(
    Hash_Node* parent, const subdiv_data& subdiv, Data_Table& data, size_t id) {
  auto node = add_node(parent, id);
  add_leaf_node(node, subdiv.quadspos, data);
  add_leaf_node(node, subdiv.quadsnorm, data);
  add_leaf_node(node, subdiv.quadstexcoord, data);
  add_leaf_node(node, subdiv.positions, data);
  add_leaf_node(node, subdiv.normals, data);
  add_leaf_node(node, subdiv.texcoords, data);
  add_leaf_node<subdiv_data>(node, subdiv, data);
  return node;
}

struct Scene_Hash {
  // private:
 public:
  const Hash_Node* root;
  Data_Table&      data;

 public:
  Scene_Hash(const Hash_Node* r, Data_Table& d) : root(r), data(d) {}

  const vector<Hash_Node*>& cameras() const {
    return root->children[0]->children;
  };
  const vector<Hash_Node*>& instances() const {
    return root->children[1]->children;
  };
  const vector<Hash_Node*>& environments() const {
    return root->children[2]->children;
  };
  const vector<Hash_Node*>& shapes() const {
    return root->children[3]->children;
  };
  const vector<Hash_Node*>& textures() const {
    return root->children[4]->children;
  };
  const vector<Hash_Node*>& materials() const {
    return root->children[5]->children;
  };
  const vector<Hash_Node*>& subdivs() const {
    return root->children[6]->children;
  };

  size_t num_cameras() const {
    if (root->children.size() <= 0) return 0;
    return root->children[0]->children.size();
  }
  size_t num_instances() const {
    if (root->children.size() <= 1) return 0;
    return root->children[1]->children.size();
  }
  size_t num_environments() const {
    if (root->children.size() <= 2) return 0;
    return root->children[2]->children.size();
  }
  size_t num_shapes() const {
    if (root->children.size() <= 3) return 0;
    return root->children[3]->children.size();
  }
  size_t num_textures() const {
    if (root->children.size() <= 4) return 0;
    return root->children[4]->children.size();
  }
  size_t num_materials() const {
    if (root->children.size() <= 5) return 0;
    return root->children[5]->children.size();
  }
  size_t num_subdivs() const {
    if (root->children.size() <= 6) return 0;
    return root->children[6]->children.size();
  }

  const auto& cameras(size_t i) const {
    auto node = root->children[0]->children[i];
    return data.get<camera_data>(node->hash);
  }
  const auto& instances(size_t i) const {
    auto node = root->children[1]->children[i];
    return data.get<instance_data>(node->hash);
  }
  const auto& environments(size_t i) const {
    auto node = root->children[2]->children[i];
    return data.get<environment_data>(node->hash);
  }
  const Shape_View shapes(size_t i) const {
    auto node = root->children[3]->children[i];
    return make_shape_view(node, data);
  }
  const Texture_View textures(size_t i) const {
    auto node = root->children[4]->children[i];
    return make_texture_view(node, data);
  }
  const auto& materials(size_t i) const {
    auto node = root->children[5]->children[i];
    return data.get<material_data>(node->hash);
  }
  const Subdiv_View subdivs(size_t i) const {
    auto node = root->children[6]->children[i];
    return make_subdiv_view(node, data);
  }

  template <typename T>
  inline Scene_Hash edit(const Hash_Node* node, const T& value) {
    auto new_root = edit_node(node, value, data);
    return Scene_Hash(new_root, data);
  }
};

inline Scene_Hash create_scene_hash(const scene_data& scene, Data_Table& data) {
  auto root         = new Hash_Node{};
  auto cameras      = add_node(root, 0);
  auto instances    = add_node(root, 1);
  auto environments = add_node(root, 2);
  auto shapes       = add_node(root, 3);
  auto textures     = add_node(root, 4);
  auto materials    = add_node(root, 5);
  auto subdivs      = add_node(root, 6);

  for (int i = 0; i < scene.cameras.size(); i++) {
    auto& camera = scene.cameras[i];
    add_leaf_node<camera_data>(cameras, camera, data, i);
  }
  for (int i = 0; i < scene.instances.size(); i++) {
    auto& instance = scene.instances[i];
    add_leaf_node<instance_data>(instances, instance, data, i);
  }
  for (int i = 0; i < scene.environments.size(); i++) {
    auto& environment = scene.environments[i];
    add_leaf_node<environment_data>(environments, environment, data, i);
  }
  for (int i = 0; i < scene.shapes.size(); i++) {
    auto& shape = scene.shapes[i];
    add_shape_node(shapes, shape, data, i);
  }
  for (int i = 0; i < scene.textures.size(); i++) {
    auto& texture = scene.textures[i];
    add_texture_node(textures, texture, data, i);
  }
  for (int i = 0; i < scene.materials.size(); i++) {
    auto& material = scene.materials[i];
    add_leaf_node<material_data>(materials, material, data, i);
  }
  for (int i = 0; i < scene.subdivs.size(); i++) {
    auto& subdiv = scene.subdivs[i];
    add_subdiv_node(subdivs, subdiv, data, i);
  }
  update_node_hash(root, data);
  return Scene_Hash(root, data);
}

inline Scene_Hash make_diff(
    const Scene_Hash& scene0, const Scene_Hash& scene1) {
  return Scene_Hash(make_diff(scene0.root, scene1.root), scene0.data);
}

inline void apply_diff(Scene_View& scene, const Scene_Hash& diff) {
  for (int i = 0; i < diff.num_cameras(); i++) {
    auto node = diff.cameras()[i];
    auto id   = node->id;
    if (node->hash == invalid_hash) {
      scene._cameras.erase(id);
    } else {
      scene._cameras[id] = &diff.cameras(i);
    }
  }
  for (int i = 0; i < diff.num_instances(); i++) {
    auto node = diff.instances()[i];
    auto id   = node->id;
    if (node->hash == invalid_hash) {
      scene._instances.erase(id);
    } else {
      scene._instances[id] = &diff.instances(i);
    }
  }
  for (int i = 0; i < diff.num_environments(); i++) {
    auto node = diff.environments()[i];
    auto id   = node->id;
    if (node->hash == invalid_hash) {
      scene._environments.erase(id);
    } else {
      scene._environments[id] = &diff.environments(i);
    }
  }
  for (int i = 0; i < diff.num_shapes(); i++) {
    auto node = diff.shapes()[i];
    auto id   = node->id;
    if (node->hash == invalid_hash) {
      scene._shapes.erase(id);
    } else {
      scene._shapes[id] = diff.shapes(i);
    }
  }
  for (int i = 0; i < diff.num_textures(); i++) {
    auto node = diff.textures()[i];
    auto id   = node->id;
    if (node->hash == invalid_hash) {
      scene._textures.erase(id);
    } else {
      scene._textures[id] = diff.textures(i);
    }
  }
  for (int i = 0; i < diff.num_materials(); i++) {
    auto node = diff.materials()[i];
    auto id   = node->id;
    if (node->hash == invalid_hash) {
      scene._materials.erase(id);
    } else {
      scene._materials[id] = &diff.materials(i);
    }
  }
  for (int i = 0; i < diff.num_subdivs(); i++) {
    auto node = diff.subdivs()[i];
    auto id   = node->id;
    if (node->hash == invalid_hash) {
      scene._subdivs.erase(id);
    } else {
      scene._subdivs[id] = diff.subdivs(i);
    }
  }
}

inline Scene_View create_scene_view(const scene_data& scene) {
  auto scene_view = Scene_View{};
  scene_view._cameras.reserve(scene.cameras.size());
  scene_view._instances.reserve(scene.instances.size());
  scene_view._environments.reserve(scene.environments.size());
  scene_view._shapes.reserve(scene.shapes.size());
  scene_view._textures.reserve(scene.textures.size());
  scene_view._materials.reserve(scene.materials.size());
  scene_view._subdivs.reserve(scene.subdivs.size());

  for (int i = 0; i < scene.cameras.size(); i++) {
    scene_view._cameras[i] = &scene.cameras[i];
  }
  for (int i = 0; i < scene.instances.size(); i++) {
    scene_view._instances[i] = &scene.instances[i];
  }
  for (int i = 0; i < scene.environments.size(); i++) {
    scene_view._environments[i] = &scene.environments[i];
  }
  for (int i = 0; i < scene.shapes.size(); i++) {
    scene_view._shapes[i] = make_shape_view(scene.shapes[i]);
  }
  for (int i = 0; i < scene.textures.size(); i++) {
    scene_view._textures[i] = make_texture_view(scene.textures[i]);
  }
  for (int i = 0; i < scene.materials.size(); i++) {
    scene_view._materials[i] = &scene.materials[i];
  }
  for (int i = 0; i < scene.subdivs.size(); i++) {
    scene_view._subdivs[i] = make_subdiv_view(scene.subdivs[i]);
  }
  return scene_view;
}

inline Scene_View create_scene_view(const Scene_Hash& scene) {
  auto scene_view = Scene_View{};
  scene_view._cameras.reserve(scene.cameras().size());
  scene_view._instances.reserve(scene.instances().size());
  scene_view._environments.reserve(scene.environments().size());
  scene_view._shapes.reserve(scene.shapes().size());
  scene_view._textures.reserve(scene.textures().size());
  scene_view._materials.reserve(scene.materials().size());
  scene_view._subdivs.reserve(scene.subdivs().size());

  for (int i = 0; i < scene.num_cameras(); i++) {
    scene_view._cameras[i] = &scene.cameras(i);
  }
  for (int i = 0; i < scene.num_instances(); i++) {
    scene_view._instances[i] = &scene.instances(i);
  }
  for (int i = 0; i < scene.num_environments(); i++) {
    scene_view._environments[i] = &scene.environments(i);
  }
  for (int i = 0; i < scene.num_shapes(); i++) {
    scene_view._shapes[i] = scene.shapes(i);
  }
  for (int i = 0; i < scene.num_textures(); i++) {
    scene_view._textures[i] = scene.textures(i);
  }
  for (int i = 0; i < scene.num_materials(); i++) {
    scene_view._materials[i] = &scene.materials(i);
  }
  for (int i = 0; i < scene.num_subdivs(); i++) {
    scene_view._subdivs[i] = scene.subdivs(i);
  }
  return scene_view;
}

}  // namespace yash
