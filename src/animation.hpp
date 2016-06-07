#pragma once

//
// アニメーション
//

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
