cmake_minimum_required(VERSION 4.3)

if(DEFINED BUILD_CONFIG AND NOT BUILD_CONFIG STREQUAL "Release")
    return()
endif()

if(NOT PLATFORM MATCHES "^(macos-arm64|windows-x64|linux-x64)$")
    message(FATAL_ERROR "Set PLATFORM to macos-arm64, windows-x64, or linux-x64.")
endif()
if(NOT DEFINED BUILD_DIR OR NOT EXISTS "${BUILD_DIR}/CMakeCache.txt")
    message(FATAL_ERROR "Set BUILD_DIR to a configured Release build directory.")
endif()
file(READ "${BUILD_DIR}/CMakeCache.txt" build_cache)
string(REGEX MATCH "(^|\n)CMAKE_CONFIGURATION_TYPES:STRING=([^\r\n]*)" multi_config "${build_cache}")
set(configurations "${CMAKE_MATCH_2}")
if(NOT build_cache MATCHES "(^|\n)CMAKE_BUILD_TYPE:STRING=Release([\r\n]|$)"
   AND NOT "Release" IN_LIST configurations)
    message(FATAL_ERROR "BUILD_DIR has no Release configuration: ${BUILD_DIR}")
endif()

file(REAL_PATH "${CMAKE_CURRENT_LIST_DIR}/.." project_dir)
set(out_dir "${project_dir}/out")
if(IS_SYMLINK "${out_dir}")
    message(FATAL_ERROR "Refusing to clean a symbolic link: ${out_dir}")
endif()
file(REAL_PATH "${BUILD_DIR}" build_dir)
cmake_path(IS_PREFIX out_dir "${build_dir}" NORMALIZE build_inside_out)
if(build_inside_out)
    message(FATAL_ERROR "BUILD_DIR must be outside the output directory: ${out_dir}")
endif()

file(REMOVE_RECURSE "${out_dir}")
# Finder can recreate .DS_Store immediately if the output folder is open.
file(GLOB leftovers "${out_dir}/*")
list(REMOVE_ITEM leftovers "${out_dir}/.DS_Store")
if(leftovers OR IS_SYMLINK "${out_dir}" OR (EXISTS "${out_dir}" AND NOT IS_DIRECTORY "${out_dir}"))
    message(FATAL_ERROR "Could not clear the output directory: ${out_dir}")
endif()

set(output_dir "${out_dir}/AFKDrount")
set(stage_dir "${output_dir}/${PLATFORM}")
set(archive "${output_dir}/AFKDrount-${PLATFORM}.tar.gz")

execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${build_dir}" --config Release --prefix "${stage_dir}"
    COMMAND_ERROR_IS_FATAL ANY
)
file(GLOB payload RELATIVE "${stage_dir}" "${stage_dir}/*")
list(FILTER payload EXCLUDE REGEX "^[.]")
if(NOT payload)
    message(FATAL_ERROR "The staging directory contains no release files: ${stage_dir}")
endif()
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar czf "${archive}.tmp" --format=gnutar ${payload}
    WORKING_DIRECTORY "${stage_dir}"
    COMMAND_ERROR_IS_FATAL ANY
)
file(RENAME "${archive}.tmp" "${archive}")
message(STATUS "Release archive: ${archive}")
