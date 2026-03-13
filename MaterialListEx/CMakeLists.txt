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

#拷贝配置文件
set(MY_CONFIG_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/config/zh_cn.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/config/item_stack_count.json"
)
copy_files_to_target(MaterialListEx "${CONFIG_FILES}" "$<TARGET_FILE_DIR:MaterialListEx>")
