#pragma once
#include "push-constant.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <limits>
namespace nitro::geometry
{
    class MeshTransformation
    {
    public:
        MeshTransformation(glm::mat4 translate = glm::mat4(1.0f), glm::mat4 scale = glm::mat4(1.0f), glm::mat4 rotate = glm::mat4(1.0f)) : m_translate(translate), m_scale(scale), m_rotate(rotate) {}
        void translate(glm::vec3 translate)
        {
            m_translate *= glm::translate(glm::mat4(1.0f), translate);
        };
        void scale(glm::vec3 scale)
        {
            m_scale *= glm::scale(glm::mat4(1.0f), scale);
        };
        void rotate(float angle, glm::vec3 axis)
        {
            m_rotate *= glm::rotate(glm::mat4(1.0f), angle, axis);
        };
        void rotate(const glm::quat &qua)
        {
            m_rotate *= glm::mat4_cast(qua);
        }
        PushConstant getTransform()
        {
            PushConstant pc;
            pc.model = m_translate * m_rotate * m_scale;
            pc.applyNormalMatrix();
            return pc;
        };
        static void computeWorldAABB(const glm::mat4 &model,
                                     const glm::vec3 &localMin, const glm::vec3 &localMax,
                                     glm::vec3 &outMin, glm::vec3 &outMax)
        {
            outMin = glm::vec3(std::numeric_limits<float>::max());
            outMax = glm::vec3(-std::numeric_limits<float>::max());
            for (int i = 0; i < 8; ++i)
            {
                glm::vec3 c{(i & 1) ? localMax.x : localMin.x,
                            (i & 2) ? localMax.y : localMin.y,
                            (i & 4) ? localMax.z : localMin.z};
                glm::vec3 w = glm::vec3(model * glm::vec4(c, 1.0f));
                outMin = glm::min(outMin, w);
                outMax = glm::max(outMax, w);
            }
        }

    private:
        glm::mat4 m_translate = glm::mat4(1.0f);
        glm::mat4 m_scale = glm::mat4(1.0f);
        glm::mat4 m_rotate = glm::mat4(1.0f);
    };
}