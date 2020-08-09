
function(foundation)
	get_filename_component(m_source_root "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

	set(
		m_sources_list
		"${m_source_root}/platform/interfaces.h"
		"${m_source_root}/platform/platform_windows.h"
		"${m_source_root}/platform/platform_windows.cpp"
		"${m_source_root}/platform/platform_ios.h"
		"${m_source_root}/platform/platform_ios.cpp"
	)
	source_group("" FILES ${m_sources_list})
	add_library(platform ${m_sources_list})
	
	set(
		m_sources_list
		"${m_source_root}/rendering/interfaces.h"
		"${m_source_root}/rendering/rendering_dx11.h"
		"${m_source_root}/rendering/rendering_dx11.cpp"
	)
	source_group("" FILES ${m_sources_list})
	add_library(rendering ${m_sources_list})	

	set(
		m_sources_list
		"${m_source_root}/math.h"
		"${m_source_root}/camera.h"
		"${m_source_root}/primitives.h"
		"${m_source_root}/parsing.h"
		"${m_source_root}/orbit_camera_controller.h"
	)
	source_group("" FILES ${m_sources_list})
	add_custom_target(gears SOURCES ${m_sources_list})
	
	set_property(TARGET platform PROPERTY FOLDER "foundation")
	set_property(TARGET rendering PROPERTY FOLDER "foundation")
	set_property(TARGET gears PROPERTY FOLDER "foundation")

endfunction()

foundation()
