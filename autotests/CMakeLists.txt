if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    message(FATAL_ERROR "Building tests in-source is not supported, please use a separate build dir or -DBUILD_TESTING=OFF.")
endif()

include(ECMAddTests)

find_package(Qt6Test ${QT_MIN_VERSION} REQUIRED Test)

add_subdirectory(core)

# Synthesize links. On some windowses the git install is set to not support symlinks (plus it may not be able to
# create them). Instead synthesize the links at configure time and configure the test accordingly.
file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/iterator-tree.in ${CMAKE_CURRENT_BINARY_DIR}/iterator-tree)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/iterator-tree.in DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(RENAME ${CMAKE_CURRENT_BINARY_DIR}/iterator-tree.in ${CMAKE_CURRENT_BINARY_DIR}/iterator-tree)
set(ITERATOR_TREE_WITH_SYMLINK ON)
file(CREATE_LINK ${CMAKE_CURRENT_BINARY_DIR}/iterator-tree/bar ${CMAKE_CURRENT_BINARY_DIR}/iterator-tree/symlink RESULT ITERATOR_TREE_WITH_SYMLINK SYMBOLIC)
set(ITERATOR_TREE_WITH_LINK ON)
file(CREATE_LINK ${CMAKE_CURRENT_BINARY_DIR}/iterator-tree/bar ${CMAKE_CURRENT_BINARY_DIR}/iterator-tree/link RESULT ITERATOR_TREE_WITH_LINK)
configure_file(test-config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/test-config.h)

ecm_add_test(directoryIteratorTest.cpp TEST_NAME directoryIteratorTest LINK_LIBRARIES Qt::Test filelightInternal)
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    # Fails on freebsd for unknown reasons -> only run it on linux.
    ecm_add_test(localListerTest.cpp TEST_NAME localListerTest LINK_LIBRARIES Qt::Test filelightInternal)
endif()
ecm_add_test(scanManagerTest.cpp TEST_NAME scanManagerTest LINK_LIBRARIES Qt::Test filelightInternal)
