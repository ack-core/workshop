
function(voxel)
	get_filename_component(m_source_root "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

	set(
		m_sources_list
		"${m_source_root}/environment/interfaces.h"
		"${m_source_root}/environment/environment.h"
		"${m_source_root}/environment/environment.cpp"
	)
	source_group("" FILES ${m_sources_list})
	add_library(environment ${m_sources_list})

	set(
		m_sources_list
		"${m_source_root}/mesh_factory/interfaces.h"
		"${m_source_root}/mesh_factory/mesh_factory.h"
		"${m_source_root}/mesh_factory/mesh_factory.cpp"
	)
	source_group("" FILES ${m_sources_list})
	add_library(mesh_factory ${m_sources_list})
	
	set_property(TARGET mesh_factory PROPERTY FOLDER "voxel")
	set_property(TARGET environment PROPERTY FOLDER "voxel")
endfunction()

voxel()
