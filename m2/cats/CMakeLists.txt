cmake_minimum_required(VERSION 3.10)
project(CatCollage)

# Указываем стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Добавляем libcurl
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})

# Добавляем OpenCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# Основной исполняемый файл
add_executable(cats main.cpp)

# Линковка библиотек, включая pthread
target_link_libraries(cats ${CURL_LIBRARIES} ${OpenCV_LIBS} -pthread)