
#pragma once
#include "foundation/math.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace voxel {
    class YardThingImpl : public YardStatic, public std::enable_shared_from_this<YardThingImpl>, public YardInterface::Thing {
    public:
        YardThingImpl(const YardFacility &facility, std::uint64_t id, const math::vector3f &position, const math::bound3f &bbox, std::string &&model);
        ~YardThingImpl() override;

    public:
        void setPosition(const math::vector3f &position) override;
        auto getPosition() const -> const math::vector3f & override;
        auto getId() const -> std::uint64_t override;

        void updateState(YardLoadingState targetState) override;
        
    private:
        const std::string _modelPath;
        std::vector<SceneInterface::VTXSVOX> _voxData;
        math::vector3f _position = {0, 0, 0};
        
        SceneInterface::StaticModelPtr _model;
        SceneInterface::BoundingBoxPtr _bboxmdl;
    };
}

