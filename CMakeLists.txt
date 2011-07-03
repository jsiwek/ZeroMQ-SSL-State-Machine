cmake_minimum_required(VERSION 2.8)
project(zeromq-ssl-state-machine)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -g")

add_executable(state_machine state_machine.c)
target_link_libraries(state_machine ssl crypto)

add_custom_target(test
    COMMAND ${PROJECT_BINARY_DIR}/state_machine 10000
            ${PROJECT_SOURCE_DIR}/server.pem ${PROJECT_SOURCE_DIR}/server.pem
    DEPENDS state_machine
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMENT "Starting state machine test server."
    SOURCES server.pem
)
