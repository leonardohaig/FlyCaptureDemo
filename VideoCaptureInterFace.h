//Program:video capture for pointgrey camera and video file
//Date:2019.5.22
//Author:liheng
//Version:V1.0

#ifndef FLYCAPTUREDEMO_VIDEOCAPTUREINTERFACE_H
#define FLYCAPTUREDEMO_VIDEOCAPTUREINTERFACE_H

#include "PtGreyVideoCapture.h"
#include <opencv2/highgui/highgui.hpp>


class CVideoCaptureInterFace:public CPtGreyVideoCapture
{
public:
    CVideoCaptureInterFace();
    virtual ~CVideoCaptureInterFace();

private:
    bool m_bVideoOnLine;
    cv::VideoCapture m_videoCapture;
    cv::VideoWriter m_videoWriter;
    bool m_bSaveOriginVideo;
    cv::Size m_imgSize;

public:
    //open video file or PointGrey camera
    bool InitVideoCapture(const std::string& videoDevice,
                          cv::Rect ROI = cv::Rect(),//just for camera
                          bool bSaveVideo=false,std::string strSavePath="./ADAS_Origin_Video.avi" );

    bool GetCameraVideo(cv::Mat& videoFrame);
};


#endif //FLYCAPTUREDEMO_VIDEOCAPTUREINTERFACE_H
