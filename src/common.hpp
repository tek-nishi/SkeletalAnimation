#pragma once

//
// 一般的な定義
//

#include <cinder/Vector.h>
#include <cinder/Quaternion.h>


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

