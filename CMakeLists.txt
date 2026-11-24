cmake_minimum_required(VERSION 3.24)
project(hatsapp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(ECM 6.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

find_package(Qt6 6.8 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    DBus
    WebEngineCore
    WebEngineWidgets
)

find_package(KF6 6.0 REQUIRED COMPONENTS
    CoreAddons
    Config
    ConfigWidgets
    DBusAddons
    I18n
    Notifications
    StatusNotifierItem
    WidgetsAddons
    WindowSystem
    XmlGui
)

add_library(hatsapp_core STATIC
    src/core/AppConfig.h
    src/core/AppConfig.cpp
    src/core/NotificationThrottler.h
    src/core/NotificationThrottler.cpp
    src/core/ProtocolRouter.h
    src/core/ProtocolRouter.cpp
    src/core/UnreadBadgeParser.h
    src/core/UnreadBadgeParser.cpp
    src/core/UrlPolicy.h
    src/core/UrlPolicy.cpp
    src/core/ZoomLevels.h
)

target_include_directories(hatsapp_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(hatsapp_core PUBLIC
    Qt6::Core
    KF6::ConfigCore
)

add_executable(hatsapp
    src/app/main.cpp
    src/app/BadgeController.h
    src/app/BadgeController.cpp
    src/app/MainWindow.h
    src/app/MainWindow.cpp
    src/app/NotificationController.h
    src/app/NotificationController.cpp
    src/app/SettingsDialog.h
    src/app/SettingsDialog.cpp
    src/app/TrayController.h
    src/app/TrayController.cpp
    src/app/WhatsAppPage.h
    src/app/WhatsAppPage.cpp
    src/app/WhatsAppView.h
    src/app/WhatsAppView.cpp
    src/resources.qrc
)

target_compile_definitions(hatsapp PRIVATE
    HATSAPP_VERSION="${PROJECT_VERSION}"
)

target_link_libraries(hatsapp PRIVATE
    hatsapp_core
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::DBus
    Qt6::WebEngineCore
    Qt6::WebEngineWidgets
    KF6::CoreAddons
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::ConfigWidgets
    KF6::DBusAddons
    KF6::I18n
    KF6::Notifications
    KF6::StatusNotifierItem
    KF6::WidgetsAddons
    KF6::WindowSystem
    KF6::XmlGui
)

install(TARGETS hatsapp
    RUNTIME DESTINATION ${KDE_INSTALL_BINDIR}
)

install(FILES data/io.github.hjr555.hatsapp.desktop
    DESTINATION ${KDE_INSTALL_APPDIR}
)

install(FILES data/hatsapp.notifyrc
    DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR}
)

install(FILES data/icons/io.github.hjr555.hatsapp.svg
    DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps
)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
