//Program:video capture for pointgrey camera
//Date:2019.5.22
//Author:liheng
//Version:V1.0

#ifndef FLYCAPTUREDEMO_PTGREYVIDEOCAPTURE_H
#define FLYCAPTUREDEMO_PTGREYVIDEOCAPTURE_H

#include "FlyCapture2.h"
#include <iostream>
#include <sstream>
#include <memory>
#include <chrono>
#include <thread>
#include <mutex>
#include <opencv2/core/core.hpp>

//
// Software trigger the camera instead of using an external hardware trigger
//
#define SOFTWARE_TRIGGER_CAMERA

class CPtGreyVideoCapture
{
public:
    CPtGreyVideoCapture();
    virtual ~CPtGreyVideoCapture();

public:
    void PrintBuildInfo();
    void PrintCameraInfo(FlyCapture2::CameraInfo *pCamInfo);
    void PrintError(FlyCapture2::Error error);
    bool CheckSoftwareTriggerPresence(std::shared_ptr<FlyCapture2::Camera> pCam);
    bool PollForTriggerReady(std::shared_ptr<FlyCapture2::Camera> pCam);
    bool FireSoftwareTrigger(std::shared_ptr<FlyCapture2::Camera> pCam);

private:
    std::shared_ptr<FlyCapture2::Camera> m_pCamera;
    bool m_bQuitVideoThread;
    bool m_bUseThread;
    std::thread m_captureThread;
    std::mutex m_videoFrameMutex;
    cv::Mat m_currentFrame;

    //camera info for init camera
    unsigned int m_serialNumber;
    int m_imageOffsetX;
    int m_imageOffsetY;
    int m_imageWidth;
    int m_imageHeight;

public:
    bool InitPtGreyCamera(unsigned int serialNumber,int offsetX = -1, int offsetY = -1,
                          int width = -1, int height = -1);

    bool InitPtGreyCamera(unsigned int serialNumber,cv::Rect ROI=cv::Rect() );

    //stop capture and disconnect the camera
    bool StopPtGreyCamera();

    bool GetCameraVideo(cv::Mat& videoFrame);

    //==============capture image from capturethread method================//
private:
    void CaptureThread();
    bool ThreadGetCameraVideo(cv::Mat& videoFrame);
public:
    void StartCaptureThread();
    //stop capture thread,but the camera is still connected
    void StopCaptureThread();
};


#endif //FLYCAPTUREDEMO_PTGREYVIDEOCAPTURE_H
