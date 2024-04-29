########### next target ###############
set(profiletest_SRCS profiletest.cpp )

add_executable(profiletest ${profiletest_SRCS})

target_link_libraries(profiletest
    kmixgui
    Qt::Core
    Qt::DBus
    Qt::Test
    KF${QT_MAJOR_VERSION}::ConfigCore
    KF${QT_MAJOR_VERSION}::ConfigGui
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::Solid
)

add_test(NAME profiletest
    COMMAND profiletest
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

