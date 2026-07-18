add_executable(portpulse portpulse.c)
target_compile_features(portpulse PRIVATE c_std_11)

if (WIN32)
    target_link_libraries(portpulse PRIVATE setupapi)
else()
    target_link_libraries(portpulse PRIVATE m)
endif()
