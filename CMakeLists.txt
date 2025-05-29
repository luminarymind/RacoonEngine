cmake_minimum_required(VERSION 3.6)

project(RacoonEngine)

add_subdirectory(Cauldron)

file(GLOB racoon_sources
    src/*.cpp
    src/*.h)

add_executable(RacoonEngine WIN32 ${racoon_sources})
target_link_libraries(RacoonEngine LINK_PUBLIC Cauldron_DX12 ImGUI d3dcompiler D3D12 amd_ags)

if(MSVC)
    set_target_properties(RacoonEngine PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/bin" DEBUG_POSTFIX "d")
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT RacoonEngine)
endif()