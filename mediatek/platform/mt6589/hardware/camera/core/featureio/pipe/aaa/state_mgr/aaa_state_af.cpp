#define LOG_TAG "aaa_state_af"

#ifndef ENABLE_MY_LOG
    #define ENABLE_MY_LOG       (1)
#endif

#include <aaa_types.h>
#include <aaa_error_code.h>
#include <aaa_log.h>
#include <dbg_aaa_param.h>
#include <aaa_hal.h>
#include "aaa_state.h"
#include <buf_mgr.h>
#include <camera_custom_nvram.h>
#include <awb_param.h>
#include <awb_mgr.h>
#include <CamDefs.h>
#include <camera_custom_AEPlinetable.h>
#include <ae_param.h>
#include <ae_mgr.h>
#include <sensor_hal.h>
#include <af_param.h>
#include <mcu_drv.h>
#include <isp_reg.h>
#include <af_mgr.h>
#include "CameraProfile.h"  // For CPTLog*()/AutoCPTLog class.
#include <flash_mgr.h>
#include "flash_tuning_custom.h"
#include <CamDefs.h>
#include <kd_camera_feature.h>

using namespace NS3A;

extern EState_T g_ePrevState;

static sem_t           g_sem;
EState_T g_nextState;

int g_isAFLampOnInAfState;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  StateAF
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
StateAF::
StateAF()
    : IState("StateAF")
{
	 sem_init(&g_sem, 0, 1);
	 g_nextState = eState_Invalid;
	 g_isAFLampOnInAfState = 0;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  eIntent_AFStart
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MRESULT
StateAF::
sendIntent(intent2type<eIntent_AFStart>)
{
    MY_LOG("[StateAF::sendIntent]<eIntent_AFStart>");

    return  S_3A_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  eIntent_AFEnd
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MRESULT
StateAF::
sendIntent(intent2type<eIntent_AFEnd>)
{
    MY_LOG("[StateAF::sendIntent]<eIntent_AFEnd>");

    // State transition: eState_AF --> g_ePrevState
    if(g_nextState!=eState_Invalid)
    {
    	transitState(eState_AF, g_nextState);
    	g_nextState=eState_Invalid;
    }
    else
    {
    	transitState(eState_AF, g_ePrevState);
    	if((FlashMgr::getInstance()->isAFLampOn()==1) && (FlashMgr::getInstance()->getFlashMode() != FLASHLIGHT_TORCH) )
    		FlashMgr::getInstance()->setAFLampOnOff(0);
	}

    g_nextState = eState_Invalid;

    return  S_3A_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  eIntent_Uninit
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MRESULT
StateAF::
sendIntent(intent2type<eIntent_Uninit>)
{
    MY_LOG("[StateAF::sendIntent]<eIntent_Uninit>");

    // AAO DMA buffer uninit
    BufMgr::getInstance().uninit();

    // AE uninit
    AeMgr::getInstance().uninit();

    // AWB uninit
    AwbMgr::getInstance().uninit();

    // AF uninit
    AfMgr::getInstance().uninit();

    // State transition: eState_AF --> eState_Uninit
    transitState(eState_AF, eState_Uninit);

    return  S_3A_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  eIntent_CameraPreviewStart
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MRESULT
StateAF::
sendIntent(intent2type<eIntent_CameraPreviewStart>)
{
    MY_LOG("[StateAF::sendIntent]<eIntent_CameraPreviewStart>");
 	exitPreview();
 	FlashMgr::getInstance()->setAFLampOnOff(0);
 	g_nextState = eState_Invalid;
    return  S_3A_OK;
}

MRESULT
StateAF::
sendIntent(intent2type<eIntent_RecordingEnd>)
{
	MY_LOG("[StateAF::sendIntent]<eIntent_RecordingEnd>");
    return  S_3A_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  eIntent_CamcorderPreviewStart
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MRESULT
StateAF::
sendIntent(intent2type<eIntent_CamcorderPreviewStart>)
{
    MY_LOG("[StateAF::sendIntent]<eIntent_CamcorderPreviewStart>");
    exitPreview();
    FlashMgr::getInstance()->setAFLampOnOff(0);
    g_nextState = eState_Invalid;
    return  S_3A_OK;
}

MRESULT StateAF::exitPreview()
{
	::sem_wait(&g_sem);
	MY_LOG("StateAF::exitPreview line=%d",__LINE__);
	MRESULT err;

    // AE uninit
    AeMgr::getInstance().uninit();

    // AWB uninit
    AwbMgr::getInstance().uninit();

    // AF uninit
    AfMgr::getInstance().uninit();

    // AAO DMA / state disable again
    err = BufMgr::getInstance().AAStatEnable(MFALSE);
    if (FAILED(err)) {
        MY_ERR("BufMgr::getInstance().AAStatEnable(MFALSE) fail\n");
        return err;
    }

    err = BufMgr::getInstance().DMAUninit(camdma2type<ECamDMA_AAO>());
    if (FAILED(err)) {
        MY_ERR("BufMgr::getInstance().DMAunInit(ECamDMA_AAO) fail\n");
        return err;
    }

    // AFO DMA / state disable again
    err = BufMgr::getInstance().AFStatEnable(MFALSE);
    if (FAILED(err)) {
        MY_ERR("BufMgr::getInstance().AFStatEnable(MFALSE) fail\n");
        return err;
    }

    err = BufMgr::getInstance().DMAUninit(camdma2type<ECamDMA_AFO>());
    if (FAILED(err)) {
        MY_ERR("BufMgr::getInstance().DMAunInit(ECamDMA_AFO) fail\n");
        return err;
    }

    // State transition: eState_AF --> eState_Init
    transitState(eState_AF, eState_Init);
    ::sem_post(&g_sem);
    return  S_3A_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  eIntent_CameraPreviewEnd
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MRESULT
StateAF::
sendIntent(intent2type<eIntent_CameraPreviewEnd>)
{
    MY_LOG("[StateAF::sendIntent]<eIntent_CameraPreviewEnd> line=%d",__LINE__);
    exitPreview();
    FlashMgr::getInstance()->setAFLampOnOff(0);
    g_nextState = eState_Invalid;
    return  S_3A_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  eIntent_CamcorderPreviewEnd
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MRESULT
StateAF::
sendIntent(intent2type<eIntent_CamcorderPreviewEnd>)
{
	MY_LOG("[StateAF::sendIntent]<eIntent_CamcorderPreviewEnd> line=%d", __LINE__);
	exitPreview();
	FlashMgr::getInstance()->setAFLampOnOff(0);
	g_nextState = eState_Invalid;
    return  S_3A_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  eIntent_VsyncUpdate
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MRESULT
StateAF::
sendIntent(intent2type<eIntent_VsyncUpdate>)
{
	::sem_wait(&g_sem);
    MRESULT err = S_3A_OK;

    MY_LOG("[StateAF::sendIntent]<eIntent_VsyncUpdate> line=%d", __LINE__);

    switch (getAFState())
    {
    case eAFState_None:
        err = sendAFIntent(intent2type<eIntent_VsyncUpdate>(), state2type<eAFState_None>());
        break;
    case eAFState_PreAF:
        err = sendAFIntent(intent2type<eIntent_VsyncUpdate>(), state2type<eAFState_PreAF>());
        break;
    case eAFState_AF:
        err = sendAFIntent(intent2type<eIntent_VsyncUpdate>(), state2type<eAFState_AF>());
        break;
    case eAFState_PostAF:
        err = sendAFIntent(intent2type<eIntent_VsyncUpdate>(), state2type<eAFState_PostAF>());
        break;
    default:
        break;
    }
    ::sem_post(&g_sem);

    return  err;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  eIntent_AFUpdate
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MRESULT
StateAF::
sendIntent(intent2type<eIntent_AFUpdate>)
{
	::sem_wait(&g_sem);
    MRESULT err = S_3A_OK;

    MY_LOG("[StateAF::sendIntent]<eIntent_AFUpdate>");

    switch (getAFState())
    {
    case eAFState_None:
        err = sendAFIntent(intent2type<eIntent_AFUpdate>(), state2type<eAFState_None>());
        break;
    case eAFState_PreAF:
        err = sendAFIntent(intent2type<eIntent_AFUpdate>(), state2type<eAFState_PreAF>());
        break;
    case eAFState_AF:
        err = sendAFIntent(intent2type<eIntent_AFUpdate>(), state2type<eAFState_AF>());
        break;
    case eAFState_PostAF:
        err = sendAFIntent(intent2type<eIntent_AFUpdate>(), state2type<eAFState_PostAF>());
        break;
    default:
        break;
    }
	::sem_post(&g_sem);
    return  err;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  eIntent_PrecaptureStart
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MRESULT
StateAF::
sendIntent(intent2type<eIntent_PrecaptureStart>)
{
    MY_LOG("[StateAF::sendIntent]<eIntent_PrecaptureStart>");

    // Init

    // State transition: eState_AF --> eState_Precapture
    //transitState(eState_AF, eState_Precapture);
    g_nextState = eState_Precapture;
    return  S_3A_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  eAFState_PreAF
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

MRESULT
StateAF::
sendAFIntent(intent2type<eIntent_VsyncUpdate>, state2type<eAFState_PreAF>)
{
	//MY_LOG("[StateAF::sendAFIntent]<eIntent_VsyncUpdate>,<eAFState_PreAF>");
	XLOGD("[StateAF::sendAFIntent]<eIntent_VsyncUpdate>,<eAFState_PreAF>");
    MRESULT err = S_3A_OK;
    BufInfo_T rBufInfo;
    // Update frame count
    updateFrameCount();
    static int is1stFrameToCheckAFLamp=1;
    static int skipFrames=0;
    static int isAFLampOn=0;

    if(AeMgr::getInstance().IsDoAEInPreAF() == MTRUE) {   // do AE/AWB before AF start

        //FlashMgr::getInstance()->capCheckAndFireFlash();
        Param_T param;
        m_pHal3A->getParams(param);
        if(is1stFrameToCheckAFLamp==1)
        {
        	skipFrames=0;
        }
        if(is1stFrameToCheckAFLamp==1 && param.u4CamMode!=eAppMode_VideoMode)
        {
        	XLOGD("eAFState_PreAF-is1stFrameToCheckAFLamp line=%d",__LINE__);
        	is1stFrameToCheckAFLamp=0;

        	isAFLampOn = cust_isNeedAFLamp(	FlashMgr::getInstance()->getFlashMode(),
        	              	FlashMgr::getInstance()->getAfLampMode(),
        					AeMgr::getInstance().IsStrobeBVTrigger());
        	XLOGD("eAFState_PreAF-cust_isNeedAFLamp ononff:%d flashM:%d AfLampM:%d triger:%d",
        					isAFLampOn,
        					FlashMgr::getInstance()->getFlashMode(),
        	              	FlashMgr::getInstance()->getAfLampMode(),
        					AeMgr::getInstance().IsStrobeBVTrigger());
        	if(isAFLampOn==1)
        	{
        		XLOGD("eAFState_PreAF-isAFLampOn=1");
        		skipFrames=2;
                     AeMgr::getInstance().doBackAEInfo();
        		FlashMgr::getInstance()->setAFLampOnOff(1);
        		//AwbMgr::getInstance().backupAWB();
        	}

            //AwbMgr::getInstance().setAFLV(AeMgr::getInstance().getLVvalue());
        }

        // Dequeue AAO DMA buffer
        BufMgr::getInstance().dequeueHwBuf(ECamDMA_AAO, rBufInfo);

        // one-shot AWB
		//if (isAFLampOn==0)
		//	AwbMgr::getInstance().setStrobeMode(AWB_STROBE_MODE_OFF);
		//else
		//	AwbMgr::getInstance().setStrobeMode(AWB_STROBE_MODE_ON);

		//AwbMgr::getInstance().doAFAWB(reinterpret_cast<MVOID *>(rBufInfo.virtAddr));

        if (skipFrames>0)
        {
        	skipFrames--;
        }
        else
        {
        	//skipFrames=0;

#if 0
        if (getFrameCount() < 0) {// AA statistics is not ready
            // Update AAO DMA base address for next frame
            err = BufMgr::getInstance().updateDMABaseAddr(camdma2type<ECamDMA_AAO>(), BufMgr::getInstance().getNextHwBuf(ECamDMA_AAO));
            return err;
        }
#endif

            // AE
            CPTLog(Event_Pipe_3A_AE, CPTFlagStart);    // Profiling Start.
            AeMgr::getInstance().doAFAE(reinterpret_cast<MVOID *>(rBufInfo.virtAddr));
            CPTLog(Event_Pipe_3A_AE, CPTFlagEnd);    // Profiling Start.

            if(AeMgr::getInstance().IsAEStable() == MTRUE) {
                MY_LOG("[StateAF::sendAFIntent]Pre AF transfer to AF state\n");

                AfMgr::getInstance().triggerAF();
                transitAFState(eAFState_AF);
                is1stFrameToCheckAFLamp=1;
            }
        }

        // Enqueue AAO DMA buffer
        BufMgr::getInstance().enqueueHwBuf(ECamDMA_AAO, rBufInfo);

        // Update AAO DMA base address for next frame
        err = BufMgr::getInstance().updateDMABaseAddr(camdma2type<ECamDMA_AAO>(), BufMgr::getInstance().getNextHwBuf(ECamDMA_AAO));
    }else {
        // change to next state directly
        AfMgr::getInstance().triggerAF();
        transitAFState(eAFState_AF);
        is1stFrameToCheckAFLamp=1; //set to 1. Then, in the first frame of AE (next AF),  check if af needs enable.
    }
    return  S_3A_OK;
}

MRESULT
StateAF::
sendAFIntent(intent2type<eIntent_AFUpdate>, state2type<eAFState_PreAF>)
{
    MY_LOG("[StateAF::sendAFIntent]<eIntent_AFUpdate>,<eAFState_PreAF>");

    MRESULT err = S_3A_OK;
    BufInfo_T rBufInfo;

    // Dequeue AFO DMA buffer
    BufMgr::getInstance().dequeueHwBuf(ECamDMA_AFO, rBufInfo);

    CPTLog(Event_Pipe_3A_Single_AF, CPTFlagStart);    // Profiling Start.
    AfMgr::getInstance().doAF(reinterpret_cast<MVOID *>(rBufInfo.virtAddr));
    CPTLog(Event_Pipe_3A_Single_AF, CPTFlagEnd);    // Profiling Start.

    // Enqueue AFO DMA buffer
    BufMgr::getInstance().enqueueHwBuf(ECamDMA_AFO, rBufInfo);

    // FIXME: state transition
    // transitAFState(eAFState_AF);
    return  S_3A_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  eAFState_AF
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MRESULT
StateAF::
sendAFIntent(intent2type<eIntent_VsyncUpdate>, state2type<eAFState_AF>)
{
#define AfLampOnSkipFrame 6
    static int skipFrames=0;
    BufInfo_T rBufInfo;

    MY_LOG("[StateAF::sendAFIntent]<eIntent_VsyncUpdate>,<eAFState_AF>");

    if (AfMgr::getInstance().isFocusFinish())
    {

/*
    	if(skipFrames>0)
    	{
    		FlashMgr::getInstance()->setAFLampOnOff(0);
    		AeMgr::getInstance().setExpTest(1);
    		skipFrames--;
    		 return S_3A_OK;
        }
    	else
    	{
    		// FIXME: state transition
            if (AeMgr::getInstance().IsDoAEInPreAF() == MTRUE)
            {
                transitAFState(eAFState_None);
                transitState(eState_AF, g_ePrevState);
            }
            else {
                transitAFState(eAFState_PostAF);
            }
    	}
    	*/


    	if(skipFrames>0)
    	{
			if(skipFrames == AfLampOnSkipFrame)
            AeMgr::getInstance().doRestoreAEInfo();

			if((FlashMgr::getInstance()->getFlashMode() != FLASHLIGHT_TORCH) && (skipFrames == AfLampOnSkipFrame-2))
			{
#ifdef MTK_AF_SYNC_RESTORE_SUPPORT
				XLOGD("af sync support");
				usleep(33000);
#else
				XLOGD("af sync NOT support");
#endif
				FlashMgr::getInstance()->setAFLampOnOff(0);
      		}


			int aeFrm;
			aeFrm = AfLampOnSkipFrame-skipFrames;
			if(aeFrm>=0)
				AeMgr::getInstance().setRestore(aeFrm);

			//BufInfo_T rBufInfo;
			//BufMgr::getInstance().dequeueHwBuf(ECamDMA_AAO, rBufInfo);

			//	if(skipFrames == AfLampOnSkipFrame-2)
			//	AwbMgr::getInstance().restoreAWB();
			//if(skipFrames <= AfLampOnSkipFrame)
			{
            // one-shot AWB
			//    MINT32 i4SceneLV = AeMgr::getInstance().getLVvalue();
				//AwbMgr::getInstance().setStrobeMode(AWB_STROBE_MODE_OFF);
			    //AwbMgr::getInstance().doAFAWB(reinterpret_cast<MVOID *>(rBufInfo.virtAddr));
			}

			// if(skipFrames==2)
			// 	AeMgr::getInstance().setExp(10000);
			//


			//int y;
			//y= aaToMean(reinterpret_cast<MVOID *>(rBufInfo.virtAddr));
			//XLOGD("skipFrames=\t%d\taeyy=\t%d",skipFrames, y);
			//BufMgr::getInstance().enqueueHwBuf(ECamDMA_AAO, rBufInfo);
            // Update AAO DMA base address for next frame
			//BufMgr::getInstance().updateDMABaseAddr(camdma2type<ECamDMA_AAO>(), BufMgr::getInstance().getNextHwBuf(ECamDMA_AAO));

            skipFrames--;
			//AeMgr::getInstance().setExpTest();
			//AeMgr::getInstance().setCnt(0);

        }
		else
		{
			if (AeMgr::getInstance().IsDoAEInPreAF() == MTRUE)
			{
                transitAFState(eAFState_None);

                if(g_nextState!=eState_Invalid)
			    {
			    	transitState(eState_AF, g_nextState);
			    	g_nextState=eState_Invalid;
			    }
			    else
                	transitState(eState_AF, g_ePrevState);
            }
			else
			{
                transitAFState(eAFState_PostAF);
            }
        }
    }
    else {
        if((FlashMgr::getInstance()->isAFLampOn()==1) && (FlashMgr::getInstance()->getFlashMode() != FLASHLIGHT_TORCH) )
        {
            skipFrames = AfLampOnSkipFrame;
            g_isAFLampOnInAfState=1;
        }
        else
        {
        		g_isAFLampOnInAfState=0;
            skipFrames = 0;
        }
    }

    return  S_3A_OK;
}

MRESULT
StateAF::
sendAFIntent(intent2type<eIntent_AFUpdate>, state2type<eAFState_AF>)
{
    MY_LOG("[StateAF::sendAFIntent]<eIntent_AFUpdate>,<eAFState_AF>");

    MRESULT err = S_3A_OK;
    BufInfo_T rBufInfo;

    if (AfMgr::getInstance().isFocusFinish())
        return S_3A_OK;

    // Dequeue AFO DMA buffer
    BufMgr::getInstance().dequeueHwBuf(ECamDMA_AFO, rBufInfo);

    CPTLog(Event_Pipe_3A_Single_AF, CPTFlagStart);    // Profiling Start.
    AfMgr::getInstance().doAF(reinterpret_cast<MVOID *>(rBufInfo.virtAddr));
    CPTLog(Event_Pipe_3A_Single_AF, CPTFlagEnd);    // Profiling Start.

    // Enqueue AFO DMA buffer
    BufMgr::getInstance().enqueueHwBuf(ECamDMA_AFO, rBufInfo);

#if 0
    if (AfMgr::getInstance().isFocusFinish())   {
        FlashMgr::getInstance()->setAFLampOnOff(0);
        if(AeMgr::getInstance().IsDoAEInPreAF() == MTRUE)   {

            MY_LOG("SingleAF Callback Notify %d", g_ePrevState);
            //AF callback notify
            AfMgr::getInstance().SingleAF_CallbackNotify();
            transitAFState(eAFState_None);
            transitState(eState_AF, g_ePrevState);
        }
        else   {
            transitAFState(eAFState_PostAF);
        }
    }
#else
    MY_LOG("AfMgr::getInstance().isFocusFinish() = %d", AfMgr::getInstance().isFocusFinish());

    if (AfMgr::getInstance().isFocusFinish()) {
        MY_LOG("SingleAF Callback Notify %d", g_ePrevState);
        //AF callback notify
        AfMgr::getInstance().SingleAF_CallbackNotify();
    }
#endif

    return  err;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  eAFState_PostAF
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MRESULT
StateAF::
sendAFIntent(intent2type<eIntent_VsyncUpdate>, state2type<eAFState_PostAF>)
{
    MRESULT err = S_3A_OK;
    BufInfo_T rBufInfo;

    MY_LOG("[StateAF::sendAFIntent]<eIntent_VsyncUpdate>,<eAFState_PostAF>");

    if(AeMgr::getInstance().IsDoAEInPreAF() == MFALSE) {    // do AE/AWB after AF done

        // Update frame count
        updateFrameCount();

        if (getFrameCount() < 0) {// AA statistics is not ready
            // Update AAO DMA base address for next frame
            err = BufMgr::getInstance().updateDMABaseAddr(camdma2type<ECamDMA_AAO>(), BufMgr::getInstance().getNextHwBuf(ECamDMA_AAO));
            return err;
        }

        // Dequeue AAO DMA buffer
        BufMgr::getInstance().dequeueHwBuf(ECamDMA_AAO, rBufInfo);

        // AE
        CPTLog(Event_Pipe_3A_AE, CPTFlagStart);    // Profiling Start.
        AeMgr::getInstance().doAFAE(reinterpret_cast<MVOID *>(rBufInfo.virtAddr));
        CPTLog(Event_Pipe_3A_AE, CPTFlagEnd);    // Profiling Start.

        //if(AeMgr::getInstance().IsAEStable() == MTRUE) {
        //	MINT32 i4SceneLV = AeMgr::getInstance().getLVvalue();
		//   AwbMgr::getInstance().doPreCapAWB(i4SceneLV, reinterpret_cast<MVOID *>(rBufInfo.virtAddr));
        //}

        // Enqueue AAO DMA buffer
        BufMgr::getInstance().enqueueHwBuf(ECamDMA_AAO, rBufInfo);

        // Update AAO DMA base address for next frame
        err = BufMgr::getInstance().updateDMABaseAddr(camdma2type<ECamDMA_AAO>(), BufMgr::getInstance().getNextHwBuf(ECamDMA_AAO));

        if(AeMgr::getInstance().IsAEStable() == MTRUE) {
            MY_LOG("[StateAF::sendAFIntent]Post AF: finish\n");
            //AF callback notify
            AfMgr::getInstance().SingleAF_CallbackNotify();
            transitAFState(eAFState_None);
            if(g_nextState!=eState_Invalid)
            {
	        	transitState(eState_AF, g_nextState);
	        	g_nextState=eState_Invalid;
	        }
	        else
            transitState(eState_AF, g_ePrevState);
        }
    }else {
        // change to next state directly
        transitAFState(eAFState_None);
        if(g_nextState!=eState_Invalid)
        {
        	transitState(eState_AF, g_nextState);
        	g_nextState=eState_Invalid;
        }
        else
        transitState(eState_AF, g_ePrevState);
    }

    return  S_3A_OK;
}

MRESULT
StateAF::
sendAFIntent(intent2type<eIntent_AFUpdate>, state2type<eAFState_PostAF>)
{
    MY_LOG("[StateAF::sendAFIntent]<eIntent_AFUpdate>,<eAFState_PostAF>");

    MRESULT err = S_3A_OK;
    BufInfo_T rBufInfo;

    // Dequeue AFO DMA buffer
    BufMgr::getInstance().dequeueHwBuf(ECamDMA_AFO, rBufInfo);

    CPTLog(Event_Pipe_3A_Single_AF, CPTFlagStart);    // Profiling Start.
    AfMgr::getInstance().doAF(reinterpret_cast<MVOID *>(rBufInfo.virtAddr));
    CPTLog(Event_Pipe_3A_Single_AF, CPTFlagEnd);    // Profiling Start.

    // Enqueue AFO DMA buffer
    BufMgr::getInstance().enqueueHwBuf(ECamDMA_AFO, rBufInfo);

    return  S_3A_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  eAFState_None
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MRESULT
StateAF::
sendAFIntent(intent2type<eIntent_VsyncUpdate>, state2type<eAFState_None>)
{
    MY_LOG("[StateAF::sendAFIntent]<eIntent_VsyncUpdate>,<eAFState_None>");
    return  S_3A_OK;
}

MRESULT
StateAF::
sendAFIntent(intent2type<eIntent_AFUpdate>, state2type<eAFState_None>)
{
    MY_LOG("[StateAF::sendAFIntent]<eIntent_AFUpdate>,<eAFState_None>");
    return  S_3A_OK;
}

