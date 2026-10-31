# Using a replacement Qt build

AFK Drount dynamically links Qt under LGPLv3. You may modify Qt and reverse
engineer the combined work to debug those modifications.

Use the sources listed in the [Qt release manifest](release.json) or in the
generated `QT-SOURCES.md` to build Qt Base and Qt Declarative. Use the same
Qt version, architecture, and compiler ABI as the package, and compatible Qt
build options. The SDK build configuration is recorded in the
SPDX files under `sbom/`. The [Qt build documentation](https://doc.qt.io/qt-6/build-sources.html)
explains how to configure and build the modules.

Make a copy of the extracted application before replacing Qt. When a change
affects several libraries, replace the corresponding Qt libraries and plugins
together. Keep the deployed paths and loader names, including the QML import
directory in `qt.conf`.

- **macOS arm64.** Replace the relevant libraries under
  `AFKDrount.app/Contents/Frameworks` and any corresponding Qt plugins under
  `Contents/PlugIns` or QML imports under `Contents/Resources/qml`. Use
  `otool -L` to compare dependency names with those of the original files.
  If you replace QtConcurrent, for example, put the compatible binary at
  `AFKDrount.app/Contents/Frameworks/QtConcurrent.framework/Versions/A/QtConcurrent`.
  Preserve the framework layout and sign modified frameworks and plugins
  before signing the application bundle for local use.

  ```sh
  codesign --force --sign - AFKDrount.app/Contents/Frameworks/QtConcurrent.framework
  codesign --force --sign - AFKDrount.app
  codesign --verify --deep --strict AFKDrount.app
  AFKDrount.app/Contents/MacOS/AFKDrount
  ```

- **Windows x64.** Use Qt built for the MSVC 2022 x64 ABI. Replace the
  corresponding Qt DLLs next to the executable and the matching plugin DLLs
  and QML imports in their deployed directories. Launch `bin/AFKDrount.exe`.
- **Linux x64.** Use a compatible GCC x64 build. Replace the corresponding Qt
  shared libraries in the package’s library directory and preserve their
  versioned symbolic links. Replace the matching plugins and QML imports in
  their deployed directories. Run `ldd bin/AFKDrount` to check where the
  executable resolves Qt before launching it.

You can also build AFK Drount against your Qt installation. Run
`npm ci --prefix src/node` from the project root. Configure a fresh CMake build
directory with `CMAKE_PREFIX_PATH` pointing to your Qt installation, then build
the `AFKDrount` target and launch the executable from the build directory.
