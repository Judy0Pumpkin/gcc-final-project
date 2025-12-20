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

#include "Snake.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

// ========== 全域變數 ==========
Snake* snake = nullptr;
int snakeModelIndex = -1;
/* 12202116 標註為外加的部分，先將這個隱藏退回原本狀態
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

  float width = 8.192f;
  float depth = 5.12f;
  float centerX = 4.096f;
  float centerZ = 2.56f;
  float y = 0.0f;

  float halfWidth = width / 2.0f;
  float halfDepth = depth / 2.0f;

  glm::vec3 v0(centerX - halfWidth, y, centerZ - halfDepth);
  glm::vec3 v1(centerX - halfWidth, y, centerZ + halfDepth);
  glm::vec3 v2(centerX + halfWidth, y, centerZ + halfDepth);
  glm::vec3 v3(centerX + halfWidth, y, centerZ - halfDepth);

  glm::vec3 normal(0.0f, 1.0f, 0.0f);

  float uRepeat = 2.0f;
  float vRepeat = 2.0f;

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
  float radius = 0.15f;

  for (int i = 0; i < (int)masses.size(); ++i) {
    glm::vec3 pos = masses[i]->getPosition();
    float r = radius;

    // 立方體 8 個頂點
    glm::vec3 v[8] = {pos + glm::vec3(-r, -r, -r), pos + glm::vec3(r, -r, -r), pos + glm::vec3(r, r, -r),
                      pos + glm::vec3(-r, r, -r),  pos + glm::vec3(-r, -r, r), pos + glm::vec3(r, -r, r),
                      pos + glm::vec3(r, r, r),    pos + glm::vec3(-r, r, r)};

    // 6 個面的索引
    int faces[24] = {
        0, 3, 2, 1,  // 前
        4, 5, 6, 7,  // 後
        7, 3, 0, 4,  // 左
        5, 1, 2, 6,  // 右
        4, 0, 1, 5,  // 下
        6, 2, 3, 7,   // 上
    };

    // 6 個面的法線
    glm::vec3 normals[6] = {glm::vec3(0, 0, -1), glm::vec3(0, 0, 1),  glm::vec3(-1, 0, 0),
                            glm::vec3(1, 0, 0),  glm::vec3(0, -1, 0), glm::vec3(0, 1, 0)};

    
    for (int f = 0; f < 24; ++f) {
      glm::vec3 vertex = v[faces[f]];
      glm::vec3 normal = normals[f / 4];

      // 第一節蛇頭的 texcoords 用左半邊 [0.0, 0.5]，蛇身用右半邊 [0.5, 1.0]
      float u, v;

      if (i == 0) {                      // 如果是蛇頭
        u = (f % 4 < 2) ? 0.0f : 0.5f;
      } else {                           // 蛇身
        u = (f % 4 < 2) ? 0.5f : 1.0f;
      }

      if (f % 4 == 1 || f % 4 == 2) {
        v = 1.0f;
      } else {
        v = 0.0f;
      }

      m->positions.push_back(vertex.x);
      m->positions.push_back(vertex.y);
      m->positions.push_back(vertex.z);

      m->normals.push_back(normal.x);
      m->normals.push_back(normal.y);
      m->normals.push_back(normal.z);

      m->texcoords.push_back(u);
      m->texcoords.push_back(v);
    }

    m->numVertex += 24;
  }
  m->textures.push_back(createTexture("../assets/models/snake/snake.jpg"));
  m->drawMode = GL_QUADS;
  return m;
}

// ========== 蛇初始化 ==========
Model* initializeSnake() {
  std::cout << "\n=== Initializing Snake ===" << std::endl;

  // 參數：節數, 質量, 每段長度, 彈簧常數, 阻尼, 起始位置
  snake = new Snake(12, 0.3f, 0.4f, 0.5f, 3.5f, glm::vec3(2.0f, 0.5f, 2.5f));

  Model* snakeModel = createSnakeModelSimple(snake);
  // ctx.models.push_back(snakeModel); // 12202326 我想將他移到統一的地方，所以先試著註解掉
  // snakeModelIndex = ctx.models.size() - 1; // 12202326 我想將他移到統一的地方，所以先試著註解掉

  std::cout << "Snake initialized!" << std::endl;
  return snakeModel;
}


// ========== 蛇更新 ==========
/* 12202116 標註為外加的部分，先將這個隱藏退回原本狀態
void updateSnake(float deltaTime) {
  if (!snake || snakeModelIndex < 0 || snakeModelIndex >= ctx.models.size()) {
    return;
  }

  simulationTime += deltaTime;
  snake->update(deltaTime, simulationTime);

  // 更新渲染模型
  Model* oldModel = ctx.models[snakeModelIndex];
  oldModel->positions.clear();
  oldModel->normals.clear();
  oldModel->texcoords.clear();
  oldModel->numVertex = 0;

  Model* newModel = createSnakeModelSimple(snake);
  oldModel->positions = newModel->positions;
  oldModel->normals = newModel->normals;
  oldModel->texcoords = newModel->texcoords;
  oldModel->numVertex = newModel->numVertex;

  delete newModel;
}
*/
// ========== 載入模型 ==========
void loadModels() {
  ctx.models.push_back(createPlane());  // 地板
  //ctx.models.push_back(createTestBox());  // 12202131 用來測試顯示是否有問題 更12202328 已不需要
  ctx.models.push_back(initializeSnake());  // 蛇模型
  snakeModelIndex = (int)ctx.models.size() - 1;
}

// ========== 設置物件 ==========
void setupObjects() {
  // 地板
  ctx.objects.push_back(new Object(0, glm::translate(glm::identity<glm::mat4>(), glm::vec3(0.0, 0.0, 0.0))));
  (*ctx.objects.rbegin())->material = mMirror;

  // 12202131 用來測試顯示是否有問題 更12202328 已不需要
  //ctx.objects.push_back(new Object(1, glm::translate(glm::identity<glm::mat4>(), glm::vec3(2.0, 2.0, 2.0))));
  //(*ctx.objects.rbegin())->material = mShinyred;

  // 蛇
  ctx.objects.push_back(
      new Object(snakeModelIndex, glm::translate(glm::identity<glm::mat4>(), glm::vec3(0.0, 0.0, 0.0))));


}

// ========== 渲染蛇 ==========
/* 12202116 標註為外加的部分，先將這個隱藏退回原本狀態
void renderSnake() {
  if (!snake || snakeModelIndex < 0 || snakeModelIndex >= ctx.models.size()) {
    return;
  }

  const auto& masses = snake->getMasses();
  float radius = 0.15f;

  glDisable(GL_CULL_FACE);
  glUseProgram(ctx.programs[1]->getProgramId());
  GLuint program = ctx.programs[1]->getProgramId();

  glm::mat4 modelMatrix = glm::identity<glm::mat4>();
  glUniformMatrix4fv(glGetUniformLocation(program, "ModelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

  // 為每個質點渲染立方體
  for (size_t i = 0; i < masses.size(); ++i) {
    glm::vec3 pos = masses[i]->getPosition();
    float r = radius;

    // 選擇材質：頭部綠色，身體紅色
    Material* currentMat = (i == 0) ? &mGreenHead : &mShinyred;

    glUniform3fv(glGetUniformLocation(program, "material.ambient"), 1, glm::value_ptr(currentMat->ambient));
    glUniform3fv(glGetUniformLocation(program, "material.diffuse"), 1, glm::value_ptr(currentMat->diffuse));
    glUniform3fv(glGetUniformLocation(program, "material.specular"), 1, glm::value_ptr(currentMat->specular));
    glUniform1f(glGetUniformLocation(program, "material.shininess"), currentMat->shininess);

    // 立方體頂點
    glm::vec3 v[8] = {pos + glm::vec3(-r, -r, -r), pos + glm::vec3(r, -r, -r), pos + glm::vec3(r, r, -r),
                      pos + glm::vec3(-r, r, -r),  pos + glm::vec3(-r, -r, r), pos + glm::vec3(r, -r, r),
                      pos + glm::vec3(r, r, r),    pos + glm::vec3(-r, r, r)};

    int faces[36] = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4, 0, 4, 7, 7, 3, 0,
                     1, 5, 6, 6, 2, 1, 0, 1, 5, 5, 4, 0, 3, 2, 6, 6, 7, 3};

    glm::vec3 normals[6] = {glm::vec3(0, 0, -1), glm::vec3(0, 0, 1),  glm::vec3(-1, 0, 0),
                            glm::vec3(1, 0, 0),  glm::vec3(0, -1, 0), glm::vec3(0, 1, 0)};

    std::vector<float> positions, norms, texcoords;

    for (int f = 0; f < 36; ++f) {
      glm::vec3 vertex = v[faces[f]];
      glm::vec3 normal = normals[f / 6];

      positions.insert(positions.end(), {vertex.x, vertex.y, vertex.z});
      norms.insert(norms.end(), {normal.x, normal.y, normal.z});
      texcoords.insert(texcoords.end(), {0.0f, 0.0f});
    }

    // 創建臨時 VAO 和 VBO
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

    glDrawArrays(GL_TRIANGLES, 0, 36);

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(3, vbo);
  }

  glEnable(GL_CULL_FACE);
}
*/
// ========== 主程式 ==========
int main() {
  initOpenGL();
  GLFWwindow* window = OpenGLContext::getWindow();
  glfwSetWindowTitle(window, "HW2 - 113550001");

  Camera camera(glm::vec3(0, 2, 5));
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

  // 主渲染循環
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    camera.move(window);

    /* 12202116 標註為外加的部分，先將這個隱藏退回原本狀態
    float currentTime = (float)glfwGetTime();
    static float lastTime = 0.0f;
    float deltaTime = currentTime - lastTime;
    lastTime = currentTime;
    if (deltaTime > 0.05f) deltaTime = 0.05f;
    */

    // 蛇控制
    /* 12202116 標註為外加的部分，先將這個隱藏退回原本狀態
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
    /* 12202116 標註為外加的部分，先將這個隱藏退回原本狀態
    updateSnake(deltaTime);
    
    glDisable(GL_CULL_FACE);
    */
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
    /* 12202116 標註為外加的部分，先將這個隱藏退回原本狀態
    renderSnake();

    // ImGui
    glDisable(GL_CULL_FACE);
    */
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
      ImGui::Begin("Lights Control");

      ImGui::Text("Directional Light");
      {
        ImGui::SameLine();
        bool enable = (ctx.directionLightEnable != 0);
        if (ImGui::Checkbox("Enable##dir", &enable)) ctx.directionLightEnable = enable ? 1 : 0;
        ImGui::SliderFloat3("Dir X/Y/Z##dir", &ctx.directionLightDirection.x, -50.0f, 50.0f);
        ImGui::ColorEdit3("Color##dir", &ctx.directionLightColor[0]);
      }
      ImGui::Separator();

      ImGui::Text("Point Light");
      {
        ImGui::SameLine();
        bool enable = (ctx.pointLightEnable != 0);
        if (ImGui::Checkbox("Enable##point", &enable)) ctx.pointLightEnable = enable ? 1 : 0;
        ImGui::SliderFloat3("Pos X/Y/Z##point", &ctx.pointLightPosition.x, -10.0f, 10.0f);
        ImGui::ColorEdit3("Color##point", &ctx.pointLightColor[0]);
      }
      ImGui::Separator();

      ImGui::Text("Spot Light");
      {
        ImGui::SameLine();
        bool enable = (ctx.spotLightEnable != 0);
        if (ImGui::Checkbox("Enable##spot", &enable)) ctx.spotLightEnable = enable ? 1 : 0;
        ImGui::SliderFloat3("Pos X/Y/Z##spot", &ctx.spotLightPosition.x, -10.0f, 10.0f);
        ImGui::ColorEdit3("Color##spot", &ctx.spotLightColor[0]);
      }
      ImGui::Separator();

      ImGui::Text("Controls:");
      ImGui::BulletText("I: Forward");
      ImGui::BulletText("K: Backward");
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
        /* 12202116 標註為外加的部分，先將這個隱藏退回原本狀態
      case GLFW_KEY_M: {
        if (snake) {
          if (currentSnakeMode == SnakeMode::LATERAL_UNDULATION) {
            currentSnakeMode = SnakeMode::RECTILINEAR_PROGRESSION;
            snake->setMovementMode(Snake::MovementMode::RECTILINEAR);
            std::cout << "Switched to RECTILINEAR PROGRESSION" << std::endl;
          } else {
            currentSnakeMode = SnakeMode::LATERAL_UNDULATION;
            snake->setMovementMode(Snake::MovementMode::LATERAL);
            std::cout << "Switched to LATERAL UNDULATION" << std::endl;
          }
        }
        break;
      }
      */
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