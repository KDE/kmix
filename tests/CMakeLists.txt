########### next target ###############
set(profiletest_SRCS profiletest.cpp )

add_executable(profiletest ${profiletest_SRCS})

target_link_libraries(profiletest
    kmixgui
    Qt::Core
    Qt::DBus
    Qt::Test
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::I18n
    KF6::Solid
)

add_test(NAME profiletest
    COMMAND profiletest
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

