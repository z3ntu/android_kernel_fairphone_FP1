#include "platform.h"
#include "mt_usbd.h"

#if CFG_FPGA_PLATFORM
#include "mt_i2c.h"
#endif

#define USB20_PHY_BASE	(USBSIF_BASE + 0x0800)
#define USB11_PHY_BASE	(USBSIF_BASE + 0x0900)

#if CFG_FPGA_PLATFORM
#define USB_I2C_ID	I2C2	/* 0 - 6 */
#define PATH_NORMAL	0
#define PATH_PMIC	1

U32 usb_i2c_read8 (U8 addr, U8 *dataBuffer)
{
	U32 ret_code = I2C_OK;
	U8 write_data = addr;

	/* set register command */
	ret_code = mt_i2c_write(USB_I2C_ID, 0xc0, &write_data, 1, PATH_NORMAL);

	if (ret_code != I2C_OK)
		return ret_code;

	ret_code = mt_i2c_read(USB_I2C_ID, 0xc0, dataBuffer, 1, PATH_NORMAL);

	return ret_code;
}

U32 usb_i2c_write8 (U8 addr, U8 value)
{
	U32 ret_code = I2C_OK;
	U8 write_data[2];

	write_data[0]= addr;
	write_data[1] = value;

	ret_code = mt_i2c_write(USB_I2C_ID, 0xc0, write_data, 2, PATH_NORMAL);

	return ret_code;
}

#define USBPHY_I2C_READ8(addr, buffer)	 usb_i2c_read8(addr, buffer)
#define USBPHY_I2C_WRITE8(addr, value)	 usb_i2c_write8(addr, value)
#endif

#define USBPHY_READ8(offset)		__raw_readb(USB20_PHY_BASE+offset)
#define USBPHY_WRITE8(offset, value)	__raw_writeb(value, USB20_PHY_BASE+offset)
#define USBPHY_SET8(offset, mask)	USBPHY_WRITE8(offset, USBPHY_READ8(offset) | mask)
#define USBPHY_CLR8(offset, mask)	USBPHY_WRITE8(offset, USBPHY_READ8(offset) & ~mask)

#define USB11PHY_READ8(offset)		__raw_readb(USB11_PHY_BASE+offset)
#define USB11PHY_WRITE8(offset, value)	__raw_writeb(value, USB11_PHY_BASE+offset)
#define USB11PHY_SET8(offset, mask)	USB11PHY_WRITE8(offset, USB11PHY_READ8(offset) | mask)
#define USB11PHY_CLR8(offset, mask)	USB11PHY_WRITE8(offset, USB11PHY_READ8(offset) & ~mask)

#if CFG_FPGA_PLATFORM
void mt_usb_phy_poweron (void)
{

	#define PHY_DRIVING   0x3

	UINT8 usbreg8;
	unsigned int i;

	/* force_suspendm = 0 */
	USBPHY_CLR8(0x6a, 0x04);

	USBPHY_I2C_WRITE8(0xff, 0x00);
	USBPHY_I2C_WRITE8(0x61, 0x04);
	USBPHY_I2C_WRITE8(0x68, 0x00);
	USBPHY_I2C_WRITE8(0x6a, 0x00);
	USBPHY_I2C_WRITE8(0x00, 0x6e);
	USBPHY_I2C_WRITE8(0x1b, 0x0c);
	USBPHY_I2C_WRITE8(0x08, 0x44);
	USBPHY_I2C_WRITE8(0x11, 0x55);
	USBPHY_I2C_WRITE8(0x1a, 0x68);

	#if defined(USB_PHY_DRIVING_TUNING)
	/* driving tuning */
	USBPHY_I2C_READ8(0xab, &usbreg8);
	usbreg8 &= ~0x3;
	usbreg8 |= PHY_DRIVING;
	USBPHY_I2C_WRITE8(0xab, usbreg8);

	for(i = 0; i < 16; i++)
	{
		USBPHY_I2C_READ8((0x92+i), &usbreg8);
		usbreg8 &= ~0x3;
		usbreg8 |= PHY_DRIVING;
		USBPHY_I2C_WRITE8((0x92+i), usbreg8);
	}

	USBPHY_I2C_READ8(0xbc, &usbreg8);
	usbreg8 &= ~0x3;
	usbreg8 |= PHY_DRIVING;
	USBPHY_I2C_WRITE8(0xbc, usbreg8);

	USBPHY_I2C_READ8(0xbe, &usbreg8);
	usbreg8 &= ~0x3;
	usbreg8 |= PHY_DRIVING;
	USBPHY_I2C_WRITE8(0xbe, usbreg8);

	USBPHY_I2C_READ8(0xbf, &usbreg8);
	usbreg8 &= ~0x3;
	usbreg8 |= PHY_DRIVING;
	USBPHY_I2C_WRITE8(0xbf, usbreg8);

	USBPHY_I2C_READ8(0xcd, &usbreg8);
	usbreg8 &= ~0x3;
	usbreg8 |= PHY_DRIVING;
	USBPHY_I2C_WRITE8(0xcd, usbreg8);

	USBPHY_I2C_READ8(0xf1, &usbreg8);
	usbreg8 &= ~0x3;
	usbreg8 |= PHY_DRIVING;
	USBPHY_I2C_WRITE8(0xf1, usbreg8);

	USBPHY_I2C_READ8(0xa7, &usbreg8);
	usbreg8 &= ~0x3;
	usbreg8 |= PHY_DRIVING;
	USBPHY_I2C_WRITE8(0xa7, usbreg8);

	USBPHY_I2C_READ8(0xa8, &usbreg8);
	usbreg8 &= ~0x3;
	usbreg8 |= PHY_DRIVING;
	USBPHY_I2C_WRITE8(0xa8, usbreg8);
	#endif

	udelay(800);
}
void mt_usb_phy_savecurrent (void)
{
	/* no need */
}
void mt_usb_phy_recover (void)
{
	/* no need */
}
void mt_usb11_phy_savecurrent(void)
{
	/* no need */
}
#else
void mt_usb_phy_poweron (void)
{
	/*
	 * swtich to USB function.
	 * (system register, force ip into usb mode).
	 */
	USBPHY_CLR8(0x6b, 0x04);
	USBPHY_CLR8(0x6e, 0x01);

	/* RG_USB20_BC11_SW_EN = 1'b0 */
	USBPHY_CLR8(0x1c, 0x80);

	/* release force suspendm */
	USBPHY_CLR8(0x6a, 0x04);

	udelay(800);

	return;
}

void mt_usb_phy_savecurrent (void)
{
	/*
	 * swtich to USB function.
	 * (system register, force ip into usb mode).
	 */
	USBPHY_CLR8(0x6b, 0x04);
	USBPHY_CLR8(0x6e, 0x01);

	/* release force suspendm */
	USBPHY_CLR8(0x6a, 0x04);
	/* RG_DPPULLDOWN./RG_DMPULLDOWN. */
	USBPHY_SET8(0x68, 0xc0);
	/* RG_XCVRSEL[1:0] = 2'b01 */
	USBPHY_CLR8(0x68, 0x30);
	USBPHY_SET8(0x68, 0x10);
	/* RG_TERMSEL = 1'b1 */
	USBPHY_SET8(0x68, 0x04);
	/* RG_DATAIN[3:0] = 4'b0000 */
	USBPHY_CLR8(0x69, 0x3c);

	/*
	 * force_dp_pulldown, force_dm_pulldown,
	 * force_xcversel, force_termsel.
	 */
	USBPHY_SET8(0x6a, 0xba);

	/* RG_USB20_BC11_SW_EN = 1'b0 */
	USBPHY_CLR8(0x1a, 0x80);
	/* RG_USB20_OTG_VBUSSCMP_EN = 1'b0 */
	USBPHY_CLR8(0x1a, 0x10);

	udelay(800);

	/* rg_usb20_pll_stable = 1 */
	USBPHY_SET8(0x63, 0x02);

	udelay(1);

	/* force suspendm = 1 */
	USBPHY_SET8(0x6a, 0x04);

	udelay(1);

	return;
}

void mt_usb_phy_recover (void)
{
	/* clean PUPD_BIST_EN */
	/* PUPD_BIST_EN = 1'b0 */
	/* PMIC will use it to detect charger type */
	USBPHY_CLR8(0x1d, 0x10);

	/* force_uart_en = 1'b0 */
	USBPHY_CLR8(0x6b, 0x04);
	/* RG_UART_EN = 1'b0 */
	USBPHY_CLR8(0x6e, 0x01);
	/* force_uart_en = 1'b0 */
	USBPHY_CLR8(0x6a, 0x04);

	USBPHY_CLR8(0x68, 0xf4);

	/* RG_DATAIN[3:0] = 4'b0000 */
	USBPHY_CLR8(0x69, 0x3c);

	USBPHY_CLR8(0x6a, 0xba);

	/* RG_USB20_BC11_SW_EN = 1'b0 */
	USBPHY_CLR8(0x1a, 0x80);
	/* RG_USB20_OTG_VBUSSCMP_EN = 1'b1 */
	USBPHY_SET8(0x1a, 0x10);

	udelay(800);

	return;
}
//ALPS00427972, implement the analog register formula
void mt_usb_calibraion ()
{
    //Set the calibration after power on
    //Add here for eFuse, chip version checking -> analog register calibration
    int input_reg = INREG16(M_HW_RES3);
    print("%s: input_reg = 0x%x \n", __func__, input_reg);
    int term_vref 	= (input_reg & RG_USB20_TERM_VREF_SEL_MASK) >> 13;     //0xE000      //0b 1110,0000,0000,0000     15~13
    int clkref 		= (input_reg & RG_USB20_CLKREF_REF_MASK)    >> 10;     //0x1C00      //0b 0001,1100,0000,0000     12~10
    int vrt_vref	= (input_reg & RG_USB20_VRT_VREF_SEL_MASK)  >> 7;      //0x0380      //0b 0000,0011,1000,0000     9~7

    print("%s: term_vref = 0x%x,  clkref = 0x%x, vrt_vref = 0x%x,\n", __func__, term_vref, clkref, vrt_vref);

    if(term_vref)
            mt_usb_phy_calibraion(1, term_vref);
    if(clkref)
            mt_usb_phy_calibraion(2, clkref);
    if(vrt_vref)
            mt_usb_phy_calibraion(3, vrt_vref);
}

void mt_usb_phy_calibraion (int case_set, int input_reg)
{
    int temp_added=0;
    int temp_test=0;
    int temp_mask;

	print("%s: case_set %d, input_reg = 0x%x \n", __func__, case_set, input_reg);

    switch(case_set)
    {
    case 1:
        //case  1
        //If M_HW_RES3[15:13] !=0
            //RG_USB20_TERM_VREF_SEL[2:0] <= RG_USB20_TERM_VREF_SEL[2:0] + M_HW_RES3[15:13]
        temp_mask = 0x07;
        temp_test = USBPHY_READ8(0x05);
		print("%s: temp_test = 0x%x \n", __func__, temp_test);
        temp_added = (USBPHY_READ8(0x05)& temp_mask) + input_reg;
		print("%s: temp_added = 0x%x \n", __func__, temp_added);
        temp_added &= 0x07;
		print("%s: temp_added = 0x%x \n", __func__, temp_added);

        USBPHY_CLR8(0x05, temp_mask);
        USBPHY_SET8(0x05, temp_added);

        temp_test = USBPHY_READ8(0x05);
        print("%s: final temp_test = 0x%x \n", __func__, temp_test);
        break;
    case 2:
        //case 2
        //If M_HW_RES3[12:10] !=0
            //RG_USB20_CLKREF_REF[2:0]<= RG_USB20_CLKREF_REF[2:0]+ M_HW_RES3[12:10]
        temp_mask = 0x07;

        temp_test = USBPHY_READ8(0x07);
		print("%s: temp_test = 0x%x \n", __func__, temp_test);
        temp_added = (USBPHY_READ8(0x07)& temp_mask) + input_reg;
		print("%s: temp_added = 0x%x \n", __func__, temp_added);
        temp_added &= 0x07;
		print("%s: temp_added = 0x%x \n", __func__, temp_added);

        USBPHY_CLR8(0x07, temp_mask);
        USBPHY_SET8(0x07, temp_added);

        temp_test = USBPHY_READ8(0x07);
        print("%s: final temp_test = 0x%x \n", __func__, temp_test);
        break;
    case 3:
        //case 3
        //If M_HW_RES3[9:7] !=0
            //RG_USB20_VRT_VREF_SEL[2:0]<=RG_USB20_VRT_VREF_SEL[2:0]+ M_HW_RES3[9:7]
        temp_mask = 0x70;

        temp_test = USBPHY_READ8(0x05);
		print("%s: temp_test = 0x%x \n", __func__, temp_test);
        temp_added = (USBPHY_READ8(0x05)& temp_mask) >> 4;
		print("%s: temp_added = 0x%x \n", __func__, temp_added);
        temp_added += input_reg;
		print("%s: temp_added = 0x%x \n", __func__, temp_added);
        temp_added &= 0x07;
		print("%s: temp_added = 0x%x \n", __func__, temp_added);

        USBPHY_CLR8(0x05, temp_mask);
        USBPHY_SET8(0x05, temp_added<<4);

        temp_test = USBPHY_READ8(0x05);
        print("%s: final temp_test = 0x%x \n", __func__, temp_test);
        break;
    }
    

	return;
}

//ALPS00427972, implement the analog register formula
void mt_usb11_phy_savecurrent(void)
{
    //4 1. swtich to USB function. (system register, force ip into usb mode.
    USB11PHY_CLR8(0x6b, 0x04);
    USB11PHY_CLR8(0x6e, 0x01);

    //4 2. release force suspendm.
    USB11PHY_CLR8(0x6a, 0x04);
    //4 3. RG_DPPULLDOWN./RG_DMPULLDOWN.
    USB11PHY_SET8(0x68, 0xc0);
    //4 4. RG_XCVRSEL[1:0] =2'b01.
    USB11PHY_CLR8(0x68, 0x30);
    USB11PHY_SET8(0x68, 0x10);
    //4 5. RG_TERMSEL = 1'b1
    USB11PHY_SET8(0x68, 0x04);
    //4 6. RG_DATAIN[3:0]=4'b0000
    USB11PHY_CLR8(0x69, 0x3c);
    //4 7.force_dp_pulldown, force_dm_pulldown, force_xcversel,force_termsel.
    USB11PHY_SET8(0x6a, 0xba);

    //4 8.RG_USB20_BC11_SW_EN 1'b0
    USB11PHY_CLR8(0x1a, 0x80);
    //4 9.RG_USB20_OTG_VBUSSCMP_EN 1'b0
    USB11PHY_CLR8(0x1a, 0x10);
    //4 10. delay 800us.
    udelay(800);
    //4 11. rg_usb20_pll_stable = 1
    USB11PHY_SET8(0x63, 0x02);

    udelay(1);
    //4 12.  force suspendm = 1.
    USB11PHY_SET8(0x6a, 0x04);

    USB11PHY_CLR8(0x6C, 0x2C);
    USB11PHY_SET8(0x6C, 0x10);
    USB11PHY_CLR8(0x6D, 0x3C);

    //4 13.  wait 1us
    udelay(1);

	return;
}
#endif

