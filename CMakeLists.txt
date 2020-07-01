cmake_minimum_required(VERSION 3.10)

# platform settings

if(${PLATFORM} STREQUAL "PLATFORM_WINDOWS")
	
elseif(${PLATFORM} STREQUAL "PLATFORM_IOS")
	set(MACOSX_BUNDLE_GUI_IDENTIFIER com.lesta.legends)
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

project(workshop CXX)

function(workshop)
	get_filename_component(m_source_root "${PROJECT_SOURCE_DIR}" ABSOLUTE)

	add_subdirectory("${m_source_root}/foundation/platform")
	add_subdirectory("${m_source_root}/foundation/rendering")

	set(
		m_sources_list
		"${m_source_root}/main.cpp"
	)
	source_group("" FILES ${m_sources_list})
	add_executable(workshop ${m_sources_list})
	
	target_include_directories(workshop PRIVATE "${m_source_root}/foundation")

	target_link_libraries(workshop platform)
	target_link_libraries(workshop rendering)
endfunction()

workshop()

