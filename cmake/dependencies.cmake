# Zlib & BZip2
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)

include_directories(SYSTEM ${ZLIB_INCLUDE_DIR})
include_directories(SYSTEM ${BZIP2_INCLUDE_DIR})

# DevIL
find_package(DevIL REQUIRED)
include_directories(SYSTEM ${IL_INCLUDE_DIR})

# LibArchive
find_package(LibArchive REQUIRED)
include_directories(SYSTEM ${LibArchive_INCLUDE_DIRS})

# Eigen
find_package(Eigen3)
include_directories(${EIGEN3_INCLUDE_DIR})
add_definitions(-DEIGEN_DONT_PARALLELIZE)
