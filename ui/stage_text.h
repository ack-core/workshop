
#pragma once

namespace ui {
    class TextImpl : public ElementImpl, public StageInterface::Text {
    public:
        TextImpl(const StageFacility &facility, const std::shared_ptr<Element> &parent);
        ~TextImpl() override;
        
    public:
    
    
    private:
    
    };
}

