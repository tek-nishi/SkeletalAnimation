#pragma once

//
// assimpによるモデル読み込み
//

#include "cinder/Matrix44.h"
#include "cinder/TriMesh.h"
#include "cinder/gl/Material.h"
#include "cinder/gl/Texture.h"
#include "cinder/ImageIo.h"
#include "cinder/AxisAlignedBox.h"

#include "assimp/Importer.hpp"      // C++ importer interface
#include "assimp/scene.h"           // Output data structure
#include "assimp/postprocess.h"     // Post processing flags


// ライブラリ読み込み指定
#if defined (_MSC_VER)
#if defined (_DEBUG)
#pragma comment(lib, "assimp-mtd")
#else
#pragma comment(lib, "assimp-mt")
#endif
#endif

// ウェイト値が小さすぎる場合への対処
#define WEIGHT_WORKAROUND

#include <map>
#include <set>
#include <limits>
#include "misc.hpp"


struct Material {
  Material()
    : has_texture(false)
  {
    // FIXME:OpenGL ES対策
    body.setFace(GL_FRONT_AND_BACK);
  }
  
  ci::gl::Material body;

  bool has_texture;
  std::string texture_name;
};

struct Weight {
  u_int vertex_id;
  float value;
};

struct Bone {
  std::string name;
  ci::Matrix44f offset;
  
  std::vector<Weight> weights;
};

struct Mesh {
  Mesh()
    : has_bone(false)
  {}

  ci::TriMesh body;
  ci::TriMesh orig;

  u_int material_index;
  
  bool has_bone;
  std::vector<Bone> bones;
};

struct Node {
  std::string name;
  
  std::vector<Mesh> mesh;
  
  ci::Matrix44f matrix;
  ci::Matrix44f global_matrix;
  ci::Matrix44f invert_matrix;

  std::vector<std::shared_ptr<Node> > children;
};


struct VectorKey {
  double time;
  ci::Vec3f value;
};
  
struct QuatKey {
  double time;
  ci::Quatf value;
};

struct NodeAnim {
  std::string node_name;

  std::vector<VectorKey> translate;
  std::vector<VectorKey> scaling;
  std::vector<QuatKey>   rotation;
};

struct Anim {
  double duration;
  std::vector<NodeAnim> body;
};


struct Model {
  std::vector<Material> material;

  // マテリアルからのテクスチャ参照は名前引き
  std::map<std::string, ci::gl::TextureRef> textures;

  // 親子関係にあるノード
  std::shared_ptr<Node> node;

  // 名前からノードを探す用(アニメーションで使う)
  std::map<std::string, std::shared_ptr<Node> > node_index;

  // 親子関係を解除した状態(全ノードの行列を更新する時に使う)
  std::vector<std::shared_ptr<Node> > node_list;

  bool has_anim;
  std::vector<Anim> animation;

  ci::AxisAlignedBox3f aabb;
};


// assimp -> Cinder へ変換する関数群
ci::Vec3f fromAssimp(const aiVector3D& v) {
  return ci::Vec3f{ v.x, v.y, v.z };
}

ci::ColorA fromAssimp(const aiColor3D& col) {
  return ci::ColorA{ col.r, col.g, col.b };
}

ci::ColorA fromAssimp(const aiColor4D& col) {
  return ci::ColorA{ col.r, col.g, col.b, col.a };
}

VectorKey fromAssimp(const aiVectorKey& key) {
  VectorKey v;

  v.time  = key.mTime;
  v.value = fromAssimp(key.mValue);
  
  return v;
}

QuatKey fromAssimp(const aiQuatKey& key) {
  QuatKey v;

  v.time  = key.mTime;
  v.value = ci::Quatf{ key.mValue.w, key.mValue.x, key.mValue.y, key.mValue.z };
  
  return v;
}


// ぼーんの情報を作成
Bone createBone(const aiBone* b) {
  Bone bone;

  bone.name = b->mName.C_Str();
  bone.offset.set(b->mOffsetMatrix[0], true);

  ci::app::console() << "bone:" << bone.name << " weights:" << b->mNumWeights << std::endl;
  
  const aiVertexWeight* w = b->mWeights;
  for (u_int i = 0; i < b->mNumWeights; ++i) {
    Weight weight{ w[i].mVertexId, w[i].mWeight };
    bone.weights.push_back(weight);
  }
  
  return bone;
}

// メッシュを生成
Mesh createMesh(const aiMesh* const m) {
  Mesh mesh;
      
  // 頂点データを取り出す
  u_int num_vtx = m->mNumVertices;
  ci::app::console() << "Vertices:" << num_vtx << std::endl;
      
  const aiVector3D* vtx = m->mVertices;
  for (u_int h = 0; h < num_vtx; ++h) {
    mesh.body.appendVertex(fromAssimp(vtx[h]));
  }

  // 法線
  if (m->HasNormals()) {
    ci::app::console() << "Has Normals." << std::endl;

    const aiVector3D* normal = m->mNormals;
    for (u_int h = 0; h < num_vtx; ++h) {
      mesh.body.appendNormal(fromAssimp(normal[h]));
    }
  }

  // テクスチャ座標(マルチテクスチャには非対応)
  if (m->HasTextureCoords(0)) {
    ci::app::console() << "Has TextureCoords." << std::endl;

    const aiVector3D* uv = m->mTextureCoords[0];
    for (u_int h = 0; h < num_vtx; ++h) {
      mesh.body.appendTexCoord(ci::Vec2f(uv[h].x, uv[h].y));
    }
  }

  // 頂点カラー(マルチカラーには非対応)
  if (m->HasVertexColors(0)) {
    ci::app::console() << "Has VertexColors." << std::endl;

    const aiColor4D* color = m->mColors[0];
    for (u_int h = 0; h < num_vtx; ++h) {
      mesh.body.appendColorRgba(fromAssimp(color[h]));
    }
  }

  // 面情報
  if (m->HasFaces()) {
    ci::app::console() << "Has Faces." << std::endl;

    const aiFace* face = m->mFaces;
    for (u_int h = 0; h < m->mNumFaces; ++h) {
      assert(face[h].mNumIndices == 3);

      const u_int* indices = face[h].mIndices;
      mesh.body.appendTriangle(indices[0], indices[1], indices[2]);
    }
  }

  // 骨情報
  mesh.has_bone = m->HasBones();
  if (mesh.has_bone) {
    ci::app::console() << "Has Bones." << std::endl;

    aiBone** b = m->mBones;
    for (u_int i = 0; i < m->mNumBones; ++i) {
      mesh.bones.push_back(createBone(b[i]));
    }

    // 骨アニメーションで書き換えるので、元を保存
    mesh.orig = mesh.body;
  }

  mesh.material_index = m->mMaterialIndex;

  return mesh;
}

// マテリアルを生成
Material createMaterial(const aiMaterial* const mat) {
  Material material;

  {
    aiColor3D color;
    mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
    material.body.setDiffuse(fromAssimp(color));
  }

  {
    aiColor3D color;
    mat->Get(AI_MATKEY_COLOR_AMBIENT, color);
    material.body.setAmbient(fromAssimp(color));
  }

  {
    aiColor3D color;
    mat->Get(AI_MATKEY_COLOR_SPECULAR, color);
    material.body.setSpecular(fromAssimp(color));
  }

  float shininess = 80.0f;
  mat->Get(AI_MATKEY_SHININESS, shininess);
  material.body.setShininess(shininess);

  {
    aiColor3D color;
    mat->Get(AI_MATKEY_COLOR_EMISSIVE, color);
    material.body.setEmission(fromAssimp(color));
  }

  aiString tex_name;
  if (mat->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), tex_name) == AI_SUCCESS) {
    material.has_texture  = true;
    material.texture_name = getFilename(std::string(tex_name.C_Str()));
  }
      
  ci::app::console() << "Diffuse:"   << material.body.getDiffuse()   << std::endl;
  ci::app::console() << "Ambient:"   << material.body.getAmbient()   << std::endl;
  ci::app::console() << "Specular:"  << material.body.getSpecular()  << std::endl;
  ci::app::console() << "Shininess:" << material.body.getShininess() << std::endl;
  ci::app::console() << "Emission:"  << material.body.getEmission()  << std::endl;
  if (material.has_texture) {
    ci::app::console() << "Texture:" << material.texture_name << std::endl;
  }

  return material;
}

// テクスチャを読み込む
std::map<std::string, ci::gl::TextureRef> loadTexrture(const Model& model) {
  std::map<std::string, ci::gl::TextureRef> textures;

  for (const auto& mat : model.material) {
    if (!mat.has_texture) continue;

    if (!model.textures.count(mat.texture_name)) {
      textures.insert(std::make_pair(mat.texture_name,
                                     ci::gl::Texture::create(ci::loadImage(ci::app::loadAsset(mat.texture_name)))));
    }
  }
  
  return textures;
}

// ノードに付随するアニメーション情報を作成
NodeAnim createNodeAnim(const aiNodeAnim* anim) {
  NodeAnim animation;

  animation.node_name = anim->mNodeName.C_Str();
  
  // 平行移動
  for (u_int i = 0; i < anim->mNumPositionKeys; ++i) {
    animation.translate.push_back(fromAssimp(anim->mPositionKeys[i]));
  }

  // スケーリング
  for (u_int i = 0; i < anim->mNumScalingKeys; ++i) {
    animation.scaling.push_back(fromAssimp(anim->mScalingKeys[i]));
  }

  // 回転
  for (u_int i = 0; i < anim->mNumRotationKeys; ++i) {
    animation.rotation.push_back(fromAssimp(anim->mRotationKeys[i]));
  }
  
  return animation;
}

// アニメーション情報を作成
Anim createAnimation(const aiAnimation* anim) {
  Anim animation;

  animation.duration = anim->mDuration;

  {
    // 階層アニメーション
    ci::app::console() << "Node anim:" << anim->mNumChannels << std::endl;

    aiNodeAnim** node_anim = anim->mChannels;
    for (u_int i = 0; i < anim->mNumChannels; ++i) {
      animation.body.push_back(createNodeAnim(node_anim[i]));
    }
  }

  {
    // メッシュアニメーション
    ci::app::console() << "Mesh anim:" << anim->mNumMeshChannels << std::endl;

#if 0
    aiMeshAnim** mesh_anim = anim->mMeshChannels;
    for (u_int i = 0; i < anim->mNumMeshChannels; ++i) {
      
    }
#endif
  }
  
  return animation;
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

// 再帰で子供のノードも生成
std::shared_ptr<Node> createNode(const aiNode* const n, aiMesh** mesh) {
  auto node = std::make_shared<Node>();

  node->name = n->mName.C_Str();

  ci::app::console() << "Node:" << node->name << std::endl;

  for (u_int i = 0; i < n->mNumMeshes; ++i) {
    node->mesh.push_back(createMesh(mesh[n->mMeshes[i]]));
  }
  node->matrix.set(n->mTransformation[0], true);

  for (u_int i = 0; i < n->mNumChildren; ++i) {
    node->children.push_back(createNode(n->mChildren[i], mesh));
  }
  
  return node;
}

#if defined (WEIGHT_WORKAROUND)

// メッシュのウェイトを正規化
//   ウェイト編集時にウェイトが極端に小さい頂点が発生しうる
//   その場合に見た目におかしくなってしまうのをいい感じに直す
void normalizeMeshWeight(Model& model) {
  for (const auto& node : model.node_list) {
    for (auto& mesh : node->mesh) {
      if (!mesh.has_bone) continue;

      // 各頂点へのウェイト書き込みを記録
      std::set<u_int> weight_index;
      std::multimap<u_int, Weight*> weight_values;
      
      for (auto& bone : mesh.bones) {
        for (auto& weight : bone.weights) {
          weight_index.insert(weight.vertex_id);
          weight_values.emplace(weight.vertex_id, &weight);
        }
      }

      // 正規化
      for (const auto i : weight_index) {
        const auto p = weight_values.equal_range(i);
        float weight = 0.0f;
        for (auto it = p.first; it != p.second; ++it) {
          weight += it->second->value;
        }
        assert(weight > 0.0f);

        {
          // ci::app::console() << "Weight min: " << weight << std::endl;
          float n = 1.0f / weight;
          for (auto it = p.first; it != p.second; ++it) {
            it->second->value *= n;
          }
        }
      }
    }
  }
}

#endif

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

// モデルの全頂点数とポリゴン数を数える
std::pair<size_t, size_t> getMeshInfo(const Model& model) {
  size_t vertex_num   = 0;
  size_t triangle_num = 0;
  
  for (const auto& node : model.node_list) {
    for (const auto& mesh : node->mesh) {
      vertex_num += mesh.body.getNumVertices();
      triangle_num += mesh.body.getNumIndices() / 3;
    }
  }

  return std::make_pair(vertex_num, triangle_num);
}


// FIXME:まさかのプロトタイプ宣言
void updateModel(Model& model, const double time, const size_t index);


// ざっくりAABBを求める
//   アニメーションで変化するのは考慮しない
ci::AxisAlignedBox3f calcAABB(Model& model) {
  // ノードの行列を更新
  updateNodeDerivedMatrix(model.node, ci::Matrix44f::identity());

  // スケルタルアニメーションを考慮
  updateModel(model, 0.0, 0);
  
  // 最小値を格納する値にはその型の最大値を
  // 最大値を格納する値にはその型の最小値を
  // それぞれ代入しておく
  float max_value = std::numeric_limits<float>::max();
  ci::Vec3f min_vtx{ max_value, max_value, max_value };
  
  float min_value = std::numeric_limits<float>::min();
  ci::Vec3f max_vtx{ min_value, min_value, min_value };

  // 全頂点を調べてAABBの頂点座標を割り出す
  for (const auto& node : model.node_list) {
    for (const auto& mesh : node->mesh) {
      const auto& verticies = mesh.body.getVertices();
      for (const auto v : verticies) {
        // ノードの行列でアフィン変換
        ci::Vec3f tv = node->global_matrix * v;
        
        min_vtx.x = std::min(tv.x, min_vtx.x);
        min_vtx.y = std::min(tv.y, min_vtx.y);
        min_vtx.z = std::min(tv.z, min_vtx.z);

        max_vtx.x = std::max(tv.x, max_vtx.x);
        max_vtx.y = std::max(tv.y, max_vtx.y);
        max_vtx.z = std::max(tv.z, max_vtx.z);
      }
    }
  }

  return ci::AxisAlignedBox3f(min_vtx, max_vtx);
}



// モデル読み込み
Model loadModel(const std::string& path) {
  Assimp::Importer importer;

  const aiScene* scene = importer.ReadFile(ci::app::getAssetPath(path).string(), 
                                           aiProcess_Triangulate
                                           | aiProcess_FlipUVs
                                           | aiProcess_RemoveRedundantMaterials);

  Model model;
  
  if (scene->HasMaterials()) {
    u_int num = scene->mNumMaterials;
    ci::app::console() << "Materials:" << num << std::endl;
    
    aiMaterial** mat = scene->mMaterials;
    for (u_int i = 0; i < num; ++i) {
      model.material.push_back(createMaterial(mat[i]));
    }

    model.textures = loadTexrture(model);
  }
  
  model.node = createNode(scene->mRootNode, scene->mMeshes);

  // ノードを名前から探せるようにする
  createNodeInfo(model.node,
                 model.node_index,
                 model.node_list);

  model.has_anim = scene->HasAnimations();
  if (model.has_anim) {
    ci::app::console() << "Animations:" << scene->mNumAnimations << std::endl;
    
    aiAnimation** anim = scene->mAnimations;
    for (u_int i = 0; i < scene->mNumAnimations; ++i) {
      model.animation.push_back(createAnimation(anim[i]));
    }
  }

#if defined (WEIGHT_WORKAROUND)
  normalizeMeshWeight(model);
#endif

  model.aabb = calcAABB(model);
  
  auto info = getMeshInfo(model);
  
  ci::app::console() << "Total vertex num:" << info.first << " triangle num:" << info.second << std::endl;
  
  return model;
}


template <typename T>
struct Comp {
  bool operator()(const double lhs, const T& rhs) { return lhs < rhs.time; }
};

// キーフレームから直線補間した値を取り出す
ci::Vec3f getLerpValue(const double time, const std::vector<VectorKey>& values) {
  // 適用キー位置を探す
  auto result = std::upper_bound(values.begin(), values.end(),
                                 time, Comp<VectorKey>());
  
  ci::Vec3f value;
  if (result == values.begin()) {
    // 先頭より小さい時間
    value = result->value;
  }
  else if (result == values.end()) {
    // 最後尾より大きい時間
    value = (result - 1)->value;
  }
  else {
    // 直線補間
    double dt = result->time - (result - 1)->time;
    double t  = time - (result - 1)->time;
    value = (result - 1)->value.lerp(t / dt, result->value);
  }

  return value;
}

ci::Quatf getLerpValue(const double time, const std::vector<QuatKey>& values) {
  auto result = std::upper_bound(values.begin(), values.end(),
                                 time, Comp<QuatKey>());

  ci::Quatf value;
  if (result == values.begin()) {
    // 先頭より小さい時間
    value = result->value;
  }
  else if (result == values.end()) {
    // 最後尾より大きい時間
    value = (result - 1)->value;
  }
  else {
    // 球面補間
    double dt = result->time - (result - 1)->time;
    double t = time - (result - 1)->time;
    value = (result - 1)->value.slerp(t / dt, result->value);
  }
  
  return value;
}

// 階層アニメーション用の行列を計算
void updateNodeMatrix(Model& model, const double time, const Anim& animation) {
  for (const auto& body : animation.body) {
    ci::Matrix44f matrix;

    // 階層アニメーションを取り出して行列を生成
    ci::Vec3f transtate = getLerpValue(time, body.translate);
    matrix.translate(transtate);

    ci::Quatf rotation = getLerpValue(time, body.rotation);
    matrix *= rotation;

    ci::Vec3f scaling = getLerpValue(time, body.scaling);
    matrix.scale(scaling);

    // ノードの行列を書き換える
    auto node = model.node_index.at(body.node_name);
    node->matrix = matrix;
  }
}


void updateMesh(Model& model) {
  for (const auto& node : model.node_list) {
    for (auto& mesh : node->mesh) {
      if (!mesh.has_bone) continue;

      // 座標変換に必要な行列を用意
      std::vector<ci::Matrix44f> bone_matrix;
      bone_matrix.reserve(mesh.bones.size());
      for (auto& bone : mesh.bones) {
        auto local_node = model.node_index.at(bone.name);
        bone_matrix.push_back(node->invert_matrix * local_node->global_matrix * bone.offset);
      }

      // 変換結果を書き出す頂点配列
      auto& body_vtx    = mesh.body.getVertices();
      auto& body_normal = mesh.body.getNormals();

      std::fill(body_vtx.begin(), body_vtx.end(), ci::Vec3f::zero());
      if (mesh.body.hasNormals()) {
        std::fill(body_normal.begin(), body_normal.end(), ci::Vec3f::zero());
      }
      
      // オリジナルの頂点データ
      const auto& orig_vtx    = mesh.orig.getVertices();
      const auto& orig_normal = mesh.orig.getNormals();

      // 全頂点の座標を再計算
      for (u_int i = 0; i < mesh.bones.size(); ++i) {
        const auto& bone = mesh.bones[i];
        const auto& m    = bone_matrix[i];

        for (const auto& weight : bone.weights) {
          body_vtx[weight.vertex_id] += weight.value * (m * orig_vtx[weight.vertex_id]);
        }

        if (mesh.body.hasNormals()) {
          for (const auto& weight : bone.weights) {
            body_normal[weight.vertex_id] += weight.value * m.transformVec(orig_normal[weight.vertex_id]);
          }
        }
      }
    }
  }
}

// アニメーションによるノード更新
void updateModel(Model& model, const double time, const size_t index) {
  if (!model.has_anim) return;

  // 最大時間でループさせている
  double current_time = std::fmod(time, model.animation[index].duration);

  // アニメーションで全ノードの行列を更新
  updateNodeMatrix(model, current_time, model.animation[index]);

  // ノードの行列を再計算
  updateNodeDerivedMatrix(model.node, ci::Matrix44f::identity());

  // メッシュアニメーションを適用
  updateMesh(model);
}

// 再帰で全ノードを描画
void drawModel(const Model& model, const std::shared_ptr<Node>& node) {
  ci::gl::pushModelView();
  ci::gl::multModelView(node->matrix);

  for (const auto& mesh : node->mesh) {
    const auto& material = model.material[mesh.material_index];
    material.body.apply();

    if (material.has_texture) {
      model.textures.at(material.texture_name)->enableAndBind();
    }
    
    ci::gl::draw(mesh.body);

    if (material.has_texture) {
      model.textures.at(material.texture_name)->unbind();
      model.textures.at(material.texture_name)->disable();
    }
  }

  for (const auto& child : node->children) {
    drawModel(model, child);
  }
  
  ci::gl::popModelView();
}

// モデル描画
void drawModel(const Model& model) {
  drawModel(model, model.node);
}
