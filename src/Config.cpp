
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include "GetStream.h"
#include "public.h"
#include "ConfigParams.h"
#include "Alarm.h"
#include "CapPicture.h"
#include "playback.h"
#include "Voice.h"
#include "tool.h"
#include "Config.h"

using namespace std;

//#define CONFIG_SUB_CHANNEL
//#define CONFIG_SUB_PARAMETERS



/**  @fn  void CALLBACK ExceptionCallBack(DWORD dwType, LONG lUserID, LONG lHandle, void *pUser)
 *   @brief Process exception.
 *   @param (IN) DWORD dwType  
 *   @param (IN) LONG lUserID  
 *   @param (IN) LONG lHandle  
 *   @param (IN) void *pUser  
 *   @return none.  
 */
#if defined(_WIN32)
static void CALLBACK ExceptionCallBack(DWORD dwType, LONG lUserID, LONG lHandle, void *pUser)
#else
static void CALLBACK g_ExceptionCallBack(DWORD dwType, LONG lUserID, LONG lHandle, void *pUser)
#endif
{
	printf("ExceptionCallBack lUserID:%d, handle:%d, user data:%p", lUserID, lHandle, pUser);

	char tempbuf[256];
	memset(tempbuf, 0, 256);
	switch(dwType)
	{
		case EXCEPTION_AUDIOEXCHANGE:		//Audio exchange exception
			cout<<"Audio exchange exception!"<<endl;
			//TODO: close audio exchange
			break;

			//Alarm//
		case EXCEPTION_ALARM:			            //Alarm exception
			cout<<"Alarm exception!"<<endl;
			//TODO: close alarm update
			break;
		case EXCEPTION_ALARMRECONNECT:  //Alarm reconnect
			cout<<"Alarm reconnect."<<endl;
			break;
		case ALARM_RECONNECTSUCCESS:      //Alarm reconnect success
			cout<<"Alarm reconnect success."<<endl;
			break;

		case EXCEPTION_SERIAL:			           //Serial exception
			cout<<"Serial exception!"<<endl;
			//TODO: close exception
			break;

			//Preview//
		case EXCEPTION_PREVIEW:			     //Preview exception
			cout<<"Preview exception!"<<endl;
			//TODO: close preview
			break;
		case EXCEPTION_RECONNECT:			 //preview reconnect
			cout<<"preview reconnecting."<<endl;
			break;
		case PREVIEW_RECONNECTSUCCESS: //Preview reconnect success
			cout<<"Preview reconncet success."<<endl;
			break;
		default:
			break;
	}
}


static void setSubStreamConfig(NET_DVR_COMPRESSION_INFO_V30& highPara){
	highPara.byStreamType=0;
	highPara.byResolution=6;//19 for 720p, 16 for vga, 6 for qvga
	highPara.byBitrateType=1;//0 for variable rate, 1 for fix
	highPara.byPicQuality=3;//0 for best, 5 for worst, 0xfe for auto
	highPara.dwVideoBitrate=4;//25 for 4096k
	highPara.dwVideoFrameRate=10;//0 for all, 1 for 1/60, 10 for 10, 14 for 15, 18 for 30
	highPara.wIntervalFrameI=50;

#ifdef CONFIG_SUB_PARAMETERS
	highPara.byVideoEncType=10;//1 for std h264, 0 for pri h264, 10 for h265
	highPara.byVideoEncComplexity=1; //0 for low, 1 for middle, 2 for high
	highPara.byEnableSvc=0;//0 for disable, 1 for enable, 2 for auto
	highPara.bySmartCodec=0;//0 for disable, 1 for enable
	highPara.byStreamSmooth=50;//1~100, 1 for clear, 100 for smooth
#endif
}

static void setMainStreamConfig(NET_DVR_COMPRESSION_INFO_V30& highPara){
	highPara.byStreamType=0;
	highPara.byResolution=19;//19 for 720p, 16 for vga, 6 for qvga
	highPara.byBitrateType=0;//0 for variable rate, 1 for fix
	highPara.byPicQuality=3;//0 for best, 5 for worst, 0xfe for auto
	highPara.dwVideoBitrate=25;//25 for 4096k, 4 for 64k
	highPara.dwVideoFrameRate=10;//0 for all, 1 for 1/60, 10 for 10, 14 for 15, 18 for 30
	highPara.wIntervalFrameI=20;

#ifdef CONFIG_SUB_PARAMETERS
	highPara.byVideoEncType=1;//1 for std h264, 0 for pri h264, 10 for h265
	highPara.byVideoEncComplexity=1; //0 for low, 1 for middle, 2 for high
	highPara.byEnableSvc=0;//0 for disable, 1 for enable, 2 for auto
	highPara.bySmartCodec=0;//0 for disable, 1 for enable
	highPara.byStreamSmooth=50;//1~100, 1 for clear, 100 for smooth
#endif

	//keep other default
	//highPara.byIntervalBPFrame=;
	//highPara.byAudioEncType=;
	//highPara.byFormatType=;
	//highPara.byAudioBitRate=2;//0 for def, 1 for 8k, 2 for 16k, 4 for 64k
	//highPara.byAudioSamplingRate=;
	//highPara.byres=;
	//highPara.wAverageVideoBitrate=;
}

static void showStreamConfig(const NET_DVR_COMPRESSION_INFO_V30& highPara){
	cout<< "*********major config**********" <<endl;
	cout<< "byStreamType= "		<<"\t" 	<< int(highPara.byStreamType)		<<endl;
	cout<< "byResolution= "		<<"\t" 	<< int(highPara.byResolution)		<<endl;
	cout<< "byBitrateType= "	<<"\t" 	<< int(highPara.byBitrateType)		<<endl;
	cout<< "byPicQuality= "		<<"\t" 	<< int(highPara.byPicQuality)		<<endl;
	cout<< "dwVideoBitrate= "	<<"\t" 	<< int(highPara.dwVideoBitrate)		<<endl;
	cout<< "dwVideoFrameRate= "	<<"\t" 	<< int(highPara.dwVideoFrameRate)	<<endl;
	cout<< "wIntervalFrameI= "	<<"\t" 	<< int(highPara.wIntervalFrameI)	<<endl;

	cout<< "byVideoEncType= "	<<"\t" 	<< int(highPara.byVideoEncType)		<<endl;
	cout<< "byVideoEncComplexity= "	<<"\t" 	<< int(highPara.byVideoEncComplexity)	<<endl;
	cout<< "byEnableSvc= "		<<"\t" 	<< int(highPara.byEnableSvc)		<<endl;
	cout<< "bySmartCodec= "		<<"\t" 	<< int(highPara.bySmartCodec)		<<endl;

	cout<< "*********minor config**********" <<endl;
	cout<< "byIntervalBPFrame= " 	<<"\t" 	<< int(highPara.byIntervalBPFrame)	<<endl;
	cout<< "byAudioEncType= " 	<<"\t" 	<< int(highPara.byAudioEncType)		<<endl;
	cout<< "byFormatType= " 	<<"\t" 	<< int(highPara.byFormatType)		<<endl;
	cout<< "byStreamSmooth= " 	<<"\t" 	<< int(highPara.byStreamSmooth)		<<endl;
	cout<< "byAudioBitRate= " 	<<"\t" 	<< int(highPara.byAudioBitRate)		<<endl;
	cout<< "byAudioSamplingRate= " 	<<"\t" 	<< int(highPara.byAudioSamplingRate)	<<endl;
	cout<< "byres= " 		<<"\t" 	<< int(highPara.byres)			<<endl;
	cout<< "wAverageVideoBitrate= " <<"\t" 	<< int(highPara.wAverageVideoBitrate)	<<endl;

}


Config::Config(){
	cout<<"Config ctor"<<endl;
}

Config::~Config(){
	cout<<"~Config"<<endl;
	NET_DVR_Cleanup();  
}
static BOOL FormatSendBufXml(char *pSendBuf)
{
    return TRUE;
}

int Config::updateConfig(string ip, int port, string user, string passwd)
{

    NET_DVR_Init();
    Demo_SDK_Version();

    //
	LONG lUserID = 0;
	cout<<__LINE__<<endl;
	NET_DVR_DEVICEINFO_V30 struDeviceInfo = {0};
	cout<<__LINE__<<endl;

  	usleep(1000*1000*1);
	cout<<__LINE__<<endl;

	//lUserID = NET_DVR_Login_V30("192.168.0.120", 8000, "admin", "Aim@12345", &struDeviceInfo);  
	lUserID = NET_DVR_Login_V30((char*)ip.c_str(), port, (char*)user.c_str(), (char*)passwd.c_str(), &struDeviceInfo);  

	if (lUserID < 0)  
	{  
		printf("Login error, %d\n", NET_DVR_GetLastError());  
		NET_DVR_Cleanup();  
		return -1;
	}  
  
	//---------------------------------------  
	NET_DVR_SetExceptionCallBack_V30(0, NULL, g_ExceptionCallBack, NULL);  


#if 0 //debug capability
	//---------------------------------------  
    cout<<"----------------------"<<endl;

    const int XML_BUF = 2048*100;
    DWORD dwBufSize;
    char* pRecvBuf = new char[XML_BUF];
    char* pSendBuf = new char[XML_BUF];
    int m_lServerID = -1;
    int m_iDevIndex = -1;
    int m_lResolution = -1;
    int m_lEncodeType = -1;
    printf(pSendBuf,"<CurrentCompressInfo><VideoEncodeType>%d</VideoEncodeType><VideoResolution>%d</VideoResolution></CurrentCompressInfo>", m_lEncodeType, m_lResolution);
    //BOOL res = NET_DVR_GetDeviceAbility(lUserID, DEVICE_ABILITY_INFO, pSendBuf, strlen(pSendBuf), pRecvBuf, XML_BUF);
    //BOOL res = NET_DVR_GetDeviceAbility(lUserID, DEVICE_VIDEOPIC_ABILITY, pSendBuf, strlen(pSendBuf), pRecvBuf, XML_BUF);
    BOOL res = NET_DVR_GetDeviceAbility(lUserID, DEVICE_SOFTHARDWARE_ABILITY, 0, 0, pRecvBuf, XML_BUF);
    
    if (res){
        cout<<"NET_DVR_GetDeviceAbility::DEVICE_SOFTHARDWARE_ABILITY success"<<endl;
        cout<<pRecvBuf<<endl;
    }
    else
        cout<<"NET_DVR_GetDeviceAbility::DEVICE_SOFTHARDWARE_ABILITY fail. error " << NET_DVR_GetLastError()<<endl;

    //
    cout<<"----------------------"<<endl;

    res = NET_DVR_GetDeviceAbility(lUserID, DEVICE_VIDEOPIC_ABILITY, 0, 0, pRecvBuf, XML_BUF);
    
    if (res){
        cout<<"NET_DVR_GetDeviceAbility::DEVICE_VIDEOPIC_ABILITY success"<<endl;
        cout<<pRecvBuf<<endl;
    }
    else
        cout<<"NET_DVR_GetDeviceAbility::DEVICE_VIDEOPIC_ABILITY fail. error " << NET_DVR_GetLastError()<<endl;
    //
    cout<<"----------------------"<<endl;

    res = NET_DVR_GetDeviceAbility(lUserID, DEVICE_NETWORK_ABILITY, 0, 0, pRecvBuf, XML_BUF);
    
    if (res){
        cout<<"NET_DVR_GetDeviceAbility::DEVICE_NETWORK_ABILITY success"<<endl;
        cout<<pRecvBuf<<endl;
    }
    else
        cout<<"NET_DVR_GetDeviceAbility::DEVICE_NETWORK_ABILITY fail. error " << NET_DVR_GetLastError()<<endl;

    cout<<"----------------------"<<endl;

    res = NET_DVR_GetDeviceAbility(lUserID, IPC_FRONT_PARAMETER_V20, 0, 0, pRecvBuf, XML_BUF);
    
    if (res){
        cout<<"NET_DVR_GetDeviceAbility::IPC_FRONT_PARAMETER_V20 success"<<endl;
        cout<<pRecvBuf<<endl;
    }
    else
        cout<<"NET_DVR_GetDeviceAbility::IPC_FRONT_PARAMETER_V20 fail. error " << NET_DVR_GetLastError()<<endl;

    cout<<"----------------------"<<endl;

    res = NET_DVR_GetDeviceAbility(lUserID, DEVICE_ENCODE_ALL_ABILITY_V20, 0, 0, pRecvBuf, XML_BUF);
    
    if (res){
        cout<<"NET_DVR_GetDeviceAbility::DEVICE_ENCODE_ALL_ABILITY_V20 success"<<endl;
        cout<<pRecvBuf<<endl;
    }
    else
        cout<<"NET_DVR_GetDeviceAbility::DEVICE_ENCODE_ALL_ABILITY_V20 fail. error " << NET_DVR_GetLastError()<<endl;

    cout<<"----------------------"<<endl;
#endif 

	//---------------------------------------  
	LONG lRealPlayHandle;  
	NET_DVR_PREVIEWINFO struPlayInfo = { 0 };  
	struPlayInfo.lChannel = 1;           //Ô¤ÀÀÍ¨µÀºÅ  
	//struPlayInfo.hPlayWnd = h;         //ÐèÒªSDK½âÂëÊ±¾ä±úÉèÎªÓÐÐ§Öµ£¬½öÈ¡Á÷²»½âÂëÊ±¿ÉÉèÎª¿Õ  
	//struPlayInfo.dwStreamType = 1;       //0-Ö÷ÂëÁ÷£¬1-×ÓÂëÁ÷£¬2-ÂëÁ÷3£¬3-ÂëÁ÷4£¬ÒÔ´ËÀàÍÆ  
	//struPlayInfo.dwLinkMode = 0;         //0- TCP·½Ê½£¬1- UDP·½Ê½£¬2- ¶à²¥·½Ê½£¬3- RTP·½Ê½£¬4-RTP/RTSP£¬5-RSTP/HTTP  

	int Ret;  
	NET_DVR_COMPRESSIONCFG_V30  struParams = { 0 };  
	DWORD dwReturnLen;  
	Ret = NET_DVR_GetDVRConfig(lUserID, NET_DVR_GET_COMPRESSCFG_V30, struPlayInfo.lChannel, &struParams, sizeof(NET_DVR_COMPRESSIONCFG_V30), &dwReturnLen);  

	if (!Ret)  
	{  
		cout << "Failed to get config" << endl;  
		printf("error code: %d\n", NET_DVR_GetLastError());  
	}  
	else  
	{  
		cout<<"----------------------------------------show main before update-----------------------------------"<<endl;
		showStreamConfig(struParams.struNormHighRecordPara);
#ifdef CONFIG_SUB_CHANNEL
		cout<<"----------------------------------------show sub before update-----------------------------------"<<endl;
		showStreamConfig(struParams.struNetPara);
#endif

		cout<<"----------------------------------------update main & sub-----------------------------------"<<endl;
		setMainStreamConfig(struParams.struNormHighRecordPara);
#ifdef CONFIG_SUB_CHANNEL
		setSubStreamConfig(struParams.struNetPara);
#endif

		cout<<"----------------------------------------set main & sub-----------------------------------"<<endl;
		int SetCamera = NET_DVR_SetDVRConfig(lUserID, NET_DVR_SET_COMPRESSCFG_V30, struPlayInfo.lChannel,  &struParams, sizeof(NET_DVR_COMPRESSIONCFG_V30));  
		if (SetCamera)  
		{  
			cout<<"update config success" <<endl;
		}  
        else{
			cout<<"update config Failed" <<endl;
            printf("config error, %d\n", NET_DVR_GetLastError());  
        }

        //for debug
	    Ret = NET_DVR_GetDVRConfig(lUserID, NET_DVR_GET_COMPRESSCFG_V30, struPlayInfo.lChannel, &struParams, sizeof(NET_DVR_COMPRESSIONCFG_V30), &dwReturnLen);  
		cout<<"----------------------------------------show main after update-----------------------------------"<<endl;
		showStreamConfig(struParams.struNormHighRecordPara);
#ifdef CONFIG_SUB_CHANNEL
		cout<<"----------------------------------------show sub after update-----------------------------------"<<endl;
		showStreamConfig(struParams.struNetPara);
#endif
	}  
	NET_DVR_Logout(lUserID);  
    //
    NET_DVR_Cleanup();

	return 0;  

}

