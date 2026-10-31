set(rounded_style_files
    ButtonSurface.qml
    Button.qml
    ToolButton.qml
    TextField.qml
    Pane.qml
    Label.qml
    ItemDelegate.qml
    SplitView.qml
    ToolBar.qml
    Dialog.qml
    DialogButtonBox.qml
)
qt_add_qml_module(RoundedStyle STATIC
    URI Rounded
    CLASS_NAME RoundedPlugin
    OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Rounded"
    IMPORTS AppAppearance
    QML_FILES ${rounded_style_files}
)
target_link_libraries(RoundedStyle PRIVATE AppAppearance Qt6::QuickControls2 Qt6::QuickTemplates2)