cmake_minimum_required(VERSION 2.8)
project(FlyCaptureDemo)

set(CMAKE_CXX_STANDARD 11)


set(OpenCV_DIR /usr/local/opencv-2.4.13.6/share/OpenCV)
find_package( OpenCV REQUIRED )

#set(FlyCapture_INCLUDE_DIRS ./flycapture ./Flycapture2)
#set(FlyCapture_LIBS ${PROJECT_SOURCE_DIR}/lib/libflycapture.so
#        ${PROJECT_SOURCE_DIR}/lib/libflycapture.so.2
#        ${PROJECT_SOURCE_DIR}/lib/libflycapture.so.2.13.3.31)

set(FlyCapture_INCLUDE_DIRS /usr/include/flycapture)
set(FlyCapture_LIBS /usr/lib/libflycapture.so)

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${FlyCapture_INCLUDE_DIRS})

add_executable(FlyCaptureDemo main.cpp SingleCameraCaptureEx.cpp CGetVideo.cpp PtGreyVideoCapture.cpp VideoCaptureInterFace.cpp)

target_link_libraries(FlyCaptureDemo ${OpenCV_LIBS})
target_link_libraries(FlyCaptureDemo ${FlyCapture_LIBS})
