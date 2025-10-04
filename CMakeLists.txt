cmake_minimum_required(VERSION 3.16)
project(sleepbook)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt5
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

# Enable automatic MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Source files
set(SOURCES
        main.cpp
        classes/mainwindow.cpp
        classes/symptomwidget.cpp
        classes/symptom.cpp
        classes/logindialog.cpp
        classes/usermanager.cpp
        classes/user.cpp
)

set(HEADERS
        classes/mainwindow.h
        classes/symptomwidget.h
        classes/symptom.h
        classes/logindialog.h
        classes/usermanager.h
        classes/user.h
)

# Create executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../binary)
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link Qt5 libraries
target_link_libraries(${PROJECT_NAME}
        Qt5::Core
        Qt5::Widgets
)