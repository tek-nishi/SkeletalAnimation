#pragma once

//
// ノード
//

#include "mesh.hpp"


struct Node {
  std::string name;

  std::vector<Mesh> mesh;

  ci::Matrix44f matrix;
  ci::Matrix44f matrix_orig;
  ci::Matrix44f global_matrix;
  ci::Matrix44f invert_matrix;

  std::vector<std::shared_ptr<Node> > children;
};


// 再帰で子供のノードも生成
std::shared_ptr<Node> createNode(const aiNode* const n, aiMesh** mesh) {
  auto node = std::make_shared<Node>();

  node->name = n->mName.C_Str();

  ci::app::console() << "Node:" << node->name << std::endl;

  for (u_int i = 0; i < n->mNumMeshes; ++i) {
    node->mesh.push_back(createMesh(mesh[n->mMeshes[i]]));
  }
  node->matrix.set(n->mTransformation[0], true);
  // 初期値を保存しておく
  node->matrix_orig = node->matrix;

  for (u_int i = 0; i < n->mNumChildren; ++i) {
    node->children.push_back(createNode(n->mChildren[i], mesh));
  }

  return node;
}

// 再帰を使って全ノード情報を生成
void createNodeInfo(const std::shared_ptr<Node>& node,
                    std::map<std::string, std::shared_ptr<Node> >& node_index,
                    std::vector<std::shared_ptr<Node> >& node_list) {

  node_index.insert(std::make_pair(node->name, node));
  node_list.push_back(node);

  for (auto child : node->children) {
    createNodeInfo(child, node_index, node_list);
  }
}


// 全ノードの親行列適用済み行列と、その逆行列を計算
//   メッシュアニメーションで利用
void updateNodeDerivedMatrix(const std::shared_ptr<Node>& node,
                             const ci::Matrix44f& parent_matrix) {
  node->global_matrix = parent_matrix * node->matrix;
  node->invert_matrix = node->global_matrix.inverted();

  for (auto child : node->children) {
    updateNodeDerivedMatrix(child, node->global_matrix);
  }
}
