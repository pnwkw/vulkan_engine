#include "triangle_renderer.h"

#include <spdlog/spdlog.h>

triangle_pipeline::triangle_pipeline(const engine_vk &engine) : pipeline(engine) {
    addShader(vk::ShaderStageFlagBits::eVertex, "passthrough");
    addShader(vk::ShaderStageFlagBits::eFragment, "red");

    this->piasci.topology = vk::PrimitiveTopology::eTriangleList;
    this->piasci.primitiveRestartEnable = VK_FALSE;

    this->bindingDescription = { triangle_vertex::bindingDescriptions()[0] };
    this->attributeDescription = {triangle_vertex::attributeDescriptions()[0] };

    this->pvisci.vertexBindingDescriptionCount = static_cast<std::uint32_t>(this->bindingDescription.size());
    this->pvisci.pVertexBindingDescriptions = this->bindingDescription.data();

    this->pvisci.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(this->attributeDescription.size());
    this->pvisci.pVertexAttributeDescriptions = this->attributeDescription.data();

    this->pvsci.viewportCount = 1;
    this->pvsci.pViewports = nullptr;

    this->pvsci.scissorCount = 1;
    this->pvsci.pScissors = nullptr;

    this->dynamicStates.emplace_back(vk::DynamicState::eViewport);
    this->dynamicStates.emplace_back(vk::DynamicState::eScissor);

    this->pdsci.dynamicStateCount = static_cast<std::uint32_t>(dynamicStates.size());
    this->pdsci.pDynamicStates = dynamicStates.data();

    this->gpci.pDynamicState = &this->pdsci;
}

triangle_renderer::triangle_renderer(const engine_vk& engine) : engine(engine), trianglePipeline(engine), swapChain(engine), renderPass(engine, swapChain), nextImage(0), currentFrame(0) {
    this->trianglePipeline.finalize(this->renderPass, this->swapChain);

    this->imageAvailableSemaphores.reserve(swapchain::FRAMES_IN_FLIGHT);
    this->renderFinishedSemaphores.reserve(swapchain::FRAMES_IN_FLIGHT);
    this->inFlightFences.reserve(swapchain::FRAMES_IN_FLIGHT);
    this->imagesInFlight.resize(this->swapChain.getNumImages(), {});

    for(std::size_t i = 0; i < swapchain::FRAMES_IN_FLIGHT; ++i) {
        this->imageAvailableSemaphores.emplace_back(this->engine.createSemaphore());
        this->renderFinishedSemaphores.emplace_back(this->engine.createSemaphore());
        this->inFlightFences.emplace_back(this->engine.createFence(vk::FenceCreateFlagBits::eSignaled));
    }

    allocateCmdBuffers();
    allocateVertexBuffer();
}

void triangle_renderer::allocateVertexBuffer() {
    const std::vector<triangle_pipeline::triangle_vertex> vertices = {
        triangle_pipeline::triangle_vertex(glm::vec3{0.0f, -0.5f, 0.0f}),
        triangle_pipeline::triangle_vertex(glm::vec3{0.5f, 0.5f, 0.0f}),
        triangle_pipeline::triangle_vertex(glm::vec3{-0.5f, 0.5f, 0.0f}),
    };

    this->vertexBuffer = this->engine.createLocalBufferWithData(std::span(vertices).size_bytes(), vk::BufferUsageFlagBits::eVertexBuffer, vertices.data());
}

void triangle_renderer::allocateCmdBuffers() {
    this->cmdBuffers = this->engine.allocateCmdBuffers(vk::QueueFlagBits::eGraphics, vk::CommandBufferLevel::ePrimary, this->swapChain.getNumImages());
}

void triangle_renderer::recordCmdBuffer(std::uint32_t index) noexcept {
    const auto extent = this->swapChain.getExtent();

    vk::Viewport viewPort {
        0.0f,
        0.0f,
        static_cast<float>(extent.width),
        static_cast<float>(extent.height),
        0.0f,
        1.0f
    };

    vk::Rect2D scissor {
        vk::Rect2D{vk::Offset2D{0, 0}, extent}
    };

    const auto& buffer = this->cmdBuffers[index];

    buffer->reset({});

    const vk::CommandBufferBeginInfo cbbi {};
    buffer->begin(cbbi);

    const std::array<float, 4> col{0.0f, 0.0f, 0.0f, 1.0f};
    const std::vector<vk::ClearValue> clearValues{ vk::ClearColorValue(col) };
    const vk::Rect2D renderArea {{0,0}, this->swapChain.getExtent()};

    this->renderPass.begin(buffer, index, renderArea, clearValues, vk::SubpassContents::eInline);

    buffer->setViewport(0, 1, &viewPort);
    buffer->setScissor(0, 1, &scissor);

    this->trianglePipeline.bind(buffer, vk::PipelineBindPoint::eGraphics);

    const vk::Buffer vertexBuffers[] = { this->vertexBuffer.buffer.get() };
    const vk::DeviceSize offsets[] = {0};

    buffer->bindVertexBuffers(0, 1, vertexBuffers, offsets);

    buffer->draw(triangle_renderer::vertex_count, 1, 0, 0);

    buffer->endRenderPass();
    buffer->end();
}

void triangle_renderer::startFrame() noexcept {
}

void triangle_renderer::drawFrame() noexcept {
    try {
        this->nextImage = this->swapChain.acquireNextImage(this->imageAvailableSemaphores[this->currentFrame].get());

        if (this->imagesInFlight[this->nextImage]) {
            this->engine.waitFence(this->imagesInFlight[this->nextImage]);
        }

        this->imagesInFlight[this->nextImage] = this->inFlightFences[currentFrame].get();

        std::vector waitSemaphores { this->imageAvailableSemaphores[this->currentFrame].get() };
        std::vector signalSemaphores { this->renderFinishedSemaphores[this->currentFrame].get() };
        std::vector<vk::PipelineStageFlags> waitStages { vk::PipelineStageFlagBits::eColorAttachmentOutput };

        vk::SubmitInfo si {
            static_cast<std::uint32_t>(waitSemaphores.size()), waitSemaphores.data(),
            waitStages.data(),
            1, &this->cmdBuffers[this->nextImage].get(),
            static_cast<std::uint32_t>(signalSemaphores.size()), signalSemaphores.data()
        };

        this->engine.resetFence(*this->inFlightFences[this->currentFrame]);

        this->recordCmdBuffer(this->nextImage);

        this->engine.submit(vk::QueueFlagBits::eGraphics, si, *this->inFlightFences[this->currentFrame]);

        this->swapChain.present(signalSemaphores, this->nextImage);

        this->currentFrame = (this->currentFrame + 1) % swapchain::FRAMES_IN_FLIGHT;

        this->engine.waitDeviceIdle();

    } catch (const vk::OutOfDateKHRError &) {
        this->engine.waitDeviceIdle();

        this->swapChain.createSwapChain();

        this->renderPass.updateFormat();
        this->renderPass.createPassAndFrameBuffers();

        this->trianglePipeline.finalize(this->renderPass, this->swapChain);

        allocateCmdBuffers();
    }
}

void triangle_renderer::endFrame() noexcept {
}
