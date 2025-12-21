#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#include "Mass.h"
#include "Spring.h"

class Snake {
 public:
  enum class MovementMode {
    SIMPLE,
    LATERAL,     // 側向波動（S形）
    RECTILINEAR  // 直線蠕動（蚯蚓式）
  };

  Snake(int numSegments, float segmentMass, float segmentLength, float springK, float damping,
        const glm::vec3& startPos, float radius);
  ~Snake();
  
  void update(float dt);

  // 控制
  //void setTargetDirection(const glm::vec3& dir);
  //void addForwardForce(float magnitude);
  void startMoving() { isMoving = true; }
  void stopMoving() { isMoving = false; }
  bool getIsMoving() const { return isMoving; }
  void setSnakeMoveDirection(int index, bool value);

  // 模式
  void setMovementMode(MovementMode mode) { movementMode = mode; }
  MovementMode getMovementMode() const { return movementMode; }

  // 獲取器
  
  glm::vec3 getHeadPosition() const;
  glm::vec3 getHeadDirection() const;
  //glm::vec3 getForwardDirection() const;
  const std::vector<Mass*>& getMasses() const { return masses; }
  std::vector<Mass*>& getMasses() { return masses; }
  float getRadius() const { return radius; }
  MovementMode getMode() const { return movementMode; }

  
  // 參數
  void setWaveAmplitude(float amp) { waveAmplitudeRectilinear = amp; }
  void setWaveFrequency(float freq) { waveFrequencyRectilinear = freq; }
  float getWaveAmplitude() const { return waveAmplitudeRectilinear; }
  float getWaveFrequency() const { return waveFrequencyRectilinear; }
  
 private:
  void createMassSpringSystem(const glm::vec3& startPos);

  // 核心物理
  
  //void updateForwardDirection();
  /* 12202303 先將這個隱藏
  //void applyDistanceConstraints();
  //void enforceDistanceConstraints();
  void enforceSoftDistanceConstraints();
  void applySteeringForce();
  void handleGroundCollision(Mass* mass);
  */
  // 運動模式
  //void applyLateralUndulation();
  void applyRectilinearProgression();
  void applyGroundFriction(Mass* mass, float dt);
  void applyDirectionalFriction();

  // 數據
  std::vector<Mass*> masses;
  std::vector<Spring*> axialSprings;

  // 參數
  int numSegments;
  float segmentMass;
  float segmentLength;
  float springK;
  float damping;
  float radius;

  int waveLength = 3;
  
  // 運動狀態
  //glm::vec3 forwardDirection;
  //glm::vec3 targetDirection;
  float waveAmplitudeRectilinear = 3.0f;
  float waveFrequencyRectilinear = 1.2f;
  //float waveSpeed;
  bool isMoving;
  bool snakeMoveDirection[3] = {false, false, false};  // 前、左、右
  float movementTimer = 0.0f;
  float groundFrictionCoeff = 0.2f;


  // 環境
  float groundHeight; 
  MovementMode movementMode;
  
  // 常數
  static constexpr float GRAVITY = 9.8f;
  //static constexpr float GROUND_HEIGHT = 0.15f;
  static constexpr float FRICTION_FORWARD = 0.5f;
  //static constexpr float FRICTION_LATERAL = 3.0f;

  
};