set_source_files_properties(Theme.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)
qt_add_qml_module(AppAppearance STATIC
    URI AppAppearance
    VERSION 1.0
    CLASS_NAME AppAppearancePlugin
    OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/AppAppearance"
    IMPORTS QtCore
    QML_FILES Theme.qml
)
target_link_libraries(AppAppearance PRIVATE Qt6::Quick)