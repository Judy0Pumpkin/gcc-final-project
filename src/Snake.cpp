#include "Snake.h"
#include <algorithm>
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// ========== 構造與析構 ==========

Snake::Snake(int numSegments, float segmentMass, float segmentLength, float springK, float damping,
             const glm::vec3& startPos, float radius)
    : numSegments(numSegments),
      segmentMass(segmentMass),
      segmentLength(segmentLength),
      springK(springK ),  // 中等硬度
      damping(damping),
      /*  // 小阻尼 12302303 先將這個隱藏
      forwardDirection(1.0f, 0.0f, 0.0f),
      targetDirection(1.0f, 0.0f, 0.0f),
      waveSpeed(2.0f),*/
      isMoving(false),
      groundHeight(0.6f) ,
      movementMode(MovementMode::RECTILINEAR),
      radius(radius) {
  std::cout << "Creating snake with " << numSegments << " segments..." << std::endl;
  createMassSpringSystem(startPos);
  std::cout << "Snake created successfully!" << std::endl;
}

Snake::~Snake() {
  for (auto* mass : masses) delete mass;
  for (auto* spring : axialSprings) delete spring;
  masses.clear();
  axialSprings.clear();
}

void Snake::createMassSpringSystem(const glm::vec3& startPos) {
  // 創建質點
  for (int i = 0; i < numSegments; ++i) {
    glm::vec3 pos = startPos + glm::vec3(i * segmentLength, 0.0f, 0.0f);
    pos.y = groundHeight;
    masses.push_back(new Mass(segmentMass, pos));
  }

  // 創建彈簧連接相鄰質點
  for (int i = 0; i < numSegments - 1; ++i) {
    axialSprings.push_back(new Spring(masses[i], masses[i + 1], springK, segmentLength, damping));
  }
}

void Snake::setSnakeMoveDirection(int index, bool value) {
  snakeMoveDirection[index] = value;
  if (snakeMoveDirection[0] || snakeMoveDirection[1] || snakeMoveDirection[2]) {
    startMoving();
  } else {
    stopMoving();
  }
}

// ========== 主更新 ==========

void Snake::update(float dt) {
  
    // 1. 清空上一幀的力
    for (auto* mass : masses) {
      mass->resetForce();
    }

    // 2. 更新前進方向
    //updateForwardDirection();

    // 3. 施加重力
    /*
    for (auto* mass : masses) {
      mass->applyForce(glm::vec3(0, -10.0f * mass->getMass(), 0));
    }*/

    

    // 5. 運動力（根據模式選擇）
    
    if (isMoving) {
      if (movementMode == MovementMode::SIMPLE) {// && snakeMoveDirection[0] == true
        masses[0]->applyForce(getHeadDirection() * FRICTION_FORWARD);  // 頭部施加前進力
      } else if (movementMode == MovementMode::LATERAL) {
        //applyLateralUndulation();  // S形波動
      } else if (movementMode == MovementMode::RECTILINEAR) {
        applyRectilinearProgression();  // 直線蠕動
      }
      movementTimer += dt;
      if (movementTimer > 10.0f / waveFrequencyRectilinear) {  // 我寫10只是為了不要讓1/waveFrequency太小
        movementTimer -= 10.0f / waveFrequencyRectilinear;
      }
    } else {
      for (int i = 0; i < axialSprings.size(); ++i) {
        axialSprings[i]->setRestLength(segmentLength);
      }
    }

    // 4. 彈簧力
    for (auto* spring : axialSprings) {
      spring->applyForce();
    }

    // 6. 轉向力
    //applySteeringForce();

    // 7. 地面摩擦
    applyDirectionalFriction();
    
    for (size_t i = 0; i < masses.size(); ++i) {
      applyGroundFriction(masses[i], dt);
    }
    
    // 8. 更新位置和速度
    for (auto* mass : masses) {
      mass->update(dt);
    }

    // 9. 地面碰撞
    /*
    for (auto* mass : masses) {
      handleGroundCollision(mass);
    }*/

    // 10. 防止過度拉伸
    //enforceSoftDistanceConstraints();
  
}

// ========== 物理約束 ==========
/* 12202303 先將這個隱藏
void Snake::enforceSoftDistanceConstraints() {
  for (size_t i = 0; i < masses.size() - 1; ++i) {
    glm::vec3 pos1 = masses[i]->getPosition();
    glm::vec3 pos2 = masses[i + 1]->getPosition();
    glm::vec3 delta = pos2 - pos1;
    float currentDist = glm::length(delta);

    if (currentDist < 0.001f) continue;

    // 允許±10%的長度變化
    float maxDist = segmentLength * 1.1f;
    float minDist = segmentLength * 0.7f;

    if (currentDist > maxDist || currentDist < minDist) {
      float target = (currentDist > maxDist) ? maxDist : minDist;
      float error = currentDist - target;
      glm::vec3 correction = glm::normalize(delta) * error * 0.3f;

      masses[i]->setPosition(pos1 + correction);
      masses[i + 1]->setPosition(pos2 - correction);
    }
  }
}

void Snake::handleGroundCollision(Mass* mass) {
  glm::vec3 pos = mass->getPosition();

  if (pos.y < groundHeight) {
    pos.y = groundHeight;
    mass->setPosition(pos);

    glm::vec3 vel = mass->getVelocity();
    if (vel.y < 0) vel.y = 0;
    vel *= 0.9f;  // 能量損失
    mass->setVelocity(vel);
  }
}
*/
// ========== 方向控制 ==========
glm::vec3 Snake::getHeadDirection() const {
  if (masses.size() < 2) return glm::vec3(0, 0, 0);

  
    glm::vec3 dir = masses[0]->getPosition() - masses[1]->getPosition();
    dir.y = 0.0f;
    float len = glm::length(dir);
    if (len > 0.001f) {
      dir= dir / len;
    }else{
      dir = glm::vec3(0, 0, 0);
  }
    return dir;
}
/*
void Snake::updateForwardDirection() {
  if (masses.size() < 2) return;

  glm::vec3 avgDir(0.0f);
  int count = std::min(3, (int)masses.size() - 1);

  for (int i = 0; i < count; ++i) {
    glm::vec3 dir = masses[i]->getPosition() - masses[i + 1]->getPosition();
    dir.y = 0.0f;
    float len = glm::length(dir);
    if (len > 0.001f) {
      avgDir += dir / len;
    }
  }

  float avgLen = glm::length(avgDir);
  if (avgLen > 0.001f) {
    glm::vec3 newDir = avgDir / avgLen;
    forwardDirection = glm::normalize(forwardDirection * 0.7f + newDir * 0.3f);
  }
}

void Snake::applySteeringForce() {
  if (!isMoving || masses.size() < 2) return;

  glm::vec3 currentDir = forwardDirection;
  glm::vec3 targetDir = targetDirection;

  // 計算轉向角度
  glm::vec3 cross = glm::cross(currentDir, targetDir);
  float turnAmount = cross.y;

  float dot = glm::dot(currentDir, targetDir);
  dot = glm::clamp(dot, -1.0f, 1.0f);
  float angleDiff = acos(dot);

  if (angleDiff < 0.01f) return;

  // 計算側向方向
  glm::vec3 up(0.0f, 1.0f, 0.0f);
  glm::vec3 rightDir = glm::normalize(glm::cross(currentDir, up));

  // 對前幾個節點施加轉向力
  float steerStrength = springK * 0.8f * angleDiff;
  int numSteer = std::min(3, (int)masses.size());

  for (int i = 0; i < numSteer; ++i) {
    float weight = 1.0f - (float)i / numSteer;
    masses[i]->applyForce(rightDir * turnAmount * steerStrength * weight);
  }

  // 平滑更新方向
  forwardDirection = glm::normalize(forwardDirection + (targetDir - currentDir) * 0.03f);
}
*/
// ========== 運動模式 ==========
/*
void Snake::applyLateralUndulation() {
  if (!isMoving || masses.size() < 3) return;

  //glm::vec3 up(0.0f, 1.0f, 0.0f);
  //glm::vec3 lateralDir = glm::normalize(glm::cross(forwardDirection, up));

  // S形波動
  for (size_t i = 1; i < masses.size(); ++i) {
    float phase = (float)i / waveLength * 2.0f * M_PI;
    float wave = waveAmplitude * sin(waveFrequency * movementTimer * 2.0f * M_PI + phase);
    //float forceMagnitude = wave * 6.0f;

    //masses[i]->applyForce(lateralDir * forceMagnitude);
  }

  //masses[0]->applyForce(forwardDirection * 2.0f);
}*/

void Snake::applyRectilinearProgression() {
  if (!isMoving || masses.size() < 3) return;

  float omega = waveFrequencyRectilinear * 2.0f * M_PI;

  for (int i = 0; i < axialSprings.size(); ++i) {
    float spatialPhase = 2.0f * M_PI * (float)(i % waveLength) / waveLength;
    float temporalPhase = omega * movementTimer;
    float wave = sin(temporalPhase - spatialPhase);

    // 修改彈簧長度產生蠕動
    float lengthMod = 1.0f - waveAmplitudeRectilinear * wave;
    axialSprings[i]->setRestLength(segmentLength * lengthMod);

    /*
    // 收縮時推進
    if (wave > 0.3f && i > 0) {
      glm::vec3 localDir = forwardDirection;
      if (i < masses.size() - 1) {
        localDir = masses[i - 1]->getPosition() - masses[i + 1]->getPosition();
        localDir.y = 0;
        float len = glm::length(localDir);
        if (len > 0.001f)
          localDir /= len;
        else
          localDir = forwardDirection;
      }
      masses[i]->applyForce(localDir * 2.0f * wave);
    }
    */
  }

  //masses[0]->applyForce(forwardDirection * 1.0f);
}

// ========== 摩擦力 ==========

void Snake::applyGroundFriction(Mass* mass, float dt) {  // 如果覺得摩擦力太大可以調小groundFrictionCoeff或質量
    glm::vec3 dir = mass->getNewVelocity(dt);
    dir.y = 0.0f;
    float len = glm::length(dir);
    if (len < 0.001f) return;
    dir = glm::normalize(dir);

    glm::vec3 frictionForce = -dir * groundFrictionCoeff * mass->getMass() * GRAVITY;
    // 根據行進方向施加摩擦力
    mass->applyForce(frictionForce);
}


void Snake::applyDirectionalFriction() {
  if (masses.size() < 2) return;
  for (int i = 0;i<masses.size();++i) {
    Mass* mass = masses[i];
    // 獲取質點的速度
    glm::vec3 velocity = mass->getVelocity();

    // 如果質點的速度接近零，則不施加摩擦
    if (glm::length(velocity) < 0.001f) {
      continue;
    }
    glm::vec3 dir;
    if (i==0){
      dir = masses[0]->getPosition() - masses[1]->getPosition();
    } else if (i==masses.size()-1){
      dir = masses[masses.size()-2]->getPosition() - masses[masses.size()-1]->getPosition();
    } else {
      dir = masses[i-1]->getPosition() - masses[i+1]->getPosition();
    }
    dir.y = 0.0f;
    if (glm::length(dir)<0.001f){
      continue;
    }
    dir = glm::normalize(dir);
    
    //消去向後的分量
    if (glm::dot(velocity, dir)<0.0f){
      velocity -= glm::dot(velocity, dir)*dir;
    }

    mass->setVelocity(velocity);
  }
}

/*
void Snake::applyDirectionalFriction(Mass* mass) {
  

  
  glm::vec3 pos = mass->getPosition();
  if (pos.y > groundHeight + 0.01f) return;

  glm::vec3 velocity = mass->getVelocity();
  glm::vec3 horizontalVel(velocity.x, 0.0f, velocity.z);

  if (movementMode == MovementMode::LATERAL) {
    
    // 非對稱摩擦（模擬蛇鱗）
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::vec3 lateralDir = glm::normalize(glm::cross(forwardDirection, up));

    float vForward = glm::dot(horizontalVel, forwardDirection);
    float vLateral = glm::dot(horizontalVel, lateralDir);

    // 側向摩擦大，前後摩擦小
    glm::vec3 frictionForward = -0.05f * vForward * forwardDirection * mass->getMass() * 9.8f;
    glm::vec3 frictionLateral = -0.6f * vLateral * lateralDir * mass->getMass() * 9.8f;

    mass->applyForce(frictionForward + frictionLateral);
    
  } else {
    // 動態摩擦（蠕動模式）
    float waveNumber = 2.0f * M_PI / (masses.size() * 0.5f);
    float phase = waveNumber * index;
    float contraction = sin(waveFrequency * time - phase);
    float frictionCoeff = 0.2f + 0.7f * std::max(0.0f, contraction);

    if (glm::length(horizontalVel) > 0.001f) {
      glm::vec3 frictionForce = -glm::normalize(horizontalVel) * frictionCoeff * mass->getMass() * 9.8f;
      mass->applyForce(frictionForce);
    }
  }
  
}
*/
/*
// ========== 控制介面 ==========

void Snake::setTargetDirection(const glm::vec3& dir) {
  glm::vec3 newDir = dir;
  newDir.y = 0.0f;
  float len = glm::length(newDir);
  if (len > 0.001f) {
    targetDirection = newDir / len;
  }
}

void Snake::addForwardForce(float magnitude) {
  if (masses.empty()) return;

  int numAffected = std::min(4, (int)masses.size());
  for (int i = 0; i < numAffected; ++i) {
    float weight = 1.0f - (float)i / (numAffected + 1);
    masses[i]->applyForce(forwardDirection * magnitude * weight);
  }
}
*/
glm::vec3 Snake::getHeadPosition() const { return masses.empty() ? glm::vec3(0.0f) : masses[0]->getPosition(); }

//glm::vec3 Snake::getForwardDirection() const { return forwardDirection; }
