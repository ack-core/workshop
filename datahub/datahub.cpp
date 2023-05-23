
#include "datahub.h"

#include <memory>
#include <variant>
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <sstream>

// TODO: tests

namespace {
    template <typename = void> std::istream &expect(std::istream &stream) {
        return stream;
    }
    template <char Ch, char... Chs> std::istream &expect(std::istream &stream) {
        if ((stream >> std::ws).peek() == Ch) {
            stream.ignore();
        }
        else {
            stream.setstate(std::ios_base::failbit);
        }
        return expect<Chs...>(stream);
    }
    
    bool readUntilMatchingBrace(std::istringstream &stream, char start, char end, std::string &output) {
        output.clear();
        stream >> std::ws;
        int  braceCounter = 1;
        char next;

        while (((next = stream.get()) != end || --braceCounter > 0) && stream) {
            if (next == start) braceCounter++;
            output += next;
            if (next == '\n')  stream >> std::ws;
        }
        
        return braceCounter == 0;
    };

    bool readUntilChar(std::istringstream &stream, char ch, std::string &output) {
        char next;
        while ((next = stream.get()) && stream) {
            output += next;
            if (next == ch) break;
        }
    };
}

namespace {
    using ElementIndex = std::uint8_t;
    using TemplateIndex = std::uint8_t;
    
    static const std::size_t MAX_STRING_LENGTH = 1024;
    static const std::size_t MAX_ELEMENTS_PER_SCOPE = 254;
    static const std::size_t MAX_SCOPE_TEMPLATES = 254;
    
    dh::EventToken getNextEventToken() {
        static dh::EventToken g_nextEventToken = 0x10;
        return g_nextEventToken++;
    }

    dh::ScopeId getNextScopeId() {
        static dh::ScopeId g_nextScopeId = 0x800000;
        return g_nextScopeId = (g_nextScopeId + 1) & 0xffffff;
    }

    std::uint64_t getCurrentTimeStamp() {
        return 0;
    }

    struct Location {
        std::uint32_t index : 8;
        std::uint32_t scopeId : 24;
    };
    
    struct MsgHeader {
        std::uint8_t cmd;
        std::uint64_t timestamp;
        Location location;
    };
    
    class MemoryQueue {
        struct Msg {
            MsgHeader header;
            std::unique_ptr<std::uint8_t[]> data;
            std::size_t length;
        };
    
    public:
        void enqueue( const MsgHeader &header, const void *data, std::size_t length ) {
            std::lock_guard<std::mutex> guard(_mutex);
            std::unique_ptr<std::uint8_t[]> m = std::make_unique<std::uint8_t[]>(length);
            memcpy(m.get(), data, length);
            _queue.emplace(Msg{header, std::move(m), length});
        }
        
        bool dequeue( MsgHeader &header, std::unique_ptr<std::uint8_t[]> &data, std::size_t &length ) {
            std::lock_guard<std::mutex> guard(_mutex);
            
            if (_queue.empty() == false) {
                header = _queue.front().header;
                data = std::move(_queue.front().data);
                length = _queue.front().length;
                _queue.pop();
            }
            
            return false;
        }
        
    private:
        std::mutex _mutex;
        std::queue<Msg> _queue;
    };
}

namespace dh {
    class ScopeImpl;
    class DataHubImpl;
    
    const std::uint8_t MSG_TYPE_VALUE_CHANGED = 0x1;
    const std::uint8_t MSG_TYPE_ITEM_ADDED = 0x2;
    const std::uint8_t MSG_TYPE_ITEM_REMOVED = 0x3;
    
    template<typename Type> struct Value {
        Type value;
        std::vector<std::pair<EventToken, std::function<void(const Type &)>>> onChangedHandlers;
    };
    
    struct Array {
        Array(TemplateIndex templateIndex) : scopeTemplateIndex(templateIndex) {}
        
        const TemplateIndex scopeTemplateIndex;
        std::unordered_map<ScopeId, std::shared_ptr<ScopeImpl>> items;
        std::vector<std::pair<EventToken, std::function<void(const ScopePtr &)>>> onAddedHandlers;
        std::vector<std::pair<EventToken, std::function<void(ScopeId)>>> onRemoveHandlers;
    };
    
    struct Element {
        std::variant<Value<bool>, Value<int>, Value<double>, Value<std::string>, Value<math::vector2f>, Value<math::vector3f>, Array> data;
        
        Element(bool value) {
            data.emplace<Value<bool>>(Value<bool>{value});
        }
        Element(int value) {
            data.emplace<Value<int>>(Value<int>{value});
        }
        Element(double value) {
            data.emplace<Value<double>>(Value<double>{value});
        }
        Element(const char *value) {
            data.emplace<Value<std::string>>(Value<std::string>{value});
        }
        Element(const math::vector2f &value) {
            data.emplace<Value<math::vector2f>>(Value<math::vector2f>{value});
        }
        Element(const math::vector3f &value) {
            data.emplace<Value<math::vector3f>>(Value<math::vector3f>{value});
        }
        Element(const Array &array) {
            data.emplace<Array>(Array(array.scopeTemplateIndex));
        }

        template<typename... Ls> static void visit(Element *element, Ls&&... ls) {
            struct ElementVisitor : Ls... { using Ls::operator()...; };
            std::visit(ElementVisitor{ls...}, element->data);
        }
    };

    using ScopeTemplate = std::vector<std::pair<std::string, Element>>;
    using TemplateArray = std::vector<std::pair<std::string, ScopeTemplate>>;
}

namespace dh {
    class ScopeImpl : public Scope {
    public:
        ScopeImpl(foundation::LoggerInterface &logger, ScopeId scopeId, TemplateIndex templateIndex, const TemplateArray &scopeTemplates, MemoryQueue &queue);
        ~ScopeImpl() = default;
        
        auto getId() const -> ScopeId override { return _id; }
        
        void setBool(const char *name, bool value) override { _setValue(name, value); }
        void setNumber(const char *name, double value) override { _setValue(name, value); }
        void setInteger(const char *name, int value) override { _setValue(name, value); }
        void setString(const char *name, const std::string &value) override { _setValue(name, value); }
        void setVector2f(const char *name, const math::vector2f &value) override { _setValue(name, value); }
        void setVector3f(const char *name, const math::vector3f &value) override { _setValue(name, value); }
        
        auto getBool(const char *name) const -> bool override { return _getValue<bool>(name); }
        auto getInteger(const char *name) const -> int override { return _getValue<int>(name); }
        auto getNumber(const char *name) const -> double override { return _getValue<double>(name); }
        auto getString(const char *name) const -> const std::string & override { return _getValue<std::string>(name); }
        auto getVector2f(const char *name) const -> const math::vector2f & override { return _getValue<math::vector2f>(name); }
        auto getVector3f(const char *name) const -> const math::vector3f & override { return _getValue<math::vector3f>(name); }
        
        void onBoolChanged(EventToken &h, const char *name, std::function<void(const bool &)> &&f) override { _setOnChangedHandler(h, name, std::move(f)); }
        void onIntegerChanged(EventToken &h, const char *name, std::function<void(const int &)> &&f) override { _setOnChangedHandler(h, name, std::move(f)); }
        void onNumberChanged(EventToken &h, const char *name, std::function<void(const double &)> &&f) override { _setOnChangedHandler(h, name, std::move(f)); }
        void onStringChanged(EventToken &h, const char *name, std::function<void(const std::string &)> &&f) override { _setOnChangedHandler(h, name, std::move(f)); }
        void onVector2fChanged(EventToken &h, const char *name, std::function<void(const math::vector2f &)> &&f) override { _setOnChangedHandler(h, name, std::move(f)); }
        void onVector3fChanged(EventToken &h, const char *name, std::function<void(const math::vector3f &)> &&f) override { _setOnChangedHandler(h, name, std::move(f)); }
        
        void onItemAdded(EventToken &handler, const char *arrayName, std::function<void(const ScopePtr &)> &&f) override;
        void onItemRemoved(EventToken &handler, const char *arrayName, std::function<void(ScopeId)> &&f) override;
        
        void removeOnChangedHandler(const char *name, EventToken handler);
        void removeOnItemAddedHandler(const char *arrayName, EventToken handler);
        void removeOnItemRemovedHandler(const char *arrayName, EventToken handler);
        
        auto addItem(const char *arrayName, std::function<void(WriteAccessor&)> &&initializer) -> ScopePtr override;
        void removeItem(const char *arrayName, const ScopePtr &scope) override;
        
        TemplateIndex getTemplateIndex() const { return _scopeTemplateIndex; }
        Element *getElementAt(ElementIndex index) { return index < _elements.size() ? &_elements[index].second : nullptr; }
        void applyChanges(const std::uint8_t *data, std::size_t length);
        
    private:
        template<typename T> void _setOnChangedHandler(EventToken &handler, const char *name, std::function<void(const T &)> &&f);
        template<typename T> void _setValue(const char *name, const T &value);
        template<typename T> const T &_getValue(const char *name) const;
        void _setValue(const char *name, const std::string &value);
        
        const ScopeId _id;
        const TemplateIndex _scopeTemplateIndex;
        const TemplateArray &_scopeTemplates;
        
        foundation::LoggerInterface &_logger;
        MemoryQueue &_queue;
        std::vector<std::pair<std::string, Element>> _elements;
    };
}

namespace dh {
    ScopeImpl::ScopeImpl(foundation::LoggerInterface &logger, ScopeId scopeId, TemplateIndex templateIndex, const TemplateArray &scopeTemplates, MemoryQueue &queue)
    : _logger(logger)
    , _id(scopeId)
    , _scopeTemplateIndex(templateIndex)
    , _scopeTemplates(scopeTemplates)
    , _queue(queue)
    , _elements(scopeTemplates[templateIndex].second)
    {}
    
    void ScopeImpl::onItemAdded(EventToken &handler, const char *arrayName, std::function<void(const ScopePtr &)> &&f) {
        for (auto &element : _elements) {
            if (element.first == arrayName) {
                if (Array *array = std::get_if<Array>(&element.second.data)) {
                    handler = getNextEventToken();
                    array->onAddedHandlers.emplace_back(std::make_pair(handler, std::move(f)));
                }
                else {
                    _logger.logError("[ScopeImpl::onItemAdded] '%s' is not an array\n", arrayName);
                }
                
                return;
            }
        }
        
        _logger.logError("[ScopeImpl::onItemAdded] Array '%s' not found\n", arrayName);
    }
    
    void ScopeImpl::onItemRemoved(EventToken &handler, const char *arrayName, std::function<void(ScopeId)> &&f) {
        for (auto &element : _elements) {
            if (element.first == arrayName) {
                if (Array *array = std::get_if<Array>(&element.second.data)) {
                    handler = getNextEventToken();
                    array->onRemoveHandlers.emplace_back(std::make_pair(handler, std::move(f)));
                }
                else {
                    _logger.logError("[ScopeImpl::onItemRemoved] '%s' is not an array\n", arrayName);
                }
                
                return;
            }
        }
        
        _logger.logError("[ScopeImpl::onItemRemoved] Array '%s' not found\n", arrayName);
    }
    
    ScopePtr ScopeImpl::addItem(const char *arrayName, std::function<void(WriteAccessor&)> &&initializer) {
        for (ElementIndex i = 0; i <_elements.size(); i++) {
            if (_elements[i].first == arrayName) {
                std::shared_ptr<ScopeImpl> result = nullptr;
                
                if (Array *array = std::get_if<Array>(&_elements[i].second.data)) {
                    ScopeId newId = getNextScopeId();
                    result = std::make_shared<ScopeImpl>(_logger, newId, _scopeTemplateIndex, _scopeTemplates, _queue);
                    initializer(*result);
                    array->items.emplace(newId, result);
                    _queue.enqueue(MsgHeader{MSG_TYPE_ITEM_ADDED, getCurrentTimeStamp(), Location{i, _id}}, &newId, sizeof(newId));
                }
                else {
                    _logger.logError("[ScopeImpl::addItem] '%s' is not an array\n", arrayName);
                }
                
                return result;
            }
        }
        
        _logger.logError("[ScopeImpl::addItem] Array '%s' not found\n", arrayName);
        return nullptr;
    }
    
    void ScopeImpl::removeItem(const char *arrayName, const ScopePtr &scope) {
        for (ElementIndex i = 0; i <_elements.size(); i++) {
            if (_elements[i].first == arrayName) {
                if (const Array *array = std::get_if<Array>(&_elements[i].second.data)) {
                    ScopeId id = scope->getId();
                    _queue.enqueue(MsgHeader{MSG_TYPE_ITEM_REMOVED, getCurrentTimeStamp(), Location{i, _id}}, &id, sizeof(id));
                }
                else {
                    _logger.logError("[ScopeImpl::removeItem] '%s' is not an array\n", arrayName);
                }
                
                return;
            }
        }
        
        _logger.logError("[ScopeImpl::removeItem] Array '%s' not found\n", arrayName);
    }
    
    void ScopeImpl::applyChanges(const std::uint8_t *data, std::size_t length) {
        std::size_t offset = 0;
        
        while (offset < length) {
            ElementIndex index = *(ElementIndex *)(data + offset);
            const std::uint8_t *valueptr = data + offset + sizeof(ElementIndex);
            offset += sizeof(ElementIndex);
            
            Element::visit(&_elements[index].second,
                [valueptr, &offset](auto &v) {
                    v.value = *(decltype(v.value) *)(valueptr);
                    offset += sizeof(v.value);
                },
                [valueptr, &offset](Value<std::string> &v) {
                    const char *str = (const char *)(valueptr + sizeof(std::uint16_t));
                    std::size_t length = *(std::uint16_t *)(valueptr);
                    v.value = std::string(str, length);
                    offset += length + sizeof(std::uint16_t);
                },
                [this, &offset, &length](Array &) {
                    _logger.logError("[ScopeImpl::applyChanges] Array cannot be deserialized\n");
                    offset = length;
                }
            );
        }
    }
    
    template<typename T> void ScopeImpl::_setOnChangedHandler(EventToken &handler, const char *name, std::function<void(const T &)> &&f) {
        for (auto &element : _elements) {
            if (element.first == name) {
                if (Value<T> *ptr = std::get_if<Value<T>>(&element.second.data)) {
                    handler = getNextEventToken();
                    ptr->onChangedHandlers.emplace_back(std::make_pair(handler, std::move(f)));
                }
                else {
                    _logger.logError("[ScopeImpl::setOnChangedHandler] '%s' is not a value\n", name);
                }
                
                return;
            }
        }
        
        _logger.logError("[ScopeImpl::setOnChangedHandler] Value '%s' not found\n", name);
    }
    
    template<typename T> void ScopeImpl::_setValue(const char *name, const T &value) {
        for (ElementIndex i = 0; i <_elements.size(); i++) {
            if (_elements[i].first == name) {
                if (std::holds_alternative<Value<T>>(_elements[i].second.data)) {
                    _queue.enqueue(MsgHeader{MSG_TYPE_VALUE_CHANGED, getCurrentTimeStamp(), Location{i, _id}}, &value, sizeof(T));
                }
                else {
                    _logger.logError("[ScopeImpl::setValue] '%s' is not a value\n", name);
                }
                
                return;
            }
        }

        _logger.logError("[ScopeImpl::setValue] Value '%s' not found\n", name);
    }

    void ScopeImpl::_setValue(const char *name, const std::string &value) {
        for (ElementIndex i = 0; i <_elements.size(); i++) {
            if (_elements[i].first == name) {
                if (std::holds_alternative<Value<std::string>>(_elements[i].second.data)) {
                    if (value.length() <= MAX_STRING_LENGTH) {
                        std::uint8_t buffer[MAX_STRING_LENGTH + sizeof(std::uint16_t)];
                        *(std::uint16_t *)(buffer) = value.length();
                        memcpy(buffer + sizeof(std::uint16_t), value.data(), value.length());
                        _queue.enqueue(MsgHeader{MSG_TYPE_VALUE_CHANGED, getCurrentTimeStamp(), Location{i, _id}}, buffer, sizeof(std::uint16_t) + value.length());
                    }
                    else {
                        _logger.logError("[ScopeImpl::setValue] String is too large to asign to '%s'\n", name);
                    }
                }
                else {
                    _logger.logError("[ScopeImpl::setValue] '%s' is not a value\n", name);
                }
                
                return;
            }
        }
        
        _logger.logError("[ScopeImpl::setValue] Value '%s' not found\n", name);
    }

    template<typename T> const T &ScopeImpl::_getValue(const char *name) const {
        for (auto &element : _elements) {
            if (element.first == name) {
                if (const Value<T> *ptr = std::get_if<Value<T>>(&element.second.data)) {
                    return ptr->value;
                }
                else {
                    _logger.logError("[ScopeImpl::getValue] '%s' is not a value\n", name);
                    return;
                }
            }
        }
        
        _logger.logError("[ScopeImpl::getValue] Value '%s' not found\n", name);
    }

}


namespace dh {
    class DataHubImpl : public DataHub, public std::enable_shared_from_this<DataHubImpl> {
    public:
        DataHubImpl(const foundation::LoggerInterfacePtr &logger);
        ~DataHubImpl();
        
        void initialize(const char *src);
        void update(float dtSec) override;
        
        std::shared_ptr<Scope> getScope(ScopeId id) override;
        std::shared_ptr<Scope> getRootScope(const char *rootScopeName) override;
        
    private:
        const foundation::LoggerInterfacePtr _logger;
        std::vector<std::pair<std::string, ScopeTemplate>> _scopeTemplates;
        std::unordered_map<ScopeId, std::shared_ptr<ScopeImpl>> _scopes;
        std::unordered_map<std::string, std::shared_ptr<ScopeImpl>> _roots;
        
        MemoryQueue _queue;
    };
    
    std::shared_ptr<DataHub> DataHub::instance(const foundation::LoggerInterfacePtr &logger, const char *src) {
        std::shared_ptr<DataHubImpl> result = std::make_shared<DataHubImpl>(logger);
        result->initialize(src);
        return result;
    }
}

namespace dh {
    DataHubImpl::DataHubImpl(const foundation::LoggerInterfacePtr &logger) : _logger(logger) {}
    DataHubImpl::~DataHubImpl() {}
    
    void DataHubImpl::initialize(const char *src) {
        std::istringstream input(src);
        
        struct fn {
            static bool parseScope(foundation::LoggerInterface &logger, const std::string &name, const std::string &src, TemplateArray &templates) {
                std::istringstream input(src);
                std::string elementName;
                ScopeTemplate result;
                
                while (input >> elementName) {
                    std::string type;
                    std::string valueString;
                    
                    if (result.size() < MAX_ELEMENTS_PER_SCOPE) {
                        if (input >> expect<':'> >> type) {
                            if (type != "array") {
                                if (input >> expect<'='> >> valueString) {
                                    if (type == "bool") {
                                        if (valueString == "true" || valueString == "false") {
                                            result.emplace_back(std::make_pair(elementName, Element(valueString == "true")));
                                            continue;
                                        }

                                        logger.logError("[DataHubImpl::initialize] Bad value for '%s' in '%s'\n", elementName.data(), name.data());
                                    }
                                    else if (type == "integer") {
                                        int value = 0;
                                        if (sscanf(valueString.data(), "%d", &value) == 1) {
                                            result.emplace_back(std::make_pair(elementName, Element(value)));
                                            continue;
                                        }

                                        logger.logError("[DataHubImpl::initialize] Bad value for '%s' in '%s'\n", elementName.data(), name.data());
                                    }
                                    else if (type == "number") {
                                        double value = 0;
                                        if (sscanf(valueString.data(), "%lf", &value) == 1) {
                                            result.emplace_back(std::make_pair(elementName, Element(value)));
                                            continue;
                                        }

                                        logger.logError("[DataHubImpl::initialize] Bad value for '%s' in '%s'\n", elementName.data(), name.data());
                                    }
                                    else if (type == "string") {
                                        if (valueString[valueString.size() - 1] != '"') {
                                            readUntilChar(input, '"', valueString);
                                        }
                                        if (valueString[0] == '"' && valueString[valueString.size() - 1] == '"') {
                                            if (valueString.length() <= MAX_STRING_LENGTH) {
                                                valueString.pop_back();
                                                result.emplace_back(std::make_pair(elementName, Element(valueString.data() + 1)));
                                                continue;
                                            }
                                        }

                                        logger.logError("[DataHubImpl::initialize] Bad value for '%s' in '%s'\n", elementName.data(), name.data());
                                    }
                                    else if (type == "vector2f") {
                                        std::string str1;
                                        math::vector2f value;

                                        if (input >> str1) {
                                            if (sscanf((valueString + " " + str1).data(), "%f %f", &value.x, &value.y) == 2) {
                                                result.emplace_back(std::make_pair(elementName, Element(value)));
                                                continue;
                                            }
                                        }

                                        logger.logError("[DataHubImpl::initialize] Bad value for '%s' in '%s'\n", elementName.data(), name.data());
                                    }
                                    else if (type == "vector3f") {
                                        std::string str1, str2;
                                        math::vector3f value;

                                        if (input >> str1 >> str2) {
                                            if (sscanf((valueString + " " + str1 + " " + str2).data(), "%f %f %f", &value.x, &value.y, &value.z) == 3) {
                                                result.emplace_back(std::make_pair(elementName, Element(value)));
                                                continue;
                                            }
                                        }

                                        logger.logError("[DataHubImpl::initialize] Bad value for '%s' in '%s'\n", elementName.data(), name.data());
                                    }
                                    else {
                                        logger.logError("[DataHubImpl::initialize] Unknown type '%s' in '%s'\n", type.data(), name.data());
                                    }
                                }
                                else {
                                    logger.logError("[DataHubImpl::initialize] Expected '= <value>' after '%s' in '%s'\n", elementName.data(), name.data());
                                }
                            }
                            else if (input >> expect<'{'>) {
                                if (readUntilMatchingBrace(input, '{', '}', valueString)) {
                                    if (parseScope(logger, name + "." + elementName, valueString, templates)) {
                                        TemplateIndex scopeTemplateIndex = static_cast<TemplateIndex>(templates.size() - 1);
                                        result.emplace_back(std::make_pair(elementName, Element(Array(scopeTemplateIndex))));
                                        continue;
                                    }
                                }
                                else {
                                    logger.logError("[DataHubImpl::initialize] Braces aren't match for '%s' in '%s'\n", elementName.data(), name.data());
                                }
                            }
                            else {
                                logger.logError("[DataHubImpl::initialize] Expected '{' after array '%s' in '%s'\n", elementName.data(), name.data());
                            }
                        }
                        else {
                            logger.logError("[DataHubImpl::initialize] Expected ':' after name '%s' in '%s'\n", elementName.data(), name.data());
                        }
                    }
                    else {
                        logger.logError("[DataHubImpl::initialize] Too many elements in scope '%s'\n", name.data());
                    }

                    return false;
                }
                
                if (templates.size() >= MAX_SCOPE_TEMPLATES) {
                    logger.logError("[DataHubImpl::initialize] Too many scope templates\n");
                    return false;
                }

                templates.emplace_back(name, std::move(result));
                return true;
            }
        };

        std::string name;
        std::string scopeText;

        while (input >> name) {
            if (input >> expect<'{'>) {
                if (readUntilMatchingBrace(input, '{', '}', scopeText)) {
                    if (fn::parseScope(*_logger, name, scopeText, _scopeTemplates) == false) {
                        _scopeTemplates.clear();
                        break;
                    }
                }
                else {
                    _logger->logError("[DataHubImpl::initialize] Braces aren't match for '%s'\n", name.data());
                    break;
                }
            }
            else {
                _logger->logError("[DataHubImpl::initialize] Expected '{' after scope name '%s'\n", name.data());
                break;
            }
        }
        
        for (TemplateIndex i = 0; i < _scopeTemplates.size(); i++) {
            if (_scopeTemplates[i].first.find('.') == std::string::npos) {
                ScopeId id = getNextScopeId();
                std::shared_ptr<ScopeImpl> scope = std::make_shared<ScopeImpl>(*_logger, id, i, _scopeTemplates, _queue);
                _roots.emplace(_scopeTemplates[i].first, std::move(scope));
            }
        }
        
        if (_roots.empty()) {
            _logger->logError("[DataHubImpl::initialize] Datahub is empty\n");
        }
    }
    
    void DataHubImpl::update(float dtSec) {
        MsgHeader header {0};
        std::unique_ptr<std::uint8_t[]> data = nullptr;
        std::size_t length = 0;
        
        if (_queue.dequeue(header, data, length)) {
            auto scope = _scopes.find(header.location.scopeId);
            if (scope != _scopes.end()) {
                if (Element *element = scope->second->getElementAt(header.location.index)) {
                    if (header.cmd == MSG_TYPE_VALUE_CHANGED) {
                        Element::visit(element,
                            [&data](auto &v) {
                                v.value = *(decltype(v.value) *)data.get();
                                for (auto &handler : v.onChangedHandlers) {
                                    handler.second(v.value);
                                }
                            },
                            [&data](Value<std::string> &v) {
                                std::size_t length = *(std::uint16_t *)data.get();
                                const char *str = (const char *)(data.get() + sizeof(std::uint16_t));
                                v.value = std::string(str, length);
                                for (auto &handler : v.onChangedHandlers) {
                                    handler.second(v.value);
                                }
                            },
                            [this](Array &) {
                                _logger->logError("[DataHubImpl::update] Received MSG_TYPE_VALUE_CHANGED for array\n");
                            }
                        );
                    }
                    else if (header.cmd == MSG_TYPE_ITEM_ADDED) {
                        if (Array *array = std::get_if<Array>(&element->data)) {
                            const ScopeId itemId = *reinterpret_cast<ScopeId *>(data.get());
                            const auto &index = _scopes.find(itemId);
                            
                            if (index != _scopes.end()) {
                                for (auto &handler : array->onAddedHandlers) {
                                    handler.second(index->second);
                                }
                            }
                            else {
                                _logger->logError("[DataHubImpl::update] Received MSG_TYPE_ITEM_ADDED for unknown item with id = %d\n", int(itemId));
                            }
                        }
                        else {
                            _logger->logError("[DataHubImpl::update] Received MSG_TYPE_ITEM_ADDED for value\n");
                        }
                    }
                    else if (header.cmd == MSG_TYPE_ITEM_REMOVED) {
                        if (Array *array = std::get_if<Array>(&element->data)) {
                            ScopeId itemId = *reinterpret_cast<ScopeId *>(data.get());
                            if (array->items.erase(itemId) == 1) {
                                for (auto &handler : array->onRemoveHandlers) {
                                    handler.second(itemId);
                                }
                            }
                            else {
                                _logger->logError("[DataHubImpl::update] Received MSG_TYPE_ITEM_REMOVED for unknown item with id = %d\n", int(itemId));
                            }
                        }
                        else {
                            _logger->logError("[DataHubImpl::update] Received MSG_TYPE_ITEM_REMOVED for value\n");
                        }
                    }
                    else {
                        _logger->logError("[DataHubImpl::update] Unknown command\n");
                    }
                }
                else {
                    _logger->logError("[DataHubImpl::update] Invalid index = %d in templateIndex = %d\n", int(header.location.index), int(scope->second->getTemplateIndex()));
                }
            }
            else {
                _logger->logError("[DataHubImpl::update] Scope with id = %d not found\n", int(header.location.scopeId));
            }
        }
    }

    std::shared_ptr<Scope> DataHubImpl::getScope(ScopeId token) {
        auto index = _scopes.find(token);
        if (index != _scopes.end()) {
            return index->second;
        }
        
        return nullptr;
    }
    
    std::shared_ptr<Scope> DataHubImpl::getRootScope(const char *rootScopeName) {
        auto index = _roots.find(rootScopeName);
        if (index != _roots.end()) {
            return index->second;
        }
        
        return nullptr;
    }

    void testDataHub() {
        
    }
}
