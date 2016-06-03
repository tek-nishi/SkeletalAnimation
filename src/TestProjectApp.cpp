//
// assimpでの読み込み
// 

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Light.h"
#include "cinder/Camera.h"

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

  gl::Light* light;
  
  float rotate;

  Model model;
  
  double current_time;
  
  
public:
  void prepareSettings(Settings* settings);

  
  void setup();
  void shutdown();
  void resize();
  
  void update();
  void draw();
};


void AssimpApp::prepareSettings(Settings* settings) {
  // 画面サイズを変更する
  settings->setWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
}

void AssimpApp::setup() {
#if defined (CINDER_COCOA_TOUCH)
    // 縦横画面両対応
    getSignalSupportedOrientations().connect([]() { return ci::app::InterfaceOrientation::All; });
#endif
    
  fov    = 35.0f;
  near_z = 1.0f;
  
  camera_persp = CameraPersp(WINDOW_WIDTH, WINDOW_HEIGHT,
                             fov,
                             near_z, 50.0f);

  camera_persp.setEyePoint(Vec3f(0.0f, 0.0f, 0.0f));
  camera_persp.setCenterOfInterestPoint(Vec3f(0.0f, 0.0f, -1000.0f));

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
  
  rotate = 0.0f;

  // モデルデータ読み込み
  // model = loadModel("astroboy_walk.dae");
  model = loadModel("mikuonndo.dae");

  current_time = 0.0;
  
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


void AssimpApp::update() {
  updateModel(model, current_time, 0);
  current_time += 1.0 / 60;
}

void AssimpApp::draw() {
  
  gl::clear(Color(0.3, 0.3, 0.3));

  gl::setMatrices(camera_persp);

  gl::enable(GL_LIGHTING);
  light->enable();

  gl::translate(Vec3f(0, -0.7, -2));
  gl::rotate(Vec3f(0, 15, 0));
  // gl::rotate(Vec3f(0, rotate, 0));
  gl::scale(Vec3f(3.0, 3.0, 3.0));
  // gl::scale(Vec3f(10.0, 10.0, 10.0));

  drawModel(model);
  
  light->disable();
}


CINDER_APP_NATIVE(AssimpApp, RendererGl)
