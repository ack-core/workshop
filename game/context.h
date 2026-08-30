
#pragma once
#include <memory>

#include "providers/resource_provider.h"
#include "core/scene.h"
#include "core/world.h"
#include "core/raycast.h"
#include "ui/stage.h"
#include "game/state_manager.h"
#include "datahub/datahub.h"

namespace game {
    struct API {
        const game::StateManagerPtr stateManager;
        const foundation::PlatformInterfacePtr &platform;
        const resource::ResourceProviderPtr &resources;
        const resource::FontAtlasProviderPtr &fonts;
        const core::SceneInterfacePtr &scene;
        const core::WorldInterfacePtr &world;
        const core::RaycastInterfacePtr &raycast;
        const ui::StageInterfacePtr &ui;
    };
    
    class Interface {
    public:
        virtual ~Interface() = default;
    };

    class Context {
        template <typename Ctx, typename... Interfaces> friend std::shared_ptr<Context> makeContext(API &&api, Interface **existInterfaces, std::size_t count);
        
    public:
        virtual void init() = 0;
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~Context() = default;
        
    protected:
        std::weak_ptr<Context> thisweak;
    };

    template <typename I> I& makeArg(Interface **existInterfaces, std::size_t count) {
        I *ptr = nullptr;
        
        for (std::size_t i = 0; i < count; i++) {
            if ((ptr = dynamic_cast<I *>(existInterfaces[i])) != nullptr) {
                break;
            }
        }
        
        return *ptr;
    }

    template <typename Ctx, typename... Interfaces> std::shared_ptr<Context> makeContext(API &&api, Interface **existInterfaces, std::size_t count) {
        auto result = std::make_shared<Ctx>(std::move(api), makeArg<Interfaces>(existInterfaces, count)...);
        result->thisweak = result;
        result->init();
        return result;
    }
    
    using MakeContextFunc = std::shared_ptr<Context>(*)(API &&api, Interface **existInterfaces, std::size_t count);
}
