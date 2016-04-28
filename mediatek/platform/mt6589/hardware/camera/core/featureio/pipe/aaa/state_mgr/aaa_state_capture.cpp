#define LOG_TAG "aaa_state_capture"

#ifndef ENABLE_MY_LOG
    #define ENABLE_MY_LOG       (1)
#endif

#include <aaa_types.h>
#include <aaa_error_code.h>
#include <aaa_log.h>
#include <dbg_aaa_param.h>
#include <aaa_hal.h>
#include <camera_custom_nvram.h>
#include <awb_param.h>
#include <awb_mgr.h>
#include <buf_mgr.h>
#include "aaa_state.h"
#include <CamDefs.h>
#include <camera_custom_AEPlinetable.h>
#include <ae_param.h>
#include <ae_mgr.h>
#include <flash_mgr.h>
#include <sensor_hal.h>
#include <af_param.h>
#include <mcu_drv.h>
#include <af_mgr.h>
#include <isp_tuning.h>
#include <dbg_isp_param.h>
#include <isp_tuning_mgr.h>
#include <lsc_mgr.h>

using namespace NS3A;
using namespace NSIspTuning;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  StateCapture
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
StateCapture::
StateCapture()
    : IState("StateCapture")
{
	MY_LOG("IState(StateCapture)  line=%d", __LINE__);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  eIntent_CaptureStart
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MRESULT
StateCapture::
sendIntent(intent2type<eIntent_CaptureStart>)
{
	MY_LOG("sendIntent(intent2type<eIntent_CaptureStart>)  line=%d", __LINE__);

/*
	FlashMgr::getInstance()->endPrecapture();
	if(FlashMgr::getInstance()->isBurstShotMode()==1)
	{
		FlashMgr::getInstance()->changeBurstEngLevel();
	}
	*/


    // AE: update capture parameter
    AeMgr::getInstance().doCapAE();

    AwbMgr::getInstance().cameraCaptureInit();

    if ((IspTuningMgr::getInstance().getOperMode() == EOperMode_Normal) ||
        (IspTuningMgr::getInstance().getSensorMode() == ESensorMode_Capture)) {

        // AAO DMA / state enable again
        MRESULT err = BufMgr::getInstance().DMAInit(camdma2type<ECamDMA_AAO>());
        if (FAILED(err)) {
            MY_ERR("BufMgr::getInstance().DMAInit(ECamDMA_AAO) fail\n");
            return err;
        }

        err = BufMgr::getInstance().AAStatEnable(MTRUE);
        if (FAILED(err)) {
            MY_ERR("BufMgr::getInstance().AAStatEnable(MTRUE) fail\n");
            return err;
        }


        // AFO DMA / state enable again
        err = BufMgr::getInstance().DMAInit(camdma2type<ECamDMA_AFO>());
        if (FAILED(err)) {
            MY_ERR("BufMgr::getInstance().DMAInit(ECamDMA_AFO) fail\n");
            return err;
        }

        err = BufMgr::getInstance().AFStatEnable(MTRUE);
        if (FAILED(err)) {
            MY_ERR("BufMgr::getInstance().AFStatEnable(MTRUE) fail\n");
            return err;
        }

    }

    return  S_3A_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  eIntent_CaptureEnd
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MRESULT
StateCapture::
sendIntent(intent2type<eIntent_CaptureEnd>)
{
	MY_LOG("sendIntent(intent2type<eIntent_CaptureEnd>)  line=%d", __LINE__);
    BufInfo_T rBufInfo;
    MRESULT err;

    //
    FlashMgr::getInstance()->capCheckAndFireFlash_End();

    if ((IspTuningMgr::getInstance().getOperMode() == EOperMode_Normal) ||
        (IspTuningMgr::getInstance().getSensorMode() == ESensorMode_Capture)) {

        // Dequeue AAO DMA buffer
        BufMgr::getInstance().dequeueHwBuf(ECamDMA_AAO, rBufInfo);

        // One-shot AWB
        MINT32 i4SceneLV = AeMgr::getInstance().getLVvalue();

        AwbMgr::getInstance().doCapAWB(i4SceneLV, reinterpret_cast<MVOID *>(rBufInfo.virtAddr));

        MY_LOG("AwbMgr::getInstance().doCapAWB() END");

       //Capture Flare compensate
	   //pass WB gain info
	   AWB_OUTPUT_T _a_rAWBOutput;
	   AwbMgr::getInstance().getAWBOutput(_a_rAWBOutput);
	   AeMgr::getInstance().doCapFlare(reinterpret_cast<MVOID *>(rBufInfo.virtAddr),reinterpret_cast<MVOID *>(&_a_rAWBOutput),FlashMgr::getInstance()->isFlashOnCapture() );

       MY_LOG("AeMgr::getInstance().doCapFlare() END");
       
        // F858
        LscMgr::getInstance()->updateTSFinput(
                static_cast<NSIspTuning::LscMgr::LSCMGR_TSF_INPUT_SRC>(NSIspTuning::LscMgr::TSF_INPUT_CAP),
                AeMgr::getInstance().getLVvalue(), AwbMgr::getInstance().getAWBCCT(),
                reinterpret_cast<MVOID *>(rBufInfo.virtAddr));
        //MTK_SWIP_PROJECT_END

        MY_LOG("LscMgr::getInstance()->updateTSFinput() END");

        // Enqueue AAO DMA buffer
        BufMgr::getInstance().enqueueHwBuf(ECamDMA_AAO, rBufInfo);

        // Update AAO DMA address
        BufMgr::getInstance().updateDMABaseAddr(camdma2type<ECamDMA_AAO>(), BufMgr::getInstance().getNextHwBuf(ECamDMA_AAO));


        // --- best shot select ---
        BufMgr::getInstance().dequeueHwBuf(ECamDMA_AFO, rBufInfo);
        AfMgr::getInstance().calBestShotValue(reinterpret_cast<MVOID *>(rBufInfo.virtAddr));
        BufMgr::getInstance().enqueueHwBuf(ECamDMA_AFO, rBufInfo);
        // --- best shot select ---

        MY_LOG("AfMgr::getInstance().calBestShotValue() END");

    }

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

    MY_LOG("sendIntent(intent2type<eIntent_CaptureEnd>) END");

    return  S_3A_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  eIntent_VsyncUpdate
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MRESULT
StateCapture::
sendIntent(intent2type<eIntent_VsyncUpdate>)
{
	MY_LOG("sendIntent(intent2type<eIntent_VsyncUpdate>)  line=%d", __LINE__);


    return  S_3A_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  eIntent_AFUpdate
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MRESULT
StateCapture::
sendIntent(intent2type<eIntent_AFUpdate>)
{
	MY_LOG("sendIntent(intent2type<eIntent_AFUpdate>)  line=%d", __LINE__);


    return  S_3A_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  eIntent_CameraPreviewStop
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MRESULT
StateCapture::
sendIntent(intent2type<eIntent_CameraPreviewEnd>)
{
	MY_LOG("sendIntent(intent2type<eIntent_CameraPreviewStop>)  line=%d", __LINE__);

    return  S_3A_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  eIntent_CameraPreviewStart
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MRESULT
StateCapture::
sendIntent(intent2type<eIntent_CameraPreviewStart>)
{
	MY_LOG("sendIntent(intent2type<eIntent_CameraPreviewStart>)  line=%d", __LINE__);
    MRESULT err;
    FlashMgr::getInstance()->turnOffFlashDevice();



    // AE init
    err = AeMgr::getInstance().cameraPreviewReinit();
    if (FAILED(err)) {
        MY_ERR("AwbMgr::getInstance().cameraPreviewReinit() fail\n");
        return err;
    }

    // AF init

    // AWB init
    err = AwbMgr::getInstance().cameraPreviewReinit();
    if (FAILED(err)) {
        MY_ERR("AwbMgr::getInstance().cameraPreviewReinit() fail\n");
        return err;
    }

    // AAO DMA / state enable again
    err = BufMgr::getInstance().DMAInit(camdma2type<ECamDMA_AAO>());
    if (FAILED(err)) {
        MY_ERR("BufMgr::getInstance().DMAInit(ECamDMA_AAO) fail\n");
        return err;
    }

    err = BufMgr::getInstance().AAStatEnable(MTRUE);
    if (FAILED(err)) {
        MY_ERR("BufMgr::getInstance().AAStatEnable(MTRUE) fail\n");
        return err;
    }

    AfMgr::getInstance().setAF_IN_HSIZE();
    AfMgr::getInstance().setFlkWinConfig();

    // AFO DMA / state enable again
    err = BufMgr::getInstance().DMAInit(camdma2type<ECamDMA_AFO>());
    if (FAILED(err)) {
        MY_ERR("BufMgr::getInstance().DMAInit(ECamDMA_AFO) fail\n");
        return err;
    }

    err = BufMgr::getInstance().AFStatEnable(MTRUE);
    if (FAILED(err)) {
        MY_ERR("BufMgr::getInstance().AFStatEnable(MTRUE) fail\n");
        return err;
    }

    // Reset frame count to -2
    resetFrameCount();

    IspTuningMgr::getInstance().validatePerFrame(MTRUE);

    // State transition: eState_Capture --> eState_CameraPreview
    transitState(eState_Capture, eState_CameraPreview);

    return  S_3A_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  eIntent_AFEnd
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MRESULT
StateCapture::
sendIntent(intent2type<eIntent_AFEnd>)
{
	MY_LOG("sendIntent(intent2type<eIntent_AFEnd>)  line=%d", __LINE__);

    return  S_3A_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  eIntent_Uninit
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MRESULT
StateCapture::
sendIntent(intent2type<eIntent_Uninit>)
{
	MY_LOG("sendIntent(intent2type<eIntent_Uninit>)  line=%d", __LINE__);
	FlashMgr::getInstance()->turnOffFlashDevice();

    // AAO DMA buffer uninit
    BufMgr::getInstance().uninit();

    // AE uninit
    AeMgr::getInstance().uninit();

    // AWB uninit
    AwbMgr::getInstance().uninit();

    // AF uninit
    AfMgr::getInstance().uninit();

    // State transition: eState_Capture --> eState_Uninit
    transitState(eState_Capture, eState_Uninit);

    return  S_3A_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  eIntent_CamcorderPreviewStart: for CTS only
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MRESULT
StateCapture::
sendIntent(intent2type<eIntent_CamcorderPreviewStart>)
{
    MRESULT err;
    FlashMgr::getInstance()->turnOffFlashDevice();

    MY_LOG("[StateCapture::sendIntent]<eIntent_CamcorderPreviewStart>");

    // AE uninit
    AeMgr::getInstance().uninit();

    // AWB uninit
    AwbMgr::getInstance().uninit();

    // AF uninit
    AfMgr::getInstance().uninit();

    // Get parameters
    Param_T rParam;
    m_pHal3A->getParams(rParam);
    MINT32 i4SensorDev = m_pHal3A->getSensorDev();

    // AE init
    err = AeMgr::getInstance().camcorderPreviewInit(i4SensorDev, rParam);
    if (FAILED(err)) {
        MY_ERR("AebMgr::getInstance().camcorderPreviewInit() fail\n");
        return err;
    }

    // AF init
    err = AfMgr::getInstance().init();
    if (FAILED(err)) {
        MY_ERR("AfMgr::getInstance().init() fail\n");
        return err;
    }

    AfMgr::getInstance().setAF_IN_HSIZE();
    AfMgr::getInstance().setFlkWinConfig();

    // AWB init
    err = AwbMgr::getInstance().camcorderPreviewInit(i4SensorDev, rParam);
    if (FAILED(err)) {
        MY_ERR("AwbMgr::getInstance().camcorderPreviewInit() fail\n");
        return err;
    }

    // AAO DMA / state enable
    err = BufMgr::getInstance().DMAInit(camdma2type<ECamDMA_AAO>());
    if (FAILED(err)) {
        MY_ERR("BufMgr::getInstance().DMAInit(ECamDMA_AAO) fail\n");
        return err;
    }

    err = BufMgr::getInstance().AAStatEnable(MTRUE);
    if (FAILED(err)) {
        MY_ERR("BufMgr::getInstance().AAStatEnable(MTRUE) fail\n");
        return err;
    }

    // AFO DMA / state enable
    err = BufMgr::getInstance().DMAInit(camdma2type<ECamDMA_AFO>());
    if (FAILED(err)) {
        MY_ERR("BufMgr::getInstance().DMAInit(ECamDMA_AFO) fail\n");
        return err;
    }

    err = BufMgr::getInstance().AFStatEnable(MTRUE);
    if (FAILED(err)) {
        MY_ERR("BufMgr::getInstance().AFStatEnable(MTRUE) fail\n");
        return err;
    }

    // Reset frame count to -2
    resetFrameCount();

    // State transition: eState_Capture --> eState_CamcorderPreview
    transitState(eState_Capture, eState_CamcorderPreview);

    return  S_3A_OK;
}



