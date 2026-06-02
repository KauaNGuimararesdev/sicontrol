cmake_minimum_required(VERSION 3.21)

project(SiControl VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Qt 6.5+ recommended (loadFromModule, modern Android Kotlin packaging).
find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Qml Network Gui Core)

qt_standard_project_setup(REQUIRES 6.5)

# ---- Application target ------------------------------------------------------
qt_add_executable(SiControl
    src/main.cpp
    # Header-only model/protocol units carry Q_OBJECT, so list them for AUTOMOC.
    src/model/Channel.h
    src/model/MixerModel.h
    src/model/ParameterMap.h
    src/hiqnet/HiQnetTypes.h
    src/hiqnet/HiQnetCodec.h
    src/hiqnet/HiQnetClient.h
    src/hiqnet/StateVarCatalog.h
)

# Android-only JNI bridge to the Kotlin transport.
if(ANDROID)
    target_sources(SiControl PRIVATE src/platform/AndroidBridge.cpp)
endif()

target_include_directories(SiControl PRIVATE src)

# ---- QML module --------------------------------------------------------------
# Theme is a singleton; mark it so it is usable unqualified within the module.
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(SiControl
    URI SiControl
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/controls/ViSiFader.qml
        qml/controls/PanControl.qml
        qml/controls/RotaryKnob.qml
        qml/controls/EqGraph.qml
        qml/views/DeviceSelectView.qml
        qml/views/BusSelectView.qml
        qml/views/MixView.qml
        qml/views/ChannelStripView.qml
        qml/views/MatrixView.qml
        qml/views/MuteGroupsView.qml
        qml/views/SettingsView.qml
    RESOURCES
        qml/js/EqMath.js
)

target_link_libraries(SiControl PRIVATE
    Qt6::Quick
    Qt6::Qml
    Qt6::Network
    Qt6::Gui
    Qt6::Core
)

# ---- Android packaging -------------------------------------------------------
# Points Qt's androiddeployqt at android/ (AndroidManifest.xml, build.gradle,
# Kotlin sources under src/). Qt merges these into the generated Gradle project.
if(ANDROID)
    set_target_properties(SiControl PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android
        QT_ANDROID_MIN_SDK_VERSION 29
        QT_ANDROID_TARGET_SDK_VERSION 34
    )
endif()
