set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_library(async SHARED
	async.cpp
	async.h
	Context.h
	Context.cpp)

target_compile_features(async PUBLIC cxx_std_17)
target_include_directories(async INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(example main.cpp)
target_link_libraries(example async)