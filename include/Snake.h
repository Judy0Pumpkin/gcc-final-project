#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#include "Mass.h"
#include "Spring.h"

class Snake {
 public:
  enum class MovementMode {
    LATERAL,     // 側向波動（S形）
    RECTILINEAR  // 直線蠕動（蚯蚓式）
  };

  Snake(int numSegments, float segmentMass, float segmentLength, float springK, float damping,
        const glm::vec3& startPos);
  ~Snake();
  /* 12202303 先將這個隱藏
  void update(float dt, float time);

  
  // 控制
  void setTargetDirection(const glm::vec3& dir);
  void addForwardForce(float magnitude);
  void startMoving() { isMoving = true; }
  void stopMoving() { isMoving = false; }
  bool getIsMoving() const { return isMoving; }

  // 模式
  void setMovementMode(MovementMode mode) { movementMode = mode; }
  MovementMode getMovementMode() const { return movementMode; }

  // 獲取器
  glm::vec3 getHeadPosition() const;
  glm::vec3 getForwardDirection() const;*/
  const std::vector<Mass*>& getMasses() const { return masses; }
  std::vector<Mass*>& getMasses() { return masses; }

  /* 12202303 先將這個隱藏
  // 參數
  void setWaveAmplitude(float amp) { waveAmplitude = amp; }
  void setWaveFrequency(float freq) { waveFrequency = freq; }
  float getWaveAmplitude() const { return waveAmplitude; }
  float getWaveFrequency() const { return waveFrequency; }
  */
 private:
  void createMassSpringSystem(const glm::vec3& startPos);

  // 核心物理
  /* 12202303 先將這個隱藏
  void updateForwardDirection();
  //void applyDistanceConstraints();
  //void enforceDistanceConstraints();
  void enforceSoftDistanceConstraints();
  void applySteeringForce();
  void handleGroundCollision(Mass* mass);
  
  // 運動模式
  void applyLateralUndulation(float time);
  void applyRectilinearProgression(float time);
  void applyGroundFriction(Mass* mass, size_t index, float time);
  */

  // 數據
  std::vector<Mass*> masses;
  std::vector<Spring*> axialSprings;

  // 參數
  int numSegments;
  float segmentMass;
  float segmentLength;
  float springK;
  float damping;

  /* 12202303 先將這個隱藏
  // 運動狀態
  glm::vec3 forwardDirection;
  glm::vec3 targetDirection;
  float waveAmplitude;
  float waveFrequency;
  float waveSpeed;
  bool isMoving;

  // 環境*/
  float groundHeight; /* 12202303 先將這個隱藏
  MovementMode movementMode;

  // 常數
  static constexpr float GRAVITY = -9.8f;
  static constexpr float GROUND_HEIGHT = 0.15f;
  static constexpr float FRICTION_FORWARD = 0.2f;
  static constexpr float FRICTION_LATERAL = 3.0f;
  */
};