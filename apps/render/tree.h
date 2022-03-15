#pragma once
// #include "../utils.h"
// #include "../basic/array.h"
#include <array>
#include <functional>
#include <vector>

// A tree is a std::vector of nodes.
// Array for each node must be stored in an array.
// node.data is just the index of the data in the array.

template <int b>  // max branching factor
struct Node {
  std::array<int, b> children;
  int                parent = -1;
  int                data   = -1;
};

template <int b>
using Tree = std::vector<Node<b>>;

template <int b>
void add_child(Tree<b>& tree, int node, int child_data) {
  tree[node].children.add(tree.size());
  tree.push_back(Node<b>{{}, node, child_data});
}

template <int b>
void remove_node(Tree<b>& tree, int node) {
  int   parent   = tree[node].parent;
  auto& children = tree[parent].children;
  for (int i = 0; i < children.count; ++i) {
    if (children[i] == node) {
      children.remove(i);
    }
  }
}

template <int b>
void remove_subtree_recursive(Tree<b>& tree, int node) {
  for (int i = 0; i < tree[node].children.count; ++i)
    remove_subtree_recursive(tree, tree[node].children[i]);

  tree[node].parent   = -1;
  tree[node].data     = -1;
  tree[node].children = {};
}

template <int b>
void remove_subtree(Tree<b>& tree, int node) {
  remove_node(tree, node);
  remove_subtree_recursive(tree, node);
}

template <int b>
void remove_child(Tree<b>& tree, int parent, int child) {
  auto& children = tree[parent].children;
  for (int i = 0; i < children.count; ++i) {
    if (children[i] == child) tree[parent].children.remove(i);
  }
}

template <int b>
int depth_first_search(
    const Tree<b>& tree, int start, std::function<bool(int)> is_solution) {
  std::vector<int> queue = {start};

  while (not queue.empty()) {
    int node = queue.back();
    queue.pop_back();
    if (is_solution(tree[node].data)) return node;

    for (int i = 0; i < tree[node].children.count; ++i)
      queue.push_back(tree[node].children[i]);
  }

  return -1;
}

template <int b>
void depth_first_search_visit(
    Tree<b>& tree, int start, std::function<bool(Tree<b>&, int)> visit) {
  std::vector<int> queue = {start};

  while (not queue.empty()) {
    int node = queue.back();
    queue.pop_back();
    bool terminate = visit(tree, node);
    if (terminate) return;

    for (int child : tree[node].children) queue.push_back(child);
  }
}

template <int b>
// Make random tree to test algorithms.
void grow_random_tree(
    Tree<b>& tree, int node, int max_branching, int max_nodes) {
  if (tree.size() > max_nodes) return;
  int num_children = rand() % max_branching;
  if (node == 0) num_children = 4;
  if (num_children <= 1) {
    tree[node].data = rand() % 26;
  } else {
    for (int i = 0; i < num_children; ++i) {
      add_child(tree, node, -1);
      grow_random_tree(tree, tree.size() - 1, max_branching, max_nodes);
    }
  }
}

template <int b>
Tree<b> make_random_tree(int seed, int max_branching, int max_nodes) {
  srand(seed);
  Tree<b> tree;
  tree.push_back(Node<b>{{}, -1, -1});
  grow_random_tree(tree, 0, max_branching, max_nodes);
  return tree;
}

// Saving and drawing tree
template <int b>
std::string string_tree(
    const Tree<b>& tree, std::function<std::string(int)> string_data) {
  std::string result = "digraph G {";

  for (int i = 0; i < tree.size(); ++i) {
    if (tree[i].data != -1)
      result += format(
          "%d [label=\"%d\n%s\"]", i, i, string_data(tree[i].data).c_str());

    for (int j = 0; j < tree[i].children.count; ++j) {
      int child = tree[i].children[j];
      // if nodes[child].edge_data != None:
      // tree += "%s -> %s [label=\"%s\"]\n" % (i, child,
      // nodes[child].edge_data)
      result += format("%d -> %d \n", i, child);
    }
  }
  result += "}\n";
  return result;
}

template <int b, typename Type>
void binary_tree_add(Tree<b>& tree, const std::vector<Type>& data, int i) {
  int node = 0;
  while (true) {
    if (tree[node].children.is_empty()) {
      add_child(tree, node, i);
      break;
    }

    int left = data[i] < data[tree[node].data];
    if (tree[node].children.count == 1) {
      if ((data[tree[tree[node].children[0]].data] < data[tree[node].data]) !=
          left) {
        if (left)
          tree[node].children = {(int)tree.size(), tree[node].children[0]};
        else
          tree[node].children.add(tree.size());

        tree.push_back(Node<b>{{}, node, i});
        break;
      }
      node = tree[node].children[0];
    } else
      node = tree[node].children[left ? 0 : 1];
  }
}

template <typename Type>
Tree<2> build_binary_tree(const std::vector<Type>& data) {
  Tree<2> tree;
  tree.reserve(data.size());
  tree.push_back(Node<2>{{}, -1, 0});

  for (int i = 1; i < data.size(); ++i) {
    binary_tree_add(tree, data, i);
  }

  return tree;
}

template <int b>
void save_tree(const Tree<b>& tree, std::string filename,
    std::function<std::string(int)> string_data) {
  FILE* file = fopen(filename.c_str(), "w");
  if (not file) return;
  auto stree = string_tree(tree, string_data);
  fprintf(file, "%s", stree.c_str());
  fclose(file);
}

void draw_tree(const std::string& filename, const std::string& outname) {
  std::string cmd = "dot -Tpng " + filename + " > " + outname;
  system(cmd.c_str());
}

template <int b>
void draw_tree(const Tree<b>& tree, const std::string& outname,
    std::function<std::string(int)> string_data) {
  save_tree(tree, outname + ".txt", string_data);
  draw_tree(outname + ".txt", outname + ".png");
}
