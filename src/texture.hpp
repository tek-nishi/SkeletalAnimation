#pragma once

//
// テクスチャ
//

#include <map>
#include <string>
#include <cinder/ImageIo.h>
#include <cinder/gl/Texture.h>
#include <cinder/ip/Resize.h>
#include "misc.hpp"


// テクスチャを読み込む
ci::gl::TextureRef loadTexrture(const std::string& path) {
  ci::app::console() << "Texture read:" << path << std::endl;

#if defined (USE_FULL_PATH)
  ci::Surface surface = ci::loadImage(path);
#else
  ci::Surface surface = ci::loadImage(ci::app::loadAsset(path));
#endif

  // サイズが２のべき乗でなければ変換
  int w = surface.getWidth();
  int h = surface.getHeight();
  int pow_w = int2pow(w);
  int pow_h = int2pow(h);

  // 大きなサイズのテクスチャは禁止
  assert((pow_w <= 2048) && (pow_h <= 2048));

  if ((w != pow_w) || (h != pow_h)) {
    // リサイズ
    surface = ci::ip::resizeCopy(surface, ci::Area{0, 0, w - 1, h - 1}, ci::Vec2i{pow_w, pow_h});
    ci::app::console() << "Texture resize: " << w << "," << h << " -> " << pow_w << "," << pow_h << std::endl;
  }

  return ci::gl::Texture::create(surface);
}
