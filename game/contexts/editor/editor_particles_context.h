
#pragma once
#include <game/context.h>
#include <unordered_map>

namespace game {
    class EditorParticlesContext : public Context {
    public:
        EditorParticlesContext(API &&api);
        ~EditorParticlesContext() override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
    };
}
