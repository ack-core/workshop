
#pragma once
#include <unordered_map>

#include "contexts/context.h"
#include "contexts/debug_context.h"
#include "contexts/yard_editor_context.h"
#include "contexts/swordsman.h"

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
        
        additional {
            testA : vector3f = 10 20 0
        }
    )";
    
    // Rule of states:
    // Context is created if the next state contains it and current state does not
    // Context is deleted if the next state does not contain it
    static const std::unordered_map<const char *, std::vector<MakeContextFunc>> states = {
        {"default",
            {
                &makeContext<SwordsmanContext> //&makeContext<DebugContext>
            }
        },
    };
}

// TODO: multi-level state machine
