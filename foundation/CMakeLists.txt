
get_filename_component(m_source_root "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

set(
	m_sources_platform_list
	"${m_source_root}/util.h"
	"${m_source_root}/math.h"
	"${m_source_root}/platform.h"
	"${m_source_root}/platform_windows.h"
	"${m_source_root}/platform_windows.cpp"
	"${m_source_root}/platform_ios.h"
	"${m_source_root}/platform_ios.cpp"
	"${m_source_root}/platform_wasm.h"
	"${m_source_root}/platform_wasm.cpp"
)
source_group("" FILES ${m_sources_platform_list})

set(
	m_sources_rendering_list
	"${m_source_root}/rendering.h"
	"${m_source_root}/rendering_dx11.h"
	"${m_source_root}/rendering_dx11.cpp"
	"${m_source_root}/rendering_metal.h"
	"${m_source_root}/rendering_metal.cpp"
)
source_group("" FILES ${m_sources_rendering_list})

set(
	m_sources_sound_list
	"${m_source_root}/sound.h"
)
source_group("" FILES ${m_sources_sound_list})

add_library(foundation
	${m_sources_platform_list}
	${m_sources_rendering_list}
	${m_sources_sound_list}
)	

set_property(TARGET foundation PROPERTY FOLDER "engine")
target_compile_options(foundation PUBLIC "-fobjc-arc")

if(${PLATFORM} STREQUAL "PLATFORM_IOS")
	set_source_files_properties(${m_source_root}/platform_ios.cpp PROPERTIES XCODE_EXPLICIT_FILE_TYPE sourcecode.cpp.objcpp)
	set_source_files_properties(${m_source_root}/rendering_metal.cpp PROPERTIES XCODE_EXPLICIT_FILE_TYPE sourcecode.cpp.objcpp)
endif()

