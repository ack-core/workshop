
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "foundation/math.h"
#include "foundation/platform.h"

namespace dh {
    class Scope;
    using ScopePtr = std::shared_ptr<Scope>;
    using ScopeId = std::uint32_t;
    using EventToken = std::uint32_t;
    
    class WriteAccessor {
    public:
        virtual void setBool(const char *name, bool value) = 0;
        virtual void setNumber(const char *name, double value) = 0;
        virtual void setInteger(const char *name, int value) = 0;
        virtual void setString(const char *name, const std::string &value) = 0;
        virtual void setVector2f(const char *name, const math::vector2f &value) = 0;
        virtual void setVector3f(const char *name, const math::vector3f &value) = 0;
        
    public:
        virtual ~WriteAccessor() = default;
    };

    class Scope : public WriteAccessor {
    public:
        virtual auto getId() const -> ScopeId = 0;
        virtual auto getBool(const char *name) const -> bool = 0;
        virtual auto getInteger(const char *name) const -> int = 0;
        virtual auto getNumber(const char *name) const -> double = 0;
        virtual auto getString(const char *name) const -> const std::string & = 0;
        virtual auto getVector2f(const char *name) const -> const math::vector2f & = 0;
        virtual auto getVector3f(const char *name) const -> const math::vector3f & = 0;
        
        virtual void onBoolChanged(EventToken &handler, const char *name, std::function<void(const bool &)> &&f) = 0;
        virtual void onIntegerChanged(EventToken &handler, const char *name, std::function<void(const int &)> &&f) = 0;
        virtual void onNumberChanged(EventToken &handler, const char *name, std::function<void(const double &)> &&f) = 0;
        virtual void onStringChanged(EventToken &handler, const char *name, std::function<void(const std::string &)> &&f) = 0;
        virtual void onVector2fChanged(EventToken &handler, const char *name, std::function<void(const math::vector2f &)> &&f) = 0;
        virtual void onVector3fChanged(EventToken &handler, const char *name, std::function<void(const math::vector3f &)> &&f) = 0;
        virtual void onItemAdded(EventToken &handler, const char *arrayName, std::function<void(const ScopePtr &)> &&f) = 0;
        virtual void onItemRemoved(EventToken &handler, const char *arrayName, std::function<void(ScopeId)> &&f) = 0;
    
        virtual auto addItem(const char *arrayName, std::function<void(WriteAccessor&)> &&initializer) -> ScopePtr = 0;
        virtual void removeItem(const char *arrayName, const ScopePtr &scope) = 0;
        
    public:
        virtual ~Scope() = default;
    };

    class DataHub {
    public:
        // Description format:
        // s--------------------------------------
        // <scope name 1> {
        //     <element name 1> : <type> = <default value>
        //     <element name 2> : <type> = <default value>
        //     <element name 3> : array {
        //         <sub-element name 1> : <type> = <default value>
        //         <sub-element name 2> : <type> = <default value>
        //         ...
        //     }
        //     ...
        // }
        // <scope name 2> {
        //     ...
        // }
        // ...
        // s--------------------------------------
        // Types:
        // <name> : bool = true|false
        // <name> : integer = -2147483648 to 2147483647
        // <name> : number = 0.464 // for example
        // <name> : string = "" // quotes are neccessary
        // <name> : vector2f = 34.66 0.0 // values are separated by space, vector3f - the same
        //
        static std::shared_ptr<DataHub> instance(const foundation::LoggerInterfacePtr &logger, const char *src);
        
    public:
        virtual void update(float dtSec) = 0;

        virtual std::shared_ptr<Scope> getScope(ScopeId token) = 0;
        virtual std::shared_ptr<Scope> getRootScope(const char *rootScopeName) = 0;
        
    public:
        virtual ~DataHub() = default;
    };
    
    using DataHubPtr = std::shared_ptr<DataHub>;
    
    void testDataHub();
}

