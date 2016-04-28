#ifndef _SENINF_DRV_BASE_H_
#define _SENINF_DRV_BASE_H_

/*******************************************************************************
*
********************************************************************************/


typedef enum {
	PAD_10BIT		= 0x0,
	PAD_8BIT_7_0	= 0x3,
	PAD_8BIT_9_2	= 0x4,
}PAD2CAM_DATA_ENUM;

typedef enum {
	MIPI_SENSOR		= 0x0,
	TEST_MODEL		= 0x1,
	CCIR656			= 0x2,
	PARALLEL_SENSOR	= 0x3,
	SERIAL_SENSOR	= 0x4,
	HD_TV			= 0x5,
	EXT_CSI2_OUT1	= 0x6,
	EXT_CSI2_OUT2	= 0x7
}SENINF_SOURCE_ENUM;

typedef enum {
	TG_12BIT	= 0x0,
	TG_10BIT	= 0x1,
	TG_8BIT		= 0x2
}SENSOR_DATA_BITS_ENUM;

typedef enum {
	RAW_8BIT_FMT		= 0x0,
	RAW_10BIT_FMT		= 0x1,
	RAW_12BIT_FMT		= 0x2,
	YUV422_FMT			= 0x3,
	CCIR656_FMT			= 0x4,
	RGB565_MIPI_FMT		= 0x5,
	RGB888_MIPI_FMT		= 0x6,
	JPEG_FMT			= 0x7
}TG_FORMAT_ENUM;

typedef enum {
	ACTIVE_HIGH		= 0x0,
	ACTIVE_LOW		= 0x1,		
}CCIR656_OUTPUT_POLARITY_ENUM;

typedef enum {
	IMMIDIANT_TRIGGER	= 0x0,
	REFERENCE_VS1		= 0x1,
	I2C1_BEFORE_I2C2	= 0x2,
	I2C2_BEFORE_I2C1	= 0x3
}N3D_I2C_TRIGGER_MODE_ENUM;

typedef enum drvSeninfCmd_s {
	CMD_SET_DEVICE				= 0x1000,
    CMD_GET_SENINF_ADDR         = 0x2001,
    CMD_DRV_SENINF_MAX             = 0xFFFF
} drvSeninfCmd_e;


#define CAM_PLL_48_GROUP        (1)
#define CAM_PLL_52_GROUP        (2)

/*******************************************************************************
*
********************************************************************************/
class SeninfDrv {
public:
    //
    static SeninfDrv* createInstance();
    virtual void   destroyInstance() = 0;

protected:
    virtual ~SeninfDrv() {};
    
public:
    virtual int init() = 0;
    //
    virtual int uninit() = 0;
	//
	virtual int waitSeninf1Irq(int mode) = 0;
    //
    virtual int waitSeninf2Irq(int mode) = 0;
	//
    typedef struct reg_s {
        unsigned long addr;
        unsigned long val;
    } reg_t;
    //
    virtual unsigned long readReg(unsigned long addr) = 0;
    virtual int writeReg(unsigned long addr, unsigned long val) = 0;
    virtual int readRegs(reg_t *pregs, int count) = 0;
    virtual int writeRegs(reg_t *pregs, int count) = 0;
    virtual int holdReg(bool isHold) = 0;
    virtual int dumpReg() = 0;
    //
    virtual int setTg1PhaseCounter(unsigned long pcEn, unsigned long mclkSel,
        unsigned long clkCnt, unsigned long clkPol,
        unsigned long clkFallEdge, unsigned long clkRiseEdge, unsigned long padPclkInv) = 0;
    //
    virtual int setTg2PhaseCounter(unsigned long pcEn, unsigned long mclkSel,
        unsigned long clkCnt, unsigned long clkPol,
        unsigned long clkFallEdge, unsigned long clkRiseEdge, unsigned long padPclkInv) = 0;

	//
    virtual int setTg1GrabRange(unsigned long pixelStart, unsigned long pixelEnd,
        unsigned long lineStart, unsigned long lineEnd) = 0;
	//
    virtual int setTg2GrabRange(unsigned long pixelStart, unsigned long pixelEnd,
        unsigned long lineStart, unsigned long lineEnd) = 0;

    //
    virtual int setTg1SensorModeCfg(unsigned long hsPol, unsigned long vsPol) = 0;
    //
    virtual int setTg2SensorModeCfg(unsigned long hsPol, unsigned long vsPol) = 0;

    //
    virtual int setTg1ViewFinderMode(unsigned long spMode, unsigned long spDelay) = 0;
 //
    virtual int setTg2ViewFinderMode(unsigned long spMode, unsigned long spDelay) = 0;	
    //
    virtual int setTg1InputCfg(PAD2CAM_DATA_ENUM padSel, SENINF_SOURCE_ENUM inSrcTypeSel,
	TG_FORMAT_ENUM inDataType, SENSOR_DATA_BITS_ENUM senInLsb) = 0;
    //
    virtual int setTg2InputCfg(PAD2CAM_DATA_ENUM padSel, SENINF_SOURCE_ENUM inSrcTypeSel,
	TG_FORMAT_ENUM inDataType, SENSOR_DATA_BITS_ENUM senInLsb) = 0;

    //
    virtual int sendCommand(int cmd, int arg1 = 0, int arg2 = 0, int arg3 = 0) = 0;
    //
    virtual int initTg1CSI2(bool csi2_en) = 0;
    //
    virtual int initTg2CSI2(bool csi2_en) = 0;	
    //    
    virtual int setTg1CSI2(unsigned long dataTermDelay, 
                        unsigned long dataSettleDelay, 
                        unsigned long clkTermDelay, 
                        unsigned long vsyncType, 
                        unsigned long dlane_num, 
                        unsigned long csi2_en,
                        unsigned long dataheaderOrder,
                        unsigned long dataFlow) = 0;
    //    
    virtual int setTg2CSI2(unsigned long dataTermDelay, 
                        unsigned long dataSettleDelay, 
                        unsigned long clkTermDelay, 
                        unsigned long vsyncType, 
                        unsigned long dlane_num, 
                        unsigned long csi2_en,
                        unsigned long dataheaderOrder,
                        unsigned long dataFlow) = 0;

    //
    virtual int setTg1IODrivingCurrent(unsigned long ioDrivingCurrent) = 0;
    //
    virtual int setTg2IODrivingCurrent(unsigned long ioDrivingCurrent) = 0;	
    //
    virtual int setTg1MCLKEn(bool isEn) = 0;
	//
	virtual int setTg2MCLKEn(bool isEn) = 0;
	//
	virtual int setFlashA(unsigned long endFrame, unsigned long startPoint, unsigned long lineUnit, unsigned long unitCount, unsigned long startLine, unsigned long startPixel, unsigned long  flashPol) = 0;
	//
	virtual int setFlashB(unsigned long contiFrm, unsigned long startFrame, unsigned long lineUnit, unsigned long unitCount, unsigned long startLine, unsigned long startPixel) = 0;
	//
	virtual int setFlashEn(bool flashEn) = 0;
	//
	virtual int setCCIR656Cfg(CCIR656_OUTPUT_POLARITY_ENUM vsPol, CCIR656_OUTPUT_POLARITY_ENUM hsPol, unsigned long hsStart, unsigned long hsEnd) = 0;
	//
	virtual int setN3DCfg(unsigned long n3dEn, unsigned long i2c1En, unsigned long i2c2En, unsigned long n3dMode) = 0;
	//
	virtual int setN3DI2CPos(unsigned long n3dPos) = 0;
	//
	virtual int setN3DTrigger(bool i2c1TrigOn, bool i2c2TrigOn) = 0;
	//
	virtual int checkSeninf1Input() = 0;
	//
	virtual int checkSeninf2Input() = 0;
	//

};

#endif // _ISP_DRV_H_

