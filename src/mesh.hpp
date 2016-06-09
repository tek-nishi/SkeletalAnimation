#pragma once

//
// メッシュ
//

#include <cinder/TriMesh.h>
#include <cinder/Matrix44.h>
#include <assimp/scene.h>
#include <string>
#include "common.hpp"
#include "misc.hpp"


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


// ボーンの情報を作成
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
