cmake_minimum_required(VERSION 3.16)

# KDE Application Version, managed by release script
set(RELEASE_SERVICE_VERSION_MAJOR "23")
set(RELEASE_SERVICE_VERSION_MINOR "07")
set(RELEASE_SERVICE_VERSION_MICRO "70")
set(KTP_AUTH_HANDLER_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(ktp-auth-handler VERSION ${KTP_AUTH_HANDLER_VERSION})

find_package(Qt5 5.3 CONFIG REQUIRED COMPONENTS DBus Gui Core Network) #Network for QSsl

find_package(ECM 1.6.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

find_package(KF5 5.0 REQUIRED COMPONENTS Wallet WidgetsAddons I18n KIO)

find_package (TelepathyQt5 0.8.9 REQUIRED)
find_package (KTp REQUIRED)
find_package (Qca-qt5 REQUIRED)
find_package(AccountsQt5 1.10 REQUIRED CONFIG)
find_package(SignOnQt5 8.55 REQUIRED CONFIG)
find_package(KAccounts REQUIRED)

# include (MacroLibrary)
#
# macro_log_feature(ACCOUNTSQT_FOUND "AccountsQt" "Support for Accounts-SSO" "https://code.google.com/p/accounts-sso/source/checkout?repo=libaccounts-qt" FALSE "" "")
# macro_log_feature(SIGNONQT_FOUND "SignonsQt" "Support for Accounts-SSO" "https://code.google.com/p/accounts-sso/source/checkout?repo=signond" FALSE "" "")
# macro_display_feature_log()
message("-----------------------------------------------------------------------------")
message("IMPORTANT: There is a runtime dependency on QCA-OSSL. ")
message("This project will build, but will fail to run properly. Please ensure it is installed.")
message("If you are a packager you _must_ mark this as a dependency.")
message("-----------------------------------------------------------------------------")

find_package(Qca-qt5-ossl QUIET)
set_package_properties(Qca-qt5-ossl PROPERTIES
         PURPOSE "Runtime-only dependency on the OSSL plugin of QCA for authentication to work properly"
         TYPE RUNTIME
         )

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMMarkNonGuiExecutable)
include(CMakePackageConfigHelpers)
include(FeatureSummary)

include_directories (${QCA_INCLUDE_DIR}
                     ${ACCOUNTSQT_INCLUDE_DIRS}
                     ${SIGNONQT_INCLUDE_DIRS}
)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

set(ktp_auth_handler_SRCS
    main.cpp
    sasl-handler.cpp
    sasl-auth-op.cpp
    tls-cert-verifier-op.cpp
    tls-handler.cpp
    types.cpp
    x-telepathy-password-auth-operation.cpp
    x-telepathy-password-prompt.cpp
    x-telepathy-sso-google-operation.cpp
    conference-auth-op.cpp
    conference-auth-observer.cpp
)

set(ktp_auth_handler_LIBS
    KTp::CommonInternals
    qca-qt5
    ${ACCOUNTSQT_LIBRARIES}
    ${SIGNONQT_LIBRARIES}
    KAccounts
    KF5::WidgetsAddons
    KF5::Wallet
    KF5::I18n
    KF5::KIOCore
    KF5::KIOWidgets
    Qt5::Core
    Qt5::DBus
)

configure_file(version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)

ki18n_wrap_ui(ktp_auth_handler_SRCS x-telepathy-password-prompt.ui)
add_executable(ktp-auth-handler ${ktp_auth_handler_SRCS})

target_link_libraries(ktp-auth-handler
    ${ktp_auth_handler_LIBS}
)

configure_file(org.freedesktop.Telepathy.Client.KTp.SASLHandler.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KTp.SASLHandler.service)
configure_file(org.freedesktop.Telepathy.Client.KTp.TLSHandler.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KTp.TLSHandler.service)

configure_file(org.freedesktop.Telepathy.Client.KTp.ConfAuthObserver.service.in
    ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KTp.ConfAuthObserver.service)

install(TARGETS ktp-auth-handler DESTINATION ${KDE_INSTALL_LIBEXECDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KTp.SASLHandler.service
    ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KTp.ConfAuthObserver.service
        DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KTp.TLSHandler.service
        DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR})
install(FILES KTp.SASLHandler.client DESTINATION ${KDE_INSTALL_DATAROOTDIR}/telepathy/clients/)
install(FILES KTp.TLSHandler.client DESTINATION ${KDE_INSTALL_DATAROOTDIR}/telepathy/clients/)
install(FILES KTp.ConfAuthObserver.client DESTINATION ${KDE_INSTALL_DATAROOTDIR}/telepathy/clients/)

ki18n_install(po)
