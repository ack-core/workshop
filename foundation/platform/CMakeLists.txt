
function(platform)
	get_filename_component(m_source_root "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

	set(
		m_sources_list
		"${m_source_root}/interfaces.h"
		"${m_source_root}/platform_windows.h"
		"${m_source_root}/platform_windows.cpp"
		"${m_source_root}/platform_ios.h"
		"${m_source_root}/platform_ios.cpp"
	)
	source_group("" FILES ${m_sources_list})
	add_library(platform ${m_sources_list})
	
	set_property(TARGET platform PROPERTY FOLDER "foundation")
endfunction()

platform()
