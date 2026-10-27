
get_filename_component(m_source_root "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

set(
	m_sources_list
	"${m_source_root}/runner.h"
	"${m_source_root}/runner.cpp"
	"${m_source_root}/math_tests.h"
	"${m_source_root}/math_tests.cpp"
	"${m_source_root}/util_tests.h"
	"${m_source_root}/util_tests.cpp"
	"${m_source_root}/shader_tests.h"
	"${m_source_root}/shader_tests.cpp"
	"${m_source_root}/platform_tests.h"
	"${m_source_root}/platform_tests.cpp"
	"${m_source_root}/rendering_tests.h"
	"${m_source_root}/rendering_tests.cpp"
)
source_group("" FILES ${m_sources_list})

add_library(testing ${m_sources_list})
set_property(TARGET testing PROPERTY FOLDER "workshop")
