
get_filename_component(m_source_root "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

set(
	m_sources_list
	"${m_source_root}/palette.h"
	"${m_source_root}/scene.h"
	"${m_source_root}/scene.cpp"
	"${m_source_root}/world.h"
	"${m_source_root}/world.cpp"
	"${m_source_root}/raycast.h"
	"${m_source_root}/raycast.cpp"
	"${m_source_root}/simulation.h"
	"${m_source_root}/simulation.cpp"
)
source_group("" FILES ${m_sources_list})
add_library(voxel ${m_sources_list})

set_property(TARGET voxel PROPERTY FOLDER "engine")
