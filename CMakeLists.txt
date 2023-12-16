cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0091 NEW)

# platform-specific settings

set(PLATFORM_POSTFIX "unknown")
if(${PLATFORM} STREQUAL "PLATFORM_WINDOWS")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /EHsc /std:c++17")
	set(PLATFORM_POSTFIX "win")
elseif(${PLATFORM} STREQUAL "PLATFORM_IOS")
	set(PLATFORM_POSTFIX "ios")
elseif(${PLATFORM} STREQUAL "PLATFORM_WASM")
	set(PLATFORM_POSTFIX "wasm")
else()
	message(FATAL_ERROR "Unsupported platform")
endif()

add_definitions(-D${PLATFORM})

# common

set(CMAKE_SUPPRESS_REGENERATION true CACHE STRING "" FORCE)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# application

project(workshop CXX)

get_filename_component(m_source_root "${PROJECT_SOURCE_DIR}" ABSOLUTE)
include_directories("${m_source_root}")

add_subdirectory("${m_source_root}/thirdparty/upng")
#add_subdirectory("${m_source_root}/thirdparty/expect")
add_subdirectory("${m_source_root}/thirdparty/stb_mini_ttf")

add_subdirectory("${m_source_root}/foundation")
#add_subdirectory("${m_source_root}/providers")
#add_subdirectory("${m_source_root}/voxel")
#add_subdirectory("${m_source_root}/ui")
#add_subdirectory("${m_source_root}/datahub")
#add_subdirectory("${m_source_root}/game")

set(
	m_sources_list
	"${m_source_root}/entry.cpp"
)
source_group("" FILES ${m_sources_list})
add_executable(workshop ${m_sources_list})

target_link_libraries(workshop PUBLIC upng)
#target_link_libraries(workshop PUBLIC expect)
target_link_libraries(workshop PUBLIC stb_mini_ttf)

target_link_libraries(workshop PUBLIC foundation)
#target_link_libraries(workshop PUBLIC providers)
#target_link_libraries(workshop PUBLIC voxel)
#target_link_libraries(workshop PUBLIC ui)
#target_link_libraries(workshop PUBLIC datahub)
#target_link_libraries(workshop PUBLIC game)

set_property(TARGET workshop PROPERTY RUNTIME_OUTPUT_DIRECTORY "${m_source_root}/binary")

# resources

get_filename_component(m_tools_root "${PROJECT_SOURCE_DIR}/tools" ABSOLUTE)
get_filename_component(m_resources_root "${PROJECT_SOURCE_DIR}/resources" ABSOLUTE)
get_filename_component(m_binary_root "${PROJECT_SOURCE_DIR}/binary" ABSOLUTE)
get_filename_component(m_package_root "${PROJECT_SOURCE_DIR}/package/${PLATFORM_POSTFIX}" ABSOLUTE)

#file(REMOVE_RECURSE "${m_binary_root}")
#file(MAKE_DIRECTORY "${m_binary_root}")

file(COPY "${m_resources_root}/" DESTINATION "${m_binary_root}/data" FILES_MATCHING PATTERN "*.ttf")
file(COPY "${m_resources_root}/" DESTINATION "${m_binary_root}/data" FILES_MATCHING PATTERN "*.png")
file(COPY "${m_resources_root}/" DESTINATION "${m_binary_root}/data" FILES_MATCHING PATTERN "*.yard")
file(COPY "${m_resources_root}/" DESTINATION "${m_binary_root}/data" FILES_MATCHING PATTERN "*.vox")
file(COPY "${m_resources_root}/" DESTINATION "${m_binary_root}/data" FILES_MATCHING PATTERN "*.txt")
file(COPY "${m_package_root}/" DESTINATION "${m_binary_root}" FILES_MATCHING PATTERN "*")

message(STATUS "Generating resource lists...")
execute_process(
	COMMAND python ${m_tools_root}/resource_list.py -r ${m_resources_root} -e .png -d ${m_binary_root}/data/textures.list
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

execute_process(
	COMMAND python ${m_tools_root}/resource_list.py -r ${m_resources_root} -e .vox -d ${m_binary_root}/data/meshes.list
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

# specific

if(${PLATFORM} STREQUAL "PLATFORM_WINDOWS")
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT workshop)
elseif(${PLATFORM} STREQUAL "PLATFORM_IOS")
	set_source_files_properties(${m_source_root}/entry.cpp PROPERTIES XCODE_EXPLICIT_FILE_TYPE sourcecode.cpp.objcpp)

	target_link_libraries( workshop PUBLIC "-framework Metal" )
	target_link_libraries( workshop PUBLIC "-framework MetalKit" )
	target_link_libraries( workshop PUBLIC "-framework UIKit" )

	file( GLOB_RECURSE m_ipa_file_list "${m_binary_root}/data/*" )

	source_group( TREE "${m_binary_root}/data" PREFIX "Resources" FILES ${m_ipa_file_list} )
	target_sources( workshop PUBLIC ${m_ipa_file_list} )

	foreach( m_ipa_file_path ${m_ipa_file_list} )
		file( RELATIVE_PATH m_ipa_file_rel_path "${m_binary_root}/data" ${m_ipa_file_path} )
		get_filename_component( m_ipa_rel_dir ${m_ipa_file_rel_path} DIRECTORY )
		set_property( SOURCE ${m_ipa_file_path} PROPERTY MACOSX_PACKAGE_LOCATION "${m_ipa_rel_dir}" )
		set_property( SOURCE ${m_ipa_file_path} PROPERTY XCODE_EXPLICIT_FILE_TYPE "Data" )
	endforeach()

	set_property(TARGET workshop PROPERTY XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.ack-games.workshop")
	set_property(TARGET workshop PROPERTY MACOSX_BUNDLE_INFO_PLIST "${m_binary_root}/info.plist")
elseif(${PLATFORM} STREQUAL "PLATFORM_WASM")
	target_link_options(workshop PUBLIC --export=initialize --export=deinitialize --export=malloc --export=free --wrap=malloc --wrap=free)
	target_link_options(workshop PUBLIC --export=updateFrame --export=fileLoaded --export=taskExecute --export=taskComplete)
	target_link_options(workshop PUBLIC --export=pointerEvent)
endif()




