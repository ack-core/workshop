
get_filename_component(m_source_root "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

set(
	m_sources_list
	"${m_source_root}/game.h"
	"${m_source_root}/state_manager.h"
	"${m_source_root}/state_manager.cpp"
	"${m_source_root}/context.h"
)
source_group("" FILES ${m_sources_list})

file(GLOB_RECURSE m_experimental_contexts_list "${m_source_root}/experimental/*.h" "${m_source_root}/experimental/*.cpp")
source_group("experimental" FILES ${m_experimental_contexts_list})

file(GLOB_RECURSE m_editor_contexts_list "${m_source_root}/editor/*.h" "${m_source_root}/editor/*.cpp")
source_group("editor" FILES ${m_editor_contexts_list})

file(GLOB_RECURSE m_game_contexts_list "${m_source_root}/game/*.h" "${m_source_root}/game/*.cpp")
source_group("contexts" FILES ${m_game_contexts_list})

add_library(game ${m_sources_list} ${m_game_contexts_list} ${m_editor_contexts_list} ${m_experimental_contexts_list})
set_property(TARGET game PROPERTY FOLDER "workshop")
