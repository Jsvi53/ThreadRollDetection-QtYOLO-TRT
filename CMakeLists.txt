cmake_minimum_required (VERSION 3.28)

# compile kit
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

# set(CMAKE_C_COMPILER gcc)
# set(CMAKE_CXX_COMPILER g++)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

if(POLICY CMP0146)
  cmake_policy(SET CMP0146 NEW)
endif()

project("THREAD_YOLO_RT_VSCODE" LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 定义目标路径
set(COMPILE_COMMANDS_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# 添加自定义命令和目标以复制 compile_commands.json 文件
add_custom_command(
    OUTPUT "${COMPILE_COMMANDS_OUTPUT_DIR}/compile_commands.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/compile_commands.json"
        "${COMPILE_COMMANDS_OUTPUT_DIR}/compile_commands.json"
    DEPENDS "${CMAKE_BINARY_DIR}/compile_commands.json"
    COMMENT "Copying compile_commands.json to output directory"
)
add_custom_target(copy_compile_commands ALL DEPENDS "${COMPILE_COMMANDS_OUTPUT_DIR}/compile_commands.json")


set(TS_FILES YOLO_zh_CN.ts)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets LinguistTools OpenGLWidgets Qml Concurrent)  # 添加组件时，名字去掉关于Qt的
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets LinguistTools OpenGLWidgets Qml Concurrent)
if(Qt6_FOUND)
    message(STATUS "Qt6 library is found!")
else()
    message(FATAL_ERROR "Cannot find Qt6 library!")
endif()

# 检查是否使用 MSVC 作为编译器
if (MSVC)
    # 如果是 MSVC，设置 OpenCV_DIR 为 MSVC 版本
    set(OpenCV_DIR "D:/program/opencv/build/x64/vc16/lib")
    find_package(OpenCV 4.10 REQUIRED)
else()
    # 如果不是 MSVC，设置 OpenCV_DIR 为 GCC 版本
    set(OpenCV_DIR "D:/program/Opencv411")
    find_package(OpenCV 4.1.1 REQUIRED)
endif()

if(OpenCV_FOUND)
    message(STATUS  "OpenCV library found at ${OpenCV_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "Cannot find OpenCV in the specified directory.")
endif()


find_package(Eigen3 REQUIRED)
if(Eigen3_FOUND)
    message(STATUS "Eigen3 library is found!")
else()
    message(FATAL_ERROR "Cannot find Eigen3 library!")
endif()

# 设置CUDA
set(CUDA_HOST_COMPILER ${CMAKE_CXX_COMPILER})
set(CUDA_TOOLKIT_ROOT_DIR "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v10.2")
find_package(CUDAToolkit REQUIRED)
if(CUDAToolkit_FOUND)
    message(STATUS "CUDA library is found!")
else()
    message(FATAL_ERROR "CUDA library not found!")
endif()

# 设置CUDA NVCC编译器标志，指定优化级别和计算能力
set(
        CUDA_NVCC_FLAGS
        ${CUDA_NVCC_FLAGS};
        -O3 # 优化级别
        -gencode arch=compute_50,code=sm_50 # 指定GPU架构和计算能力，这里是针对sm_61
)

# 设置TensorRT的根目录并查找TensorRT头文件和库
set(TENSORRT_ROOT "D:/program/TensorRT-8.5.1.7")
find_path(TENSORRT_INCLUDE_DIR NvInfer.h HINTS ${TENSORRT_ROOT} ${CUDA_TOOLKIT_ROOT_DIR} PATH_SUFFIXES include)
MESSAGE(STATUS "Found TensorRT headers at ${TENSORRT_INCLUDE_DIR}")
find_library(TENSORRT_LIBRARY_INFER nvinfer HINTS ${TENSORRT_ROOT} ${TENSORRT_BUILD} ${CUDA_TOOLKIT_ROOT_DIR} PATH_SUFFIXES lib lib64 lib/x64)
find_library(TENSORRT_LIBRARY_INFER_PLUGIN nvinfer_plugin HINTS ${TENSORRT_ROOT} ${TENSORRT_BUILD} ${CUDA_TOOLKIT_ROOT_DIR} PATH_SUFFIXES lib lib64 lib/x64)
find_library(TENSORRT_LIBRARY_NVONNXPARSER nvonnxparser HINTS ${TENSORRT_ROOT} ${TENSORRT_BUILD} ${CUDA_TOOLKIT_ROOT_DIR} PATH_SUFFIXES lib lib64 lib/x64)
find_library(TENSORRT_LIBRARY_NVPARSERS nvparsers HINTS ${TENSORRT_ROOT} ${TENSORRT_BUILD} ${CUDA_TOOLKIT_ROOT_DIR} PATH_SUFFIXES lib lib64 lib/x64)
set(TENSORRT_LIBRARY ${TENSORRT_LIBRARY_INFER} ${TENSORRT_LIBRARY_INFER_PLUGIN} ${TENSORRT_LIBRARY_NVONNXPARSER} ${TENSORRT_LIBRARY_NVPARSERS})
MESSAGE(STATUS "Find TensorRT libs at ${TENSORRT_LIBRARY}")
# 处理标准库查找结果
find_package_handle_standard_args(TENSORRT DEFAULT_MSG TENSORRT_INCLUDE_DIR TENSORRT_LIBRARY)
# 如果没有找到TensorRT库，输出错误信息
if(NOT TENSORRT_FOUND)
    message(ERROR "Cannot find TensorRT library.")
endif()

aux_source_directory("src" SRC_LIST)
# file(GLOB_RECURSE QT_SOURCES "include/*.h" "src/*.cpp" "src/*.h" "src/*.ui")    # GLOB_RECURSE 递归查找所有源文件

set(TS_FILES YOLO_zh_CN.ts)
set(QT_SOURCES "include/yolowindow.h" "src/yolowindow.ui" "include/qswitchbutton.h")
list(APPEND SRC_LIST ${QT_SOURCES} ${TS_FILES})
qt_add_executable(THREAD_YOLO_RT_VSCODE MANUAL_FINALIZATION WIN32 main.cpp main.hpp)
qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
target_sources(THREAD_YOLO_RT_VSCODE PRIVATE ${SRC_LIST})
target_include_directories(THREAD_YOLO_RT_VSCODE PRIVATE ${OpenCV_INCLUDE_DIRS} ${TENSORRT_INCLUDE_DIR} ${CUDAToolkit_INCLUDE_DIRS} ${EIGEN3_INCLUDE_DIR} "include")
set(QT_LIBRARIES Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::OpenGLWidgets Qt${QT_VERSION_MAJOR}::Qml Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Concurrent Qt${QT_VERSION_MAJOR}::Network)   #模块需要单独找包
target_link_libraries(THREAD_YOLO_RT_VSCODE PRIVATE ${OpenCV_LIBS} ${CUDA_cublas_LIBRARY} ${CUDA_cudart_static_LIBRARY} ${TENSORRT_LIBRARY} ${QT_LIBRARIES})


if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET THREAD_YOLO_RT_VSCODE PROPERTY CXX_STANDARD 20)
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(THREAD_YOLO_RT_VSCODE)
endif()

# TODO: 如有需要，请添加测试并安装目标。
# target_link_directories(THREAD_YOLO_RT_VSCODE PRIVATE ${CUDAToolkit_LIBRARY_DIR})
# set_target_properties(THREAD_YOLO_RT_VSCODE PROPERTIES WIN32_EXECUTABLE TRUE)