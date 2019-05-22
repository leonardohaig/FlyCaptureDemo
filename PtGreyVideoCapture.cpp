//
// Created by liheng on 5/22/19.
//

#include "PtGreyVideoCapture.h"

CPtGreyVideoCapture::CPtGreyVideoCapture()
:m_bQuitVideoThread(false)
,m_bUseThread(false)
,m_serialNumber(0)
,m_imageOffsetX(-1)
,m_imageOffsetY(-1)
,m_imageWidth(-1)
,m_imageHeight(-1)
{
    ;
}

CPtGreyVideoCapture::~CPtGreyVideoCapture()
{
    StopCaptureThread();
    StopPtGreyCamera();
}


void PrintBuildInfo()
{
    FlyCapture2::FC2Version fc2Version;
    FlyCapture2::Utilities::GetLibraryVersion(&fc2Version);

    std::ostringstream version;
    version << "FlyCapture2 library version: " << fc2Version.major << "."
            << fc2Version.minor << "." << fc2Version.type << "."
            << fc2Version.build;
    std::cout << version.str() << std::endl;

    std::ostringstream timeStamp;
    timeStamp << "Application build date: " << __DATE__ << " " << __TIME__;
    std::cout << timeStamp.str() << std::endl << std::endl;
}

void CPtGreyVideoCapture::PrintCameraInfo(FlyCapture2::CameraInfo *pCamInfo)
{
    std::cout << std::endl;
    std::cout << "*** CAMERA INFORMATION ***" << std::endl;
    std::cout << "Serial number - " << pCamInfo->serialNumber << std::endl;
    std::cout << "Camera model - " << pCamInfo->modelName << std::endl;
    std::cout << "Camera vendor - " << pCamInfo->vendorName << std::endl;
    std::cout << "Sensor - " << pCamInfo->sensorInfo << std::endl;
    std::cout << "Resolution - " << pCamInfo->sensorResolution << std::endl;
    std::cout << "Firmware version - " << pCamInfo->firmwareVersion << std::endl;
    std::cout << "Firmware build time - " << pCamInfo->firmwareBuildTime << std::endl
         << std::endl;
}

void CPtGreyVideoCapture::PrintError(FlyCapture2::Error error)
{ error.PrintErrorTrace(); }

bool CPtGreyVideoCapture::CheckSoftwareTriggerPresence(std::shared_ptr<FlyCapture2::Camera> pCam)
{
    const unsigned int k_triggerInq = 0x530;

    FlyCapture2::Error error;
    unsigned int regVal = 0;

    error = pCam->ReadRegister(k_triggerInq, &regVal);

    if (error != FlyCapture2::PGRERROR_OK)
    {
        PrintError(error);
        return false;
    }

    if ((regVal & 0x10000) != 0x10000)
    {
        return false;
    }

    return true;
}

bool CPtGreyVideoCapture::PollForTriggerReady(std::shared_ptr<FlyCapture2::Camera> pCam)
{
    const unsigned int k_softwareTrigger = 0x62C;
    FlyCapture2::Error error;
    unsigned int regVal = 0;

    do
    {
        error = pCam->ReadRegister(k_softwareTrigger, &regVal);
        if (error != FlyCapture2::PGRERROR_OK)
        {
            PrintError(error);
            return false;
        }

    } while ((regVal >> 31) != 0);

    return true;
}

bool CPtGreyVideoCapture::FireSoftwareTrigger(std::shared_ptr<FlyCapture2::Camera> pCam)
{
    const unsigned int k_softwareTrigger = 0x62C;
    const unsigned int k_fireVal = 0x80000000;
    FlyCapture2::Error error;

    error = pCam->WriteRegister(k_softwareTrigger, k_fireVal);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        PrintError(error);
        return false;
    }

    return true;
}

bool CPtGreyVideoCapture::InitPtGreyCamera(unsigned int serialNumber,
        int offsetX, int offsetY,int width, int height)
{
    m_serialNumber = serialNumber;
    m_imageOffsetX = offsetX;
    m_imageOffsetY = offsetY;
    m_imageWidth = width;
    m_imageHeight = height;


    FlyCapture2::Error error;

    //
    // Initialize BusManager and retrieve number of cameras detected
    //
    FlyCapture2::BusManager busMgr;
    unsigned int numCameras = 10;

    // Check to make sure GigE cameras are connected/discovered
    FlyCapture2::CameraInfo camInfo[10];
    error = busMgr.DiscoverGigECameras(camInfo, &numCameras);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        PrintError(error);
        return false;
    }

    std::cout << "Number of GigE cameras discovered: " << numCameras << std::endl;
    if( numCameras>=1 )
    {
        error = busMgr.ForceAllIPAddressesAutomatically();
        if (error != FlyCapture2::PGRERROR_OK)
        {
            PrintError(error);
            return false;
        }
    }

    //Find all cameras
    error = busMgr.GetNumOfCameras(&numCameras);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        PrintError(error);
        return false;
    }

    std::cout << "Number of cameras detected: " << numCameras << std::endl;

    if (numCameras < 1)
    {
        std::cout << "Insufficient number of cameras... exiting" << std::endl;
        std::cout << "Make sure at least one cameras are connected for example to "
                "run." << std::endl;
        return false;
    }

    //=========================================//
    //
    // Initialize an array of cameras
    //
    // *** NOTES ***
    // The size of the array is equal to the number of cameras detected.
    // The array of cameras will be used for connecting, configuring,
    // and capturing images.
    //
    //FlyCapture2::Camera *pCameras = new FlyCapture2::Camera[numCameras];
    //FlyCapture2::Camera *pCameras = new FlyCapture2::Camera[1];
    m_pCamera = std::make_shared<FlyCapture2::Camera>();
    bool bFindCamera = false;

    //
    // Prepare each camera to acquire images
    //
    // *** NOTES ***
    // For pseudo-simultaneous streaming, each camera is prepared as if it
    // were just one, but in a loop. Notice that cameras are selected with
    // an index. We demonstrate pseduo-simultaneous streaming because true
    // simultaneous streaming would require multiple process or threads,
    // which is too complex for an example.
    //
    for (unsigned int i = 0; i < numCameras; i++)
    {
        FlyCapture2::PGRGuid guid;
        error = busMgr.GetCameraFromIndex(i, &guid);
        if (error != FlyCapture2::PGRERROR_OK)
        {
            PrintError(error);

            std::cin.ignore();
            return false;
        }

        // Connect to a camera
        error = m_pCamera->Connect(&guid);
        if (error != FlyCapture2::PGRERROR_OK)
        {
            PrintError(error);
            return false;
        }

        // Get the camera information
        FlyCapture2::CameraInfo camInfo;
        error = m_pCamera->GetCameraInfo(&camInfo);
        if (error != FlyCapture2::PGRERROR_OK)
        {
            PrintError(error);

            return false;
        }
        if( camInfo.serialNumber == serialNumber )
        {
            PrintCameraInfo(&camInfo);
            bFindCamera = true;
            break;
        }
        else
        {
            error = m_pCamera->Disconnect();
            if (error != FlyCapture2::PGRERROR_OK)
            {
                PrintError(error);
            }
        }
    }

    //====================================//
    if( !bFindCamera )
    {
        std::cerr << "Can not find the setted camera:"<<serialNumber<< std::endl;
        return false;
    }
    //set the camera
    // Power on the camera
    const unsigned int k_cameraPower = 0x610;
    const unsigned int k_powerVal = 0x80000000;
    error = m_pCamera->WriteRegister(k_cameraPower, k_powerVal);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        PrintError(error);
        return false;
    }
    const unsigned int millisecondsToSleep = 100;
    unsigned int regVal = 0;
    unsigned int retries = 10;

    // Wait for camera to complete power-up
    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(millisecondsToSleep));
        error = m_pCamera->ReadRegister(k_cameraPower, &regVal);
        if (error == FlyCapture2::PGRERROR_TIMEOUT)
        {
            // ignore timeout errors, camera may not be responding to
            // register reads during power-up
        }
        else if (error != FlyCapture2::PGRERROR_OK)
        {
            PrintError(error);
            return false;
        }

        retries--;
    } while ((regVal & k_powerVal) == 0 && retries > 0);

    // Check for timeout errors after retrying
    if (error == FlyCapture2::PGRERROR_TIMEOUT)
    {
        PrintError(error);
        return false;
    }

    // Turn Timestamp on
    FlyCapture2::EmbeddedImageInfo imageInfo;
    imageInfo.timestamp.onOff = true;
    error =m_pCamera->SetEmbeddedImageInfo(&imageInfo);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        PrintError(error);

        return false;
    }


    // Turn trigger mode on
    FlyCapture2::TriggerMode triggerMode;
    error = m_pCamera->GetTriggerMode(&triggerMode);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        PrintError(error);
        return -1;
    }

    // Set camera to trigger mode 0
    triggerMode.onOff = true;
    triggerMode.mode = 0;
    triggerMode.parameter = 0;
#ifdef SOFTWARE_TRIGGER_CAMERA
    // A source of 7 means software trigger
    triggerMode.source = 7;
#else
    // Triggering the camera externally using source 0.
    triggerMode.source = 0;
#endif
    error = m_pCamera->SetTriggerMode(&triggerMode);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        PrintError(error);
        return -1;
    }

    // Poll to ensure camera is ready
    bool retVal = PollForTriggerReady(m_pCamera);
    if (!retVal)
    {
        std::cerr << "Error polling for trigger ready!" << std::endl;
        return false;
    }

    // Get the camera configuration
    FlyCapture2::FC2Config config;
    error = m_pCamera->GetConfiguration(&config);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        PrintError(error);
        return false;
    }

    // Set the grab timeout to 1 seconds
    config.grabTimeout = 1000;
    // Set the camera configuration
    error = m_pCamera->SetConfiguration(&config);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        PrintError(error);
        return false;
    }

    //=======================set=========================//
    //set shutter
    FlyCapture2::Property t_shutter_pro;
    t_shutter_pro.type = FlyCapture2::SHUTTER;
    t_shutter_pro.absControl = true;
    t_shutter_pro.autoManualMode = false;
    t_shutter_pro.absValue = 10.0;
    error = m_pCamera->SetProperty(&t_shutter_pro);
    if (error != FlyCapture2::PGRERROR_OK)
    {
    	PrintError(error);
    	return false;
    }

    //set exposure
    FlyCapture2::Property t_exposure_pro;
    t_exposure_pro.type = FlyCapture2::AUTO_EXPOSURE;
    t_exposure_pro.absControl = true;
    t_exposure_pro.autoManualMode = true;
    error = m_pCamera->SetProperty(&t_exposure_pro);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        PrintError(error);
        return false;
    }

    //set gain
    FlyCapture2::Property t_gain_pro;
    t_gain_pro.type = FlyCapture2::GAIN;
    t_gain_pro.absControl = true;
    t_gain_pro.autoManualMode = true;
    error = m_pCamera->SetProperty(&t_gain_pro);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        PrintError(error);
        return false;
    }

    //set ROI
    if (offsetX != -1 && offsetY != -1 && width != -1 && height != -1)
    {
        FlyCapture2::Format7ImageSettings f7ImageSettings;

        unsigned int packetSize;
        float percentage;
        error = m_pCamera->GetFormat7Configuration(&f7ImageSettings, &packetSize, &percentage);
        if (error != FlyCapture2::PGRERROR_OK)
        {
            PrintError(error);
            return false;
        }

        f7ImageSettings.width = width;
        f7ImageSettings.height = height;
        f7ImageSettings.offsetX = offsetX;
        f7ImageSettings.offsetY = offsetY;
        //f7ImageSettings.pixelFormat = FlyCapture2::PIXEL_FORMAT_BGR;

        FlyCapture2::Format7PacketInfo packetInfo;

        error = m_pCamera->SetFormat7Configuration(&f7ImageSettings, packetInfo.recommendedBytesPerPacket);
        if (error != FlyCapture2::PGRERROR_OK)
        {
            PrintError(error);
            return false;
        }
    }



    // Camera is ready,Start streaming on camera
    error = m_pCamera->StartCapture();
    if (error != FlyCapture2::PGRERROR_OK)
    {
        PrintError(error);

        return false;
    }

#ifdef SOFTWARE_TRIGGER_CAMERA
    if (!CheckSoftwareTriggerPresence(m_pCamera))
    {
        std::cerr << "SOFT_ASYNC_TRIGGER not implemented on this camera! Stopping "
                "application"
             << std::endl;
        return false;
    }
#else
    std::cerr << "Trigger the camera by sending a trigger pulse to GPIO"
         << triggerMode.source << std::endl;

#endif



    return true;
}

bool CPtGreyVideoCapture::InitPtGreyCamera(unsigned int serialNumber,cv::Rect ROI )
{
    if( ROI == cv::Rect() )
        return InitPtGreyCamera(serialNumber,-1,-1,-1,-1);
    else
        return InitPtGreyCamera(serialNumber,ROI.x,ROI.y,ROI.width,ROI.height);
}

bool CPtGreyVideoCapture::StopPtGreyCamera()
{
    try
    {
        if( m_pCamera == nullptr )
            return true;

        FlyCapture2::Error error;

        // =================Turn trigger mode off.===================//
        FlyCapture2::TriggerMode triggerMode;
        error = m_pCamera->GetTriggerMode(&triggerMode);
        if (error != FlyCapture2::PGRERROR_OK)
        {
            PrintError(error);
            //return false;
        }

        triggerMode.onOff = false;
        error = m_pCamera->SetTriggerMode(&triggerMode);
        if (error != FlyCapture2::PGRERROR_OK)
        {
            PrintError(error);
            //return false;
        }



        // Stop capturing images
        error = m_pCamera->StopCapture();
        if (error != FlyCapture2::PGRERROR_OK)
        {
            PrintError(error);
            //return false;
        }

        // Disconnect the camera
        error = m_pCamera->Disconnect();
        if (error != FlyCapture2::PGRERROR_OK)
        {
            PrintError(error);
            //return false;
        }


        return true;
    }
    catch (std::exception& exception)
    {
        std::cerr<<"Error occured when stop the camera:"<<exception.what()<<std::endl;
        return false;
    }
    catch (...)
    {
        std::cerr<<"Error occured when stop the camera!"<<std::endl;
        return false;
    }

}

bool CPtGreyVideoCapture::GetCameraVideo(cv::Mat& videoFrame)
{
    videoFrame.release();

    try
    {
        if( m_bUseThread )
        {
            std::lock_guard<std::mutex> Lock(m_videoFrameMutex);
            m_currentFrame.copyTo(videoFrame);
        }
        else
        {
            FlyCapture2::Error error;

#ifdef SOFTWARE_TRIGGER_CAMERA
            // Check that the trigger is ready
            PollForTriggerReady(m_pCamera);

            // Fire software trigger
            bool retVal = FireSoftwareTrigger(m_pCamera);
            if (!retVal)
            {
                std::cerr << "Error firing software trigger" << std::endl;
                return false;
            }
#endif

            // Grab image
            FlyCapture2::Image rawImage;
            error = m_pCamera->RetrieveBuffer(&rawImage);
            if (error != FlyCapture2::PGRERROR_OK)
            {
                PrintError(error);
                return false;
            }

            //convert original image to BGR format,and save in convertedImage
            FlyCapture2::Image convertedImage;
            rawImage.Convert(FlyCapture2::PixelFormat::PIXEL_FORMAT_BGR, &convertedImage);
            cv::Mat cvImage(convertedImage.GetRows(), convertedImage.GetCols(), CV_8UC3, convertedImage.GetData());
            videoFrame = cvImage.clone();
        }
    }
    catch (...)
    {
        std::cerr<<"Error occured when capture video from PtGrey camera."<<std::endl;
        return false;
    }

    return true;
}

void CPtGreyVideoCapture::CaptureThread()
{
    std::cout<<"Start capture video from capture thread."<<std::endl;

    cv::Mat videoFrame;
    while( !m_bQuitVideoThread )
    {
        videoFrame.release();

        if( !ThreadGetCameraVideo(videoFrame) )
        {
            bool bInitSuccess = false;
            do {
                std::cerr<<"Capture video failed in thread,Restart the camera."<<std::endl;
                bInitSuccess = InitPtGreyCamera(m_serialNumber,m_imageOffsetX,m_imageOffsetY,m_imageWidth,m_imageHeight);

                std::this_thread::sleep_for(std::chrono::microseconds(500));//sleep 500ms
            }while(!bInitSuccess && !m_bQuitVideoThread);
        }

        std::lock_guard<std::mutex> Lock(m_videoFrameMutex);
        videoFrame.copyTo(m_currentFrame);
    }

    std::cout<<"Quit Video capture thread."<<std::endl;

}

bool CPtGreyVideoCapture::ThreadGetCameraVideo(cv::Mat& videoFrame)
{
    try
    {
        FlyCapture2::Error error;

#ifdef SOFTWARE_TRIGGER_CAMERA
        // Check that the trigger is ready
        PollForTriggerReady(m_pCamera);

        // Fire software trigger
        bool retVal = FireSoftwareTrigger(m_pCamera);
        if (!retVal)
        {
            std::cerr << "Error firing software trigger" << std::endl;
            return false;
        }
#endif

        // Grab image
        FlyCapture2::Image rawImage;
        error = m_pCamera->RetrieveBuffer(&rawImage);
        if (error != FlyCapture2::PGRERROR_OK)
        {
            PrintError(error);
            return false;
        }

        //convert original image to BGR format,and save in convertedImage
        FlyCapture2::Image convertedImage;
        rawImage.Convert(FlyCapture2::PixelFormat::PIXEL_FORMAT_BGR, &convertedImage);
        cv::Mat cvImage(convertedImage.GetRows(), convertedImage.GetCols(), CV_8UC3, convertedImage.GetData());
        videoFrame = cvImage.clone();

        return true;
    }
    catch (...)
    {
        std::cerr<<"Error occured when capture video from PtGrey camera by thread."<<std::endl;
        return false;
    }

    return true;
}

void CPtGreyVideoCapture::StartCaptureThread()
{
    m_bUseThread = true;
    m_bQuitVideoThread = false;

    m_captureThread = std::thread(&CPtGreyVideoCapture::CaptureThread,this);

    std::this_thread::sleep_for(std::chrono::microseconds(1000));//sleep 1s
}

void CPtGreyVideoCapture::StopCaptureThread()
{
    m_bQuitVideoThread = true;
    if( m_captureThread.joinable() )
        m_captureThread.join();

    m_bUseThread = false;
}