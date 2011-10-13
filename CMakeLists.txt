project(telepathy-kde-auth-handler)

set (CMAKE_MODULE_PATH
     "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
     ${CMAKE_MODULE_PATH}
)

set(KDE_MIN_VERSION "4.4.75")
find_package (KDE4 4.4.75 REQUIRED)
find_package (TelepathyQt4 0.7.3 REQUIRED)

include (KDE4Defaults)
include (MacroLibrary)

add_definitions (${KDE4_DEFINITIONS}
)

include_directories (${KDE4_INCLUDES}
                     ${TELEPATHY_QT4_INCLUDE_DIR}
)


if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.gitmodules)
execute_process(COMMAND git submodule init
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

execute_process(COMMAND git submodule update
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
endif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.gitmodules)


set (telepathy_kde_common_internals_SRCS
     common/telepathy-handler-application.cpp
)

set(telepathy_kde_auth_handler_SRCS
    main.cpp
    password-prompt.cpp
    sasl-handler.cpp
    sasl-auth-op.cpp
    tls-cert-verifier-op.cpp
    tls-handler.cpp
    types.cpp
    ${telepathy_kde_common_internals_SRCS}
)

kde4_add_ui_files(telepathy_kde_auth_handler_SRCS password-prompt.ui)
kde4_add_executable(telepathy-kde-auth-handler ${telepathy_kde_auth_handler_SRCS})

target_link_libraries(telepathy-kde-auth-handler
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTXML_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${TELEPATHY_QT4_LIBRARIES}
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
)

configure_file(org.freedesktop.Telepathy.Client.KDE.SASL.Handler.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KDE.SASL.Handler.service)
configure_file(org.freedesktop.Telepathy.Client.KDE.TLS.Handler.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KDE.TLS.Handler.service)


install(TARGETS telepathy-kde-auth-handler DESTINATION ${LIBEXEC_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KDE.SASL.Handler.service
        DESTINATION ${DBUS_SERVICES_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KDE.TLS.Handler.service
        DESTINATION ${DBUS_SERVICES_INSTALL_DIR})
install(FILES KDE.AuthHandler.client DESTINATION ${SHARE_INSTALL_PREFIX}/telepathy/clients/)
