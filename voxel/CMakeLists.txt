
get_filename_component(m_source_root "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

set(
	m_sources_list
	"${m_source_root}/scene.h"
	"${m_source_root}/scene.cpp"
	"${m_source_root}/simulation.h"
	"${m_source_root}/simulation.cpp"
	"${m_source_root}/yard.h"
	"${m_source_root}/yard.cpp"
	"${m_source_root}/yard_base.h"
	"${m_source_root}/yard_base.cpp"
	"${m_source_root}/yard_actor.h"
	"${m_source_root}/yard_actor.cpp"
	"${m_source_root}/yard_stead.h"
	"${m_source_root}/yard_stead.cpp"
	"${m_source_root}/yard_thing.h"
	"${m_source_root}/yard_thing.cpp"
)
source_group("" FILES ${m_sources_list})
add_library(voxel ${m_sources_list})

set_property(TARGET voxel PROPERTY FOLDER "engine")
