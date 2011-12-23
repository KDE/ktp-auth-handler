project(ktp-auth-handler)

set (CMAKE_MODULE_PATH
     "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
     ${CMAKE_MODULE_PATH}
)

set(IS_KTP_INTERNAL_MODULE TRUE)
set(KDE_MIN_VERSION "4.4.75")
find_package (KDE4 4.4.75 REQUIRED)
find_package (TelepathyQt4 0.8.9 REQUIRED)
find_package (KTp REQUIRED)

include (KDE4Defaults)
include (MacroLibrary)

add_definitions (${KDE4_DEFINITIONS}
)

include_directories (${KDE4_INCLUDES}
                     ${TELEPATHY_QT4_INCLUDE_DIR}
                     ${KTP_INCLUDE_DIR}
)

set(ktp_auth_handler_SRCS
    main.cpp
    password-prompt.cpp
    sasl-handler.cpp
    sasl-auth-op.cpp
    tls-cert-verifier-op.cpp
    tls-handler.cpp
    types.cpp
    ${ktp_common_internals_SRCS}
)

kde4_add_ui_files(ktp_auth_handler_SRCS password-prompt.ui)
kde4_add_executable(ktp-auth-handler ${ktp_auth_handler_SRCS})

target_link_libraries(ktp-auth-handler
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTXML_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${TELEPATHY_QT4_LIBRARIES}
    ${KTP_LIBRARIES}
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
)

configure_file(org.freedesktop.Telepathy.Client.KDE.SASLHandler.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KDE.SASLHandler.service)
#configure_file(org.freedesktop.Telepathy.Client.KDE.TLSHandler.service.in
#               ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KDE.TLSHandler.service)


install(TARGETS ktp-auth-handler DESTINATION ${LIBEXEC_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KDE.SASLHandler.service
        DESTINATION ${DBUS_SERVICES_INSTALL_DIR})
#install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KDE.TLSHandler.service
#        DESTINATION ${DBUS_SERVICES_INSTALL_DIR})
install(FILES KDE.SASLHandler.client DESTINATION ${SHARE_INSTALL_PREFIX}/telepathy/clients/)
#install(FILES KDE.TLSHandler.client DESTINATION ${SHARE_INSTALL_PREFIX}/telepathy/clients/)
