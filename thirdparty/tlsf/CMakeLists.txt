
get_filename_component(m_source_root "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

set(
	m_sources_list
	"${m_source_root}/tlsf.h"
	"${m_source_root}/tlsf.cpp"
)
source_group("" FILES ${m_sources_list})
add_library(tlsf ${m_sources_list})

set_property(TARGET tlsf PROPERTY FOLDER "thirdparty")
