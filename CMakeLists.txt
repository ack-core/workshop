cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0091 NEW)

message(STATUS "toolchain: ${CMAKE_TOOLCHAIN_FILE}")

project(workshop CXX)

# platform-specific settings

if (${PLATFORM} STREQUAL "PLATFORM_WINDOWS")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /EHsc /std:c++20 /wd4250 /wd4305")
	set(PLATFORM_POSTFIX "windows")
elseif (${PLATFORM} STREQUAL "PLATFORM_IOS")
	set(PLATFORM_POSTFIX "ios")
elseif (${PLATFORM} STREQUAL "PLATFORM_WASM")
	set(PLATFORM_POSTFIX "wasm")
	set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -O0")
else()
	message(FATAL_ERROR "Unsupported platform")
endif()

add_definitions(-D${PLATFORM})
add_definitions(-D${APPTYPE})

# common

set(CMAKE_SUPPRESS_REGENERATION true CACHE STRING "" FORCE)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# application

get_filename_component(m_source_root "${PROJECT_SOURCE_DIR}" ABSOLUTE)
include_directories("${m_source_root}")

add_subdirectory("${m_source_root}/thirdparty/tlsf")
add_subdirectory("${m_source_root}/thirdparty/upng")
add_subdirectory("${m_source_root}/thirdparty/stb_mini_ttf")

add_subdirectory("${m_source_root}/foundation")
add_subdirectory("${m_source_root}/providers")
add_subdirectory("${m_source_root}/core")
add_subdirectory("${m_source_root}/ui")

if (APPTYPE STREQUAL "IS_GAME" OR APPTYPE STREQUAL "IS_EDITOR")
	add_subdirectory("${m_source_root}/game")
elseif (APPTYPE STREQUAL "IS_TESTS")
	add_subdirectory("${m_source_root}/testing")
endif()

set(m_sources_list "${m_source_root}/entry.cpp")

source_group("" FILES ${m_sources_list})
add_executable(app ${m_sources_list})

target_link_libraries(app PUBLIC tlsf)
target_link_libraries(app PUBLIC upng)
target_link_libraries(app PUBLIC stb_mini_ttf)

target_link_libraries(app PUBLIC foundation)
target_link_libraries(app PUBLIC providers)
target_link_libraries(app PUBLIC core)
target_link_libraries(app PUBLIC ui)

if (APPTYPE STREQUAL "IS_GAME" OR APPTYPE STREQUAL "IS_EDITOR")
	target_link_libraries(app PUBLIC game)
elseif (APPTYPE STREQUAL "IS_TESTS")
	target_link_libraries(app PUBLIC testing)
endif()

if (APPTYPE STREQUAL "IS_GAME")
	set_property(TARGET app PROPERTY RUNTIME_OUTPUT_DIRECTORY "${m_source_root}/binary_${PLATFORM_POSTFIX}_game")
	get_filename_component(m_binary_root "${PROJECT_SOURCE_DIR}/binary_${PLATFORM_POSTFIX}_game" ABSOLUTE)
	set(APPTYPE_POSTFIX "game")
elseif (APPTYPE STREQUAL "IS_TESTS")
	set_property(TARGET app PROPERTY RUNTIME_OUTPUT_DIRECTORY "${m_source_root}/binary_${PLATFORM_POSTFIX}_tests")
	get_filename_component(m_binary_root "${PROJECT_SOURCE_DIR}/binary_${PLATFORM_POSTFIX}_tests" ABSOLUTE)
	set(APPTYPE_POSTFIX "tests")
elseif (APPTYPE STREQUAL "IS_EDITOR")
	set_property(TARGET app PROPERTY RUNTIME_OUTPUT_DIRECTORY "${m_source_root}/binary_editor")
	get_filename_component(m_binary_root "${PROJECT_SOURCE_DIR}/binary_editor" ABSOLUTE)
	set(APPTYPE_POSTFIX "editor")
else()
	message(FATAL_ERROR "Unknown app type")
endif()

# resources

get_filename_component(m_tools_root "${PROJECT_SOURCE_DIR}/tools" ABSOLUTE)
get_filename_component(m_resources_root "${PROJECT_SOURCE_DIR}/resources" ABSOLUTE)
get_filename_component(m_game_resources_root "${PROJECT_SOURCE_DIR}/game/resources" ABSOLUTE)
get_filename_component(m_tests_resources_root "${PROJECT_SOURCE_DIR}/testing/resources" ABSOLUTE)
get_filename_component(m_package_root "${PROJECT_SOURCE_DIR}/package/${PLATFORM_POSTFIX}" ABSOLUTE)
get_filename_component(m_package_app_root "${PROJECT_SOURCE_DIR}/package/${PLATFORM_POSTFIX}_${APPTYPE_POSTFIX}" ABSOLUTE)

#file(REMOVE_RECURSE "${m_binary_root}")
#file(MAKE_DIRECTORY "${m_binary_root}")

file(COPY "${m_package_root}/" DESTINATION "${m_binary_root}" FILES_MATCHING PATTERN "*")
if (EXISTS ${m_package_app_root})
	file(COPY "${m_package_app_root}/" DESTINATION "${m_binary_root}" FILES_MATCHING PATTERN "*")
endif()

message(STATUS "Copying files from resources...")

if (APPTYPE STREQUAL "IS_GAME" OR APPTYPE STREQUAL "IS_EDITOR")
	file(COPY "${m_resources_root}/meshes" DESTINATION "${m_binary_root}/data" FILES_MATCHING PATTERN "*.copy_only_dir_structure")
	file(COPY "${m_resources_root}/emitters" DESTINATION "${m_binary_root}/data" FILES_MATCHING PATTERN "*.copy_only_dir_structure")
	file(COPY "${m_resources_root}/grounds" DESTINATION "${m_binary_root}/data" FILES_MATCHING PATTERN "*.copy_only_dir_structure")
	file(COPY "${m_resources_root}/raycast" DESTINATION "${m_binary_root}/data" FILES_MATCHING PATTERN "*.copy_only_dir_structure")
	file(COPY "${m_resources_root}/collision" DESTINATION "${m_binary_root}/data" FILES_MATCHING PATTERN "*.copy_only_dir_structure")

	file(COPY "${m_resources_root}/raycast" DESTINATION "${m_binary_root}/data" FILES_MATCHING PATTERN "*.txt" PATTERN "*")
	file(COPY "${m_resources_root}/collision" DESTINATION "${m_binary_root}/data" FILES_MATCHING PATTERN "*.txt" PATTERN "*")
	file(COPY "${m_resources_root}/textures" DESTINATION "${m_binary_root}/data" FILES_MATCHING PATTERN "*.png" PATTERN "*")
	file(COPY "${m_resources_root}/primary.ttf" DESTINATION "${m_binary_root}/data")
	
	if (EXISTS ${m_game_resources_root}/base)
		file(COPY "${m_game_resources_root}/base" DESTINATION "${m_binary_root}/data" FILES_MATCHING PATTERN "*.*" PATTERN "*")
	endif()

	message(STATUS "Generating palette...")
	execute_process(
		COMMAND python ${m_tools_root}/palette2code.py -s ${m_resources_root}/palette.png -d ${PROJECT_SOURCE_DIR}/core/palette.h
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	)
	message(STATUS "Generating grounds...")
	execute_process(
		COMMAND python ${m_tools_root}/gen_grounds.py -s ${m_resources_root}/grounds -d ${m_binary_root}/data/grounds -p ${m_resources_root}/palette.png
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	)
	message(STATUS "Generating optimized meshes...")
	execute_process(
		COMMAND python ${m_tools_root}/gen_meshes.py -s ${m_resources_root}/meshes -d ${m_binary_root}/data/meshes -o 1 WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	)
	message(STATUS "Generating emitters...")
	execute_process(
		COMMAND python ${m_tools_root}/gen_emitters.py -s ${m_resources_root}/emitters -d ${m_binary_root}/data/emitters WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	)
endif()
if (APPTYPE STREQUAL "IS_GAME")
	if (EXISTS ${m_game_resources_root})
		file(COPY "${m_game_resources_root}/raycast" DESTINATION "${m_binary_root}/data" FILES_MATCHING PATTERN "*.txt" PATTERN "*")
		file(COPY "${m_game_resources_root}/collision" DESTINATION "${m_binary_root}/data" FILES_MATCHING PATTERN "*.txt" PATTERN "*")
		file(COPY "${m_game_resources_root}/textures" DESTINATION "${m_binary_root}/data" FILES_MATCHING PATTERN "*.png" PATTERN "*")
		file(COPY "${m_game_resources_root}/primary.ttf" DESTINATION "${m_binary_root}/data")
	endif()
	message(STATUS "Generating game's grounds...")
	if (EXISTS ${m_game_resources_root}/grounds)
		execute_process(
			COMMAND python ${m_tools_root}/gen_grounds.py -s ${m_game_resources_root}/grounds -d ${m_binary_root}/data/grounds -p ${m_resources_root}/palette.png
			WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		)
	endif()
	message(STATUS "Generating game's optimized meshes...")
	if (EXISTS ${m_game_resources_root}/meshes)
		execute_process(
			COMMAND python ${m_tools_root}/gen_meshes.py -s ${m_game_resources_root}/meshes -d ${m_binary_root}/data/meshes -o 1 WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		)
	endif()
	message(STATUS "Generating game's emitters...")
	if (EXISTS ${m_game_resources_root}/emitters)
		execute_process(
			COMMAND python ${m_tools_root}/gen_emitters.py -s ${m_game_resources_root}/emitters -d ${m_binary_root}/data/emitters WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		)
	endif()
	message(STATUS "Generating game's prefabs...")
	execute_process(
		COMMAND python ${m_tools_root}/prefabs.py -s ${m_resources_root}/prefabs -g ${m_game_resources_root}/prefabs -d ${m_binary_root}/data/prefabs.bin 
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	)
endif()
if (APPTYPE STREQUAL "IS_TESTS")
	if (EXISTS ${m_tests_resources_root})
		file(COPY "${m_tests_resources_root}/" DESTINATION "${m_binary_root}/data" FILES_MATCHING PATTERN "*" PATTERN "*")
	endif()
endif()
if (APPTYPE STREQUAL "IS_EDITOR")
	message(STATUS "Generating prefabs...")
	execute_process(
		COMMAND python ${m_tools_root}/prefabs.py -s ${m_resources_root}/prefabs -d ${m_binary_root}/data/prefabs.bin 
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	)
endif()

message(STATUS "Generating resource lists...")
execute_process(
	COMMAND python ${m_tools_root}/resource_list.py -r ${m_binary_root}/data -d ${PROJECT_SOURCE_DIR}/providers
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

# specific

if (PLATFORM STREQUAL "PLATFORM_WINDOWS")
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT app)
	if (APPTYPE STREQUAL "IS_GAME")
		set_target_properties(app PROPERTIES OUTPUT_NAME "workshop")
		set_target_properties(app PROPERTIES WIN32_EXECUTABLE TRUE)
	elseif (APPTYPE STREQUAL "IS_TESTS")
		set_target_properties(app PROPERTIES OUTPUT_NAME "tests")
		set_target_properties(app PROPERTIES WIN32_EXECUTABLE FALSE)
		target_link_options(app PRIVATE "/ENTRY:WinMainCRTStartup")
	endif()

	#target_link_options(app PRIVATE "/ENTRY:mainCRTStartup")
	set_target_properties(app PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${m_binary_root}")
	set_target_properties(app PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${m_binary_root}")

elseif (PLATFORM STREQUAL "PLATFORM_IOS")
	set_source_files_properties(${m_source_root}/entry.cpp PROPERTIES XCODE_EXPLICIT_FILE_TYPE sourcecode.cpp.objcpp)
	set_source_files_properties(${m_source_root}/tests.cpp PROPERTIES XCODE_EXPLICIT_FILE_TYPE sourcecode.cpp.objcpp)

	target_link_libraries(app PUBLIC "-framework Metal")
	target_link_libraries(app PUBLIC "-framework MetalKit")
	target_link_libraries(app PUBLIC "-framework UIKit")

	file(GLOB_RECURSE m_ipa_file_list "${m_binary_root}/data/*")

	source_group(TREE "${m_binary_root}/data" PREFIX "Resources" FILES ${m_ipa_file_list})
	target_sources(app PUBLIC ${m_ipa_file_list})

	foreach (m_ipa_file_path ${m_ipa_file_list})
		file(RELATIVE_PATH m_ipa_file_rel_path "${m_binary_root}/data" ${m_ipa_file_path})
		get_filename_component( m_ipa_rel_dir ${m_ipa_file_rel_path} DIRECTORY)
		set_property(SOURCE ${m_ipa_file_path} PROPERTY MACOSX_PACKAGE_LOCATION "${m_ipa_rel_dir}")
		set_property(SOURCE ${m_ipa_file_path} PROPERTY XCODE_EXPLICIT_FILE_TYPE "Data")
	endforeach()

	set_property(TARGET app PROPERTY XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.ack-games.workshop")
	set_property(TARGET app PROPERTY MACOSX_BUNDLE_INFO_PLIST "${m_binary_root}/info.plist")
	set_target_properties(app PROPERTIES OUTPUT_NAME "workshop")

elseif (PLATFORM STREQUAL "PLATFORM_WASM")
	if (APPTYPE STREQUAL "IS_GAME")
		set_target_properties(app PROPERTIES OUTPUT_NAME "index")
	elseif (APPTYPE STREQUAL "IS_TESTS")
		set_target_properties(app PROPERTIES OUTPUT_NAME "tests")
	elseif (APPTYPE STREQUAL "IS_EDITOR")
		set_target_properties(app PROPERTIES OUTPUT_NAME "editor")
	endif()
	target_link_options(app PUBLIC --export=initialize --export=deinitialize --export=malloc --export=free)
	target_link_options(app PUBLIC --export=updateFrame --export=fileLoaded --export=fileSaved --export=taskExecute --export=taskComplete)
	target_link_options(app PUBLIC --export=pointerEvent --export=editorEvent --export=resized --export=keyboardEvent --export=wheelEvent)
endif()




