#pragma once
#include "push-constant.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace nitro::geometry
{
    class MeshTransformation
    {
    public:
        void translate(glm::vec3 translate)
        {
            m_translate += glm::translate(glm::mat4(1.0f), translate);
        };
        void scale(glm::vec3 scale)
        {
            m_scale += glm::scale(glm::mat4(1.0f), scale);
        };
        void rotate(float angle, glm::vec3 axis)
        {
            m_rotate += glm::rotate(glm::mat4(1.0f), angle, axis);
        };
        PushConstant getTransform()
        {
            PushConstant pc;
            pc.model = m_translate * m_rotate * m_scale;
            pc.applyNormalMatrix();
            return pc;
        };

    private:
        glm::mat4 m_translate = glm::mat4(1.0f);
        glm::mat4 m_scale = glm::mat4(1.0f);
        glm::mat4 m_rotate = glm::mat4(1.0f);
    };
}