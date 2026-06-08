#include <GL/glut.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define PI 3.14159265358979323846f

float transition = 0.0f;
float flagWave = 0.0f;
float personX = -1.15f;
float stepAnim = 0.0f;

float cloudOffset1 = 0.0f;
float cloudOffset2 = 0.0f;

float leftPeopleOffset = 0.0f;
float rightPeopleOffset = 0.0f;
float leftWalkAnim = 0.0f;
float rightWalkAnim = 1.2f;
int leftCrowdStopped = 0;
int rightCrowdStopped = 0;

float rippleAnim = 0.0f;
float flowerColorShift = 0.0f;
float textFade = 0.0f;

float centerBurstTimer = 0.0f;
int centerTextStarted = 0;

float pondCenterX = 0.0f;
float pondCenterY = -0.34f;
float pondRadiusX = 0.24f;
float pondRadiusY = 0.11f;

typedef struct {
    float x, y, speed, scale, flap;
} Bird;

Bird birds[6] = {
    {-0.20f, 0.70f, 0.0020f, 1.00f, 0.0f},
    { 0.05f, 0.78f, 0.0017f, 1.10f, 0.6f},
    { 0.28f, 0.68f, 0.0018f, 0.95f, 1.2f},
    { 0.48f, 0.74f, 0.0015f, 1.00f, 2.0f},
    { 0.68f, 0.66f, 0.0014f, 0.85f, 0.3f},
    {-0.45f, 0.64f, 0.0021f, 0.90f, 1.5f}
};

typedef struct {
    float x, y;
    float dx, dy;
    float life;
    float r, g, b;
    int active;
} Particle;

typedef struct {
    float x, y;
    float targetY;
    float speed;
    float r, g, b;
    int active;
} Rocket;

#define FIREWORK_COUNT 8
#define PARTICLES_PER_FIREWORK 120
#define ROCKET_COUNT 2

Particle fireworks[FIREWORK_COUNT][PARTICLES_PER_FIREWORK];
Rocket rockets[ROCKET_COUNT];
float rocketLaunchTimer = 0.0f;

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

float randomFloat(float min, float max) {
    return min + (max - min) * (rand() / (float)RAND_MAX);
}

void rect(float x1, float y1, float x2, float y2) {
    glBegin(GL_QUADS);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();
}

void filledCircle(float cx, float cy, float r, int n) {
    glBegin(GL_POLYGON);
    for (int i = 0; i < n; i++) {
        float a = 2.0f * PI * i / n;
        glVertex2f(cx + r * cosf(a), cy + r * sinf(a));
    }
    glEnd();
}

void plotPixel(float x, float y) {
    glVertex2f(x, y);
}

void lineDDA(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float steps = fabsf(dx) > fabsf(dy) ? fabsf(dx) : fabsf(dy);

    if (steps == 0) {
        glBegin(GL_POINTS);
        plotPixel(x1, y1);
        glEnd();
        return;
    }

    float xInc = dx / steps;
    float yInc = dy / steps;
    float x = x1;
    float y = y1;

    glBegin(GL_POINTS);
    for (int i = 0; i <= (int)steps; i++) {
        plotPixel(x, y);
        x += xInc;
        y += yInc;
    }
    glEnd();
}

void lineBresenham(float x1, float y1, float x2, float y2) {
    int scale = 1000;

    int ix1 = (int)(x1 * scale);
    int iy1 = (int)(y1 * scale);
    int ix2 = (int)(x2 * scale);
    int iy2 = (int)(y2 * scale);

    int dx = abs(ix2 - ix1);
    int dy = abs(iy2 - iy1);
    int sx = (ix1 < ix2) ? 1 : -1;
    int sy = (iy1 < iy2) ? 1 : -1;
    int err = dx - dy;

    glBegin(GL_POINTS);
    while (1) {
        plotPixel(ix1 / (float)scale, iy1 / (float)scale);

        if (ix1 == ix2 && iy1 == iy2) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            ix1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            iy1 += sy;
        }
    }
    glEnd();
}

void midpointCircle(float xc, float yc, float r) {
    int scale = 1000;

    int R = (int)(r * scale);
    int CX = (int)(xc * scale);
    int CY = (int)(yc * scale);

    int x = 0;
    int y = R;
    int p = 1 - R;

    glBegin(GL_POINTS);
    while (x <= y) {
        plotPixel((CX + x) / (float)scale, (CY + y) / (float)scale);
        plotPixel((CX - x) / (float)scale, (CY + y) / (float)scale);
        plotPixel((CX + x) / (float)scale, (CY - y) / (float)scale);
        plotPixel((CX - x) / (float)scale, (CY - y) / (float)scale);

        plotPixel((CX + y) / (float)scale, (CY + x) / (float)scale);
        plotPixel((CX - y) / (float)scale, (CY + x) / (float)scale);
        plotPixel((CX + y) / (float)scale, (CY - x) / (float)scale);
        plotPixel((CX - y) / (float)scale, (CY - x) / (float)scale);

        if (p < 0) {
            p += 2 * x + 3;
        } else {
            p += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
    glEnd();
}

float pondHalfWidthAtY(float y) {
    float dy = y - pondCenterY;
    float val = 1.0f - (dy * dy) / (pondRadiusY * pondRadiusY);

    if (val <= 0.0f) return 0.0f;
    return pondRadiusX * sqrtf(val);
}

void drawClippedHLineInPond(float y, float x1, float x2) {
    float halfW = pondHalfWidthAtY(y);
    if (halfW <= 0.0f) return;

    float leftBound = pondCenterX - halfW;
    float rightBound = pondCenterX + halfW;

    if (x1 > x2) {
        float temp = x1;
        x1 = x2;
        x2 = temp;
    }

    if (x2 < leftBound || x1 > rightBound) return;

    if (x1 < leftBound) x1 = leftBound;
    if (x2 > rightBound) x2 = rightBound;

    glBegin(GL_LINES);
    glVertex2f(x1, y);
    glVertex2f(x2, y);
    glEnd();
}

void drawSky() {
    float u = transition;
    float topR = lerp(0.55f, 0.05f, u);
    float topG = lerp(0.82f, 0.05f, u);
    float topB = lerp(0.97f, 0.20f, u);

    float botR = lerp(1.00f, 0.10f, u);
    float botG = lerp(0.86f, 0.10f, u);
    float botB = lerp(0.74f, 0.26f, u);

    glBegin(GL_QUADS);
    glColor3f(topR, topG, topB);
    glVertex2f(-1.0f, 1.0f);
    glVertex2f(1.0f, 1.0f);

    glColor3f(botR, botG, botB);
    glVertex2f(1.0f, 0.05f);
    glVertex2f(-1.0f, 0.05f);
    glEnd();
}

void drawSunMoon() {
    float horizonY = -0.02f;

    float moonX = 0.70f;

    float delayedMoonT = (transition - 0.25f) / 0.75f;
    if (delayedMoonT < 0.0f) delayedMoonT = 0.0f;
    if (delayedMoonT > 1.0f) delayedMoonT = 1.0f;

    float moonY = lerp(-0.20f, 0.76f, delayedMoonT);

    float sunX = -moonX;
    float sunY = lerp(0.76f, -0.20f, transition);

    glPointSize(2.0f);

    if (sunY > horizonY) {
        glPushMatrix();
        glTranslatef(sunX, sunY, 0.0f);

        // Rotation for the sun
        glRotatef(transition * 200.0f, 0.0f, 0.0f, 1.0f);

        glColor3f(1.0f, 0.75f, 0.10f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);

        glVertex2f(0.0f, 0.10f);   glVertex2f(0.0f, 0.14f);
        glVertex2f(0.0f, -0.10f);  glVertex2f(0.0f, -0.14f);
        glVertex2f(-0.10f, 0.0f);  glVertex2f(-0.14f, 0.0f);
        glVertex2f(0.10f, 0.0f);   glVertex2f(0.14f, 0.0f);

        glVertex2f(-0.075f, 0.075f); glVertex2f(-0.105f, 0.105f);
        glVertex2f(0.075f, 0.075f);  glVertex2f(0.105f, 0.105f);
        glVertex2f(-0.075f, -0.075f); glVertex2f(-0.105f, -0.105f);
        glVertex2f(0.075f, -0.075f);  glVertex2f(0.105f, -0.105f);

        glEnd();

        glColor3f(1.0f, 0.55f, 0.05f);
        filledCircle(0.0f, 0.0f, 0.085f, 60);

        glColor3f(0.95f, 0.35f, 0.02f);
        midpointCircle(0.0f, 0.0f, 0.085f);

        glPopMatrix();
    }

    if (transition > 0.30f) {
        glColor3f(1.0f, 1.0f, 1.0f);
        filledCircle(moonX, moonY, 0.060f, 60);

        glColor3f(0.85f, 0.85f, 0.85f);
        midpointCircle(moonX, moonY, 0.060f);
    }
}

void drawStars() {
    if (transition < 0.45f) return;

    glColor3f(1.0f, 1.0f, 1.0f);
    glPointSize(2.5f);
    glBegin(GL_POINTS);
    glVertex2f(-0.85f, 0.90f);
    glVertex2f(-0.70f, 0.84f);
    glVertex2f(-0.58f, 0.88f);
    glVertex2f(0.18f, 0.90f);
    glVertex2f(0.35f, 0.82f);
    glVertex2f(0.54f, 0.88f);
    glVertex2f(0.78f, 0.92f);
    glEnd();
}

void drawCloud(float x, float y, float s) {
    float fade = 1.0f - 0.8f * transition;
    glColor3f(fade, fade, fade);

    filledCircle(x, y, 0.03f * s, 30);
    filledCircle(x + 0.04f * s, y + 0.01f * s, 0.04f * s, 30);
    filledCircle(x + 0.08f * s, y, 0.03f * s, 30);
    filledCircle(x + 0.045f * s, y - 0.015f * s, 0.035f * s, 30);
}

void drawClouds() {
    drawCloud(-0.88f + cloudOffset1, 0.82f, 1.0f);
    drawCloud(-0.52f + cloudOffset1, 0.68f, 0.8f);
    drawCloud(-0.88f + cloudOffset1 - 1.8f, 0.82f, 1.0f);
    drawCloud(-0.52f + cloudOffset1 - 1.8f, 0.68f, 0.8f);

    drawCloud(0.45f + cloudOffset2, 0.80f, 1.0f);
    drawCloud(0.74f + cloudOffset2, 0.67f, 0.8f);
    drawCloud(0.45f + cloudOffset2 - 1.8f, 0.80f, 1.0f);
    drawCloud(0.74f + cloudOffset2 - 1.8f, 0.67f, 0.8f);
}

void drawGround() {
    glColor3f(0.33f, 0.62f, 0.22f);
    rect(-1.0f, -1.0f, 1.0f, -0.02f);

    glColor3f(0.24f, 0.53f, 0.18f);
    rect(-1.0f, -0.06f, 1.0f, -0.02f);
}

void drawBaseRoad() {
    glColor3f(0.50f, 0.16f, 0.16f);
    rect(-1.0f, -0.01f, 1.0f, 0.02f);

    glColor3f(0.75f, 0.75f, 0.75f);
    rect(-1.0f, 0.02f, 1.0f, 0.028f);

    glColor3f(0.48f, 0.15f, 0.15f);
    rect(-1.0f, 0.028f, 1.0f, 0.040f);

    glColor3f(0.95f, 0.95f, 0.95f);
    glPointSize(1.2f);
    lineDDA(-1.0f, 0.02f, 1.0f, 0.02f);
    lineDDA(-1.0f, 0.028f, 1.0f, 0.028f);
}

void drawFlower(float x, float y) {
    float t = flowerColorShift + x * 6.0f + y * 4.0f;

    float pr = 0.55f + 0.35f * (sinf(t) * 0.5f + 0.5f);
    float pg = 0.15f + 0.25f * (sinf(t + 2.1f) * 0.5f + 0.5f);
    float pb = 0.15f + 0.25f * (sinf(t + 4.2f) * 0.5f + 0.5f);

    glColor3f(0.00f, 0.42f, 0.00f);
    glLineWidth(2.4f);
    glBegin(GL_LINES);
    glVertex2f(x, y);
    glVertex2f(x, y + 0.060f);
    glEnd();

    glColor3f(0.10f, 0.78f, 0.20f);

    glBegin(GL_POLYGON);
    glVertex2f(x, y + 0.020f);
    glVertex2f(x - 0.028f, y + 0.031f);
    glVertex2f(x - 0.010f, y + 0.044f);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(x, y + 0.034f);
    glVertex2f(x + 0.028f, y + 0.045f);
    glVertex2f(x + 0.010f, y + 0.056f);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(x, y + 0.012f);
    glVertex2f(x - 0.018f, y + 0.017f);
    glVertex2f(x - 0.006f, y + 0.026f);
    glEnd();

    glColor3f(pr, pg, pb);
    filledCircle(x - 0.010f, y + 0.068f, 0.010f, 20);
    filledCircle(x + 0.010f, y + 0.068f, 0.010f, 20);
    filledCircle(x,         y + 0.080f, 0.010f, 20);
    filledCircle(x,         y + 0.056f, 0.010f, 20);

    glColor3f(1.0f, 0.85f, 0.0f);
    filledCircle(x, y + 0.068f, 0.0065f, 20);
}

void drawFlowers() {
    for (float x = -0.98f; x <= -0.62f; x += 0.050f) drawFlower(x, -0.08f);
    for (float x = 0.62f; x <= 0.98f; x += 0.050f) drawFlower(x, -0.08f);
}

void drawReflectionInPond() {
    float cx = pondCenterX;
    float cy = pondCenterY;

    glLineWidth(1.0f);
    glColor4f(0.18f, 0.18f, 0.18f, 0.18f);

    for (float y = cy + 0.010f; y >= cy - 0.005f; y -= 0.002f) {
        drawClippedHLineInPond(y, cx - 0.11f, cx + 0.11f);
    }

    float h[] = {0.24f, 0.21f, 0.18f, 0.15f, 0.12f, 0.09f, 0.07f};
    float w[] = {0.012f, 0.028f, 0.044f, 0.060f, 0.076f, 0.092f, 0.108f};

    for (int i = 6; i >= 0; i--) {
        float topY = cy + 0.010f;
        float bottomY = cy - h[i];

        for (float y = topY; y >= bottomY; y -= 0.0025f) {
            float t = (topY - y) / (topY - bottomY + 0.0001f);
            float halfWidth = w[i] * (1.0f - t);

            drawClippedHLineInPond(y, cx - halfWidth, cx);
            drawClippedHLineInPond(y, cx, cx + halfWidth);
        }
    }

    glColor4f(0.10f, 0.10f, 0.10f, 0.14f);
    for (int i = 0; i < 6; i++) {
        float y = cy - 0.02f - i * 0.025f;
        drawClippedHLineInPond(y, cx - 0.08f + i * 0.01f, cx + 0.08f - i * 0.01f);
    }
}

void drawPond() {
    float r = lerp(0.58f, 0.20f, transition);
    float g = lerp(0.82f, 0.35f, transition);
    float b = lerp(0.96f, 0.55f, transition);

    float cx = pondCenterX;
    float cy = pondCenterY;
    float rx = pondRadiusX;
    float ry = pondRadiusY;

    glColor3f(0.45f, 0.30f, 0.14f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 120; i++) {
        float a = 2.0f * PI * i / 120.0f;
        glVertex2f(cx + (rx + 0.035f) * cosf(a), cy + (ry + 0.025f) * sinf(a));
    }
    glEnd();

    glColor3f(0.32f, 0.55f, 0.20f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 120; i++) {
        float a = 2.0f * PI * i / 120.0f;
        glVertex2f(cx + (rx + 0.018f) * cosf(a), cy + (ry + 0.012f) * sinf(a));
    }
    glEnd();

    glColor3f(r, g, b);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 120; i++) {
        float a = 2.0f * PI * i / 120.0f;
        glVertex2f(cx + rx * cosf(a), cy + ry * sinf(a));
    }
    glEnd();

    drawReflectionInPond();

    glColor4f(1.0f, 1.0f, 1.0f, 0.20f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 120; i++) {
        float a = 2.0f * PI * i / 120.0f;
        glVertex2f(cx - 0.03f + (rx * 0.55f) * cosf(a), cy + 0.01f + (ry * 0.35f) * sinf(a));
    }
    glEnd();

    glColor4f(0.15f, 0.15f, 0.15f, 0.30f);
    glBegin(GL_TRIANGLES);
        glVertex2f(-0.17f, -0.255f);
        glVertex2f( 0.00f, -0.405f);
        glVertex2f( 0.17f, -0.255f);
    glEnd();

    glColor4f(0.28f, 0.28f, 0.28f, 0.20f);
    glBegin(GL_TRIANGLES);
        glVertex2f(-0.11f, -0.27f);
        glVertex2f( 0.00f, -0.37f);
        glVertex2f( 0.11f, -0.27f);
    glEnd();

    glColor4f(0.20f, 0.20f, 0.20f, 0.18f);
    rect(-0.01f, -0.27f, 0.01f, -0.39f);

    glColor3f(0.78f, 0.92f, 1.0f);
    for (int k = 0; k < 4; k++) {
        float phase = rippleAnim + k * 0.9f;
        float rrX = 0.05f + 0.03f * k + 0.004f * sinf(phase);
        float rrY = 0.015f + 0.010f * k + 0.0025f * cosf(phase);

        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 60; i++) {
            float a = 2.0f * PI * i / 60.0f;
            glVertex2f(cx + rrX * cosf(a), cy - 0.01f + rrY * sinf(a));
        }
        glEnd();
    }
}

void drawPondFlowers() {
    float cx = 0.0f;
    float cy = -0.34f;

    for (int i = 0; i < 42; i++) {
        float a = 2.0f * PI * i / 42.0f;
        float rx = 0.295f + 0.010f * sinf(i * 1.7f);
        float ry = 0.155f + 0.008f * cosf(i * 1.3f);

        float x = cx + rx * cosf(a);
        float y = cy + ry * sinf(a);
        drawFlower(x, y);
    }
}

void drawSritiShoudho() {
    glColor3f(0.55f, 0.55f, 0.55f);
    glBegin(GL_QUADS);
        glVertex2f(-0.45f, 0.02f);
        glVertex2f( 0.45f, 0.02f);
        glVertex2f( 0.45f, 0.05f);
        glVertex2f(-0.45f, 0.05f);
    glEnd();

    float h[] = {0.95f, 0.85f, 0.75f, 0.65f, 0.55f, 0.45f, 0.35f};
    float w[] = {0.05f, 0.12f, 0.18f, 0.24f, 0.30f, 0.36f, 0.42f};

    for (int i = 6; i >= 0; i--) {
        glBegin(GL_TRIANGLES);
            glColor3f(0.65f, 0.65f, 0.65f);
            glVertex2f(-w[i], 0.05f);
            glVertex2f(0.0f, h[i]);
            glVertex2f(0.0f, 0.05f);

            glColor3f(0.45f, 0.45f, 0.45f);
            glVertex2f(0.0f, h[i]);
            glVertex2f(w[i], 0.05f);
            glVertex2f(0.0f, 0.05f);
        glEnd();

        glColor3f(0.1f, 0.1f, 0.1f);
        glPointSize(1.0f);
        lineBresenham(-w[i], 0.05f, 0.0f, h[i]);
        lineBresenham(0.0f, h[i], w[i], 0.05f);
    }

    glColor3f(0.2f, 0.2f, 0.2f);
    lineBresenham(-0.04f, 0.23f, 0.04f, 0.23f);
    lineBresenham(-0.04f, 0.23f, 0.0f, 0.38f);
    lineBresenham(0.04f, 0.23f, 0.0f, 0.38f);
}

void drawFlag() {
    glColor3f(0.25f, 0.25f, 0.25f);
    rect(-0.81f, -0.02f, -0.802f, 0.42f);

    glColor3f(0.12f, 0.12f, 0.12f);
    glPointSize(1.2f);
    lineDDA(-0.806f, -0.02f, -0.806f, 0.42f);

    float w1 = 0.014f * sinf(flagWave);
    float w2 = 0.010f * sinf(flagWave + 0.7f);

    glColor3f(0.0f, 0.42f, 0.20f);
    glBegin(GL_POLYGON);
    glVertex2f(-0.802f, 0.39f);
    glVertex2f(-0.65f, 0.39f + w1);
    glVertex2f(-0.65f, 0.28f + w2);
    glVertex2f(-0.802f, 0.28f);
    glEnd();

    glColor3f(0.88f, 0.08f, 0.12f);
    filledCircle(-0.73f, 0.335f + (w1 + w2) * 0.25f, 0.028f, 40);

    glColor3f(0.60f, 0.05f, 0.08f);
    glPointSize(1.3f);
    midpointCircle(-0.73f, 0.335f + (w1 + w2) * 0.25f, 0.028f);
}

void drawBird(float x, float y, float s, float flap) {
    float wing = 0.020f * s + 0.018f * s * sinf(flap);

    glColor3f(0.10f, 0.10f, 0.10f);
    glLineWidth(2.2f);

    glBegin(GL_LINE_STRIP);
    glVertex2f(x - 0.05f * s, y);
    glVertex2f(x - 0.02f * s, y + wing);
    glVertex2f(x, y + 0.004f * s);
    glEnd();

    glBegin(GL_LINE_STRIP);
    glVertex2f(x, y + 0.004f * s);
    glVertex2f(x + 0.025f * s, y + wing * 0.9f);
    glVertex2f(x + 0.055f * s, y);
    glEnd();

    glBegin(GL_LINES);
    glVertex2f(x - 0.008f * s, y + 0.002f * s);
    glVertex2f(x + 0.008f * s, y + 0.002f * s);
    glEnd();
}

void drawBirds() {
    for (int i = 0; i < 6; i++) {
        drawBird(birds[i].x, birds[i].y, birds[i].scale, birds[i].flap);
    }
}

void drawHeldFlower(float x, float y, float s) {
    glColor3f(0.0f, 0.50f, 0.0f);
    glBegin(GL_LINES);
    glVertex2f(x, y);
    glVertex2f(x, y + 0.018f * s);
    glEnd();

    glColor3f(0.90f, 0.10f, 0.10f);
    filledCircle(x - 0.004f * s, y + 0.022f * s, 0.005f * s, 16);
    filledCircle(x + 0.004f * s, y + 0.022f * s, 0.005f * s, 16);

    glColor3f(1.0f, 0.85f, 0.0f);
    filledCircle(x, y + 0.022f * s, 0.003f * s, 16);
}

void drawStandingWithFlower(float x, float y, float s, int dir) {
    float headY = y + 0.034f * s;
    float shoulderY = y + 0.016f * s;
    float waistY = y - 0.012f * s;

    glColor3f(0.96f, 0.80f, 0.67f);
    filledCircle(x, headY, 0.010f * s, 20);

    glColor3f(0.96f, 0.80f, 0.67f);
    rect(x - 0.003f * s, y + 0.018f * s, x + 0.003f * s, y + 0.024f * s);

    glColor3f(0.10f, 0.30f, 0.85f);
    glBegin(GL_POLYGON);
    glVertex2f(x - 0.014f * s, shoulderY);
    glVertex2f(x + 0.014f * s, shoulderY);
    glVertex2f(x + 0.010f * s, waistY);
    glVertex2f(x - 0.010f * s, waistY);
    glEnd();

    glColor3f(0.96f, 0.80f, 0.67f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2f(x - 0.010f * s * dir, y + 0.010f * s);
    glVertex2f(x - 0.018f * s * dir, y - 0.002f * s);

    glVertex2f(x + 0.010f * s * dir, y + 0.010f * s);
    glVertex2f(x + 0.016f * s * dir, y - 0.002f * s);
    glEnd();

    drawHeldFlower(x + 0.016f * s * dir, y - 0.006f * s, 0.9f * s);

    glColor3f(0.12f, 0.12f, 0.12f);
    rect(x - 0.010f * s, waistY, x + 0.010f * s, y - 0.030f * s);

    glColor3f(0.05f, 0.05f, 0.05f);
    glBegin(GL_LINES);
    glVertex2f(x - 0.004f * s, y - 0.030f * s);
    glVertex2f(x - 0.010f * s, y - 0.058f * s);

    glVertex2f(x + 0.004f * s, y - 0.030f * s);
    glVertex2f(x + 0.010f * s, y - 0.058f * s);
    glEnd();
}

void drawWalkingCrowdPerson(float x, float y, float s, float phase, int dir) {
    float leg = 0.010f * s * sinf(phase);
    float arm = 0.008f * s * sinf(phase);

    float headY = y + 0.034f * s;
    float shoulderY = y + 0.016f * s;
    float waistY = y - 0.012f * s;

    glColor3f(0.96f, 0.80f, 0.67f);
    filledCircle(x, headY, 0.010f * s, 20);

    glColor3f(0.96f, 0.80f, 0.67f);
    rect(x - 0.003f * s, y + 0.018f * s, x + 0.003f * s, y + 0.024f * s);

    glColor3f(0.10f, 0.30f, 0.85f);
    glBegin(GL_POLYGON);
    glVertex2f(x - 0.014f * s, shoulderY);
    glVertex2f(x + 0.014f * s, shoulderY);
    glVertex2f(x + 0.010f * s, waistY);
    glVertex2f(x - 0.010f * s, waistY);
    glEnd();

    glColor3f(0.96f, 0.80f, 0.67f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2f(x - 0.010f * s * dir, y + 0.010f * s);
    glVertex2f(x - 0.022f * s * dir, y - 0.002f * s - arm);

    glVertex2f(x + 0.010f * s * dir, y + 0.010f * s);
    glVertex2f(x + 0.022f * s * dir, y - 0.002f * s + arm);
    glEnd();

    drawHeldFlower(x + 0.022f * s * dir, y - 0.002f * s + arm - 0.004f * s, 0.9f * s);

    glColor3f(0.12f, 0.12f, 0.12f);
    rect(x - 0.010f * s, waistY, x + 0.010f * s, y - 0.030f * s);

    glColor3f(0.05f, 0.05f, 0.05f);
    glBegin(GL_LINES);
    glVertex2f(x - 0.004f * s, y - 0.030f * s);
    glVertex2f(x - 0.014f * s, y - 0.058f * s - leg);

    glVertex2f(x + 0.004f * s, y - 0.030f * s);
    glVertex2f(x + 0.014f * s, y - 0.058f * s + leg);
    glEnd();
}

void drawPeople() {
    for (int i = 0; i < 5; i++) {
        float x = -0.96f + i * 0.042f + leftPeopleOffset;
        if (leftCrowdStopped) drawStandingWithFlower(x, -0.025f, 1.15f, 1);
        else drawWalkingCrowdPerson(x, -0.025f, 1.15f, leftWalkAnim + i * 0.7f, 1);
    }

    for (int i = 0; i < 3; i++) {
        float x = 0.86f + i * 0.032f - rightPeopleOffset;
        if (rightCrowdStopped) drawStandingWithFlower(x, -0.050f, 1.08f, -1);
        else drawWalkingCrowdPerson(x, -0.050f, 1.08f, rightWalkAnim + i * 0.7f, -1);
    }
}

void drawWalkingPerson() {
    float x = personX;
    float leg = 0.018f * sinf(stepAnim);
    float arm = 0.014f * sinf(stepAnim);

    glColor3f(0.96f, 0.80f, 0.67f);
    filledCircle(x, 0.010f, 0.014f, 20);

    glColor3f(0.96f, 0.80f, 0.67f);
    rect(x - 0.003f, -0.002f, x + 0.003f, 0.002f);

    glColor3f(0.85f, 0.18f, 0.18f);
    glBegin(GL_POLYGON);
    glVertex2f(x - 0.014f, -0.002f);
    glVertex2f(x + 0.014f, -0.002f);
    glVertex2f(x + 0.010f, -0.045f);
    glVertex2f(x - 0.010f, -0.045f);
    glEnd();

    glColor3f(0.96f, 0.80f, 0.67f);
    glLineWidth(2.5f);
    glBegin(GL_LINES);
    glVertex2f(x - 0.010f, -0.010f);
    glVertex2f(x - 0.028f, -0.024f - arm);

    glVertex2f(x + 0.010f, -0.010f);
    glVertex2f(x + 0.030f,  0.000f + arm);
    glEnd();

    glColor3f(0.10f, 0.10f, 0.18f);
    rect(x - 0.010f, -0.045f, x + 0.010f, -0.065f);

    glColor3f(0.05f, 0.05f, 0.05f);
    glBegin(GL_LINES);
    glVertex2f(x - 0.004f, -0.065f);
    glVertex2f(x - 0.022f, -0.110f - leg);

    glVertex2f(x + 0.004f, -0.065f);
    glVertex2f(x + 0.024f, -0.110f + leg);
    glEnd();

    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_LINES);
    glVertex2f(x + 0.030f, 0.000f + arm);
    glVertex2f(x + 0.030f, 0.095f);
    glEnd();

    glColor3f(0.15f, 0.15f, 0.15f);
    glPointSize(1.1f);
    lineDDA(x + 0.030f, 0.000f + arm, x + 0.030f, 0.095f);

    float w = 0.008f * sinf(flagWave * 1.8f);

    glColor3f(0.0f, 0.42f, 0.20f);
    glBegin(GL_POLYGON);
    glVertex2f(x + 0.030f, 0.088f);
    glVertex2f(x + 0.100f, 0.088f + w);
    glVertex2f(x + 0.100f, 0.045f + w);
    glVertex2f(x + 0.030f, 0.045f);
    glEnd();

    glColor3f(0.90f, 0.10f, 0.10f);
    filledCircle(x + 0.067f, 0.066f + w * 0.5f, 0.010f, 20);

    glColor3f(0.60f, 0.05f, 0.05f);
    midpointCircle(x + 0.067f, 0.066f + w * 0.5f, 0.010f);
}

void drawNightLights() {
    if (transition < 0.45f) return;

    float glow = (transition - 0.45f) / 0.55f;
    glColor3f(glow, glow * 0.85f, glow * 0.35f);

    for (float x = -0.32f; x <= 0.32f; x += 0.12f)
        filledCircle(x, 0.012f, 0.010f, 20);

    filledCircle(-0.82f, -0.05f, 0.008f, 20);
    filledCircle(-0.73f, -0.05f, 0.008f, 20);
    filledCircle(0.73f, -0.05f, 0.008f, 20);
    filledCircle(0.82f, -0.05f, 0.008f, 20);
}

void explodeRocket(float cx, float cy, float cr, float cg, float cb) {
    int k = -1;
    for (int i = 0; i < FIREWORK_COUNT; i++) {
        int busy = 0;
        for (int j = 0; j < PARTICLES_PER_FIREWORK; j++) {
            if (fireworks[i][j].active) { busy = 1; break; }
        }
        if (!busy) { k = i; break; }
    }
    if (k == -1) k = rand() % FIREWORK_COUNT;

    for (int i = 0; i < PARTICLES_PER_FIREWORK; i++) {
        float angle = 2.0f * PI * i / PARTICLES_PER_FIREWORK;
        float band = (i % 4 == 0) ? 1.15f : ((i % 3 == 0) ? 0.95f : 0.75f);
        float speed = randomFloat(0.004f, 0.0115f) * band;

        fireworks[k][i].x = cx;
        fireworks[k][i].y = cy;
        fireworks[k][i].dx = cosf(angle) * speed;
        fireworks[k][i].dy = sinf(angle) * speed;
        fireworks[k][i].life = randomFloat(0.75f, 1.15f);
        fireworks[k][i].r = cr;
        fireworks[k][i].g = cg;
        fireworks[k][i].b = cb;
        fireworks[k][i].active = 1;
    }
}

void launchPairRockets() {
    rockets[0].x = -0.68f;
    rockets[0].y = 0.04f;
    rockets[0].targetY = randomFloat(0.64f, 0.84f);
    rockets[0].speed = randomFloat(0.010f, 0.012f);
    rockets[0].r = 1.0f;
    rockets[0].g = 0.82f;
    rockets[0].b = 0.25f;
    rockets[0].active = 1;

    rockets[1].x = 0.68f;
    rockets[1].y = 0.04f;
    rockets[1].targetY = randomFloat(0.64f, 0.84f);
    rockets[1].speed = randomFloat(0.010f, 0.012f);
    rockets[1].r = 1.0f;
    rockets[1].g = 0.25f;
    rockets[1].b = 0.20f;
    rockets[1].active = 1;
}

void triggerCenterBurst() {
    explodeRocket(0.0f, 0.90f, 1.0f, 0.85f, 0.20f);
    explodeRocket(-0.02f, 0.88f, 1.0f, 0.35f, 0.15f);
    explodeRocket(0.02f, 0.88f, 1.0f, 0.55f, 0.20f);
    centerTextStarted = 1;
}

void updateRockets() {
    for (int i = 0; i < ROCKET_COUNT; i++) {
        if (!rockets[i].active) continue;
        rockets[i].y += rockets[i].speed;

        if (rockets[i].y >= rockets[i].targetY) {
            explodeRocket(rockets[i].x, rockets[i].y, rockets[i].r, rockets[i].g, rockets[i].b);
            explodeRocket(rockets[i].x + randomFloat(-0.03f, 0.03f),
                          rockets[i].y + randomFloat(-0.02f, 0.02f),
                          rockets[i].r * 0.9f, rockets[i].g * 0.9f, rockets[i].b * 0.9f);
            rockets[i].active = 0;
        }
    }
}

void updateFireworks() {
    for (int i = 0; i < FIREWORK_COUNT; i++) {
        for (int j = 0; j < PARTICLES_PER_FIREWORK; j++) {
            if (!fireworks[i][j].active) continue;

            fireworks[i][j].x += fireworks[i][j].dx;
            fireworks[i][j].y += fireworks[i][j].dy;
            fireworks[i][j].dy -= 0.00009f;
            fireworks[i][j].dx *= 0.994f;
            fireworks[i][j].life -= 0.011f;

            if (fireworks[i][j].life <= 0.0f) fireworks[i][j].active = 0;
        }
    }
}

void drawRocketTrail(float x, float y, float r, float g, float b) {
    glLineWidth(2.2f);
    glBegin(GL_LINES);
    for (int i = 0; i < 12; i++) {
        float t = i / 12.0f;
        glColor3f(r * (1.0f - t), g * (1.0f - t), b * (1.0f - t));
        glVertex2f(x, y - 0.010f * i);
        glVertex2f(x, y - 0.010f * (i + 1));
    }
    glEnd();
}

void drawRockets() {
    if (transition < 0.55f) return;

    for (int i = 0; i < ROCKET_COUNT; i++) {
        if (!rockets[i].active) continue;

        drawRocketTrail(rockets[i].x, rockets[i].y, rockets[i].r, rockets[i].g, rockets[i].b);

        glColor3f(1.0f, 0.95f, 0.75f);
        filledCircle(rockets[i].x, rockets[i].y, 0.010f, 18);

        glColor3f(rockets[i].r, rockets[i].g, rockets[i].b);
        filledCircle(rockets[i].x, rockets[i].y, 0.005f, 18);
    }
}

void drawFireworks() {
    if (transition < 0.55f) return;

    glPointSize(4.2f);
    glBegin(GL_POINTS);
    for (int i = 0; i < FIREWORK_COUNT; i++) {
        for (int j = 0; j < PARTICLES_PER_FIREWORK; j++) {
            if (!fireworks[i][j].active) continue;

            float fade = fireworks[i][j].life;
            glColor3f(fireworks[i][j].r * fade,
                      fireworks[i][j].g * fade,
                      fireworks[i][j].b * fade);
            glVertex2f(fireworks[i][j].x, fireworks[i][j].y);
        }
    }
    glEnd();

    for (int i = 0; i < FIREWORK_COUNT; i++) {
        for (int j = 0; j < PARTICLES_PER_FIREWORK; j += 8) {
            if (!fireworks[i][j].active) continue;

            float fade = fireworks[i][j].life * 0.32f;
            glColor4f(fireworks[i][j].r * fade,
                      fireworks[i][j].g * fade,
                      fireworks[i][j].b * fade, 1.0f);
            filledCircle(fireworks[i][j].x, fireworks[i][j].y, 0.008f, 16);
        }
    }
}

void drawText(const char* text, float x, float y, float scale) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(scale, scale, 1.0f);
    for (const char* c = text; *c != '\0'; c++) {
        glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
    }
    glPopMatrix();
}

void drawCelebrationText() {
    if (!centerTextStarted) return;

    float a = textFade;
    glColor4f(1.0f, 0.92f, 0.20f, a);
    drawText("HAPPY INDEPENDENCE DAY", -0.58f, 0.84f, 0.00060f);

    glColor4f(1.0f, 0.45f, 0.10f, a * 0.35f);
    drawText("HAPPY INDEPENDENCE DAY", -0.577f, 0.843f, 0.00062f);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    drawSky();
    drawSunMoon();
    drawStars();
    drawClouds();

    drawGround();
    drawBaseRoad();
    drawSritiShoudho();
    drawFlag();

    drawFlowers();
    drawPond();
    drawPondFlowers();

    drawPeople();
    drawWalkingPerson();
    drawNightLights();

    drawRockets();
    drawFireworks();
    drawCelebrationText();

    drawBirds();

    glutSwapBuffers();
}

void update(int value) {
    if (transition < 1.0f) transition += 0.0009f;

    flagWave += 0.10f;
    flowerColorShift += 0.01f;
    rippleAnim += 0.05f;

    if (centerTextStarted && textFade < 1.0f) {
        textFade += 0.015f;
    }

    personX += 0.0025f;
    if (personX > 1.15f) personX = -1.15f;
    stepAnim += 0.18f;

    cloudOffset1 += 0.00008f;
    cloudOffset2 += 0.00012f;

    if (cloudOffset1 > 1.8f) cloudOffset1 = 0.0f;
    if (cloudOffset2 > 1.8f) cloudOffset2 = 0.0f;

    if (!leftCrowdStopped) {
        if (leftPeopleOffset < 0.56f) {
            leftPeopleOffset += 0.00045f;
            leftWalkAnim += 0.12f;
        } else {
            leftCrowdStopped = 1;
        }
    }

    if (!rightCrowdStopped) {
        if (rightPeopleOffset < 0.48f) {
            rightPeopleOffset += 0.00040f;
            rightWalkAnim += 0.12f;
        } else {
            rightCrowdStopped = 1;
        }
    }

    for (int i = 0; i < 6; i++) {
        birds[i].x += birds[i].speed;
        birds[i].flap += 0.18f + i * 0.02f;
        if (birds[i].x > 1.15f) birds[i].x = -1.15f;
    }

    if (transition > 0.55f) {
        rocketLaunchTimer += 0.016f;
        if (rocketLaunchTimer > 1.9f) {
            launchPairRockets();
            rocketLaunchTimer = 0.0f;
        }
    }

    if (transition > 0.72f) {
        centerBurstTimer += 0.016f;
        if (centerBurstTimer > 3.2f) {
            triggerCenterBurst();
            centerBurstTimer = 0.0f;
        }
    }

    updateRockets();
    updateFireworks();

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void init() {
    srand((unsigned int)time(NULL));

    glClearColor(1, 1, 1, 1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (int i = 0; i < FIREWORK_COUNT; i++) {
        for (int j = 0; j < PARTICLES_PER_FIREWORK; j++) {
            fireworks[i][j].active = 0;
        }
    }

    for (int i = 0; i < ROCKET_COUNT; i++) {
        rockets[i].active = 0;
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1000, 700);
    glutCreateWindow("Bangladesh Independence Day - Sriti Shoudho Final");

    init();
    glutDisplayFunc(display);
    glutTimerFunc(16, update, 0);
    glutMainLoop();
    return 0;
}

