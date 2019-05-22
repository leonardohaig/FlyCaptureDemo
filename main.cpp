#include <iostream>
#include <opencv2/opencv.hpp>
#include "CGetVideo.h"
#include <chrono>
#include "PtGreyVideoCapture.h"
#include "VideoCaptureInterFace.h"

int main()
{
    std::cout<<"start test flycapture demo."<<std::endl;

    //CPtGreyVideoCapture ptGreyVideoCapture;
    CVideoCaptureInterFace ptGreyVideoCapture;
    if( !ptGreyVideoCapture.InitVideoCapture("18072427",cv::Rect(640,360,640,480),true))
        return -1;

    //if( !ptGreyVideoCapture.InitPtGreyCamera(18072427,640,360,640,480) )
    //    return -1;

    ptGreyVideoCapture.StartCaptureThread();

    long long int nFrameID = 1;
    int nWaitTime = 1;
    while (true)
    {

        cv::Mat src;
        src.release();

        auto start = std::chrono::steady_clock::now();

        if( !ptGreyVideoCapture.GetCameraVideo(src) )
            continue;





        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();//us
        float fps = (1e6*1.0/duration);


        if( !src.empty() )
        {
            char info[256];

            sprintf(info,"Frame:%lld FPs:%.2f",nFrameID,fps);
            cv::putText(src,info,cv::Point(0,20),CV_FONT_HERSHEY_SIMPLEX,0.8,cv::Scalar(0,255,0),2);

            cv::imshow("src",src);
            ++nFrameID;


            char chKey = cv::waitKey(nWaitTime);
            if (chKey == 27) break;
            if (chKey == ' ') nWaitTime = !nWaitTime;
        }
    }
//
//    std::shared_ptr<CGetVideo> m_pGetVideo;
//Loop1:
//    m_pGetVideo = std::make_shared<CGetVideo>(18072427, false,640,360,640,480); // 网口相机编号:17369069  12.5：18072414
//    //m_pGetVideo->StopCaptureThread();
//    //m_pGetVideo->StartCaptureThread();
//    //CGetVideo m_getVideo(18072427,true,640,360,640,480); // 网口相机编号:17369069  12.5：18072414
//    //std::cout<<"Frame rate="<<m_pGetVideo->GetFrameRate()<<std::endl;
//
//    long long int nFrameID = 1;
//    int nWaitTime = 1;
//    while (true)
//    {
//
//        cv::Mat src;
//        src.release();
//
//        auto start = std::chrono::steady_clock::now();
//
//        if( !m_pGetVideo->GetVideoFrame(src) )
//        {
//            //m_pGetVideo->StopCaptureThread();
//            goto Loop1;
//        }
//
//
//        auto end = std::chrono::steady_clock::now();
//        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();//us
//        float fps = (1e6*1.0/duration);
//
//
//        if( !src.empty() )
//        {
//            char info[256];
//
//            sprintf(info,"Frame:%lld FPs:%.2f",nFrameID,fps);
//            cv::putText(src,info,cv::Point(0,20),CV_FONT_HERSHEY_SIMPLEX,0.8,cv::Scalar(0,255,0),2);
//
//            cv::imshow("src",src);
//            ++nFrameID;
//
//
//            char chKey = cv::waitKey(nWaitTime);
//            if (chKey == 27) break;
//            if (chKey == ' ') nWaitTime = !nWaitTime;
//        }
//
//
//
//
//
//    }
//


    return 0;
}