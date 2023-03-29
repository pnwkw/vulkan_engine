#ifndef DISPLAY_TRIANGLE_RENDERER_H

#include "swapchain.h"
#include "pipeline.h"
#include "renderpass.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

class triangle_pipeline : public pipeline {
public:
    class triangle_vertex {
        glm::vec3 pos;

    public:
        explicit triangle_vertex(const glm::vec3& pos): pos(pos) {};

        static constexpr auto bindingDescriptions() {
            std::array<vk::VertexInputBindingDescription, 1> vibd {};

            vibd[0].binding = 0;
            vibd[0].stride = sizeof(triangle_vertex);
            vibd[0].inputRate = vk::VertexInputRate::eVertex;

            return vibd;
        };

        static constexpr auto attributeDescriptions() {
            std::array<vk::VertexInputAttributeDescription, 1> viad {};

            viad[0].binding = 0;
            viad[0].location = 0;
            viad[0].format = vk::Format::eR32G32B32Sfloat;
            viad[0].offset = 0; //offsetof(triangle_vertex, pos)

            return viad;
        }

    };
private:
    std::vector<vk::DynamicState> dynamicStates{};
    std::vector<vk::VertexInputBindingDescription> bindingDescription{};
    std::vector<vk::VertexInputAttributeDescription> attributeDescription{};

public:
    explicit triangle_pipeline(const engine_vk& engine);
};

class triangle_renderer {
    static constexpr std::size_t vertex_count = 3;
private:
    const engine_vk& engine;
    triangle_pipeline trianglePipeline;
    swapchain swapChain;
    renderpass renderPass;

    engine_vk::vk_buffer vertexBuffer;
    engine_vk::vk_buffer ssboBuffer;
    std::vector<engine_vk::vk_buffer> uniformBuffers;
    std::vector<vk::DescriptorSet> descriptorSets;

    std::vector<vk::UniqueCommandBuffer> cmdBuffers;
    std::vector<vk::UniqueSemaphore> imageAvailableSemaphores;
    std::vector<vk::UniqueSemaphore> renderFinishedSemaphores;
    std::vector<vk::UniqueFence> inFlightFences;
    std::vector<vk::Fence> imagesInFlight;

    std::uint32_t nextImage;
    std::size_t currentFrame;

    void allocateCmdBuffers();
    void allocateVertexBuffer();
    void recordCmdBuffer(std::uint32_t index) noexcept;

public:
    explicit triangle_renderer(const engine_vk& engine);
    void startFrame() noexcept;
    void drawFrame() noexcept;
    void endFrame() noexcept;
};
#define DISPLAY_TRIANGLE_RENDERER_H

#endif //DISPLAY_TRIANGLE_RENDERER_H
