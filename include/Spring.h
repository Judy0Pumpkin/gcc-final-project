#pragma once

#include <glm/glm.hpp>
#include "Mass.h"

class Spring {
 public:
  /**
   * 
   * @param m1 質量1
   * @param m2 質量2
   * @param springConstant 彈簧常數 k（剛度），單位：N/m
   * @param restLength 靜止長度 L，單位：m
   * @param dampingConstant 阻尼係數 D，單位：Ns/m
   */
  Spring(Mass* m1, Mass* m2, float springConstant, float restLength, float dampingConstant);

  /**
   * 計算並施加彈簧力
   * 1. 計算當前彈簧長度
   * 2. 計算彈簧力（胡克定律）
   * 3. 計算阻尼力
   * 4. 對兩個質點施加相反的力
   */
  void applyForce();

  /**
   * 設置靜止長度
   * 這是模擬肌肉收縮的關鍵！
   * 通過動態改變 L，可以讓蛇的身體產生波動
   *
   * @param length 新的靜止長度
   */
  void setRestLength(float length) { restLength = length; }

  /**
   * 獲取靜止長度
   */
  float getRestLength() const { return restLength; }

  // === Getters ===
  Mass* getMass1() const { return mass1; }
  Mass* getMass2() const { return mass2; }

 private:
  Mass* mass1;            // 第一個質點
  Mass* mass2;            // 第二個質點
  float springConstant;   // k - 彈簧常數（剛度）
  float restLength;       // L - 靜止長度
  float dampingConstant;  // D - 阻尼係數
};
