#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

#include "../Ball.h"
#include "../Table.h"
#include "../Physics.h"
#include "../Header/Util.h"

const int SCREEN_WIDTH = 1600;
const int SCREEN_HEIGHT = 900;
const int NUM_CIRCLE_SEGMENTS = 40;

// Shot power settings
const float CHARGE_DURATION = 2.0f;
const float MIN_POWER = 0.6f;
const float MAX_POWER = 7.2f;

// Global input variables
double mouseX = 0.0, mouseY = 0.0;
Ball* whiteBall = nullptr;
int currentScreenWidth = SCREEN_WIDTH;
int currentScreenHeight = SCREEN_HEIGHT;

// Global charging system variables
bool isCharging = false;
double chargeStartTime = 0.0;

// White ball respawn position
float whiteBallStartX = -0.4f;
float whiteBallStartY = 0.0f;

// Convert mouse coordinates to OpenGL world coordinates (with aspect ratio correction)
void screenToWorld(double screenX, double screenY, float& worldX, float& worldY) {
    float aspectRatio = (float)currentScreenWidth / (float)currentScreenHeight;

    // Normalize to -1 to 1 range
    float normalizedX = (screenX / currentScreenWidth) * 2.0f - 1.0f;
    float normalizedY = -((screenY / currentScreenHeight) * 2.0f - 1.0f);

    // Apply aspect ratio correction (same as projection matrix)
    worldX = normalizedX * aspectRatio;
    worldY = normalizedY;
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && whiteBall) {
        if (whiteBall->isStopped() && whiteBall->active) {
            if (action == GLFW_PRESS) {
                isCharging = true;
                chargeStartTime = glfwGetTime();
            }
            else if (action == GLFW_RELEASE && isCharging) {
                isCharging = false;
                float chargeTime = static_cast<float>(glfwGetTime() - chargeStartTime);
                chargeTime = clamp(chargeTime, 0.0f, CHARGE_DURATION);

                float power = MIN_POWER + (MAX_POWER - MIN_POWER) * (chargeTime / CHARGE_DURATION);

                float worldX, worldY;
                screenToWorld(mouseX, mouseY, worldX, worldY);

                float dx = worldX - whiteBall->x;
                float dy = worldY - whiteBall->y;
                float dist = length(dx, dy);

                if (dist > 0.01f) {
                    dx /= dist;
                    dy /= dist;

                    whiteBall->vx = dx * power;
                    whiteBall->vy = dy * power;
                }
            }
        }
    }
}

void mousePosCallback(GLFWwindow* window, double xpos, double ypos) {
    mouseX = xpos;
    mouseY = ypos;
}

void setupBalls(std::vector<Ball>& balls) {
    balls.clear();

    float ballRadius = 0.025f;

    // White ball (cue ball)
    balls.push_back(Ball(-0.4f, 0.0f, ballRadius, 1.0f, 1.0f, 1.0f, true));

    // Set up colored balls in triangle formation
    float startX = 0.3f;
    float startY = 0.0f;
    float spacing = ballRadius * 2.2f;

    // Red
    balls.push_back(Ball(startX, startY, ballRadius, 1.0f, 0.0f, 0.0f));

    // Row 2
    balls.push_back(Ball(startX + spacing, startY + spacing * 0.866f, ballRadius, 1.0f, 1.0f, 0.0f));
    balls.push_back(Ball(startX + spacing, startY - spacing * 0.866f, ballRadius, 0.0f, 0.0f, 1.0f));

    // Row 3
    balls.push_back(Ball(startX + spacing * 2, startY, ballRadius, 1.0f, 0.5f, 0.0f));
    balls.push_back(Ball(startX + spacing * 2, startY + spacing * 1.732f, ballRadius, 0.5f, 0.0f, 0.5f));
    balls.push_back(Ball(startX + spacing * 2, startY - spacing * 1.732f, ballRadius, 0.0f, 1.0f, 1.0f));

    // Update white ball pointer
    whiteBall = &balls[0];
}

void drawAimLine(unsigned int lineShader, unsigned int lineVAO, const Ball& ball, float mouseWorldX, float mouseWorldY) {
    if (!ball.active || !ball.isStopped()) return;

    float dx = mouseWorldX - ball.x;
    float dy = mouseWorldY - ball.y;
    float dist = length(dx, dy);

    if (dist < 0.01f) return;

    dx /= dist;
    dy /= dist;

    std::vector<float> lineVertices;
    float maxLength = dist;
    float dashLength = 0.03f;
    float gapLength = 0.02f;

    float currentLength = 0.0f;
    bool isDash = true;

    while (currentLength < maxLength) {
        float segmentLength = isDash ? dashLength : gapLength;
        if (currentLength + segmentLength > maxLength) {
            segmentLength = maxLength - currentLength;
        }

        if (isDash && segmentLength > 0) {
            float x1 = ball.x + dx * currentLength;
            float y1 = ball.y + dy * currentLength;
            float x2 = ball.x + dx * (currentLength + segmentLength);
            float y2 = ball.y + dy * (currentLength + segmentLength);

            lineVertices.push_back(x1);
            lineVertices.push_back(y1);
            lineVertices.push_back(x2);
            lineVertices.push_back(y2);
        }

        currentLength += segmentLength;
        isDash = !isDash;
    }

    if (lineVertices.empty()) return;

    unsigned int lineVBO;
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(float), lineVertices.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glUseProgram(lineShader);
    glUniform3f(glGetUniformLocation(lineShader, "uColor"), 1.0f, 1.0f, 1.0f);
    glUniform1f(glGetUniformLocation(lineShader, "uAlpha"), 0.7f);

    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, lineVertices.size() / 2);

    glDeleteBuffers(1, &lineVBO);
}

void drawPowerBar(unsigned int shader, float powerPercent) {
    float barWidth = 0.3f;
    float barHeight = 0.05f;
    float barX = -barWidth / 2.0f;
    float barY = -0.9f;

    // Background bar (gray)
    std::vector<float> bgVertices = {
        barX - 0.01f, barY - 0.01f,
        barX - 0.01f, barY + barHeight + 0.01f,
        barX + barWidth + 0.01f, barY + barHeight + 0.01f,
        barX + barWidth + 0.01f, barY - 0.01f
    };

    // Power bar (color depends on power level)
    std::vector<float> barVertices = {
        barX, barY,
        barX, barY + barHeight,
        barX + barWidth * powerPercent, barY + barHeight,
        barX + barWidth * powerPercent, barY
    };

    unsigned int barVAO, barVBO;
    glGenVertexArrays(1, &barVAO);
    glGenBuffers(1, &barVBO);

    glBindVertexArray(barVAO);
    glBindBuffer(GL_ARRAY_BUFFER, barVBO);

    // Draw background
    glBufferData(GL_ARRAY_BUFFER, bgVertices.size() * sizeof(float), bgVertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glUseProgram(shader);
    glUniform1f(glGetUniformLocation(shader, "uRadius"), 1.0f);
    glUniform2f(glGetUniformLocation(shader, "uPos"), 0.0f, 0.0f);
    glUniform3f(glGetUniformLocation(shader, "uColor"), 0.3f, 0.3f, 0.3f);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // Draw power bar
    glBufferData(GL_ARRAY_BUFFER, barVertices.size() * sizeof(float), barVertices.data(), GL_DYNAMIC_DRAW);

    // Color: green -> yellow -> red
    float r = powerPercent < 0.5f ? powerPercent * 2.0f : 1.0f;
    float g = powerPercent < 0.5f ? 1.0f : 1.0f - (powerPercent - 0.5f) * 2.0f;
    glUniform3f(glGetUniformLocation(shader, "uColor"), r, g, 0.0f);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDeleteBuffers(1, &barVBO);
    glDeleteVertexArrays(1, &barVAO);
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "2D Billiard Game", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, mousePosCallback);

    glfwGetWindowSize(window, &currentScreenWidth, &currentScreenHeight);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int shader = createShader("shaders/shader.vert", "shaders/shader.frag");
    unsigned int lineShader = createShader("shaders/line.vert", "shaders/line.frag");

    float aspectRatio = (float)currentScreenWidth / (float)currentScreenHeight;
    float left = -aspectRatio;
    float right = aspectRatio;
    float bottom = -1.0f;
    float top = 1.0f;

    float orthoMatrix[16] = {
        2.0f / (right - left), 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f / (top - bottom), 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -(right + left) / (right - left), -(top + bottom) / (top - bottom), 0.0f, 1.0f
    };

    glUseProgram(shader);
    glUniformMatrix4fv(glGetUniformLocation(shader, "uProjection"), 1, GL_FALSE, orthoMatrix);

    glUseProgram(lineShader);
    glUniformMatrix4fv(glGetUniformLocation(lineShader, "uProjection"), 1, GL_FALSE, orthoMatrix);

    std::vector<float> circleVertices;
    Ball::generateCircleVertices(circleVertices, NUM_CIRCLE_SEGMENTS);

    unsigned int circleVAO, circleVBO;
    glGenVertexArrays(1, &circleVAO);
    glGenBuffers(1, &circleVBO);

    glBindVertexArray(circleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
    glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float), circleVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    Table table(-1.5f, 1.5f, 0.8f, -0.8f);
    std::vector<float> tableVertices;
    Table::generateTableVertices(tableVertices, table.left, table.right, table.top, table.bottom);

    unsigned int tableVAO, tableVBO;
    glGenVertexArrays(1, &tableVAO);
    glGenBuffers(1, &tableVBO);

    glBindVertexArray(tableVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tableVBO);
    glBufferData(GL_ARRAY_BUFFER, tableVertices.size() * sizeof(float), tableVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    std::vector<float> wallVertices;
    float cushion = table.cushionThickness;
    wallVertices.insert(wallVertices.end(), { table.left, table.top, table.left, table.top - cushion, table.right, table.top - cushion, table.right, table.top });
    wallVertices.insert(wallVertices.end(), { table.left, table.bottom, table.left, table.bottom + cushion, table.right, table.bottom + cushion, table.right, table.bottom });
    wallVertices.insert(wallVertices.end(), { table.left, table.top, table.left + cushion, table.top, table.left + cushion, table.bottom, table.left, table.bottom });
    wallVertices.insert(wallVertices.end(), { table.right, table.top, table.right - cushion, table.top, table.right - cushion, table.bottom, table.right, table.bottom });

    unsigned int wallVAO, wallVBO;
    glGenVertexArrays(1, &wallVAO);
    glGenBuffers(1, &wallVBO);

    glBindVertexArray(wallVAO);
    glBindBuffer(GL_ARRAY_BUFFER, wallVBO);
    glBufferData(GL_ARRAY_BUFFER, wallVertices.size() * sizeof(float), wallVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    unsigned int lineVAO;
    glGenVertexArrays(1, &lineVAO);

    std::vector<Ball> balls;
    setupBalls(balls);

    glClearColor(0.15f, 0.15f, 0.2f, 1.0f);

    float lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float dt = currentTime - lastTime;
        lastTime = currentTime;

        if (dt > 0.1f) dt = 0.1f;

        // Update window size for coordinate conversion
        glfwGetWindowSize(window, &currentScreenWidth, &currentScreenHeight);

        glClear(GL_COLOR_BUFFER_BIT);

        Physics::updatePhysics(balls, table, dt);

        // Respawn white ball if it fell into a pocket
        if (whiteBall && !whiteBall->active && whiteBall->isWhite) {
            whiteBall->x = whiteBallStartX;
            whiteBall->y = whiteBallStartY;
            whiteBall->vx = 0;
            whiteBall->vy = 0;
            whiteBall->active = true;
        }

        // Draw table surface
        glUseProgram(shader);
        glUniform1f(glGetUniformLocation(shader, "uRadius"), 1.0f);
        glUniform2f(glGetUniformLocation(shader, "uPos"), 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(shader, "uColor"), 0.1f, 0.6f, 0.2f);
        glBindVertexArray(tableVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // Draw walls/cushions
        glUniform3f(glGetUniformLocation(shader, "uColor"), 0.4f, 0.2f, 0.1f);
        glBindVertexArray(wallVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glDrawArrays(GL_TRIANGLE_FAN, 4, 4);
        glDrawArrays(GL_TRIANGLE_FAN, 8, 4);
        glDrawArrays(GL_TRIANGLE_FAN, 12, 4);

        // Draw pockets
        table.draw(shader, tableVAO, circleVAO, NUM_CIRCLE_SEGMENTS);

        // Draw balls
        for (auto& ball : balls) {
            ball.draw(shader, circleVAO, NUM_CIRCLE_SEGMENTS);
        }

        // Draw aim line and power bar
        if (whiteBall && whiteBall->active && whiteBall->isStopped()) {
            float worldX, worldY;
            screenToWorld(mouseX, mouseY, worldX, worldY);
            drawAimLine(lineShader, lineVAO, *whiteBall, worldX, worldY);

            if (isCharging) {
                float chargeTime = static_cast<float>(glfwGetTime() - chargeStartTime);
                chargeTime = clamp(chargeTime, 0.0f, CHARGE_DURATION);
                float powerPercent = chargeTime / CHARGE_DURATION;
                drawPowerBar(shader, powerPercent);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &circleVAO);
    glDeleteBuffers(1, &circleVBO);
    glDeleteVertexArrays(1, &tableVAO);
    glDeleteBuffers(1, &tableVBO);
    glDeleteVertexArrays(1, &wallVAO);
    glDeleteBuffers(1, &wallVBO);
    glDeleteVertexArrays(1, &lineVAO);
    glDeleteProgram(shader);
    glDeleteProgram(lineShader);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}