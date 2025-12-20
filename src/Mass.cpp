#include "Mass.h"


Mass::Mass(float mass, const glm::vec3& position)
    : mass(mass),
      position(position),
      velocity(0.0f, 0.0f, 0.0f),  
      force(0.0f, 0.0f, 0.0f) {  
}

//施加力,力是累加的
void Mass::applyForce(const glm::vec3& f) {
  force += f; 
}

//重置
void Mass::resetForce() { force = glm::vec3(0.0f, 0.0f, 0.0f); }


void Mass::update(float dt) {
  
  // F = ma 
  glm::vec3 acceleration = force / mass;

  // v(t+dt) = v(t) + a*dt
  velocity += acceleration * dt;

  // x(t+dt) = x(t) + v*dt
  position += velocity * dt;

}