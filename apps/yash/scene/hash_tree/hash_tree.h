#pragma once

#include "data_table.h"

namespace yash {

// Implementation of a hash/Merkle tree.
struct Hash_Node {
  Hash_Node*         parent   = nullptr;
  vector<Hash_Node*> children = {};
  Hash               hash     = {};
  size_t             id       = -1;

  const Hash_Node* at(size_t id) const {
    for (auto& child : children) {
      if (child->id == id) return child;
    }
    return nullptr;
  }
};

inline Hash_Node* add_node(Hash_Node* parent, size_t id = -1) {
  auto node    = new Hash_Node{};
  node->parent = parent;
  node->id     = id;
  parent->children.push_back(node);
  return node;
}

template <typename S, typename T = S>
inline Hash_Node* add_leaf_node(
    Hash_Node* parent, const T& value, Data_Table& data, size_t id = -1) {
  auto node  = add_node(parent, id);
  node->hash = data.maybe_add(value);
  return node;
}

template <typename T>
inline Hash_Node* add_leaf_node(
    Hash_Node* parent, const vector<T>& vec, Data_Table& data, size_t id = -1) {
  auto node  = add_node(parent, id);
  node->hash = data.maybe_add(vec);
  return node;
}

inline Hash make_hash(const Hash_Node* node, const Data_Table& data) {
  auto hashes = vector<byte>{};
  for (auto& c : node->children) {
    hashes.insert(hashes.end(), c->hash.begin(), c->hash.end());
  }
  return make_hash(hashes);
}

template <typename T>
inline Hash_Node* edit_node(
    const Hash_Node* edit_node, const T& value, Data_Table& data) {
  // Create new node and data.
  auto hash = make_hash(value);

  auto node  = new Hash_Node{};
  *node      = *edit_node;
  node->hash = hash;
  data.set(hash, value);

  // Move upwards creating new nodes, until root.
  while (node->parent) {
    auto index = 0;
    for (int i = 0; i < node->parent->children.size(); i++) {
      if (node->parent->children[i] == node) {
        index = i;
        break;
      }
    }

    auto parent             = new Hash_Node{};
    *parent                 = *node->parent;
    parent->children[index] = node;
    parent->hash            = make_hash(parent, data);

    node = parent;
  }

  // Return new root.
  return node;
}

inline bool update_node_hash(Hash_Node* node, const Data_Table& data) {
  assert(!node->children.empty());  // Leaf node's hash must be uptaded besed
                                    // on its content.

  // Recursive call on subtree.
  for (auto& child : node->children) {
    if (child->children.empty()) continue;
    update_node_hash(child, data);
  }

  // Hash of a non-leaf node is the hash of the concatenation of its children'
  // hashes.
  auto hashes = vector<byte>{};
  for (auto& c : node->children) {
    hashes.insert(hashes.end(), c->hash.begin(), c->hash.end());
  }
  node->hash = make_hash(hashes);
  return true;
}

inline const Hash_Node* make_diff(const Hash_Node* r0, const Hash_Node* r1) {
  auto diff_root = new Hash_Node{};
  auto stack =
      vector<std::tuple<const Hash_Node*, const Hash_Node*, Hash_Node*>>{};
  stack.push_back({r0, r1, diff_root});
  while (stack.size()) {
    auto [node0, node1, diff_node] = stack.back();
    stack.pop_back();

    for (int i = 0; i < node0->children.size(); i++) {
      auto child0 = node0->children[i];
      auto child1 = node1->at(child0->id);
      if (child1) {
        if (child0->hash == child1->hash) goto next_node;
        auto node  = add_node(diff_node, child0->id);
        node->hash = child1->hash;
        stack.push_back({child0, child1, node});
        break;
      } else {
        // child0 not found in node1, it was deleted.
        auto node  = add_node(diff_node, child0->id);
        node->hash = invalid_hash;
      }

    next_node:
      continue;
    }
  }
  return diff_root;
}

}  // namespace yash
