#include <stdio.h>
#include <malloc.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/io.h>
#include <sys/stat.h>
#include <cpuid.h>

#ifndef MIN
	#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
	#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

static int fileExists(const char *path)
{
	struct stat buffer;

	return (stat(path, &buffer) == 0);
}

static int ReadFileContent(const char *pName, char *pBuf, int len)
{
	FILE *fp = fopen(pName, "rt");

	if (fp)
	{
		fread(pBuf, 1, len, fp);
		fclose(fp);
		for (int i = 0; i < len; i++)
		{
			if (pBuf[i] == '\n' || pBuf[i] == '\r')
			{
				pBuf[i] = 0;
				break;
			} 
		}
		return 1;
	}
	return 0;
}

enum CpuVendor
{
	cvUnknown,
	cvIntel,
	cvAMD,
};
static enum CpuVendor g_CpuVendor = cvUnknown;

static void GetCpuVender()
{
	unsigned int level = 0;
	unsigned int eax = 0;
	unsigned int ebx = 0;
	unsigned int ecx = 0;
	unsigned int edx = 0;

	__get_cpuid(level, &eax, &ebx, &ecx, &edx);
	if (ebx == 0x756e6547) g_CpuVendor = cvIntel; /* Genu */
	else if (ebx == 0x68747541) g_CpuVendor = cvAMD; /* Auth */
}

// io port

static uint8_t IoRead8(uint16_t Port)
{
	return inb(Port);
}

static uint8_t IoWrite8(uint16_t Port, uint8_t Value)
{
	outb(Value, Port);
	return Value;
}

static uint8_t ECRamOperate(uint8_t isWin4, uint8_t isRead, uint16_t address, uint8_t data)
{
	uint8_t reg_addr = isWin4 ? 0x2E : 0x4E;
	uint8_t reg_data = isWin4 ? 0x2F : 0x4F;
	uint8_t addr_upper = (uint8_t)((address >> 8) & 255);
	uint8_t addr_lower = (uint8_t)(address & 255);

	if (ioperm(reg_addr, 2, 1)) { perror("ioperm"); exit(1); }

	IoWrite8(reg_addr, 46);
	IoWrite8(reg_data, 16);
	IoWrite8(reg_addr, 47);
	IoWrite8(reg_data, addr_lower);

	IoWrite8(reg_addr, 46);
	IoWrite8(reg_data, 17);
	IoWrite8(reg_addr, 47);
	IoWrite8(reg_data, addr_upper);

	IoWrite8(reg_addr, 46);
	IoWrite8(reg_data, 18);
	IoWrite8(reg_addr, 47);
	if (isRead) data = IoRead8(reg_data);
	else IoWrite8(reg_data, data);
	return data;
}

// Embedded Controller

static uint8_t EC_OBF = 0x01;  // Output Buffer Full
static uint8_t EC_IBF = 0x02;  // Input Buffer Full
static uint8_t EC_DATA = 0x62; // Data Port
static uint8_t EC_SC = 0x66;   // Status/Command Port
static uint8_t RD_EC = 0x80;   // Read Embedded Controller
static uint8_t WR_EC = 0x81;   // Write Embedded Controller

static uint16_t EC_TMEOUT = 100;
static uint16_t EC_RETRY = 5;

static uint8_t EcStatus(uint8_t flag)
{
	uint8_t done = flag == EC_OBF ? 0x01 : 0x00;

	for (uint16_t i = 0; i < EC_TMEOUT; i++)
	{
		uint8_t result = IoRead8(EC_SC);

		// First and second bit of returned value represent
		// the status of OBF and IBF flags respectively
		if (((done ? ~result : result) & flag) == 0) return 1;
		else usleep(1000);
	}

	return 0;
}

static uint8_t EcOperate(uint8_t IsRead, uint8_t bRegister, uint8_t *pValue)
{
	uint8_t operationType = IsRead ? RD_EC : WR_EC;

	for (uint16_t i = 0; i < EC_RETRY; i++)
	{
		if (EcStatus(EC_IBF)) // Wait until IBF is free
		{
			IoWrite8(EC_SC, operationType); // Write operation type to the Status/Command port
			if (EcStatus(EC_IBF)) // Wait until IBF is free
			{
				IoWrite8(EC_DATA, bRegister); // Write register address to the Data port
				if (EcStatus(EC_IBF)) // Wait until IBF is free
				{
					if (IsRead)
					{
						if (EcStatus(EC_OBF)) // Wait until OBF is full
						{
							*pValue = IoRead8(EC_DATA); // Read from the Data port
							return 1;
						}
					}
					else
					{
						IoWrite8(EC_DATA, *pValue); // Write to the Data port
						return 1;
					}
				}
			}
		}
	}
	return 0;
}

static uint8_t EcRead(uint8_t bRegister, uint8_t *pValue)
{
	return EcOperate(1, bRegister, pValue);
}

static uint8_t EcWrite(uint8_t bRegister, uint8_t value)
{
	return EcOperate(0, bRegister, &value);
}

static void EcChangeMode(uint8_t Mode)
{
	EcWrite(0xbf, Mode);
	usleep(10000);
	EcWrite(0xbf, 0xff);
}

static void EcCmd(uint8_t cmd, uint8_t p1, uint8_t p2)
{
	EcWrite(0x6d, cmd);
	EcWrite(0xb1, p1);
	EcWrite(0xb2, p2);
	EcWrite(0xbf, 0x10);
	usleep(10000);
	EcWrite(0xbf, 0xff);
	usleep(10000);
}

static void EcSetPixelSub(uint8_t js, uint8_t led, uint8_t r, uint8_t g, uint8_t b)
{
	EcCmd(js, led * 3, r);
	EcCmd(js, led * 3 + 1, g);
	EcCmd(js, led * 3 + 2, b);
}

static const uint8_t LEFT_JOYSTICK = 1;
static const uint8_t RIGHT_JOYSTICK = 2;

static const uint8_t RIGHT_LED = 1;
static const uint8_t BOTTOM_LED = 2;
static const uint8_t LEFT_LED = 3;
static const uint8_t TOP_LED = 4;

static void EcSetPixel(uint8_t r, uint8_t g, uint8_t b)
{
	EcSetPixelSub(LEFT_JOYSTICK, RIGHT_LED, r, g, b);
	EcSetPixelSub(LEFT_JOYSTICK, BOTTOM_LED, r, g, b);
	EcSetPixelSub(LEFT_JOYSTICK, LEFT_LED, r, g, b);
	EcSetPixelSub(LEFT_JOYSTICK, TOP_LED, r, g, b);

	EcSetPixelSub(RIGHT_JOYSTICK, RIGHT_LED, r, g, b);
	EcSetPixelSub(RIGHT_JOYSTICK, BOTTOM_LED, r, g, b);
	EcSetPixelSub(RIGHT_JOYSTICK, LEFT_LED, r, g, b);
	EcSetPixelSub(RIGHT_JOYSTICK, TOP_LED, r, g, b);
}

enum SupportDevice
{
	sdNone,
	sdAOKZOE_A1,
	sdAOKZOE_A1Pro,
	sdAYANEO_2,
	sdAYANEO_2021,
	sdAYANEO_2021Pro,
	sdAYANEO_AIR,
	sdAYANEO_AIRLite,
	sdAYANEO_AIRPro,
	sdAYANEO_AIRPlus,
	sdAYANEO_AIR1S,
	sdAYANEO_NEXT,
	sdAYANEO_KUN,
	sdAYANEO_SLIDER,
	sdAYANEO_FLIP,
	sdGPD_WinMax2AMD,
	sdGPD_WinMax2Intel,
	sdGPD_Win4,
	sdGPD_WinMini,
	sdOne_XPlayerMiniAMD,
	sdOne_XPlayerMiniIntel,
	sdOne_XPlayerMiniPro,
	sdOne_XPlayer2,
	sdASUS_RogAlly,
};
static enum SupportDevice g_SupportDevice = sdNone;
static int g_SubModel = 0;

static int DetectSupportModel(char *ManufacturerName, char *ProductName, char *Version)
{
	if (ManufacturerName && ProductName && Version)
	{
		if (strcasecmp(ManufacturerName, "AOKZOE") == 0)
		{
			if (strcasecmp(ProductName, "AOKZOE A1 AR07") == 0) g_SupportDevice = sdAOKZOE_A1;
			else if (strcasecmp(ProductName, "AOKZOE A1 Pro") == 0) g_SupportDevice = sdAOKZOE_A1Pro;
		}
		else if (strcasecmp(ManufacturerName, "AYADEVICE") == 0 || strcasecmp(ManufacturerName, "AYANEO") == 0)
		{
			if (strcasecmp(ProductName, "AIR") == 0) g_SupportDevice = sdAYANEO_AIR;
			else if (strcasecmp(ProductName, "AIR Lite") == 0) g_SupportDevice = sdAYANEO_AIRLite;
			else if (strcasecmp(ProductName, "AIR Pro") == 0) g_SupportDevice = sdAYANEO_AIRPro;
			else if (strcasecmp(ProductName, "AIR Plus") == 0) g_SupportDevice = sdAYANEO_AIRPlus;
			else if (strcasecmp(ProductName, "AB05-Mendocino") == 0 || strcasecmp(ProductName, "AB05-Intel") == 0)
			{
				g_SupportDevice = sdAYANEO_AIRPlus;
				g_SubModel = 1;
			}			
			else if (strcasecmp(ProductName, "AIR 1S") == 0) g_SupportDevice = sdAYANEO_AIR1S;
			else if (strcasecmp(ProductName, "AYA NEO FOUNDER") == 0 || strcasecmp(ProductName, "AYANEO 2021") == 0) g_SupportDevice = sdAYANEO_2021;
			else if (strcasecmp(ProductName, "AYANEO 2021 Pro") == 0 || strcasecmp(ProductName, "AYANEO 2021 Retro Power") == 0) g_SupportDevice = sdAYANEO_2021Pro;
			else if (strcasecmp(ProductName, "NEXT") == 0 || strcasecmp(ProductName,  "NEXT Pro") == 0 || strcasecmp(ProductName, "NEXT Advance") == 0) g_SupportDevice = sdAYANEO_NEXT;
			else if (strcasecmp(ProductName, "AYANEO 2") == 0 || strcasecmp(ProductName, "GEEK") == 0) g_SupportDevice = sdAYANEO_2;
			else if (strcasecmp(ProductName, "KUN") == 0) g_SupportDevice = sdAYANEO_KUN;
			else if (strcasecmp(ProductName, "AS01") == 0) g_SupportDevice = sdAYANEO_SLIDER;
			else if (strcasecmp(ProductName, "FLIP DS") == 0 || strcasecmp(ProductName, "FLIP KB") == 0) g_SupportDevice = sdAYANEO_FLIP;			
		}
		else if (strcasecmp(ManufacturerName, "GPD") == 0)
		{
			if (strcasecmp(ProductName, "G1619-03") == 0) g_SupportDevice = sdGPD_WinMax2Intel;
			else if (strcasecmp(ProductName, "G1619-04") == 0) g_SupportDevice = sdGPD_WinMax2AMD;
			else if (strcasecmp(ProductName, "G1618-04") == 0) g_SupportDevice = sdGPD_Win4;
			else if (strcasecmp(ProductName, "G1617-01") == 0) g_SupportDevice = sdGPD_WinMini;
		}
		else if (strcasecmp(ManufacturerName, "ONE-NETBOOK TECHNOLOGY CO., LTD.") == 0 || strcasecmp(ManufacturerName, "ONE-NETBOOK") == 0)
		{
			if (strcasecmp(ProductName, "ONE XPLAYER") == 0 || strcasecmp(ProductName, "ONEXPLAYER Mini Pro") == 0)
			{
				if (strcasecmp(Version, "V01") == 0) g_SupportDevice = sdOne_XPlayerMiniAMD;
				else if (strcasecmp(Version, "1002-C") == 0) g_SupportDevice = sdOne_XPlayerMiniIntel;
				else if (strcasecmp(Version, "V03") == 0) g_SupportDevice = sdOne_XPlayerMiniPro;				
			}
			else if (strcasecmp(ProductName, "ONE XPLAYER2") == 0) g_SupportDevice = sdOne_XPlayer2;
		}
		else if (strcasecmp(ManufacturerName, "ASUSTEK COMPUTER INC.") == 0)
		{
			if (strstr(ProductName, "RC71L")) g_SupportDevice = sdASUS_RogAlly;
		}
	}
	return g_SupportDevice > sdNone;
}

enum FanControlType
{
	fctNotDetect = -1,
	fctNone,
	fctOneXPlayer,
	fctOneXPlayerMini,
	fctOneXPlayer2,
	fctAyaNeo2,
	fctAyaNeoAir,
	fctAyaNeoAir1S,
	fctAyaNeoAirPlus,
	fctAyaNeoKun,
	fctAyaNeoSlider,
	fctAyaNeoFlip,
	fctGpdWinMax2,
	fctGpdWin4,
	fctGpdWinMini,
};
static enum FanControlType g_FanControlType = fctNotDetect;

static int CheckFanEnable(int Force)
{
	if (Force || g_FanControlType == fctNotDetect)
	{
		// fctOneXPlayer
		if (g_SupportDevice == sdOne_XPlayerMiniIntel || g_SupportDevice == sdOne_XPlayerMiniAMD) g_FanControlType = fctOneXPlayerMini;
		else if (g_SupportDevice == sdOne_XPlayerMiniPro || g_SupportDevice == sdAOKZOE_A1 || g_SupportDevice == sdAOKZOE_A1Pro) g_FanControlType = fctOneXPlayerMini;
		else if (g_SupportDevice == sdOne_XPlayer2) g_FanControlType = fctOneXPlayer2;
		else if (g_SupportDevice == sdAYANEO_2) g_FanControlType = fctAyaNeo2;
		else if (g_SupportDevice == sdAYANEO_AIR || g_SupportDevice == sdAYANEO_AIRLite || g_SupportDevice == sdAYANEO_AIRPro) g_FanControlType = fctAyaNeoAir;
		else if (g_SupportDevice == sdAYANEO_AIR1S) g_FanControlType = fctAyaNeoAir1S;
		else if (g_SupportDevice == sdAYANEO_AIRPlus) g_FanControlType = fctAyaNeoAirPlus;
		else if (g_SupportDevice == sdAYANEO_KUN) g_FanControlType = fctAyaNeoKun;
		else if (g_SupportDevice == sdAYANEO_SLIDER) g_FanControlType = fctAyaNeoSlider;
		else if (g_SupportDevice == sdAYANEO_FLIP) g_FanControlType = fctAyaNeoFlip;
		else if (g_SupportDevice == sdGPD_WinMax2Intel || g_SupportDevice == sdGPD_WinMax2AMD) g_FanControlType = fctGpdWinMax2;
		else if (g_SupportDevice == sdGPD_Win4)
		{
			g_FanControlType = fctGpdWin4;

			// magic code
			ECRamOperate(1, 1, 0x20, 0x00);
			ECRamOperate(1, 1, 0x1060, 0x00);
			ECRamOperate(1, 0, 0x1060, 0x80);
			//ECRamOperate(1, 0, 0xC311, 0x00); // auto fan
			//ECRamOperate(1, 1, 0x1841, 0x00);
			if (ECRamOperate(1, 1, 0x2000, 0) == 0x55)
			{
				uint8_t EC_Chip_Ver = ECRamOperate(1, 1, 0x1060, 0);
				
				EC_Chip_Ver = EC_Chip_Ver | 0x80;
				ECRamOperate(1, 0, 0x1060, EC_Chip_Ver);
			}
		}
		else if (g_SupportDevice == sdGPD_WinMini)
		{
			g_FanControlType = fctGpdWinMini;
		}
	}
	return g_FanControlType >= fctOneXPlayer;
}

static int CheckModelSub()
{
	char ManufacturerName[110] = { 0, };
	char ProductName[110] = { 0, };
	char Version[110] = { 0, };

	ReadFileContent("/sys/devices/virtual/dmi/id/sys_vendor", ManufacturerName, 100);
	ReadFileContent("/sys/devices/virtual/dmi/id/product_name", ProductName, 100);
	ReadFileContent("/sys/devices/virtual/dmi/id/product_version", Version, 100);
	if (DetectSupportModel(ManufacturerName, ProductName, Version) <= 0) return -1;

	if (ioperm(EC_DATA, 2, 1)) return -2;
	if (ioperm(EC_SC, 2, 1)) return -3;
	if (CheckFanEnable(0))
	{
		if (g_FanControlType == fctGpdWin4)
		{
			if (ioperm(0x2E, 2, 1)) return -4;
		}
		else
		{
			if (ioperm(0x4E, 2, 1)) return -4;
		}
	}
	return 0;
}

static int CheckModel()
{
	static int ret = CheckModelSub();

	return ret;
}

static void AyaNeoLed(int Off)
{
	if (Off)
	{
		if (g_FanControlType == fctAyaNeoAir || g_FanControlType == fctAyaNeoAir1S || g_FanControlType == fctAyaNeo2)
		{
			EcCmd(0x03, 0x02, 0x00); // 밝기 끄기
			EcCmd(0x03, 0x02, 0xc0); // LED 완전히 끄기
		}
		else if (g_FanControlType == fctAyaNeoAirPlus)
		{
			ECRamOperate(0, 0, 0xd187, 0xa5);
			ECRamOperate(0, 0, 0xd1b2, 0x31);
			ECRamOperate(0, 0, 0xd1c6, 0x01);
			ECRamOperate(0, 0, 0xd172, 0x31);
			ECRamOperate(0, 0, 0xd186, 0x01);
			ECRamOperate(0, 0, 0xd187, 0xa5);
	//		ECRamOperate(0, 0, 0xd170, 0x00);
	//		ECRamOperate(0, 0, 0xd186, 0x01);
		}
	}
	else
	{
		if (g_FanControlType == fctAyaNeoAir || g_FanControlType == fctAyaNeoAir1S || g_FanControlType == fctAyaNeo2)
		{
			int isCharge = fileExists("/sys/class/power_supply/ADP0/online") || fileExists("/sys/class/power_supply/ADP1/online");
			
			if (isCharge) EcChangeMode(0xe0);  // 약한 빨간색
			else EcChangeMode(0xe3); // 약한 하늘색 
//			EcChangeMode(0xe1);			// 약한 녹색
// 			EcChangeMode(0xe2);			// 약한 파란색
//			EcCmd(0x03, 0x02, 0x40);	// 약한 밝기
			EcCmd(0x03, 0x02, 0x80);	// 강한 밝기
//			EcSetPixel(0xff, 0x00, 0xff);
		}
		else if (g_FanControlType == fctAyaNeoAirPlus)
		{
			ECRamOperate(0, 0, 0xd187, 0xa5);
			ECRamOperate(0, 0, 0xd1b2, 0x31);
			ECRamOperate(0, 0, 0xd1c6, 0x01);

			ECRamOperate(0, 0, 0xd187, 0xa5);
			ECRamOperate(0, 0, 0xd172, 0x31);
			ECRamOperate(0, 0, 0xd186, 0x01);

			ECRamOperate(0, 0, 0xd187, 0xa5);
			ECRamOperate(0, 0, 0xd170, 0x00);
			ECRamOperate(0, 0, 0xd186, 0x01);
			ECRamOperate(0, 0, 0xd160, 0x80);
		}
	}
}

extern "C"
int GetFanValueType()
{
	if (g_FanControlType == fctAyaNeoAirPlus || g_FanControlType == fctAyaNeoKun || g_FanControlType == fctAyaNeoSlider) return 0; // don't support
	else if (g_FanControlType == fctGpdWinMax2 || g_FanControlType == fctGpdWin4 || g_FanControlType == fctGpdWinMini) return 1; // rpm
	else if (g_FanControlType > fctNone) return 2; // percent
	return -1;
}

extern "C"
int GetFanValue()
{
	if (g_FanControlType == fctAyaNeo2 || g_FanControlType == fctAyaNeoAir1S || g_FanControlType == fctAyaNeoKun || g_FanControlType == fctAyaNeoFlip)
	{
		int val = ECRamOperate(0, 1, 0x1809, 0);

		return MIN(val * 100 / 255, 100);
	}
	else if (g_FanControlType == fctAyaNeoAir)
	{
		int val = ECRamOperate(0, 1, 0x1803, 0);

		return MIN(val/* * 100 / 255*/, 100);
	}
	else if (g_FanControlType == fctAyaNeoAirPlus) // I don't know!!
	{
		int rpm = ECRamOperate(0, 1, 0x1821, 0);
		uint8_t low = ECRamOperate(0, 1, 0x1820, 0);

		return 0xfff - ((rpm << 8) + low);
	}
	else if (g_FanControlType == fctOneXPlayer)
	{
		uint8_t Data = 0;

		if (EcRead(0x4B, &Data))
		{
			char Vender[16] = { 0, };

			if (g_CpuVendor == cvIntel) Data = Data * 100 / 255;
			if (Data > 100) Data = 100;
			return Data;
		}
	}
	else if (g_FanControlType == fctOneXPlayer2)
	{
		int val = ECRamOperate(0, 1, 0x1809, 0);

		return val * 100 / 184;
	}
	else if (g_FanControlType == fctOneXPlayerMini)
	{
		int val = ECRamOperate(0, 1, 0x1809, 0);

//		return val * 100 / 184;
		return MIN(val * 100 / 255, 100);
	}
	else if (g_FanControlType == fctGpdWinMax2)
	{
		int rpm = ECRamOperate(0, 1, 0x218, 0);
		uint8_t low = ECRamOperate(0, 1, 0x219, 0);

		return (rpm << 8) | low;
	}
	else if (g_FanControlType == fctGpdWin4)
	{
		int rpm = ECRamOperate(1, 1, 0xC880, 0);
		uint8_t low = ECRamOperate(1, 1, 0xC881, 0);

		return (rpm << 8) | low;
	}
	else if (g_FanControlType == fctGpdWinMini)
	{
		int rpm = ECRamOperate(0, 1, 0x478, 0);
		uint8_t low = ECRamOperate(0, 1, 0x479, 0);

		return (rpm << 8) | low;
	}

	return -1;
}

extern "C"
void UpdateFanControl(int FanSpeed)
{
	int max = -1;
	int fan = -1;

	if (g_FanControlType == fctAyaNeo2) max = 100;
	else if (g_FanControlType == fctAyaNeoAir || g_FanControlType == fctAyaNeoAir1S || g_FanControlType == fctAyaNeoKun || g_FanControlType == fctAyaNeoFlip) max = 100;
	else if (g_FanControlType == fctAyaNeoAirPlus || g_FanControlType == fctAyaNeoSlider) max = 255;
	else if (g_FanControlType == fctOneXPlayer) max = g_CpuVendor == cvIntel ? 255 : 100;
	else if (g_FanControlType == fctOneXPlayer2) max = 184;
	else if (g_FanControlType == fctOneXPlayerMini) max = 255;
	else if (g_FanControlType == fctGpdWinMax2) max = 184;
	else if (g_FanControlType == fctGpdWin4) max = 127;
	else if (g_FanControlType == fctGpdWinMini) max = 244;

	if (max > 0) fan = max * FanSpeed / 100;
	if (fan >= 0)
	{
		if (g_FanControlType == fctAyaNeo2) ECRamOperate(0, 0, 0x44B, fan);
		else if (g_FanControlType == fctAyaNeoAir || g_FanControlType == fctAyaNeoAir1S || g_FanControlType == fctAyaNeoKun || g_FanControlType == fctAyaNeoFlip) ECRamOperate(0, 0, 0x44B, fan);
		else if (g_FanControlType == fctAyaNeoAirPlus || g_FanControlType == fctAyaNeoSlider)
		{
			if (g_SubModel == 1) ECRamOperate(0, 0, 0x1802, fan);
			else ECRamOperate(0, 0, 0x1804, fan);
		}
		else if (g_FanControlType == fctOneXPlayer) EcWrite(0x4B, fan);
		else if (g_FanControlType == fctOneXPlayer2) ECRamOperate(0, 0, 0x44B, fan);
		else if (g_FanControlType == fctOneXPlayerMini) ECRamOperate(0, 0, 0x44B, fan);
		else if (g_FanControlType == fctGpdWinMax2) ECRamOperate(0, 0, 0x1809, fan);
		else if (g_FanControlType == fctGpdWin4) ECRamOperate(1, 0, 0xC311, fan < 1 ? 1 : fan);
		else if (g_FanControlType == fctGpdWinMini) ECRamOperate(0, 0, 0x47a, fan);
	}
}

extern "C"
void SetFanControlManual(int Manual)
{
	if (g_FanControlType == fctAyaNeo2) ECRamOperate(0, 0, 0x44A, Manual == 0 ? 0x00 : 0x01);
	else if (g_FanControlType == fctAyaNeoAir || g_FanControlType == fctAyaNeoAir1S || g_FanControlType == fctAyaNeoKun || g_FanControlType == fctAyaNeoFlip) ECRamOperate(0, 0, 0x44A, Manual == 0 ? 0x00 : 0x01);
	else if (g_FanControlType == fctAyaNeoAirPlus || g_FanControlType == fctAyaNeoSlider) ECRamOperate(0, 0, 0xd1c8, Manual == 0 ? 0x00 : 0xa5);
	else if (g_FanControlType == fctOneXPlayer) EcWrite(0x4A, Manual == 0 ? 0x00 : 0x01);
	else if (g_FanControlType == fctOneXPlayer2) ECRamOperate(0, 0, 0x44A, Manual == 0 ? 0x00 : 0x01);
	else if (g_FanControlType == fctOneXPlayerMini) ECRamOperate(0, 0, 0x44A, Manual == 0 ? 0x00 : 0x01);
	else if (g_FanControlType == fctGpdWinMax2) ECRamOperate(0, 0, 0x275, Manual == 0 ? 0x00 : 0x01);
	else if (g_FanControlType == fctGpdWin4)
	{
		if (Manual == 0) ECRamOperate(1, 0, 0xC311, 0x00);
	}
	else if (g_FanControlType == fctGpdWinMini)
	{
		if (Manual == 0) ECRamOperate(0, 0, 0x47a, 0x00);
	}
	if (Manual) UpdateFanControl(30);
}

// cpu temp code from mangohud(https://github.com/flightlessmango/MangoHud)
#include <dirent.h>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>

static bool starts_with(const std::string& s,  const char *t)
{
    return s.rfind(t, 0) == 0;
}

static bool ends_with(const std::string& s,  const char *t, bool icase = false)
{
    std::string s0(s);
    std::string s1(t);

    if (s0.size() < s1.size())
        return false;

    if (icase)
	{
        std::transform(s0.begin(), s0.end(), s0.begin(), ::tolower);
        std::transform(s1.begin(), s1.end(), s1.begin(), ::tolower);
    }

    size_t pos = s0.size() - s1.size();
    return (s0.rfind(s1, pos) == pos);
}

enum LS_FLAGS
{
    LS_DIRS = 0x01,
    LS_FILES = 0x02,
};

static std::vector<std::string> ls(const char* root, const char* prefix = nullptr, LS_FLAGS flags = LS_DIRS)
{
    std::vector<std::string> list;
    struct dirent* dp;

    DIR* dirp = opendir(root);
    if (!dirp)
	{
        printf("Error opening directory '%s': %s\n", root, strerror(errno));
        return list;
    }

    while ((dp = readdir(dirp)))
	{
        if ((prefix && !starts_with(dp->d_name, prefix))
            || !strcmp(dp->d_name, ".")
            || !strcmp(dp->d_name, ".."))
            continue;

        switch (dp->d_type)
		{
        case DT_LNK:
		{
            struct stat s;
            std::string path(root);
            if (path.back() != '/')
                path += "/";
            path += dp->d_name;

            if (stat(path.c_str(), &s))
                continue;

            if (((flags & LS_DIRS) && S_ISDIR(s.st_mode))
                || ((flags & LS_FILES) && S_ISREG(s.st_mode)))
			{
                list.push_back(dp->d_name);
            }
            break;
        }
        case DT_DIR:
            if (flags & LS_DIRS)
                list.push_back(dp->d_name);
            break;
        case DT_REG:
            if (flags & LS_FILES)
                list.push_back(dp->d_name);
            break;
        }
    }

    closedir(dirp);
    return list;
}

static std::string read_line(const std::string& filename)
{
    std::string line;
    std::ifstream file(filename);
    if (file.fail()){
        return line;
    }
    std::getline(file, line);
    return line;
}

static bool file_exists(const std::string& path)
{
    struct stat s;
    return !stat(path.c_str(), &s) && !S_ISDIR(s.st_mode);
}

static bool find_input(const std::string& path, const char* input_prefix, std::string& input, const std::string& name)
{
    auto files = ls(path.c_str(), input_prefix, LS_FILES);
    for (auto& file : files)
	{
        if (!ends_with(file, "_label"))
            continue;

        auto label = read_line(path + "/" + file);
        if (label != name)
            continue;

        auto uscore = file.find_first_of("_");
        if (uscore != std::string::npos)
		{
            file.erase(uscore, std::string::npos);
            input = path + "/" + file + "_input";
            return true;
        }
    }
    return false;
}

static bool find_fallback_input(const std::string& path, const char* input_prefix, std::string& input)
{
    auto files = ls(path.c_str(), input_prefix, LS_FILES);
    if (!files.size())
        return false;

    std::sort(files.begin(), files.end());
    for (auto& file : files)
	{
        if (!ends_with(file, "_input"))
            continue;
        input = path + "/" + file;
		printf("fallback cpu %s input: %s\n", input_prefix, input.c_str());
        return true;
    }
    return false;
}

static FILE *GetCpuFile()
{
    std::string name, path, input;
    std::string hwmon = "/sys/class/hwmon/";

    auto dirs = ls(hwmon.c_str());
    for (auto& dir : dirs)
	{
        path = hwmon + dir;
        name = read_line(path + "/name");

        printf("hwmon: sensor name: %s\n", name.c_str());
        if (name == "coretemp")
		{
            find_input(path, "temp", input, "Package id 0");
            break;
        }
        else if ((name == "zenpower" || name == "k10temp"))
		{
            if (!find_input(path, "temp", input, "Tdie"))
                find_input(path, "temp", input, "Tctl");
            break;
        }
		else if (name == "atk0110")
		{
            find_input(path, "temp", input, "CPU Temperature");
            break;
        }
		else if (name == "it8603")
		{
            find_input(path, "temp", input, "temp1");
            break;
        }
		else if (starts_with(name, "nct"))
		{
            // Only break if nct module has TSI0_TEMP node
            if (find_input(path, "temp", input, "TSI0_TEMP"))
                break;

        } else if (name == "asusec")
		{
            find_input(path, "temp", input, "CPU");
            break;
        }
		else
		{
            path.clear();
        }
    }
    if (path.empty() || (!file_exists(input) && !find_fallback_input(path, "temp", input)))
	{
        printf("Could not find cpu temp sensor location\n");
        return NULL;
    }
	else
	{
        printf("hwmon: using input: %s\n", input.c_str());
        return fopen(input.c_str(), "r");
    }
}

static FILE *g_TempFile = NULL;

extern "C"
int GetCpuTemp()
{
	if (g_TempFile)
	{
		int temp = -1;

		rewind(g_TempFile);
		fflush(g_TempFile);
		if (fscanf(g_TempFile, "%d", &temp) == 1) return temp / 1000;
	}
	return -1;
}

extern "C"
int InitModel()
{
	if (!g_TempFile) g_TempFile = GetCpuFile();	
	GetCpuVender();
	return CheckModel();
}

int main(int argc, char* argv[])
{
	int ret = CheckModel();

	if (ret < 0)
	{
		if (ret == -1) perror("cannot detect model");
		else perror("ioperm");	
		exit(1);
	}
	GetCpuVender();

	// FAN control to auto
	SetFanControlManual(0);

	// AyaNeo LED
	if (argc >= 2 && strcmp(argv[1], "on") == 0) AyaNeoLed(1);
	else if (argc >= 2 && strcmp(argv[1], "off") == 0) AyaNeoLed(0);
	return 0;
}
