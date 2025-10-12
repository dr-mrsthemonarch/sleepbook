
cmake_minimum_required(VERSION 3.16)
project(sleepbook)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Release optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # Compiler-specific optimizations
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${PROJECT_NAME} PRIVATE
                -O3                    # Maximum optimization
                -DNDEBUG              # Disable debug assertions
                -march=native         # Optimize for current CPU architecture
                -flto                 # Link-time optimization
                -ffast-math           # Fast math optimizations
                -funroll-loops        # Unroll loops for better performance
        )
        target_link_options(${PROJECT_NAME} PRIVATE
                -flto                 # Link-time optimization
                -s                    # Strip symbols
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${PROJECT_NAME} PRIVATE
                /O2                   # Maximum optimization
                /DNDEBUG             # Disable debug assertions
                /GL                  # Whole program optimization
        )
        target_link_options(${PROJECT_NAME} PRIVATE
                /LTCG                # Link-time code generation
        )
    endif()
endif()

# Find Qt5
find_package(Qt5 REQUIRED COMPONENTS Core Widgets OpenGL)

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
        classes/dataencryption.cpp
        classes/qcustomplot.cpp
        classes/histogramwidget.cpp
        classes/datapathmanager.cpp
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
)

# Create executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../binary)
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Add OpenGL definition for QCustomPlot
target_compile_definitions(${PROJECT_NAME} PRIVATE QCUSTOMPLOT_USE_OPENGL)

# Link Qt5 libraries
target_link_libraries(${PROJECT_NAME}
        Qt5::Core
        Qt5::Widgets
        Qt5::OpenGL
)

# For macOS app bundles
if(APPLE)
    set_target_properties(sleepbook PROPERTIES
            AUTOMOC ON
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_GUI_IDENTIFIER "Sleep Book"
            MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    )
endif()