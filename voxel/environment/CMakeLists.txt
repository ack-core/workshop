
function(environment)
	get_filename_component(m_source_root "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

	set(
		m_sources_list
		"${m_source_root}/interfaces.h"
		"${m_source_root}/environment.h"
		"${m_source_root}/environment.cpp"
	)
	source_group("" FILES ${m_sources_list})
	add_library(environment ${m_sources_list})
	
	set_property(TARGET environment PROPERTY FOLDER "voxel")
endfunction()

environment()
