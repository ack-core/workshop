
function(game)
	get_filename_component(m_source_root "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

	set(
		m_sources_list
		"${m_source_root}/castle_controller/interfaces.h"
		"${m_source_root}/castle_controller/castle_controller.h"
		"${m_source_root}/castle_controller/castle_controller.cpp"
	)
	source_group("" FILES ${m_sources_list})
	add_library(castle_controller ${m_sources_list})

	set(
		m_sources_list
		"${m_source_root}/creature_controller/interfaces.h"
		"${m_source_root}/creature_controller/creature_controller.h"
		"${m_source_root}/creature_controller/creature_controller.cpp"
	)
	source_group("" FILES ${m_sources_list})
	add_library(creature_controller ${m_sources_list})

	set(
		m_sources_list
		"${m_source_root}/projectiles/interfaces.h"
		"${m_source_root}/projectiles/projectiles.h"
		"${m_source_root}/projectiles/projectiles.cpp"
	)
	source_group("" FILES ${m_sources_list})
	add_library(projectiles ${m_sources_list})
	
	set_property(TARGET castle_controller PROPERTY FOLDER "game")
	set_property(TARGET creature_controller PROPERTY FOLDER "game")
	set_property(TARGET projectiles PROPERTY FOLDER "game")
endfunction()

game()
