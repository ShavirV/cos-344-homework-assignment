//shavir vallabh
//u23718146
//
// DINO PARK PHUKET - SWAMP WATERFALL HOLE
// =========================================
// Inspired by the primeval swamp and waterfall areas at Dino Park
// Miniature Golf, Kata Beach, Phuket (dinopark.com).
//
// Features: irregular swamp course, bone bridge, waterfall,
// low-poly Brachiosaurus, small Pac-Man, reeds, mud patches.
//
// Controls:
//   1=ball  2=log  3=ramp  4=hole  5=pacman  6=dinosaur  0=deselect
//   WASD=move  +/-=scale  Q/E=rotate  Space=momentum
//   Enter=wireframe  P=screenshot  Esc=quit

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <string>
#include <ctime>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "shader.hpp"
#include "Shape.h"
#include "Square.h"
#include "Triangle.h"
#include "Polygon.h"
#include "Vector.h"
#include "Matrix.h"

using namespace std;

const int WIN_W = 1000;
const int WIN_H = 1000;

struct RGB { float r, g, b; };
RGB pastel(RGB c) {
    float t=0.5f;
    return {c.r+t*(1-c.r),c.g+t*(1-c.g),c.b+t*(1-c.b)};
}
enum class Prim { TRIFAN, TRILIST };

struct ShapeRenderer {
    Shape<3>* shape;
    GLuint vao, vbo;
    RGB normalColour, selectedColour;
    Prim prim;
    int vertexCount;

    void upload(bool wireframe, bool selected) {
        float* pts = shape->getPoints();
        RGB col = selected ? selectedColour : normalColour;
        vector<float> buf;
        if (!wireframe) {
            for (int i=0;i<vertexCount;i++){
                buf.push_back(pts[i*3+0]); buf.push_back(pts[i*3+1]);
                buf.push_back(col.r); buf.push_back(col.g); buf.push_back(col.b);
            }
        } else {
            auto pushV=[&](int idx){
                buf.push_back(pts[idx*3+0]); buf.push_back(pts[idx*3+1]);
                buf.push_back(col.r); buf.push_back(col.g); buf.push_back(col.b);
            };
            auto pushEdge=[&](int a,int b){pushV(a);pushV(b);};
            if (prim==Prim::TRIFAN){
                for(int i=0;i<vertexCount-2;i++){pushEdge(0,i+1);pushEdge(i+1,i+2);pushEdge(i+2,0);}
            } else {
                for(int i=0;i<vertexCount/3;i++){pushEdge(i*3,i*3+1);pushEdge(i*3+1,i*3+2);pushEdge(i*3+2,i*3);}
            }
        }
        delete[] pts;
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER,buf.size()*sizeof(float),buf.data(),GL_STREAM_DRAW);
        glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)(2*sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }

    void draw(bool wireframe, bool selected){
        upload(wireframe,selected);
        glBindVertexArray(vao);
        if(!wireframe){
            GLenum mode=(prim==Prim::TRIFAN)?GL_TRIANGLE_FAN:GL_TRIANGLES;
            glDrawArrays(mode,0,vertexCount);
        } else {
            int tc=(prim==Prim::TRIFAN)?vertexCount-2:vertexCount/3;
            glDrawArrays(GL_LINES,0,tc*6);
        }
        glBindVertexArray(0);
    }
};

vector<ShapeRenderer> renderers;
int selectedIdx=-1;
bool wireframe=false;
double lastEnter=0.0;

int idxBall=-1,idxLog=-1,idxRamp=-1,idxHole=-1,idxPac=-1,idxDino=-1;

bool momentumCharged=false;
const float MOMENTUM_MULT=12.0f;

struct CircleBound{float x,y,r;};
struct AABBBound{float cx,cy,hw,hh;};
vector<CircleBound> circleBounds;
vector<AABBBound> aabbBounds;

const float GRASS_RADIUS=0.84f;
const float BALL_RADIUS=0.038f;

map<int,vector<int>> groups;

Vector<3> v3(float x,float y){
    Vector<3> v; v[0]=x; v[1]=y; v[2]=1.0f; return v;
}

int addShape(Shape<3>* s,Prim prim,int vc,RGB col){
    ShapeRenderer sr;
    sr.shape=s; sr.prim=prim; sr.vertexCount=vc;
    sr.normalColour=col; sr.selectedColour=pastel(col);
    glGenVertexArrays(1,&sr.vao); glGenBuffers(1,&sr.vbo);
    int idx=(int)renderers.size();
    renderers.push_back(sr);
    return idx;
}

void applyToGroup(int lead,char op,char arg){
    auto it=groups.find(lead);
    vector<int>* m=(it!=groups.end())?&it->second:nullptr;
    auto apply=[&](int idx){
        Shape<3>* s=renderers[idx].shape;
        if(op=='m')s->move(arg);
        else if(op=='s')s->scale(arg);
        else if(op=='r')s->rotate(arg);
    };
    if(m) for(int i:*m)apply(i); else apply(lead);
}

void getBallCentre(float&bx,float&by){
    float c[3]={0,0,0};
    renderers[idxBall].shape->midPoint(c);
    bx=c[0]; by=c[1];
}

bool ballCollides(){
    float bx,by; getBallCentre(bx,by);
    if(sqrtf(bx*bx+by*by)+BALL_RADIUS>GRASS_RADIUS)return true;
    for(auto&cb:circleBounds){float dx=bx-cb.x,dy=by-cb.y;if(sqrtf(dx*dx+dy*dy)<BALL_RADIUS+cb.r)return true;}
    for(auto&ab:aabbBounds){if(bx>ab.cx-ab.hw-BALL_RADIUS&&bx<ab.cx+ab.hw+BALL_RADIUS&&by>ab.cy-ab.hh-BALL_RADIUS&&by<ab.cy+ab.hh+BALL_RADIUS)return true;}
    return false;
}

bool moveBallSafe(char dir){
    Shape<3>* ball=renderers[idxBall].shape;
    int steps=momentumCharged?(int)MOMENTUM_MULT:1;
    momentumCharged=false;
    char undo=(dir=='w'?'s':dir=='s'?'w':dir=='a'?'d':'a');
    for(int i=0;i<steps;i++){ball->move(dir);if(ballCollides()){ball->move(undo);return false;}}
    return true;
}

void saveScreenshot(){
    int w=WIN_W,h=WIN_H,rb=w*3;
    vector<unsigned char> buf(rb*h);
    glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,buf.data());
    for(int i=0;i<w*h;i++){unsigned char t=buf[i*3];buf[i*3]=buf[i*3+2];buf[i*3+2]=t;}
    int stride=(rb+3)&~3,pad=stride-rb,pds=stride*h,fs=54+pds;
    unsigned char hdr[54]={};
    hdr[0]='B';hdr[1]='M';
    hdr[2]=fs&0xFF;hdr[3]=(fs>>8)&0xFF;hdr[4]=(fs>>16)&0xFF;hdr[5]=(fs>>24)&0xFF;
    hdr[10]=54;hdr[14]=40;
    hdr[18]=w&0xFF;hdr[19]=(w>>8)&0xFF;hdr[20]=(w>>16)&0xFF;hdr[21]=(w>>24)&0xFF;
    hdr[22]=h&0xFF;hdr[23]=(h>>8)&0xFF;hdr[24]=(h>>16)&0xFF;hdr[25]=(h>>24)&0xFF;
    hdr[26]=1;hdr[28]=24;
    time_t now=time(nullptr);tm* t=localtime(&now);
    char fn[64];snprintf(fn,sizeof(fn),"screenshot_%02d%02d%02d.bmp",t->tm_hour,t->tm_min,t->tm_sec);
    FILE* f=fopen(fn,"wb");if(!f){cerr<<"Cannot open "<<fn<<"\n";return;}
    fwrite(hdr,1,54,f);
    unsigned char p[3]={0,0,0};
    for(int row=0;row<h;row++){fwrite(buf.data()+row*rb,1,rb,f);if(pad>0)fwrite(p,1,pad,f);}
    fclose(f);cout<<"Screenshot: "<<fn<<"\n";
}

void buildScene(){
    // ---- COLOURS: 16 distinct colours for the Dino Park swamp theme ----
    RGB cDarkBG   ={0.05f,0.06f,0.04f}; // near-black jungle
    RGB cRock     ={0.35f,0.30f,0.22f}; // grey-brown rock border
    RGB cBoneRing ={0.82f,0.78f,0.65f}; // ivory bone accent ring
    RGB cSwamp    ={0.12f,0.28f,0.14f}; // dark swampy green surface
    RGB cWater    ={0.05f,0.22f,0.30f}; // deep blue-green swamp water
    RGB cFall     ={0.28f,0.68f,0.90f}; // bright cerulean waterfall
    RGB cFallMist ={0.55f,0.80f,0.95f}; // pale waterfall mist
    RGB cBridge   ={0.68f,0.62f,0.50f}; // weathered wooden planks
    RGB cBoneW    ={0.95f,0.93f,0.82f}; // bright ivory bones
    RGB cDino     ={0.62f,0.50f,0.28f}; // Brachiosaurus sandy brown
    RGB cDinoD    ={0.48f,0.38f,0.18f}; // darker dino detail
    RGB cMud      ={0.28f,0.18f,0.10f}; // dark mud patch
    RGB cReed     ={0.15f,0.62f,0.18f}; // bright swamp reed green
    RGB cStart    ={0.88f,0.55f,0.08f}; // amber starting mat
    RGB cHoleCol  ={0.55f,0.05f,0.70f}; // deep purple hole
    RGB cBall     ={0.98f,0.98f,0.98f}; // white ball
    RGB cPac      ={0.95f,0.88f,0.00f}; // pacman yellow
    RGB cEye      ={0.04f,0.04f,0.04f}; // near black
    RGB cLog      ={0.50f,0.28f,0.10f}; // obstacle log
    RGB cRamp     ={0.75f,0.15f,0.10f}; // obstacle ramp

    // -- BACKGROUND: full screen near-black jungle night --
    addShape(new Square<3>(v3(0,0),2.2f,2.2f), Prim::TRIFAN,4,cDarkBG);

    // -- ROCKY BORDER: irregular 11-gon, asymmetric for organic feel --
    //addShape(new Polygon<3>(v3(-0.04f,0.02f),1.08f,11,0.55f), Prim::TRIFAN,13,cRock);

    addShape(new Square<3>(v3(-1.4,-1.4),2.2f,9.2f), Prim::TRIFAN,4,cRock);

    // -- BONE/IVORY ACCENT RING: thin ring just inside rock --
    addShape(new Polygon<3>(v3(-0.04f,0.02f),1.03f,11,0.55f), Prim::TRIFAN,13,cBoneRing);

    // -- SWAMP GRASS SURFACE: irregular 13-gon, organic lump shape --
    addShape(new Polygon<3>(v3(-0.05f,0.00f),0.94f,13,0.50f), Prim::TRIFAN,15,cSwamp);

    // -- SWAMP POOLS: partial polygon fans = irregular blob shapes --
    // Main pool (central-left) - largest hazard
    addShape(new Polygon<3>(v3(-0.22f,0.08f),0.38f,14,0.3f),  Prim::TRIFAN,13,cWater);
    // Tendril extending down-left
    addShape(new Polygon<3>(v3(-0.42f,-0.15f),0.20f,10,0.8f), Prim::TRIFAN,9,cWater);
    // Secondary pool near hole (top-right)
    addShape(new Polygon<3>(v3(0.42f,0.38f),0.22f,12,1.1f),   Prim::TRIFAN,10,cWater);

    // -- ROCKY OUTCROPS: islands in the swamp --
    addShape(new Polygon<3>(v3(-0.18f,0.22f),0.12f,7,0.9f),   Prim::TRIFAN,8,cRock);
    addShape(new Polygon<3>(v3(-0.35f,-0.05f),0.07f,6),        Prim::TRIFAN,8,cRock);

    // -- WATERFALL: cascading blue rectangles, left edge --
    addShape(new Square<3>(v3(-0.72f,0.65f),0.12f,0.07f),  Prim::TRIFAN,4,cFall);
    addShape(new Square<3>(v3(-0.72f,0.42f),0.28f,0.09f),  Prim::TRIFAN,4,cFall);
    addShape(new Square<3>(v3(-0.70f,0.20f),0.22f,0.13f),  Prim::TRIFAN,4,cFall);
    addShape(new Square<3>(v3(-0.68f,0.02f),0.14f,0.18f),  Prim::TRIFAN,4,cFall);
    // Mist at base
    addShape(new Polygon<3>(v3(-0.65f,-0.08f),0.16f,12),   Prim::TRIFAN,14,cFallMist);
    addShape(new Polygon<3>(v3(-0.55f,-0.04f),0.04f,8),    Prim::TRIFAN,10,cFallMist);
    addShape(new Polygon<3>(v3(-0.74f,-0.12f),0.03f,8),    Prim::TRIFAN,10,cFallMist);

    // -- BRIDGE: 3 plank rectangles crossing the main swamp --
    float by=0.08f;
    addShape(new Square<3>(v3(-0.33f,by),0.055f,0.18f),    Prim::TRIFAN,4,cBridge);
    addShape(new Square<3>(v3(-0.18f,by),0.055f,0.18f),    Prim::TRIFAN,4,cBridge);
    addShape(new Square<3>(v3(-0.03f,by),0.055f,0.18f),    Prim::TRIFAN,4,cBridge);

    // -- BONE PILLARS: 4 large bone circles flanking bridge entrance/exit --
    // Bones are large circles (cross-section of dinosaur femur/tibia)
    // Left entry pair
    addShape(new Polygon<3>(v3(-0.44f,by+0.10f),0.065f,12),Prim::TRIFAN,14,cBoneW);
    addShape(new Polygon<3>(v3(-0.44f,by-0.10f),0.065f,12),Prim::TRIFAN,14,cBoneW);
    addShape(new Square<3>(v3(-0.44f,by),0.10f,0.045f),     Prim::TRIFAN,4,cBoneW);
    // Right exit pair
    addShape(new Polygon<3>(v3(0.08f,by+0.10f),0.065f,12), Prim::TRIFAN,14,cBoneW);
    addShape(new Polygon<3>(v3(0.08f,by-0.10f),0.065f,12), Prim::TRIFAN,14,cBoneW);
    addShape(new Square<3>(v3(0.08f,by),0.10f,0.045f),      Prim::TRIFAN,4,cBoneW);
    // Horizontal crossbars connecting left to right
    addShape(new Square<3>(v3(-0.18f,by+0.10f),0.022f,0.54f),Prim::TRIFAN,4,cBoneW);
    addShape(new Square<3>(v3(-0.18f,by-0.10f),0.022f,0.54f),Prim::TRIFAN,4,cBoneW);
    // Mid-span knob circles
    addShape(new Polygon<3>(v3(-0.18f,by+0.10f),0.035f,10), Prim::TRIFAN,12,cBoneW);
    addShape(new Polygon<3>(v3(-0.18f,by-0.10f),0.035f,10), Prim::TRIFAN,12,cBoneW);

    // -- MUD PATCHES: dark irregular circles on swamp grass --
    addShape(new Polygon<3>(v3(0.20f,-0.22f),0.065f,7,0.4f),Prim::TRIFAN,8,cMud);
    addShape(new Polygon<3>(v3(0.48f,0.05f),0.045f,7,1.0f), Prim::TRIFAN,8,cMud);
    addShape(new Polygon<3>(v3(-0.52f,0.50f),0.038f,6),      Prim::TRIFAN,8,cMud);

    // -- SWAMP REEDS: thin triangles at water edges --
    // Left swamp cluster
    addShape(new Triangle<3>(v3(-0.48f,0.28f),v3(-0.52f,0.08f),v3(-0.44f,0.08f)),Prim::TRILIST,3,cReed);
    addShape(new Triangle<3>(v3(-0.38f,0.32f),v3(-0.42f,0.14f),v3(-0.34f,0.14f)),Prim::TRILIST,3,cReed);
    addShape(new Triangle<3>(v3(-0.58f,0.20f),v3(-0.62f,0.04f),v3(-0.54f,0.04f)),Prim::TRILIST,3,cReed);
    // Waterfall base cluster
    addShape(new Triangle<3>(v3(-0.60f,0.08f),v3(-0.64f,-0.10f),v3(-0.56f,-0.10f)),Prim::TRILIST,3,cReed);
    addShape(new Triangle<3>(v3(-0.54f,0.12f),v3(-0.57f,-0.04f),v3(-0.51f,-0.04f)),Prim::TRILIST,3,cReed);
    // Ferns near waterfall base (wider, drooping)
    addShape(new Triangle<3>(v3(-0.76f,-0.05f),v3(-0.86f,-0.22f),v3(-0.66f,-0.22f)),Prim::TRILIST,3,cReed);
    addShape(new Triangle<3>(v3(-0.70f,-0.08f),v3(-0.80f,-0.22f),v3(-0.60f,-0.22f)),Prim::TRILIST,3,cReed);
    // Right pool cluster
    addShape(new Triangle<3>(v3(0.52f,0.50f),v3(0.49f,0.34f),v3(0.55f,0.34f)),Prim::TRILIST,3,cReed);
    addShape(new Triangle<3>(v3(0.44f,0.52f),v3(0.41f,0.38f),v3(0.47f,0.38f)),Prim::TRILIST,3,cReed);

    // -- OBSTACLES --
    idxLog=addShape(new Square<3>(v3(0.25f,0.12f),0.055f,0.34f),Prim::TRIFAN,4,cLog);
    addShape(new Square<3>(v3(-0.12f,-0.38f),0.05f,0.28f),Prim::TRIFAN,4,cLog);
    aabbBounds.push_back({0.25f,0.12f,0.17f,0.0275f});

    idxRamp=addShape(
        new Triangle<3>(v3(0.38f,0.22f),v3(0.24f,0.00f),v3(0.52f,0.00f)),
        Prim::TRILIST,3,cRamp);
    addShape(
        new Triangle<3>(v3(-0.10f,-0.48f),v3(-0.22f,-0.66f),v3(0.02f,-0.66f)),
        Prim::TRILIST,3,cRamp);


    // -- LOW-POLY BRACHIOSAURUS: lower-right, outside course boundary --
    float dx=0.72f,dy=-0.55f;

    idxDino=addShape(new Polygon<3>(v3(dx,dy+0.08f),0.20f,8,0.0f),      Prim::TRIFAN,9,cDino);
    addShape(new Polygon<3>(v3(dx+0.02f,dy+0.04f),0.14f,6,0.1f),         Prim::TRIFAN,7,cDinoD);
    addShape(new Square<3>(v3(dx-0.08f,dy+0.24f),0.24f,0.07f),           Prim::TRIFAN,4,cDino);
    addShape(new Triangle<3>(v3(dx-0.04f,dy+0.16f),v3(dx-0.14f,dy+0.16f),v3(dx+0.04f,dy+0.08f)),Prim::TRILIST,3,cDino);
    addShape(new Triangle<3>(v3(dx-0.21f,dy+0.38f),v3(dx-0.06f,dy+0.42f),v3(dx-0.06f,dy+0.30f)),Prim::TRILIST,3,cDino);
    addShape(new Polygon<3>(v3(dx-0.14f,dy+0.38f),0.012f,8),              Prim::TRIFAN,10,cEye);
    addShape(new Triangle<3>(v3(dx+0.18f,dy+0.12f),v3(dx+0.18f,dy+0.04f),v3(dx+0.40f,dy+0.08f)),Prim::TRILIST,3,cDino);
    // Legs
    addShape(new Square<3>(v3(dx-0.10f,dy-0.10f),0.18f,0.04f),Prim::TRIFAN,4,cDinoD);
    addShape(new Square<3>(v3(dx-0.02f,dy-0.10f),0.18f,0.04f),Prim::TRIFAN,4,cDinoD);
    addShape(new Square<3>(v3(dx+0.09f,dy-0.10f),0.18f,0.04f),Prim::TRIFAN,4,cDinoD);
    addShape(new Square<3>(v3(dx+0.16f,dy-0.10f),0.18f,0.04f),Prim::TRIFAN,4,cDinoD);
    // Back spines
    addShape(new Triangle<3>(v3(dx-0.05f,dy+0.30f),v3(dx-0.08f,dy+0.24f),v3(dx-0.02f,dy+0.24f)),Prim::TRILIST,3,cDinoD);
    addShape(new Triangle<3>(v3(dx+0.03f,dy+0.28f),v3(dx+0.00f,dy+0.22f),v3(dx+0.06f,dy+0.22f)),Prim::TRILIST,3,cDinoD);
    addShape(new Triangle<3>(v3(dx+0.10f,dy+0.26f),v3(dx+0.07f,dy+0.20f),v3(dx+0.13f,dy+0.20f)),Prim::TRILIST,3,cDinoD);
    // Group dino: 15 parts starting at idxDino
    {vector<int> g; for(int i=idxDino;i<idxDino+15;i++)g.push_back(i); groups[idxDino]=g;}

    // -- STARTING MAT: amber square, lower-left --
    addShape(new Square<3>(v3(-0.60f,-0.68f),0.10f,0.14f),Prim::TRIFAN,4,cStart);

    // -- GOLF HOLE: deep purple circle, top-right --
    idxHole=addShape(new Polygon<3>(v3(0.62f,0.60f),0.07f,60),Prim::TRIFAN,62,cHoleCol);

    // -- GOLF BALL: starts on starting mat --
    idxBall=addShape(new Polygon<3>(v3(-0.60f,-0.68f),0.038f,60),Prim::TRIFAN,62,cBall);

    // Collision bounds
    circleBounds.push_back({-0.18f, 0.22f, 0.12f});  // rocky outcrop
    circleBounds.push_back({-0.35f,-0.05f, 0.07f});  // small rock
    circleBounds.push_back({-0.22f, 0.08f, 0.30f});  // main swamp pool
    circleBounds.push_back({ 0.42f, 0.38f, 0.18f});  // right pool
}

void selectShape(int idx){
    if(selectedIdx>=0){
        auto it=groups.find(selectedIdx);
        if(it!=groups.end())for(int i:it->second)renderers[i].shape->deselect();
        else renderers[selectedIdx].shape->deselect();
    }
    selectedIdx=idx;
    if(idx>=0){
        auto it=groups.find(idx);
        if(it!=groups.end())for(int i:it->second)renderers[i].shape->select();
        else renderers[idx].shape->select();
    }
}

const char* getError(){const char* d;glfwGetError(&d);return d;}
inline void startUpGLFW(){glewExperimental=true;if(!glfwInit())throw getError();}
inline void startUpGLEW(){glewExperimental=true;if(glewInit()!=GLEW_OK){glfwTerminate();throw getError();}}
inline GLFWwindow* setUp(){
    startUpGLFW();
    glfwWindowHint(GLFW_SAMPLES,4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* w=glfwCreateWindow(WIN_W,WIN_H,"u23718146",NULL,NULL);
    if(!w){cout<<getError();glfwTerminate();throw "Window failed";}
    glfwMakeContextCurrent(w);
    startUpGLEW();
    return w;
}

void keyCallback(GLFWwindow* win,int key,int,int action,int){
    if(action==GLFW_RELEASE)return;
    if(action==GLFW_PRESS){
        switch(key){
            case GLFW_KEY_1:selectShape(idxBall);return;
            case GLFW_KEY_2:selectShape(idxLog);return;
            case GLFW_KEY_3:selectShape(idxRamp);return;
            case GLFW_KEY_4:selectShape(idxHole);return;
            case GLFW_KEY_5:selectShape(idxPac);return;
            case GLFW_KEY_6:selectShape(idxDino);return;
            case GLFW_KEY_0:selectShape(-1);return;
            case GLFW_KEY_ENTER:{double n=glfwGetTime();if(n-lastEnter>0.25){wireframe=!wireframe;lastEnter=n;}return;}
            case GLFW_KEY_P:saveScreenshot();return;
            case GLFW_KEY_SPACE:if(selectedIdx==idxBall)momentumCharged=true;return;
            case GLFW_KEY_ESCAPE:glfwSetWindowShouldClose(win,GLFW_TRUE);return;
            default:break;
        }
    }
    if(selectedIdx<0)return;
    bool isBall=(selectedIdx==idxBall);
    switch(key){
        case GLFW_KEY_W:if(isBall)moveBallSafe('w');else applyToGroup(selectedIdx,'m','w');break;
        case GLFW_KEY_S:if(isBall)moveBallSafe('s');else applyToGroup(selectedIdx,'m','s');break;
        case GLFW_KEY_A:if(isBall)moveBallSafe('a');else applyToGroup(selectedIdx,'m','a');break;
        case GLFW_KEY_D:if(isBall)moveBallSafe('d');else applyToGroup(selectedIdx,'m','d');break;
        case GLFW_KEY_EQUAL:case GLFW_KEY_KP_ADD:applyToGroup(selectedIdx,'s','+');break;
        case GLFW_KEY_MINUS:case GLFW_KEY_KP_SUBTRACT:applyToGroup(selectedIdx,'s','-');break;
        case GLFW_KEY_LEFT:case GLFW_KEY_Q:applyToGroup(selectedIdx,'r','l');break;
        case GLFW_KEY_RIGHT:case GLFW_KEY_E:applyToGroup(selectedIdx,'r','r');break;
        default:break;
    }
}

int main(){
    GLFWwindow* window;
    try{window=setUp();}
    catch(const char* e){cout<<e<<endl;return -1;}

    glfwSetInputMode(window,GLFW_STICKY_KEYS,GL_TRUE);
    glfwSetKeyCallback(window,keyCallback);

    GLuint prog=LoadShaders("vertex.glsl","fragment.glsl");
    if(!prog){cerr<<"Shader load failed\n";glfwTerminate();return -1;}

    buildScene();

    // Jungle night: very dark background (distinct from all shapes)
    glClearColor(0.03f,0.04f,0.02f,1.0f);

    cout<<"DINO PARK PHUKET - SWAMP WATERFALL HOLE\n"
        <<"  1:ball  2:log  3:ramp  4:hole  5:pacman  6:dinosaur  0:deselect\n"
        <<"  WASD:move  +/-:scale  Q/E:rotate  Space:momentum  P:screenshot\n"
        <<"  Enter:wireframe  Esc:quit\n";

    while(!glfwWindowShouldClose(window)){
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(prog);
        for(int i=0;i<(int)renderers.size();i++)
            renderers[i].draw(wireframe,i==selectedIdx);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    for(auto&r:renderers){delete r.shape;glDeleteVertexArrays(1,&r.vao);glDeleteBuffers(1,&r.vbo);}
    glDeleteProgram(prog);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}