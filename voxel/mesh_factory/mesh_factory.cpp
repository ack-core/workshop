
#include "interfaces.h"
#include "mesh_factory.h"

namespace voxel {
    std::vector<Frame> loadModel(const std::shared_ptr<foundation::PlatformInterface> &platform, const char *fullPath, float offsetX, float offsetY, float offsetZ) {
        return {};
    }

    std::shared_ptr<foundation::RenderingTexture2D> loadPalette(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &renderingDevice, const char *fullPath) {
        return {};
    }
}

namespace voxel {
    StaticMeshImpl::StaticMeshImpl(std::shared_ptr<foundation::RenderingStructuredData> &&geometry) : _geometry(geometry) {

    }

    StaticMeshImpl::~StaticMeshImpl() {

    }

    void StaticMeshImpl::applyTransform(int x, int y, int z, Rotation rotation) {

    }

    const std::shared_ptr<foundation::RenderingStructuredData> &StaticMeshImpl::getGeometry() const {
        return _geometry;
    }
}

namespace voxel {
    DynamicMeshImpl::DynamicMeshImpl(std::shared_ptr<foundation::RenderingStructuredData> &&geometry) : _geometry(geometry) {
        ::memset(_transform, 0, 16 * sizeof(float));
    }

    DynamicMeshImpl::~DynamicMeshImpl() {

    }

    void DynamicMeshImpl::setTransform(const float(&position)[3], float rotationXZ) {

    }

    void DynamicMeshImpl::playAnimation(const char *name, std::function<void(DynamicMesh &)> &&finished, bool cycled) {

    }

    void DynamicMeshImpl::update(float dtSec) {

    }

    const float(&DynamicMeshImpl::getTransform() const)[16]{
        return _transform;
    }

    const std::shared_ptr<foundation::RenderingStructuredData> &DynamicMeshImpl::getGeometry() const {
        return _geometry;
    }
}

namespace voxel {
    MeshFactoryImpl::MeshFactoryImpl(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &rendering) {

    }

    MeshFactoryImpl::~MeshFactoryImpl() {

    }

    std::shared_ptr<StaticMesh> MeshFactoryImpl::createStaticMesh(const char *resourcePath) {
        return {};
    }

    std::shared_ptr<DynamicMesh> MeshFactoryImpl::createDynamicMesh(const char *resourcePath) {
        return {};
    }

    std::shared_ptr<StaticMesh> MeshFactoryImpl::mergeStaticMeshes(const std::vector<std::shared_ptr<StaticMesh>> &meshes) {
        return {};
    }
}

namespace voxel {
    std::shared_ptr<MeshFactory> MeshFactory::instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &rendering) {
        return std::make_shared<MeshFactoryImpl>(platform, rendering);
    }
}