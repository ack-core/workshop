
get_filename_component(m_source_root "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

set(
	m_sources_list
	"${m_source_root}/runner.h"
	"${m_source_root}/runner.cpp"
	"${m_source_root}/math_tests.cpp"
	"${m_source_root}/util_tests.cpp"
	"${m_source_root}/shader_tests.cpp"
	"${m_source_root}/platform_tests.cpp"
	"${m_source_root}/rendering_tests.cpp"
	"${m_source_root}/resource_provider_tests.cpp"
	"${m_source_root}/fontatlas_provider_tests.cpp"
	"${m_source_root}/scene_tests.cpp"
	"${m_source_root}/raycast_tests.cpp"
	"${m_source_root}/simulation_tests.cpp"
	"${m_source_root}/emitter_baker_tests.cpp"
	"${m_source_root}/stage_tests.cpp"
)
source_group("" FILES ${m_sources_list})

add_library(testing ${m_sources_list})
set_property(TARGET testing PROPERTY FOLDER "workshop")
