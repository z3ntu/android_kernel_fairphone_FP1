/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
#define LOG_TAG "FLICKER"

#include <stdlib.h>
#include <stdio.h>
#include <utils/threads.h>
#include <cutils/log.h>
#include "CamTypes.h"
#include <asm/arch/mt6589_sync_write.h> // For dsb() in isp_reg.h.
#include "isp_reg.h"
#include "isp_drv.h"
#include "aaa_hal_base.h"
#include "eis_hal_base.h"

#include "camera_custom_nvram.h"

#include "CamDefs.h"
#include "awb_param.h"
#include "ae_param.h"
#include "af_param.h"
#include "camera_custom_AEPlinetable.h"
#include "dbg_aaa_param.h"
#include "dbg_flicker_param.h"
#include "dbg_ae_param.h"
#include "ae_mgr.h"

#include "FlickerDetection.h"
//#include "isp_sysram_drv.h"
#include "sensor_drv.h"

#include "isp_mgr.h"
#include "mcu_drv.h"
#include "af_mgr.h"
#include "camera_custom_flicker.h"
//#include <content/IContentManager.h>
#include "flicker_hal.h"


//marine mark
#define FLICKER_DEBUG

#define FLICKER_MAX_LENG  2048
#define MHAL_FLICKER_WORKING_BUF_SIZE (FLICKER_MAX_LENG*4*3)    // flicker support max size
#define FLICKER_SUPPORT_MAX_SIZE (2500*2*3)
#define FLICKER_EIS_ErrCount_Thre  300
#define FLICKER_EIS_ErrCount_Thre2  150

#ifdef FLICKER_DEBUG
#include <string.h>
#include <cutils/xlog.h>
#define LOGD(fmt, arg...)  XLOGD(fmt, ##arg)

#define FLICKER_HAL_TAG             "[FLK Hal] "
#define FLICKER_LOG(fmt, arg...)    LOGD(FLICKER_HAL_TAG fmt, ##arg)
#define FLICKER_ERR(fmt, arg...)    //LOGE(FLICKER_HAL_TAG "Err: %5d: "fmt, __LINE__, ##arg)
#else
#define FLICKER_LOG(a,...)
#define FLICKER_ERR(a,...)
#endif


using namespace NSCamCustom;

#define FLK_DBG_LOG(fmt, arg...)    XLOGD(FLICKER_HAL_TAG fmt, ##arg)

/*
*
*/
/*******************************************************************************
*
********************************************************************************/
FlickerHalBase* FlickerHalBase::createInstance()
{
FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);
	FlickerHalBase* pObj;
	pObj = FlickerHal::getInstance();
    return pObj;
}

FlickerHalBase* FlickerHalBase::getInstance()
{
FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);

    return FlickerHal::getInstance();
}

FlickerHalBase* FlickerHal::getInstance()
{
FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);
    static FlickerHal singleton;
    return &singleton;
}
/*******************************************************************************
*
********************************************************************************/
MINT32 FlickerHal::open(MINT32 i4SensorDev)
{
	FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);
    if (init(i4SensorDev) != 0)  {
        FLICKER_LOG("singleton.init() fail \n");
        return -1;
    }
    return 0;
}

void FlickerHal::close()
{
	FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);
	uninit();
}

MINT32 FlickerHal::createBuf()
{
	FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);

		m_pVectorData1 = (MINT32*)malloc(MHAL_FLICKER_WORKING_BUF_SIZE);
		if(m_pVectorData1 == NULL)
		{
			FLICKER_LOG("memory1 is not enough");
			return -1;
		}

		m_pVectorData2 = (MINT32*)malloc(MHAL_FLICKER_WORKING_BUF_SIZE);
		if(m_pVectorData2 == NULL)
		{
			FLICKER_LOG("memory2 is not enough");
			return -1;
		}


		mpIMemDrv = IMemDrv::createInstance();
		if(mpIMemDrv == NULL)
		{
			FLICKER_LOG("mpIMemDrv is NULL");
			return -1;
		}

		for(int i = 0; i < 2; i++)
		{
			flkbufInfo[i].size = flkbufInfo[i].virtAddr = flkbufInfo[i].phyAddr = 0;
			flkbufInfo[i].memID = -1;
		}

		if(!mpIMemDrv->init())
		{
			FLICKER_LOG(" mpIMemDrv->init() error");
			return -1;
		}

		for(int i = 0; i < 2; i++)
		{
			flkbufInfo[i].size = FLICKER_SUPPORT_MAX_SIZE;
			if(mpIMemDrv->allocVirtBuf(&flkbufInfo[i]) < 0)
			{
				FLICKER_LOG("[init] mpIMemDrv->allocVirtBuf():%d, error",i);
				return -1;

			}
			if(mpIMemDrv->mapPhyAddr(&flkbufInfo[i]) < 0)
			{
				FLICKER_LOG("[createMemBuf] mpIMemDrv->mapPhyAddr() error, i(%d)\n",i);
				if (mpIMemDrv->freeVirtBuf(&flkbufInfo[i]) < 0)
				{
					FLICKER_LOG("[destroyMemBuf] mpIMemDrv->freeVirtBuf() error, i(%d)\n",i);
				}

				return -1;
			}

			//buffer address need to align
		}

    return 0;
}
MVOID FlickerHal::releaseBuf()
{
	FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);

		if(m_pVectorData1 != NULL)
		{
			free(m_pVectorData1);
			m_pVectorData1 = NULL;
		}

		if(m_pVectorData2 != NULL)
		{
			free(m_pVectorData2);
			m_pVectorData2 = NULL;
		}

		if(mpIMemDrv)
		{
			for(MINT32 i = 0; i < 2; ++i)
			{
				if(0 == flkbufInfo[i].virtAddr)
				{
					FLICKER_LOG("[destroyMemBuf] Buffer doesn't exist, i(%d)\n",i);
					continue;
				}

				if(mpIMemDrv->unmapPhyAddr(&flkbufInfo[i]) < 0)
				{
					FLICKER_LOG("[destroyMemBuf] mpIMemDrv->unmapPhyAddr() error, i(%d)\n",i);
				}

				if (mpIMemDrv->freeVirtBuf(&flkbufInfo[i]) < 0)
				{
					FLICKER_LOG("[destroyMemBuf] mpIMemDrv->freeVirtBuf() error, i(%d)\n",i);
				}
			}
			mpIMemDrv->uninit();
			mpIMemDrv->destroyInstance();
		}
}
/*******************************************************************************
*
********************************************************************************/
void FlickerHal::destroyInstance()
{
	FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);

}

/*******************************************************************************
*
********************************************************************************/
FlickerHal::FlickerHal()
{
	FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);
	MINT8 i;

    //FLICKER_LOG("FlickerHal() \n");

    m_bFlickerEnable = MFALSE;
    m_bFlickerEnablebit = MFALSE;
    m_u4SensorPixelClkFreq = 0;
    m_u4FreqFrame = 0;
    m_u4Freq000 = 0;
    m_u4Freq100 = 0;
    m_u4Freq120 = 0;
    m_flickerStatus = INCONCLUSIVE;
    m_EIS_LMV_Flag = SMALL_MOTION;
   // m_pFlickerSysram = NULL;
    m_pSensorDrv = NULL;
    m_pIspDrv = NULL;
	m_pIspRegMap=NULL;
    m_u4FlickerFreq = HAL_FLICKER_AUTO_50HZ;
    m_u4FlickerWidth = 0;
    m_u4FlickerHeight = 0;
	m_FlickerMotionErrCount=0;
	m_FlickerMotionErrCount2=0;
	m_bPause=0;



	mai4GMV_X=0;
	mai4GMV_Y=0;

    for(i=0; i<8; i++) {
    	m_vAMDF[i] = 0;
    }
}

/*******************************************************************************
*
********************************************************************************/
FlickerHal::~FlickerHal()
{
	FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);
}

/*******************************************************************************
*
********************************************************************************/
MINT32 FlickerHal::init(MINT32 i4SensorDev)
{
	FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);
    MINT32 err = 0;
//    NSCamCustom::FlickerThresholdSetting_T strFlickerThreshold;
	FLKThreSetting_T strFlickerThreshold;
    FLICKER_LOG("init - mUsers: %d \n", mUsers);
    Mutex::Autolock lock(mLock);
	m_SensorDev=i4SensorDev;
    if (mUsers > 0) {
        FLICKER_LOG("%d has created \n", mUsers);
        android_atomic_inc(&mUsers);
        return 0;
    }


#if 1
	//EIS
    mpEisHal = EisHalBase::createInstance("AutoFliker");
	if (!mpEisHal) {
		FLICKER_LOG("createInstance mpEisHal fail \n");
		goto create_fail_exit;
	}
#endif
    //sensor driver
    m_pSensorDrv = SensorDrv::createInstance(i4SensorDev);
    if (!m_pSensorDrv) {
        FLICKER_LOG("createInstance SensorDrv fail \n");
        goto create_fail_exit;
    }

   // marine , need to modif sensor type
    err = m_pSensorDrv->sendCommand((SENSOR_DEV_ENUM)i4SensorDev,CMD_SENSOR_GET_PIXEL_CLOCK_FREQ, &m_u4SensorPixelClkFreq, NULL, NULL);
    if(err != 0) {
    	FLICKER_LOG("No plck. \n");
    }
    //FLICKER_LOG("[Flicker Hal]init - m_u4SensorPixelClkFreq: %d \n", m_u4SensorPixelClkFreq);


   err = m_pSensorDrv->sendCommand((SENSOR_DEV_ENUM)i4SensorDev,CMD_SENSOR_GET_FRAME_SYNC_PIXEL_LINE_NUM, &m_u4PixelsInLine, NULL, NULL);
    if(err != 0) {
    	FLICKER_LOG("No pixels per line. \n");
    }

    m_u4PixelsInLine &= 0x0000FFFF;

    // Create isp driver
    m_pIspDrv = IspDrv::createInstance();

    if (!m_pIspDrv) {
        FLICKER_LOG("createInstance IspDrv fail \n");
        goto create_fail_exit;
    }
	else
	{
       if( m_pIspDrv->init()<0)
       {

	   	FLICKER_LOG("ISP init fail \n");
        goto create_fail_exit;
       }
	   else
	   {
		   m_pIspRegMap=( isp_reg_t*)m_pIspDrv->getRegAddr();

	   }


    }

    android_atomic_inc(&mUsers);

    m_pVectorAddress1 = NULL;




    m_u4FlickerWidth = 0;
    m_u4FlickerHeight = 0;

#if 0
    m_pVectorData1 = (MINT32*)malloc(MHAL_FLICKER_WORKING_BUF_SIZE);
    if(m_pVectorData1 == NULL)
    {
        FLICKER_LOG("memory1 is not enough");
        return -2;
    }

    m_pVectorData2 = (MINT32*)malloc(MHAL_FLICKER_WORKING_BUF_SIZE);
    if(m_pVectorData2 == NULL)
    {
        FLICKER_LOG("memory2 is not enough");
        return -3;
    }


	mpIMemDrv = IMemDrv::createInstance();
	if(mpIMemDrv == NULL)
	{
		FLICKER_LOG("mpIMemDrv is NULL");
		goto create_fail_exit;
	}

	for(int i = 0; i < 2; i++)
	{
		flkbufInfo[i].size = flkbufInfo[i].virtAddr = flkbufInfo[i].phyAddr = 0;
		flkbufInfo[i].memID = -1;
	}

	if(!mpIMemDrv->init())
    {
        FLICKER_LOG(" mpIMemDrv->init() error");
        goto create_fail_exit;
    }

	for(int i = 0; i < 2; i++)
	{
		flkbufInfo[i].size = FLICKER_SUPPORT_MAX_SIZE;
		if(mpIMemDrv->allocVirtBuf(&flkbufInfo[i]) < 0)
		{
			FLICKER_LOG("[init] mpIMemDrv->allocVirtBuf():%d, error",i);
			goto create_fail_exit;

		}
		if(mpIMemDrv->mapPhyAddr(&flkbufInfo[i]) < 0)
		{
			FLICKER_LOG("[createMemBuf] mpIMemDrv->mapPhyAddr() error, i(%d)\n",i);
            if (mpIMemDrv->freeVirtBuf(&flkbufInfo[i]) < 0)
            {
                FLICKER_LOG("[destroyMemBuf] mpIMemDrv->freeVirtBuf() error, i(%d)\n",i);
            }

			goto create_fail_exit;
		}

		//buffer address need to align
	}

#endif
   /////////////////////////////
   //marine test
   getFlickerThresPara(eFLKSpeed_Normal,&strFlickerThreshold);
   setFlickerThresholdParams(&strFlickerThreshold);


   //configure flicker window , it can be a fixed window
	setWindowInfo();

	//enableFlickerDetection(1);



    return err;

create_fail_exit:

    if (m_pSensorDrv) {
        m_pSensorDrv->destroyInstance();
        m_pSensorDrv = NULL;
    }

    if (m_pIspDrv) {
        m_pIspDrv->destroyInstance();
        m_pIspDrv = NULL;
    }

    return -1;
}
/*******************************************************************************
*
********************************************************************************/

MVOID* FlickerHal::uninitThread(MVOID* arg)
{
	FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);
	MBOOL	ret = MTRUE;

	FlickerHal *self = (FlickerHal*)arg;

    FLICKER_LOG("uninit Thread\n");
	self->uninit();

	pthread_detach(pthread_self());

	return (MVOID*)ret;



}


/*******************************************************************************
*
********************************************************************************/
MINT32 FlickerHal::uninit()
{
	FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);
    MINT32 err = 0;
    MINT32 i4PollingTime = 10, i4Index;
    MINT32 i4FlickerStatus;

    FLICKER_LOG("uninit - mUsers: %d \n", mUsers);

    if (mUsers <= 0) {
        // No more users
        return 0;
    }

    Mutex::Autolock lock(mLock);



    // More than one user
    android_atomic_dec(&mUsers);

   if (mUsers == 0) {




	//setFlickerDrv(0);
	enableFlickerDetection(0);


	 if (m_pSensorDrv) {
		 //marine mark for testing
		m_pSensorDrv->destroyInstance();
		m_pSensorDrv = NULL;
	}
#if  1
	 if (mpEisHal) {
         mpEisHal->destroyInstance("AutoFliker");
		 mpEisHal = NULL;
	 }
#endif
	if (m_pIspDrv) {
		m_pIspDrv->uninit();
		m_pIspDrv->destroyInstance();
		m_pIspDrv = NULL;
	}

#if 0

	if(m_pVectorData1 != NULL)
	{
		free(m_pVectorData1);
		m_pVectorData1 = NULL;
	}

	if(m_pVectorData2 != NULL)
	{
		free(m_pVectorData2);
		m_pVectorData2 = NULL;
	}

	if(mpIMemDrv)
	{
		for(MINT32 i = 0; i < 2; ++i)
		{
			if(0 == flkbufInfo[i].virtAddr)
			{
				FLICKER_LOG("[destroyMemBuf] Buffer doesn't exist, i(%d)\n",i);
				continue;
			}

			if(mpIMemDrv->unmapPhyAddr(&flkbufInfo[i]) < 0)
			{
				FLICKER_LOG("[destroyMemBuf] mpIMemDrv->unmapPhyAddr() error, i(%d)\n",i);
			}

			if (mpIMemDrv->freeVirtBuf(&flkbufInfo[i]) < 0)
			{
				FLICKER_LOG("[destroyMemBuf] mpIMemDrv->freeVirtBuf() error, i(%d)\n",i);
			}
		}
		mpIMemDrv->uninit();
		mpIMemDrv->destroyInstance();
	}
#endif

    }
    else {
        FLICKER_LOG("Still %d users \n", mUsers);
    }
	//FLICKER_LOG("[uninit] exit\n");

    return 0;
}



/*******************************************************************************
*
********************************************************************************/
MINT32 FlickerHal::setFlickerDrv(
    MBOOL flicker_en)
{
	FLK_DBG_LOG("FFLK func=%s line=%d flicker_en=%d",__FUNCTION__, __LINE__,(int)flicker_en);
    int ret = 0;


    if(flicker_en == 1) {  // enable flicker

    //set flk mode as column vector output
		ISP_WRITE_BITS(m_pIspRegMap,CAM_FLK_CON,FLK_MODE,0);
    // FLK enable
		//ISP_WRITE_ENABLE_BITS(m_pIspRegMap,CAM_CTL_EN1,FLK_EN,1);
	//FLK enable set
		ISP_WRITE_ENABLE_BITS(m_pIspRegMap,CAM_CTL_EN1_SET,FLK_EN_SET,1);
		ISP_WRITE_ENABLE_BITS(m_pIspRegMap,CAM_CTL_EN1_CLR,FLK_EN_CLR,0);
    // FLK DMA enable
	//	ISP_WRITE_ENABLE_BITS(m_pIspRegMap,CAM_CTL_DMA_EN,ESFKO_EN,1);
	//	ISP_WRITE_ENABLE_BITS(m_pIspRegMap,CAM_CTL_DMA_EN_SET,ESFKO_EN_SET,1);
	// FLK DMA Done interrupt Enable
		//ISP_WRITE_BITS(m_pIspRegMap,CAM_CTL_DMA_INT,ESFKO_DONE_EN,1);
		ISP_WRITE_BITS(m_pIspRegMap,CAM_CTL_INT_EN,FLK_DON_EN,1) ;




		mpIMemDrv->cacheFlushAll();
    } else {   // disable flicker

			// disable flk
			//ISP_WRITE_ENABLE_BITS(m_pIspRegMap,CAM_CTL_EN1,FLK_EN,0);
 			//disable flk dma
			//ISP_WRITE_ENABLE_BITS(m_pIspRegMap,CAM_CTL_DMA_EN,ESFKO_EN,0);
			//FLK enable set
			ISP_WRITE_ENABLE_BITS(m_pIspRegMap,CAM_CTL_EN1_SET,FLK_EN_SET,0);
			ISP_WRITE_ENABLE_BITS(m_pIspRegMap,CAM_CTL_EN1_CLR,FLK_EN_CLR,1);
			//disable flk dma done interrupt
			//ISP_WRITE_BITS(m_pIspRegMap,CAM_CTL_DMA_INT,ESFKO_DONE_EN,0);

#if 1
		// enable ESFKO done
		//ISP_WRITE_ENABLE_BITS(m_pIspRegMap , CAM_CTL_INT_EN, FLK_DON_EN, 1);

		// wait FLK  done

/*
		ISP_DRV_WAIT_IRQ_STRUCT WaitIrq;
		WaitIrq.Clear = ISP_DRV_IRQ_CLEAR_WAIT;
		WaitIrq.Type = ISP_DRV_IRQ_TYPE_INT;
		WaitIrq.Status = ISP_DRV_IRQ_INT_STATUS_FLK_DON_ST;
		WaitIrq.Timeout = 400; // 400 ms

		m_pIspDrv->waitIrq(WaitIrq);*/
		//usleep(100000);

		FLK_DBG_LOG("FFLK func=%s line=%d ",__FUNCTION__, __LINE__);


		//ISP_WRITE_BITS(m_pIspRegMap,CAM_CTL_INT_EN,FLK_DON_EN,0) ;
#endif


    }






    return ret;
}

/*******************************************************************************
*
********************************************************************************/
MINT32 FlickerHal::setFlickerWinConfig(FLKWinCFG_T* ptFlkWinCfg)
{
	FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);
    int ret = 0;

	ISP_WRITE_BITS(m_pIspRegMap,CAM_FLK_WNUM,FLK_WNUM_X,ptFlkWinCfg->m_u4NumX);
	ISP_WRITE_BITS(m_pIspRegMap,CAM_FLK_WNUM,FLK_WNUM_Y,ptFlkWinCfg->m_u4NumY);
	ISP_WRITE_BITS(m_pIspRegMap,CAM_FLK_SOFST,FLK_SOFST_X,ptFlkWinCfg->m_u4OffsetX);
	ISP_WRITE_BITS(m_pIspRegMap,CAM_FLK_SOFST,FLK_SOFST_Y,ptFlkWinCfg->m_u4OffsetY);
	ISP_WRITE_BITS(m_pIspRegMap,CAM_FLK_WSIZE,FLK_WSIZE_X,ptFlkWinCfg->m_u4SizeX);
	ISP_WRITE_BITS(m_pIspRegMap,CAM_FLK_WSIZE,FLK_WSIZE_Y,ptFlkWinCfg->m_u4SizeY);


//FLICKER_LOG("[setFlickerConfig]:flicker win No.=0x%08x\n", (int) ISP_READ_REG(m_pIspRegMap, CAM_FLK_WNUM));
FLICKER_LOG("[setFlickerConfig]:flicker win (x,y)=0x%08x\n", (int) ISP_READ_REG(m_pIspRegMap, CAM_FLK_SOFST));
FLICKER_LOG("[setFlickerConfig]:flicker win (w,h)=0x%08x\n", (int) ISP_READ_REG(m_pIspRegMap, CAM_FLK_WSIZE));

    return ret;
}

/*******************************************************************************
*
********************************************************************************/
MINT32 FlickerHal::setFlickerDMAConfig(
    unsigned long flicker_DMA_address ,MINT32 DMASize )
{
	FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);
    int ret = 0;

    ISP_WRITE_REG(m_pIspRegMap, CAM_ESFKO_XSIZE, DMASize);
    ISP_WRITE_REG(m_pIspRegMap, CAM_ESFKO_YSIZE, 0);
    ISP_WRITE_REG(m_pIspRegMap, CAM_ESFKO_STRIDE, DMASize);

    ISP_WRITE_REG(m_pIspRegMap, CAM_ESFKO_BASE_ADDR,flicker_DMA_address);
    ISP_WRITE_REG(m_pIspRegMap, CAM_ESFKO_OFST_ADDR,0);

    //FLICKER_LOG("ESFKO X size:0x%08x \n", ISP_READ_REG(m_pIspRegMap, CAM_ESFKO_XSIZE));
    //FLICKER_LOG("CAM_ESFKO_STRIDE:0x%08x \n",  ISP_READ_REG(m_pIspRegMap, CAM_ESFKO_STRIDE));
    //FLICKER_LOG("[setFlickerDMAConfig]:CAM_ESFKO_BASE_ADDR:0x%08x\n", ISP_READ_REG(m_pIspRegMap, CAM_ESFKO_BASE_ADDR));
    return ret;
}

MUINT32 FlickerHal:: GetFlicker_CurrentDMA()
{
	FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);
	return (MUINT32)ISP_READ_REG(m_pIspRegMap, CAM_ESFKO_BASE_ADDR) ;
}

/*******************************************************************************
*
********************************************************************************/
MBOOL FlickerHal::Updated()
	{
		FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);
		//Mutex::Autolock lock(mLock);
		//
		MBOOL	ret = MFALSE;
		MINT32	err = 0;
		MINT32	i4DetectedResult = -1;
        MINT32  _mai4GMV_X,_mai4GMV_Y;
		//
		//	(1) Bypass if auto detection is disable.
		if	( ! m_bFlickerEnable )
		{
			ret = MTRUE;
			goto lbExit;
		}
		//
		//	(2) Update EIS/AAA Info.
#if 1
		if	(
				! updateEISInfo()
			||	! updateAAAInfo()
			)
		{
			goto lbExit;
		}
#endif
		//
		//	(3) Analyze the flicker by passing EIS information.
        _mai4GMV_X=static_cast<MINT32>(mai4GMV_X);
        _mai4GMV_Y=static_cast<MINT32>(mai4GMV_Y);

        err = analyzeFlickerFrequency(1, &_mai4GMV_X, &_mai4GMV_Y, mai4AFWin);
		if	(err)
		{
			FLICKER_LOG("Updated] mpFlickerHal->analyzeFlickerFrequency() - (err)=(%x)",  err);
			goto lbExit;
		}
		//
		//	(4) Get the flicker result from flicker hal
		err = getFlickerStatus(&i4DetectedResult);
		//FLICKER_LOG("Updated] mi4DetectedResult=(%d)",  i4DetectedResult);
		if	(err)
		{
			FLICKER_LOG("[Updated] mpFlickerHal->getFlickerStatus() - (err)=(%d,%x)", err);
			goto lbExit;
		}
		//
		//	(5) Debug info.
		//marine , need to modify "m_bFlickerModeChange"
		if	( mi4DetectedResult != i4DetectedResult || m_bFlickerModeChange )
		{
			FLICKER_LOG("[Updated] detected result:(old, new)=(%d, %d)",  mi4DetectedResult, i4DetectedResult);
			m_bFlickerModeChange = MFALSE;
		}
		mi4DetectedResult = i4DetectedResult;
		//
		//	(6) Pass the flicker result to Hal 3A.
        if(i4DetectedResult==HAL_FLICKER_AUTO_OFF)
        {

            FLICKER_LOG("[Updated]i4DetectedResult==HAL_FLICKER_AUTO_OFF");
            goto lbExit;
        }
        else if(i4DetectedResult==HAL_FLICKER_AUTO_50HZ)
        {
             NS3A::AeMgr::getInstance().setAEAutoFlickerMode(0);
        }
        else if(i4DetectedResult==HAL_FLICKER_AUTO_60HZ)
        {
            NS3A::AeMgr::getInstance().setAEAutoFlickerMode(1);
        }

		if	(err)
		{
			FLICKER_LOG("[Updated] set3AParam(HAL_3A_AE_FLICKER_AUTO_MODE) - (err)=(%x)", err);
			goto lbExit;
		}

		ret = MTRUE;
	lbExit:
		return	ret;
	}

/*******************************************************************************
*
********************************************************************************/

MBOOL FlickerHal::sendCommand(FLKCmd_T eCmd ,void* pi4Arg)
{
	FLK_DBG_LOG("FFLK func=%s line=%d cmd=%d",__FUNCTION__, __LINE__, (int)eCmd);

    MBOOL ret = MFALSE;
    Mutex::Autolock lock(mLock);

	if(eCmd==FLKCmd_Update)
	{


		ret=Updated();
	}
	else if(eCmd==FLKCmd_GetDimenion)
	{
		pi4Arg=(void*)&strFlkWinCfg ;
	}
	else if(eCmd==FLKCmd_SetFLKMode)
	{
		//AutoDetectEnable(1);

	}
	else if(eCmd==FLKCmd_SetWindow)
	{

	}
	else if(eCmd==FLKCmd_FlkEnable)
	{
		if(!m_bPause)
		enableFlickerDetection(1);
	}
	else if(eCmd==FLKCmd_FlkDISable)
	{
		enableFlickerDetection(0);
	}
	else if(eCmd==FLKCmd_FlkPause)
	{
		if(m_bPause ||(!m_bFlickerEnable))
			return ret;
		m_bPause=1;
		enableFlickerDetection(0);
	}
	else if(eCmd==FLKCmd_FlkResume)
	{
		if((!m_bPause) ||m_bFlickerEnable)
			return ret;
		m_bPause=0;
		enableFlickerDetection(1);
	}

	else if(eCmd==FLKCmd_Uninit)
	{
	pthread_create(&mUninitThread, NULL, FlickerHal::uninitThread, this);
		//uninit();
	}

    return ret;
}

/*******************************************************************************
*
********************************************************************************/
MINT32 FlickerHal::analyzeFlickerFrequency(MINT32 i4LMVcnt, MINT32 *i4LMV_x, MINT32 *i4LMV_y, MINT64 *i4vAFstatisic)
{
	FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);
//MINT32 i4LMV_x[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, i4LMV_y[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MINT64 i4vAFstat[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
MINT32 i4FlickerStatus;
MINT32 i,i4Buff_idx=0;
    MINT64 *i4vAFInfo = NULL;
MINT32 i4DataLen = 0;
MUINT32 u4Height;
MINT32 *m_FickerSW_buff_1;
MINT32 *m_FickerSW_buff_2;



    if(m_bFlickerEnable)
    {
        i4FlickerStatus = ISP_READ_REG(m_pIspRegMap, CAM_CTL_INT_STATUSX);

    	 if(ISP_READ_BITS(m_pIspRegMap,CAM_CTL_INT_STATUSX,ESFKO_ERR_ST)||ISP_READ_BITS(m_pIspRegMap,CAM_CTL_INT_STATUSX,FLK_ERR_ST)) {
           // FLICKER_ERR("[Flicker] The Flicker status error 0x%08x\n", i4FlickerStatus);
            FLICKER_LOG("[analyzeFlickerFrequency] The Flicker status error 0x%08x\n", i4FlickerStatus);
    	 }


        if(1)//(ISP_READ_BITS(m_pIspRegMap,CAM_CTL_DMA_INT,ESFKO_DONE_ST)) { // check the flicker DMA done status
        {

		   // get the AF statistic information
           if(i4vAFstatisic == NULL) {
           	i4vAFInfo = &i4vAFstat[0];
           } else {
              i4vAFInfo = i4vAFstatisic;
           }

		   if(GetFlicker_CurrentDMA()==flkbufInfo[0].phyAddr)
		   {
			   	i4Buff_idx=0;
				m_FickerSW_buff_1=m_pVectorData1;  //m_FickerSW_buff_1 is n-1 data
				m_FickerSW_buff_2=m_pVectorData2;  // m_FickerSW_buff_1 is n-2 data
		   }
		   else
		   {
			   	i4Buff_idx=1;
				m_FickerSW_buff_1=m_pVectorData2;  //m_FickerSW_buff_1 is n-1 data
				m_FickerSW_buff_2=m_pVectorData1;  //m_FickerSW_buff_2 is n-2 data
		   }


			m_pVectorAddress1 = (MINT32 *) (8*((flkbufInfo[(i4Buff_idx+1)%2].virtAddr + 7)/8));

           // get the EIS LMV information
           setLMVcnt(i4LMVcnt);

           if(m_u4FlickerHeight > FLICKER_MAX_LENG) {
           	i4DataLen = 3*FLICKER_MAX_LENG /2 ;
           	u4Height = FLICKER_MAX_LENG;
           } else {
           	i4DataLen = 3*m_u4FlickerHeight /2 ;
           	u4Height = m_u4FlickerHeight;
           }
               for(i=0; i<i4DataLen; i++) {
                   m_FickerSW_buff_1[2*i+0] = m_pVectorAddress1[i] &0x0000FFFF;
                   m_FickerSW_buff_1[2*i+1] =(m_pVectorAddress1[i] &0xFFFF0000)>>16;
               }
		/*	FLICKER_LOG("PGN:0x%x ,GMR:0x%x	", ISP_READ_REG(m_pIspRegMap, CAM_SGG_PGN), ISP_READ_REG(m_pIspRegMap, CAM_SGG_GMR));
			for(i=0;i<60;i++)
				{
				FLICKER_LOG("%d: %x  ,%x ,%x ,%x ,%x ,%x ,  %x ,%x  , %x , %x , \n"
			 ,i*10,m_pVectorAddress1[0+3*0+i*30],
				   m_pVectorAddress1[0+3*1+i*30],
				   m_pVectorAddress1[0+3*2+i*30],
				   m_pVectorAddress1[0+3*3+i*30],
				   m_pVectorAddress1[0+3*4+i*30],
				   m_pVectorAddress1[0+3*5+i*30],
				   m_pVectorAddress1[0+3*6+i*30],
				   m_pVectorAddress1[0+3*7+i*30],
				   m_pVectorAddress1[0+3*8+i*30],
				   m_pVectorAddress1[0+3*9+i*30]);
				}*/
			//switch FKO dst add.  to another buffer
   		   setFlickerDMAConfig(flkbufInfo[(i4Buff_idx+1)%2].phyAddr,FLK_DMA_Size);

		  //to avoid EIS treat banding as big motion
		   if(abs(*i4LMV_x)+abs(*i4LMV_y)>26)
		   {
				if(m_FlickerMotionErrCount<FLICKER_EIS_ErrCount_Thre)
		   			m_FlickerMotionErrCount++;
		   }
		   else
		   {
			   if(m_FlickerMotionErrCount>0)
			   		m_FlickerMotionErrCount--;
			   if(m_FlickerMotionErrCount<(FLICKER_EIS_ErrCount_Thre-10))
			   {
					m_FlickerMotionErrCount2=0;
					//FLICKER_LOG("flicker-EIS error count --reset count 2\n");
			   }

		   }
		  // FLICKER_LOG("EIS (ErrCount,ErrCount2)=(%d,%d)\n",m_FlickerMotionErrCount,m_FlickerMotionErrCount2);

		   if(m_FlickerMotionErrCount==FLICKER_EIS_ErrCount_Thre)
		   {
			   FLICKER_LOG("FLK-EIS count full:%d\n",m_FlickerMotionErrCount2);
			   *i4LMV_x=0;
			   *i4LMV_y=0;

			   if(m_FlickerMotionErrCount2<FLICKER_EIS_ErrCount_Thre2)
			  	 m_FlickerMotionErrCount2++;
			   else
			   {
				   m_FlickerMotionErrCount=0;
				   m_FlickerMotionErrCount2=0;
				   FLICKER_LOG("FLK-EIS count full--reset\n");

			   }


		   }

           m_flickerStatus = detectFlicker_SW(m_u4FlickerWidth, u4Height, i4LMV_x, i4LMV_y, 13, &m_EIS_LMV_Flag, i4vAFInfo, m_FickerSW_buff_1, m_FickerSW_buff_2, m_u4SensorPixelClkFreq, m_u4PixelsInLine, m_vAMDF, m_flickerStatus, &m_u4FreqFrame, &m_u4Freq000, &m_u4Freq100, &m_u4Freq120);
           if(m_flickerStatus == FK100) {
		   	 if(m_u4FlickerFreq==HAL_FLICKER_AUTO_60HZ)
		   	 {
			 	m_FlickerMotionErrCount=0;
				m_FlickerMotionErrCount2=0;
			 }
           	 m_u4FlickerFreq = HAL_FLICKER_AUTO_50HZ;
           } else if (m_flickerStatus == FK120) {
			   if(m_u4FlickerFreq==HAL_FLICKER_AUTO_50HZ)
			   {
				  m_FlickerMotionErrCount=0;
				  m_FlickerMotionErrCount2=0;
			   }

               m_u4FlickerFreq = HAL_FLICKER_AUTO_60HZ;
           }
          // FLICKER_LOG("Status:%d,%d,%d,%d,%d,%d, %d\n", m_EIS_LMV_Flag, m_flickerStatus, m_u4FreqFrame, m_u4Freq000, m_u4Freq100, m_u4Freq120, m_u4FlickerFreq);
		  //  FLICKER_LOG("AMDF : %d,%d,%d,%d,%d,%d,%d,%d, LMV:%d %d\n", m_vAMDF[0], m_vAMDF[1], m_vAMDF[2], m_vAMDF[3], m_vAMDF[4], m_vAMDF[5], m_vAMDF[6], m_vAMDF[7], i4LMV_x[0],  i4LMV_y[0]);



       // output result to log files
//           FLICKER_LOG("AF vector : %d,%d,%d,%d,%d,%d,%d,%d,%d\n", i4vAFInfo[0], i4vAFInfo[1], i4vAFInfo[2], i4vAFInfo[3], i4vAFInfo[4], i4vAFInfo[5], i4vAFInfo[6], i4vAFInfo[7], i4vAFInfo[8]);
      //     FLICKER_LOG("CAM_FLK_CON:0x%08x CAM_FLK_INTVL:0x%08x CAM_FLK_GADDR:0x%08x\n", (int) ISP_REG(m_pIspRegMap, CAM_FLK_CON), (int) ISP_REG(m_pIspRegMap, CAM_FLK_INTVL), (int) ISP_REG(m_pIspRegMap, CAM_FLK_GADDR));
      //     FLICKER_LOG("CAM_AFWIN0:0x%08x CAM_AFWIN1:0x%08x CAM_AFWIN2:0x%08x\n", (int) ISP_REG(m_pIspRegMap, CAM_AFWIN0), (int) ISP_REG(m_pIspRegMap, CAM_AFWIN1), (int) ISP_REG(m_pIspRegMap, CAM_AFWIN2));
      //     FLICKER_LOG("CAM_AFWIN3:0x%08x CAM_AFWIN4:0x%08x CAM_AFWIN5:0x%08x\n", (int) ISP_REG(m_pIspRegMap, CAM_AFWIN3), (int) ISP_REG(m_pIspRegMap, CAM_AFWIN4), (int) ISP_REG(m_pIspRegMap, CAM_AFWIN5));
      //     FLICKER_LOG("CAM_AFWIN6:0x%08x CAM_AFWIN7:0x%08x CAM_AFWIN8:0x%08x\n", (int) ISP_REG(m_pIspRegMap, CAM_AFWIN6), (int) ISP_REG(m_pIspRegMap, CAM_AFWIN7), (int) ISP_REG(m_pIspRegMap, CAM_AFWIN8));
//           FLICKER_LOG("LMV_x:%d,LMV_y:%d\n",  i4LMV_x[0],  i4LMV_y[0]);
//           FLICKER_LOG("%d,%d,%d,%d,%d,%d,%d,%d, \n",  i4LMV_x[0],  i4LMV_y[0],  i4LMV_x[1],  i4LMV_y[1],  i4LMV_x[2],  i4LMV_y[2],  i4LMV_x[3],  i4LMV_y[3]);
//           FLICKER_LOG("%d,%d,%d,%d,%d,%d,%d,%d, \n",  i4LMV_x[4],  i4LMV_y[4],  i4LMV_x[5],  i4LMV_y[5],  i4LMV_x[6],  i4LMV_y[6],  i4LMV_x[7],  i4LMV_y[7]);
//           FLICKER_LOG("%d,%d,%d,%d,%d,%d,%d,%d, \n",  i4LMV_x[8],  i4LMV_y[8],  i4LMV_x[9],  i4LMV_y[9],  i4LMV_x[10], i4LMV_y[10], i4LMV_x[11], i4LMV_y[11]);
//           FLICKER_LOG("%d,%d,%d,%d,%d,%d,%d,%d \n", i4LMV_x[12], i4LMV_y[12], i4LMV_x[13], i4LMV_y[13], i4LMV_x[14], i4LMV_y[14], i4LMV_x[15], i4LMV_y[15]);

        }else {
            if(m_bFlickerEnable) {
               setFlickerDrv(m_bFlickerEnable);    // Save the column vector and difference
               m_bFlickerEnablebit = MFALSE;
               FLICKER_LOG("[Flicker] i4FlickerStatus:0x%08x , Enablebit:%d\n", i4FlickerStatus,  m_bFlickerEnablebit);
            }
        }
    }
    return 0;
}

/*******************************************************************************
*
********************************************************************************/
//MVOID FlickerHal::setFlickerThresholdParams(NSCamCustom::FlickerThresholdSetting_T *strFlickerThres)
MVOID FlickerHal::setFlickerThresholdParams(FLKThreSetting_T *strFlickerThres)
{
	FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);
    MINT32 threc[2] = {0, 0}, threa[2] = {0, 0}, thref[3] = {0, 0, 0};

    threc[0] = strFlickerThres->u4FlickerPoss1;
    threc[1] = strFlickerThres->u4FlickerPoss2;
    threa[0] = strFlickerThres->u4FlickerFreq1;
    threa[1] = strFlickerThres->u4FlickerFreq2;
    thref[0] = strFlickerThres->u4ConfidenceLevel1;
    thref[1] = strFlickerThres->u4ConfidenceLevel2;
    thref[2] = strFlickerThres->u4ConfidenceLevel3;

//   FLICKER_LOG("threc:%d,%d threa:%d,%d thref:%d,%d,%d, \n", threc[0], threc[1], threa[0], threa[1], thref[0], thref[1], thref[2]);

    setThreshold(threc, threa, thref);
}

/*******************************************************************************
*
********************************************************************************/
MINT32 FlickerHal::enableFlickerDetection(MBOOL bEnableFlicker)
{
	FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);
    MVOID * rPhyAddress = NULL;
    MVOID * rVirAddress = NULL;
    MINT32 i4FlickerStatus;
    MINT32 i4PollingTime = 10, i4Index;
    MINT32 ret = 0,i;	// 0: no error.

	if(bEnableFlicker==m_bFlickerEnable)
   		 return ret;
	FLICKER_LOG("[enableFlickerDetection]bEnableFlicker= %d\n",bEnableFlicker);

    m_bFlickerEnable = bEnableFlicker;
    if(m_bFlickerEnable && m_pIspDrv)
	{
		if(flkbufInfo[0].virtAddr!=0)
		{

			rPhyAddress = (MVOID*)flkbufInfo[0].phyAddr;
			rVirAddress = (MVOID*)flkbufInfo[0].virtAddr;

			/*
			FLK DMA size:
			2 bytes for per line in one window
			ESFKO_XSIZE = (FLK_WNUM_X * FLK_WNUM_Y * FLK_WSIZE_Y * 2) - 1
			ESFKO_YSIZE = 0 */
			FLK_DMA_Size=(strFlkWinCfg.m_u4NumX*strFlkWinCfg.m_u4NumY*strFlkWinCfg.m_u4SizeY*2)-1;
			setFlickerDMAConfig((MUINT32)rPhyAddress ,FLK_DMA_Size);

		}
		else
		{
			FLICKER_LOG("!!!UNABLE to update pPhyAddr,pVirAddr!!!\n");
		}


        setFlickerDrv(m_bFlickerEnable);    // Save the column vector and difference
    }
	else {
        setFlickerDrv(m_bFlickerEnable);    // disable theflicker
    }
//    FLICKER_LOG("Flicker enable:%d\n", bEnableFlicker);
    return ret;
}

/*******************************************************************************
*
********************************************************************************/
MINT32 FlickerHal::setWindowInfo()
{
	FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);
    MUINT32 u4Height;
    MUINT32 u4Width;
    MINT32 u4PixelEnd, u4PixelStart, u4LineEnd, u4LineStart;
	MUINT32 u4ToleranceLine=20;



        u4PixelEnd = ISP_READ_BITS(m_pIspRegMap, CAM_TG_SEN_GRAB_PXL, PXL_E);
        u4PixelStart =  ISP_READ_BITS(m_pIspRegMap, CAM_TG_SEN_GRAB_PXL, PXL_S);
        u4LineEnd = ISP_READ_BITS(m_pIspRegMap, CAM_TG_SEN_GRAB_LIN, LIN_E);
        u4LineStart =  ISP_READ_BITS(m_pIspRegMap, CAM_TG_SEN_GRAB_LIN, LIN_S);
        u4Width =  u4PixelEnd - u4PixelStart + 1 - 4;
        u4Height = u4LineEnd - u4LineStart + 1 -6;

            m_u4FlickerWidth = u4Width;
            m_u4FlickerHeight = u4Height-u4ToleranceLine;
            FLICKER_LOG("[setWindowInfo] width:%d ,%d height:%d ,%d\n", u4Width, m_u4FlickerWidth, u4Height, m_u4FlickerHeight);
            if(m_u4FlickerHeight > FLICKER_MAX_LENG-6){
                u4Height = FLICKER_MAX_LENG-6;
            } else {
                u4Height = m_u4FlickerHeight;
            }
			strFlkWinCfg.m_uImageW=u4Width;
			strFlkWinCfg.m_uImageH=u4Height;
			strFlkWinCfg.m_u4NumX=3;
			strFlkWinCfg.m_u4NumY=3;
			strFlkWinCfg.m_u4OffsetX=0;
			strFlkWinCfg.m_u4OffsetY=0+u4ToleranceLine;
			//strFlkWinCfg.m_u4SizeX=(u4Width-strFlkWinCfg.m_u4OffsetX)/3;
			//strFlkWinCfg.m_u4SizeY=(u4Height-strFlkWinCfg.m_u4OffsetY+u4ToleranceLine)/3;
			strFlkWinCfg.m_u4SizeX=((u4Width-strFlkWinCfg.m_u4OffsetX)/6)*2;
			strFlkWinCfg.m_u4SizeY=((u4Height-strFlkWinCfg.m_u4OffsetY+u4ToleranceLine)/6)*2;

            setFlickerWinConfig(&strFlkWinCfg);
            //FLICKER_LOG("[setWindowInfo] m_u4SizeX:%d , m_u4SizeY:%d \n", strFlkWinCfg.m_u4SizeX, strFlkWinCfg.m_u4SizeY );

		  // FLICKER_LOG("[setWindowInfo] exist window infor\n");
        return 0;
}

/*******************************************************************************
*
********************************************************************************/
MINT32 FlickerHal::getFlickerStatus(MINT32 *a_flickerStatus)
{
	FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);

    if(m_bFlickerEnable == MFALSE) {
    	*a_flickerStatus = HAL_FLICKER_AUTO_OFF;
    } else {
        *a_flickerStatus = m_u4FlickerFreq;
    }

    return 0;
}
#if 1
MBOOL
FlickerHal::
updateEISInfo()
{
	FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);
   // using namespace NSContent;
    //
    MBOOL ret = MFALSE;
   MUINT32 MV_X,MV_Y;
// MUINT32 MVCrop_w,MVCrop_h;
	//marine add
#if 1
	if (mpEisHal)
	{
		//mpEisHal->getEISResult(mai4GMV_X,floatMV_X,mai4GMV_Y,floatMV_Y,MVCrop_w,MVCrop_h);
		mpEisHal->getEISGmv(MV_X,MV_Y);
		mai4GMV_X=static_cast<MINT32>(MV_X);
		mai4GMV_Y=static_cast<MINT32>(MV_Y);
		mai4GMV_X=mai4GMV_X/256;
		mai4GMV_Y=mai4GMV_Y/256;

	}
	else
		return MFALSE;
#endif
    //
    ret = MTRUE;

    return  ret;
}

MBOOL
FlickerHal::
updateAAAInfo()
{
	FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);
    //  (1) Update AF Window Info.
#if 1
    AF_FULL_STAT_T   AFStatus;
    //AfMgr mpHal3A = AfMgr::getInstance();
    int j = 0;
    ::memset(&AFStatus, 0, sizeof(AFStatus));

    AFStatus = AfMgr::getInstance().getAFFullStat();//mpHal3A->getAFFullStat();

    for (int i = 0; i < 9; i++)
    {
        j = (i/3)*2;
        mai4AFWin[i] = AFStatus.i8StatH[(i%3)*2+j*6]+AFStatus.i8StatH[(i%3)*2+1+j*6]+AFStatus.i8StatH[(i%3)*2+(j+1)*6]+AFStatus.i8StatH[(i%3)*2+1+(j+1)*6];
    }

  //  FLICKER_LOG("[updateAAAInfo] AF Win:(%llx,%llx,%llx,%llx,%llx,%llx,%llx,%llx,%llx)\n", mai4AFWin[0], mai4AFWin[1], mai4AFWin[2], mai4AFWin[3], mai4AFWin[4], mai4AFWin[5], mai4AFWin[6], mai4AFWin[7], mai4AFWin[8]);
#endif
    return  MTRUE;
}

MBOOL
FlickerHal::
getFlickerThresPara(eFlickerDetectSpeed idx ,FLKThreSetting_T *ptFlickerThreshold)
{
	FLK_DBG_LOG("FFLK func=%s line=%d",__FUNCTION__, __LINE__);
	switch(idx)
	{
		case eFLKSpeed_Slow :
			ptFlickerThreshold->u4FlickerPoss1=9;
			ptFlickerThreshold->u4FlickerPoss2=11;
			ptFlickerThreshold->u4FlickerFreq1=35;
			ptFlickerThreshold->u4FlickerFreq2=40;
			ptFlickerThreshold->u4ConfidenceLevel1=13;
			ptFlickerThreshold->u4ConfidenceLevel2=13;
			ptFlickerThreshold->u4ConfidenceLevel3=13;

			break;
		case eFLKSpeed_Normal :
			ptFlickerThreshold->u4FlickerPoss1=9;
			ptFlickerThreshold->u4FlickerPoss2=11;
			ptFlickerThreshold->u4FlickerFreq1=35;
			ptFlickerThreshold->u4FlickerFreq2=40;
			ptFlickerThreshold->u4ConfidenceLevel1=9;
			ptFlickerThreshold->u4ConfidenceLevel2=9;
			ptFlickerThreshold->u4ConfidenceLevel3=9;

			break;
		case eFLKSpeed_Fast :
			ptFlickerThreshold->u4FlickerPoss1=9;
			ptFlickerThreshold->u4FlickerPoss2=11;
			ptFlickerThreshold->u4FlickerFreq1=34;
			ptFlickerThreshold->u4FlickerFreq2=39;
			ptFlickerThreshold->u4ConfidenceLevel1=3;
			ptFlickerThreshold->u4ConfidenceLevel2=3;
			ptFlickerThreshold->u4ConfidenceLevel3=3;
			break;


	}
	return 1;
}


#endif
