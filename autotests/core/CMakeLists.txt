SET(FILENAME_TEST_SRCS
    testFileTree.cpp
    ../../src/fileTree.cpp
)

ecm_add_test(
    ${FILENAME_TEST_SRCS}
    TEST_NAME "filetree_test"
    LINK_LIBRARIES  Qt${QT_MAJOR_VERSION}::Test
    KF5::CoreAddons
)
