add_library(AppCredentials STATIC
    AppStorage.h
    CredentialVault.cpp CredentialVault.h CredentialBackend.h
    macos/CredentialBackend.cpp
    windows/CredentialBackend.cpp
    linux/CredentialBackend.cpp
)
target_include_directories(AppCredentials PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(AppCredentials PUBLIC Qt6::Core PRIVATE Qt6::Concurrent)
target_compile_definitions(AppCredentials PUBLIC
    $<$<CONFIG:Release>:AFK_RELEASE_PROFILE>
    "$<$<CONFIG:Debug>:AFK_DEBUG_DATA_DIR=\"${CMAKE_BINARY_DIR}/debug-data\">"
)

if(APPLE)
    find_library(SECURITY_FRAMEWORK Security REQUIRED)
    target_link_libraries(AppCredentials PRIVATE "${SECURITY_FRAMEWORK}")
elseif(WIN32)
    target_link_libraries(AppCredentials PRIVATE Advapi32)
elseif(UNIX AND NOT ANDROID)
    find_package(Qt6 6.12 REQUIRED COMPONENTS DBus)
    target_link_libraries(AppCredentials PRIVATE Qt6::DBus)
else()
    message(FATAL_ERROR "Secure credential storage currently supports macOS, Windows and Linux desktops")
endif()

if(NOT UNIX OR APPLE OR ANDROID)
    set_source_files_properties(linux/CredentialBackend.cpp PROPERTIES SKIP_AUTOMOC ON)
endif()
