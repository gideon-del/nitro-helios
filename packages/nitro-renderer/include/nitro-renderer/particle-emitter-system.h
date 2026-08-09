#pragma once
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/render-graph.h>

namespace nitro::renderer
{

    enum class EmitterType : uint32_t
    {
        Continuous = 0,
        Burst = 1
    };
    struct EmitterDesc
    {
        glm::vec4 position;
        glm::vec4 direction;
        glm::vec4 startColor;
        glm::vec4 endColor;
        glm::vec4 gravity;
        glm::vec4 wind = glm::vec4(0.0f);
        glm::vec4 spawnAreaExtent = glm::vec4(0.0f);
        float startSize = 1.0f;
        float endSize = 1.0f;
        float spawnRate;
        float initialSpeed;
        float speedVariance;
        float spread;
        float minLifetime;
        float maxLifetime;
        float spawnAccumulator = 0.0f;
        float drag;
        float burstCount = 500.0f;
        float hasFired = 0.0f;
        float swayAmplitude = 0.0f;
        float swayFrequency = 0.0f;
        EmitterType type = EmitterType::Continuous;
        float _pads;
    };
    class ParticleEmitterSystem
    {

    public:
        static constexpr uint32_t s_MAX_EMITTERS = 1000;
        ParticleEmitterSystem()
        {
            m_emitters.reserve(s_MAX_EMITTERS);
        }
        int addEmitter(EmitterDesc &desc, rhi::RHIBuffer *emitterBuffer)
        {
            if (m_emitters.size() >= s_MAX_EMITTERS)
                return -1;

            m_emitters.push_back(desc);
            int index = static_cast<int>(m_emitters.size() - 1);
            size_t offset = index * sizeof(EmitterDesc);
            emitterBuffer->upload(&desc, sizeof(EmitterDesc), offset);
            return index;
        };
        EmitterDesc &getEmitter(int i) { return m_emitters[i]; }
        uint32_t getEmitterCount() { return static_cast<uint32_t>(m_emitters.size()); }

        void syncFieldToGPU(rhi::RHIBuffer *emitterBuffer, uint32_t emitterIndex, size_t fieldOffset, const void *data, size_t size)
        {
            size_t offset = emitterIndex * sizeof(EmitterDesc) + fieldOffset;
            emitterBuffer->upload(data, size, offset);
        }

    private:
        std::vector<EmitterDesc> m_emitters;
    };
} // namespace nitro::renderer
