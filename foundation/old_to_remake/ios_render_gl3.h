#pragma once

#define GLES_SILENCE_DEPRECATION
#import <GLKit/GLKit.h>

namespace platform {
    class IOSRender : public RenderingDevice {
    public:
        IOSRender(const std::shared_ptr<Platform> &platform);
        ~IOSRender();

        void updateCameraTransform(const float (&camPos)[3], const float(&camDir)[3], const float(&camVP)[16]);
        
        std::shared_ptr<Shader> createShader(
            const char *shadersrc,
            const std::initializer_list<ShaderInput> &vertex,
            const std::initializer_list<ShaderInput> &instance,
            const void *prmnt
        );
        
        std::shared_ptr<Texture2D> createTexture(
            Texture2D::Format format,
            std::uint32_t width,
            std::uint32_t height,
            const std::initializer_list<const std::uint8_t *> &mipsData
        );
        
        std::shared_ptr<StructuredData> createData(const void *data, std::uint32_t count, std::uint32_t stride);
        
        void applyShader(const std::shared_ptr<Shader> &shader, const void *constants);
        void applyTextures(const std::initializer_list<const Texture2D *> &textures);
        
        void drawGeometry(std::uint32_t vertexCount, Topology topology);
        void drawGeometry(
            const std::shared_ptr<StructuredData> &vertexData,
            const std::shared_ptr<StructuredData> &instanceData,
            std::uint32_t vertexCount,
            std::uint32_t instanceCount,
            Topology topology
        );
        
        void prepareFrame();
        void presentFrame(float dtSec);
        void getFrameBufferData(std::uint8_t *imgFrame);

    private:
        struct FrameData {
            float viewProjMatrix[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
            float cameraPosition[4] = {0, 0, 0, 0};
            float cameraDirection[4] = {0, 0, 0, 0};
            float renderTargetBounds[4] = {0, 0, 0, 0};
        }
        _frameData;
        
        std::shared_ptr<Platform> _platform;
        std::shared_ptr<Shader> _currentShader;
        
        GLuint _shaderFrameDataBuffer;
        GLuint _shaderConstStreamBuffer;
        std::size_t _shaderConstStreamOffset;
    };

    void RenderingDevice::updateCameraTransform(const float (&camPos)[3], const float(&camDir)[3], const float(&camVP)[16]) {
        static_cast<IOSRender *>(this)->updateCameraTransform(camPos, camDir, camVP);
    }

    std::shared_ptr<Shader> RenderingDevice::createShader(
        const char *shadersrc,
        const std::initializer_list<ShaderInput> &vertex,
        const std::initializer_list<ShaderInput> &instance,
        const void *prmnt
    )
    {
        return static_cast<IOSRender *>(this)->createShader(shadersrc, vertex, instance, prmnt);
    }

    std::shared_ptr<Texture2D> RenderingDevice::createTexture(
        Texture2D::Format format,
        std::uint32_t width,
        std::uint32_t height,
        const std::initializer_list<const std::uint8_t *> &mipsData
    )
    {
        return static_cast<IOSRender *>(this)->createTexture(format, width, height, mipsData);
    }

    std::shared_ptr<StructuredData> RenderingDevice::createData(const void *data, std::uint32_t count, std::uint32_t stride) {
        return static_cast<IOSRender *>(this)->createData(data, count, stride);
    }

    void RenderingDevice::applyShader(const std::shared_ptr<Shader> &shader, const void *constants) {
        static_cast<IOSRender *>(this)->applyShader(shader, constants);
    }
    
    void RenderingDevice::applyTextures(const std::initializer_list<const Texture2D *> &textures) {
        static_cast<IOSRender *>(this)->applyTextures(textures);
    }

    void RenderingDevice::drawGeometry(std::uint32_t vertexCount, Topology topology) {
        static_cast<IOSRender *>(this)->drawGeometry(vertexCount, topology);
    }
    
    void RenderingDevice::drawGeometry(
        const std::shared_ptr<StructuredData> &vertexData,
        const std::shared_ptr<StructuredData> &instanceData,
        std::uint32_t vertexCount,
        std::uint32_t instanceCount,
        Topology topology
    )
    {
        static_cast<IOSRender *>(this)->drawGeometry(vertexData, instanceData, vertexCount, instanceCount, topology);
    }

    void RenderingDevice::prepareFrame() {
        static_cast<IOSRender *>(this)->prepareFrame();
    }
    
    void RenderingDevice::presentFrame(float dtSec) {
        static_cast<IOSRender *>(this)->presentFrame(dtSec);
    }
    
    void RenderingDevice::getFrameBufferData(std::uint8_t *imgFrame) {
        static_cast<IOSRender *>(this)->getFrameBufferData(imgFrame);
    }
}
