#include "Table.h"
#include "Header/Util.h"
#include <cmath>

Table::Table() : left(-0.8f), right(0.8f), top(0.5f), bottom(-0.5f), cushionThickness(0.03f) {
    setupPockets();
}

Table::Table(float left, float right, float top, float bottom)
    : left(left), right(right), top(top), bottom(bottom), cushionThickness(0.03f) {
    setupPockets();
}

void Table::setupPockets() {
    pockets.clear();

    // Larger pockets for easier gameplay
    float pocketRadius = 0.07f;

    float cornerOffset = 0.02f;

    // Corner pockets
    pockets.push_back(Pocket(left + cornerOffset, top - cornerOffset, pocketRadius));
    pockets.push_back(Pocket(right - cornerOffset, top - cornerOffset, pocketRadius));
    pockets.push_back(Pocket(left + cornerOffset, bottom + cornerOffset, pocketRadius));
    pockets.push_back(Pocket(right - cornerOffset, bottom + cornerOffset, pocketRadius));

    // Middle pockets (on long sides)
    float midX = (left + right) / 2.0f;
    pockets.push_back(Pocket(midX, top, pocketRadius * 0.9f));
    pockets.push_back(Pocket(midX, bottom, pocketRadius * 0.9f));
}

void Table::draw(unsigned int shaderProgram, unsigned int tableVAO, unsigned int pocketVAO, int numPocketSegments) {
    glUseProgram(shaderProgram);

    GLint colorLoc = glGetUniformLocation(shaderProgram, "uColor");
    GLint posLoc = glGetUniformLocation(shaderProgram, "uPos");
    GLint radiusLoc = glGetUniformLocation(shaderProgram, "uRadius");

    // Black pockets
    glUniform3f(colorLoc, 0.0f, 0.0f, 0.0f);

    glBindVertexArray(pocketVAO);
    for (const auto& pocket : pockets) {
        glUniform2f(posLoc, pocket.x, pocket.y);
        glUniform1f(radiusLoc, pocket.radius);
        glDrawArrays(GL_TRIANGLE_FAN, 0, numPocketSegments + 2);
    }
}

bool Table::isInPocket(float x, float y, float ballRadius) const {
    for (const auto& pocket : pockets) {
        float dist = distance(x, y, pocket.x, pocket.y);
        if (dist < pocket.radius * 0.7f) {
            return true;
        }
    }
    return false;
}

void Table::generateTableVertices(std::vector<float>& vertices, float left, float right, float top, float bottom) {
    vertices.clear();
    vertices.push_back(left);
    vertices.push_back(top);
    vertices.push_back(left);
    vertices.push_back(bottom);
    vertices.push_back(right);
    vertices.push_back(bottom);
    vertices.push_back(right);
    vertices.push_back(top);
}