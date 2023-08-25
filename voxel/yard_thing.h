
#pragma once
#include "foundation/math.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace voxel {
    class YardThing : public YardStatic, public std::enable_shared_from_this<YardThing> {
    public:
        YardThing(const YardFacility &facility, const math::bound3f &bbox, std::string &&model);
        ~YardThing() override;

    public:
        void updateState(YardStatic::State targetState) override;
        
    private:
        const std::string _modelPath;
        std::vector<SceneInterface::VTXSVOX> _voxData;
        
        SceneInterface::StaticModelPtr _model;
        SceneInterface::BoundingBoxPtr _bboxmdl;
    };
}

