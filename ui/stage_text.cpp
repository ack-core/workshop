
#include "stage.h"
#include "stage_base.h"
#include "stage_text.h"

namespace ui {
    TextImpl::TextImpl(const StageFacility &facility, const std::shared_ptr<Element> &parent) : ElementImpl(facility, parent) {
    
    }
    TextImpl::~TextImpl() {
    
    }
}
