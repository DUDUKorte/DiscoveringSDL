#pragma once

#define GLM_ENABLE_EXPERIMENTAL

// útli pra printar o vec3 com log e tal
#define VEC3TOSTR(v) ("( " + std::to_string((v).x) + ", " + std::to_string((v).y) + ", " + std::to_string((v).z) + " )")

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Axis {
    glm::vec3 x;
    glm::vec3 y;
    glm::vec3 z;
};
