
get_filename_component(m_source_root "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

set(
	m_sources_list
	"${m_source_root}/mesh_factory.h"
	"${m_source_root}/mesh_factory.cpp"
)
source_group("" FILES ${m_sources_list})
add_library(voxel ${m_sources_list})

set_property(TARGET voxel PROPERTY FOLDER "engine")
