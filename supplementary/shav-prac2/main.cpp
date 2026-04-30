//shavir vallabhios
//u23718146

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <fstream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "shader.hpp"
#include "Shape.h"
#include "Square.h"
#include "Triangle.h"
#include "Polygon.h"
#include "Vector.h"
#include "Matrix.h"
#include <cstring>

using namespace std;

//window
const int WIN_W = 1000;
const int WIN_H = 1000;

//store colours
struct RGB { float r, g, b; };

//pastel: blend t=0.55 toward white
RGB pastel(RGB c) {
    float t = 0.5f;
    return { c.r + t*(1-c.r), c.g + t*(1-c.g), c.b + t*(1-c.b) };
}

/*
 * primitive type tells the renderer how to draw and how to build wireframe.
 *   TRIFAN  - GL_TRIANGLE_FAN  (Polygon, Square)
 *   TRILIST - GL_TRIANGLES (Triangle)
 */
enum class Prim { TRIFAN, TRILIST };

struct ShapeRenderer {
    Shape<3>*  shape;
    GLuint vao, vbo;
    RGB normalColour;
    RGB selectedColour;
    Prim prim;
    int vertexCount; //number of VERTICES (not floats) for the draw call

    //build a flat interleaved buffer: [x y z  r g b]  per vertex (6 floats)
    //z is dropped, since 2d
    void upload(bool wireframe, bool selected) {
        float* pts = shape->getPoints();  //returns n*vertexCount floats (n=3)
        RGB col = selected ? selectedColour : normalColour;

        //for wireframe we expand triangles into line pairs
        vector<float> buf;

        if (!wireframe) {
            //filled: just pack x,y,r,g,b per vertex, skipping z
            for (int i = 0; i < vertexCount; i++) {
                buf.push_back(pts[i*3 + 0]);  //x
                buf.push_back(pts[i*3 + 1]);  //y
                buf.push_back(col.r);
                buf.push_back(col.g);
                buf.push_back(col.b);
            }
        } else {
            //Wireframe: convert each triangle to 3 line segments (6 vertices)
            //need to reconstruct the triangle list first
            //For TRIFAN: triangle i = {fan[0], fan[i], fan[i+1]}
            //For TRILIST: triangle i = {list[i*3], list[i*3+1], list[i*3+2]}

            auto pushV = [&](int idx) {
                buf.push_back(pts[idx*3 + 0]);
                buf.push_back(pts[idx*3 + 1]);
                buf.push_back(col.r);
                buf.push_back(col.g);
                buf.push_back(col.b);
            };
            auto pushEdge = [&](int a, int b) {
                pushV(a); pushV(b);
            };

            if (prim == Prim::TRIFAN) {
                //fan has vertexCount-2 triangles
                //triangle i: vertex 0, vertex i+1, vertex i+2 
                int triCount = vertexCount - 2;
                for (int i = 0; i < triCount; i++) {
                    int a = 0, b = i+1, c = i+2;
                    pushEdge(a, b);
                    pushEdge(b, c);
                    pushEdge(c, a);
                }
            } else {
                //TRILIST: every 3 vertices form one triangle
                int triCount = vertexCount / 3;
                for (int i = 0; i < triCount; i++) {
                    int a = i*3, b = i*3+1, c = i*3+2;
                    pushEdge(a, b);
                    pushEdge(b, c);
                    pushEdge(c, a);
                }
            }
        }

        delete[] pts;

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     buf.size() * sizeof(float),
                     buf.data(), GL_STREAM_DRAW);

        //Attribute 0: position (x, y)
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        //Attribute 1: colour (r, g, b)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(2*sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    void draw(bool wireframe, bool selected) {
        upload(wireframe, selected);
        glBindVertexArray(vao);

        if (!wireframe) {
            GLenum mode = (prim == Prim::TRIFAN) ? GL_TRIANGLE_FAN : GL_TRIANGLES;
            glDrawArrays(mode, 0, vertexCount);
        } else {
            // Each triangle -> 3 edges -> 6 vertices
            int triCount = (prim == Prim::TRIFAN)
                           ? vertexCount - 2
                           : vertexCount / 3;
            glDrawArrays(GL_LINES, 0, triCount * 6);
        }

        glBindVertexArray(0);
    }
};

//global state
vector<ShapeRenderer> renderers; //all renderable shapes, back-to-front order
int selectedIdx   = -1; //index into renderers, -1 = none
bool wireframe    = false;
double lastEnter  = 0.0; //debounce Enter key

//indices of the 6 selectable shapes. pac+1, tree+1, tree+2 used because compound shapes
int idxBall = -1, idxLog = -1, idxRamp = -1, idxHole = -1; int idxPac = -1; int idxTree = -1;

//momentum, space bar sends the ball:
bool momentumCharged = false;
const float MOMENTUM_MULT = 15.0f;


//aabb stored as (cx, cy, halfwidth, halfheight)
struct CircleBound { float x, y, r; }; 
struct AABBBound   { float cx, cy, hw, hh; };

vector<CircleBound> circleBounds;  //bushes
vector<AABBBound>   aabbBounds; //logs

const float GRASS_RADIUS = 0.98f;  //ball must stay within grass defined by this
const float BALL_RADIUS  = 0.038f;


//make a shape and add it to the global list, returning its index in the list
int addShape(Shape<3>* s, Prim prim, int vertexCount, RGB col) {
    ShapeRenderer sr;
    sr.shape = s;
    sr.prim = prim;
    sr.vertexCount = vertexCount;
    sr.normalColour = col;
    sr.selectedColour = pastel(col);
    glGenVertexArrays(1, &sr.vao);
    glGenBuffers(1, &sr.vbo);
    int idx = (int)renderers.size();
    renderers.push_back(sr);
    return idx;
}

void getBallCentre(float& bx, float& by) {
    float coords[3] = {0,0,0};
    renderers[idxBall].shape->midPoint(coords);
    bx = coords[0];
    by = coords[1];
}

bool ballCollides() {
    float bx, by;
    getBallCentre(bx, by);

    // 1. Grass boundary
    float distFromOrigin = std::sqrt(bx*bx + by*by);
    if (distFromOrigin + BALL_RADIUS > GRASS_RADIUS)
        return true;

    // 2. Circle obstacles
    for (auto& cb : circleBounds) {
        float dx = bx - cb.x, dy = by - cb.y;
        float dist = std::sqrt(dx*dx + dy*dy);
        if (dist < BALL_RADIUS + cb.r)
            return true;
    }

    // 3. AABB obstacles (Minkowski sum: expand AABB by ball radius)
    for (auto& ab : aabbBounds) {
        float left  = ab.cx - ab.hw - BALL_RADIUS;
        float right = ab.cx + ab.hw + BALL_RADIUS;
        float bot   = ab.cy - ab.hh - BALL_RADIUS;
        float top   = ab.cy + ab.hh + BALL_RADIUS;
        if (bx > left && bx < right && by > bot && by < top)
            return true;
    }

    return false;
}

bool moveBallSafe(char dir) {
    Shape<3>* ball = renderers[idxBall].shape;

    int steps = momentumCharged ? (int)MOMENTUM_MULT : 1;
    momentumCharged = false;  // consume charge regardless of outcome

    for (int i = 0; i < steps; i++) {
        ball->move(dir);
        if (ballCollides()) {
            // Undo this step with the opposite direction
            char undo = dir;
            if      (dir == 'w') undo = 's';
            else if (dir == 's') undo = 'w';
            else if (dir == 'a') undo = 'd';
            else if (dir == 'd') undo = 'a';
            ball->move(undo);
            return false;  // stop trying further steps
        }
    }
    return true;
}

void saveScreenshot() {
    int w = WIN_W, h = WIN_H;
    int rowBytes = w * 3;

    //glReadPixels gives BGR bottom-to-top, BMP also wants bottom-to-top
    //BUT BMP wants BGR, and GL_RGB gives us RGB, so we swap R and B per pixel
    vector<unsigned char> buf(rowBytes * h);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, buf.data());

    //swap R and B 
    for (int i = 0; i < w * h; i++) {
        unsigned char tmp = buf[i*3 + 0]; //R
        buf[i*3 + 0] = buf[i*3 + 2]; //R = B
        buf[i*3 + 2] = tmp; //B = R
    }

    //bmp requires multiple of 4 row bytes
    int stride = (rowBytes + 3) & ~3;  //round up to nearest 4
    int padding = stride - rowBytes;
    int pixelDataSize = stride * h;

    //BMP file header (14 bytes) + DIB header (40 bytes) = 54 bytes total
    int fileSize = 54 + pixelDataSize;

    unsigned char header[54] = {};

    //i have no fucking idea man, we gotta write the header byte by byte
    header[0] = 'B'; header[1] = 'M';
    header[2] = fileSize & 0xFF;
    header[3] = (fileSize >> 8)  & 0xFF;
    header[4] = (fileSize >> 16) & 0xFF;
    header[5] = (fileSize >> 24) & 0xFF;
    // bytes 6-9: reserved, already 0
    header[10] = 54;  //pixel offset

    //bitmapinfo header starting at [14]
    header[14] = 40;  //header size
    header[18] = w & 0xFF; //cap height
    header[19] = (w >> 8)  & 0xFF;
    header[20] = (w >> 16) & 0xFF;
    header[21] = (w >> 24) & 0xFF;
    header[22] = h & 0xFF;
    header[23] = (h >> 8)  & 0xFF;
    header[24] = (h >> 16) & 0xFF;
    header[25] = (h >> 24) & 0xFF;
    header[26] = 1;   //colour planes
    header[28] = 24;  //bits per pixel (RGB = 24)

    //timestamped filename
    time_t now = time(nullptr);
    tm* t = localtime(&now);
    char fname[64];
    snprintf(fname, sizeof(fname), "screenshot_%02d%02d%02d.bmp",
             t->tm_hour, t->tm_min, t->tm_sec);

    FILE* f = fopen(fname, "wb");
    if (!f) { cerr << "Could not open " << fname << "\n"; return; }

    fwrite(header, 1, 54, f);

    //write pixel rows bottom to top
    unsigned char pad[3] = {0, 0, 0};
    for (int row = 0; row < h; row++) {
        fwrite(buf.data() + row * rowBytes, 1, rowBytes, f);
        if (padding > 0) fwrite(pad, 1, padding, f);
    }

    fclose(f);
    cout << "Screenshot saved: " << fname << "\n";
}

//helper to make a Vector<3> for homogeneous 2D: (x, y, 1)
Vector<3> v3(float x, float y) {
    Vector<3> v;
    v[0] = x; v[1] = y; v[2] = 1.0f;
    return v;
}

void buildScene() {
    RGB cGrass     = {0.11f, 0.50f, 0.11f};
    RGB cConcrete  = {0.36f, 0.36f, 0.36f};
    RGB cPac       = {0.95f, 0.85f, 0.00f};
    RGB cPacEye    = {0.05f, 0.05f, 0.05f};
    RGB cDrool     = {0.60f, 0.80f, 1.00f};
    RGB cRiver     = {0.25f, 0.35f, 0.75f}; 
    RGB cFlowerP   = {1.00f, 0.40f, 0.70f};
    RGB cFlowerY   = {1.00f, 0.90f, 0.10f};
    RGB cTreeTrunk = {0.50f, 0.30f, 0.05f};
    RGB cTreeTop   = {0.05f, 0.40f, 0.05f};
    RGB cBush      = {0.00f, 0.30f, 0.00f};
    RGB cHole      = {0.73f, 0.05f, 0.73f};
    RGB cBall      = {1.00f, 1.00f, 1.00f};
    RGB cLog       = {0.65f, 0.35f, 0.10f};
    RGB cDarkLog   = {0.25f, 0.15f, 0.05f};
    RGB cRamp      = {0.80f, 0.20f, 0.10f};

    //background
    addShape(new Polygon<3>(v3(0.0f, 0.0f), 1.6f, 4, 0.785f),
             Prim::TRIFAN, 6, cPacEye);

    //border
    addShape(new Polygon<3>(v3(0.0f, 0.0f), 1.09f, 11, 0.4f),
             Prim::TRIFAN, 13, cConcrete);
    
    //second border
    //border
    addShape(new Polygon<3>(v3(0.0f, 0.0f), 1.07f, 11, 0.4f),
             Prim::TRIFAN, 13, cLog);

    //grass
    addShape(new Polygon<3>(v3(0.0f, 0.0f), 0.99f, 11, 0.4f),
             Prim::TRIFAN, 13, cGrass);

    //puddle
    addShape(new Polygon<3>(v3(0.13f, -0.3f), 0.3f, 18, 0.3f),
             Prim::TRIFAN, 18, cRiver);  
    addShape(new Polygon<3>(v3(0.3f, -0.43f), 0.25f, 19, 0.78f),
             Prim::TRIFAN, 21, cRiver); 


    //pacman drooling 
    addShape(new Polygon<3>(v3(0.05f, -0.05f), 0.11f, 4, 0.4),
             Prim::TRIFAN, 6, cDrool);
    //widens as it falls
    addShape(new Square<3>(v3(0.17f, -0.10f), 0.10f, 0.06f),
             Prim::TRIFAN, 4, cDrool);
    addShape(new Square<3>(v3(0.19f, -0.21f), 0.10f, 0.08f),
             Prim::TRIFAN, 4, cDrool);
    //blob at the bottom just before hitting the puddle
    addShape(new Polygon<3>(v3(0.21f, -0.32f), 0.08f, 10),
             Prim::TRIFAN, 12, cDrool);

    //pac man
    idxPac = addShape(new Polygon<3>(v3(-0.20f, 0.15f), 0.42f, 12, 0.4f),
             Prim::TRIFAN, 12, cPac); 

    // Eye - small dark circle
    addShape(new Polygon<3>(v3(-0.08f, 0.40f), 0.045f, 20),
             Prim::TRIFAN, 22, cPacEye);

    
    //trees
    // Tree 1 - upper left, moved inward
    idxTree = addShape(new Square<3>(v3(-0.72f, 0.22f), 0.14f, 0.05f),
             Prim::TRIFAN, 4, cTreeTrunk);
    addShape(new Triangle<3>(
                 v3(-0.72f,  0.48f),
                 v3(-0.86f,  0.18f),
                 v3(-0.58f,  0.18f)),
             Prim::TRILIST, 3, cTreeTop);
    addShape(new Triangle<3>(
                 v3(-0.72f,  0.60f),
                 v3(-0.82f,  0.38f),
                 v3(-0.62f,  0.38f)),
             Prim::TRILIST, 3, cTreeTop);

    //tree2, lower left
    addShape(new Square<3>(v3(-0.40f, -0.58f), 0.14f, 0.05f),
             Prim::TRIFAN, 4, cTreeTrunk);
    addShape(new Triangle<3>(
                 v3(-0.40f, -0.42f),
                 v3(-0.54f, -0.62f),
                 v3(-0.26f, -0.62f)),
             Prim::TRILIST, 3, cTreeTop);
    addShape(new Triangle<3>(
                 v3(-0.40f, -0.25f),
                 v3(-0.50f, -0.47f),
                 v3(-0.30f, -0.47f)),
             Prim::TRILIST, 3, cTreeTop);

    //flowers
    auto makeFlower = [&](float fx, float fy, float pr, float cr) {
        //6 petals as pentagons in a ring, then yellow centre
        float offsets[6][2] = {
            { 0.00f,  cr}, { cr*0.87f,  cr*0.50f}, { cr*0.87f, -cr*0.50f},
            { 0.00f, -cr}, {-cr*0.87f, -cr*0.50f}, {-cr*0.87f,  cr*0.50f}
        };
        for (auto& o : offsets)
            //for each offset create a pentagon at the resulting coords
            addShape(new Polygon<3>(v3(fx+o[0], fy+o[1]), pr, 7),
                     Prim::TRIFAN, 9, cFlowerP);
        addShape(new Polygon<3>(v3(fx, fy), pr*0.7f, 16),
                 Prim::TRIFAN, 18, cFlowerY);
    };

    makeFlower( 0.03f,  0.63f, 0.055f, 0.058f);  // top
    makeFlower( 0.65f,  0.18f, 0.048f, 0.052f);  // mid right
    makeFlower( -0.63f, -0.18f, 0.040f, 0.044f);  //lower left


    //bush octagons (or lily pads or whatever)
    addShape(new Polygon<3>(v3( 0.48f, -0.10f), 0.065f, 8),
             Prim::TRIFAN, 10, cBush);
    addShape(new Polygon<3>(v3(-0.02f, -0.38f), 0.055f, 8),
             Prim::TRIFAN, 10, cBush);
    addShape(new Polygon<3>(v3(-0.55f, 0.60f), 0.055f, 8),
             Prim::TRIFAN, 10, cBush);

    //log obstacle (selectable key 2) - horizontal, just right of mouth
    idxLog = addShape(new Square<3>(v3(0.22f, -0.48f), 0.06f, 0.8f),
                      Prim::TRIFAN, 4, cLog);
    //another one
    addShape(new Square<3>(v3(0.72f, 0.38f), 0.07f, 0.4f),
             Prim::TRIFAN, 4, cLog);

    //triangle, key 3
    idxRamp = addShape(
        new Triangle<3>(v3(0.42f,  0.35f), v3(0.28f, 0.07f), v3(0.56f, 0.07f)),
        Prim::TRILIST, 3, cRamp);
    // Second tri, lower area
    addShape(
        new Triangle<3>(v3(-0.63f, -0.28f), v3(-0.62f, -0.52f), v3(-0.53f, -0.52f)),
        Prim::TRILIST, 3, cRamp);

    //hole
    idxHole = addShape(new Polygon<3>(v3(0.52f, 0.58f), 0.07f, 60),
                       Prim::TRIFAN, 62, cHole);

    //starting area
    addShape(new Square<3>(v3(-0.03f, -0.75f), 0.15f, 0.20f),
                       Prim::TRIFAN, 4, cBush);

    //golf ball
    idxBall = addShape(new Polygon<3>(v3(-0.03f, -0.75f), 0.038f, 60),
                       Prim::TRIFAN, 62, cBall);
}


void selectShape(int idx) {
    // Deselect current
    if (selectedIdx >= 0)
        renderers[selectedIdx].shape->deselect();

    selectedIdx = idx;

    if (idx >= 0)
        renderers[idx].shape->select();
}

const char* getError() {
    const char* d; glfwGetError(&d); return d;
}
inline void startUpGLFW() {
    glewExperimental = true;
    if (!glfwInit()) throw getError();
}
inline void startUpGLEW() {
    glewExperimental = true;
    if (glewInit() != GLEW_OK) { glfwTerminate(); throw getError(); }
}
inline GLFWwindow* setUp() {
    startUpGLFW();
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* w = glfwCreateWindow(WIN_W, WIN_H, "u23718146", NULL, NULL);
    if (!w) { cout << getError(); glfwTerminate(); throw "Window failed"; }
    glfwMakeContextCurrent(w);
    startUpGLEW();
    return w;
}

//callback to run everything in
void keyCallback(GLFWwindow* win, int key, int /*sc*/, int action, int /*mods*/) {
    if (action == GLFW_RELEASE) return;

    //single key press
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_1: selectShape(idxBall); return;
            case GLFW_KEY_2: selectShape(idxLog);  return;
            case GLFW_KEY_3: selectShape(idxRamp); return;
            case GLFW_KEY_4: selectShape(idxHole); return;
            case GLFW_KEY_0: selectShape(-1);      return;

            case GLFW_KEY_ENTER: {
                //wireframe toggle, using a small delay
                double now = glfwGetTime();
                if (now - lastEnter > 0.25) {
                    wireframe = !wireframe;
                    lastEnter = now;
                }
                return;
            }

            case GLFW_KEY_P:
                saveScreenshot();
                return;

            case GLFW_KEY_SPACE:
                if (selectedIdx == idxBall) {
                    momentumCharged = true;
                }
                return;

            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(win, GLFW_TRUE);
                return;
            default: break;
        }
    }

    //transformations
    
    if (selectedIdx < 0) return;
    Shape<3>* s = renderers[selectedIdx].shape;
    bool isBall = (selectedIdx == idxBall);
    
    switch (key) {
        //WASD = translate
        case GLFW_KEY_W:
            if (isBall) moveBallSafe('w');
            else s->move('w');
            break;
        case GLFW_KEY_S:
            if (isBall) moveBallSafe('s');
            else s->move('s');
            break;
        case GLFW_KEY_A:
            if (isBall) moveBallSafe('a');
            else s->move('a');
            break;
        case GLFW_KEY_D:
            if (isBall) moveBallSafe('d');
            else s->move('d');
            break;

        //+/- = scale
        case GLFW_KEY_EQUAL:
        case GLFW_KEY_KP_ADD:    s->scale('+'); break;
        case GLFW_KEY_MINUS:
        case GLFW_KEY_KP_SUBTRACT: s->scale('-'); break;

        //L/R = rotate
        case GLFW_KEY_LEFT:
        case GLFW_KEY_Q: s->rotate('l'); break;
        case GLFW_KEY_RIGHT:
        case GLFW_KEY_E: s->rotate('r'); break;

        default: break;
    }
}

int main() {
    GLFWwindow* window;
    try { window = setUp(); }
    catch (const char* e) { cout << e << endl; return -1; }

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetKeyCallback(window, keyCallback);

    //load shaders
    GLuint prog = LoadShaders("vertex.glsl", "fragment.glsl");
    if (prog == 0) { cerr << "Shader load failed\n"; glfwTerminate(); return -1; }

    buildScene();

    //dark background
    glClearColor(0.08f, 0.08f, 0.08f, 1.0f);

    //Main rendering loop
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(prog);

        //draw all shapes back to front (painter's algorithm)
        for (int i = 0; i < (int)renderers.size(); i++) {
            renderers[i].draw(wireframe, i == selectedIdx);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    //cleanup
    for (auto& r : renderers) {
        delete r.shape;
        glDeleteVertexArrays(1, &r.vao);
        glDeleteBuffers(1, &r.vbo);
    }
    glDeleteProgram(prog);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}