/***********************************************************************
*   Copyright (C) Huizhou Desay SV Automotive Co., Ltd.                *
*                 All Rights Reserved.                                                                                   *
*   Department : CT AD  NJ                                    *
*   AUTHOR     :                                       *
************************************************************************
* Object        :
* Module        :
* Instance      :
* Description   :
*-----------------------------------------------------------------------
* Version:0.1
* Date:2016/9/2
* Author:ShangQIuju
***********************************************************************/
/*-History--------------------------------------------------------------
* Version       Date         Name            Changes and comments
------------------------------------------------------------------------
*  0.1          2016.9.2     ShangQiuju     Init version.
------------------------------------------------------------------------
*  0.2	    2016.10.12  ShangQiuju     Add handler of Swdl status event.
                  when receive Swdl status event don't reset mcu,wait reset
		    request from soc.
------------------------------------------------------------------------
*  0.3         2016.12.26   ShangQiuju    For interface is updated in FDL complex driver,so 
                 update using interface in app.
------------------------------------------------------------------------
*  0.4         2017.1.3   ShangQiuju    Update for fuction translation.
------------------------------------------------------------------------
*  0.5         2017.1.14   ShangQiuju    Add UPGRADE_FLAG_ADDRESS for Hal DEV update.
*----------------------------------------------------------------------
*  0.6         2017.3.13   ShangQiuju     Remove ununsed test function, and eol spi upgrade test function.
*=====================================================================*/
#include "config.h"
#include "define.h"
#include "SocManager.h"
#include "TYPES.h"
#include "Signal_if.h"
#include "Upgrade.h"
#include "fbl_jmpToFbl.h"
#include "Hal_if.h"
#include "ComplexDrvier_Globaldef_IF.H"
#include "GPIO_if.h"
#include "socdriver.h"
#include "Tools.h"


void UpgradeMcuReqHandler(void);
BOOL bSwdlStatusEventOn = FALSE;
BOOL bUpdateFlagWrEEPOk = FALSE;
/***********************************************************************
*  Name        : Upgrade_DISPATCHERMSG
*  Description : handle upgrade spi message.        
*  Parameter   : commandid,data
*  Returns     : None
***********************************************************************/
void Upgrade_DISPATCHERMSG(U16 commandid, U8* data)
{
    switch(commandid)
    {
        case NAVI_SWDL_CONFIG_EVENT://0x5410
            if(data[0]&MCU_SW_UPDATE)
            {
                bSwdlStatusEventOn = TRUE;
                UpgradeMcuReqHandler();
            }
        break;
        case NAVI_SWDL_STATUS_EVENT://0x5413
            if((data[0]==SWDL_RADIO)&&(data[1]==SWDL_STATE_START))
            {
                bSwdlStatusEventOn = TRUE;
                UpgradeMcuReqHandler();
            }
            break;
	case NAVI_REQ_TO_RESET_EVENT: //6414
              if(bUpdateFlagWrEEPOk&&bSwdlStatusEventOn)
              {
                  bUpdateFlagWrEEPOk=FALSE;
                  bSwdlStatusEventOn = FALSE;
                  uP24G_SHDN_N_HIGH();
                  delay_ms(300);
                  uP24G_SHDN_N_LOW();
                  OSWDG_SoftReset();
	       }
		break;
       default:
	   	break;
    }
}

 BOOL FblSetFblStartMagic(void)
{	
    U8 i;
    BOOL rte_val=FALSE;
#if 1
    if(64!= HALBlock_Read(DEV_FDL,UPGRADE_FLAG_ADDRESS, &FblWriteData.UpdateFlag[0],64))return;
 #else
    FblWriteData.UpdateFlag[0]=0x3C;
    FblWriteData.UpdateFlag[1]=0xC3;
    FblWriteData.UpdateFlag[2]=0x3C;
    FblWriteData.UpdateFlag[3]=0xC3;
#endif
    FblSetFblStartMagicFlag();
    for(i=0;i<8;i++)
  	FblWriteData.MagicFlag[i]=fblStartMagicFlag[i];

	
    if(ERR_OK == HALBlock_Write(DEV_FDL,UPGRADE_FLAG_ADDRESS,&FblWriteData.UpdateFlag[0],64))
    {
        if(64 == HALBlock_Read(DEV_FDL,UPGRADE_FLAG_ADDRESS, &FblReadData.UpdateFlag[0],64))
        {
          if(is_same_arrays(&FblWriteData.MagicFlag[0], &FblReadData.MagicFlag[0], 8) == TRUE)
           {
               rte_val=TRUE;
           }
	    else
	    {
                rte_val=FALSE;
	    }
        }
     }

    return rte_val;
}

/***********************************************************************
*  Name        : UpgradeMcuReqHandler
*  Description : set upgrade flag at fdl address 0xFF200000-0xff200040      
*  Parameter   : None
*  Returns     : None
***********************************************************************/
 void UpgradeMcuReqHandler(void)
{	
#if 1
    FblWriteData.UpdateFlag[0]=0x5A;
    FblWriteData.UpdateFlag[1]=0xA5;
    FblWriteData.UpdateFlag[2]=0x5A;
    FblWriteData.UpdateFlag[3]=0xA5;
    if(ERR_OK == HALBlock_Write(DEV_FDL,UPGRADE_FLAG_ADDRESS,&FblWriteData.UpdateFlag[0],64))
    {
        if(64 == HALBlock_Read(DEV_FDL,UPGRADE_FLAG_ADDRESS, &FblReadData.UpdateFlag[0],64))
        {
          if(is_same_arrays(&FblWriteData.UpdateFlag[0], &FblReadData.UpdateFlag[0], 4) == TRUE)
           {
               bUpdateFlagWrEEPOk=TRUE;
               /* if(bSwdlStatusEventOn)
                {
                    bSwdlStatusEventOn = FALSE;
                    OSWDG_SoftReset();
                }*/
           }
	    else
	    {
                bUpdateFlagWrEEPOk=FALSE;
	    }
        }
     }

#else
    U32 upgradeFlag = 0x5AA55AA5;
    U32 readData = 0;
	
    if(ERR_OK == HALBlock_Write(DEV_FDL,UPGRADE_FLAG_ADDRESS,(U8*)&upgradeFlag,UPGRADE_FLAG_LEN_OF_WORD))
    {
        if(UPGRADE_FLAG_LEN_OF_WORD == HALBlock_Read(DEV_FDL,UPGRADE_FLAG_ADDRESS, (U8*)&readData,UPGRADE_FLAG_LEN_OF_WORD))
        {
           if(readData == upgradeFlag)
           {
               bUpdateFlagWrEEPOk=TRUE;
               /* if(bSwdlStatusEventOn)
                {
                    bSwdlStatusEventOn = FALSE;
                    OSWDG_SoftReset();
                }*/
           }
	    else
	    {
                bUpdateFlagWrEEPOk=FALSE;
	    }
        }
     }

#endif
}
  

/**************** END OF FILE *****************************************/

