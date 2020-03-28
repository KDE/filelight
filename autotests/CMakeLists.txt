include(ECMAddTests)

find_package(Qt5Test ${QT_MIN_VERSION} REQUIRED Test)
find_package(KF5CoreAddons ${KF5_DEP_VERSION} REQUIRED)

add_subdirectory(core)
