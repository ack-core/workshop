
get_filename_component(m_source_root "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

set(
	m_sources_platform_list
	"${m_source_root}/platform.h"
	"${m_source_root}/platform_windows.h"
	"${m_source_root}/platform_windows.cpp"
	"${m_source_root}/platform_ios.h"
	"${m_source_root}/platform_ios.cpp"
)
source_group("" FILES ${m_sources_platform_list})

set(
	m_sources_rendering_list
	"${m_source_root}/rendering.h"
	"${m_source_root}/rendering_dx11.h"
	"${m_source_root}/rendering_dx11.cpp"
)
source_group("" FILES ${m_sources_rendering_list})

set(
	m_sources_sound_list
	"${m_source_root}/sound.h"
)
source_group("" FILES ${m_sources_sound_list})

set(
	m_sources_gears_list
	"${m_source_root}/gears/math.h"
	"${m_source_root}/gears/camera.h"
	"${m_source_root}/gears/primitives.h"
	"${m_source_root}/gears/parsing.h"
	"${m_source_root}/gears/orbit_camera_controller.h"
)
source_group("gears" FILES ${m_sources_gears_list})

add_library(foundation
	${m_sources_platform_list}
	${m_sources_rendering_list}
	${m_sources_sound_list}
	${m_sources_gears_list}
)	

set_property(TARGET foundation PROPERTY FOLDER "engine")

