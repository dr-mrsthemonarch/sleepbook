# SleepBook 📊

A comprehensive sleep and symptom tracking application built with Qt5 and C++20.

## Overview

SleepBook is a desktop application designed to help users track their sleep patterns and associated symptoms over time. The application provides secure user management, data visualization through histograms, and encrypted data storage to protect user privacy.

## Features

### 🔐 User Management
- Secure login system with encrypted user authentication
- Multi-user support with individual data isolation
- Data encryption to protect sensitive health information

### 📈 Sleep & Symptom Tracking
- Track various symptoms and their severity levels
- Record sleep-related data and patterns
- Custom symptom widgets for easy data entry

### 📊 Data Visualization
- Interactive histograms powered by QCustomPlot
- Visual representation of sleep patterns and symptom trends
- OpenGL-accelerated rendering for smooth performance

### 💾 Data Management
- Encrypted local data storage
- Secure data path management
- User-specific data organization

### Data Security
The application implements multiple layers of security:
- User authentication with encrypted credentials
- Application data encryption at rest
- Secure data path management
- User-specific data isolation
- Serialized data protection

### Symptom Tracking System
The application supports flexible symptom tracking with:
- **Dynamic symptom creation**: Users can define custom symptoms
- **Type-safe data entry**: Different input methods for different data types
- **Automatic column naming**: Database-safe naming conventions
- **Serialization support**: Persistent storage of symptom definitions

## Usage
### Getting Started
1. **First Launch**: Create a new user account through the secure login dialog
2. **Setup Symptoms**: Define the symptoms and metrics you want to track
3. **Daily Tracking**: Use the symptom widgets to record daily data
4. **View Trends**: Access histogram visualizations to analyze patterns over time

### Symptom Types
- **Binary Symptoms**: Simple yes/no tracking (e.g., "Headache present")
- **Count Symptoms**: Numerical tracking (e.g., "Number of wake-ups")
- **Quantity Symptoms**: Measured values (e.g., "Hours of sleep", "Pain level 1-10")


## Technical Specifications

- **Language**: C++20
- **GUI Framework**: Qt5 (Core, Widgets, OpenGL)
- **Graphics**: QCustomPlot with OpenGL acceleration
- **Build System**: CMake 3.16+
- **Platforms**: Windows, macOS, Linux

## Requirements

- Qt5 development libraries (Core, Widgets, OpenGL)
- CMake 3.16 or higher
- C++20 compatible compiler (GCC, Clang, or MSVC)
- OpenGL support for enhanced graphics performance

# Building sleepbook
## Prerequisites
### Common Requirements
- **CMake** 3.16 or higher
- **C++ Compiler** with C++20 support
- **Qt5** with Core, Widgets, and OpenGL components

### macOS Requirements
- **Xcode Command Line Tools** or **Xcode**
- **Clang** compiler (comes with Xcode)
- **Qt5** installed via:
    - Homebrew: `brew install qt@5`
    - Qt installer from Qt website

# Create build directory
```mkdir build && cd build```

# Configure the project
```cmake .. -DCMAKE_BUILD_TYPE=Release```

# Build the application
```cmake --build . --config Release```

# The executable will be in the binary/ directory



### Windows Requirements
- **Visual Studio 2019/2022** with C++ development tools, or
- **MinGW-w64** with GCC
- **Qt5** installed via:
    - Qt installer from Qt website
    - vcpkg: `vcpkg install qt5-base qt5-widgets qt5-opengl`

# Create build directory
```mkdir build && cd build```

# Configure for Visual Studio
```cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Release```

# Build the application
```cmake --build . --config Release```

The executable will be in the binary/ directory