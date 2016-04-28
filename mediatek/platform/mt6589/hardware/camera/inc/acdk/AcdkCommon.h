///////////////////////////////////////////////////////////////////////////////
// No Warranty
// Except as may be otherwise agreed to in writing, no warranties of any
// kind, whether express or implied, are given by MTK with respect to any MTK
// Deliverables or any use thereof, and MTK Deliverables are provided on an
// "AS IS" basis.  MTK hereby expressly disclaims all such warranties,
// including any implied warranties of merchantability, non-infringement and
// fitness for a particular purpose and any warranties arising out of course
// of performance, course of dealing or usage of trade.  Parties further
// acknowledge that Company may, either presently and/or in the future,
// instruct MTK to assist it in the development and the implementation, in
// accordance with Company's designs, of certain softwares relating to
// Company's product(s) (the "Services").  Except as may be otherwise agreed
// to in writing, no warranties of any kind, whether express or implied, are
// given by MTK with respect to the Services provided, and the Services are
// provided on an "AS IS" basis.  Company further acknowledges that the
// Services may contain errors, that testing is important and Company is
// solely responsible for fully testing the Services and/or derivatives
// thereof before they are used, sublicensed or distributed.  Should there be
// any third party action brought against MTK, arising out of or relating to
// the Services, Company agree to fully indemnify and hold MTK harmless.
// If the parties mutually agree to enter into or continue a business
// relationship or other arrangement, the terms and conditions set forth
// hereunder shall remain effective and, unless explicitly stated otherwise,
// shall prevail in the event of a conflict in the terms in any agreements
// entered into between the parties.
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2008, MediaTek Inc.
// All rights reserved.
//
// Unauthorized use, practice, perform, copy, distribution, reproduction,
// or disclosure of this information in whole or in part is prohibited.
////////////////////////////////////////////////////////////////////////////////
// AcdkCommon  $Revision$
////////////////////////////////////////////////////////////////////////////////

//! \file  AcdkCommon.h

#ifndef _ACDKCOMMON_H_
#define _ACDKCOMMON_H_

#include "AcdkTypes.h"

/**  
*@enum eIMAGE_TYPE
*/
typedef enum 
{
    RGB565_TYPE          = 0x00000001, 
    RGB888_TYPE          = 0x00000002, 
    PURE_RAW8_TYPE       = 0x00000004,
    PURE_RAW10_TYPE      = 0x00000008,
    PROCESSED_RAW8_TYPE  = 0x00000010,
    PROCESSED_RAW10_TYPE = 0x00000020,
    JPEG_TYPE            = 0x00000040 
}eIMAGE_TYPE;

/**  
*@enum eRAW_ColorOrder
*@brief Color order of RAW image
*/
typedef enum
{
    RawPxlOrder_B = 0,  //! B Gb Gr R
    RawPxlOrder_Gb,     //! Gb B R Gr
    RawPxlOrderr_Gr,    //! Gr R B Gb
    RawPxlOrder_R       //! R Gr Gb B
}eRAW_ColorOrder;

/**  
*@struct bufInfo
*@brief Infomation of image (except RAW)
*/
typedef struct
{
    MUINT8 *bufAddr; 
    MUINT32 imgWidth;
    MUINT32 imgHeight;
    MUINT32 imgSize; 
} bufInfo;

/**  
*@struct RAWBufInfo
*@brief Infomation of RAW image
*/
typedef struct  
{
    MUINT8 *bufAddr;
    MUINT32 bitDepth;
    MUINT32 imgWidth;
    MUINT32 imgHeight;
    MUINT32 imgSize;
    MBOOL   isPacked;
    eRAW_ColorOrder eColorOrder;
}RAWBufInfo; 

/**  
*@struct ImageBufInfo
*@brief Infomation of image buffer
*/
typedef struct 
{
    eIMAGE_TYPE eImgType;   //! Image type 
    union 
    {
        bufInfo imgBufInfo; 
        RAWBufInfo RAWImgBufInfo; 
    }; 
}ImageBufInfo; 

/**  
*@struct ROIRect
*@brief CCT Application
*/
typedef struct 
{
    MUINT32 u4StartX;              //! Start X position for clip
    MUINT32 u4StartY;              //! Start Y position for clip
    MUINT32 u4ROIWidth;            //! Width of ROI 
    MUINT32 u4ROIHeight;           //! Height of ROI 
}ROIRect;

#endif //end AcdkCommon.h

