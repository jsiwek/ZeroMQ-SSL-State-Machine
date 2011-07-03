cmake_minimum_required(VERSION 2.8)
project(zeromq-ssl-state-machine)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -g")

set(COMMON_SRC state_machine.c state_machine.h)
set(KEYS_FILE ${PROJECT_SOURCE_DIR}/server.pem)
set(TEST_PORT 10000)

add_executable(sm_server sm_server.c ${COMMON_SRC})
target_link_libraries(sm_server ssl crypto)

add_executable(sm_client sm_client.c ${COMMON_SRC})
target_link_libraries(sm_client ssl crypto)

add_custom_target(test_server
    COMMAND sm_server ${TEST_PORT} ${KEYS_FILE} ${KEYS_FILE}
    DEPENDS sm_server
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMENT "Starting state machine test server (port ${TEST_PORT})."
    SOURCES ${KEYS_FILE}
)

add_custom_target(test_client
    COMMAND sm_client ${TEST_PORT} ${KEYS_FILE} ${KEYS_FILE}
    DEPENDS sm_client
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMENT "Starting state machine test client."
    SOURCES ${KEYS_FILE}
)
