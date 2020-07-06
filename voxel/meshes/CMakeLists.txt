
function(meshes)
	get_filename_component(m_source_root "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

	set(
		m_sources_list
		"${m_source_root}/interfaces.h"
		"${m_source_root}/meshes.h"
		"${m_source_root}/meshes.cpp"
	)
	source_group("" FILES ${m_sources_list})
	add_library(meshes ${m_sources_list})
	
	set_property(TARGET meshes PROPERTY FOLDER "voxel")
endfunction()

meshes()
