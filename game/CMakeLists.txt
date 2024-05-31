
get_filename_component(m_source_root "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

set(
	m_sources_list
	"${m_source_root}/game.h"
	"${m_source_root}/state_manager.h"
	"${m_source_root}/state_manager.cpp"
	"${m_source_root}/contexts/context.h"
)
source_group("" FILES ${m_sources_list})
source_group("contexts" FILES "${m_source_root}/contexts/context.h")

if (APPTYPE STREQUAL "IS_CLIENT")
	set(
		m_contexts_list
		"${m_source_root}/contexts/debug_context.h"
		"${m_source_root}/contexts/debug_context.cpp"
	)
	source_group("contexts" FILES ${m_contexts_list})

elseif (APPTYPE STREQUAL "IS_PE")
	set(
		m_contexts_list
		"${m_source_root}/contexts/particle_editor_context.h"
		"${m_source_root}/contexts/particle_editor_context.cpp"
	)
	source_group("contexts" FILES ${m_contexts_list})
endif()

add_library(game ${m_sources_list} ${m_contexts_list})
set_property(TARGET game PROPERTY FOLDER "workshop")
