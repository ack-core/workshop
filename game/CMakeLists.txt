
get_filename_component(m_source_root "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

set(
	m_sources_list
	"${m_source_root}/game.h"
	"${m_source_root}/state_manager.h"
	"${m_source_root}/state_manager.cpp"
	"${m_source_root}/context.h"
)
source_group("" FILES ${m_sources_list})

if (APPTYPE STREQUAL "IS_CLIENT")
	set(
		m_contexts_list
		"${m_source_root}/contexts/debug_context.h"
		"${m_source_root}/contexts/debug_context.cpp"
	)
	source_group("contexts" FILES ${m_contexts_list})
endif()

if (APPTYPE STREQUAL "IS_CLIENT" OR APPTYPE STREQUAL "IS_EDITOR")
	set(
		m_editor_contexts_list
		"${m_source_root}/contexts/editor/node_access_interface.h"
		"${m_source_root}/contexts/editor/editor_main_context.h"
		"${m_source_root}/contexts/editor/editor_main_context.cpp"
		"${m_source_root}/contexts/editor/editor_camera_context.h"
		"${m_source_root}/contexts/editor/editor_camera_context.cpp"
		"${m_source_root}/contexts/editor/editor_static_mesh_context.h"
		"${m_source_root}/contexts/editor/editor_static_mesh_context.cpp"
		"${m_source_root}/contexts/editor/editor_particles_context.h"
		"${m_source_root}/contexts/editor/editor_particles_context.cpp"
	)
	source_group("contexts/editor" FILES ${m_editor_contexts_list})

endif()

add_library(game ${m_sources_list} ${m_contexts_list} ${m_editor_contexts_list})
set_property(TARGET game PROPERTY FOLDER "workshop")
