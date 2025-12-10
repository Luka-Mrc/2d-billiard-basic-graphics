#include "./Ball.h"
#include "Header/Util.h"
#include <cmath>
#define M_PI 3.1415

Ball::Ball() : x(0), y(0), vx(0), vy(0), radius(0.03f),
r(1), g(1), b(1), active(true), isWhite(false) {}

Ball::Ball(float x, float y, float radius, float r, float g, float b, bool isWhite)
    : x(x), y(y), vx(0), vy(0), radius(radius),
    r(r), g(g), b(b), active(true), isWhite(isWhite) {}

void Ball::update(float dt) {
    if (!active) return;

    x += vx * dt;
    y += vy * dt;
}

void Ball::applyFriction(float friction) {
    if (!active) return;

    float speed = length(vx, vy);
    if (speed > 0.0001f) {
        float newSpeed = speed - friction;
        if (newSpeed < 0) newSpeed = 0;

        float ratio = newSpeed / speed;
        vx *= ratio;
        vy *= ratio;
    }
    else {
        vx = 0;
        vy = 0;
    }
}

void Ball::draw(unsigned int shaderProgram, unsigned int VAO, int numSegments) {
    if (!active) return;

    glUseProgram(shaderProgram);

    GLint posLoc = glGetUniformLocation(shaderProgram, "uPos");
    GLint radiusLoc = glGetUniformLocation(shaderProgram, "uRadius");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "uColor");

    glUniform2f(posLoc, x, y);
    glUniform1f(radiusLoc, radius);
    glUniform3f(colorLoc, r, g, b);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, numSegments + 2);
}

bool Ball::isStopped() const {
    return length(vx, vy) < 0.0001f;
}

void Ball::stop() {
    vx = 0;
    vy = 0;
}

void Ball::generateCircleVertices(std::vector<float>& vertices, int numSegments) {
    vertices.clear();

    vertices.push_back(0.0f);
    vertices.push_back(0.0f);

    for (int i = 0; i <= numSegments; ++i) {
        float angle = i * 2.0f * M_PI / numSegments;
        vertices.push_back(std::cos(angle));
        vertices.push_back(std::sin(angle));
    }
}