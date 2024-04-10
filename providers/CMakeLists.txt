
get_filename_component(m_source_root "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

set(
	m_sources_list
	"${m_source_root}/mesh_provider.h"
	"${m_source_root}/mesh_provider.cpp"
	"${m_source_root}/meshes_list.h"
	"${m_source_root}/ground_provider.h"
	"${m_source_root}/ground_provider.cpp"
	"${m_source_root}/grounds_list.h"
	"${m_source_root}/texture_provider.h"
	"${m_source_root}/texture_provider.cpp"
	"${m_source_root}/textures_list.h"
	#"${m_source_root}/fontatlas_provider.h"
	#"${m_source_root}/fontatlas_provider.cpp"
)
source_group("" FILES ${m_sources_list})
add_library(providers ${m_sources_list})

set_property(TARGET providers PROPERTY FOLDER "engine")
