//
// Created by zxkj on 18-12-17.
//
//======================================//
//Update Content:添加图像畸变矫正功能，实现在视频输出前进行畸变矫正，该功能可属于可选项
//Data:2019.4.17
//Author:liheng
//Version:V2.0
//------------------------------------//
//Update Content: 添加在线视频保存功能，实现对原始视频的保存
//Data:2019.3.13
//Author:liheng
//Version:V1.2
//------------------------------------//
//文件内容：获取视频帧类
//说明：可以读取离线视频或者在线PointGrey相机视频
//版本：v1.1
//更新日期：2018-1023
//更新内容：1.增加设置相机读取的帧尺寸功能
//			2.增加返回视频帧率函数
//			3.取消函数中的software trigger输出提示
//			4.修改读取离线视频退出CGetVideo时，成员的析构错误
//======================================//


#include "SingleCameraCaptureEx.h"
#include <opencv2/opencv.hpp>
#include <memory>
#include <thread>
#include <mutex>

class CGetVideo
{
public:
    //输入参数：相机编号
    //offsetX,offsetY图像截取起点坐标；
    //width,height图像大小
    //对1024X768图像：448,216；1024,768
    //对640X480图像：640,360,640,480
    //strSavedPath:原始视频保存路径，如果不为空，将保存原始视频
    CGetVideo(unsigned int t_in_camSerialNum,
            bool bUseThread = false,
            int offsetX = -1, int offsetY =-1,
            int width = -1,int height = -1,const std::string& strSavedPath = std::string());

    //输入参数：视频文件路径
    CGetVideo(const std::string& videoPath);

    virtual ~CGetVideo();

private:
    std::shared_ptr<cv::VideoWriter> m_pVideoWriter;
    //区分视频读取方式
    //true----实时读取相机视频；false----读取离线视频
    bool m_bGetOnline;
    bool m_bUseThread;


private:
    SingleCameraCaptureEx m_singleCam;
    bool GetVideoFrameOnline(cv::Mat& videoFrame);

private:
    cv::VideoCapture m_videoReader;
    bool m_bFileRight;//读取视频文件是否成功
    bool GetVideoFrameByFile(cv::Mat& videoFrame);

private:
    cv::Mat m_rectifyMap1;
    cv::Mat m_rectifyMap2;

    bool m_bQuitCaptureThread;
    std::thread m_captureThread;
    std::mutex m_videoFrameMutex;

    cv::Mat m_currentFrame;

    void CaptureThread();

public:

    //初始化。初始化内参矩阵、畸变矩阵进行畸变矫正(该函数可选)
    bool InitVideoCapture(const std::string& camerParamPath);

    bool GetVideoFrame(cv::Mat& videoFrame);

    void StartCaptureThread();
    void StopCaptureThread();

    double GetFrameRate();

};

//#endif //IAU_ROS_CGETVIDEO_H
