#define LOG_TAG "ResMgrDrv"
//-----------------------------------------------------------------------------
#include <utils/Errors.h>
#include <cutils/log.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <cutils/atomic.h>
#include <sys/ioctl.h>
#include <camera_pipe_mgr.h>
#include <cutils/xlog.h>
#include <utils/threads.h>
#include "bandwidth_control.h"
#include <hdmitx.h>
#include "CamTypes.h"
#include "res_mgr_drv.h"
#include "res_mgr_drv_imp.h"
//-----------------------------------------------------------------------------
ResMgrDrvImp::ResMgrDrvImp()
{
    //LOG_MSG("");
    mUser = 0;
    mFdCamPipeMgr = -1;
    mFdHdmiTx = -1;
}
//----------------------------------------------------------------------------
ResMgrDrvImp::~ResMgrDrvImp()
{
    //LOG_MSG("");
}
//-----------------------------------------------------------------------------
ResMgrDrv* ResMgrDrv::CreateInstance(void)
{
    return ResMgrDrvImp::GetInstance();
}
//-----------------------------------------------------------------------------
ResMgrDrv* ResMgrDrvImp::GetInstance(void)
{
    static ResMgrDrvImp Singleton;
    //
    //LOG_MSG("");
    //
    return &Singleton;
}
//----------------------------------------------------------------------------
MVOID ResMgrDrvImp::DestroyInstance(void) 
{
}
//----------------------------------------------------------------------------
MBOOL ResMgrDrvImp::Init(void)
{
    MBOOL Result = MTRUE;
    //
    Mutex::Autolock lock(mLock);
    //
    if(mUser == 0)
    {
        LOG_MSG("First user(%d)",mUser);
    }
    else
    {
        LOG_MSG("More user(%d)",mUser);
        android_atomic_inc(&mUser);
        goto EXIT;
    }
    //
    if(mFdCamPipeMgr < 0)
    {
        mFdCamPipeMgr = open(RES_MGR_DRV_DEVNAME_PIPE_MGR, O_RDONLY, 0);
        if(mFdCamPipeMgr < 0)
        {
            LOG_ERR("CamPipeMgr kernel open fail, errno(%d):%s",errno,strerror(errno));
            Result = MFALSE;
            goto EXIT;
        }
    }
    else
    {
        LOG_MSG("CamPipeMgr kernel is opened already");
    }
    //
    android_atomic_inc(&mUser);
    //
    EXIT:
    return Result;
}
//----------------------------------------------------------------------------
MBOOL ResMgrDrvImp::Uninit(void)
{
    MBOOL Result = MTRUE;
    //
    Mutex::Autolock lock(mLock);
    //
    if(mUser <= 0)
    {
        LOG_WRN("No user(%d)",mUser);
        goto EXIT;
    }
    //
    android_atomic_dec(&mUser);
    //
    if(mUser == 0)
    {
        LOG_MSG("Last user(%d)",mUser);
    }
    else
    {
        LOG_MSG("More user(%d)",mUser);
        goto EXIT;
    }
    //
    if(mFdCamPipeMgr >= 0)
    {
        close(mFdCamPipeMgr);
        mFdCamPipeMgr = -1;
    }
    //
    if(mFdHdmiTx >= 0)
    {
        close(mFdHdmiTx);
        mFdHdmiTx = -1;
    }
    //
    EXIT:
    return Result;
}
//-----------------------------------------------------------------------------
MBOOL ResMgrDrvImp::GetMode(RES_MGR_DRV_MODE_STRUCT* pMode)
{
    MBOOL Result = MTRUE;
    //
    if(mUser <= 0)
    {
        LOG_ERR("No user");
        Result = MFALSE;
        goto EXIT;
    }
    //
    if(ioctl(mFdCamPipeMgr, CAM_PIPE_MGR_GET_MODE, (CAM_PIPE_MGR_MODE_STRUCT*)pMode) == 0)
    {
        LOG_MSG("ScenSw(%d),ScenHw(%d),Dev(%d)",
                pMode->ScenSw,
                pMode->ScenHw,
                pMode->Dev);
    }
    else
    {
        LOG_ERR("GET_MODE fail");
        Result = MFALSE;
        goto EXIT;
    }
    //
    EXIT:
    //LOG_MSG("Result(%d)",Result);
    return Result;

}
//-----------------------------------------------------------------------------
MBOOL ResMgrDrvImp::SetMode(RES_MGR_DRV_MODE_STRUCT* pMode)
{
    MBOOL Result = MTRUE, IsCdpConcurLocked = MFALSE, IsCdpLinkLocked = MFALSE;;
    RES_MGR_DRV_MODE_STRUCT     CurrMode;
    CAM_PIPE_MGR_LOCK_STRUCT    CamPipeMgrLock;
    CAM_PIPE_MGR_UNLOCK_STRUCT  CamPipeMgrUnlock;
    CAM_PIPE_MGR_DISABLE_STRUCT CamPipeMgrDisable;
    //
    if(mUser <= 0)
    {
        LOG_ERR("No user");
        Result = MFALSE;
        goto EXIT;
    }
    //
    if(mFdCamPipeMgr < 0)
    {
        LOG_ERR("CamPipeMgr kernel is not opened");
        Result = MFALSE;
        goto EXIT;
    }
    //
    LOG_MSG("ScenSw(%d),ScenHw(%d),Dev(%d)",
            pMode->ScenSw,
            pMode->ScenHw,
            pMode->Dev);
    //
    GetMode(&CurrMode);
    //
    if( CurrMode.ScenSw == (pMode->ScenSw) &&
        CurrMode.ScenHw == (pMode->ScenHw) &&
        CurrMode.Dev == (pMode->Dev))
    {
        LOG_MSG("OK:Same");
        goto EXIT;
    }
    //
    switch(CurrMode.ScenSw)
    {
        case RES_MGR_DRV_SCEN_SW_NONE:
        {
            IsCdpLinkLocked = MTRUE;
            break;
        }
        default:
        {
            //do nothing.
        }
    }
    //
    if(IsCdpLinkLocked)
    {
        CamPipeMgrLock.PipeMask = CAM_PIPE_MGR_PIPE_MASK_CDP_LINK;
        CamPipeMgrLock.Timeout = RES_MGR_DRV_WAIT_CDP_LINK_TIMEOUT;
        if(ioctl(mFdCamPipeMgr, CAM_PIPE_MGR_LOCK, &CamPipeMgrLock) == 0)
        {
            CamPipeMgrDisable.PipeMask = CAM_PIPE_MGR_PIPE_MASK_CDP_LINK;
            if(ioctl(mFdCamPipeMgr, CAM_PIPE_MGR_DISABLE_PIPE, &CamPipeMgrDisable) == 0)
            {
                CamPipeMgrUnlock.PipeMask = CAM_PIPE_MGR_PIPE_MASK_CDP_LINK;
                if(ioctl(mFdCamPipeMgr, CAM_PIPE_MGR_UNLOCK, &CamPipeMgrUnlock) == 0)
                {
                    LOG_MSG("CDP_LINK:LOCK->DISABLE->UNLOCK OK");
                }
                else
                {
                    LOG_ERR("CDP_LINK:LOCK->DISABLE->UNLOCK fail");
                    Result = MFALSE;
                }
            }
            else
            {
                LOG_ERR("CDP_LINK:LOCK->DISABLE fail");
                Result = MFALSE;
            }
        }
        else
        {
            LOG_ERR("CDP_LINK:LOCK fail");
            Result = MFALSE;
        }
    }

    //
    #if 0
    switch(CurrMode.ScenSw)
    {
        case RES_MGR_DRV_SCEN_SW_CAM_PRV:
        case RES_MGR_DRV_SCEN_SW_VIDEO_PRV:
        {
            if(CurrMode.ScenHw == RES_MGR_DRV_SCEN_HW_VSS)
            {
                IsCdpConcurLocked = MTRUE;
            }
            break;
        }
        case RES_MGR_DRV_SCEN_SW_ZSD:
        {
            if(CurrMode.ScenHw == RES_MGR_DRV_SCEN_HW_ZSD)
            {
                IsCdpConcurLocked = MTRUE;
            }
            break;
        }
        default:
        {
            //do nothing.
        }
    }
    //
    if(IsCdpConcurLocked)
    {
        CamPipeMgrLock.PipeMask = CAM_PIPE_MGR_PIPE_MASK_CDP_CONCUR;
        CamPipeMgrLock.Timeout = RES_MGR_DRV_WAIT_CDP_CONCUR_TIMEOUT;
        if(ioctl(mFdCamPipeMgr, CAM_PIPE_MGR_LOCK, &CamPipeMgrLock) == 0)
        {
            CamPipeMgrDisable.PipeMask = CAM_PIPE_MGR_PIPE_MASK_CDP_CONCUR;
            if(ioctl(mFdCamPipeMgr, CAM_PIPE_MGR_DISABLE_PIPE, &CamPipeMgrDisable) == 0)
            {
                CamPipeMgrUnlock.PipeMask = CAM_PIPE_MGR_PIPE_MASK_CDP_CONCUR;
                if(ioctl(mFdCamPipeMgr, CAM_PIPE_MGR_UNLOCK, &CamPipeMgrUnlock) == 0)
                {
                    LOG_MSG("CDP_CONCUR:LOCK->DISABLE->UNLOCK OK");
                }
                else
                {
                    LOG_ERR("CDP_CONCUR:LOCK->DISABLE->UNLOCK fail");
                    Result = MFALSE;
                }
            }
            else
            {
                LOG_ERR("CDP_CONCUR:LOCK->DISABLE fail");
                Result = MFALSE;
            }
        }
        else
        {
            LOG_ERR("CDP_CONCUR:LOCK fail");
            Result = MFALSE;
        }
    }
    #endif
    //
    if(Result)
    {
        if(ioctl(mFdCamPipeMgr, CAM_PIPE_MGR_SET_MODE, (CAM_PIPE_MGR_MODE_STRUCT*)pMode) == 0)
        {
            LOG_MSG("OK");
        }
        else
        {
            LOG_ERR("SET_MODE fail");
            Result = MFALSE;
        }
    }
    //
    if(Result)
    {
        BWC BwcIns;
        //
        if(pMode->Dev == RES_MGR_DRV_DEV_VT)
        {
            if(pMode->ScenSw == RES_MGR_DRV_SCEN_SW_NONE)
            {
                BwcIns.Profile_Change(BWCPT_VIDEO_TELEPHONY,false);
            }
            else
            {
                BwcIns.Profile_Change(BWCPT_VIDEO_TELEPHONY,true);
            }
        }
        else
        {
            switch(CurrMode.ScenSw)
            {
                case RES_MGR_DRV_SCEN_SW_CAM_PRV:
                case RES_MGR_DRV_SCEN_SW_VIDEO_PRV:
                case RES_MGR_DRV_SCEN_SW_VIDEO_REC:
                case RES_MGR_DRV_SCEN_SW_VIDEO_VSS:
                {
                    BwcIns.Profile_Change(BWCPT_CAMERA_PREVIEW,false);
                    break;
                }
                case RES_MGR_DRV_SCEN_SW_CAM_CAP:
                {
                    BwcIns.Profile_Change(BWCPT_CAMERA_CAPTURE,false);
                    break;
                }
                case RES_MGR_DRV_SCEN_SW_ZSD:
                {
                    BwcIns.Profile_Change(BWCPT_CAMERA_ZSD,false);
                    break;
                }
                default:
                {
                    //do nothing.
                }
            }
            //
            switch(pMode->ScenSw)
            {
                case RES_MGR_DRV_SCEN_SW_NONE:
                case RES_MGR_DRV_SCEN_SW_CAM_IDLE:
                {
                    BwcIns.Profile_Change(BWCPT_CAMERA_PREVIEW,false);
                    break;
                }
                case RES_MGR_DRV_SCEN_SW_CAM_PRV:
                case RES_MGR_DRV_SCEN_SW_VIDEO_PRV:
                case RES_MGR_DRV_SCEN_SW_VIDEO_REC:
                case RES_MGR_DRV_SCEN_SW_VIDEO_VSS:
                {
                    BwcIns.Profile_Change(BWCPT_CAMERA_PREVIEW,true);
                    break;
                }
                case RES_MGR_DRV_SCEN_SW_CAM_CAP:
                {
                    BwcIns.Profile_Change(BWCPT_CAMERA_CAPTURE,true);
                    break;
                }
                case RES_MGR_DRV_SCEN_SW_ZSD:
                {
                    BwcIns.Profile_Change(BWCPT_CAMERA_ZSD,true);
                    break;
                }
                default:
                {
                    //do nothing.
                }
            }
            //
            switch(pMode->ScenSw)
            {
                case RES_MGR_DRV_SCEN_SW_NONE:
                {
                    CloseHdmi(MFALSE);
                    break;
                }
                default:
                {
                    CloseHdmi(MTRUE);
                    break;
                }
            }

        }
    }
    //
    EXIT:
    //LOG_MSG("Result(%d)",Result);
    return Result;

}
//-----------------------------------------------------------------------------
MBOOL ResMgrDrvImp::CloseHdmi(MBOOL En)
{
    MBOOL Result = MTRUE;
    //
    LOG_MSG("En(%d)",En);
    //
    if(mUser <= 0)
    {
        LOG_ERR("No user");
        Result = MFALSE;
        goto EXIT;
    }
    //
    if(mFdHdmiTx < 0)
    {
        mFdHdmiTx = open(RES_MGR_DRV_DEVNAME_HDMITX, O_RDONLY, 0);
        if(mFdHdmiTx < 0)
        {
            LOG_WRN("HDMITX kernel open fail, errno(%d):%s",errno,strerror(errno));
        }
    }
    //
    if(mFdHdmiTx >= 0)
    {
        if(En)
        {
            if(ioctl(mFdHdmiTx, MTK_HDMI_FORCE_CLOSE, 0) < 0)
            {
                LOG_ERR("HDMI_FORCE_CLOSE fail, errno(%d):%s",errno,strerror(errno));
                Result = MFALSE;
            }
        }
        else
        {
            if(ioctl(mFdHdmiTx, MTK_HDMI_FORCE_OPEN, 0) < 0)
            {
                LOG_ERR("HDMI_FORCE_OPEN fail, errno(%d):%s",errno,strerror(errno));
                Result = MFALSE;
            }
        }
    }
    //
    EXIT:
    //LOG_MSG("Result(%d)",Result);
    return Result;
}
//-----------------------------------------------------------------------------

