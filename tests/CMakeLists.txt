########### next target ###############
set(profiletest_SRCS profiletest.cpp )

add_executable(profiletest ${profiletest_SRCS})

target_link_libraries(profiletest
    kmixgui
    Qt::Core
    Qt::DBus
    Qt::Test
    KF5::ConfigCore
    KF5::ConfigGui
    KF5::I18n
    KF5::Solid
)

add_test(NAME profiletest
    COMMAND profiletest
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

