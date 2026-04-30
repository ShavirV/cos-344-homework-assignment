#include "../../include/Triangle.h"

Triangle::Triangle (float x1, float y1, float x2, float y2, float x3, float y3, float r, float g, float b, float a)
{
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = a;

    vertices[0] = x1;
    vertices[1] = y1;

    vertices[2] = x2;
    vertices[3] = y2;

    vertices[4] = x3;
    vertices[5] = y3;

    cx = (vertices[0] + vertices[2] + vertices[4]) / 3.0f;
    cy = (vertices[1] + vertices[3] + vertices[5]) / 3.0f;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0); // params (attribute location, 2 floats per vertex, float data type, dont normalize data, jump 2 floats to get to next vertex, start reading from byte 0)
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

Triangle::~Triangle()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);//cleans up buffers
}

void Triangle::draw(GLuint shaderProgram, bool wireframe)
{
    GLint colourLocation = glGetUniformLocation(shaderProgram, "colour");

    glUseProgram(shaderProgram);

    if (select)
    {
        float pastR = r * 0.7f + 0.3f;
        float pastB = b * 0.7f + 0.3f;
        float pastG = g * 0.7f + 0.3f;
        glUniform4f(colourLocation, pastR, pastG, pastB, a);
    }
    else
    {
        glUniform4f(colourLocation, r, g, b, a);
    }

    glBindVertexArray(VAO);

    if (wireframe)
    {
        float lineVertices[] = {
            vertices[0], vertices[1],  
            vertices[2], vertices[3],
            vertices[2], vertices[3],  
            vertices[4], vertices[5],
            vertices[4], vertices[5],  
            vertices[0], vertices[1]
        };

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glDrawArrays(GL_LINES, 0, 6);

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    }
    else
    {
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    glBindVertexArray(0);
}

void Triangle::setRGBA(float r, float g, float b, float a)
{
    this->r = r;
    this->b = b;
    this->g = g;
    this->a = a;
}

void Triangle::selected(bool selected)
{
    this->select = selected;
}

bool Triangle::isSelected()
{
    return select;
}

void Triangle::translate(float x, float y)
{
    if (!inBounds(x,y))
    {
        return;
    }

    for (int i = 0; i< 6; i++)
    {
        if (i%2 == 0)
        {
            vertices[i] += x;
        }
        else
        {
            vertices[i]+= y;
        }
    }

    cx+= x;
    cy+=y;

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

bool Triangle::inBounds(float dx, float dy)
{
    for (int i =0; i< 6; i+=2)
    {
        if (vertices[i] + dx > 0.85f || vertices[i] + dx < -0.85f)
        {
            return false;
        }

        if (vertices[i+1] + dy > 0.85f || vertices[i+1] + dy < -0.85f)
        {
           return false;
        }
    }

    return true;
}

void Triangle::rotateAround(float angle, float cx, float cy)
{
    float cosA = cos(angle);
    float sinA = sin(angle);

    for (int i =0; i< 6; i+=2)
    {
        float x = vertices[i] - cx;
        float y = vertices[i+1] - cy;

        float newX = x*cosA - y*sinA;
        float newY = x*sinA + y*cosA;

        vertices[i] = newX +cx;
        vertices[i+1] = newY + cy;
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Triangle::rotate(float angle)
{
    rotateAround(angle, cx, cy);
}

void Triangle::scaleAround(float ds, float cx, float cy)
{
    for (int i = 0; i < 6; i += 2)
    {
        float x = vertices[i]   - cx;
        float y = vertices[i+1] - cy;

        float newX = x * ds;
        float newY = y * ds;

        vertices[i]   = newX + cx;
        vertices[i+1] = newY + cy;
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Triangle::scale(float ds)
{
    scaleAround(ds, cx, cy);
}