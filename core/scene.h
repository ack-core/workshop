
#pragma once
#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/layouts.h"
#include "foundation/math.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace core {
    // Interface provides 3D-scene with camera and drawable objects. Scene keeps objects alive while user holds their pointers. 
    //
    class SceneInterface {
    public:
        static std::shared_ptr<SceneInterface> instance(
            const foundation::PlatformInterfacePtr &platform,
            const foundation::RenderingInterfacePtr &rendering
        );
        
    public:
        struct Arrows {
            virtual void setEnabled(bool enabled) = 0;
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual void setPosition(const math::vector3f &pos) = 0;
            virtual void setArrow(std::uint32_t index, const math::vector3f &start, const math::vector3f &end, const math::color &rgba) = 0;
            virtual void clear() = 0;
            virtual ~Arrows() = default;
        };
        struct LineSet {
            virtual void setEnabled(bool enabled) = 0;
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual void setPosition(const math::vector3f &pos) = 0;
            virtual void setLine(std::uint32_t index, const math::vector3f &start, const math::vector3f &end, const math::color &rgba) = 0;
            virtual void fillAsCircle(std::uint32_t segCount, float radius, const math::color &rgba) = 0;
            virtual void fillAsClosedPolygon(const std::vector<math::vector3f> &points, const math::color &rgba) = 0;
            virtual void clear() = 0;
            virtual ~LineSet() = default;
        };
        struct BoundingSphere {
            virtual void setEnabled(bool enabled) = 0;
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual void setPosition(const math::vector3f &pos) = 0;
            virtual void setRadius(float radius) = 0;
            virtual void setColor(const math::color &rgba) = 0;
            virtual ~BoundingSphere() = default;
        };
        struct BoundingBox {
            virtual void setEnabled(bool enabled) = 0;
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual void setPosition(const math::vector3f &pos) = 0;
            virtual void setBBox(const math::bound3f &bbox) = 0;
            virtual void setColor(const math::color &rgba) = 0;
            virtual ~BoundingBox() = default;
        };
        struct VoxelMesh {
            virtual void resetOffset() = 0;
            virtual auto getCenterOffset() const -> math::vector3f = 0;
            virtual void setCenterOffset(const math::vector3f& offset) = 0;
            virtual void setEnabled(bool enabled) = 0;
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual void setPosition(const math::vector3f &pos) = 0;
            virtual void setFrame(std::uint32_t index) = 0;
            virtual auto getFrameCount() const -> std::uint32_t = 0;
            virtual auto getDescription() const -> const util::Description & = 0;
            virtual ~VoxelMesh() = default;
        };
        struct GroundMesh {
            virtual void setEnabled(bool enabled) = 0;
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual void setPosition(const math::vector3f &pos) = 0;
            virtual ~GroundMesh() = default;
        };
        struct Vegetation {
            virtual void setEnabled(bool enabled) = 0;
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual void setPosition(const math::vector3f &pos) = 0;
            virtual ~Vegetation() = default;
        };
        struct Particles {
            virtual void setEnabled(bool enabled) = 0;
            virtual void setTransform(const math::transform3f &trfm) = 0;
            virtual void setPosition(const math::vector3f &pos) = 0;
            virtual void setTimings(float totalTimeSec, float fadingTimeSec, const std::vector<math::vector3f> &positionHistory) = 0;
            virtual ~Particles() = default;
        };
        struct CustomMesh {
            virtual void setEnabled(bool enabled) = 0;
            virtual void setTexture(const foundation::RenderTexturePtr &texture) = 0;
            virtual void setGeometry(foundation::RenderTopology topology, const void *data, std::uint32_t vcnt, const std::uint32_t *indexes = nullptr, std::uint32_t icnt = 0) = 0;
            virtual void setShaderConstants(const void *constants) = 0;
            virtual ~CustomMesh() = default;
        };

        using ArrowsPtr = std::shared_ptr<Arrows>;
        using LineSetPtr = std::shared_ptr<LineSet>;
        using BoundingSpherePtr = std::shared_ptr<BoundingSphere>;
        using BoundingBoxPtr = std::shared_ptr<BoundingBox>;
        using VoxelMeshPtr = std::shared_ptr<VoxelMesh>;
        using GroundMeshPtr = std::shared_ptr<GroundMesh>;
        using VegetationPtr = std::shared_ptr<Vegetation>;
        using ParticlesPtr = std::shared_ptr<Particles>;
        using CustomMeshPtr = std::shared_ptr<CustomMesh>;

        struct StatInfo {
            std::size_t arrowsCount = 0;
            std::size_t lineSetsCount = 0;
            std::size_t boundingSpheresCount = 0;
            std::size_t boundingBoxesCount = 0;
            std::size_t voxelMeshesCount = 0;
            std::size_t groundMeshesCount = 0;
            std::size_t vegetationsCount = 0;
            std::size_t particleEmittersCount = 0;
        };

    public:
        // Set camera
        // @position    - camera position
        // @sceneCenter - point camera looks at
        // @shift       - camera-space offset (right, up, forward) applied after look-at
        //
        virtual void setCameraLookAt(const math::vector3f &position, const math::vector3f &sceneCenter, const math::vector3f &shift = {0, 0, 0}) = 0;

        // Set directional light and fog
        // @dark         - unlit color, half of it is removable by AO
        // @sunDirection - direction to sun
        // @sunColor     - sun light color
        // @fogCenter    - point fog distance is measured from
        // @fogColor     - color of the fog and empty background
        // @fogRadius    - no fog closer than .x, full fog further than .y
        //
        virtual void setEnvironment(const math::color &dark, const math::vector3f &sunDirection, const math::color &sunColor, const math::vector3f &fogCenter, const math::color &fogColor, const math::vector2f &fogRadius) = 0;

        // Set of debug arrows, drawn if lines drawing enabled
        //
        virtual auto addArrows() -> ArrowsPtr = 0;

        // Set of debug lines, drawn if lines drawing enabled
        //
        virtual auto addLineSet() -> LineSetPtr = 0;

        // Debug wireframe sphere, drawn if lines drawing enabled
        //
        virtual auto addBoundingSphere(const math::vector3f &position, float radius, const math::color &rgba) -> BoundingSpherePtr = 0;

        // Debug wireframe box, drawn if lines drawing enabled
        //
        virtual auto addBoundingBox(const math::vector3f &position, const math::bound3f &bbox, const math::color &rgba) -> BoundingBoxPtr = 0;

        // Animated voxel model
        // @frames      - animation frames with VTXMVOX layout
        // @desc        - model description. Center offset (pivot in voxels) is taken from it
        //
        virtual auto addVoxelMesh(const std::vector<foundation::RenderDataPtr> &frames, const util::Description &desc) -> VoxelMeshPtr = 0;

        // Textured ground
        // @mesh        - geometry with VTXNRMUV layout
        // @texture     - R8UN palette indexes (same palette as voxel meshes)
        //
        virtual auto addGroundMesh(const foundation::RenderDataPtr &mesh, const foundation::RenderTexturePtr &texture) -> GroundMeshPtr = 0;

        // Textured vegetation (grass, foliage) placed by map
        // @map         - 4 bytes per cell
        // @margs       - (map width, map height, vegetation map index, 255 is empty)
        // @desc        - vegetation description
        // @tx          - R8UN palette indexes (same palette as voxel meshes), 0 is transparent
        //
        virtual auto addVegetation(const ByteDataPtr &map, const math::vector3i &margs, const util::Description &desc, const foundation::RenderTexturePtr &tx) -> VegetationPtr = 0;

        // Voxel vegetation (trunks, stones) placed by map
        // @vxm         - voxel meshes as (voxel count, voxels). Mesh is chosen by map cell
        //
        virtual auto addVegetation(const ByteDataPtr &map, const math::vector3i &margs, const util::Description &desc, const std::vector<std::pair<std::uint32_t, ByteDataPtr>> &vxm) -> VegetationPtr = 0;

        // Particles emitter. 
        // @tx          - particle texture
        // @map         - baked particles map, VERTICAL_PIXELS_PER_PARTICLE rows per particle
        // @desc        - emitter description
        //
        virtual auto addParticles(const foundation::RenderTexturePtr &tx, const foundation::RenderTexturePtr &map, const util::Description &desc) -> ParticlesPtr = 0;

        // Mesh with user geometry, shader and optional texture (texture slot 0)
        // @layout          - vertex layout of geometry
        // @shaderSource    - shader with optional 'const' block, data is copied by setShaderConstants
        // @renderToGBuffer - true: drawn into gbuffer and lit like voxel meshes. Shader must write float4(normal * 0.5 + 0.5, paletteIndex / 255.0) to output_color[0]
        //                    false: drawn straight to the screen with alpha blending and gbuffer depth
        //
        virtual auto addCustomMesh(const foundation::InputLayout &layout, const char *shaderSource, bool renderToGBuffer) -> CustomMeshPtr = 0;

        // Camera properties in world space
        //
        virtual auto getCameraPosition() const -> math::vector3f = 0;
        virtual auto getCameraDirection() const -> math::vector3f = 0;
        virtual auto getCameraRight() const -> math::vector3f = 0;

        // Project world point to screen
        // @return - screen position in pixels or numeric_limits<float>::min() if point is behind camera
        //
        virtual auto getScreenCoordinates(const math::vector3f &worldPosition) const -> math::vector2f = 0;

        // Unproject screen point
        // @screenPosition - position in pixels
        // @outCamPosition - optional camera position
        // @return         - normalized world direction from camera
        //
        virtual auto getWorldDirection(const math::vector2f &screenPosition, math::vector3f *outCamPosition = nullptr) const -> math::vector3f = 0;

        // Remove released objects and draw frame
        //
        virtual void updateAndDraw(float dtSec) = 0;

        // Enable drawing of debug objects: arrows, lines, bounding volumes
        //
        virtual void setLinesDrawingEnabled(bool enabled) = 0;

        // Enable ambient occlusion in gbuffer lighting pass. Enabled by default
        //
        virtual void setAOEnabled(bool enabled) = 0;

        // Draw gbuffer geometry as triangle edges
        //
        virtual void setWireframeEnabled(bool enabled) = 0;

        // Returns counters usable in statistics and testing. Released objects are counted until next updateAndDraw
        //
        virtual auto getStatistics() const -> StatInfo = 0;

    public:
        virtual ~SceneInterface() = default;
    };
    
    using SceneInterfacePtr = std::shared_ptr<SceneInterface>;
}

