//
// Created by nvidia on 7/19/18.
//
#include "SingleCameraCaptureEx.h"

SingleCameraCaptureEx::SingleCameraCaptureEx() { m_num_cam = 0; m_pCameras = nullptr; }

SingleCameraCaptureEx::~SingleCameraCaptureEx()
{
	if (m_num_cam != 0)
	{
		m_pCameras->StopCapture();
		m_pCameras->Disconnect();
	}

	delete[] m_pCameras; m_pCameras = nullptr;
}

fl2::Error SingleCameraCaptureEx::initial_Cam(unsigned int cam_serialNum) {
	fl2::Error error;
	// Initialize BusManager and retrieve number of cameras detected
	fl2::BusManager busMgr;
	m_num_cam = 0;
	error = busMgr.GetNumOfCameras(&m_num_cam);
	if (error != fl2::PGRERROR_OK)
	{
		PrintError(error);
		return error;
	}

	std::cout << "Number of cameras detected: " << m_num_cam << endl;

	// Check to make sure at least two cameras are connected before running
	if (m_num_cam < 1)
	{
		std::cout << "Insufficient number of cameras." << endl;
		std::cout << "Make sure at least " << 1 << " cameras are connected for example to "
			"run."
			<< endl;
		std::cout << "Press Enter to exit." << endl;
		m_num_cam = 0;
		return error;
	}

	// Initialize an array of cameras
	//
	// *** NOTES ***
	// The size of the array is equal to the number of cameras detected.
	// The array of cameras will be used for connecting, configuring,
	// and capturing images.

	m_pCameras = new fl2::Camera[1];

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
	unsigned int tcam_id = 0;
	for (unsigned int i = 0; i < m_num_cam; i++)
	{

		//������򿪵�����Ƿ�Ϊ�����豸������ָ�������
		bool t_step = true;

		fl2::PGRGuid guid;
		error = busMgr.GetCameraFromIndex(i, &guid);
		if (error != fl2::PGRERROR_OK)
		{
			PrintError(error);
			m_num_cam = 0;
			delete[] m_pCameras; m_pCameras = nullptr;
			return error;
		}//if(error != PGRERROR_OK)

		 // Connect to a camera
		error = m_pCameras[tcam_id].Connect(&guid);
		if (error != fl2::PGRERROR_OK)
		{
			PrintError(error);
			m_num_cam = 0;
			delete[] m_pCameras; m_pCameras = nullptr;
			return error;
		}//if (error != PGRERROR_OK)

		 // Get the camera information
		fl2::CameraInfo camInfo;
		error = m_pCameras[tcam_id].GetCameraInfo(&camInfo);
		if (error != fl2::PGRERROR_OK)
		{
			PrintError(error);
			delete[] m_pCameras; m_pCameras = nullptr;
			m_num_cam = 0;
			return error;
		}//if (error != PGRERROR_OK)

		if (camInfo.serialNumber == cam_serialNum)
		{
			m_cam_serialNum = cam_serialNum;
			t_step = false;
		}

		if (t_step)
		{
			error = m_pCameras[tcam_id].Disconnect();
			if (error != fl2::PGRERROR_OK)
			{
				PrintError(error);
			}//if (error != PGRERROR_OK)
			continue;
		}//if
		else
		{
			PrintCameraInfo(&camInfo);

			// Turn Timestamp on
			fl2::EmbeddedImageInfo imageInfo;
			imageInfo.timestamp.onOff = true;
			error = m_pCameras[tcam_id].SetEmbeddedImageInfo(&imageInfo);
			if (error != fl2::PGRERROR_OK)
			{
				PrintError(error);
				delete[] m_pCameras; m_pCameras = nullptr;
				std::cout << "Press Enter to exit." << endl;
				m_num_cam = 0;
				return error;
			}//if (error != PGRERROR_OK)

			tcam_id++;
		}//else (t_step==false)

	}//for(i)

	 //�Ƿ�ɹ���ʼ���������������
	if (tcam_id == 1)
	{
		m_num_cam = 1;
	}
	else
	{
		m_pCameras->StopCapture();
		m_pCameras->Disconnect();

		m_num_cam = 0;
		m_cam_serialNum = 0;
		delete[] m_pCameras; m_pCameras = nullptr;
		cout << "insufficient number of camera" << endl;
	}

	return fl2::Error();
}

fl2::Error SingleCameraCaptureEx::set_camera(int offsetX, int offsetY, int width, int height)
{

	fl2::Error error;
	if (m_num_cam<1)
	{
		std::cout << "Insufficient number of cameras" << endl;
		return error;
	}

	//-------------------------------------���������������------------------------------------//
	const unsigned int millisecondsToSleep = 100;
	unsigned int regVal = 0;
	unsigned int retries = 10;

	//------------------------------------------set softer trigger--------------------------------//
	// Power on the camera
	const unsigned int k_cameraPower2 = 0x610;
	const unsigned int k_powerVal2 = 0x80000000;
	error = m_pCameras->WriteRegister(k_cameraPower2, k_powerVal2);
	if (error != fl2::PGRERROR_OK)
	{
		PrintError(error);
		return error;
	}

	regVal = 0;
	retries = 10;

	// Wait for camera to complete power-up
	do
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(millisecondsToSleep));
		error = m_pCameras->ReadRegister(k_cameraPower2, &regVal);
		if (error == fl2::PGRERROR_TIMEOUT)
		{
			// ignore timeout errors, camera may not be responding to
			// register reads during power-up
		}
		else if (error != fl2::PGRERROR_OK)
		{
			PrintError(error);
			return error;
		}

		retries--;
	} while ((regVal & k_powerVal2) == 0 && retries > 0);

	// Check for timeout errors after retrying
	if (error == fl2::PGRERROR_TIMEOUT)
	{
		PrintError(error);
		return error;
	}

	//------------------------------����primary camera����
	fl2::TriggerMode t_prim_triggerM;
	error = m_pCameras->GetTriggerMode(&t_prim_triggerM);
	if (error != fl2::PGRERROR_OK)
	{
		PrintError(error);
		return error;
	}
	t_prim_triggerM.onOff = true;
	t_prim_triggerM.mode = 0;
	t_prim_triggerM.parameter = 0;
	// A source of 7 means software trigger
	t_prim_triggerM.source = 7;
	error = m_pCameras->SetTriggerMode(&t_prim_triggerM);
	if (error != fl2::PGRERROR_OK)
	{
		PrintError(error);
		return error;
	}

	//--------------------Poll to ensure camera trigger mode is ready
	bool retVal = PollForTriggerReady(m_pCameras);
	if (!retVal)
	{
		cout << endl;
		cout << "Error polling for trigger ready!" << endl;
		return fl2::Error();
	}//if

	 // ----------------------------Get the camera configuration
	fl2::FC2Config config;
	error = m_pCameras->GetConfiguration(&config);
	if (error != fl2::PGRERROR_OK)
	{
		PrintError(error);
		return fl2::Error();
	}

	// Set the grab timeout to 1 seconds
	config.grabTimeout = 1000;

	// Set the camera configuration
	error = m_pCameras->SetConfiguration(&config);
	if (error != fl2::PGRERROR_OK)
	{
		PrintError(error);
		return fl2::Error();
	}

	////---------------------------------------�����ع�ʱ��-------------------------------------//
	//fl2::Property t_shutter_pro;
	//t_shutter_pro.type = fl2::SHUTTER;
	//t_shutter_pro.absControl = true;
	//t_shutter_pro.autoManualMode = false;
	//t_shutter_pro.absValue = 10.0;

	//error = m_pCameras->SetProperty(&t_shutter_pro);
	//if (error != fl2::PGRERROR_OK)
	//{
	//	PrintError(error);
	//	return error;
	//}

	//---------------------------------------�����ع�����-------------------------------------//
	fl2::Property t_exposure_pro;
	t_exposure_pro.type = fl2::AUTO_EXPOSURE;
	t_exposure_pro.absControl = true;
	t_exposure_pro.autoManualMode = true;

	error = m_pCameras->SetProperty(&t_exposure_pro);
	if (error != fl2::PGRERROR_OK)
	{
		PrintError(error);
		return error;
	}

	//---------------------------------------����Gain-------------------------------------//
	fl2::Property t_gain_pro;
	t_gain_pro.type = fl2::GAIN;
	t_gain_pro.absControl = true;
	t_gain_pro.autoManualMode = true;

	error = m_pCameras->SetProperty(&t_gain_pro);
	if (error != fl2::PGRERROR_OK)
	{
		PrintError(error);
		return error;
	}

	//设置图像大小
	if (offsetX != -1 && offsetY != -1 && width != -1 && height != -1)
	{
		FlyCapture2::Format7ImageSettings f7ImageSettings;

		unsigned int packetSize;
		float percentage;
		m_pCameras->GetFormat7Configuration(&f7ImageSettings, &packetSize, &percentage);

		f7ImageSettings.width = width;
		f7ImageSettings.height = height;
		f7ImageSettings.offsetX = offsetX;
		f7ImageSettings.offsetY = offsetY;

        //f7ImageSettings.pixelFormat = fl2::PIXEL_FORMAT_MONO8;

		m_pCameras->SetFormat7Configuration(&f7ImageSettings, packetSize);
	}


	// Camera is ready, start capturing images
	error = m_pCameras->StartCapture();
	if (error != fl2::PGRERROR_OK)
	{
		PrintError(error);
		return fl2::Error();
	}

	if (!CheckSoftwareTriggerPresence(m_pCameras))
	{
		cout << "SOFT_ASYNC_TRIGGER not implemented on this camera! Stopping "
			"application"
			<< endl;
	}

	return fl2::Error();
}

void SingleCameraCaptureEx::PrintError(fl2::Error error) {
	error.PrintErrorTrace();
}

void SingleCameraCaptureEx::PrintCameraInfo(fl2::CameraInfo *pCamInfo) {
	std::cout << endl;
	std::cout << "*** CAMERA INFORMATION ***" << endl;
	std::cout << "Serial number - " << pCamInfo->serialNumber << endl;
	std::cout << "Camera model - " << pCamInfo->modelName << endl;
	std::cout << "Camera vendor - " << pCamInfo->vendorName << endl;
	std::cout << "Sensor - " << pCamInfo->sensorInfo << endl;
	std::cout << "Resolution - " << pCamInfo->sensorResolution << endl;
	std::cout << "Firmware version - " << pCamInfo->firmwareVersion << endl;
	std::cout << "Firmware build time - " << pCamInfo->firmwareBuildTime << endl
		<< endl;
}

bool SingleCameraCaptureEx::CheckSoftwareTriggerPresence(fl2::Camera *pCam) {
	const unsigned int k_triggerInq = 0x530;

	fl2::Error error;
	unsigned int regVal = 0;

	error = pCam->ReadRegister(k_triggerInq, &regVal);

	if (error != fl2::PGRERROR_OK)
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

bool SingleCameraCaptureEx::PollForTriggerReady(fl2::Camera *pCam) {
	const unsigned int k_cameraPower = 0x62C;
	fl2::Error error;
	unsigned int regVal = 0;

	do
	{
		error = pCam->ReadRegister(k_cameraPower, &regVal);
		if (error != fl2::PGRERROR_OK)
		{
			PrintError(error);
			return false;
		}

	} while ((regVal >> 31) != 0);

	return true;
}

bool SingleCameraCaptureEx::FireSoftwareTrigger(fl2::Camera *pCam) {
	const unsigned int k_softwareTrigger = 0x62C;
	const unsigned int k_fireVal = 0x80000000;
	fl2::Error error;

	error = pCam->WriteRegister(k_softwareTrigger, k_fireVal);
	if (error != fl2::PGRERROR_OK)
	{
		PrintError(error);
		return false;
	}

	return true;
}

double SingleCameraCaptureEx::GetFrameRate()
{
	FlyCapture2::Property prop;
	prop.type = FlyCapture2::FRAME_RATE;
	if (m_pCameras == NULL)
	{
		return 0.0;
	}
	else
	{
		FlyCapture2::Error error = m_pCameras->GetProperty(&prop);
		return (error == FlyCapture2::PGRERROR_OK) ? prop.absValue : 0.0;
	}
	
}
