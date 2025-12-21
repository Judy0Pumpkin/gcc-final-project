#pragma once
#include <glm/glm.hpp>

class Mass {
 public:
 
  Mass(float mass, const glm::vec3& position);

  
  void applyForce(const glm::vec3& force);
  void resetForce();
  void update(float dt);

  // getter
  glm::vec3 getPosition() const { return position; }
  glm::vec3 getVelocity() const { return velocity; }
  float getMass() const { return mass; }
  glm::vec3 getNewVelocity(float dt) const {
	glm::vec3 acceleration = force / mass;
	return velocity + acceleration * dt;
  }

  // setter
  void setPosition(const glm::vec3& pos) { position = pos; }
  void setVelocity(const glm::vec3& vel) { velocity = vel; }

 private:
  float mass;          // 質量 (m)，單位：kg
  glm::vec3 position;  // 位置 (x)，單位：m
  glm::vec3 velocity;  // 速度 (v)，單位：m/s
  glm::vec3 force;     // 累積的力 (F)，單位：N
};
