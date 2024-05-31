
#pragma once
#include <initializer_list>

#include "contexts/context.h"
#include "contexts/debug_context.h"
#include "contexts/particle_editor_context.h"

namespace game {
    // Main datahub description
    static const char *datahub = R"(
        root {
            test1 : bool = true
            test2 : string = "test"
            testA : vector3f = 10 20 0
            test3 : array {
                test31 : bool = false
                test32 : string = "test"
                test34 : array {
                    test34X : number = 0
                }
            }
        }
        
        graphics {
            drawBoundBoxes : bool = true
            drawSimulation : bool = true
        }
    )";
    
    // Rule of states:
    // Context is created if the next state contains it and current state does not
    // Context is deleted if the next state does not contain it
    static const struct {
        const char *name;
        const std::initializer_list<MakeContextFunc> makers;
    }
    states[] = {
#ifdef IS_CLIENT
        {"default", {
            &makeContext<DebugContext>
        }}
#endif
#ifdef IS_PE
        {"default", {
            &makeContext<ParticleEditorContext>
        }}
#endif
    };
}

