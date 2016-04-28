#ifndef ISP_DRV_IMP_H
#define ISP_DRV_IMP_H
//-----------------------------------------------------------------------------
//#include <utils/Errors.h>
#include "common/CamTypes.h"
#include "isp_drv.h"
#include "imem_drv.h"   // For IMemDrv*.
//-----------------------------------------------------------------------------
using namespace android;
//-----------------------------------------------------------------------------
#define ISP_DRV_DEV_NAME            "/dev/camera-isp"
#define ISP_DRV_VIR_DEFAULT_DATA    (0)
#define ISP_DRV_VIR_ADDR_ALIGN      (0x03) // 4 bytes alignment
#define ISP_DRV_CQ_DESCRIPTOR_ADDR_ALIGN      (0x07) // 8 bytes alignment



typedef struct {
    MUINT32     checkBit;
    MUINT32     checkOffset;
}ISP_DRV_TURNING_TOP;

typedef struct {
    ISP_DRV_CQ_ENUM             virtualAddrCq;
    ISP_DRV_DESCRIPTOR_CQ_ENUM  descriptorCq;
}ISP_DRV_CQ_MAPPING;




//-----------------------------------------------------------------------------
class IspDrvImp : public IspDrv
{
    public:
        IspDrvImp();
        virtual ~IspDrvImp();
    //
    public:
        static IspDrv*  getInstance(bool fgIsGdmaMode = MFALSE);
        virtual void    destroyInstance(void);
        virtual MBOOL   init(void);
        virtual MBOOL   uninit(void);
        virtual MBOOL   waitIrq(ISP_DRV_WAIT_IRQ_STRUCT WaitIrq);
        virtual MBOOL   readIrq(ISP_DRV_READ_IRQ_STRUCT* pReadIrq);
        virtual MBOOL   checkIrq(ISP_DRV_CHECK_IRQ_STRUCT CheckIrq);
        virtual MBOOL   clearIrq(ISP_DRV_CLEAR_IRQ_STRUCT ClearIrq);
        virtual MBOOL   reset(void);
        virtual MBOOL   resetBuf(void);
        virtual MUINT32 getRegAddr(void);
        virtual isp_reg_t* getRegAddrMap(void);
        virtual MBOOL   readRegs(
            ISP_DRV_REG_IO_STRUCT*  pRegIo,
            MUINT32                 Count);
        virtual MUINT32 readReg(MUINT32 Addr);
        virtual MBOOL   writeRegs(
            ISP_DRV_REG_IO_STRUCT*  pRegIo,
            MUINT32                 Count);
        virtual MBOOL   writeReg(
            MUINT32     Addr,
            MUINT32     Data);
        virtual MBOOL   holdReg(MBOOL En);
        virtual MBOOL   dumpReg(void);
        virtual MBOOL   IsReadOnlyMode(void);
//        virtual MBOOL   IsGdmaMode(void);

#if defined(_use_kernel_ref_cnt_)
        virtual MBOOL   kRefCntCtrl(ISP_REF_CNT_CTRL_STRUCT* pCtrl);
#endif

        virtual MUINT32 GlobalPipeCountInc(void);
        virtual MUINT32 GlobalPipeCountDec(void);
        virtual MBOOL   rtBufCtrl(void *pBuf_ctrl);

#if defined(_rtbc_use_cq0c_)
        virtual MBOOL cqRingBuf(void *pBuf_ctrl);
#endif
    //
    private:
        volatile MINT32 mInitCount;
        mutable Mutex   mLock;
        //
        bool            m_fgIsGdmaMode;
        //
        /*imem*/
        IMemDrv*        m_pIMemDrv ;
        IMEM_BUF_INFO   m_ispVirRegBufInfo;
        IMEM_BUF_INFO   m_ispCQDescBufInfo;
    //
};
//-----------------------------------------------------------------------------
class IspDrvVirImp : public IspDrv
{
    public:
        IspDrvVirImp();
        virtual ~IspDrvVirImp();
    //
    public:
        static IspDrv*  getInstance(ISP_DRV_CQ_ENUM cq, MUINT32 ispVirRegAddr);
        virtual void    destroyInstance(void);
        virtual MBOOL   init(void){ /*LOG_ERR("Not support");*/return MTRUE;}
        virtual MBOOL   uninit(void){/*LOG_ERR("Not support");*/return MTRUE;}
        virtual MBOOL   waitIrq(ISP_DRV_WAIT_IRQ_STRUCT WaitIrq){/*LOG_ERR("Not support");*/return MTRUE;}
        virtual MBOOL   readIrq(ISP_DRV_READ_IRQ_STRUCT* pReadIrq){/*LOG_ERR("Not support");*/return MTRUE;}
        virtual MBOOL   checkIrq(ISP_DRV_CHECK_IRQ_STRUCT CheckIrq){/*LOG_ERR("Not support");*/return MTRUE;}
        virtual MBOOL   clearIrq(ISP_DRV_CLEAR_IRQ_STRUCT ClearIrq){/*LOG_ERR("Not support");*/return MTRUE;}
        virtual MBOOL   reset(void);
        virtual MBOOL   resetBuf(void){/*LOG_ERR("Not support");*/return MTRUE;}
        virtual MUINT32 getRegAddr(void);
        virtual isp_reg_t* getRegAddrMap(void){return NULL;}
        virtual MBOOL   readRegs(
            ISP_DRV_REG_IO_STRUCT*  pRegIo,
            MUINT32                 Count);
        virtual MUINT32 readReg(MUINT32 Addr);
        virtual MBOOL   writeRegs(
            ISP_DRV_REG_IO_STRUCT*  pRegIo,
            MUINT32                 Count);
        virtual MBOOL   writeReg(
            MUINT32     Addr,
            MUINT32     Data);
        virtual MBOOL   holdReg(MBOOL En){/*LOG_ERR("Not support");*/return MTRUE;}
        virtual MBOOL   dumpReg(void){/*LOG_ERR("Not support");*/return MTRUE;}
        virtual MBOOL   IsReadOnlyMode(void){return MFALSE;}
//      virtual MBOOL   IsGdmaMode(void){return MFALSE;}
        virtual MBOOL   rtBufCtrl(void *pBuf_ctrl){return MTRUE;}
        virtual MUINT32 GlobalPipeCountInc(void){return MTRUE;};
        virtual MUINT32 GlobalPipeCountDec(void){return MTRUE;};
        //
    //
    private:
        mutable Mutex   mLock;
        MUINT32 * mpIspVirRegBuffer;
};
//-----------------------------------------------------------------------------
#endif

