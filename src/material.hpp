#pragma once

//
// マテリアル
//

#include <cinder/gl/Material.h>
#include <assimp/scene.h>
#include <string>
#include "common.hpp"
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
