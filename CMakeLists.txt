cmake_minimum_required(VERSION 3.16)
project(sleepbook LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../binary)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../binary)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../binary)

# Find Qt5
find_package(Qt5 REQUIRED COMPONENTS Core Widgets OpenGL)

# Enable automatic Qt tools
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Sources
set(SOURCES
        main.cpp
        classes/mainwindow.cpp
        classes/symptomwidget.cpp
        classes/symptom.cpp
        classes/logindialog.cpp
        classes/usermanager.cpp
        classes/user.cpp
        classes/dataencryption.cpp
        classes/qcustomplot.cpp
        classes/histogramwidget.cpp
        classes/datapathmanager.cpp
        classes/wordcloudwidget.cpp
)

set(HEADERS
        classes/mainwindow.h
        classes/symptomwidget.h
        classes/symptom.h
        classes/logindialog.h
        classes/usermanager.h
        classes/user.h
        classes/dataencryption.h
        classes/qcustomplot.h
        classes/histogramwidget.h
        classes/datapathmanager.h
        classes/wordcloudwidget.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Enable OpenGL in QCustomPlot
target_compile_definitions(${PROJECT_NAME} PRIVATE QCUSTOMPLOT_USE_OPENGL)

# Link Qt5 libraries
target_link_libraries(${PROJECT_NAME}
        Qt5::Core
        Qt5::Widgets
        Qt5::OpenGL
)

# --- Platform-specific optimization flags ---
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        # Windows (MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE
                /O2      # Optimize for speed
                /DNDEBUG # Disable assertions
                /GL      # Whole Program Optimization
        )
        target_link_options(${PROJECT_NAME} PRIVATE
                /LTCG    # Link-time code generation
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # Linux/macOS (GCC or Clang)
        target_compile_options(${PROJECT_NAME} PRIVATE
                -O3
                -DNDEBUG
                -flto
                -ffast-math
                -funroll-loops
        )
        target_link_options(${PROJECT_NAME} PRIVATE
                -flto
                -s
        )
    endif()
endif()

# --- macOS bundle settings ---
if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.sleepbook.app"
            MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    )
endif()

# --- Windows-specific: ensure console hidden for GUI apps ---
if(WIN32)
    # If you want a GUI app with no console window:
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
