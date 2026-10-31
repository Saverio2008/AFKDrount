cmake_minimum_required(VERSION 3.19)

set(afk_qt_license_dir "${CMAKE_CURRENT_LIST_DIR}/../licenses/qt")
file(READ "${afk_qt_license_dir}/release.json" afk_qt_release)
string(JSON afk_qt_version GET "${afk_qt_release}" version)
set(afk_qt_modules qtbase qtdeclarative)

function(write_qt_source_notes output)
    set(notes "## Qt ${afk_qt_version} corresponding source\n\n")
    string(APPEND notes
        "AFK Drount bundles the dynamically linked Qt Base and Qt Declarative libraries under LGPLv3.\n"
        "The Qt Project provides these source archives, including the Qt build scripts, at no charge.\n\n")
    foreach(module IN LISTS afk_qt_modules)
        string(JSON commit GET "${afk_qt_release}" modules "${module}" commit)
        string(JSON source_url GET "${afk_qt_release}" modules "${module}" source_url)
        string(JSON sha256 GET "${afk_qt_release}" modules "${module}" sha256)
        get_filename_component(archive "${source_url}" NAME)
        string(APPEND notes
            "- [${archive}](${source_url})\n"
            "  · [Source commit](https://github.com/qt/${module}/tree/${commit})\n"
            "  · SHA-256 `${sha256}`\n")
    endforeach()
    string(APPEND notes
        "\nThe shipped Qt libraries come from Qt’s official desktop SDK. AFK Drount uses Qt’s unmodified source.\n"
        "The Qt license texts, replacement instructions, and platform-specific SDK build metadata are in\n"
        "`Contents/Resources/licenses/qt/` inside the macOS app, or\n"
        "`share/doc/AFKDrount/licenses/qt/` in the Windows and Linux archives.\n")
    file(WRITE "${output}" "${notes}")
endfunction()

function(install_qt_notices qt_version qt_cmake_dir notice_dir source_notes)
    if(NOT "${qt_version}" VERSION_EQUAL "${afk_qt_version}")
        message(FATAL_ERROR "Qt ${qt_version} needs its own release notices and corresponding source links.")
    endif()
    file(STRINGS "${afk_qt_license_dir}/THIRD-PARTY.txt" attribution_title LIMIT_COUNT 1)
    if(NOT attribution_title STREQUAL "Qt ${afk_qt_version} third-party attributions")
        message(FATAL_ERROR "The third-party attributions must be updated for Qt ${afk_qt_version}.")
    endif()

    get_filename_component(qt_sdk_dir "${qt_cmake_dir}/../../.." ABSOLUTE)
    set(qt_sboms)
    foreach(module IN LISTS afk_qt_modules)
        set(sbom_path "${qt_sdk_dir}/sbom/${module}-${afk_qt_version}.spdx.json")
        if(NOT EXISTS "${sbom_path}")
            message(FATAL_ERROR "Missing Qt SDK build metadata: ${sbom_path}")
        endif()
        file(READ "${sbom_path}" sbom)
        string(JSON source GET "${sbom}" packages 0 downloadLocation)
        string(JSON commit GET "${afk_qt_release}" modules "${module}" commit)
        if(NOT source MATCHES "@${commit}$")
            message(FATAL_ERROR "Qt ${module} SDK source does not match the release source links: ${source}")
        endif()
        list(APPEND qt_sboms "${sbom_path}")
    endforeach()

    set(qt_notice_destination "${CMAKE_INSTALL_PREFIX}/${notice_dir}/licenses/qt")
    write_qt_source_notes("${source_notes}")
    file(INSTALL DESTINATION "${qt_notice_destination}" TYPE FILE FILES
        "${afk_qt_license_dir}/GPL-3.0-only.txt"
        "${afk_qt_license_dir}/LGPL-3.0-only.txt"
        "${afk_qt_license_dir}/REPLACING-QT.md"
        "${afk_qt_license_dir}/THIRD-PARTY.txt"
        "${afk_qt_license_dir}/release.json")
    file(INSTALL DESTINATION "${qt_notice_destination}" TYPE FILE RENAME SOURCES.md FILES "${source_notes}")
    file(INSTALL DESTINATION "${qt_notice_destination}/sbom" TYPE FILE FILES ${qt_sboms})
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}" TYPE FILE FILES "${source_notes}")
endfunction()

if(DEFINED AFK_QT_SOURCE_NOTES_OUTPUT)
    write_qt_source_notes("${AFK_QT_SOURCE_NOTES_OUTPUT}")
endif()
