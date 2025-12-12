#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <map>

#include <ft2build.h>
#include FT_FREETYPE_H

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

// FreeType text rendering structures
struct Character {
    unsigned int TextureID;
    int SizeX, SizeY;
    int BearingX, BearingY;
    unsigned int Advance;
};

std::map<char, Character> Characters;
unsigned int textVAO, textVBO;
unsigned int textShader;

// Convert mouse coordinates to OpenGL world coordinates (with aspect ratio correction)
void screenToWorld(double screenX, double screenY, float& worldX, float& worldY) {
    float aspectRatio = (float)currentScreenWidth / (float)currentScreenHeight;
    float normalizedX = (screenX / currentScreenWidth) * 2.0f - 1.0f;
    float normalizedY = -((screenY / currentScreenHeight) * 2.0f - 1.0f);
    worldX = normalizedX * aspectRatio;
    worldY = normalizedY;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
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
    balls.push_back(Ball(-0.4f, 0.0f, ballRadius, 1.0f, 1.0f, 1.0f, true));
    float startX = 0.3f;
    float startY = 0.0f;
    float spacing = ballRadius * 2.2f;
    balls.push_back(Ball(startX, startY, ballRadius, 1.0f, 0.0f, 0.0f));
    balls.push_back(Ball(startX + spacing, startY + spacing * 0.866f, ballRadius, 1.0f, 1.0f, 0.0f));
    balls.push_back(Ball(startX + spacing, startY - spacing * 0.866f, ballRadius, 0.0f, 0.0f, 1.0f));
    balls.push_back(Ball(startX + spacing * 2, startY, ballRadius, 1.0f, 0.5f, 0.0f));
    balls.push_back(Ball(startX + spacing * 2, startY + spacing * 1.732f, ballRadius, 0.5f, 0.0f, 0.5f));
    balls.push_back(Ball(startX + spacing * 2, startY - spacing * 1.732f, ballRadius, 0.0f, 1.0f, 1.0f));
    whiteBall = &balls[0];
}

bool initFreeType() {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) return false;
    FT_Face face;
    if (FT_New_Face(ft, "C:/Windows/Fonts/arial.ttf", 0, &face)) return false;
    FT_Set_Pixel_Sizes(face, 0, 48);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        Character character = { texture, (int)face->glyph->bitmap.width, (int)face->glyph->bitmap.rows, face->glyph->bitmap_left, face->glyph->bitmap_top, (unsigned int)face->glyph->advance.x };
        Characters.insert(std::pair<char, Character>(c, character));
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return true;
}

unsigned int createTextShader() {
    const char* vs = "#version 330 core\nlayout (location = 0) in vec4 vertex;\nout vec2 TexCoords;\nuniform mat4 projection;\nvoid main() {\ngl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\nTexCoords = vertex.zw;\n}";
    const char* fs = "#version 330 core\nin vec2 TexCoords;\nout vec4 color;\nuniform sampler2D text;\nuniform vec3 textColor;\nvoid main() {\nvec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);\ncolor = vec4(textColor, 1.0) * sampled;\n}";
    unsigned int v = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v, 1, &vs, NULL);
    glCompileShader(v);
    unsigned int f = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f, 1, &fs, NULL);
    glCompileShader(f);
    unsigned int s = glCreateProgram();
    glAttachShader(s, v);
    glAttachShader(s, f);
    glLinkProgram(s);
    glDeleteShader(v);
    glDeleteShader(f);
    return s;
}

void renderText(const std::string& text, float x, float y, float scale, float r, float g, float b) {
    glUseProgram(textShader);
    glUniform3f(glGetUniformLocation(textShader, "textColor"), r, g, b);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);
    for (char c : text) {
        Character ch = Characters[c];
        float xpos = x + ch.BearingX * scale;
        float ypos = y - (ch.SizeY - ch.BearingY) * scale;
        float w = ch.SizeX * scale;
        float h = ch.SizeY * scale;
        float vertices[6][4] = { {xpos, ypos + h, 0.0f, 0.0f}, {xpos, ypos, 0.0f, 1.0f}, {xpos + w, ypos, 1.0f, 1.0f}, {xpos, ypos + h, 0.0f, 0.0f}, {xpos + w, ypos, 1.0f, 1.0f}, {xpos + w, ypos + h, 1.0f, 0.0f} };
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += (ch.Advance >> 6) * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
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
    std::vector<float> bgVertices = { barX - 0.01f, barY - 0.01f, barX - 0.01f, barY + barHeight + 0.01f, barX + barWidth + 0.01f, barY + barHeight + 0.01f, barX + barWidth + 0.01f, barY - 0.01f };
    std::vector<float> barVertices = { barX, barY, barX, barY + barHeight, barX + barWidth * powerPercent, barY + barHeight, barX + barWidth * powerPercent, barY };
    unsigned int barVAO, barVBO;
    glGenVertexArrays(1, &barVAO);
    glGenBuffers(1, &barVBO);
    glBindVertexArray(barVAO);
    glBindBuffer(GL_ARRAY_BUFFER, barVBO);
    glBufferData(GL_ARRAY_BUFFER, bgVertices.size() * sizeof(float), bgVertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glUseProgram(shader);
    glUniform1f(glGetUniformLocation(shader, "uRadius"), 1.0f);
    glUniform2f(glGetUniformLocation(shader, "uPos"), 0.0f, 0.0f);
    glUniform3f(glGetUniformLocation(shader, "uColor"), 0.3f, 0.3f, 0.3f);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBufferData(GL_ARRAY_BUFFER, barVertices.size() * sizeof(float), barVertices.data(), GL_DYNAMIC_DRAW);
    float r = powerPercent < 0.5f ? powerPercent * 2.0f : 1.0f;
    float g = powerPercent < 0.5f ? 1.0f : 1.0f - (powerPercent - 0.5f) * 2.0f;
    glUniform3f(glGetUniformLocation(shader, "uColor"), r, g, 0.0f);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDeleteBuffers(1, &barVBO);
    glDeleteVertexArrays(1, &barVAO);
}

bool checkGameOver(const std::vector<Ball>& balls) {
    for (const auto& ball : balls) {
        if (!ball.isWhite && ball.active) return false;
    }
    return true;
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

    GLFWcursor* cursor = loadImageToCursor("./cursor.png");
    if (cursor) glfwSetCursor(window, cursor);


    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, mousePosCallback);
    glfwGetWindowSize(window, &currentScreenWidth, &currentScreenHeight);
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (!initFreeType()) {
        std::cerr << "Failed to initialize FreeType" << std::endl;
        return -1;
    }
    unsigned int shader = createShader("shaders/shader.vert", "shaders/shader.frag");
    unsigned int lineShader = createShader("shaders/line.vert", "shaders/line.frag");
    textShader = createTextShader();
    float aspectRatio = (float)currentScreenWidth / (float)currentScreenHeight;
    float left = -aspectRatio;
    float right = aspectRatio;
    float bottom = -1.0f;
    float top = 1.0f;
    float orthoMatrix[16] = { 2.0f / (right - left), 0.0f, 0.0f, 0.0f, 0.0f, 2.0f / (top - bottom), 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, -(right + left) / (right - left), -(top + bottom) / (top - bottom), 0.0f, 1.0f };
    glUseProgram(shader);
    glUniformMatrix4fv(glGetUniformLocation(shader, "uProjection"), 1, GL_FALSE, orthoMatrix);
    glUseProgram(lineShader);
    glUniformMatrix4fv(glGetUniformLocation(lineShader, "uProjection"), 1, GL_FALSE, orthoMatrix);
    float textProjection[16] = { 2.0f / currentScreenWidth, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f / currentScreenHeight, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f };
    glUseProgram(textShader);
    glUniformMatrix4fv(glGetUniformLocation(textShader, "projection"), 1, GL_FALSE, textProjection);
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
    bool gameOver = false;
    while (!glfwWindowShouldClose(window)) {

        const double targetFrameTime = 1.0 / 75.0;
        double frameStart = glfwGetTime();

        float currentTime = glfwGetTime();
        float dt = currentTime - lastTime;
        lastTime = currentTime;
        if (dt > 0.1f) dt = 0.1f;
        glfwGetWindowSize(window, &currentScreenWidth, &currentScreenHeight);
        aspectRatio = (float)currentScreenWidth / (float)currentScreenHeight;
        float textProj[16] = { 2.0f / currentScreenWidth, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f / currentScreenHeight, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f };
        glUseProgram(textShader);
        glUniformMatrix4fv(glGetUniformLocation(textShader, "projection"), 1, GL_FALSE, textProj);
        glClear(GL_COLOR_BUFFER_BIT);
        Physics::updatePhysics(balls, table, dt);
        if (!gameOver) {
            gameOver = checkGameOver(balls);
        }
        if (whiteBall && !whiteBall->active && whiteBall->isWhite) {
            whiteBall->x = whiteBallStartX;
            whiteBall->y = whiteBallStartY;
            whiteBall->vx = 0;
            whiteBall->vy = 0;
            whiteBall->active = true;
        }
        glUseProgram(shader);
        glUniform1f(glGetUniformLocation(shader, "uRadius"), 1.0f);
        glUniform2f(glGetUniformLocation(shader, "uPos"), 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(shader, "uColor"), 0.1f, 0.6f, 0.2f);
        glUniform1f(glGetUniformLocation(shader, "uAlpha"), 1.0f);
        glBindVertexArray(tableVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glUniform3f(glGetUniformLocation(shader, "uColor"), 0.4f, 0.2f, 0.1f);
        glBindVertexArray(wallVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glDrawArrays(GL_TRIANGLE_FAN, 4, 4);
        glDrawArrays(GL_TRIANGLE_FAN, 8, 4);
        glDrawArrays(GL_TRIANGLE_FAN, 12, 4);
        table.draw(shader, tableVAO, circleVAO, NUM_CIRCLE_SEGMENTS);
        for (auto& ball : balls) {
            ball.draw(shader, circleVAO, NUM_CIRCLE_SEGMENTS);
        }

        double frameEnd = glfwGetTime();
        double frameTime = frameEnd - frameStart;
        if (frameTime < targetFrameTime) {
            glfwWaitEventsTimeout(targetFrameTime - frameTime);
        }


        float textX = 60.0f;
        float textY = currentScreenHeight - 60.0f;
        renderText("Luka Marić RA154/2022", textX, textY, 1.0f, 1.0f, 1.0f, 1.0f);
        if (gameOver) {
            float centerX = currentScreenWidth / 2.0f - 150.0f;
            float centerY = currentScreenHeight / 2.0f;
            renderText("GAME OVER", centerX, centerY, 2.0f, 1.0f, 0.2f, 0.2f);
        }
        if (whiteBall && whiteBall->active && whiteBall->isStopped() && !gameOver) {
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
    glDeleteVertexArrays(1, &circleVAO);
    glDeleteBuffers(1, &circleVBO);
    glDeleteVertexArrays(1, &tableVAO);
    glDeleteBuffers(1, &tableVBO);
    glDeleteVertexArrays(1, &wallVAO);
    glDeleteBuffers(1, &wallVBO);
    glDeleteVertexArrays(1, &lineVAO);
    glDeleteVertexArrays(1, &textVAO);
    glDeleteBuffers(1, &textVBO);
    glDeleteProgram(shader);
    glDeleteProgram(lineShader);
    glDeleteProgram(textShader);
    for (auto& pair : Characters) {
        glDeleteTextures(1, &pair.second.TextureID);
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}