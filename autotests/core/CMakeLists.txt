SET(FILENAME_TEST_SRCS
    testFileTree.cpp
)

ecm_add_test(
    ${FILENAME_TEST_SRCS}
    TEST_NAME "filetree_test"
    LINK_LIBRARIES  Qt${QT_MAJOR_VERSION}::Test filelightInternal
    KF5::CoreAddons
)
