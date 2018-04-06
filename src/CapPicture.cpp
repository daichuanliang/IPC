/*
* Copyright(C) 2010,Hikvision Digital Technology Co., Ltd 
* 
* File   name£ºCapPicture.cpp
* Discription£º
* Version    £º1.0
* Author     £ºpanyd
* Create Date£º2010_3_25
* Modification History£º
*/

#include "public.h"
#include "CapPicture.h"
#include <stdio.h>
#include <time.h>

/*******************************************************************
      Function:   Demo_Capture
   Description:   Capture picture.
     Parameter:   (IN)   none 
        Return:   0--success£¬-1--fail.   
**********************************************************************/
int Demo_Capture()
{
    NET_DVR_Init();
    long lUserID;
    //login
    NET_DVR_DEVICEINFO_V30 struDeviceInfo;
    lUserID = NET_DVR_Login_V30("192.168.1.104", 8000, "admin", "Aim@12345", &struDeviceInfo);
    if (lUserID < 0)
    {
        printf("pyd1---Login error, %d\n", NET_DVR_GetLastError());
        return HPR_ERROR;
    }
    printf("pyd1---Login success\n");


/*
    BOOL devAbi;
    DWORD JPEGcaptureAbility =DEVIE_JPEG_CAP_ABILITY;
    DWORD resolutionAbility = PIC_CAPTURE_ABILITY;
    */



    //
    NET_DVR_JPEGPARA strPicPara = {0};
    strPicPara.wPicQuality = 1; //0 best, 1 good, 2 normal
    strPicPara.wPicSize = 0xff;
    int iRet;


    int cnt =0;


    time_t ts, te, dur;
    
    ts=time(NULL);

    for (cnt=0;cnt<5000;cnt++){

    iRet = NET_DVR_CaptureJPEGPicture(lUserID, struDeviceInfo.byStartChan, &strPicPara, "./ssss.jpeg");
    if (!iRet)
    {
        printf("pyd1---NET_DVR_CaptureJPEGPicture error, %d\n", NET_DVR_GetLastError());
        return HPR_ERROR;
    }
    if (cnt%200 == 0){
        te=time(NULL);
        dur = te-ts;
        ts = te;
        printf("##@@## fps=%ld\n", long(200.0/dur));
    }
}
     

    //logout
    NET_DVR_Logout_V30(lUserID);
    NET_DVR_Cleanup();

    return HPR_OK;

}
