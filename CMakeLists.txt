cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0091 NEW)

message(STATUS "toolchain: ${CMAKE_TOOLCHAIN_FILE}")

# platform-specific settings

set(PLATFORM_POSTFIX "unknown")
if (${PLATFORM} STREQUAL "PLATFORM_WINDOWS")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /EHsc /std:c++17")
	set(PLATFORM_POSTFIX "win")

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

project(workshop CXX)

get_filename_component(m_source_root "${PROJECT_SOURCE_DIR}" ABSOLUTE)
include_directories("${m_source_root}")

add_subdirectory("${m_source_root}/thirdparty/tlsf")
add_subdirectory("${m_source_root}/thirdparty/upng")
add_subdirectory("${m_source_root}/thirdparty/stb_mini_ttf")

add_subdirectory("${m_source_root}/foundation")

if (NOT APPTYPE STREQUAL "IS_TESTS")
	add_subdirectory("${m_source_root}/providers")
	add_subdirectory("${m_source_root}/voxel")
	add_subdirectory("${m_source_root}/ui")
	add_subdirectory("${m_source_root}/datahub")
	add_subdirectory("${m_source_root}/game")
	set(m_sources_list "${m_source_root}/entry.cpp")
else()
	set(m_sources_list "${m_source_root}/tests.cpp")
endif()

source_group("" FILES ${m_sources_list})
add_executable(workshop ${m_sources_list})

target_link_libraries(workshop PUBLIC tlsf)
target_link_libraries(workshop PUBLIC upng)
target_link_libraries(workshop PUBLIC stb_mini_ttf)

target_link_libraries(workshop PUBLIC foundation)

if (NOT APPTYPE STREQUAL "IS_TESTS")
	target_link_libraries(workshop PUBLIC providers)
	target_link_libraries(workshop PUBLIC voxel)
	target_link_libraries(workshop PUBLIC ui)
	target_link_libraries(workshop PUBLIC datahub)
	target_link_libraries(workshop PUBLIC game)
endif()

if (APPTYPE STREQUAL "IS_GAME")
	set_property(TARGET workshop PROPERTY RUNTIME_OUTPUT_DIRECTORY "${m_source_root}/binary_${PLATFORM_POSTFIX}")
	get_filename_component(m_binary_root "${PROJECT_SOURCE_DIR}/binary_${PLATFORM_POSTFIX}" ABSOLUTE)
elseif (APPTYPE STREQUAL "IS_TESTS")
	set_property(TARGET workshop PROPERTY RUNTIME_OUTPUT_DIRECTORY "${m_source_root}/binary_${PLATFORM_POSTFIX}_tests")
	get_filename_component(m_binary_root "${PROJECT_SOURCE_DIR}/binary_${PLATFORM_POSTFIX}_tests" ABSOLUTE)
elseif (APPTYPE STREQUAL "IS_EDITOR")
	set_property(TARGET workshop PROPERTY RUNTIME_OUTPUT_DIRECTORY "${m_source_root}/binary_editor")
	get_filename_component(m_binary_root "${PROJECT_SOURCE_DIR}/binary_editor" ABSOLUTE)
endif()

# resources

get_filename_component(m_tools_root "${PROJECT_SOURCE_DIR}/tools" ABSOLUTE)
get_filename_component(m_resources_root "${PROJECT_SOURCE_DIR}/resources" ABSOLUTE)
get_filename_component(m_package_root "${PROJECT_SOURCE_DIR}/package/${PLATFORM_POSTFIX}" ABSOLUTE)

#file(REMOVE_RECURSE "${m_binary_root}")
#file(MAKE_DIRECTORY "${m_binary_root}")

file(COPY "${m_package_root}/" DESTINATION "${m_binary_root}" FILES_MATCHING PATTERN "*")

if (NOT APPTYPE STREQUAL "IS_TESTS")
	message(STATUS "Copying files from resources...")
	file(COPY "${m_resources_root}/meshes" DESTINATION "${m_binary_root}/data" FILES_MATCHING PATTERN "*.copy_only_dir_structure")
	file(COPY "${m_resources_root}/emitters" DESTINATION "${m_binary_root}/data" FILES_MATCHING PATTERN "*.copy_only_dir_structure")
	file(COPY "${m_resources_root}/grounds" DESTINATION "${m_binary_root}/data" FILES_MATCHING PATTERN "*.copy_only_dir_structure")
	file(COPY "${m_resources_root}/raycast" DESTINATION "${m_binary_root}/data" FILES_MATCHING PATTERN "*.txt" PATTERN "*")
	file(COPY "${m_resources_root}/collision" DESTINATION "${m_binary_root}/data" FILES_MATCHING PATTERN "*.txt" PATTERN "*")
	file(COPY "${m_resources_root}/textures" DESTINATION "${m_binary_root}/data" FILES_MATCHING PATTERN "*.png" PATTERN "*")

	message(STATUS "Generating palette...")
	execute_process(
		COMMAND python ${m_tools_root}/palette.py -s ${m_resources_root}/palette.png -d ${PROJECT_SOURCE_DIR}/voxel/palette.h
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	)
	message(STATUS "Generating prefabs...")
	execute_process(
		COMMAND python ${m_tools_root}/prefabs.py -s ${m_resources_root}/prefabs -d ${m_binary_root}/data/prefabs.bin
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	)
	message(STATUS "Generating grounds...")
	execute_process(
		COMMAND python ${m_tools_root}/gen_grounds.py -s ${m_resources_root}/grounds -d ${m_binary_root}/data/grounds -p ${m_resources_root}/palette.png
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	)
	message(STATUS "Generating optimized meshes...")
	execute_process(
		COMMAND python ${m_tools_root}/gen_meshes.py -s ${m_resources_root}/meshes -d ${m_binary_root}/data/meshes -o 1
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	)
	message(STATUS "Generating emitters...")
	execute_process(
		COMMAND python ${m_tools_root}/gen_emitters.py -s ${m_resources_root}/emitters -d ${m_binary_root}/data/emitters
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	)
	message(STATUS "Generating resource lists...")
	execute_process(
		COMMAND python ${m_tools_root}/resource_list.py -r ${m_binary_root}/data -d ${PROJECT_SOURCE_DIR}/providers
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
endif()

# specific

if (PLATFORM STREQUAL "PLATFORM_WINDOWS")
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT workshop)

elseif (PLATFORM STREQUAL "PLATFORM_IOS")
	set_source_files_properties(${m_source_root}/entry.cpp PROPERTIES XCODE_EXPLICIT_FILE_TYPE sourcecode.cpp.objcpp)
	set_source_files_properties(${m_source_root}/tests.cpp PROPERTIES XCODE_EXPLICIT_FILE_TYPE sourcecode.cpp.objcpp)

	target_link_libraries(workshop PUBLIC "-framework Metal")
	target_link_libraries(workshop PUBLIC "-framework MetalKit")
	target_link_libraries(workshop PUBLIC "-framework UIKit")

	file(GLOB_RECURSE m_ipa_file_list "${m_binary_root}/data/*")

	source_group(TREE "${m_binary_root}/data" PREFIX "Resources" FILES ${m_ipa_file_list})
	target_sources(workshop PUBLIC ${m_ipa_file_list})

	foreach (m_ipa_file_path ${m_ipa_file_list})
		file(RELATIVE_PATH m_ipa_file_rel_path "${m_binary_root}/data" ${m_ipa_file_path})
		get_filename_component( m_ipa_rel_dir ${m_ipa_file_rel_path} DIRECTORY)
		set_property(SOURCE ${m_ipa_file_path} PROPERTY MACOSX_PACKAGE_LOCATION "${m_ipa_rel_dir}")
		set_property(SOURCE ${m_ipa_file_path} PROPERTY XCODE_EXPLICIT_FILE_TYPE "Data")
	endforeach()

	set_property(TARGET workshop PROPERTY XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.ack-games.workshop")
	set_property(TARGET workshop PROPERTY MACOSX_BUNDLE_INFO_PLIST "${m_binary_root}/info.plist")

elseif (PLATFORM STREQUAL "PLATFORM_WASM")
	if (APPTYPE STREQUAL "IS_GAME")
		set_target_properties(workshop PROPERTIES OUTPUT_NAME "index")
	elseif (APPTYPE STREQUAL "IS_TESTS")
		set_target_properties(workshop PROPERTIES OUTPUT_NAME "tests")
	elseif (APPTYPE STREQUAL "IS_EDITOR")
		set_target_properties(workshop PROPERTIES OUTPUT_NAME "editor")
	endif()
	target_link_options(workshop PUBLIC --export=initialize --export=deinitialize --export=malloc --export=free)
	target_link_options(workshop PUBLIC --export=updateFrame --export=fileLoaded --export=fileSaved --export=taskExecute --export=taskComplete)
	target_link_options(workshop PUBLIC --export=pointerEvent --export=editorEvent --export=resized --export=keyboardEvent --export=wheelEvent)
endif()




