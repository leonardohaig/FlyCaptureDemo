#include "CGetVideo.h"


// 获取系统当前时间
static std::string GetCurrentSystemTime()
{
    auto tt = std::chrono::system_clock::to_time_t
            (std::chrono::system_clock::now());
    struct tm* ptm = localtime(&tt);
    char date[60] = { 0 };
    sprintf(date, "%d%02d%02d-%02d%02d%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}

static std::string GetNewName(const std::string strName )
{
    std::string strNewName("");
    if(strName.empty() )
    {
        std::cerr<<"the input name is empty !"<<std::endl;
    }
    else
    {
        std::size_t nPos = strName.rfind("."); //找到分隔符 . 位置

        if (nPos !=std::string::npos )
        {
            std::string strPrefix = strName.substr(0,nPos);
            std::string strPostfix = strName.substr(nPos+1);

            strNewName = strPrefix + "-" + GetCurrentSystemTime() + "." + strPostfix;
        }
        else
        {
            std::cerr<<"Please input the file's format and save path!"<<std::endl;
        }

    }

    return strNewName;

}



CGetVideo::CGetVideo(unsigned int t_in_camSerialNum, bool bUseThread,
        int offsetX, int offsetY,int width, int height,const std::string& strSavedPath)
:m_bGetOnline(true)
,m_bUseThread(bUseThread)
{
    m_singleCam.initial_Cam(t_in_camSerialNum);

    if ( offsetX==-1 && offsetY == -1 && width == -1 && height == -1 )
        m_singleCam.set_camera();
    else
        m_singleCam.set_camera(offsetX,offsetY,width,height);

    if( !strSavedPath.empty() )
    {
        cv::Mat videoFrame;
        GetVideoFrameOnline(videoFrame);

        std::string strNewName = GetNewName(strSavedPath);
        m_pVideoWriter = std::make_shared<cv::VideoWriter>();
        m_pVideoWriter->open(strNewName,CV_FOURCC('D', 'I', 'V', 'X'),15,videoFrame.size());

        if( !m_pVideoWriter->isOpened() )
        {
            std::cerr<<"Write the origin video failed !"<<std::endl;
        }
    }

    m_rectifyMap1.release();
    m_rectifyMap2.release();

    m_bQuitCaptureThread = false;
}

CGetVideo::CGetVideo(const std::string& videoPath)
:m_bGetOnline(false)
,m_bFileRight(false)
{
    if (!m_videoReader.open(videoPath))
    {
        cerr << "error in opening video_1!" << endl;
        m_bFileRight = false;
        //return -1;
    }
    else
    {
        if (m_videoReader.isOpened())
        {
#if CV_MAJOR_VERSION ==  2
            cout << "视频文件一共" << m_videoReader.get(CV_CAP_PROP_FRAME_COUNT) << "帧." << endl;
#else
            cout << "视频文件一共" << m_videoReader.get(cv::CAP_PROP_FRAME_COUNT) << "帧." << endl;
#endif
            m_bFileRight = true;
        }
    }

    m_rectifyMap1.release();
    m_rectifyMap2.release();

    m_bQuitCaptureThread = false;

}

CGetVideo::~CGetVideo()
{
    //StopCaptureThread();
}

bool CGetVideo::GetVideoFrameOnline(cv::Mat& videoFrame)
{
    if (m_singleCam.m_num_cam<1)
    {
        cerr << "Insufficient number of cameras" << endl;
        return false;
    }
    //cout << "check trigger" << endl;
    // Check that the trigger is ready
    m_singleCam.PollForTriggerReady(m_singleCam.m_pCameras);

    //cout << "Press the Enter key to initiate a software trigger" << endl;
    //cin.ignore();

    // Fire software trigger
    bool retVal = m_singleCam.FireSoftwareTrigger(m_singleCam.m_pCameras);
    if (!retVal)
    {
        cout << endl;
        cerr << "Error firing software trigger" << endl;
        return false;
    }

    // Grab image
    fl2::Image image;
    fl2::Error error = m_singleCam.m_pCameras->RetrieveBuffer(&image);
    image.GetTimeStamp();
    if (error != fl2::PGRERROR_OK)
    {
        m_singleCam.PrintError(error);
        cv::waitKey(100);
        return false;
        //return -1;
    }
    //cout << "retrievebuffer complete" << endl;

    fl2::Image t_im;
    image.Convert(fl2::PixelFormat::PIXEL_FORMAT_BGR, &t_im);
    cv::Mat show_img(t_im.GetRows(), t_im.GetCols(), CV_8UC3, t_im.GetData());

    videoFrame = show_img.clone();

    return true;
}

bool CGetVideo::GetVideoFrameByFile(cv::Mat& videoFrame)
{
    if (!m_bFileRight)
    {
        cerr << "构造函数中读取视频路径错误." << endl;
        return false;
    }

    m_videoReader >> videoFrame;

    //if (!m_videoReader.read(videoFrame))
    //{
    //	cerr << "error in reading video_1!" << endl;
    //	return false;
    //}

    return true;
}

bool CGetVideo::InitVideoCapture(const std::string& camerParamPath)
{
    int nWidth = 1280;
    int nHeight = 720;

    float fx = .0;
    float fy = .0;
    float cu = .0;
    float cv = .0;

    float k1 = .0;
    float k2 = .0;
    float k3 = .0;
    float p1 = .0;
    float p2 = .0;


    try
    {
        cv::FileStorage fileStorage;
        fileStorage.open(camerParamPath,cv::FileStorage::READ);
        if( !fileStorage.isOpened() )
        {
            std::cerr<<"Open "<< camerParamPath << "Failed !"<<std::endl;
            return false;
        }

        fileStorage["intrinsics"]["imgWidth"] >> nWidth;
        fileStorage["intrinsics"]["imgHeight"] >> nHeight;

        fileStorage["intrinsics"]["fu"] >> fx;
        fileStorage["intrinsics"]["fv"] >> fy;
        fileStorage["intrinsics"]["cu"] >> cu;
        fileStorage["intrinsics"]["cv"] >> cv;

        fileStorage["intrinsics"]["k1"] >> k1;
        fileStorage["intrinsics"]["k2"] >> k2;
        fileStorage["intrinsics"]["k3"] >> k3;
        fileStorage["intrinsics"]["p1"] >> p1;
        fileStorage["intrinsics"]["p2"] >> p2;

        fileStorage.release();

        //构造内参矩阵、畸变矩阵
        cv::Mat cameraMatrix(3,3,CV_32FC1,cv::Scalar(0));
        cv::Mat distCoeffs(1,5,CV_32FC1,cv::Scalar(0));

        cameraMatrix.at<float>(0,0) = fx;
        cameraMatrix.at<float>(0,2) = cu;
        cameraMatrix.at<float>(1,1) = fy;
        cameraMatrix.at<float>(1,2) = cv;
        cameraMatrix.at<float>(2,2) = 1.0f;

        distCoeffs.at<float>(0) = k1;
        distCoeffs.at<float>(1) = k2;
        distCoeffs.at<float>(2) = p1;
        distCoeffs.at<float>(3) = p2;
        distCoeffs.at<float>(4) = k3;

        cv::Mat videoFrame(nHeight,nWidth,CV_8UC3);
        //if( !GetVideoFrame(videoFrame) )
        //{
        //    std::cerr<<"Read the video failed in InitVideoCapture(...)"<<std::endl;
        //    return false;
        //}

        cv::Mat newCameraMatrix = cv::getOptimalNewCameraMatrix(cameraMatrix,distCoeffs,videoFrame.size(),1,videoFrame.size(),0);
        newCameraMatrix = cameraMatrix.clone();
        cv::initUndistortRectifyMap(cameraMatrix,distCoeffs,cv::Mat(),
                newCameraMatrix,
                videoFrame.size(),CV_16SC2,m_rectifyMap1,m_rectifyMap2);
        //cv::remap(videoFrame,videoFrame,m_rectifyMap1,m_rectifyMap2,cv::INTER_LINEAR);

    }
    catch(cv::Exception& e)
    {
        std::cerr<<"Open "<< camerParamPath << " catched error: "<<e.what()<<std::endl;
        return false;
    }
    catch(...)
    {
        std::cerr<<"Open "<< camerParamPath << " catched unknown error!"<<std::endl;
        return false;
    }


    return true;
}


bool CGetVideo::GetVideoFrame(cv::Mat& videoFrame)
{
    bool bCheck = false;
    if (m_bGetOnline)
    {
        if( m_bUseThread )
        {
            std::lock_guard<std::mutex> Lock(m_videoFrameMutex);//选用lock_guard,考虑其时间成本比较低
            videoFrame = m_currentFrame.clone();
            bCheck = !m_currentFrame.empty();
        }
        else
        {
            bCheck = GetVideoFrameOnline(videoFrame);
        }

        //进行翻转操作
        //cv::flip(videoFrame,videoFrame,0);
        //cv::flip(videoFrame,videoFrame,1);
    }
    else
    {
        bCheck = GetVideoFrameByFile(videoFrame);
    }

    if( m_pVideoWriter && bCheck )
    {
        m_pVideoWriter->write(videoFrame);
    }

    if( !m_rectifyMap1.empty() && !m_rectifyMap2.empty() )
        cv::remap(videoFrame,videoFrame,m_rectifyMap1,m_rectifyMap2,cv::INTER_LINEAR,cv::BORDER_TRANSPARENT);

    return bCheck;
}

void CGetVideo::StartCaptureThread()
{
    if( m_bUseThread && m_bGetOnline )
    {
        m_bQuitCaptureThread = false;

        std::this_thread::sleep_for(std::chrono::microseconds(2*1000));
        m_captureThread = std::thread(&CGetVideo::CaptureThread,this);
        //std::this_thread::sleep_for(std::chrono::microseconds(2*1000));
    }
}

void CGetVideo::StopCaptureThread()
{
    if (m_bGetOnline && m_bUseThread )
    {
        m_bQuitCaptureThread = true;
        if( m_captureThread.joinable() )
            m_captureThread.join();

        std::this_thread::sleep_for(std::chrono::microseconds(2*1000));
    }

}

void CGetVideo::CaptureThread()
{
    std::cout<<"Start Video capture thread.m_bQuitCaptureThread="<<m_bQuitCaptureThread<<std::endl;

    cv::Mat videoFrame;
    while ( !m_bQuitCaptureThread )
    {
        videoFrame.release();

        if( GetVideoFrameOnline(videoFrame) )
        {
            std::lock_guard<std::mutex> Lock(m_videoFrameMutex);//选用lock_guard,考虑其时间成本比较低

            //m_currentFrame = videoFrame.clone();
            videoFrame.copyTo(m_currentFrame);
            std::cout<<"m_currentFrame="<<m_currentFrame.size()<<std::endl;
        }
    }

    std::cout<<"Quit Video capture thread."<<std::endl;
}

double CGetVideo::GetFrameRate()
{
    if (m_bGetOnline)
        return m_singleCam.GetFrameRate();
    else
#if CV_MAJOR_VERSION == 2
        return m_videoReader.get(CV_CAP_PROP_FPS);
#else
        return m_videoReader.get(cv::CAP_PROP_FPS);
#endif


}
