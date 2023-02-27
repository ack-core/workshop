
#pragma once
#include <memory>

#include "voxel/scene.h"
#include "voxel/yard.h"
#include "ui/stage.h"
#include "datahub/datahub.h"

namespace game {
    struct API {
        const foundation::PlatformInterfacePtr platform;
        const voxel::SceneInterfacePtr scene;
        const voxel::YardInterfacePtr yard;
        const ui::StageInterfacePtr ui;
        const dh::DataHubPtr dh;
    };
    
    class Context {
    public:
        virtual void update(float dtSec) = 0;
    
    public:
        virtual ~Context() = default;
    };

    template <typename Ctx> std::unique_ptr<Context> makeContext(API &&api);
    using MakeContextFunc = std::unique_ptr<Context>(*)(API &&api);
}
