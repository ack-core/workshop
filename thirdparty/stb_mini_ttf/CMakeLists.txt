
get_filename_component(m_source_root "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

set(
	m_sources_list
	"${m_source_root}/stb_mini_ttf.h"
	"${m_source_root}/stb_mini_ttf.cpp"
)
source_group("" FILES ${m_sources_list})
add_library(stb_mini_ttf ${m_sources_list})

set_property(TARGET stb_mini_ttf PROPERTY FOLDER "thirdparty")
