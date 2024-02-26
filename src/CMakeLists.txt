find_library(ZBD_LIBRARIES NAMES zbd REQUIRED)

add_library(zbuf common.cpp datastructure.cpp zalp.cpp zBuffer.cpp zController.cpp)

target_link_libraries(zbuf zbd)