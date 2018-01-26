# https://gist.github.com/Rod-Persky/e6b93e9ee31f9516261b
# This depends on the compiler we are using, see top level file for DestDir
if (WIN32)
get_target_property(QT5_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
get_filename_component(QT5_WINDEPLOYQT_EXECUTABLE ${QT5_QMAKE_EXECUTABLE} PATH)
set(QT5_WINDEPLOYQT_EXECUTABLE "${QT5_WINDEPLOYQT_EXECUTABLE}/windeployqt.exe")

if(CMAKE_BUILD_TYPE STREQUAL Release)
    set(BinaryType "--release")
else()
    set(BinaryType "--debug")
endif()

add_custom_command(TARGET Jahshaka POST_BUILD
    COMMAND ${QT5_WINDEPLOYQT_EXECUTABLE} ${BinaryType} ${DestDir} $<TARGET_FILE_DIR:Jahshaka>)
endif(WIN32)