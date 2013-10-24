project(ktp-auth-handler)

set(KTP_AUTH_HANDLER_VERSION "0.7.0")

set (CMAKE_MODULE_PATH
     "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
     ${CMAKE_MODULE_PATH}
)
cmake_policy(SET CMP0002 OLD)

set(IS_KTP_INTERNAL_MODULE TRUE)
set(KDE_MIN_VERSION "4.4.75")
find_package (KDE4 4.4.75 REQUIRED)
find_package (TelepathyQt4 0.8.9 REQUIRED)
find_package (KTp REQUIRED)
find_package (QJSON REQUIRED)
find_package (QCA2 REQUIRED)
find_package (AccountsQt)
find_package (SignOnQt)

include (KDE4Defaults)
include (MacroLibrary)

macro_log_feature(ACCOUNTSQT_FOUND "AccountsQt" "Support for Accounts-SSO" "https://code.google.com/p/accounts-sso/source/checkout?repo=libaccounts-qt" FALSE "" "")
macro_log_feature(SIGNONQT_FOUND "SignonsQt" "Support for Accounts-SSO" "https://code.google.com/p/accounts-sso/source/checkout?repo=signond" FALSE "" "")
macro_display_feature_log()

add_definitions (${KDE4_DEFINITIONS})

include_directories (${KDE4_INCLUDES}
                     ${TELEPATHY_QT4_INCLUDE_DIR}
                     ${KTP_INCLUDE_DIR}
                     ${QJSON_INCLUDE_DIR}
                     ${QCA2_INCLUDE_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}/kssl
)

set(ktp_auth_handler_SRCS
    main.cpp
    sasl-handler.cpp
    sasl-auth-op.cpp
    tls-cert-verifier-op.cpp
    tls-handler.cpp
    types.cpp
    x-telepathy-password-auth-operation.cpp
    x-telepathy-password-prompt.cpp
    x-messenger-oauth2-auth-operation.cpp
    x-messenger-oauth2-prompt.cpp
    conference-auth-op.cpp
    conference-auth-observer.cpp
)

set(ktp_auth_handler_LIBS
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTXML_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTWEBKIT_LIBRARY}
    ${TELEPATHY_QT4_LIBRARIES}
    ${KTP_LIBRARIES}
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KDEWEBKIT_LIBS}
    ${QJSON_LIBRARIES}
    ${QCA2_LIBRARIES}
)

#If we find Accounts-SSO frameworks, add support for it
if(ACCOUNTSQT_FOUND AND SIGNONQT_FOUND)

    add_definitions(-DHAVE_SSO)
    set(ktp_auth_handler_SRCS
        ${ktp_auth_handler_SRCS}
        x-telepathy-sso-facebook-operation.cpp
        x-telepathy-sso-google-operation.cpp
        getcredentialsjob.cpp
    )

    set(ktp_auth_handler_LIBS
        ${ktp_auth_handler_LIBS}
        ${ACCOUNTSQT_LIBRARIES}
        ${SIGNONQT_LIBRARIES}
    )

    include_directories(${ACCOUNTSQT_INCLUDE_DIRS}
                        ${SIGNONQT_INCLUDE_DIRS}
    )

endif(ACCOUNTSQT_FOUND AND SIGNONQT_FOUND)

configure_file(version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)

kde4_add_ui_files(ktp_auth_handler_SRCS x-telepathy-password-prompt.ui)
kde4_add_executable(ktp-auth-handler ${ktp_auth_handler_SRCS})

target_link_libraries(ktp-auth-handler
    ${ktp_auth_handler_LIBS}
)

configure_file(org.freedesktop.Telepathy.Client.KTp.SASLHandler.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KTp.SASLHandler.service)
configure_file(org.freedesktop.Telepathy.Client.KTp.TLSHandler.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KTp.TLSHandler.service)

configure_file(org.freedesktop.Telepathy.Client.KTp.ConfAuthObserver.service.in
    ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KTp.ConfAuthObserver.service)

install(TARGETS ktp-auth-handler DESTINATION ${LIBEXEC_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KTp.SASLHandler.service
    ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KTp.ConfAuthObserver.service
        DESTINATION ${DBUS_SERVICES_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KTp.TLSHandler.service
        DESTINATION ${DBUS_SERVICES_INSTALL_DIR})
install(FILES KTp.SASLHandler.client DESTINATION ${SHARE_INSTALL_PREFIX}/telepathy/clients/)
install(FILES KTp.TLSHandler.client DESTINATION ${SHARE_INSTALL_PREFIX}/telepathy/clients/)
install(FILES KTp.ConfAuthObserver.client DESTINATION ${SHARE_INSTALL_PREFIX}/telepathy/clients/)
