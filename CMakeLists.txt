cmake_minimum_required(VERSION 3.5)

project(join_server VERSION 0.0.$ENV{TRAVIS_BUILD_NUMBER})

find_package(Threads)
find_package(Boost COMPONENTS system REQUIRED)
# find_package(MYSQLCPPCONN REQUIRED)

include_directories(${Boost_INCLUDE_DIR})
# include_directories(${MYSQLCPPCONN_INCLUDE_DIRS})

add_executable(join_server main.cpp)

set_target_properties(join_server PROPERTIES
	CXX_STANDARD 14
	CXX_STANDARD_REQUIRED ON
	COMPILE_OPTIONS "-Wpedantic;-Wall;-Wextra;"
)

target_link_libraries(join_server
	${CMAKE_THREAD_LIBS_INIT}
	${Boost_LIBRARIES}
	# ${MYSQLCPPCONN_LIBRARIES}
	mysqlcppconn
)

install(TARGETS join_server RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT sava_test@mail.ru)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

include (CPack)
