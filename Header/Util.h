#ifndef UTIL_H
#define UTIL_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <cmath>

int endProgram(std::string message);
unsigned int compileShader(GLenum type, const char* source);
unsigned int createShader(const char* vsSource, const char* fsSource);
unsigned loadImageToTexture(const char* filePath);
GLFWcursor* loadImageToCursor(const char* filePath);

// Math utility functions
inline float length(float x, float y) {
    return std::sqrt(x * x + y * y);
}

inline float distance(float x1, float y1, float x2, float y2) {
    return length(x2 - x1, y2 - y1);
}

inline float dot(float x1, float y1, float x2, float y2) {
    return x1 * x2 + y1 * y2;
}

inline float clamp(float value, float minVal, float maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

#endif