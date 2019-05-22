//
// Created by nvidia on 7/19/18.
//

#ifndef GS3_U3_CAMCAPTURE_SINGLECAMERACAPTUREEX_H
#define GS3_U3_CAMCAPTURE_SINGLECAMERACAPTUREEX_H

#include "flycapture/FlyCapture2.h"
//#include<flycapture/FlyCapture2.h>
#include<iostream>
#include<vector>
#include <thread>
#include <chrono>

#define fl2 FlyCapture2

using namespace std;

class SingleCameraCaptureEx {

public:
	SingleCameraCaptureEx();

	virtual ~SingleCameraCaptureEx();

	//ʹ���豸���кŽ���ָ�������ʼ��
	fl2::Error initial_Cam(unsigned int cam_serialNum);//m_cam_serialNum,m_pCameras

	//��������в�������
	fl2::Error set_camera(int offsetX = -1, int offsetY = -1,
		int width = -1, int height = -1);

	void PrintError(fl2::Error error);
	void PrintCameraInfo(fl2::CameraInfo *pCamInfo);
	//�������Ĵ���״̬
	bool CheckSoftwareTriggerPresence(fl2::Camera *pCam);
	//Poll to ensure camera trigger mode is readyl
	bool PollForTriggerReady(fl2::Camera *pCam);
	//���������ȡͼ��
	bool FireSoftwareTrigger(fl2::Camera *pCam);

public:

	fl2::Camera* m_pCameras;
	unsigned int m_cam_serialNum, m_num_cam;

	double GetFrameRate();
};


#endif //GS3_U3_CAMCAPTURE_SINGLECAMERACAPTUREEX_H
