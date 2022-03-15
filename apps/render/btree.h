#pragma once
#include "tree.h"
// #include <vector>
// using std::vector;

using Bucket = array<Type, 2 * t - 1>;

// a node has maximum 2*t - 1 keys
// a node has maximum 2*t children

template <typename Type, int t>
struct BTree {
  Tree<2 * t>    tree;
  vector<Bucket> buckets;
  int            root = -1;
};

template <typename Type, int t>
inline Bucket& bucket(BTree<Type, t>& btree, int node) {
  assert(btree.tree[node].data == node);  // so, why? If bucket will be stored
                                          // on disk, this may be useful...
  return btree.buckets[btree.tree[node].data];
}

template <typename Type, int t>
inline const Bucket& bucket(const BTree<Type, t>& btree, int node) {
  assert(btree.tree[node].data == node);
  return btree.buckets[btree.tree[node].data];
}

template <typename Type, int t>
inline array<int, 2 * t>& children(BTree<Type, t>& btree, int node) {
  return btree.tree[node].children;
}

template <typename Type, int t>
inline const array<int, 2 * t>& children(
    const BTree<Type, t>& btree, int node) {
  return btree.tree[node].children;
}

template <typename Type, int t>
inline int parent(const BTree<Type, t>& btree, int node) {
  return btree.tree[node].parent;
}

template <typename Type, int t>
inline int& parent(BTree<Type, t>& btree, int node) {
  return btree.tree[node].parent;
}

template <typename Type, int t>
int size(const BTree<Type, t>& btree, int node) {
  return btree.buckets[btree.tree[node].data].count;
}

template <typename Type, int t>
inline int add_node(BTree<Type, t>& btree, int parent) {
  int added_node = btree.tree.size();
  btree.tree.push_back(Node<2 * t>{{}, parent, (int)btree.buckets.size()});
  btree.buckets.push_back({});
  return added_node;
}

template <typename Type, int t>
inline bool is_full(const BTree<Type, t>& btree, int node) {
  return size(btree, node) == 2 * t - 1;
}

template <typename Type, int t>
inline bool is_leaf(const BTree<Type, t>& btree, int node) {
  return children(btree, node).is_empty();
}

template <typename Type, int t>
BTree<Type, t> make_btree() {
  BTree<Type, t> btree;

  return btree;
}

struct BTree_pos {
  int node;
  int pos;
};

template <typename Type, int t>
BTree_pos search(BTree<Type, t>& btree, const Type& elem, int node) {
  while (true) {
    int   i   = 0;
    auto& cmp = bucket(btree, node);
    int   s   = size(btree, node);
    while (i < s and elem > cmp[i]) {
      i += 1;
    }
    if (i < s and elem == cmp[i]) return BTree_pos{node, i};

    if (is_leaf(btree, node)) return BTree_pos{-1, -1};

    node = children(btree, node)[i];
  }
}

template <typename Type, int t>
inline BTree_pos search(BTree<Type, t>& btree, const Type& elem) {
  return search(btree, elem, btree.root);
}

template <typename Type, int t>
void split_child(BTree<Type, t>& btree, int p, int child_pos) {
  // node to split (must be full).
  int node = children(btree, p)[child_pos];
  assert(is_full(btree, node));
  assert(not is_full(btree, p));

  int new_node = add_node(btree, p);

  // @Speed: use memcpy
  for (int i = t; i < 2 * t - 1; ++i) {
    bucket(btree, new_node).add(bucket(btree, node)[i]);
  }
  if (children(btree, node).count) {
    for (int i = t; i < 2 * t; ++i) {
      children(btree, new_node).add(children(btree, node)[i]);
    }
  }
  // bucket(btree, p).insert(child_pos, bucket(btree, node)[t - 1]);
  for (int i = child_pos; i < bucket(btree, p).count; i++) {
    bucket(btree, p)[i + 1] = bucket(btree, p)[i];
  }
  bucket(btree, p).count += 1;
  bucket(btree, p)[child_pos] = bucket(btree, node)[t - 1];
  children(btree, p).insert(new_node, child_pos + 1);

  // shrink node that has been split.
  bucket(btree, node).count = t - 1;
  if (children(btree, node).count > t) children(btree, node).count = t;
}

template <typename Type, int t>
void add_element_nonfull(BTree<Type, t>& btree, int node, const Type& element) {
  int i = size(btree, node) - 1;
  if (is_leaf(btree, node)) {
    while (i >= 0 and element < bucket(btree, node)[i]) {
      // if(element == bucket(btree, node)[i]) return;
      bucket(btree, node)[i + 1] = bucket(btree, node)[i];
      i -= 1;
    }
    bucket(btree, node)[i + 1] = element;
    bucket(btree, node).count += 1;
  } else {
    while (i >= 0 and element < bucket(btree, node)[i]) {
      // if(element == bucket(btree, node)[i]) return;
      i -= 1;
    }
    i += 1;
    if (is_full(btree, children(btree, node)[i])) {
      split_child(btree, node, i);
      if (element > bucket(btree, node)[i]) i += 1;
    }
    add_element_nonfull(btree, children(btree, node)[i], element);
  }
}

template <typename Type, int t>
void add_element(BTree<Type, t>& btree, const Type& element) {
  if (btree.tree.size() == 0) {
    add_node(btree, -1);
    bucket(btree, 0).add(element);
    btree.root = 0;
    return;
  }

  int r = btree.root;
  if (is_full(btree, r)) {
    btree.root = add_node(btree, -1);
    children(btree, btree.root).add(r);
    parent(btree, r) = btree.root;
    split_child(btree, btree.root, 0);
  }
  add_element_nonfull(btree, btree.root, element);
}

// <template <typename Type, int t>
// void add_element(BTree<Type, t>& btree, const Type& elem) {
//     int node = 0;
// }
