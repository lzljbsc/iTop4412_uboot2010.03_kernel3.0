#include <common.h>
#include <common.h>
#include <asm/io.h>

#include "pmic.h"
#include "gpio.h"
#include "i2c.h"

#define Inp32(_addr)            readl(_addr)
#define Outp32(addr, data)      (*(volatile u32 *)(addr) = (data))

const PMIC_Type_st gVoltage_Type[3] = 
{
	
	{"TA4_PMU",	MAX8997_ADDR,	I2C1},

};
#ifdef CONFIG_TA4
extern void lowlevel_init_max8997(unsigned char Address,unsigned char *Val,int flag);
#endif

#define CONFIG_PM_VDD_ARM       1.2
#define CONFIG_PM_VDD_INT       1.0
#define CONFIG_PM_VDD_G3D       1.1
#define CONFIG_PM_VDD_MIF       1.1
#define CONFIG_PM_VDD_LDO14     1.8


typedef enum{	
    PMIC_BUCK1=0,	
    PMIC_BUCK2,	
    PMIC_BUCK3,	
    PMIC_BUCK4,	
    PMIC_LDO14,	
    PMIC_LDO10,
} PMIC_RegNum;


#define CALC_S5M8767_VOLT1(x)  ( (x<600) ? 0 : ((x-600)/6.25) )
#define CALC_S5M8767_VOLT2(x)  ( (x<650) ? 0 : ((x-650)/6.25) )

void I2C_S5M8767_VolSetting(PMIC_RegNum eRegNum, u8 ucVolLevel, u8 ucEnable)
{
	u8 reg_addr, reg_bitpos, reg_bitmask, vol_level;
	u8 read_data;

	reg_bitpos = 0;
	reg_bitmask = 0xFF;
	if(eRegNum == 0)
	{
		reg_addr = 0x33;
	}
	else if(eRegNum == 1)
	{
		reg_addr = 0x35;
	}
	else if(eRegNum == 2)
	{
		reg_addr = 0x3E;
	}
	else if(eRegNum == 3)
	{
		reg_addr = 0x47;
	}
	else if(eRegNum == 4)
	{
		reg_addr = 0x48;
	}
#if  defined(CONFIG_SCP_1GDDR)
	else if(7 == eRegNum)
	{
		reg_addr = 0x59;
		ucVolLevel = 0x40;	// 1.55v
	}
#endif
	else
		while(1);

	vol_level = ucVolLevel&reg_bitmask;
	lowlevel_init_max8997(reg_addr,&vol_level,1);

}

void pmic8767_init(void)
{
	float vdd_arm, vdd_int, vdd_g3d;
	float vdd_mif;
	u8 read_data;

	vdd_arm = CONFIG_PM_VDD_ARM;
	vdd_int = CONFIG_PM_VDD_INT;
	vdd_g3d = CONFIG_PM_VDD_G3D;
	vdd_mif = CONFIG_PM_VDD_MIF;

	I2C_S5M8767_VolSetting(PMIC_BUCK1, CALC_S5M8767_VOLT2(vdd_mif * 1000), 1);
	I2C_S5M8767_VolSetting(PMIC_BUCK2, CALC_S5M8767_VOLT1(vdd_arm * 1000), 1);
	I2C_S5M8767_VolSetting(PMIC_BUCK3, CALC_S5M8767_VOLT1(vdd_int * 1000), 1);
	I2C_S5M8767_VolSetting(PMIC_BUCK4, CALC_S5M8767_VOLT1(vdd_g3d * 1000), 1);

	//I2C_S5M8767_VolSetting(6, CALC_S5M8767_VOLT1(1.5 * 1000), 1);

#if  defined(CONFIG_SCP_1GDDR)
	//set Buck8 to 1.5v, because LDO2's out  decide by Buck8
	I2C_S5M8767_VolSetting(7, CALC_S5M8767_VOLT1(1.55 * 1000), 1);
#endif
}


int Is_TC4_Dvt = 0;

void GPIO_Reset_A(void)
{
	// RESET
	GPIO_SetFunctionEach(eGPIO_X1, eGPIO_2, eGPO); // XEINT[5]
	GPIO_SetPullUpDownEach(eGPIO_X1, eGPIO_2, eGPUDdis); // XEINT[5]	

	GPIO_SetDataEach(eGPIO_X1, eGPIO_2, 0); // XEINT[5]
	GPIO_SetDataEach(eGPIO_X1, eGPIO_2, 1); // XEINT[5]

}

void GPIO_Reset_B(void)
{
	// RESET
	GPIO_SetFunctionEach(eGPIO_X1, eGPIO_0, eGPO); // XEINT[5]
	GPIO_SetPullUpDownEach(eGPIO_X1, eGPIO_0, eGPUDdis); // XEINT[5]	
	
	GPIO_SetDataEach(eGPIO_X1, eGPIO_0, 0); // XEINT[5] 
	GPIO_SetDataEach(eGPIO_X1, eGPIO_0, 1); // XEINT[5]

}

void PMIC_InitIp(void)
{
    u8 id;
    unsigned int val;
    u8 addr;
    u8 uSendData[2];

    /* PMIC S5M8767 实际连接在 I2C0 上， I2C1 未连接 */

    /* GPD1_0 GPD1_1 -> I2C0 */
    GPIO_Init();
    GPIO_SetFunctionEach(eGPIO_D1, eGPIO_0, 2);
    GPIO_SetFunctionEach(eGPIO_D1, eGPIO_1, 2);

    GPIO_SetDSEach(eGPIO_D1,eGPIO_0,3);
    GPIO_SetDSEach(eGPIO_D1,eGPIO_1,3);

    GPIO_SetPullUpDownEach(eGPIO_D1, eGPIO_0, 0);
    GPIO_SetPullUpDownEach(eGPIO_D1, eGPIO_1, 0);

    I2C_InitIp(I2C0, I2C_TX_CLOCK_125KHZ, I2C_TIMEOUT_INFINITY);

    /* GPD1_2 GPD1_3 -> I2C1 */
    GPIO_SetFunctionEach(eGPIO_D1, eGPIO_2, 2);	// GPIO I2C1 setting
    GPIO_SetFunctionEach(eGPIO_D1, eGPIO_3, 2);	// GPIO I2C1 setting

    GPIO_SetDSEach(eGPIO_D1,eGPIO_2,3);
    GPIO_SetDSEach(eGPIO_D1,eGPIO_3,3);
    GPIO_SetPullUpDownEach(eGPIO_D1, eGPIO_2, 0);	// Pull-Up/Down Disable
    GPIO_SetPullUpDownEach(eGPIO_D1, eGPIO_3, 0);	// Pull-Up/Down Disable

    if (I2C_InitIp(I2C1, I2C_TX_CLOCK_125KHZ, I2C_TIMEOUT_INFINITY) != 1) {
        printf("PMIC init filed!\n");
    }

    Is_TC4_Dvt = 0;

    /* 读取 PMIC ID 寄存器，确认型号 */
    lowlevel_init_max8997(0,&id,0);
    if(id == 0x77)
    {
        printf("Max8997 @ TC4 EVT\n");
        Is_TC4_Dvt = 0;
    }
    else if(id == 0x67)//DVT board
    {
        printf("S5M8767(VER2.0)\n");
        Is_TC4_Dvt = 1;
    }
    else if(id == 0x1)
    {
        printf("S5M8767(VER3.0) \n");
        Is_TC4_Dvt = 2;
    }
    else if(id == 0x2)
    {
        printf("S5M8767(VER4.0) \n");
        Is_TC4_Dvt = 2;
    }  
    else if(id == 0x3 || (0x5 == id) || (21 == id))
    {
        printf("S5M8767(VER5.0)\n");
        Is_TC4_Dvt = 2;
    }  
    else
    {
        printf("S5M8767(VER6.0)\n");
        Is_TC4_Dvt = 2;
    }

    if(Is_TC4_Dvt)
    {
        pmic8767_init();
        if(Is_TC4_Dvt == 2)
        {
            val = 0x58;
            lowlevel_init_max8997(0x5a,&val,1);
        }

        /* dg change for kinds of coreboard*/
#if  defined(CONFIG_SCP_1GDDR)
        val = 0x32;
        lowlevel_init_max8997(0x70, &val, 1);
#endif
    }

    /*---mj configure for emmc ---*/
    uSendData[0] =0x6f;
    uSendData[1] =0x68;

    I2C_SendEx(I2C1, 0xcc, NULL, 0, uSendData, 2);

    /* add by cym 20141224 for TP_IOCTL GPX0_3 set low */
    Outp32(GPX0CON,(Inp32(GPX0CON)&(~(0xf << 12)))|(0x1 << 12));
    Outp32(GPX0DAT,(Inp32(GPX0DAT)&(~(0x1 << 3)))|(0x0 << 3));
    /* end add */

    /* add by cym 20150701 7 inch screen twinkle when syatem start */
    Outp32(GPL1DRV,(Inp32(GPL1DRV)&(~(0x3)))|(0x2));
    /* end add */
}

void PMIC_SetVoltage_Buck(PMIC_et ePMIC_TYPE ,u8 addr, u8 value )
{
	u8 uSendData[2];
	uSendData[0] = addr;
	uSendData[1] = value;
	I2C_SendEx(gVoltage_Type[ePMIC_TYPE].ucI2C_Ch, gVoltage_Type[ePMIC_TYPE].ucDIV_Addr, NULL, 0, uSendData, 2);
}

void PMIC_GetVoltage_Buck(PMIC_et ePMIC_TYPE ,u8 addr, u8 *value )
{
	u8 uSendData[2];
	u8 uGetData[2];
	
	uSendData[0] = addr;
	//uSendData[1] = value;
	I2C_RecvEx(gVoltage_Type[ePMIC_TYPE].ucI2C_Ch, gVoltage_Type[ePMIC_TYPE].ucDIV_Addr, uSendData, 1, uGetData, 1);
	*value = uGetData[0];
}		


