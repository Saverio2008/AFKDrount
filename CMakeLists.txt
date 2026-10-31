cmake_minimum_required(VERSION 4.3)
project(AFKDrount VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.12 REQUIRED COMPONENTS Quick QuickControls2 QuickTemplates2 Concurrent Widgets Network)
get_target_property(qt_uses_frameworks Qt6::Core FRAMEWORK)
if(APPLE AND qt_uses_frameworks)
    foreach(qt_module IN ITEMS Core Concurrent Gui Widgets Network OpenGL Qml QmlMeta QmlModels QmlWorkerScript Quick QuickControls2 QuickTemplates2)
        get_target_property(qt_include_dirs Qt6::${qt_module} INTERFACE_INCLUDE_DIRECTORIES)
        list(TRANSFORM qt_include_dirs REPLACE
            "/include/Qt${qt_module}$" "/lib/Qt${qt_module}.framework/Headers")
        set_target_properties(Qt6::${qt_module} PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${qt_include_dirs}")
    endforeach()
endif()
qt_standard_project_setup(REQUIRES 6.12)

add_subdirectory(src/platform)
add_subdirectory(src/qml/appearance)
add_subdirectory(src/qml/styles/Rounded)

qt_add_executable(AFKDrount src/main.cpp)
target_include_directories(AFKDrount PRIVATE src/core src/platform)

set(app_qml_files
    src/qml/Main.qml
    src/qml/pages/MainPage.qml
    src/qml/pages/MonitorPage.qml
    src/qml/components/AppearanceSelector.qml
    src/qml/pages/SettingsPage.qml
    src/qml/components/SelectionList.qml
    src/qml/components/AccountContextMenu.qml
    src/qml/components/InstanceContextMenu.qml
    src/qml/components/DeleteConfirmationDialog.qml
    src/qml/components/SessionList.qml
    src/qml/components/SessionLogView.qml
    src/qml/components/SessionStatistics.qml
    src/qml/components/SessionTextView.qml
    src/qml/components/ApplySessionConfigurationDialog.qml
    src/qml/components/CloseSessionsDialog.qml
    src/qml/components/EmptyState.qml
    src/qml/components/AccountPanel.qml
    src/qml/components/AccountDialog.qml
    src/qml/components/InstancePanel.qml
    src/qml/components/InstanceEditor.qml
    src/qml/components/CompactTextField.qml
    src/qml/components/DisclosureRow.qml
    src/qml/components/AdvancedInstanceSettings.qml
    src/qml/components/TasksEditor.qml
    src/qml/components/TaskDialog.qml
    src/qml/components/TaskStepEditor.qml
    src/qml/components/FailurePolicyRow.qml
    src/qml/components/CountingRulesEditor.qml
    src/qml/components/CountingRuleDialog.qml
    src/qml/components/VersionSelector.qml
    src/qml/components/ItemPicker.qml
    src/qml/components/CustomItemEditor.qml
    src/qml/components/RegionEditor.qml
    src/qml/components/RunPanel.qml
    src/qml/components/CreateInstanceDialog.qml
    src/qml/components/UnsavedChangesDialog.qml
)
foreach(qml_file IN LISTS app_qml_files)
    get_filename_component(qml_filename "${qml_file}" NAME)
    set_source_files_properties("${qml_file}" PROPERTIES QT_RESOURCE_ALIAS "${qml_filename}")
endforeach()
qt_add_qml_module(AFKDrount
    URI AFKDrount
    IMPORTS AppAppearance
    SOURCES src/core/InstanceStore.h src/core/InstanceStore.cpp
        src/core/GameCatalog.h src/core/GameCatalog.cpp
        src/core/MinecraftImages.h src/core/MinecraftImages.cpp
        src/core/OfficialLanguageCache.h src/core/OfficialLanguageCache.cpp
        src/core/InstanceConfiguration.h src/core/InstanceConfiguration.cpp
        src/core/CountingWorker.h src/core/CountingWorker.cpp
        src/core/BotSession.h src/core/BotSession.cpp
        src/core/SessionStore.h src/core/SessionStore.cpp
        src/platform/DesktopLifecycle.h src/platform/DesktopLifecycle.cpp
        src/core/LanguageManager.h src/core/LanguageManager.cpp
        src/core/AccountStore.h src/core/AccountStore.cpp src/core/Clipboard.h
    QML_FILES ${app_qml_files}
)
target_link_libraries(AFKDrount PRIVATE
    Qt6::Quick Qt6::QuickControls2 Qt6::Widgets Qt6::Network AppAppearanceplugin RoundedStyleplugin AppCredentials
)

qt_add_resources(AFKDrount "translations"
    PREFIX /lang
    BASE src/lang
    FILES src/lang/en.json src/lang/zh_CN.json src/lang/zh_TW.json src/lang/es_ES.json
)
file(GLOB minecraft_item_icons CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/assets/minecraft/26.1/items/*.png"
)
qt_add_resources(AFKDrount "minecraft_inventory_icons"
    PREFIX /minecraft
    BASE src/assets/minecraft
    FILES ${minecraft_item_icons} src/assets/minecraft/26.1/skins/steve.png
)

set_target_properties(AFKDrount PROPERTIES
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_BUNDLE_NAME "AFK Drount"
    MACOSX_BUNDLE_GUI_IDENTIFIER personal.MineflayerAFKHelper
    WIN32_EXECUTABLE ON
)

set(app_icon_dir "${CMAKE_CURRENT_SOURCE_DIR}/src/assets/app")
qt_add_resources(AFKDrount "application_icon"
    PREFIX /app
    BASE "${app_icon_dir}"
    FILES "${app_icon_dir}/AFKDrount.png"
)
if(APPLE)
    set(app_bundle_icon "${app_icon_dir}/AFKDrount.icns")
    set_source_files_properties("${app_bundle_icon}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(AFKDrount PRIVATE "${app_bundle_icon}")
    set_target_properties(AFKDrount PROPERTIES MACOSX_BUNDLE_ICON_FILE AFKDrount.icns)
elseif(WIN32)
    enable_language(RC)
    set(app_icon_rc "${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc")
    file(CONFIGURE OUTPUT "${app_icon_rc}" CONTENT "1 ICON \"@app_icon_dir@/AFKDrount.ico\"\n" @ONLY)
    set_source_files_properties("${app_icon_rc}" PROPERTIES OBJECT_DEPENDS "${app_icon_dir}/AFKDrount.ico")
    target_sources(AFKDrount PRIVATE "${app_icon_rc}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    install(FILES "${app_icon_dir}/AFKDrount.png"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/512x512/apps")
    install(FILES src/platform/linux/AFKDrount.desktop
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
endif()

find_program(NODE_EXECUTABLE NAMES node REQUIRED)
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/src/node/node_modules/prismarine-auth/package.json"
    OR NOT EXISTS "${PROJECT_SOURCE_DIR}/src/node/node_modules/mineflayer/package.json")
    message(FATAL_ERROR "Install backend dependencies first: npm ci --prefix src/node")
endif()
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/game-data")
execute_process(
    COMMAND "${NODE_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/src/node/generate-game-catalog.mjs"
        "${CMAKE_BINARY_DIR}/game-data/catalog.json"
    RESULT_VARIABLE catalog_result
    ERROR_VARIABLE catalog_error
)
if(NOT catalog_result EQUAL 0)
    message(FATAL_ERROR "Could not generate game catalog: ${catalog_error}")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/node/game-data.mjs"
    "${PROJECT_SOURCE_DIR}/src/node/generate-game-catalog.mjs"
    "${PROJECT_SOURCE_DIR}/src/node/package-lock.json"
)
qt_add_resources(AFKDrount "game_catalog"
    PREFIX /game-data
    BASE "${CMAKE_BINARY_DIR}/game-data"
    FILES "${CMAKE_BINARY_DIR}/game-data/catalog.json"
)
file(GLOB node_runtime_files CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/node/*.mjs"
    "${PROJECT_SOURCE_DIR}/src/node/package.json"
    "${PROJECT_SOURCE_DIR}/src/node/package-lock.json"
)
set(backend_subdir "backend")
set(node_subpath "backend/node${CMAKE_EXECUTABLE_SUFFIX}")
if(APPLE)
    set(backend_subdir "../Resources/backend")
    set(node_subpath "node")
    target_link_options(AFKDrount PRIVATE "LINKER:-headerpad_max_install_names")
endif()
target_compile_definitions(AFKDrount PRIVATE
    AFK_BACKEND_SUBDIR="${backend_subdir}"
    AFK_NODE_SUBPATH="${node_subpath}"
)
set(backend_dir "$<TARGET_FILE_DIR:AFKDrount>/${backend_subdir}")
add_custom_target(account_backend ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${backend_dir}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${node_runtime_files}
        "${backend_dir}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/src/node/node_modules"
        "${backend_dir}/node_modules"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${backend_dir}/tests"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${NODE_EXECUTABLE}"
        "$<TARGET_FILE_DIR:AFKDrount>/${node_subpath}"
    COMMENT "Copying account backend and Node runtime"
    VERBATIM
)
if(APPLE)
    # Remove the legacy generated backend directory from existing app bundles.
    add_custom_command(TARGET account_backend PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:AFKDrount>/backend"
        VERBATIM
    )
endif()
add_dependencies(AFKDrount account_backend)

install(TARGETS AFKDrount
    BUNDLE DESTINATION .
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

if(NOT APPLE)
    install(DIRECTORY "${backend_dir}"
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
        USE_SOURCE_PERMISSIONS
    )
endif()

if(APPLE)
    set(third_party_notice_dir "AFKDrount.app/Contents/Resources")
    set(installed_node_executable "AFKDrount.app/Contents/MacOS/${node_subpath}")
else()
    set(third_party_notice_dir "${CMAKE_INSTALL_DATADIR}/doc/AFKDrount")
    set(installed_node_executable "${CMAKE_INSTALL_BINDIR}/${node_subpath}")
endif()
install(FILES LICENSE THIRD_PARTY_NOTICES.md DESTINATION "${third_party_notice_dir}")
install(DIRECTORY licenses/npm DESTINATION "${third_party_notice_dir}/licenses")
configure_file(cmake/InstallNodeLicense.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/InstallNodeLicense.cmake" @ONLY)
install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/InstallNodeLicense.cmake")
install(CODE "
    include(\"${CMAKE_CURRENT_SOURCE_DIR}/cmake/QtRelease.cmake\")
    install_qt_notices(\"${Qt6_VERSION}\" \"${Qt6_DIR}\" \"${third_party_notice_dir}\"
        \"${CMAKE_CURRENT_BINARY_DIR}/QT-SOURCES.md\")
")

qt_generate_deploy_qml_app_script(
    TARGET AFKDrount
    OUTPUT_SCRIPT app_deploy_script
)
install(SCRIPT "${app_deploy_script}")

set(package_arch "${CMAKE_SYSTEM_PROCESSOR}")
if(APPLE AND CMAKE_OSX_ARCHITECTURES)
    set(package_arch "${CMAKE_OSX_ARCHITECTURES}")
elseif(WIN32 AND CMAKE_CXX_COMPILER_ARCHITECTURE_ID)
    set(package_arch "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
endif()
string(TOLOWER "${package_arch}" package_arch)
if(APPLE AND package_arch STREQUAL "arm64")
    set(package_platform macos-arm64)
elseif(package_arch MATCHES "^(x86_64|amd64|x64)$" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    if(WIN32)
        set(package_platform windows-x64)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(package_platform linux-x64)
    endif()
endif()

if(package_platform AND (CMAKE_BUILD_TYPE STREQUAL "Release" OR "Release" IN_LIST CMAKE_CONFIGURATION_TYPES))
    add_custom_target(package_release ALL
        COMMAND "${CMAKE_COMMAND}" "-DBUILD_DIR=${CMAKE_BINARY_DIR}" "-DBUILD_CONFIG=$<CONFIG>"
            "-DPLATFORM=${package_platform}" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PackageRelease.cmake"
        DEPENDS AFKDrount
        USES_TERMINAL
        VERBATIM
    )
endif()
