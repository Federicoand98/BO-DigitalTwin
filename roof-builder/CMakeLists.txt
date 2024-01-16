cmake_minimum_required (VERSION 3.8)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("roof-builder")

include(FetchContent)
set(CMAKE_CXX_STANDARD 20)

set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs")

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

FetchContent_Declare(
	DataFrame
	GIT_REPOSITORY "https://github.com/hosseinmoein/DataFrame.git"
	GIT_TAG        "master"
	GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(DataFrame)

FetchContent_Declare(
	geos
	GIT_REPOSITORY "https://github.com/libgeos/geos.git"
	GIT_TAG        "main"
	GIT_PROGRESS TRUE
)
FetchContent_GetProperties(geos)
if (NOT geos_POPULATED)
	FetchContent_Populate(geos)
endif ()
FetchContent_MakeAvailable(geos)

FetchContent_Declare(
	opencv2
	GIT_REPOSITORY "https://github.com/opencv/opencv.git"
	GIT_TAG        "4.x"
	GIT_PROGRESS TRUE
)
FetchContent_GetProperties(opencv2)
if (NOT opencv2_POPULATED)
	FetchContent_Populate(opencv2)
endif ()
FetchContent_MakeAvailable(opencv2)

set(OpenCV_DIR ${CMAKE_CURRENT_BINARY_DIR})
include_directories(${OpenCV_INCLUDE_DIRS})
find_package(OpenCV REQUIRED)

find_package(PDAL 1.0.0 REQUIRED CONFIG)
include_directories(${PDAL_INCLUDE_DIRS})
link_directories(${PDAL_LIBRARY_DIRS})
add_definitions(${PDAL_DEFINITIONS})

set(GEOS_INCLUDE_DIRS ${geos_SOURCE_DIR}/include)
set(GEOS_LIBRARIES ${geos_BINARY_DIR}/lib)
include_directories(${GEOS_INCLUDE_DIRS})

add_executable (roof-builder ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE DataFrame geos ${PDAL_LIBRARIES} ${OpenCV_LIBS})

list(APPEND CMAKE_PREFIX_PATH ${geos_BINARY_DIR})

target_compile_definitions(${PROJECT_NAME} PUBLIC ASSETS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/" OUTPUT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/output/" MULTITHREADING=1)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET roof-builder PROPERTY CXX_STANDARD 20)
endif()
