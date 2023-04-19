#ifndef __PMIC_H__
#define __PMIC_H__

// TODO: 精简 8997 代码
#define MAX8997_ADDR	0xcc    //TA4 PMU- I2C1

typedef enum
{
	eSingle_Buck ,
	eMAXIM ,
	eNS ,
} PMIC_MODEL_et;

typedef enum
{
	eVDD_ARM ,
	eVDD_INT ,
	eVDD_G3D ,
	TA4_PMU  ,
} PMIC_et;

typedef enum
{
	eVID_MODE0 ,
	eVID_MODE1 ,
	eVID_MODE2 ,
	eVID_MODE3,
} PMIC_MODE_et;

typedef struct {
	char*	name;           //voltage name
	u8	ucDIV_Addr;     //device address
	u8	ucI2C_Ch;       //iic ch
}PMIC_Type_st; 

extern void PMIC_InitIp(void);

#ifndef CONFIG_TA4
void PMIC_SetVoltage_Buck(PMIC_et ePMIC_TYPE ,PMIC_MODE_et uVID, float fVoltage_value );
#else
void PMIC_SetVoltage_Buck(PMIC_et ePMIC_TYPE ,u8 addr, u8 value );
void PMIC_GetVoltage_Buck(PMIC_et ePMIC_TYPE ,u8 addr, u8 *value );

#endif

#endif //__PMIC_H__

