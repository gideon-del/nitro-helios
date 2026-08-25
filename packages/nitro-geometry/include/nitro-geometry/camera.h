#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "ray.h"
namespace nitro::geometry
{
    struct OrbitalCamera
    {

        glm::vec3 getEye()
        {
            glm::vec3 eye;

            eye.x = m_target.x + m_radius * std::sin(m_theta) * std::cos(m_phi);
            eye.y = m_target.y + m_radius * std::cos(m_theta);
            eye.z = m_target.z + m_radius * std::sin(m_theta) * std::sin(m_phi);

            return eye;
        }

        glm::mat4 getView()
        {
            return glm::lookAt(getEye(), m_target, glm::vec3(0.0f, 1.0f, 0.0f));
        }

        void onMouseMove(float dx, float dy)
        {
            float sensitivity = 0.01;
            m_phi -= dx * sensitivity;
            m_theta -= dy * sensitivity;
            m_theta = glm::clamp(m_theta, 0.1f, glm::pi<float>() - 0.1f);
            calculateView();
            calculateInvViewProj();
            calculateViewProj();
        }

        void onScroll(float delta)
        {
            float sensitivity = 0.01;
            m_radius -= delta * sensitivity;
            m_radius = glm::max(m_radius, 0.1f);
            calculateView();
            calculateInvViewProj();
            calculateViewProj();
        }

        void moveForward(float amount)
        {
            glm::vec3 forward = m_target - getEye();
            forward.y = 0.0f;
            forward = glm::normalize(forward);

            m_target += forward * amount;
            calculateView();
            calculateInvViewProj();
            calculateViewProj();
        }

        void moveRight(float amount)
        {
            glm::vec3 forward = m_target - getEye();
            forward.y = 0.0f;
            forward = glm::normalize(forward);
            glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

            m_target += right * amount;
            calculateView();
            calculateInvViewProj();
            calculateViewProj();
        }

        void setViewPort(float width, float height)
        {
            m_width = width;
            m_height = height;

            calculateProj();
            calculateInvViewProj();
            calculateViewProj();
        }

        void setTheta(float theta)
        {
            m_theta = theta;

            calculateView();
            calculateInvViewProj();
            calculateViewProj();
        };
        void setPhi(float phi)
        {
            m_phi = phi;

            calculateView();
            calculateInvViewProj();
            calculateViewProj();
        };
        void setRadius(float radius)
        {
            m_radius = radius;

            calculateView();
            calculateInvViewProj();
            calculateViewProj();
        };

        void setLens(float fov, float near, float far)
        {
            m_fov = glm::radians(fov);
            m_near = near;
            m_far = far;

            calculateProj();
            calculateInvViewProj();
            calculateViewProj();
        }

        void setFlipY(bool flipY)
        {

            m_flipY = flipY;

            calculateProj();
            calculateInvViewProj();
            calculateViewProj();
        }

        Ray reconstructRayFromUV(glm::vec2 uv)
        {
            glm::vec2 ndc{uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f};
            if (m_flipY)
            {
                ndc.y = uv.y * 2.0f - 1.0f;
            }
            glm::vec4 pn = m_invViewProj * glm::vec4(ndc, 0.0, 1.0);
            glm::vec4 pf = m_invViewProj * glm::vec4(ndc, 1.0, 1.0);

            glm::vec3 pNear = glm::vec3(pn) / pn.w;
            glm::vec3 pFar = glm::vec3(pf) / pf.w;
            glm::vec3 direction = glm::normalize(pFar - pNear);

            Ray ray{pNear, direction};
            ray.tMax = glm::length(pFar - pNear);

            return ray;
        };
        void focus(const glm::vec3 &center, float extent)
        {
            m_target = center;
            m_radius = extent * 2;

            calculateView();
            calculateViewProj();
            calculateInvViewProj();
        };
        const float near() const
        {
            return m_near;
        }
        const float far() const
        {
            return m_far;
        }
        const float fov() const
        {
            return m_fov;
        }
        const float theta() const
        {
            return m_theta;
        }
        const float phi() const
        {
            return m_phi;
        }
        const float radius() const
        {
            return m_radius;
        }
        const float width() const
        {
            return m_width;
        }
        const float height() const
        {
            return m_height;
        }
        const glm::mat4 &proj() const
        {
            return m_proj;
        }
        const glm::mat4 &view() const
        {
            return m_view;
        }
        const glm::mat4 &viewProj() const
        {
            return m_viewProj;
        }
        const glm::mat4 &invViewProj() const
        {
            return m_invViewProj;
        }
        bool flipY() const { return m_flipY; }

    private:
        float m_theta = glm::radians(45.0f);
        float m_phi = glm::radians(60.0f);
        float m_radius = 10.0f;

        glm::vec3 m_target{0.0f, 0.0f, 0.0f};

        float m_width = 1.0f, m_height = 1.0f;
        float m_fov = 60.0f;

        float m_near = 0.1f, m_far = 100.0f;
        glm::mat4 m_view;
        glm::mat4 m_proj;
        glm::mat4 m_invViewProj;
        glm::mat4 m_viewProj;
        bool m_flipY = false;

        void calculateProj()
        {
            float aspect = m_width / m_height;
            m_proj = glm::perspectiveRH_ZO(m_fov, aspect, m_near, m_far);
            if (m_flipY)
            {
                m_proj[1][1] *= -1;
            }
        }

        void calculateViewProj()
        {
            m_viewProj = m_proj * m_view;
        };

        void calculateInvViewProj()
        {
            m_invViewProj = glm::inverse(m_proj * m_view);
        }

        void calculateView()
        {
            m_view = glm::lookAt(getEye(), m_target, glm::vec3(0.0f, 1.0f, 0.0f));
        }
    };

}