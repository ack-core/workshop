
#pragma once
#include <memory>
#include "foundation/math.h"
#include "foundation/util.h"
#include "foundation/rendering.h"

namespace core {
    // Bakes particles emitter description into a map texture consumed by SceneInterface::addParticles
    //
    class EmitterBaker {
    public:
        enum class ShapeType {
            DISK = 1, // args: radius
            BOX,      // args: x-size, y-size, z-size
            MESH      // args: unused
        };
        enum class ShapeDistribution {
            RANDOM = 1,   // a random point from start shape to a random point of the end shape
            SHUFFLED,     // a random point from start shape to the corresponding point of the end shape
            LINEAR,       // the i-th point from start shape to the corresponding point of the end shape
        };
        enum class BakingTime {
            BAKE10 = 1,   // map frame time in ms
            BAKE20,
            BAKE50,
            BAKE100,
        };

    public:
        // Set emitter parameters. Missing values are zero/false/empty
        // @desc - emitter description:
        //         emissionTimeMs        : integer, duration of one emission cycle
        //         particleLifeTimeMs    : integer
        //         bakingFrameType       : integer, BakingTime value
        //         particlesToEmit       : integer, particles per emission cycle
        //         looped                : bool
        //         randomSeed            : integer
        //         startShapeType        : integer, ShapeType value
        //         startShapeFill        : bool, points fill the shape instead of its outline
        //         startShapeArgs        : vector3f, see ShapeType
        //         endShapeType          : integer, ShapeType value
        //         endShapeFill          : bool
        //         endShapeArgs          : vector3f
        //         endShapeOffset        : vector3f, end shape position relative to emitter position (start shape is always there), defines emitter direction
        //         shapeDistributionType : integer, ShapeDistribution value
        //         emissionGraphData     : string, graph of emitted particles share over cycle time
        //         widthGraphData        : string, graph of particle width over lifetime
        //         heightGraphData       : string, graph of particle height over lifetime
        //         speedGraphData        : string, graph of particle speed over lifetime
        //         alphaGraphData        : string, graph of particle alpha over lifetime
        //         Graph string format: "<time0> <value0> <spread0> <time1> <value1> <spread1> ..." with time in [0..1] ascending
        //
        virtual void setParameters(const util::Description &desc) = 0;
        virtual void setEndShapeOffset(const math::vector3f &offset) = 0;
        virtual void refresh() = 0;

        virtual auto getMap(const foundation::RenderingInterfacePtr &rendering) -> const foundation::RenderTexturePtr & = 0;
        virtual auto getMapRaw() const -> const std::uint8_t * = 0;
        virtual auto getMapWidth() const -> std::uint32_t = 0;
        virtual auto getMapHeight() const -> std::uint32_t = 0;
        virtual auto getDescription() const -> const util::Description & = 0;

        virtual auto getMinXYZ() const -> const math::vector3f & = 0;
        virtual auto getMaxXYZ() const -> const math::vector3f & = 0;
        virtual auto getMaxSize() const -> const math::vector2f & = 0;

        virtual ~EmitterBaker() = default;
    };

    using EmitterBakerPtr = std::shared_ptr<EmitterBaker>;

    EmitterBakerPtr makeEmitterBaker();
}
