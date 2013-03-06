project(ktp-auth-handler)

set(KTP_AUTH_HANDLER_VERSION "0.5.80")

set (CMAKE_MODULE_PATH
     "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
     ${CMAKE_MODULE_PATH}
)

set(IS_KTP_INTERNAL_MODULE TRUE)
set(KDE_MIN_VERSION "4.4.75")
find_package (KDE4 4.4.75 REQUIRED)
find_package (TelepathyQt4 0.8.9 REQUIRED)
find_package (KTp REQUIRED)
find_package (QJSON REQUIRED)
find_package (QCA2 REQUIRED)

include (KDE4Defaults)
include (MacroLibrary)

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

configure_file(version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)

kde4_add_ui_files(ktp_auth_handler_SRCS x-telepathy-password-prompt.ui)
kde4_add_executable(ktp-auth-handler ${ktp_auth_handler_SRCS})

target_link_libraries(ktp-auth-handler
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
