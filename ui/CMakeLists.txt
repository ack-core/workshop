
get_filename_component(m_source_root "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

set(
	m_sources_list
	"${m_source_root}/stage.h"
	"${m_source_root}/stage.cpp"
)
source_group("" FILES ${m_sources_list})
add_library(ui ${m_sources_list})

set_property(TARGET ui PROPERTY FOLDER "engine")
