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
 *
 * 我們需要：
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
  //
  // 圖示：
  //   質點2 ●------------→ 質點1
  //        pos2    direction   pos1
  //
  glm::vec3 direction = pos1 - pos2;
  float currentLength = glm::length(direction);

  // 安全檢查：避免除以零
  if (currentLength < 0.0001f) return;

  // === 步驟 3：計算單位方向向量 ===
  glm::vec3 unitDir = direction / currentLength;

  // === 步驟 4：計算彈簧力（胡克定律）===
  //
  // 公式：F_spring = -k(l - L)
  //
  // 解釋：
  // - 如果 l > L（彈簧被拉長）
  //   → (l - L) > 0
  //   → F_spring < 0（負號）
  //   → 力的方向與 unitDir 相反，把質點1拉回去
  //
  // - 如果 l < L（彈簧被壓縮）
  //   → (l - L) < 0
  //   → F_spring > 0（負負得正）
  //   → 力的方向與 unitDir 相同，把質點1推出去
  //
  float springForceMag = -springConstant * (currentLength - restLength);

  // === 步驟 5：計算阻尼力 ===
  //
  // 公式：F_damping = -D * (v_rel · direction)
  //
  // 其中 v_rel 是相對速度
  // 我們只關心沿著彈簧方向的速度分量
  //
  // 解釋：
  // - 如果兩個質點正在遠離（彈簧正在拉長）
  //   → dot(v_rel, unitDir) > 0
  //   → F_damping < 0（阻止拉長）
  //
  // - 如果兩個質點正在接近（彈簧正在壓縮）
  //   → dot(v_rel, unitDir) < 0
  //   → F_damping > 0（阻止壓縮）
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

