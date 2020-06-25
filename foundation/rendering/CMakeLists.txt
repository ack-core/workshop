
function(rendering)
	get_filename_component(m_source_root "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

	set(
		m_sources_list
		"${m_source_root}/interfaces.h"
		"${m_source_root}/rendering_dx11.h"
		"${m_source_root}/rendering_dx11.cpp"
	)
	source_group("" FILES ${m_sources_list})
	add_library(rendering ${m_sources_list})
	
	set_property(TARGET rendering PROPERTY FOLDER "foundation")
endfunction()

rendering()
