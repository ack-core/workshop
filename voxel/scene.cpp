
#include "scene.h"

namespace {
    // XFace Indeces : [-y-z, -y+z, +y-z, +y+z]
    // YFace Indeces : [-z-x, -z+x, +z-x, +z+x]
    // ZFace Indeces : [-y-x, -y+x, +y-x, +y+x]
    const char *g_staticMeshShaderSrc = R"(
        fixed {
            axis[3] : float4 =
                [0.0, 0.0, 1.0, 0.0]
                [1.0, 0.0, 0.0, 0.0]
                [0.0, 1.0, 0.0, 0.0]
                
            cube[12] : float4 =
                [-0.5, -0.5, 0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0]
                [0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, -0.5, -0.5, 1.0][0.5, 0.5, -0.5, 1.0]
                [0.5, 0.5, -0.5, 1.0][0.5, 0.5, 0.5, 1.0][-0.5, 0.5, -0.5, 1.0][-0.5, 0.5, 0.5, 1.0]

            faceUV[4] : float2 = [0.0, 0.0][1.0, 0.0][0.0, 1.0][1.0, 1.0]
        }
        inout {
            texcoord : float2
            koeff : float4
        }
        vssrc {
            float3 cubeCenter = float3(instance_position_color.xyz);
            float3 toCamera = frame_cameraPosition.xyz - cubeCenter;
            float3 toCamSign = _sign(toCamera);
            float4 relVertexPos = float4(toCamSign, 1.0) * fixed_cube[vertex_ID];
            float4 absVertexPos = float4(cubeCenter, 0.0) + relVertexPos;
            float3 faceNormal = toCamSign * fixed_axis[vertex_ID >> 2].xyz;

            int3 faceIndeces = int3(float3(2.0, 2.0, 2.0) * _step(0.0, relVertexPos.yzy) + _step(0.0, relVertexPos.zxx));
            
            float4 koeff = float4(0.0, 0.0, 0.0, 0.0);
            koeff = koeff + _step(0.5, -faceNormal.x) * instance_lightFaceNX;
            koeff = koeff + _step(0.5,  faceNormal.x) * instance_lightFacePX;
            koeff = koeff + _step(0.5, -faceNormal.y) * instance_lightFaceNY;
            koeff = koeff + _step(0.5,  faceNormal.y) * instance_lightFacePY;
            koeff = koeff + _step(0.5, -faceNormal.z) * instance_lightFaceNZ;
            koeff = koeff + _step(0.5,  faceNormal.z) * instance_lightFacePZ;
            
            float2 texcoord = float2(0.0, 0.0);
            texcoord = texcoord + _step(0.5, _abs(faceNormal.x)) * fixed_faceUV[faceIndeces.x];
            texcoord = texcoord + _step(0.5, _abs(faceNormal.y)) * fixed_faceUV[faceIndeces.y];
            texcoord = texcoord + _step(0.5, _abs(faceNormal.z)) * fixed_faceUV[faceIndeces.z];

            output_position = _transform(absVertexPos, frame_viewProjMatrix);
            output_texcoord = texcoord;
            output_koeff = koeff;
        }
        fssrc {
            float m0 = _lerp(input_koeff[0], input_koeff[1], input_texcoord.x);
            float m1 = _lerp(input_koeff[2], input_koeff[3], input_texcoord.x);
            float k = _pow(_smooth(0, 1, _lerp(m0, m1, input_texcoord.y)), 0.5);
            output_color = float4(k, k, k, 1.0);
        }
    )";

    // XFace Indeces : [-y-z, -y+z, +y-z, +y+z]
    // YFace Indeces : [-z-x, -z+x, +z-x, +z+x]
    // ZFace Indeces : [-y-x, -y+x, +y-x, +y+x]
    const char *g_dynamicMeshShaderSrc = R"(
        fixed {
            axis[3] : float4 =
                [0.0, 0.0, 1.0, 0.0]
                [1.0, 0.0, 0.0, 0.0]
                [0.0, 1.0, 0.0, 0.0]
                
            cube[12] : float4 =
                [-0.5, -0.5, 0.5, 1.0][-0.5, 0.5, 0.5, 1.0][0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0]
                [0.5, -0.5, 0.5, 1.0][0.5, 0.5, 0.5, 1.0][0.5, -0.5, -0.5, 1.0][0.5, 0.5, -0.5, 1.0]
                [0.5, 0.5, -0.5, 1.0][0.5, 0.5, 0.5, 1.0][-0.5, 0.5, -0.5, 1.0][-0.5, 0.5, 0.5, 1.0]

            faceUV[4] : float2 = [0.0, 0.0][1.0, 0.0][0.0, 1.0][1.0, 1.0]
        }
        const {
            modelTransform : matrix4
        }
        inout {
            texcoord : float2
            koeff : float4
        }
        vssrc {
            float3 cubeCenter = float3(instance_position_color.xyz);
            float3 toCamera = frame_cameraPosition.xyz - _transform(float4(cubeCenter, 1.0), const_modelTransform).xyz;
            float3 toCamSign = _sign(_transform(const_modelTransform, float4(toCamera, 0.0)).xyz);
            float4 relVertexPos = float4(toCamSign, 1.0) * fixed_cube[vertex_ID];
            float4 absVertexPos = float4(cubeCenter, 0.0) + relVertexPos;
            float3 faceNormal = toCamSign * fixed_axis[vertex_ID >> 2].xyz;

            int3 faceIndeces = int3(float3(2.0, 2.0, 2.0) * _step(0.0, relVertexPos.yzy) + _step(0.0, relVertexPos.zxx));
            
            float4 koeff = float4(0.0, 0.0, 0.0, 0.0);
            koeff = koeff + _step(0.5, -faceNormal.x) * instance_lightFaceNX;
            koeff = koeff + _step(0.5,  faceNormal.x) * instance_lightFacePX;
            koeff = koeff + _step(0.5, -faceNormal.y) * instance_lightFaceNY;
            koeff = koeff + _step(0.5,  faceNormal.y) * instance_lightFacePY;
            koeff = koeff + _step(0.5, -faceNormal.z) * instance_lightFaceNZ;
            koeff = koeff + _step(0.5,  faceNormal.z) * instance_lightFacePZ;
            
            float2 texcoord = float2(0.0, 0.0);
            texcoord = texcoord + _step(0.5, _abs(faceNormal.x)) * fixed_faceUV[faceIndeces.x];
            texcoord = texcoord + _step(0.5, _abs(faceNormal.y)) * fixed_faceUV[faceIndeces.y];
            texcoord = texcoord + _step(0.5, _abs(faceNormal.z)) * fixed_faceUV[faceIndeces.z];

            output_position = _transform(_transform(absVertexPos, const_modelTransform), frame_viewProjMatrix);
            output_texcoord = texcoord;
            output_koeff = koeff;
        }
        fssrc {
            float m0 = _lerp(input_koeff[0], input_koeff[1], input_texcoord.x);
            float m1 = _lerp(input_koeff[2], input_koeff[3], input_texcoord.x);
            float k = _pow(_smooth(0, 1, _lerp(m0, m1, input_texcoord.y)), 0.5);
            output_color = float4(k, k, k, 1.0);
        }
    )";
}

namespace voxel {
    struct StaticVoxelModel {
        voxel::Mesh mesh;
        foundation::RenderDataPtr voxelBuffer;
        foundation::RenderDataPtr positionBuffer;
    };
    
    class SceneImpl : public Scene {
    public:
        SceneImpl(const voxel::MeshFactoryPtr &factory, const foundation::RenderingInterfacePtr &rendering, const char *palette);
        ~SceneImpl() override;
        
        SceneObjectToken addStaticModel(const char *voxPath, const int16_t(&offset)[3]) override;
        SceneObjectToken addLightSource(const math::vector3f &position, float intensivity, const math::color &rgba) override;
        
        void updateAndDraw(float dtSec) override;
        
        const foundation::RenderDataPtr &getPositions() const override {
            return _singleModel.positionBuffer;
        }

        
    private:
        voxel::MeshFactoryPtr _factory;
        foundation::RenderingInterfacePtr _rendering;
        foundation::RenderShaderPtr _staticMeshShader;
        foundation::RenderShaderPtr _dynamicMeshShader;
        
        StaticVoxelModel _singleModel;
    };
    
    SceneImpl::SceneImpl(const voxel::MeshFactoryPtr &factory, const foundation::RenderingInterfacePtr &rendering, const char *palette) : _factory(factory), _rendering(rendering) {
        _staticMeshShader = rendering->createShader("static_voxel_mesh", g_staticMeshShaderSrc,
            { // vertex
                {"ID", foundation::RenderShaderInputFormat::ID}
            },
            { // instance
                {"position_color", foundation::RenderShaderInputFormat::SHORT4},
                {"lightFaceNX", foundation::RenderShaderInputFormat::BYTE4_NRM},
                {"lightFacePX", foundation::RenderShaderInputFormat::BYTE4_NRM},
                {"lightFaceNY", foundation::RenderShaderInputFormat::BYTE4_NRM},
                {"lightFacePY", foundation::RenderShaderInputFormat::BYTE4_NRM},
                {"lightFaceNZ", foundation::RenderShaderInputFormat::BYTE4_NRM},
                {"lightFacePZ", foundation::RenderShaderInputFormat::BYTE4_NRM},
            }
        );
    }
    
    SceneImpl::~SceneImpl() {
        
    }
    
    std::shared_ptr<Scene> Scene::instance(const voxel::MeshFactoryPtr &factory, const foundation::RenderingInterfacePtr &rendering, const char *palette) {
        return std::make_shared<SceneImpl>(factory, rendering, palette);
    }
    
    SceneObjectToken SceneImpl::addStaticModel(const char *voxPath, const int16_t(&offset)[3]) {
        if (_factory->createMesh(voxPath, offset, _singleModel.mesh)) {
            _singleModel.positionBuffer = _rendering->createData(_singleModel.mesh.frames[0].positions.get(), _singleModel.mesh.frames[0].voxelCount, sizeof(voxel::VoxelPosition));
            _singleModel.voxelBuffer = _rendering->createData(_singleModel.mesh.frames[0].voxels.get(), _singleModel.mesh.frames[0].voxelCount, sizeof(voxel::VoxelInfo));
        }
        return nullptr;
    }
    
    SceneObjectToken SceneImpl::addLightSource(const math::vector3f &position, float intensivity, const math::color &rgba) {
        return nullptr;
    }
    
    void SceneImpl::updateAndDraw(float dtSec) {
        _rendering->beginPass("static_voxel_model", _staticMeshShader); //, foundation::RenderPassConfig(0.8f, 0.775f, 0.75f)
        _rendering->drawGeometry(nullptr, _singleModel.voxelBuffer, 12, _singleModel.mesh.frames[0].voxelCount, foundation::RenderTopology::TRIANGLESTRIP);
        _rendering->endPass();
    }
}

