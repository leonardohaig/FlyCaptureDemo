//
// Created by liheng on 5/22/19.
//

#include "VideoCaptureInterFace.h"

static bool isSerialNum(const std::string& str)
{
    for(int i=0; i<str.size(); ++i)
    {
       int tmp = (int)str[i];
       if( tmp>=48 && tmp<=57)
           continue;
       else
           return false;
    }

    return true;
}

// get the system time
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


CVideoCaptureInterFace::CVideoCaptureInterFace():m_bVideoOnLine(false)
{}

CVideoCaptureInterFace::~CVideoCaptureInterFace()
{

}


//open video file or PointGrey camera
bool CVideoCaptureInterFace::InitVideoCapture(const std::string& videoDevice,cv::Rect ROI,
                      bool bSaveVideo,std::string strSavePath)
{
    m_bVideoOnLine = isSerialNum(videoDevice);

    try
    {
        //open PointGrey Camera
        if( m_bVideoOnLine )
        {
            if( !InitPtGreyCamera(std::atoi(videoDevice.data()),ROI) )
                return false;

            std::this_thread::sleep_for(std::chrono::microseconds(500));//sleep 500ms

            cv::Mat frame;
            if( !GetCameraVideo(frame) )
                return false;

            m_imgSize.width = frame.cols;
            m_imgSize.height = frame.rows;
        }
        else
        {
            if( !m_videoCapture.open(videoDevice) )
            {
                std::cerr<<"Open the video file "<< videoDevice <<" Failed!"<<std::endl;
                return false;
            }

#if (CV_MAJOR_VERSION == 2 )
            m_imgSize.width = cvRound( m_videoCapture.get(CV_CAP_PROP_FRAME_WIDTH) );
            m_imgSize.height = cvRound( m_videoCapture.get(CV_CAP_PROP_FRAME_HEIGHT) );
#else
            m_imgSize.width = cvRound( m_videoCapture.get(cv::CAP_PROP_FRAME_WIDTH) );
            m_imgSize.height = cvRound( m_videoCapture.get(cv::CAP_PROP_FRAME_HEIGHT) );
#endif

        }

        //set the video save format
        if( bSaveVideo )
        {

            std::string strNewName = GetNewName(strSavePath);
            if( !strNewName.empty() )
            {
                m_bSaveOriginVideo = bSaveVideo;

                int fourcc;
#if (CV_MAJOR_VERSION == 2 || CV_MAJOR_VERSION==3 )
                fourcc = CV_FOURCC('M', 'J', 'P', 'G');
#elif CV_MAJOR_VERSION == 4
                fourcc = cv::CAP_OPENCV_MJPEG;
#endif
                if( !m_videoWriter.open(strNewName, fourcc, 15.0, m_imgSize) )
                    return false;
            }
            else
            {
                std::cerr<<"Error occured when set the video save format!"<<std::endl;
                return false;
            }
        }

    }
    catch(cv::Exception& exception)
    {
        std::cerr<<"Error occured when init the camera,captured error:"<<exception.what()<<std::endl;
        return false;
    }
    catch (...)
    {
        std::cerr<<"Unknown error occured when init the camera."<<std::endl;
        return false;
    }

    return true;
}

bool CVideoCaptureInterFace::GetCameraVideo(cv::Mat& videoFrame)
{
    try
    {
        if( m_bVideoOnLine )
        {
            if(!CPtGreyVideoCapture::GetCameraVideo(videoFrame))
            {
                std::cerr<<"Capture the Camera video failed."<<std::endl;
                return false;
            }
        }
        else
        {
            m_videoCapture >> videoFrame;
            if( videoFrame.empty() )
            {
                std::cerr<<"Capture the video file failed."<<std::endl;
                return false;
            }
        }

        //save video
        if( m_bSaveOriginVideo )
            m_videoWriter << videoFrame;

    }
    catch(cv::Exception& exception)
    {
        std::cerr<<"Error occured when capture the image: "<<exception.what()<<std::endl;
        return false;
    }
    catch(...)
    {
        std::cerr<<"Error occured when capture the image."<<std::endl;
        return false;
    }

    return true;
}
