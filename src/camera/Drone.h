#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>

class Drone
{
public:
    Drone();
    ~Drone();

    void init(GLuint shaderProgram);

    void update(GLFWwindow* window, float dt, float mouseDx, float mouseDy);

    // Draw the drone body. Only does anything in 3rd-person mode 
    void draw(const glm::mat4& view,
              const glm::mat4& proj,
              const glm::vec3& sunDir,
              const glm::vec3& camPos);

    void drawDepth(GLuint depthShader); 

    void cleanup();

    glm::mat4 view() const { return mView; }
    glm::mat4 projection(float aspect) const;

    glm::vec3 eyePos() const { return mEyePos; }

    void frameOn(const glm::vec3& target, float radius);

    void onKey(int key, int action);

    glm::vec3 position() const { return mPos; }
    float     yaw()      const { return mYaw;   }
    float     pitch()    const { return mPitch; }
    float     roll()     const { return mRoll;  }
    bool      thirdPerson() const { return mThirdPerson; }

private:
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 color;
    };
    void buildBody(float radius, float halfHeight, const glm::vec3& colour);
    void buildArm(const glm::vec3& tipDir,
                  float armLen, float armHalfW, float armHalfH,
                  float motorR, float motorH,
                  float propLen, float propHalfW,
                  const glm::vec3& armColour,
                  const glm::vec3& motorColour,
                  const glm::vec3& propColour,
                  const glm::vec3& ledColour,
                  int   propIndex);  
    void buildCylinderY(const glm::vec3& centre, float r, float h, int segs,
                        const glm::vec3& colour);
    void buildSphere(const glm::vec3& centre, float r,
                     int latSegs, int lonSegs,
                     const glm::vec3& colour);
    void buildTorusY(const glm::vec3& centre, float R, float r,
                     int majorSegs, int minorSegs,
                     const glm::vec3& colour);
    void buildBox(const glm::vec3& mn, const glm::vec3& mx,
                  const glm::vec3& colour);
    void buildLED(const glm::vec3& centre, float halfSize,
                  const glm::vec3& colour);
    void pushQuad(const glm::vec3& a, const glm::vec3& b,
                  const glm::vec3& c, const glm::vec3& d,
                  const glm::vec3& n, const glm::vec3& colour);
    void pushTri(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c,
                 const glm::vec3& n, const glm::vec3& colour);
    std::vector<Vertex>       mVertices;
    std::vector<unsigned int> mIndices;
    struct PropMeta {
        GLuint    indexOffset;
        GLuint    indexCount;
        glm::vec3 hubLocal;
    };
    PropMeta mProps[4];

    GLuint mTotalIndexCount;   

    GLuint  mVAO;
    GLuint  mVBO;
    GLuint  mEBO;
    bool    mInitialised;

    GLuint mProgram;
    GLint  mLocMVP;
    GLint  mLocModel;
    GLint  mLocNormalMat;
    GLint  mLocLightDir;
    GLint  mLocLightColor;
    GLint  mLocAmbient;
    GLint  mLocViewPos;

    glm::vec3 mPos;         
    float     mYaw;          
    float     mPitch;     
    float     mRoll;      

    float     mCamYawOffset;   
    float     mCamPitchOffset; 

    float     mPropSpin;    

    bool      mThirdPerson;  // V key toggle
    bool      mZoom;         // Z key held

    glm::mat4 mView;
    glm::vec3 mEyePos;

    glm::mat4 bodyModel() const;

    // Forward / right / up unit vectors for the drone body in world space,
    // derived from yaw + pitch + roll
    glm::vec3 bodyForward() const;
    glm::vec3 bodyRight() const;
    glm::vec3 bodyUp() const;
};