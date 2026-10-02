
#include <vector>
#include <algorithm>

#ifdef PLATFORM_IOS

#if ! __has_feature(objc_arc)
#error "ARC is off"
#endif

#include "util.h"
#include "shader.h"
#include "rendering_metal.h"

namespace {
    static const MTLPrimitiveType g_topologies[] = {
        MTLPrimitiveTypePoint,
        MTLPrimitiveTypeLine,
        MTLPrimitiveTypeLineStrip,
        MTLPrimitiveTypeTriangle,
        MTLPrimitiveTypeTriangleStrip,
    };
    
    static const std::uint32_t MAX_TEXTURES = 4;
    static const std::uint32_t FRAME_CONST_BINDING_INDEX = 0;
    static const std::uint32_t DRAW_CONST_BINDING_INDEX = 1;
    static const std::uint32_t VS_INPUT_BINDING_START = 2;
    static const std::uint32_t VS_INPUT_VERTEX_COUNT = 3;
    
    std::uint32_t roundTo256(std::uint32_t value) {
        std::uint32_t result = ((value - std::uint32_t(1)) & ~std::uint32_t(255)) + 256;
        return result;
    }
    
    const std::string &generateRenderPipelineStateName(const foundation::MetalShader *platformShader, const foundation::MetalTarget *platformTarget, foundation::BlendType blendType) {
        static std::string buffer = std::string(1024, 0);
        strcpy(buffer.data(), "r0_b0_");
        strcpy(buffer.data() + 6, platformShader->getName().c_str());
        buffer[1] = platformTarget ? '0' + int(platformTarget->getTexture(0)->getFormat()) : int(foundation::RenderTextureFormat::RGBA8UN);
        buffer[4] = '0' + int(blendType);
        return buffer;
    }
    
    void (*initializeBlendOptions[])(MTLRenderPipelineColorAttachmentDescriptor *target) = {
        [](MTLRenderPipelineColorAttachmentDescriptor *target){},
        [](MTLRenderPipelineColorAttachmentDescriptor *target){
            target.blendingEnabled = false;
        },
        [](MTLRenderPipelineColorAttachmentDescriptor *target){
            target.blendingEnabled = true;
            target.alphaBlendOperation = MTLBlendOperationAdd;
            target.rgbBlendOperation = MTLBlendOperationAdd;
            target.sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
            target.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
            target.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
            target.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        },
        [](MTLRenderPipelineColorAttachmentDescriptor *target){
            target.alphaBlendOperation = MTLBlendOperationAdd;
            target.rgbBlendOperation = MTLBlendOperationAdd;
            target.sourceAlphaBlendFactor = MTLBlendFactorOne;
            target.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
            target.destinationAlphaBlendFactor = MTLBlendFactorOne;
            target.destinationRGBBlendFactor = MTLBlendFactorOne;
        }
    };

    std::weak_ptr<foundation::RenderingInterface> g_instance;
}

namespace foundation {
    MetalShader::MetalShader(const std::string &name, const InputLayout &layout, id<MTLLibrary> library, std::uint32_t constBufferLength)
        : _name(name)
        , _layout(layout)
        , _constBufferLength(constBufferLength)
        , _library(library)
    {}
    
    MetalShader::~MetalShader() {
        _library = nil;
    }
    
    const InputLayout &MetalShader::getInputLayout() const {
        return _layout;
    }

    std::uint32_t MetalShader::getConstBufferLength() const {
        return _constBufferLength;
    }
    
    id<MTLFunction> MetalShader::getVertexShader() const {
        return [_library newFunctionWithName:@"main_vertex"];
    }
    
    id<MTLFunction> MetalShader::getFragmentShader() const {
        return [_library newFunctionWithName:@"main_fragment"];
    }
    
    const std::string &MetalShader::getName() const {
        return _name;
    }
}

namespace foundation {
    MetalTexture::MetalTexture(id<MTLTexture> texture, RenderTextureFormat fmt, std::uint32_t w, std::uint32_t h, std::uint32_t mipCount)
        : _texture(texture)
        , _format(fmt)
        , _width(w)
        , _height(h)
        , _mipCount(mipCount)
    {
    }
    
    MetalTexture::~MetalTexture() {
        _texture = nil;
    }
    
    std::uint32_t MetalTexture::getWidth() const {
        return _width;
    }
    
    std::uint32_t MetalTexture::getHeight() const {
        return _height;
    }
    
    std::uint32_t MetalTexture::getMipCount() const {
        return _mipCount;
    }
    
    RenderTextureFormat MetalTexture::getFormat() const {
        return _format;
    }

    id<MTLTexture> MetalTexture::getNativeTexture() const {
        return _texture;
    }
}

namespace foundation {
    MetalTarget::MetalTarget(__strong id<MTLTexture> *targets, std::uint32_t count, id<MTLTexture> depth, RenderTextureFormat fmt, std::uint32_t w, std::uint32_t h)
        : _count(count)
        , _width(w)
        , _height(h)
    {
        for (unsigned i = 0; i < count; i++) {
            _textures[i] = std::make_shared<RTTexture>(*this, fmt, targets[i]);
        }
        _depth = std::make_shared<RTTexture>(*this, RenderTextureFormat::R32F, depth);
    }
    
    MetalTarget::~MetalTarget() {
        for (unsigned i = 0; i < _count; i++) {
            _textures[i] = nullptr;
        }
        _depth = nullptr;
    }
    
    std::uint32_t MetalTarget::getWidth() const {
        return _width;
    }
    
    std::uint32_t MetalTarget::getHeight() const {
        return _height;
    }
    
    RenderTextureFormat MetalTarget::getFormat() const {
        return _textures[0]->getFormat();
    }
    
    std::uint32_t MetalTarget::getTextureCount() const {
        return _count;
    }
    
    const std::shared_ptr<RenderTexture> &MetalTarget::getTexture(unsigned index) const {
        return _textures[index];
    }
    
    const std::shared_ptr<RenderTexture> &MetalTarget::getDepth() const {
        return _depth;
    }
}

namespace foundation {
    MetalData::MetalData(id<MTLBuffer> vdata, id<MTLBuffer> indexes, std::uint32_t vcnt, std::uint32_t icnt, std::uint32_t stride)
        : _vertices(vdata)
        , _indexes(indexes)
        , _vcount(vcnt)
        , _icount(icnt)
        , _stride(stride)
    {
    }
    
    MetalData::~MetalData() {
        _vertices = nil;
        _indexes = nil;
    }
    
    std::uint32_t MetalData::getVertexCount() const {
        return _vcount;
    }
    std::uint32_t MetalData::getIndexCount() const {
        return _icount;
    }
    
    std::uint32_t MetalData::getStride() const {
        return _stride;
    }
    
    id<MTLBuffer> MetalData::getVertexes() const {
        return _vertices;
    }
    id<MTLBuffer> MetalData::getIndexes() const {
        return _indexes;
    }
}

namespace foundation {
    MetalRendering::MetalRendering(const std::shared_ptr<PlatformInterface> &platform) : _platform(platform) {
        @autoreleasepool {
            _platform->logMsg("[RENDER] Initialization : Metal");
            
            _device = MTLCreateSystemDefaultDevice();
            _commandQueue = [_device newCommandQueue];
            _frameBufferingSemaphore = dispatch_semaphore_create(BUFFERED_FRAMES_MAX);
            
            MTLSamplerDescriptor *samplerDesc = [MTLSamplerDescriptor new];
            samplerDesc.minFilter = MTLSamplerMinMagFilterNearest;
            samplerDesc.magFilter = MTLSamplerMinMagFilterNearest;
            _samplerStates[int(foundation::SamplerType::NEAREST)] = [_device newSamplerStateWithDescriptor:samplerDesc];
            
            samplerDesc.minFilter = MTLSamplerMinMagFilterLinear;
            samplerDesc.magFilter = MTLSamplerMinMagFilterLinear;
            _samplerStates[int(foundation::SamplerType::LINEAR)] = [_device newSamplerStateWithDescriptor:samplerDesc];
            
            MTLDepthStencilDescriptor *depthDesc = [MTLDepthStencilDescriptor new];
            depthDesc.depthCompareFunction = MTLCompareFunctionAlways;
            depthDesc.depthWriteEnabled = NO;
            _depthStates[int(foundation::DepthBehavior::DISABLED)] = [_device newDepthStencilStateWithDescriptor:depthDesc];

            depthDesc.depthCompareFunction = MTLCompareFunctionGreater;
            depthDesc.depthWriteEnabled = NO;
            _depthStates[int(foundation::DepthBehavior::TEST_ONLY)] = [_device newDepthStencilStateWithDescriptor:depthDesc];
            
            depthDesc.depthCompareFunction = MTLCompareFunctionGreater;
            depthDesc.depthWriteEnabled = YES;
            _depthStates[int(foundation::DepthBehavior::TEST_AND_WRITE)] = [_device newDepthStencilStateWithDescriptor:depthDesc];
            
            for (std::uint32_t i = 0; i < BUFFERED_FRAMES_MAX; i++) {
                _constantsBuffers[i] = [_device newBufferWithLength:CONSTANT_BUFFER_OFFSET_MAX options:MTLResourceStorageModeShared];
                _dynamicBuffers[i] = [_device newBufferWithLength:DYNAMIC_BUFFER_OFFSET_MAX options:MTLResourceStorageModeShared];
            }
            
            _currentCommandBuffer = [_commandQueue commandBuffer];
            _platform->logMsg("[RENDER] Initialization : complete");
        }
    }
    
    MetalRendering::~MetalRendering() {}
    
    void MetalRendering::updateFrameConstants(const math::transform3f &vp, const math::transform3f &svp, const math::transform3f &ivp, const math::vector3f &camPos, const math::vector3f &camDir) {
        _frameConstants.plmVPMatrix = vp;
        _frameConstants.stdVPMatrix = svp;
        _frameConstants.invVPMatrix = ivp;
        _frameConstants.cameraPosition.xyz = camPos;
        _frameConstants.cameraDirection.xyz = camDir;
    }
    
    RenderShaderPtr MetalRendering::createShader(const char *shadersrc, const InputLayout &layout) {
        std::shared_ptr<RenderShader> result;
        std::string shaderName = std::to_string(std::hash<std::string>{}(shadersrc));
        
        auto shaderIndex = _shaders.find(shaderName);
        if (shaderIndex != _shaders.end()) {
            return shaderIndex->second;
        }
        else {
            util::strstream input(shadersrc, strlen(shadersrc));
            std::string error;
            
            const auto &[vs, fs, constLength] = foundation::makePlatformShaderSource(shadersrc, layout, error);
            const std::string nativeShader = shaderUtils::makeLines(vs + fs);
            
            if (error.empty()) {
                @autoreleasepool {
                    NSError *nsError = nil;
                    MTLCompileOptions* compileOptions = [MTLCompileOptions new];
                    compileOptions.languageVersion = MTLLanguageVersion2_0;
                    compileOptions.fastMathEnabled = true;
                    
                    id<MTLLibrary> library = [_device newLibraryWithSource:[NSString stringWithUTF8String:nativeShader.data()] options:compileOptions error:&nsError];
                    
                    if (library) {
                        result = std::make_shared<MetalShader>(shaderName, layout, library, constLength);
                    }
                    else {
                        const char *errorDesc = [[nsError localizedDescription] UTF8String];
                        _platform->logError("[MetalRendering::createShader] generated code:\n--------------------\n%s\n--------------------\n%s\n", nativeShader.data(), errorDesc);
                    }
                }
            }
            else {
                _platform->logError("[MetalRendering::createShader] shader error : %s\n", error.data());
            }
        }

        return result;
    }
    
    namespace {
        static MTLPixelFormat nativeTextureFormat[] = {
            MTLPixelFormatR8Unorm,
            MTLPixelFormatR16Float,
            MTLPixelFormatR32Float,
            MTLPixelFormatRG8Unorm,
            MTLPixelFormatRG16Float,
            MTLPixelFormatRG32Float,
            MTLPixelFormatRGBA8Unorm,
            MTLPixelFormatRGBA16Float,
            MTLPixelFormatRGBA32Float
        };
    }
        
    RenderTexturePtr MetalRendering::createTexture(RenderTextureFormat format, std::uint32_t w, std::uint32_t h, const std::initializer_list<const void *> &mipsData) {
        auto getTexture2DPitch = [](RenderTextureFormat fmt, std::size_t width) {
            switch (fmt) {
            case RenderTextureFormat::R8UN:
                return width;
            case RenderTextureFormat::R16F:
                return width * sizeof(std::uint16_t);
            case RenderTextureFormat::R32F:
                return width * sizeof(float);
            case RenderTextureFormat::RG8UN:
                return width * 2;
            case RenderTextureFormat::RG16F:
                return width * 2 * sizeof(std::uint16_t);
            case RenderTextureFormat::RG32F:
                return width * 2 * sizeof(float);
            case RenderTextureFormat::RGBA8UN:
                return width * 4;
            case RenderTextureFormat::RGBA16F:
                return width * 4 * sizeof(std::uint16_t);
            case RenderTextureFormat::RGBA32F:
                return width * 4 * sizeof(float);
            default:
                return std::size_t(0);
            }
        };
        
        @autoreleasepool {
            size_t mipmapLevelCount = std::max(size_t(1), mipsData.size());
            
            MTLTextureDescriptor *desc = [MTLTextureDescriptor new];
            desc.pixelFormat = nativeTextureFormat[int(format)];
            desc.width = w;
            desc.height = h;
            desc.mipmapLevelCount = mipmapLevelCount;
            desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
            id<MTLTexture> texture = [_device newTextureWithDescriptor:desc];
            
            unsigned counter = 0;
            for (auto &item : mipsData) {
                MTLRegion region = {{ 0, 0, 0 }, {w >> counter, h >> counter, 1}};
                [texture replaceRegion:region mipmapLevel:counter withBytes:item bytesPerRow:getTexture2DPitch(format, w)];
            }
            
            return std::make_shared<MetalTexture>(texture, format, w, h, mipmapLevelCount);
        }
    }
    
    RenderTargetPtr MetalRendering::createRenderTarget(RenderTextureFormat format, std::uint32_t count, std::uint32_t w, std::uint32_t h, bool withZBuffer) {
        @autoreleasepool {
            MTLTextureDescriptor *desc = [MTLTextureDescriptor new];
            id<MTLTexture> targets[RenderTarget::MAX_TEXTURE_COUNT] = {nullptr, nullptr, nullptr, nullptr};
            
            desc.pixelFormat = nativeTextureFormat[int(format)];
            desc.width = w;
            desc.height = h;
            desc.mipmapLevelCount = 1;
            desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite | MTLTextureUsageRenderTarget;
            desc.storageMode = MTLStorageModePrivate;

            for (unsigned i = 0; i < count; i++) {
                targets[i] = [_device newTextureWithDescriptor:desc];
            }

            id<MTLTexture> depth = nil;
            
            if (withZBuffer) {
                desc.pixelFormat = MTLPixelFormatDepth32Float;
                desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
                desc.storageMode = MTLStorageModePrivate;
                desc.width = w;
                desc.height = h;
                desc.mipmapLevelCount = 1;
                depth = [_device newTextureWithDescriptor:desc];
            }
            
            return std::make_shared<MetalTarget>(targets, count, depth, format, w, h);
        }
    }
    
    RenderDataPtr MetalRendering::createData(const InputLayout &layout, const void *data, std::uint32_t vcnt, const std::uint32_t *indexes, std::uint32_t icnt) {
        const std::uint32_t stride = layout.getStride();
        if (indexes && layout.repeat > 1) {
            _platform->logError("[MetalRendering::createData] Vertex repeat is incompatible with indexed data");
            return nullptr;
        }
        @autoreleasepool {
            id<MTLBuffer> vbuffer = [_device newBufferWithBytes:data length:(vcnt * stride) options:MTLResourceStorageModeShared];
            id<MTLBuffer> ibuffer = nil;
            
            if (indexes) {
                ibuffer = [_device newBufferWithBytes:indexes length:(icnt * sizeof(std::uint32_t)) options:MTLResourceStorageModeShared];
            }
            
            return std::make_shared<MetalData>(vbuffer, ibuffer, vcnt, icnt, stride);
        }
    }
    
    float MetalRendering::getBackBufferWidth() const {
        return _platform->getScreenWidth();
    }
    
    float MetalRendering::getBackBufferHeight() const {
        return _platform->getScreenHeight();
    }
    
    math::transform3f MetalRendering::getStdVPMatrix() const {
        return _frameConstants.stdVPMatrix;
    }
    
    void MetalRendering::forTarget(const RenderTargetPtr &target, const RenderTexturePtr &depth, const std::optional<math::color> &rgba, util::callback<void(RenderingInterface &)> &&pass) {
        if (_view == nil) {
            _view = (__bridge MTKView *)_platform->attachNativeRenderingContext((__bridge void *)_device);
        }
        if (_view) {
            MTLRenderPassDescriptor *renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
            MTLViewport viewPort {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
            
            if (depth) {
                renderPassDescriptor.depthAttachment.loadAction = MTLLoadActionLoad;
                renderPassDescriptor.depthAttachment.texture = static_cast<const MetalTexBase *>(depth.get())->getNativeTexture();
            }
            else {
                renderPassDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
                renderPassDescriptor.depthAttachment.clearDepth = 0.0;
            }
            
            MTLLoadAction colorClearAction = MTLLoadActionLoad;
            MTLClearColor colorClearValue = MTLClearColorMake(0, 0, 0, 0);
            
            if (rgba.has_value()) {
                colorClearAction = MTLLoadActionClear;
                colorClearValue = MTLClearColorMake(rgba->r, rgba->g, rgba->b, rgba->a);
            }
            
            if (target) {
                for (std::uint32_t i = 0; i < target->getTextureCount(); i++) {
                    renderPassDescriptor.colorAttachments[i].texture = static_cast<const MetalTexBase *>(target->getTexture(i).get())->getNativeTexture();
                    renderPassDescriptor.colorAttachments[i].storeAction = MTLStoreActionStore;
                    renderPassDescriptor.colorAttachments[i].loadAction = colorClearAction;
                    renderPassDescriptor.colorAttachments[i].clearColor = colorClearValue;
                }
                
                renderPassDescriptor.depthAttachment.texture = static_cast<const MetalTexBase *>(target->getDepth().get())->getNativeTexture();
                renderPassDescriptor.depthAttachment.storeAction = MTLStoreActionStore;
                
                viewPort.width = _frameConstants.rtBounds.x = float(target->getWidth());
                viewPort.height = _frameConstants.rtBounds.y = float(target->getHeight());
            }
            else {
                renderPassDescriptor.colorAttachments[0].texture = _view.currentDrawable.texture;
                renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
                renderPassDescriptor.colorAttachments[0].loadAction = colorClearAction;
                renderPassDescriptor.colorAttachments[0].clearColor = colorClearValue;
                renderPassDescriptor.depthAttachment.storeAction = MTLStoreActionStore;
                
                if (depth == nullptr) {
                    renderPassDescriptor.depthAttachment.texture = _view.depthStencilTexture;
                }
                
                viewPort.width = _frameConstants.rtBounds.x = _platform->getScreenWidth();
                viewPort.height = _frameConstants.rtBounds.y = _platform->getScreenHeight();
            }
            
            _finishRenderCommandEncoder();
            _currentRenderCommandEncoder = [_currentCommandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
            _currentTarget = target;
            _isForTarget = true;
            
            _appendConstantBuffer(&_frameConstants, sizeof(FrameConstants), FRAME_CONST_BINDING_INDEX);
            [_currentRenderCommandEncoder setViewport:viewPort];
            
            pass(*this);
            
            [_currentRenderCommandEncoder endEncoding];
            _currentRenderCommandEncoder = nil;
            _currentTarget = nullptr;
            _isForTarget = false;
        }
    }
    
    void MetalRendering::applyShader(const RenderShaderPtr &shader, foundation::RenderTopology topology, BlendType blendType, DepthBehavior depthBehavior) {
        if (_isForTarget) {
            if (_currentRenderCommandEncoder == nil) {
                MTLRenderPassDescriptor *renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
                
                renderPassDescriptor.colorAttachments[0].texture = _view.currentDrawable.texture;
                renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
                renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
                renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
                renderPassDescriptor.depthAttachment.texture = _view.depthStencilTexture;
                renderPassDescriptor.depthAttachment.storeAction = MTLStoreActionStore;
                renderPassDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
                renderPassDescriptor.depthAttachment.clearDepth = 0.0f;
                
                _currentRenderCommandEncoder = [_currentCommandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
                
                MTLViewport viewPort {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
                viewPort.width = _frameConstants.rtBounds.x = _platform->getScreenWidth();
                viewPort.height = _frameConstants.rtBounds.y = _platform->getScreenHeight();
                _appendConstantBuffer(&_frameConstants, sizeof(FrameConstants), FRAME_CONST_BINDING_INDEX);
                [_currentRenderCommandEncoder setViewport:viewPort];
            }
            
            const MetalShader *platformShader = static_cast<const MetalShader *>(shader.get());
            const MetalTarget *platformTarget = static_cast<const MetalTarget *>(_currentTarget.get());
            
            if (platformShader) {
                id<MTLRenderPipelineState> state = nil;
                const std::string &stateName = generateRenderPipelineStateName(platformShader, platformTarget, blendType);
                
                auto index = _renderPipelineStates.find(stateName);
                if (index == _renderPipelineStates.end()) {
                    MTLRenderPipelineDescriptor *desc = [MTLRenderPipelineDescriptor new];
                    desc.vertexFunction = platformShader->getVertexShader();
                    desc.fragmentFunction = platformShader->getFragmentShader();
                    
                    if (platformTarget) {
                        for (std::uint32_t i = 0; i < platformTarget->getTextureCount(); i++) {
                            desc.colorAttachments[i].pixelFormat = nativeTextureFormat[int(platformTarget->getTexture(0)->getFormat())];
                            initializeBlendOptions[int(blendType)](desc.colorAttachments[i]);
                        }
                        if (platformTarget->getDepth()) {
                            desc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
                        }
                    }
                    else {
                        desc.colorAttachments[0].pixelFormat = _view.colorPixelFormat;
                        desc.depthAttachmentPixelFormat = _view.depthStencilPixelFormat;
                        initializeBlendOptions[int(blendType)](desc.colorAttachments[0]);
                    }
                    
                    NSError *error;
                    if ((state = [_device newRenderPipelineStateWithDescriptor:desc error:&error]) != nil) {
                        _renderPipelineStates.emplace(stateName, state);
                    }
                    else {
                        _platform->logError("[MetalRendering::applyState] %s\n", [[error localizedDescription] UTF8String]);
                    }
                }
                else {
                    state = index->second;
                }
                
                if (state) {
                    [_currentRenderCommandEncoder setRenderPipelineState:state];
                    [_currentRenderCommandEncoder setDepthStencilState:_depthStates[int(depthBehavior)]];
                    
                    _currentTopology = topology;
                    _currentShader = shader;
                }
            }
        }
    }
        
    void MetalRendering::applyShaderConstants(const void *constants) {
        if (_currentRenderCommandEncoder) {
            const MetalShader *platformShader = static_cast<const MetalShader *>(_currentShader.get());
            const std::uint32_t constBufferLength = platformShader->getConstBufferLength();
            
            if (platformShader && constBufferLength) {
                _appendConstantBuffer(constants, constBufferLength, DRAW_CONST_BINDING_INDEX);
            }
        }
    }

    void MetalRendering::_applyTextures(const std::pair<RenderTexturePtr, foundation::SamplerType> *textures, std::size_t size) {
        if (_currentRenderCommandEncoder && _currentShader) {
            if (size <= MAX_TEXTURES) { // TODO: apply fisrt MAX_TEXTURES textures anyway
                const NSRange range {0, size};
                __unsafe_unretained id<MTLTexture> texarray[MAX_TEXTURES] = {nil};
                __unsafe_unretained id<MTLSamplerState> smarray[MAX_TEXTURES] = {nil};
                
                std::uint32_t index = 0;
                for (std::size_t i = 0; i < size; i++) {
                    texarray[index] = textures[i].first ? static_cast<const MetalTexBase *>(textures[i].first.get())->getNativeTexture() : nil;
                    smarray[index] = _samplerStates[int(textures[i].second)];
                    index++;
                }
                
                [_currentRenderCommandEncoder setFragmentSamplerStates:smarray withRange:range];
                [_currentRenderCommandEncoder setFragmentTextures:texarray withRange:range];
                [_currentRenderCommandEncoder setVertexSamplerStates:smarray withRange:range];
                [_currentRenderCommandEncoder setVertexTextures:texarray withRange:range];
            }
        }
    }
    
    void MetalRendering::applyTextures(const std::initializer_list<std::pair<RenderTexturePtr, SamplerType>> &textures) {
        _applyTextures(textures.begin(), textures.size());
    }
    
    void MetalRendering::applyTextures(const std::vector<std::pair<RenderTexturePtr, foundation::SamplerType>> &textures) {
        _applyTextures(textures.data(), textures.size());
    }
    
    void MetalRendering::draw(std::uint32_t vertexCount) {
        if (_currentRenderCommandEncoder && _currentShader) {
            const InputLayout &layout = _currentShader->getInputLayout();
            const MTLPrimitiveType topology = g_topologies[int(_currentTopology)];
            
            [_currentRenderCommandEncoder setVertexBuffer:nil offset:0 atIndex:VS_INPUT_BINDING_START];
            
            if (layout.repeat > 1) {
                [_currentRenderCommandEncoder setVertexBytes:&vertexCount length:sizeof(std::uint32_t) atIndex:VS_INPUT_VERTEX_COUNT];
                [_currentRenderCommandEncoder drawPrimitives:topology vertexStart:0 vertexCount:layout.repeat instanceCount:vertexCount];
            }
            else {
                [_currentRenderCommandEncoder drawPrimitives:topology vertexStart:0 vertexCount:vertexCount];
            }
        }
    }
    
    void MetalRendering::draw(const RenderDataPtr &inputData, std::uint32_t instanceCount) {
        if (_currentRenderCommandEncoder && _currentShader && inputData) {
            const InputLayout &layout = _currentShader->getInputLayout();
            const MetalData *implData = static_cast<const MetalData *>(inputData.get());
            const MTLPrimitiveType topology = g_topologies[int(_currentTopology)];
            
            std::uint32_t vcnt = 1;
            std::uint32_t icnt = 0;
            id<MTLBuffer> vbuffer = nil;
            id<MTLBuffer> ibuffer = nil;
            
            if (implData) {
                vcnt = implData->getVertexCount();
                icnt = implData->getIndexCount();
                vbuffer = implData->getVertexes();
                ibuffer = implData->getIndexes();
            }

            //[_currentRenderCommandEncoder setTriangleFillMode:MTLTriangleFillModeLines];
            [_currentRenderCommandEncoder setVertexBuffer:vbuffer offset:0 atIndex:VS_INPUT_BINDING_START];

            if (layout.repeat > 1) {
                [_currentRenderCommandEncoder setVertexBytes:&vcnt length:sizeof(std::uint32_t) atIndex:VS_INPUT_VERTEX_COUNT];
                [_currentRenderCommandEncoder drawPrimitives:topology vertexStart:0 vertexCount:layout.repeat instanceCount:vcnt * instanceCount];
            }
            else {
                if (ibuffer) {
                    [_currentRenderCommandEncoder drawIndexedPrimitives:topology indexCount:icnt indexType:MTLIndexTypeUInt32 indexBuffer:ibuffer indexBufferOffset:0 instanceCount:instanceCount];
                }
                else if (vbuffer) {
                    [_currentRenderCommandEncoder drawPrimitives:topology vertexStart:0 vertexCount:vcnt instanceCount:instanceCount];
                }
            }
        }
    }

    void MetalRendering::draw(const void *data, std::uint32_t vcnt, const std::uint32_t *indexes, std::uint32_t icnt) {
        if (_currentRenderCommandEncoder && _currentShader) {
            const InputLayout &layout = _currentShader->getInputLayout();
            const MTLPrimitiveType topology = g_topologies[int(_currentTopology)];
            const std::uint32_t vlen = vcnt * layout.getStride();
            const std::uint32_t roundedSize = roundTo256(vlen + icnt * sizeof(std::uint32_t));
            
            if (_dynamicBufferOffset + roundedSize < DYNAMIC_BUFFER_OFFSET_MAX) {
                std::uint8_t *dynamicMemory = static_cast<std::uint8_t *>([_dynamicBuffers[_dynamicBuffersIndex] contents]);
                std::memcpy(dynamicMemory + _dynamicBufferOffset, data, vlen);
                if (indexes) {
                    std::memcpy(dynamicMemory + _dynamicBufferOffset + vlen, indexes, icnt * sizeof(std::uint32_t));
                }
                
                [_currentRenderCommandEncoder setVertexBuffer:_dynamicBuffers[_dynamicBuffersIndex] offset:_dynamicBufferOffset atIndex:VS_INPUT_BINDING_START];
            }
            else {
                _platform->logError("[MetalRendering::_appendConstantBuffer] Out of dynamic buffer length\n");
                return;
            }
            
            if (layout.repeat > 1) {
                [_currentRenderCommandEncoder setVertexBytes:&vcnt length:sizeof(std::uint32_t) atIndex:VS_INPUT_VERTEX_COUNT];
                [_currentRenderCommandEncoder drawPrimitives:topology vertexStart:0 vertexCount:layout.repeat instanceCount:vcnt];
            }
            else {
                if (indexes) {
                    const std::uint32_t ioff = _dynamicBufferOffset + vlen;
                    [_currentRenderCommandEncoder drawIndexedPrimitives:topology indexCount:icnt indexType:MTLIndexTypeUInt32 indexBuffer:_dynamicBuffers[_dynamicBuffersIndex] indexBufferOffset:ioff];
                }
                else {
                    [_currentRenderCommandEncoder drawPrimitives:topology vertexStart:0 vertexCount:vcnt];
                }
            }
            
            _dynamicBufferOffset += roundedSize;
        }

    }
        
    void MetalRendering::presentFrame() {
        _finishRenderCommandEncoder();
        
        if (_view && _currentCommandBuffer) {
            MetalRendering *m = this;
            
            [_currentCommandBuffer presentDrawable:_view.currentDrawable];
            [_currentCommandBuffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull) {
                dispatch_semaphore_signal(m->_frameBufferingSemaphore);
            }];
            [_currentCommandBuffer commit];
            _currentCommandBuffer = [_commandQueue commandBuffer];
            
            _constantsBuffersIndex = (_constantsBuffersIndex + 1) % BUFFERED_FRAMES_MAX;
            _constantsBufferOffset = 0;

            _dynamicBuffersIndex = (_dynamicBuffersIndex + 1) % BUFFERED_FRAMES_MAX;
            _dynamicBufferOffset = 0;

            dispatch_semaphore_wait(_frameBufferingSemaphore, DISPATCH_TIME_FOREVER);
        }
    }
    
    void MetalRendering::_appendConstantBuffer(const void *buffer, std::uint32_t size, std::uint32_t index) {
        std::uint32_t roundedSize = roundTo256(size);
        
        if (_constantsBufferOffset + roundedSize < CONSTANT_BUFFER_OFFSET_MAX) {
            std::uint8_t *constantsMemory = static_cast<std::uint8_t *>([_constantsBuffers[_constantsBuffersIndex] contents]);
            std::memcpy(constantsMemory + _constantsBufferOffset, buffer, size);
            
            [_currentRenderCommandEncoder setVertexBuffer:_constantsBuffers[_constantsBuffersIndex] offset:_constantsBufferOffset atIndex:index];
            [_currentRenderCommandEncoder setFragmentBuffer:_constantsBuffers[_constantsBuffersIndex] offset:_constantsBufferOffset atIndex:index];
            
            _constantsBufferOffset += roundedSize;
            return true;
        }
        else {
            _platform->logError("[MetalRendering::_appendConstantBuffer] Out of constants buffer length\n");
        }
        return false;
    }
    
    void MetalRendering::_finishRenderCommandEncoder() {
        if (_currentRenderCommandEncoder) {
            [_currentRenderCommandEncoder endEncoding];
            _currentRenderCommandEncoder = nil;
            _currentShader = nullptr;
        }
    }
}

namespace foundation {
    std::shared_ptr<RenderingInterface> RenderingInterface::instance(const std::shared_ptr<PlatformInterface> &platform) {
        std::shared_ptr<RenderingInterface> result;

        if (g_instance.use_count() == 0) {
            g_instance = result = std::make_shared<MetalRendering>(platform);
        }
        else {
            result = g_instance.lock();
        }

        return result;
    }
}

#endif // PLATFORM_IOS
