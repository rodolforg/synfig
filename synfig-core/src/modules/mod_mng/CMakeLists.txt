add_library(mod_mng MODULE
        "${CMAKE_CURRENT_LIST_DIR}/trgt_mng.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/main.cpp"
)

target_link_libraries(mod_mng PRIVATE libsynfig PkgConfig::LIBMNG)

install (
    TARGETS mod_mng
    DESTINATION lib/synfig/modules
)
