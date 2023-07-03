
get_filename_component(m_source_root "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

set(
	m_sources_list
	"${m_source_root}/stage_base.h"
	"${m_source_root}/stage_base.cpp"
	"${m_source_root}/stage.h"
	"${m_source_root}/stage.cpp"
	"${m_source_root}/stage_pivot.h"
	"${m_source_root}/stage_pivot.cpp"
	"${m_source_root}/stage_image.h"
	"${m_source_root}/stage_image.cpp"
	"${m_source_root}/stage_text.h"
	"${m_source_root}/stage_text.cpp"
)
source_group("" FILES ${m_sources_list})
add_library(ui ${m_sources_list})

set_property(TARGET ui PROPERTY FOLDER "engine")
