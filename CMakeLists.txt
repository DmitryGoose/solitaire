cmake_minimum_required(VERSION 3.10)
project(Solitaire VERSION 1.0)

# Установка стандарта C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Поиск SFML
find_package(SFML 2.5 COMPONENTS graphics audio window system REQUIRED)

# Поиск nlohmann_json (установленного через apt)
find_package(nlohmann_json 3 REQUIRED)

# Включаем директории с заголовочными файлами
include_directories(${CMAKE_SOURCE_DIR}/include)

# Собираем исходники
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Создание исполняемого файла
add_executable(${PROJECT_NAME} ${SOURCES})

# Линковка SFML и JSON
target_link_libraries(${PROJECT_NAME}
    sfml-graphics
    sfml-audio
    sfml-window
    sfml-system
    nlohmann_json::nlohmann_json
)

# Копирование ресурсов в директорию сборки
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
