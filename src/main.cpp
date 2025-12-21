#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#include <GLFW/glfw3.h>
#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#undef GLAD_GL_IMPLEMENTATION
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

#include "camera.h"
#include "context.h"
#include "gl_helper.h"
#include "model.h"
#include "opengl_context.h"
#include "program.h"
#include "utils.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <random> 

#include "Snake.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;
int appleModelIndex = -1;
glm::vec3 applePosition(4.0f, 0.5f, 2.5f);
bool hitWall = false;
int score = 0;

const float FLOOR_WIDTH = 8.192f * 1.0f;
const float FLOOR_DEPTH = 5.12f * 1.0f;
const float FLOOR_CENTER_X = FLOOR_WIDTH / 2.0f;
const float FLOOR_CENTER_Z = FLOOR_DEPTH / 2.0f;
const float WALL_HEIGHT = 1.0f;
const float WALL_THICKNESS = 0.2f;

int wallModelIndices[4] = {-1, -1, -1, -1};  // 牆壁 前、後、左、右

bool pause = false;

enum class GameState { STOPPED, RUNNING, PAUSED };
GameState gameState = GameState::STOPPED;

float gameTimer = 60.0f;  
float elapsedTime = 0.0f;  
const int WIN_SCORE = 5;  

// 遊戲結果
enum class GameResult { NONE, WIN, LOSE };
GameResult gameResult = GameResult::NONE;

bool snakeViewMode = false;  //true是蛇視角


// 隨機數字生成器
std::random_device rd;
std::mt19937 gen(rd());

// ========== 全域變數 ==========
Snake* snake = nullptr;
int snakeModelIndex = -1;
    /* 12202116 標註為外加的部分，先將這個隱藏退回原本狀態 12211727 此處已無功用
float simulationTime = 0.0f;

enum class SnakeMode { LATERAL_UNDULATION, RECTILINEAR_PROGRESSION };

SnakeMode currentSnakeMode = SnakeMode::LATERAL_UNDULATION;
*/

Context ctx;
Material mFlatwhite;
Material mShinyred;
Material mGreenHead;
Material mMirror;

// ========== 函數宣告 ==========
void initOpenGL();
void resizeCallback(GLFWwindow* window, int width, int height);
void keyCallback(GLFWwindow* window, int key, int, int action, int);

// ========== 材質設定 ==========
void loadMaterial() {
  mFlatwhite.ambient = glm::vec3(0.1f, 0.1f, 0.1f);
  mFlatwhite.diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
  mFlatwhite.specular = glm::vec3(0.1f, 0.1f, 0.1f);
  mFlatwhite.shininess = 0.0f;

  mShinyred.ambient = glm::vec3(0.1985f, 0.0f, 0.0f);
  mShinyred.diffuse = glm::vec3(0.5921f, 0.0167f, 0.0f);
  mShinyred.specular = glm::vec3(0.5973f, 0.2083f, 0.2083f);
  mShinyred.shininess = 100.0f;

  mGreenHead.ambient = glm::vec3(0.0f, 0.15f, 0.0f);
  mGreenHead.diffuse = glm::vec3(0.1f, 0.8f, 0.1f);
  mGreenHead.specular = glm::vec3(0.2f, 0.6f, 0.2f);
  mGreenHead.shininess = 50.0f;

  mMirror.ambient = glm::vec3(0.1f, 0.1f, 0.1f);
  mMirror.diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
  mMirror.specular = glm::vec3(0.7f, 0.7f, 0.7f);
  mMirror.shininess = 64.0f;
  mMirror.reflectivity = 0.05f;
}

// ========== 著色器程式 ==========
void loadPrograms() {
  ctx.programs.push_back(new SkyboxProgram(&ctx));
  ctx.programs.push_back(new LightProgram(&ctx));
  ctx.programs.push_back(new ShadowProgram(&ctx));

  for (auto iter = ctx.programs.begin(); iter != ctx.programs.end(); iter++) {
    if (!(*iter)->load()) {
      std::cout << "Load program fail, force terminate" << std::endl;
      exit(1);
    }
  }
  glUseProgram(0);
}

// ========== Shadow Map 初始化 ==========
void initShadowMap() {
  float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};

  // Direction light shadow map
  glGenFramebuffers(1, &ctx.shadowMapFBO);
  glGenTextures(1, &ctx.shadowMap);
  glBindTexture(GL_TEXTURE_2D, ctx.shadowMap);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, ctx.shadowMapWidth, ctx.shadowMapHeight, 0, GL_DEPTH_COMPONENT,
               GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
  glBindFramebuffer(GL_FRAMEBUFFER, ctx.shadowMapFBO);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, ctx.shadowMap, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Spot light shadow map
  glGenFramebuffers(1, &ctx.spotShadowMapFBO);
  glGenTextures(1, &ctx.spotShadowMap);
  glBindTexture(GL_TEXTURE_2D, ctx.spotShadowMap);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, ctx.shadowMapWidth, ctx.shadowMapHeight, 0, GL_DEPTH_COMPONENT,
               GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
  glBindFramebuffer(GL_FRAMEBUFFER, ctx.spotShadowMapFBO);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, ctx.spotShadowMap, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Point light shadow map
  glGenFramebuffers(1, &ctx.pointShadowMapFBO);
  glGenTextures(1, &ctx.pointShadowMap);
  glBindTexture(GL_TEXTURE_CUBE_MAP, ctx.pointShadowMap);
  for (int i = 0; i < 6; ++i) {
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                 NULL);
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ========== 地板模型 ==========
Model* createPlane() {
  Model* m = new Model();

  float width = FLOOR_WIDTH;
  float depth = FLOOR_DEPTH;
  float centerX = FLOOR_CENTER_X;
  float centerZ = FLOOR_CENTER_Z;
  float y = 0.0f;

  float halfWidth = width / 2.0f;
  float halfDepth = depth / 2.0f;

  glm::vec3 v0(centerX - halfWidth, y, centerZ - halfDepth);
  glm::vec3 v1(centerX - halfWidth, y, centerZ + halfDepth);
  glm::vec3 v2(centerX + halfWidth, y, centerZ + halfDepth);
  glm::vec3 v3(centerX + halfWidth, y, centerZ - halfDepth);

  glm::vec3 normal(0.0f, 1.0f, 0.0f);

  // 貼圖
  float uRepeat = 10.0f;
  float vRepeat = 10.0f;

  glm::vec2 t0(0.0f, 0.0f);
  glm::vec2 t1(0.0f, vRepeat);
  glm::vec2 t2(uRepeat, vRepeat);
  glm::vec2 t3(uRepeat, 0.0f);

  // 三角形 1
  m->positions.insert(m->positions.end(), {v0.x, v0.y, v0.z, v1.x, v1.y, v1.z, v2.x, v2.y, v2.z});
  m->normals.insert(m->normals.end(),
                    {normal.x, normal.y, normal.z, normal.x, normal.y, normal.z, normal.x, normal.y, normal.z});
  m->texcoords.insert(m->texcoords.end(), {t0.x, t0.y, t1.x, t1.y, t2.x, t2.y});

  // 三角形 2
  m->positions.insert(m->positions.end(), {v0.x, v0.y, v0.z, v2.x, v2.y, v2.z, v3.x, v3.y, v3.z});
  m->normals.insert(m->normals.end(),
                    {normal.x, normal.y, normal.z, normal.x, normal.y, normal.z, normal.x, normal.y, normal.z});
  m->texcoords.insert(m->texcoords.end(), {t0.x, t0.y, t2.x, t2.y, t3.x, t3.y});

  m->textures.push_back(createTexture("../assets/models/Wood_maps/AT_Wood.jpg"));
  m->numVertex = 6;
  m->drawMode = GL_TRIANGLES;

  return m;
}

glm::vec3 generateRandomApplePosition() {
  // 在地板範圍內隨機生成位置
  float margin = 1.0f;
  std::uniform_real_distribution<float> distX(margin, FLOOR_WIDTH - margin);
  std::uniform_real_distribution<float> distZ(margin, FLOOR_DEPTH - margin);

  return glm::vec3(distX(gen), 0.4f, distZ(gen));
}


Model* createApple() {
  Model* m = Model::fromObjectFile("../assets/apple/Apple.obj");

  if (!m) {
    std::cout << "[ERROR] Failed to load Apple.obj!" << std::endl;
    return nullptr;
  }

  std::cout << "[DEBUG] Apple loaded! Vertices: " << m->numVertex << std::endl;

  m->textures.push_back(createTexture("../assets/apple/Apple_BaseColor.png"));

  // 初始位置隨機生成
  applePosition = glm::vec3(3.0f, 0.4f, 3.0f);

  m->modelMatrix = glm::identity<glm::mat4>();
  m->modelMatrix = glm::translate(m->modelMatrix, applePosition);
  m->modelMatrix = glm::scale(m->modelMatrix, glm::vec3(0.008f)); //蘋果大小
  m->drawMode = GL_TRIANGLES;

  std::cout << "[INFO] Apple spawned at: (" << applePosition.x << ", " << applePosition.z << ")" << std::endl;

  return m;
}


void respawnFirstApple() {  //新生一個蘋果
  applePosition = glm::vec3(3.0f, 0.4f, 3.0f);

  // 更新蘋果的 modelMatrix
  if (appleModelIndex >= 0 && appleModelIndex < ctx.models.size()) {
    Model* apple = ctx.models[appleModelIndex];
    apple->modelMatrix = glm::identity<glm::mat4>();
    apple->modelMatrix = glm::translate(apple->modelMatrix, applePosition);
    apple->modelMatrix = glm::scale(apple->modelMatrix, glm::vec3(0.008f));  // 蘋果大小
  }

  std::cout << "[INFO] Apple respawned at: (" << applePosition.x << ", " << applePosition.z << ")" << std::endl;
}


void respawnApple() { //新生一個蘋果
  applePosition = generateRandomApplePosition();

  // 更新蘋果的 modelMatrix
  if (appleModelIndex >= 0 && appleModelIndex < ctx.models.size()) {
    Model* apple = ctx.models[appleModelIndex];
    apple->modelMatrix = glm::identity<glm::mat4>();
    apple->modelMatrix = glm::translate(apple->modelMatrix, applePosition);
    apple->modelMatrix = glm::scale(apple->modelMatrix, glm::vec3(0.008f));   // 蘋果大小
  }

  std::cout << "[INFO] Apple respawned at: (" << applePosition.x << ", " << applePosition.z << ")" << std::endl;
}

void generateSphere(std::vector<float>& positions, std::vector<float>& normals, std::vector<float>& texcoords,
                    const glm::vec3& center, float radius, int segments = 16, int rings = 12) {
  // segments: 經度分段數 (水平)
  // rings: 緯度分段數 (垂直)

  for (int ring = 0; ring <= rings; ++ring) {
    float theta = ring * M_PI / rings;  // 緯度角度 (0 到 π)
    float sinTheta = sin(theta);
    float cosTheta = cos(theta);

    for (int seg = 0; seg <= segments; ++seg) {
      float phi = seg * 2.0f * M_PI / segments;  // 經度角度 (0 到 2π)
      float sinPhi = sin(phi);
      float cosPhi = cos(phi);

      // 計算球面上的點
      float x = cosPhi * sinTheta;
      float y = cosTheta;
      float z = sinPhi * sinTheta;

      // 頂點位置
      glm::vec3 vertex = center + glm::vec3(x, y, z) * radius;
      positions.push_back(vertex.x);
      positions.push_back(vertex.y);
      positions.push_back(vertex.z);

      // 法線 從中心指向頂點（朝外）
      normals.push_back(x);
      normals.push_back(y);
      normals.push_back(z);

      // 紋理座標
      float u = (float)seg / segments;
      float v = (float)ring / rings;
      texcoords.push_back(u);
      texcoords.push_back(v);
    }
  }
}


Model* createTestBox() {  // 12202131 在隱藏掉外加snake的程式後，用來測試顯示是否有問題 更12202328 已不需要
  Model* m = new Model();

  // 6 個面 * 4 個頂點 * 3 (x,y,z)
  // 立方體中心在原點，邊長 2（-1 ~ 1）

  int pos[24 * 3] = {
      // Front face (z = +1)
      -1, -1, 1,  // v0
      1, -1, 1,   // v1
      1, 1, 1,    // v2
      -1, 1, 1,   // v3

      // Back face (z = -1)
      1, -1, -1,   // v4
      -1, -1, -1,  // v5
      -1, 1, -1,   // v6
      1, 1, -1,    // v7

      // Left face (x = -1)
      -1, -1, -1,  // v8
      -1, -1, 1,   // v9
      -1, 1, 1,    // v10
      -1, 1, -1,   // v11

      // Right face (x = +1)
      1, -1, 1,   // v12
      1, -1, -1,  // v13
      1, 1, -1,   // v14
      1, 1, 1,    // v15

      // Top face (y = +1)
      -1, 1, 1,   // v16
      1, 1, 1,    // v17
      1, 1, -1,   // v18
      -1, 1, -1,  // v19

      // Bottom face (y = -1)
      -1, -1, -1,  // v20
      1, -1, -1,   // v21
      1, -1, 1,    // v22
      -1, -1, 1    // v23
  };

  int norm[24 * 3] = {// Front (+Z)
                      0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1,
                      // Back (-Z)
                      0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1,
                      // Left (-X)
                      -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0,
                      // Right (+X)
                      1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0,
                      // Top (+Y)
                      0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0,
                      // Bottom (-Y)
                      0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0};

  for (int i = 0; i < 24 * 3; i++) {
    m->positions.push_back(static_cast<float>(pos[i]));
    m->normals.push_back(static_cast<float>(norm[i]));
  }

  m->numVertex = 24;       // 6 faces * 4 vertices
  m->drawMode = GL_QUADS;  // 一面一個 quad

  return m;
}



// ========== 蛇模型 ==========
Model* createSnakeModelSimple(Snake* snake) {
  if (!snake) return nullptr;

  Model* m = new Model();
  const auto& masses = snake->getMasses();
  float radius = snake->getRadius();

  const int segments = 16;
  const int rings = 12;

  std::vector<float> spherePositions;
  std::vector<float> sphereNormals;
  std::vector<float> sphereTexcoords;

  for (int i = 0; i < (int)masses.size(); ++i) {
    glm::vec3 pos = masses[i]->getPosition();

    spherePositions.clear();
    sphereNormals.clear();
    sphereTexcoords.clear();

    generateSphere(spherePositions, sphereNormals, sphereTexcoords, pos, radius, segments, rings);

    // 調整紋理座標
    for (size_t j = 0; j < sphereTexcoords.size(); j += 2) {
      if (i == 0) {
        sphereTexcoords[j] = sphereTexcoords[j] * 0.5f;
      } else {
        sphereTexcoords[j] = 0.5f + sphereTexcoords[j] * 0.5f;
      }
    }

    // 生成三角形 - 修正繞序為逆時針（CCW）朝外
    for (int ring = 0; ring < rings; ++ring) {
      for (int seg = 0; seg < segments; ++seg) {
        int i0 = ring * (segments + 1) + seg;
        int i1 = i0 + segments + 1;
        int i2 = i1 + 1;
        int i3 = i0 + 1;

        // 三角形 1: i0, i2, i1 (改變順序！)
        // 從外面看是逆時針
        for (int idx : {i0, i2, i1}) {
          m->positions.push_back(spherePositions[idx * 3]);
          m->positions.push_back(spherePositions[idx * 3 + 1]);
          m->positions.push_back(spherePositions[idx * 3 + 2]);

          m->normals.push_back(sphereNormals[idx * 3]);
          m->normals.push_back(sphereNormals[idx * 3 + 1]);
          m->normals.push_back(sphereNormals[idx * 3 + 2]);

          m->texcoords.push_back(sphereTexcoords[idx * 2]);
          m->texcoords.push_back(sphereTexcoords[idx * 2 + 1]);
        }

        // 三角形 2: i0, i3, i2 (改變順序！)
        // 從外面看是逆時針
        for (int idx : {i0, i3, i2}) {
          m->positions.push_back(spherePositions[idx * 3]);
          m->positions.push_back(spherePositions[idx * 3 + 1]);
          m->positions.push_back(spherePositions[idx * 3 + 2]);

          m->normals.push_back(sphereNormals[idx * 3]);
          m->normals.push_back(sphereNormals[idx * 3 + 1]);
          m->normals.push_back(sphereNormals[idx * 3 + 2]);

          m->texcoords.push_back(sphereTexcoords[idx * 2]);
          m->texcoords.push_back(sphereTexcoords[idx * 2 + 1]);
        }

        m->numVertex += 6;
      }
    }
  }

  m->textures.push_back(createTexture("../assets/models/snake/snake.jpg"));
  m->drawMode = GL_TRIANGLES;
  return m;
}


Model* createDirectionBox(const glm::vec3& startPos, const glm::vec3& direction, float length, float width,
                          float height) {
  Model* m = new Model();

  // 根據方向向量來確定長方體頂點
  glm::vec3 v[8];
  glm::vec3 right = glm::normalize(glm::cross(direction, glm::vec3(0, 1, 0)));
  glm::vec3 up = glm::cross(direction, right);

  // 從 startPos 往 direction 方向延伸 length 長度
  // 不再是從中心往兩邊延伸，而是從起點往前延伸
  glm::vec3 endPos = startPos + direction * length;

  // 計算每個頂點位置
  v[0] = startPos - right * width - up * height;
  v[1] = endPos - right * width - up * height;
  v[2] = endPos + right * width - up * height;
  v[3] = startPos + right * width - up * height;
  v[4] = startPos - right * width + up * height;
  v[5] = endPos - right * width + up * height;
  v[6] = endPos + right * width + up * height;
  v[7] = startPos + right * width + up * height;

  // 6 個面的索引
  int faces[24] = {
      0, 3, 2, 1,  // 底
      4, 5, 6, 7,  // 頂
      7, 3, 0, 4,  // 左
      5, 1, 2, 6,  // 右
      4, 0, 1, 5,  // 後
      6, 2, 3, 7,  // 前
  };

  // 法線
  glm::vec3 normals[6] = {
      -up,         // 底
      up,          // 頂
      -right,      // 左
      right,       // 右
      -direction,  // 後
      direction    // 前
  };

  // UV坐標
  for (int f = 0; f < 24; ++f) {
    glm::vec3 vertex = v[faces[f]];
    glm::vec3 normal = normals[f / 4];

    float u, vCoord;
    u = (f % 4 < 2) ? 0.0f : 1.0f;
    vCoord = (f % 4 == 1 || f % 4 == 2) ? 1.0f : 0.0f;

    m->positions.push_back(vertex.x);
    m->positions.push_back(vertex.y);
    m->positions.push_back(vertex.z);

    m->normals.push_back(normal.x);
    m->normals.push_back(normal.y);
    m->normals.push_back(normal.z);

    m->texcoords.push_back(u);
    m->texcoords.push_back(vCoord);
  }

  m->numVertex = 24;
  m->drawMode = GL_QUADS;
  
  m->textures.push_back(createTexture("../assets/models/snake/snake.jpg"));

  return m;
}

Model* createWall(float width, float height, float depth, glm::vec3 position, glm::vec3 normal_dir) {
  Model* m = new Model();

  glm::vec3 n = glm::normalize(normal_dir);

 
  std::vector<glm::vec3> vertices;

  if (abs(n.z) > 0.5f) {
    // 前後牆
    float hw = width / 2.0f;
    vertices = {position + glm::vec3(-hw, 0, 0), position + glm::vec3(hw, 0, 0), position + glm::vec3(hw, height, 0),
                position + glm::vec3(-hw, height, 0)};
  } else {
    // 左右牆
    float hd = depth / 2.0f;
    vertices = {position + glm::vec3(0, 0, hd), position + glm::vec3(0, 0, -hd), position + glm::vec3(0, height, -hd),
                position + glm::vec3(0, height, hd)};
  }

  // 正面朝向場地內部
  int order[6];
  if (n.z > 0 || n.x > 0) {
  
    order[0] = 0;
    order[1] = 1;
    order[2] = 2;
    order[3] = 0;
    order[4] = 2;
    order[5] = 3;
  } else {

    order[0] = 0;
    order[1] = 2;
    order[2] = 1;
    order[3] = 0;
    order[4] = 3;
    order[5] = 2;
  }

  // 三角形 1
  for (int i = 0; i < 3; i++) {
    m->positions.push_back(vertices[order[i]].x);
    m->positions.push_back(vertices[order[i]].y);
    m->positions.push_back(vertices[order[i]].z);
    m->normals.push_back(n.x);
    m->normals.push_back(n.y);
    m->normals.push_back(n.z);
  }
  m->texcoords.insert(m->texcoords.end(), {0, 0, 1, 0, 1, 1});

  // 三角形 2
  for (int i = 3; i < 6; i++) {
    m->positions.push_back(vertices[order[i]].x);
    m->positions.push_back(vertices[order[i]].y);
    m->positions.push_back(vertices[order[i]].z);
    m->normals.push_back(n.x);
    m->normals.push_back(n.y);
    m->normals.push_back(n.z);
  }
  m->texcoords.insert(m->texcoords.end(), {0, 0, 1, 1, 0, 1});

  m->numVertex = 6;
  m->drawMode = GL_TRIANGLES;
  m->textures.push_back(createTexture("../assets/models/Wood_maps/AT_Wood.jpg"));

  return m;
}

Model* initializeSnake() {
  std::cout << "\n=== Initializing Snake ===" << std::endl;

  // 起始位置在地板中央
  glm::vec3 startPos(FLOOR_CENTER_X, 0.5f, FLOOR_CENTER_Z);

  // 參數：節數, 質量, 每段長度, 彈簧常數, 阻尼, 起始位置, 半徑
  snake = new Snake(7, 0.020f, 0.178f, 1.0f, 3.5f, startPos, 0.2f);  // 12210456 i change k to 1.0f from 0.5f

  Model* snakeModel = createSnakeModelSimple(snake);

  // ctx.models.push_back(snakeModel); // 12202326 我想將他移到統一的地方，所以先試著註解掉
  // snakeModelIndex = ctx.models.size() - 1; // 12202326 我想將他移到統一的地方，所以先試著註解掉

  std::cout << "Snake initialized at center!" << std::endl;
  return snakeModel;
}



// ========== 蛇更新 ==========

void updateSnake(float dtFrame) {
  if (!snake || snakeModelIndex < 0 || snakeModelIndex >= ctx.models.size()) {
    return;
  }

  const float maxSubDt = 0.002f;
  int substeps = (int)std::ceil(dtFrame / maxSubDt);
  if (substeps < 1) substeps = 1;
  if (substeps > 30) {
    std::cerr << "[WARNING] Too many substeps: " << substeps << std::endl;
    substeps = 30;
  }

  float dtSub = dtFrame / substeps;

  for (int s = 0; s < substeps; ++s) {
    snake->update(dtSub);
  }

  Model* model = ctx.models[snakeModelIndex];

  model->positions.clear();
  model->normals.clear();
  model->texcoords.clear();
  model->numVertex = 0;

  const auto& masses = snake->getMasses();
  float radius = snake->getRadius();

  const int segments = 16;
  const int rings = 12;

  std::vector<float> spherePositions;
  std::vector<float> sphereNormals;
  std::vector<float> sphereTexcoords;

  for (int i = 0; i < (int)masses.size(); ++i) {
    glm::vec3 pos = masses[i]->getPosition();

    spherePositions.clear();
    sphereNormals.clear();
    sphereTexcoords.clear();

    generateSphere(spherePositions, sphereNormals, sphereTexcoords, pos, radius, segments, rings);

    for (size_t j = 0; j < sphereTexcoords.size(); j += 2) {
      if (i == 0) {
        sphereTexcoords[j] = sphereTexcoords[j] * 0.5f;
      } else {
        sphereTexcoords[j] = 0.5f + sphereTexcoords[j] * 0.5f;
      }
    }

    // 生成三角形 - 使用修正後的繞序
    for (int ring = 0; ring < rings; ++ring) {
      for (int seg = 0; seg < segments; ++seg) {
        int i0 = ring * (segments + 1) + seg;
        int i1 = i0 + segments + 1;
        int i2 = i1 + 1;
        int i3 = i0 + 1;

        // 三角形 1: i0, i2, i1 (逆時針朝外)
        for (int idx : {i0, i2, i1}) {
          model->positions.insert(model->positions.end(), {spherePositions[idx * 3], spherePositions[idx * 3 + 1],
                                                           spherePositions[idx * 3 + 2]});

          model->normals.insert(model->normals.end(),
                                {sphereNormals[idx * 3], sphereNormals[idx * 3 + 1], sphereNormals[idx * 3 + 2]});

          model->texcoords.insert(model->texcoords.end(), {sphereTexcoords[idx * 2], sphereTexcoords[idx * 2 + 1]});
        }

        // 三角形 2: i0, i3, i2 (逆時針朝外)
        for (int idx : {i0, i3, i2}) {
          model->positions.insert(model->positions.end(), {spherePositions[idx * 3], spherePositions[idx * 3 + 1],
                                                           spherePositions[idx * 3 + 2]});

          model->normals.insert(model->normals.end(),
                                {sphereNormals[idx * 3], sphereNormals[idx * 3 + 1], sphereNormals[idx * 3 + 2]});

          model->texcoords.insert(model->texcoords.end(), {sphereTexcoords[idx * 2], sphereTexcoords[idx * 2 + 1]});
        }

        model->numVertex += 6;
      }
    }
  }
}
// 檢測蛇頭是否碰到蘋果
bool checkAppleCollision() {
  if (!snake || appleModelIndex < 0) return false;

  glm::vec3 headPos = snake->getHeadPosition();
  float distance = glm::length(glm::vec2(headPos.x - applePosition.x, headPos.z - applePosition.z));

  float collisionRadius = 0.5f;  // 碰撞半徑待確認
  return distance < collisionRadius;
}

// 檢測蛇頭是否撞牆
bool checkWallCollision() {
  if (!snake) return false;

  glm::vec3 headPos = snake->getHeadPosition();
  float margin = 0.3f;  // 碰撞半徑待確認

  if (headPos.x < margin || headPos.x > FLOOR_WIDTH - margin || headPos.z < margin ||
      headPos.z > FLOOR_DEPTH - margin) {
    return true;
  }
  return false;
}




// ========== 載入模型 ==========
void loadModels() {
  // 地板 (index 0)
  ctx.models.push_back(createPlane()); // 地板

  // 蛇 (index 1)
  ctx.models.push_back(initializeSnake()); // 蛇模型
  snakeModelIndex = (int)ctx.models.size() - 1;

  // 蘋果
  Model* apple = createApple();
  if (apple) {
    ctx.models.push_back(apple);
    appleModelIndex = (int)ctx.models.size() - 1;
    std::cout << "Loaded: Apple (index " << appleModelIndex << ")" << std::endl;
  }

  // 四面牆壁
  // 前牆 (Z = 0)
  Model* frontWall =
      createWall(FLOOR_WIDTH, WALL_HEIGHT, WALL_THICKNESS, glm::vec3(FLOOR_CENTER_X, 0, 0), glm::vec3(0, 0, 1));
  ctx.models.push_back(frontWall);
  wallModelIndices[0] = (int)ctx.models.size() - 1;

  // 後牆 (Z = FLOOR_DEPTH)
  Model* backWall = createWall(FLOOR_WIDTH, WALL_HEIGHT, WALL_THICKNESS, glm::vec3(FLOOR_CENTER_X, 0, FLOOR_DEPTH),
                               glm::vec3(0, 0, -1));
  ctx.models.push_back(backWall);
  wallModelIndices[1] = (int)ctx.models.size() - 1;

  // 左牆 (X = 0)
  Model* leftWall =
      createWall(WALL_THICKNESS, WALL_HEIGHT, FLOOR_DEPTH, glm::vec3(0, 0, FLOOR_CENTER_Z), glm::vec3(1, 0, 0));
  ctx.models.push_back(leftWall);
  wallModelIndices[2] = (int)ctx.models.size() - 1;

  // 右牆 (X = FLOOR_WIDTH)
  Model* rightWall = createWall(WALL_THICKNESS, WALL_HEIGHT, FLOOR_DEPTH, glm::vec3(FLOOR_WIDTH, 0, FLOOR_CENTER_Z),
                                glm::vec3(-1, 0, 0));
  ctx.models.push_back(rightWall);
  wallModelIndices[3] = (int)ctx.models.size() - 1;

  std::cout << "Loaded: 4 walls" << std::endl;
  std::cout << "=== Total models: " << ctx.models.size() << " ===" << std::endl;
}


// ========== 設置物件 ==========
void setupObjects() {
  // 地板
  ctx.objects.push_back(new Object(0, glm::identity<glm::mat4>()));
  (*ctx.objects.rbegin())->material = mMirror;

  // 12202131 用來測試顯示是否有問題 更12202328 已不需要
  // ctx.objects.push_back(new Object(1, glm::translate(glm::identity<glm::mat4>(), glm::vec3(2.0, 2.0, 2.0))));
  //(*ctx.objects.rbegin())->material = mShinyred;

  
  // 蛇
  // ctx.objects.push_back(new Object(snakeModelIndex, glm::translate(glm::identity<glm::mat4>(), glm::vec3(0.0, 0.0,
  // 0.0)))); //12210411原本想嘗試用這個框架的，但好像確實有渲染的問題，要改別的檔TwT，所以再改回去
  

  // 蘋果
  if (appleModelIndex >= 0) {
    ctx.objects.push_back(new Object(appleModelIndex, glm::identity<glm::mat4>()));
    (*ctx.objects.rbegin())->material = mFlatwhite;
  }

  // 四面牆壁
  for (int i = 0; i < 4; i++) {
    if (wallModelIndices[i] >= 0) {
      ctx.objects.push_back(new Object(wallModelIndices[i], glm::identity<glm::mat4>()));
      (*ctx.objects.rbegin())->material = mShinyred;
    }
  }
}

// ========== 渲染蛇 ==========
void renderSnake() {
  if (!snake || snakeModelIndex < 0 || snakeModelIndex >= ctx.models.size()) {
    return;
  }

  glUseProgram(ctx.programs[1]->getProgramId());
  GLuint program = ctx.programs[1]->getProgramId();

  glm::mat4 modelMatrix = glm::identity<glm::mat4>();
  glUniformMatrix4fv(glGetUniformLocation(program, "ModelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

  glUniform3fv(glGetUniformLocation(program, "material.ambient"), 1, glm::value_ptr(mFlatwhite.ambient));
  glUniform3fv(glGetUniformLocation(program, "material.diffuse"), 1, glm::value_ptr(mFlatwhite.diffuse));
  glUniform3fv(glGetUniformLocation(program, "material.specular"), 1, glm::value_ptr(mFlatwhite.specular));
  glUniform1f(glGetUniformLocation(program, "material.shininess"), mFlatwhite.shininess);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, ctx.models[snakeModelIndex]->textures[0]);
  glUniform1i(glGetUniformLocation(program, "ourTexture"), 0);

  std::vector<float> positions = ctx.models[snakeModelIndex]->positions;
  std::vector<float> norms = ctx.models[snakeModelIndex]->normals;
  std::vector<float> texcoords = ctx.models[snakeModelIndex]->texcoords;

  GLuint vao, vbo[3];
  glGenVertexArrays(1, &vao);
  glGenBuffers(3, vbo);

  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
  glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), positions.data(), GL_DYNAMIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
  glBufferData(GL_ARRAY_BUFFER, norms.size() * sizeof(float), norms.data(), GL_DYNAMIC_DRAW);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
  glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(float), texcoords.data(), GL_DYNAMIC_DRAW);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(2);

  // 改用 GL_TRIANGLES 繪製！
  glDrawArrays(GL_TRIANGLES, 0, ctx.models[snakeModelIndex]->numVertex);

  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(3, vbo);
  
    // --- 渲染長方體 --- 12211730 debug用 還在修正
    // 假設你的長方體已經建立並加入至模型
  // --- 渲染方向指示器 ---
  if (glm::length(snake->getForwardDirection()) >= 0.001f) {
    // 從第二個質點（身體）出發，不穿過頭
    const auto& masses = snake->getMasses();
    glm::vec3 startPos;
    if (masses.size() >= 2) {
      startPos = masses[0]->getPosition();  // 從第二節身體出發
    } else {
      startPos = snake->getHeadPosition();
    }

    glm::vec3 direction = -snake->getForwardDirection();

    // 參數：起點, 方向, 長度, 寬度, 高度
    // 長度短一點 (0.15), 寬度是原本兩倍 (0.04)
    Model* boxModel = createDirectionBox(startPos, snake->getForwardDirection(), 0.3f, 0.04f, 0.02f);

    glUseProgram(program);

    glm::mat4 boxModelMatrix = glm::identity<glm::mat4>();
    glUniformMatrix4fv(glGetUniformLocation(program, "ModelMatrix"), 1, GL_FALSE, glm::value_ptr(boxModelMatrix));

    // 設定紅色材質
    glm::vec3 redAmbient(0.3f, 0.0f, 0.0f);
    glm::vec3 redDiffuse(1.0f, 0.0f, 0.0f);
    glm::vec3 redSpecular(0.5f, 0.2f, 0.2f);
    float redShininess = 32.0f;

    glUniform3fv(glGetUniformLocation(program, "material.ambient"), 1, glm::value_ptr(redAmbient));
    glUniform3fv(glGetUniformLocation(program, "material.diffuse"), 1, glm::value_ptr(redDiffuse));
    glUniform3fv(glGetUniformLocation(program, "material.specular"), 1, glm::value_ptr(redSpecular));
    glUniform1f(glGetUniformLocation(program, "material.shininess"), redShininess);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, boxModel->textures[0]);
    glUniform1i(glGetUniformLocation(program, "ourTexture"), 0);

    std::vector<float> boxPositions = boxModel->positions;
    std::vector<float> boxNormals = boxModel->normals;
    std::vector<float> boxTexcoords = boxModel->texcoords;

    GLuint boxVAO, boxVBO[3];
    glGenVertexArrays(1, &boxVAO);
    glGenBuffers(3, boxVBO);

    glBindVertexArray(boxVAO);

    glBindBuffer(GL_ARRAY_BUFFER, boxVBO[0]);
    glBufferData(GL_ARRAY_BUFFER, boxPositions.size() * sizeof(float), boxPositions.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, boxVBO[1]);
    glBufferData(GL_ARRAY_BUFFER, boxNormals.size() * sizeof(float), boxNormals.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, boxVBO[2]);
    glBufferData(GL_ARRAY_BUFFER, boxTexcoords.size() * sizeof(float), boxTexcoords.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(2);

    glDrawArrays(GL_QUADS, 0, boxModel->numVertex);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteVertexArrays(1, &boxVAO);
    glDeleteBuffers(3, boxVBO);

    delete boxModel;  // 記得釋放記憶體！
  }
  //glEnable(GL_CULL_FACE);
}

void renderSnakeShadow(GLuint shadowProgram) {
  if (!snake || snakeModelIndex < 0 || snakeModelIndex >= ctx.models.size()) {
    return;
  }

  glm::mat4 snakeModelMatrix = glm::identity<glm::mat4>();
  glUniformMatrix4fv(glGetUniformLocation(shadowProgram, "ModelMatrix"), 1, GL_FALSE, glm::value_ptr(snakeModelMatrix));

  std::vector<float>& positions = ctx.models[snakeModelIndex]->positions;

  GLuint snakeVAO, snakeVBO;
  glGenVertexArrays(1, &snakeVAO);
  glGenBuffers(1, &snakeVBO);

  glBindVertexArray(snakeVAO);
  glBindBuffer(GL_ARRAY_BUFFER, snakeVBO);
  glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), positions.data(), GL_DYNAMIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);

  glDrawArrays(GL_TRIANGLES, 0, ctx.models[snakeModelIndex]->numVertex);

  glBindVertexArray(0);
  glDeleteVertexArrays(1, &snakeVAO);
  glDeleteBuffers(1, &snakeVBO);
}


// ========== 主程式 ==========
int main() {
  initOpenGL();
  GLFWwindow* window = OpenGLContext::getWindow();
  glfwSetWindowTitle(window, "HW2 - 113550001");

  Camera camera(glm::vec3(FLOOR_CENTER_X-2, 6, FLOOR_CENTER_Z +8));
  camera.initialize(OpenGLContext::getAspectRatio());
  glfwSetWindowUserPointer(window, &camera);
  ctx.camera = &camera;
  ctx.window = window;

  loadMaterial();
  loadModels();
  loadPrograms();
  // initializeSnake(); 12202116 標註為外加的部分，先將這個隱藏退回原本狀態 更12202336 已移到 loadModels 裡面
  setupObjects();
  initShadowMap();

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330 core");
  float lastTime = (float)glfwGetTime();

  int frameCount = 0;
  float fpsTimer = 0.0f;


  
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
   
    if (snakeViewMode && snake) {
      // 蛇視角模式
      glm::vec3 headPos = snake->getHeadPosition();
      glm::vec3 forwardDir = snake->getForwardDirection();

      if (glm::length(forwardDir) < 0.001f) {
        forwardDir = glm::vec3(0, 0, -1);
      }

      // 相機在在蛇頭後上方
      float heightOffset = 1.5f;  // 高度偏移
      float backOffset = 2.0f;    // 往後偏移

      glm::vec3 cameraPos = headPos - forwardDir * backOffset + glm::vec3(0, heightOffset, 0);

     
      glm::vec3 lookAtPos = headPos + forwardDir * 1.0f;

      camera.setPosition(cameraPos);
      camera.setLookAt(lookAtPos);
    } else {
      // 自由視角模式
      camera.move(window);
    }



    float currentTime = (float)glfwGetTime();
    float deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    frameCount++;
    fpsTimer += deltaTime;
    if (fpsTimer >= 1.0f) {
      float fps = frameCount / fpsTimer;
      std::cout << "[FPS] " << fps << std::endl;
      if (fps < 30.0f) {
        std::cerr << "[WARNING] Low FPS detected!" << std::endl;
      }
      frameCount = 0;
      fpsTimer = 0.0f;
    }

    // **限制極端 deltaTime**
    if (deltaTime > 0.05f) {
      std::cerr << "[WARNING] Large frame time: " << deltaTime << std::endl;
      deltaTime = 0.05f;
    }
    // 蛇控制
    /* 12202116 標註為外加的部分，先將這個隱藏退回原本狀態 更12210116 我正在將這個部分更改到 keyCallback，實際操作改到update之類的地方
    if (snake) {
      bool shouldMove = false;

      if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
        shouldMove = true;
        snake->addForwardForce(8.0f);
      }

      if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
        snake->addForwardForce(-2.0f);
      }

      if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) {
        shouldMove = true;
        glm::vec3 dir = snake->getForwardDirection();
        float angle = glm::radians(2.0f * deltaTime * 60.0f);
        glm::vec3 newDir(dir.x * cos(angle) - dir.z * sin(angle), 0.0f, dir.x * sin(angle) + dir.z * cos(angle));
        snake->setTargetDirection(newDir);
      }

      if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        shouldMove = true;
        glm::vec3 dir = snake->getForwardDirection();
        float angle = glm::radians(-2.0f * deltaTime * 60.0f);
        glm::vec3 newDir(dir.x * cos(angle) - dir.z * sin(angle), 0.0f, dir.x * sin(angle) + dir.z * cos(angle));
        snake->setTargetDirection(newDir);
      }

      if (shouldMove) {
        snake->startMoving();
      } else {
        snake->stopMoving();
      }
    }
    */
    
    if (gameState == GameState::RUNNING) {
      // 更新計時器
      gameTimer -= deltaTime;
      elapsedTime += deltaTime;

      // 時間到
      if (gameTimer <= 0.0f) {
        gameTimer = 0.0f;
        gameState = GameState::STOPPED;
        if (score >= WIN_SCORE) {
          gameResult = GameResult::WIN;
        } else {
          gameResult = GameResult::LOSE;
        }
        std::cout << "[INFO] Time's up!" << std::endl;
      }

      // 更新蛇
      updateSnake(deltaTime);

      // 檢測蘋果碰撞
      if (checkAppleCollision()) {
        score++;
        std::cout << "[SCORE] Ate an apple! Score: " << score << std::endl;
        respawnApple();

        for (auto& obj : ctx.objects) {
          if (obj->modelIndex == appleModelIndex) {
            obj->transformMatrix = glm::identity<glm::mat4>();
          }
        }

        // 檢查是否贏了
        if (score >= WIN_SCORE) {
          gameState = GameState::STOPPED;
          gameResult = GameResult::WIN;
          std::cout << "[INFO] You Win!" << std::endl;
        }
      }

      // 檢測牆壁碰撞
      if (checkWallCollision()) {
        if (!hitWall) {
          hitWall = true;
          gameState = GameState::STOPPED;
          gameResult = GameResult::LOSE;
          std::cout << "[WARNING] Hit the wall! Game Over!" << std::endl;
        }
      }
    } else {
      // 遊戲暫停或停止時，仍然渲染蛇（但不更新物理）
      // 不呼叫 updateSnake
    }



    if (checkAppleCollision()) {
      score++;
      std::cout << "[SCORE] Ate an apple! Score: " << score << std::endl;
      respawnApple();

      // 更新蘋果的 Object transform
      for (auto& obj : ctx.objects) {
        if (obj->modelIndex == appleModelIndex) {
          obj->transformMatrix = glm::identity<glm::mat4>();
        }
      }
    }

    // 檢測牆壁碰撞
    if (checkWallCollision()) {
      if (!hitWall) {
        hitWall = true;
        std::cout << "[WARNING] Hit the wall!" << std::endl;
        // 待加入遊戲結束邏輯
      }
    } else {
      hitWall = false;  // 離開牆壁後重置
    }

    
    //glDisable(GL_CULL_FACE); // 12210216這個我不太懂，但我覺得應該沒差？
    
    // Shadow pass - Direction light
    if (ctx.directionLightEnable) {
      glm::vec3 lightDir = glm::normalize(ctx.directionLightDirection);
      glm::vec3 lightPos = -lightDir * 15.0f + glm::vec3(4.096f, 0.0f, 2.56f);
      glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 50.0f);
      glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(4.096f, 0.0f, 2.56f), glm::vec3(0.0f, 1.0f, 0.0f));
      ctx.lightSpaceMatrix = lightProjection * lightView;

      glViewport(0, 0, ctx.shadowMapWidth, ctx.shadowMapHeight);
      glBindFramebuffer(GL_FRAMEBUFFER, ctx.shadowMapFBO);
      glClear(GL_DEPTH_BUFFER_BIT);

      GLuint shadowProgram = ctx.programs[2]->getProgramId();
      glUseProgram(shadowProgram);
      glUniformMatrix4fv(glGetUniformLocation(shadowProgram, "lightSpaceMatrix"), 1, GL_FALSE,
                         glm::value_ptr(ctx.lightSpaceMatrix));

      for (auto& obj : ctx.objects) {
        renderSnakeShadow(shadowProgram);
        //if (obj->modelIndex == snakeModelIndex) continue; 12202116 標註為外加的部分，先將這個隱藏退回原本狀態
        Model* model = ctx.models[obj->modelIndex];
        glm::mat4 modelMatrix = obj->transformMatrix * model->modelMatrix;
        glUniformMatrix4fv(glGetUniformLocation(shadowProgram, "ModelMatrix"), 1, GL_FALSE,
                           glm::value_ptr(modelMatrix));
        glBindVertexArray(ctx.programs[1]->VAO[obj->modelIndex]);
        glDrawArrays(model->drawMode, 0, model->numVertex);
      }

      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Shadow pass - Spot light
    if (ctx.spotLightEnable) {
      glm::vec3 spotPos = ctx.spotLightPosition;
      glm::vec3 spotDir = glm::normalize(ctx.spotLightDirection);
      glm::vec3 spotTarget = spotPos + spotDir;

      glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
      if (abs(glm::dot(spotDir, up)) > 0.99f) {
        up = glm::vec3(1.0f, 0.0f, 0.0f);
      }

      glm::mat4 spotProjection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 25.0f);
      glm::mat4 spotView = glm::lookAt(spotPos, spotTarget, up);
      ctx.spotLightSpaceMatrix = spotProjection * spotView;

      glViewport(0, 0, ctx.shadowMapWidth, ctx.shadowMapHeight);
      glBindFramebuffer(GL_FRAMEBUFFER, ctx.spotShadowMapFBO);
      glClear(GL_DEPTH_BUFFER_BIT);

      GLuint shadowProgram = ctx.programs[2]->getProgramId();
      glUseProgram(shadowProgram);
      glUniformMatrix4fv(glGetUniformLocation(shadowProgram, "lightSpaceMatrix"), 1, GL_FALSE,
                         glm::value_ptr(ctx.spotLightSpaceMatrix));

      for (auto& obj : ctx.objects) {
        //if (obj->modelIndex == snakeModelIndex) continue; 12202116 標註為外加的部分，先將這個隱藏退回原本狀態
        Model* model = ctx.models[obj->modelIndex];
        glm::mat4 modelMatrix = obj->transformMatrix * model->modelMatrix;
        glUniformMatrix4fv(glGetUniformLocation(shadowProgram, "ModelMatrix"), 1, GL_FALSE,
                           glm::value_ptr(modelMatrix));
        glBindVertexArray(ctx.programs[1]->VAO[obj->modelIndex]);
        glDrawArrays(model->drawMode, 0, model->numVertex);
      }

      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Shadow pass - Point light
    if (ctx.pointLightEnable) {
      float nearPlane = 0.1f;
      float farPlane = ctx.pointLightFarPlane;
      glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, nearPlane, farPlane);
      glm::vec3 lightPos = ctx.pointLightPosition;

      glm::mat4 shadowTransforms[6] = {
          shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1, 0, 0), glm::vec3(0, -1, 0)),
          shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0)),
          shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
          shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1)),
          shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0, 0, 1), glm::vec3(0, -1, 0)),
          shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0, 0, -1), glm::vec3(0, -1, 0))};

      glViewport(0, 0, 1024, 1024);
      GLuint shadowProgram = ctx.programs[2]->getProgramId();
      glUseProgram(shadowProgram);

      for (int face = 0; face < 6; ++face) {
        glBindFramebuffer(GL_FRAMEBUFFER, ctx.pointShadowMapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                               ctx.pointShadowMap, 0);
        glClear(GL_DEPTH_BUFFER_BIT);

        glUniformMatrix4fv(glGetUniformLocation(shadowProgram, "lightSpaceMatrix"), 1, GL_FALSE,
                           glm::value_ptr(shadowTransforms[face]));

        for (auto& obj : ctx.objects) {
          //if (obj->modelIndex == snakeModelIndex) continue; 12202116 標註為外加的部分，先將這個隱藏退回原本狀態
          Model* model = ctx.models[obj->modelIndex];
          glm::mat4 modelMatrix = obj->transformMatrix * model->modelMatrix;
          glUniformMatrix4fv(glGetUniformLocation(shadowProgram, "ModelMatrix"), 1, GL_FALSE,
                             glm::value_ptr(modelMatrix));
          glBindVertexArray(ctx.programs[1]->VAO[obj->modelIndex]);
          glDrawArrays(model->drawMode, 0, model->numVertex);
        }

      }

      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // 恢復視口
    int windowWidth, windowHeight;
    glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);

    // 清除緩衝
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 啟用深度測試和背面剔除
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);

    // 渲染場景
    for (size_t i = 0; i < ctx.programs.size(); i++) {
      ctx.programs[i]->doMainLoop();
    }
    
    renderSnake();
    /*
    // ImGui
    glDisable(GL_CULL_FACE);
    */

    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
      // 設定視窗大小和位置（置中顯示）
      ImGui::SetNextWindowSize(ImVec2(300, 250), ImGuiCond_FirstUseEver);
      ImGui::Begin("Game Control");

      // ===== 遊戲狀態顯示 =====
      ImGui::Separator();

      // 根據遊戲狀態顯示不同內容
      if (gameState == GameState::STOPPED && gameResult == GameResult::NONE) {
        // 遊戲尚未開始
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Press P to Start!");
        ImGui::Text("Collect %d apples to win!", WIN_SCORE);
        ImGui::Text("Time limit: 60 seconds");
      } else if (gameState == GameState::RUNNING) {
        // 遊戲進行中
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "GAME RUNNING");

        // 顯示倒數計時
        int minutes = (int)gameTimer / 60;
        int seconds = (int)gameTimer % 60;
        ImGui::Text("Time: %d:%02d", minutes, seconds);

        // 進度條
        float progress = gameTimer / 60.0f;
        ImGui::ProgressBar(progress, ImVec2(-1, 0), "");
      } else if (gameState == GameState::PAUSED) {
        // 遊戲暫停
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "PAUSED");
        ImGui::Text("Press P to Resume");

        int minutes = (int)gameTimer / 60;
        int seconds = (int)gameTimer % 60;
        ImGui::Text("Time: %d:%02d", minutes, seconds);
      }

      ImGui::Separator();

      // ===== 分數顯示 =====
      ImGui::Text("Score: %d / %d", score, WIN_SCORE);

      // 分數進度條
      float scoreProgress = (float)score / WIN_SCORE;
      ImGui::ProgressBar(scoreProgress, ImVec2(-1, 0), "");

      // ===== 遊戲結果顯示 =====
      if (gameResult == GameResult::WIN) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "*** YOU WIN! ***");
        ImGui::Text("Final Score: %d", score);
        ImGui::Text("Press P to Play Again");
      } else if (gameResult == GameResult::LOSE) {
        ImGui::Separator();
        if (hitWall) {
          ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "*** YOU LOSE! ***");
          ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Hit the wall!");
        } else {
          ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "*** YOU LOSE! ***");
          ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Time's up!");
        }
        ImGui::Text("Final Score: %d", score);
        ImGui::Text("Press P to Play Again");
      }

      ImGui::Separator();

      // ===== 蘋果位置（debug 用）=====
      ImGui::Text("Apple: (%.1f, %.1f)", applePosition.x, applePosition.z);

      // ===== 控制說明 =====
      ImGui::Separator();
      ImGui::Text("Controls:");
      ImGui::BulletText("P: Start/Pause/Restart");
      ImGui::BulletText("I: Forward");
      ImGui::BulletText("J/L: Turn Left/Right");
      ImGui::BulletText("M: Switch Movement Mode");
      ImGui::BulletText("F1: Toggle Cursor");

      ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


#ifdef __APPLE__
    glFlush();
#endif
    glfwSwapBuffers(window);
  }

  // 清理
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  return 0;
}

// ========== 按鍵回調 ==========
void keyCallback(GLFWwindow* window, int key, int, int action, int) {
  if (key == GLFW_KEY_ESCAPE) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
    return;
  }

  if (action == GLFW_PRESS) {
    switch (key) {
      case GLFW_KEY_P: {
        // P 鍵：開始/暫停/重新開始
        if (gameState == GameState::STOPPED) {
          // 重置遊戲
          gameState = GameState::RUNNING;
          gameResult = GameResult::NONE;
          gameTimer = 60.0f;
          elapsedTime = 0.0f;
          score = 0;
          hitWall = false;

          // 重置蛇位置
          if (snake) {
            snake->reset();
          }

          // 重置蘋果位置
          respawnFirstApple();

          std::cout << "[INFO] Game Started!" << std::endl;
        } else if (gameState == GameState::RUNNING) {
          // 暫停
          gameState = GameState::PAUSED;
          std::cout << "[INFO] Game Paused" << std::endl;
        } else if (gameState == GameState::PAUSED) {
          // 繼續
          gameState = GameState::RUNNING;
          std::cout << "[INFO] Game Resumed" << std::endl;
        }
        break;
      }

      case GLFW_KEY_F1: {
        Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));
        if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
          glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
          int ww, hh;
          glfwGetWindowSize(window, &ww, &hh);
          glfwSetCursorPos(window, static_cast<double>(ww) / 2.0, static_cast<double>(hh) / 2.0);
          if (cam) cam->setLastMousePos(window);
        } else {
          glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
          if (cam) cam->setLastMousePos(window);
        }
        break;
      }

      case GLFW_KEY_B: {
        ctx.shadowEnable = !ctx.shadowEnable;
        break;
      }

      case GLFW_KEY_N: {
        ctx.spotLightCutOffEnable = !ctx.spotLightCutOffEnable;
        break;
      }

      case GLFW_KEY_M: {
        if (snake) {
          if (snake->getMode() == Snake::MovementMode::LATERAL) {
            snake->setMovementMode(Snake::MovementMode::RECTILINEAR);
            std::cout << "Switched to RECTILINEAR PROGRESSION" << std::endl;
          } else if (snake->getMode() == Snake::MovementMode::RECTILINEAR) {
            snake->setMovementMode(Snake::MovementMode::SIMPLE);
            std::cout << "Switched to SIMPLE MOVEMENT" << std::endl;
          } else if (snake->getMode() == Snake::MovementMode::SIMPLE) {
            snake->setMovementMode(Snake::MovementMode::LATERAL);
            std::cout << "Switched to LATERAL UNDULATION" << std::endl;
          }
        }
        break;
      }

      // 蛇的控制只在遊戲進行中有效
      case GLFW_KEY_I:
        if (gameState == GameState::RUNNING && snake) {
          snake->setSnakeMoveDirection(0, true);
        }
        break;
      case GLFW_KEY_J:
        if (gameState == GameState::RUNNING && snake) {
          snake->setSnakeMoveDirection(1, true);
        }
        break;
      case GLFW_KEY_L:
        if (gameState == GameState::RUNNING && snake) {
          snake->setSnakeMoveDirection(2, true);
        }
        break;
      case GLFW_KEY_Y: {
        snakeViewMode = !snakeViewMode;
        if (snakeViewMode) {
          std::cout << "Snake View" << std::endl;
        } else {
          std::cout << "Free Camera" << std::endl;
        }
        break;
      }
      default:
        break;
    }
  } else if (action == GLFW_RELEASE) {
    switch (key) {
      case GLFW_KEY_I:
        if (snake) snake->setSnakeMoveDirection(0, false);
        break;
      case GLFW_KEY_J:
        if (snake) snake->setSnakeMoveDirection(1, false);
        break;
      case GLFW_KEY_L:
        if (snake) snake->setSnakeMoveDirection(2, false);
        break;
      default:
        break;
    }
  }
}

// ========== 視窗大小回調 ==========
void resizeCallback(GLFWwindow* window, int width, int height) {
  OpenGLContext::framebufferResizeCallback(window, width, height);
  auto ptr = static_cast<Camera*>(glfwGetWindowUserPointer(window));
  if (ptr) {
    ptr->updateProjectionMatrix(OpenGLContext::getAspectRatio());
  }
}

// ========== OpenGL 初始化 ==========
void initOpenGL() {
#ifdef __APPLE__
  OpenGLContext::createContext(21, GLFW_OPENGL_ANY_PROFILE);
#else
  OpenGLContext::createContext(21, GLFW_OPENGL_ANY_PROFILE);
#endif

  GLFWwindow* window = OpenGLContext::getWindow();
  glfwSetKeyCallback(window, keyCallback);
  glfwSetFramebufferSizeCallback(window, resizeCallback);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

#ifndef NDEBUG
  OpenGLContext::printSystemInfo();
  OpenGLContext::enableDebugCallback();
#endif
}