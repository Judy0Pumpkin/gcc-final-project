#include "model.h"
#include <fstream>
#include <glm/vec3.hpp>
#include <iostream>
#include <sstream>
#include <vector>

Model* Model::fromObjectFile(const char* obj_file) {
  Model* m = new Model();
  std::ifstream ObjFile(obj_file);
  if (!ObjFile.is_open()) {
    std::cout << "Can't open File !" << std::endl;
    return NULL;
  }

  std::vector<glm::vec3> temp_positions;
  std::vector<glm::vec2> temp_texcoords;
  std::vector<glm::vec3> temp_normals;
  std::string line;

  while (std::getline(ObjFile, line)) {
    std::istringstream iss(line);
    std::string prefix;
    iss >> prefix;

    if (prefix == "v") {
      glm::vec3 pos;
      iss >> pos.x >> pos.y >> pos.z;
      temp_positions.push_back(pos);
    } else if (prefix == "vt") {
      glm::vec2 tex;
      iss >> tex.x >> tex.y;
      temp_texcoords.push_back(tex);
    } else if (prefix == "vn") {
      glm::vec3 norm;
      iss >> norm.x >> norm.y >> norm.z;
      temp_normals.push_back(norm);
    } else if (prefix == "f") {
      std::vector<std::string> vertices;
      std::string vertex;
      while (iss >> vertex) {
        vertices.push_back(vertex);
      }

      if (vertices.size() == 3) {
        // 三角形
        for (int i = 0; i < 3; i++) {
          int pos_idx, tex_idx, norm_idx;
          char slash;
          std::istringstream viss(vertices[i]);
          viss >> pos_idx >> slash >> tex_idx >> slash >> norm_idx;

          pos_idx--;
          tex_idx--;
          norm_idx--;

          m->positions.push_back(temp_positions[pos_idx].x);
          m->positions.push_back(temp_positions[pos_idx].y);
          m->positions.push_back(temp_positions[pos_idx].z);

          m->texcoords.push_back(temp_texcoords[tex_idx].x);
          m->texcoords.push_back(temp_texcoords[tex_idx].y);

          m->normals.push_back(temp_normals[norm_idx].x);
          m->normals.push_back(temp_normals[norm_idx].y);
          m->normals.push_back(temp_normals[norm_idx].z);
        }
        m->numVertex += 3;

      } else if (vertices.size() >= 4) {
        // 四邊形或多邊形：用 fan triangulation
        for (size_t i = 1; i < vertices.size() - 1; i++) {
          int indices[3] = {0, (int)i, (int)(i + 1)};

          for (int j = 0; j < 3; j++) {
            int vertex_index = indices[j];
            int pos_idx, tex_idx, norm_idx;
            char slash;
            std::istringstream viss(vertices[vertex_index]);
            viss >> pos_idx >> slash >> tex_idx >> slash >> norm_idx;

            pos_idx--;
            tex_idx--;
            norm_idx--;

            m->positions.push_back(temp_positions[pos_idx].x);
            m->positions.push_back(temp_positions[pos_idx].y);
            m->positions.push_back(temp_positions[pos_idx].z);

            m->texcoords.push_back(temp_texcoords[tex_idx].x);
            m->texcoords.push_back(temp_texcoords[tex_idx].y);

            m->normals.push_back(temp_normals[norm_idx].x);
            m->normals.push_back(temp_normals[norm_idx].y);
            m->normals.push_back(temp_normals[norm_idx].z);
          }
          m->numVertex += 3;
        }
      }
    }
  }

  ObjFile.close();
  return m;
}