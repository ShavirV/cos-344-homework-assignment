#include "Hole3.h"
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

Hole3::Hole3() : meshCount(0)
{
    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;
    {
        Mesh* teeMat = new Mesh(Mesh::makeCuboid(1.5f, 0.08f, 1.5f, 1.0f, 0.8f, 0.0f, 1.0f));
        teeMat->translate(-12.0f, 0.1f + 0.04f, 0.0f);
        teeMat->setup();
        addMesh(teeMat);

        Mesh* teeBorder = new Mesh(Mesh::makeCuboid(1.6f, 0.02f, 1.6f, 1.0f, 1.0f, 1.0f, 1.0f));
        teeBorder->translate(-12.0f, 0.1f + 0.02f, 0.0f);
        teeBorder->setup();
        addMesh(teeBorder);
    }

    {
        Mesh* m = new Mesh(Mesh::makeCuboid(12.0f, 0.2f, 10.0f,
                                            0.35f, 0.75f, 0.25f, 1.0f));
        m->translate(-8.0f, 0.0f, 0.0f);
        m->setup();
        addMesh(m);
    }

    {
        Mesh* m = new Mesh(Mesh::makeRamp2(
        -2.0f, 0.1f, 0.0f,   // top of lower fairway
         6.0f, 1.6f, 0.0f,   // top of upper fairway
        10.0f));
        m->rotate(-90,0.0f, 1.0f, 0.0f);
        m->setup();
        addMesh(m);
    }

    {
        Mesh* m = new Mesh(Mesh::makeCuboid(12.0f, 0.2f, 10.0f,
                                            0.28f, 0.60f, 0.20f, 1.0f));
        m->translate(12.0f, 1.5f, 0.0f);
        m->setup();
        addMesh(m);
    }

    auto addRock = [&](float x, float y, float z, float sc)
    {
        Mesh* m = new Mesh(Mesh::makeRock(sc));
        m->translate(x, y, z);
        m->setup();
        addMesh(m);
    };

    addRock(2.0f, 0.75f + 1.0f, -2.5f, 0.8f);
    addRock(2.5f, 0.75f + 1.0f,  0.0f, 1.0f);
    addRock(2.0f, 0.75f + 1.0f,  2.5f, 0.7f);

    {
        Mesh* m = new Mesh(Mesh::makeCylinder(0.08f, 3.0f, 8,
                                               0.8f, 0.8f, 0.8f, 1.0f));
        m->translate(12.0f, 1.5f + 1.5f, 0.0f);   // base sits on green (y=1.5), centre at +1.5
        m->setup();
        addMesh(m);
    }

    {
        Mesh* m = new Mesh(Mesh::makeCone(0.5f, 1.0f, 8,
                                           0.9f, 0.1f, 0.1f, 1.0f));
        m->rotate(90.0f, 0.0f, 0.0f, 1.0f);        // lay cone on its side → flag shape
        m->translate(12.3f, 1.5f + 2.8f, 0.0f);
        m->setup();
        addMesh(m);
    }

    {
        Mesh* m = new Mesh(Mesh::makeCylinder(0.3f, 0.4f, 16,
                                               0.1f, 0.1f, 0.1f, 1.0f));
        m->translate(12.0f, 1.7f, 0.0f);
        m->setup();
        addMesh(m);
    }
}

Hole3::~Hole3()
{
    for (int i = 0; i < meshCount; i++)
    {
        delete meshes[i];
        meshes[i] = nullptr;
    }
}

void Hole3::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES)
        meshes[meshCount++] = m;
}

void Hole3::draw(GLuint shaderProgram, bool wireframe,
                 const glm::vec3& lightDir,
                 const glm::vec3& lightColor,
                 const glm::vec3& camPos)
{
    glUseProgram(shaderProgram);
    GLint locLD = glGetUniformLocation(shaderProgram, "uLightDir");
    GLint locVP = glGetUniformLocation(shaderProgram, "uViewPos");
    GLint locLC = glGetUniformLocation(shaderProgram, "uLightColor");
    if (locLD != -1) glUniform3fv(locLD, 1, glm::value_ptr(glm::normalize(lightDir)));
    if (locVP != -1) glUniform3fv(locVP, 1, glm::value_ptr(camPos));
    if (locLC != -1) glUniform3fv(locLC, 1, glm::value_ptr(lightColor));

    for (int i = 0; i < meshCount; i++)
        meshes[i]->draw(shaderProgram, wireframe);
}

void Hole3::drawDepth(GLuint depthShader)
{
    for (int i = 0; i < meshCount; i++)
        if (meshes[i]) meshes[i]->drawDepth(depthShader);
}

void Hole3::translate(float x, float y, float z)
{
    for (int i = 0; i < meshCount; i++){

        if (i == 3)
        {
            meshes[i]->rotate(90, 0.0f, 1.0f, 0.0f);
        }
        meshes[i]->translate(x, y, z);
        if (i == 3)
        {
            meshes[i]->rotate(-90, 0.0f, 1.0f, 0.0f);
        }


    }
}

void Hole3::rotate(float angle, float ax, float ay, float az)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->rotate(angle, ax, ay, az);
}