#include "Spring.h"
#include <glm/glm.hpp>

/**
 * 構造函數
 */
Spring::Spring(Mass* m1, Mass* m2, float springConstant, float restLength, float dampingConstant)
    : mass1(m1), mass2(m2), springConstant(springConstant), restLength(restLength), dampingConstant(dampingConstant) {}

/**
 * 計算並施加彈簧力
 *
 * 這是彈簧系統的核心函數
 *
 * 根據論文的公式：
 *   f = -k(l - L) - D * (dl/dt)
 * 1. 計算當前彈簧向量和長度
 * 2. 計算彈簧力（胡克定律部分）
 * 3. 計算阻尼力（速度相關部分）
 * 4. 對兩個質點施加相反的力
 */
void Spring::applyForce() {
  // === 步驟 1：獲取兩個質點的位置和速度 ===
  glm::vec3 pos1 = mass1->getPosition();
  glm::vec3 pos2 = mass2->getPosition();
  glm::vec3 vel1 = mass1->getVelocity();
  glm::vec3 vel2 = mass2->getVelocity();

  // === 步驟 2：計算彈簧向量（從質點2指向質點1）===

  glm::vec3 direction = pos1 - pos2;
  float currentLength = glm::length(direction);

 
  if (currentLength < 0.0001f) return;

  // === 步驟 3：計算單位方向向量 ===
  glm::vec3 unitDir = direction / currentLength;

  // === 步驟 4：計算彈簧力（胡克定律）===
  // 公式：F_spring = -k(l - L)
  //
  float springForceMag = -springConstant * (currentLength - restLength);

  // === 步驟 5：計算阻尼力 ===
  //
  // 
  glm::vec3 relativeVelocity = vel1 - vel2;
  float dampingForceMag = -dampingConstant * glm::dot(relativeVelocity, unitDir);

  // === 步驟 6：合併彈簧力和阻尼力 ===
  float totalForceMag = springForceMag + dampingForceMag;
  glm::vec3 force = unitDir * totalForceMag;

  // === 步驟 7：對兩個質點施加相反的力 ===
  //
  // 牛頓第三定律：作用力與反作用力
  //
  // 質點1 受到 force
  // 質點2 受到 -force
  //
  mass1->applyForce(force);
  mass2->applyForce(-force);
}

