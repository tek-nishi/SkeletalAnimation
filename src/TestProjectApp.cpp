﻿//
// assimpでの読み込み
// 

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Light.h"
#include "cinder/Camera.h"
#include "cinder/Arcball.h"

#include "model.hpp"


using namespace ci;
using namespace ci::app;


class AssimpApp : public AppNative {
  enum {
    WINDOW_WIDTH  = 800,
    WINDOW_HEIGHT = 600,
  };
  
  // 投影変換をおこなうカメラを定義
  // 透視投影（Perspective）
  CameraPersp camera_persp;

  float fov;
  float near_z;
  float far_z;

  gl::Light* light;

  Vec2i mouse_prev_pos;
  int touch_num;
  
  Quatf rotate;
  Vec3f translate;
  float z_distance;
  
  Model model;
  Vec3f offset;
  
  double animation_begin_time;

  
  void setupCamera();

  
public:
  void prepareSettings(Settings* settings);
  
  void setup();
  void shutdown();

  void resize();

  void fileDrop(FileDropEvent event);
  
  void mouseDown(MouseEvent event);
  void mouseDrag(MouseEvent event);

  void keyDown(KeyEvent event);	
  
  void touchesBegan(TouchEvent event);
  void touchesMoved(TouchEvent event);
  void touchesEnded(TouchEvent event);
  
  void update();
  void draw();
};


// 読み込んだモデルの大きさに応じてカメラを設定する
void AssimpApp::setupCamera() {
  // 初期位置はモデルのAABBの中心位置とする
  offset = -model.aabb.getCenter();
  
  // モデルがスッポリ画面に入るようカメラ位置を調整
  float w = model.aabb.getSize().length() / 2.0f;
  float distance = w / std::tan(toRadians(fov / 2.0f));

  z_distance = distance;

  rotate     = Quatf::identity();
  translate  = Vec3f::zero();

  // NearクリップとFarクリップを決める
  float size = model.aabb.getSize().length();
  near_z = size * 0.01f;
  far_z  = size * 100.0f;

  camera_persp.setNearClip(near_z);
  camera_persp.setFarClip(far_z);
  
  animation_begin_time = 0.0f;
}


void AssimpApp::prepareSettings(Settings* settings) {
  // 画面サイズを変更する
  settings->setWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);

  // マルチタッチ有効
  touch_num = 0;
  settings->enableMultiTouch();
}

void AssimpApp::setup() {
#if defined (CINDER_COCOA_TOUCH)
  // 縦横画面両対応
  getSignalSupportedOrientations().connect([]() { return ci::app::InterfaceOrientation::All; });
#endif
  
  // モデルデータ読み込み
  model = loadModel("miku.dae");
  
  // カメラの設定
  fov = 35.0f;
  setupCamera();
  
  camera_persp = CameraPersp(WINDOW_WIDTH, WINDOW_HEIGHT,
                             fov,
                             near_z, far_z);

  camera_persp.setEyePoint(Vec3f::zero());
  camera_persp.setCenterOfInterestPoint(Vec3f{ 0.0f, 0.0f, -1.0f });

  // ライトの設定
  light = new gl::Light(gl::Light::DIRECTIONAL, 0);
  light->setAmbient(Color(0.0, 0.0, 0.0));
  light->setDiffuse(Color(0.9, 0.9, 0.9));
  light->setSpecular(Color(0.5, 0.5, 0.6));
  light->setDirection(Vec3f(0, 0, 1));

  // 環境光を完全に真っ暗に
  GLfloat lmodel_ambient[] = { 0.0, 0.0, 0.0, 0.0 };
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);

#if !defined (CINDER_COCOA_TOUCH)
  // 拡散光と鏡面反射を個別に計算する
  // TIPS:これで、テクスチャを張ったポリゴンもキラーン!!ってなる
  glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
#endif
  
  gl::enableDepthRead();
  gl::enableDepthWrite();
  gl::enableAlphaBlending();
  
  gl::enable(GL_CULL_FACE);
  gl::enable(GL_NORMALIZE);
}

void AssimpApp::shutdown() {
  delete light;
}


void AssimpApp::resize() {
  float aspect = ci::app::getWindowAspectRatio();
  camera_persp.setAspectRatio(aspect);
  
  if (aspect < 1.0) {
    // 画面が縦長になったら、幅基準でfovを求める
    // fovとnear_zから投影面の幅の半分を求める
    float half_w = std::tan(ci::toRadians(fov / 2)) * near_z;

    // 表示画面の縦横比から、投影面の高さの半分を求める
    float half_h = half_w / aspect;

    // 投影面の高さの半分とnear_zから、fovが求まる
    float new_fov = std::atan(half_h / near_z) * 2;
    camera_persp.setFov(ci::toDegrees(new_fov));
  }
  else {
    // 横長の場合、fovは固定
    camera_persp.setFov(fov);
  }
}


void AssimpApp::fileDrop(FileDropEvent event) {
  const auto& path = event.getFiles();

  console() << path[0].filename() << std::endl;

  model = loadModel(path[0].filename().string());
  console() << model.aabb.getSize() << std::endl;
  offset = -model.aabb.getCenter();

  setupCamera();
  animation_begin_time = getElapsedSeconds();
  touch_num = 0;
}


void AssimpApp::mouseDown(MouseEvent event) {
  if (touch_num > 1) return;
  
  if (event.isLeft()) {
    // TIPS:マウスとワールド座標で縦方向の向きが逆
    auto pos = event.getPos();
    mouse_prev_pos = pos;
  }
}

void AssimpApp::mouseDrag(MouseEvent event) {
  if (touch_num > 1) return;

  if (event.isLeftDown()) {
    auto mouse_pos = event.getPos();
    Vec2f d{ mouse_pos - mouse_prev_pos };
    float l = d.length();
    if (l > 0.0f) {
      d.normalize();

      Vec3f v1{   d.x, -d.y, 0.0f };
      Vec3f v2{ -v1.y, v1.x, 0.0f };

      Quatf r{ v2, l * 0.01f };
      rotate = rotate * r;
    }
    
    mouse_prev_pos = mouse_pos;
  }
}


void AssimpApp::keyDown(KeyEvent event) {
  int key_code = event.getCode();

  if (key_code == KeyEvent::KEY_r) {
    setupCamera();

    touch_num = 0;
  }
}


void AssimpApp::touchesBegan(TouchEvent event) {
  const auto& touches = event.getTouches();

  touch_num += touches.size();
}

void AssimpApp::touchesMoved(TouchEvent event) {
//  if (touch_num < 2) return;
  
  const auto& touches = event.getTouches();

#if defined (CINDER_COCOA_TOUCH)
  if (touch_num == 1) {
    Vec2f d{ touches[0].getPos() -  touches[0].getPrevPos() };
    float l = d.length();
    if (l > 0.0f) {
      d.normalize();

      Vec3f v1{   d.x, -d.y, 0.0f };
      Vec3f v2{ -v1.y, v1.x, 0.0f };

      Quatf r{ v2, l * 0.01f };
      rotate = rotate * r;
    }
    
    return;
  }
#endif
  if (touches.size() < 2) return;
  
  Vec3f v1{ touches[0].getX(), -touches[0].getY(), 0.0f };
  Vec3f v1_prev{ touches[0].getPrevX(), -touches[0].getPrevY(), 0.0f };
  Vec3f v2{ touches[1].getX(), -touches[1].getY(), 0.0f };
  Vec3f v2_prev{ touches[1].getPrevX(), -touches[1].getPrevY(), 0.0f };

  Vec3f d = v1 - v1_prev;

  float l = (v2 - v1).length();
  float l_prev = (v2_prev - v1_prev).length();
  float ld = l - l_prev;

  // 距離に応じて比率を変える
  float t = std::tan(fov / 2.0f) * z_distance;

  if (std::abs(ld) < 3.0f) {
    translate -= d * t * 0.0005f;
  }
  else {
    z_distance = std::max(z_distance + ld * t * 0.001f, 0.01f);
  }
}

void AssimpApp::touchesEnded(TouchEvent event) {
  const auto& touches = event.getTouches();

  // 最悪マイナス値にならないよう
  touch_num = std::max(touch_num - int(touches.size()), 0);
}


void AssimpApp::update() {
  double current_time = getElapsedSeconds() - animation_begin_time;
  
  updateModel(model, current_time, 0);
}

void AssimpApp::draw() {
  
  gl::clear(Color(0.3, 0.3, 0.3));

  gl::setMatrices(camera_persp);

  gl::enable(GL_LIGHTING);
  light->enable();

  gl::translate(Vec3f(0, 0.0, -z_distance));

  // FIXME:CinderのQuatfをOpenGLに渡す実装がよくない
  gl::multModelView(rotate.toMatrix44());
  gl::translate(translate);

  gl::translate(offset);
  drawModel(model);
  
  light->disable();
}


CINDER_APP_NATIVE(AssimpApp, RendererGl)
