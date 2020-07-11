cmake_minimum_required(VERSION 3.10)

cmake_policy(SET CMP0091 NEW)

# platform-specific settings

if(${PLATFORM} STREQUAL "PLATFORM_WINDOWS")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /std:c++17")
elseif(${PLATFORM} STREQUAL "PLATFORM_IOS")
	set(MACOSX_BUNDLE_GUI_IDENTIFIER com.vox.workshop)
	set(MACOSX_BUNDLE_BUNDLE_VERSION "0.0.0.0")
	set(MACOSX_BUNDLE_SHORT_VERSION_STRING "0.0")
	set(CMAKE_OSX_DEPLOYMENT_TARGET "13.2")
	set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
	set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
	set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "workshop")
else()
	message(FATAL_ERROR "Unsupported platform")
endif()

add_definitions(-D${PLATFORM})

# common

set(CMAKE_SUPPRESS_REGENERATION true CACHE STRING "" FORCE)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

function(workshop)
	get_filename_component(m_source_root "${PROJECT_SOURCE_DIR}" ABSOLUTE)
	include_directories("${m_source_root}")

	add_subdirectory("${m_source_root}/thirdparty/upng")
	add_subdirectory("${m_source_root}/foundation")
	add_subdirectory("${m_source_root}/voxel")

	set(
		m_sources_list
		"${m_source_root}/main.cpp"
	)
	source_group("" FILES ${m_sources_list})
	add_executable(workshop ${m_sources_list})

	target_link_libraries(workshop upng)
	target_link_libraries(workshop platform)
	target_link_libraries(workshop rendering)
	target_link_libraries(workshop mesh_factory)
	target_link_libraries(workshop environment)
	
	set_property(TARGET workshop PROPERTY RUNTIME_OUTPUT_DIRECTORY_DEBUG "${m_source_root}/binary")
	set_property(TARGET workshop PROPERTY RUNTIME_OUTPUT_DIRECTORY_RELEASE "${m_source_root}/binary")
endfunction()

function(resources)
	get_filename_component(m_resources_root "${PROJECT_SOURCE_DIR}/resources" ABSOLUTE)
	get_filename_component(m_binary_root "${PROJECT_SOURCE_DIR}/binary" ABSOLUTE)

	file(COPY "${m_resources_root}/system" DESTINATION "${m_binary_root}")
endfunction()

# application

project(workshop CXX)

workshop()
resources()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT workshop)