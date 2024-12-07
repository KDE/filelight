if(NOT UNIX)
    return()
endif()

SET(FILENAME_TEST_SRCS
    testFileTree.cpp testFileTree.h
)

ecm_add_test(
    ${FILENAME_TEST_SRCS}
    TEST_NAME "filetree_test"
    LINK_LIBRARIES
        Qt6::Test
        filelightInternal
)
