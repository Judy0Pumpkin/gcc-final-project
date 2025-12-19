#include <fstream>
#include <glm/vec3.hpp>
#include <iostream>
#include <sstream>
#include <vector>
#include "model.h"

Model* Model::fromObjectFile(const char* obj_file) {
  Model* m = new Model();
  std::ifstream ObjFile(obj_file);
  if (!ObjFile.is_open()) {
    std::cout << "Can't open File !" << std::endl;
    return NULL;
  }

  // TODO#1: Load model data from OBJ file
  std::vector<glm::vec3> temp_positions;
  std::vector<glm::vec2> temp_texcoords;
  std::vector<glm::vec3> temp_normals;
  std::string line;

  while (std::getline(ObjFile, line)) {
    std::istringstream iss(line);
    std::string prefix;
    iss >> prefix;

    if (prefix == "v") {
      // Vertex position
      glm::vec3 pos;
      iss >> pos.x >> pos.y >> pos.z;
      temp_positions.push_back(pos);
    } else if (prefix == "vt") {
      // Texture coordinate
      glm::vec2 tex;
      iss >> tex.x >> tex.y;
      temp_texcoords.push_back(tex);
    } else if (prefix == "vn") {
      // Normal
      glm::vec3 norm;
      iss >> norm.x >> norm.y >> norm.z;
      temp_normals.push_back(norm);
    } else if (prefix == "f") {
      // Face
      std::vector<std::string> vertices;
      std::string vertex;
      while (iss >> vertex) {
        vertices.push_back(vertex);
      }

      
    int triangle_indices[6] = {0, 1, 2, 0, 2, 3};

    for (int i = 0; i < 6; i++) {
        int vertex_index = triangle_indices[i];
        int pos_idx, tex_idx, norm_idx;
        char slash;
        std::istringstream viss(vertices[vertex_index]);

        viss >> pos_idx >> slash >> tex_idx >> slash >> norm_idx;

        // OBJ indices are 1-based, convert to 0-based
        pos_idx--;
        tex_idx--;
        norm_idx--;

        // Add position
        m->positions.push_back(temp_positions[pos_idx].x);
        m->positions.push_back(temp_positions[pos_idx].y);
        m->positions.push_back(temp_positions[pos_idx].z);

        // Add texcoord
        m->texcoords.push_back(temp_texcoords[tex_idx].x);
        m->texcoords.push_back(temp_texcoords[tex_idx].y);

        // Add normal
        m->normals.push_back(temp_normals[norm_idx].x);
        m->normals.push_back(temp_normals[norm_idx].y);
        m->normals.push_back(temp_normals[norm_idx].z);
    }
    m->numVertex += 6;  
      
    }
  }

  ObjFile.close();
  return m;
}