cmake_minimum_required(VERSION 3.13)
project(MaterialListEx LANGUAGES CXX)

add_executable(MaterialListEx
    main.cpp
    LitematicToMaterialList.cpp
    RegionProcess.cpp
)

target_link_libraries(MaterialListEx
    PUBLIC
        ${COMMON_LIBS}
)
