// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <driver/int.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include <driver/jpeg_enc.h>
#include <driver/jpeg_enc_types.h>
#include <driver/dvp_camera.h>
#include <driver/dvp_camera_types.h>

#include <driver/i2c.h>

#include "dvp_sensor_devices.h"

#define HM1055_WRITE_ADDRESS (0x48)
#define HM1055_READ_ADDRESS (0x49)

#define HM1055_CHIP_ID (0x0955)

#define TAG "hm1055"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)

#define SENSOR_I2C_READ(reg, value) \
	do {\
		dvp_camera_i2c_read_uint16((HM1055_WRITE_ADDRESS >> 1), reg, value);\
	}while (0)

#define SENSOR_I2C_WRITE(reg, value) \
	do {\
		dvp_camera_i2c_write_uint16((HM1055_WRITE_ADDRESS >> 1), reg, value);\
	}while (0)

bool hm1055_read_flag = false;

// HM_1055_DEV
/*MCLK = 60MHz 5fps default*/
const uint16_t sensor_hm1055_init_talbe[][2] =
{
	{0x0022, 0x00},
	{0x0020, 0x00},
	{0x0025, 0x00}, //PLL ON
	{0x0026, 0x97}, //PCLK=48MHz,
	{0x0027, 0x30},
	{0x0028, 0xC0},
	{0x002A, 0x44},
	{0x002B, 0x01},
	{0x002C, 0x00}, //PLL2[7]=0
	{0x0004, 0x10},
	{0x0006, 0x00}, // 0x00
	{0x000F, 0x18}, //Fixed raw length and Fixed frame rate//0x18
	{0x0012, 0x01},
	{0x0013, 0x02},
	{0x0044, 0x04},
	{0x004A, 0x2A},
	{0x004B, 0x72},
	{0x004E, 0x30},
	{0x0070, 0x2A},
	{0x0071, 0x57},
	{0x0072, 0x55},
	{0x0073, 0x30},
	{0x0077, 0x04},
	{0x0080, 0xC2},
	{0x0082, 0xA2},
	{0x0083, 0xF0},
	{0x0085, 0x11},
	{0x0086, 0x22},
	{0x0087, 0x08},
	{0x0088, 0x6e},
	{0x0089, 0x2A},
	{0x008A, 0x2F},
	{0x008D, 0x20},
	{0x008f, 0x77},
	{0x0090, 0x01},
	{0x0091, 0x02},
	{0x0092, 0x03},
	{0x0093, 0x04},
	{0x0094, 0x14},
	{0x0095, 0x09},
	{0x0096, 0x0A},
	{0x0097, 0x0B},
	{0x0098, 0x0C},
	{0x0099, 0x04},
	{0x009A, 0x14},
	{0x009B, 0x34},
	{0x00A0, 0x00},
	{0x00A1, 0x00},
	{0x0B3B, 0x0B},
	{0x0040, 0x0A},
	{0x0053, 0x0A},
	{0x0120, 0x36},
	{0x0121, 0x80},
	{0x0122, 0xEB},
	{0x0123, 0xCC},
	{0x0124, 0xDE},
	{0x0125, 0xDF},
	{0x0126, 0x70},
	{0x0128, 0x1F},
	{0x0129, 0x8F},
	{0x0132, 0xF8},
	{0x011F, 0x08},
	{0x0144, 0x04},
	{0x0145, 0x00},
	{0x0146, 0x20},
	{0x0147, 0x20},
	{0x0148, 0x14},
	{0x0149, 0x14},
	{0x0156, 0x0C},
	{0x0157, 0x0C},
	{0x0158, 0x0A},
	{0x0159, 0x0A},
	{0x015A, 0x03},
	{0x015B, 0x40},
	{0x015C, 0x21},
	{0x015E, 0x0F},
	{0x0168, 0xC8},
	{0x0169, 0xC8},
	{0x016A, 0x96},
	{0x016B, 0x96},
	{0x016C, 0x64},
	{0x016D, 0x64},
	{0x016E, 0x32},
	{0x016F, 0x32},
	{0x01EF, 0xF1},
	{0x0131, 0x44},
	{0x014C, 0x60},
	{0x014D, 0x24},
	{0x015D, 0x90},
	{0x01D8, 0x40},
	{0x01D9, 0x20},
	{0x01DA, 0x23},
	{0x0150, 0x05},
	{0x0155, 0x07},
	{0x0178, 0x10},
	{0x017A, 0x10},
	{0x01BA, 0x10},
	{0x0176, 0x00},
	{0x0179, 0x10},
	{0x017B, 0x10},
	{0x01BB, 0x10},
	{0x0177, 0x00},
	{0x01E7, 0x20},
	{0x01E8, 0x30},
	{0x01E9, 0x50},
	{0x01E4, 0x1A},
	{0x01E5, 0x20},
	{0x01E6, 0x04},
	{0x0210, 0x21},
	{0x0211, 0x0A},
	{0x0212, 0x21},
	{0x01DB, 0x04},
	{0x01DC, 0x14},
	{0x0151, 0x08},
	{0x01F2, 0x18},
	{0x01F8, 0x3C},
	{0x01FE, 0x24},
	{0x0213, 0x03},
	{0x0214, 0x03},
	{0x0215, 0x10},
	{0x0216, 0x08},
	{0x0217, 0x05},
	{0x0218, 0xB8},
	{0x0219, 0x01},
	{0x021A, 0xB8},
	{0x021B, 0x01},
	{0x021C, 0xB8},
	{0x021D, 0x01},
	{0x021E, 0xB8},
	{0x021F, 0x01},
	{0x0220, 0x21}, //
	{0x0221, 0x58}, //
	{0x0222, 0x10}, //
	{0x0223, 0x80},
	{0x0224, 0x68}, //
	{0x0225, 0x00}, //
	{0x0226, 0x90}, //
	{0x022A, 0x50}, //
	{0x022B, 0x00}, //
	{0x022C, 0x80},
	{0x022D, 0x11},
	{0x022E, 0x08},
	{0x022F, 0x11},
	{0x0230, 0x08},
	{0x0233, 0x11},
	{0x0234, 0x08},
	{0x0235, 0x88},
	{0x0236, 0x02},
	{0x0237, 0x88},
	{0x0238, 0x02},
	{0x023B, 0x88},
	{0x023C, 0x02},
	{0x023D, 0x68},
	{0x023E, 0x01},
	{0x023F, 0x68},
	{0x0240, 0x01},
	{0x0243, 0x68},
	{0x0244, 0x01},
	{0x0251, 0x10}, //
	{0x0252, 0x00},
	{0x0260, 0x00},
	{0x0261, 0x48}, //
	{0x0262, 0x30}, //
	{0x0263, 0x80}, //
	{0x0264, 0x6D}, //
	{0x0265, 0x00}, //
	{0x0266, 0x80}, //
	{0x026A, 0x54}, //
	{0x026B, 0x00}, //
	{0x026C, 0x80}, //
	{0x0278, 0x98},
	{0x0279, 0x20},
	{0x027A, 0x80},
	{0x027B, 0x73},
	{0x027C, 0x08},
	{0x027D, 0x80},
	{0x0280, 0x08}, //
	{0x0282, 0x10}, //
	{0x0284, 0x24}, //
	{0x0286, 0x49}, //
	{0x0288, 0x63}, //
	{0x028a, 0x73}, //
	{0x028c, 0x81}, //
	{0x028e, 0x8E}, //
	{0x0290, 0x99}, //
	{0x0292, 0xA3}, //
	{0x0294, 0xB1}, //
	{0x0296, 0xBA}, //
	{0x0298, 0xC7}, //
	{0x029a, 0xD4}, //
	{0x029c, 0xE2},
	{0x029e, 0x2A},
	{0x02A0, 0x02},
	{0x02C0, 0x7D},
	{0x02C1, 0x01},
	{0x02C2, 0x7C},
	{0x02C3, 0x04},
	{0x02C4, 0x01},
	{0x02C5, 0x04},
	{0x02C6, 0x3E},
	{0x02C7, 0x04},
	{0x02C8, 0x90},
	{0x02C9, 0x01},
	{0x02CA, 0x52},
	{0x02CB, 0x04},
	{0x02CC, 0x04},
	{0x02CD, 0x04},
	{0x02CE, 0xA9},
	{0x02CF, 0x04},
	{0x02D0, 0xAD},
	{0x02D1, 0x01},
	{0x0302, 0x00},
	{0x0303, 0x00},
	{0x0304, 0x00},
	{0x02e0, 0x04},
	{0x02F0, 0x4E},
	{0x02F1, 0x04},
	{0x02F2, 0xB1},
	{0x02F3, 0x00},
	{0x02F4, 0x63},
	{0x02F5, 0x04},
	{0x02F6, 0x28},
	{0x02F7, 0x04},
	{0x02F8, 0x29},
	{0x02F9, 0x04},
	{0x02FA, 0x51},
	{0x02FB, 0x00},
	{0x02FC, 0x64},
	{0x02FD, 0x04},
	{0x02FE, 0x6B},
	{0x02FF, 0x04},
	{0x0300, 0xCF},
	{0x0301, 0x00},
	{0x0305, 0x08},
	{0x0306, 0x40},
	{0x0307, 0x00},
	{0x032D, 0x70},
	{0x032E, 0x01},
	{0x032F, 0x00},
	{0x0330, 0x01},
	{0x0331, 0x70},
	{0x0332, 0x01},
	{0x0333, 0x82},
	{0x0334, 0x82},
	{0x0335, 0x86},
	{0x0340, 0x30},
	{0x0341, 0x44},
	{0x0342, 0x4A},
	{0x0343, 0x3C},
	{0x0344, 0x83},
	{0x0345, 0x4D},
	{0x0346, 0x75},
	{0x0347, 0x56},
	{0x0348, 0x68},
	{0x0349, 0x5E},
	{0x034A, 0x5C},
	{0x034B, 0x65},
	{0x034C, 0x52},
	{0x0350, 0x88},
	{0x0352, 0x18},
	{0x0354, 0x80},
	{0x0355, 0x50},
	{0x0356, 0x88},
	{0x0357, 0xE0},
	{0x0358, 0x00},
	{0x035A, 0x00},
	{0x035B, 0xAC},
	{0x0360, 0x02},
	{0x0361, 0x18},
	{0x0362, 0x50},
	{0x0363, 0x6C},
	{0x0364, 0x00},
	{0x0365, 0xF0},
	{0x0366, 0x08},
	{0x036A, 0x10},
	{0x036B, 0x18},
	{0x036E, 0x10},
	{0x0370, 0x10},
	{0x0371, 0x18},
	{0x0372, 0x0C},
	{0x0373, 0x38},
	{0x0374, 0x3A},
	{0x0375, 0x12},
	{0x0376, 0x20},
	{0x0380, 0xFF},
	{0x0381, 0x54},
	{0x0382, 0x44},
	{0x038A, 0x80},
	{0x038B, 0x0A},
	{0x038C, 0xC1},
	{0x038E, 0x50},
	{0x038F, 0x02},
	{0x0390, 0xEA},
	{0x0391, 0x01},
	{0x0392, 0x03},
	{0x0393, 0x80},
	{0x0395, 0x22},
	{0x0398, 0x02},
	{0x0399, 0xF0},
	{0x039A, 0x03},
	{0x039B, 0xAC},
	{0x039C, 0x04},
	{0x039D, 0x68},
	{0x039E, 0x05},
	{0x039F, 0xE0},
	{0x03A0, 0x07},
	{0x03A1, 0x58},
	{0x03A2, 0x08},
	{0x03A3, 0xD0},
	{0x03A4, 0x0B},
	{0x03A5, 0xC0},
	{0x03A6, 0x18},
	{0x03A7, 0x1C},
	{0x03A8, 0x20},
	{0x03A9, 0x24},
	{0x03AA, 0x28},
	{0x03AB, 0x30},
	{0x03AC, 0x24},
	{0x03AD, 0x21},
	{0x03AE, 0x1C},
	{0x03AF, 0x18},
	{0x03B0, 0x17},
	{0x03B1, 0x13},
	{0x03B7, 0x64},
	{0x03B8, 0x00},
	{0x03B9, 0xB4},
	{0x03BA, 0x00},
	{0x03bb, 0xff},
	{0x03bc, 0xff},
	{0x03bd, 0xff},
	{0x03be, 0xff},
	{0x03bf, 0xff},
	{0x03c0, 0xff},
	{0x03c1, 0x01},
	{0x03e0, 0x04},
	{0x03e1, 0x11},
	{0x03e2, 0x01},
	{0x03e3, 0x04},
	{0x03e4, 0x10},
	{0x03e5, 0x21},
	{0x03e6, 0x11},
	{0x03e7, 0x00},
	{0x03e8, 0x11},
	{0x03e9, 0x32},
	{0x03ea, 0x12},
	{0x03eb, 0x01},
	{0x03ec, 0x21},
	{0x03ed, 0x33},
	{0x03ee, 0x23},
	{0x03ef, 0x01},
	{0x03f0, 0x11},
	{0x03f1, 0x32},
	{0x03f2, 0x12},
	{0x03f3, 0x01},
	{0x03f4, 0x10},
	{0x03f5, 0x21},
	{0x03f6, 0x11},
	{0x03f7, 0x00},
	{0x03f8, 0x04},
	{0x03f9, 0x11},
	{0x03fa, 0x01},
	{0x03fb, 0x04},
	{0x03DC, 0x47},
	{0x03DD, 0x5A},
	{0x03DE, 0x41},
	{0x03DF, 0x53},
	{0x0420, 0x82},
	{0x0421, 0x00},
	{0x0422, 0x00},
	{0x0423, 0x88},
	{0x0430, 0x08},
	{0x0431, 0x30},
	{0x0432, 0x0c},
	{0x0433, 0x04},
	{0x0435, 0x08},
	{0x0450, 0xFF},
	{0x0451, 0xD0},
	{0x0452, 0xB8},
	{0x0453, 0x88},
	{0x0454, 0x00},
	{0x0458, 0x80},
	{0x0459, 0x03},
	{0x045A, 0x00},
	{0x045B, 0x50},
	{0x045C, 0x00},
	{0x045D, 0x90},
	{0x0465, 0x02},
	{0x0466, 0x14},
	{0x047A, 0x00},
	{0x047B, 0x00},
	{0x047C, 0x04},
	{0x047D, 0x50},
	{0x047E, 0x04},
	{0x047F, 0x90},
	{0x0480, 0x58},
	{0x0481, 0x06},
	{0x0482, 0x08},
	{0x04B0, 0x50},
	{0x04B6, 0x30},
	{0x04B9, 0x10},
	{0x04B3, 0x00},
	{0x04B1, 0x85},
	{0x04B4, 0x00},
	{0x0540, 0x00},
	{0x0541, 0x7D},
	{0x0542, 0x00},
	{0x0543, 0x96},
	{0x0580, 0x04},
	{0x0581, 0x0F},
	{0x0582, 0x04},
	{0x05A1, 0x20}, //SHARPNESS STRENGTH
	{0x05A2, 0x21},
	{0x05A3, 0x86}, //[7]EA_EN, [6] EE_CENTER,
	{0x05A4, 0x30}, //SHARP_ORE
	{0x05A5, 0xFF},
	{0x05A6, 0x00},
	{0x05A7, 0x24},
	{0x05A8, 0x24},
	{0x05A9, 0x02},
	{0x05B1, 0x18}, //EDGE SUB
	{0x05B2, 0x0C},
	{0x05B4, 0x1F},
	{0x05AE, 0x75},
	{0x05AF, 0x78},
	{0x05B6, 0x00},
	{0x05B7, 0x14}, //SHARP_ORE ALPHA0
	{0x05BF, 0x18}, //EDGESUB ALPHA0
	{0x05C1, 0x20}, //SHARPNESS STRENGTH OUTDOOR
	{0x05C2, 0x30}, //SHARP ORE OUTDOOR
	{0x05C7, 0x00},
	{0x05CC, 0x04},
	{0x05CD, 0x00},
	{0x05CE, 0x03},
	{0x05E4, 0x08},
	{0x05E5, 0x00},
	{0x05E6, 0x07},
	{0x05E7, 0x05},
	{0x05E8, 0x08},
	{0x05E9, 0x00},
	{0x05EA, 0xD7},
	{0x05EB, 0x02},
	{0x0660, 0x00},
	{0x0661, 0x16},
	{0x0662, 0x07},
	{0x0663, 0xf1},
	{0x0664, 0x07},
	{0x0665, 0xde},
	{0x0666, 0x07},
	{0x0667, 0xe7},
	{0x0668, 0x00},
	{0x0669, 0x35},
	{0x066a, 0x07},
	{0x066b, 0xf9},
	{0x066c, 0x07},
	{0x066d, 0xb7},
	{0x066e, 0x00},
	{0x066f, 0x27},
	{0x0670, 0x07},
	{0x0671, 0xf3},
	{0x0672, 0x07},
	{0x0673, 0xc5},
	{0x0674, 0x07},
	{0x0675, 0xee},
	{0x0676, 0x00},
	{0x0677, 0x16},
	{0x0678, 0x01},
	{0x0679, 0x80},
	{0x067a, 0x00},
	{0x067b, 0x85},
	{0x067c, 0x07},
	{0x067d, 0xe1},
	{0x067e, 0x07},
	{0x067f, 0xf5},
	{0x0680, 0x07},
	{0x0681, 0xb9},
	{0x0682, 0x00},
	{0x0683, 0x31},
	{0x0684, 0x07},
	{0x0685, 0xe6},
	{0x0686, 0x07},
	{0x0687, 0xd3},
	{0x0688, 0x00},
	{0x0689, 0x18},
	{0x068a, 0x07},
	{0x068b, 0xfa},
	{0x068c, 0x07},
	{0x068d, 0xd2},
	{0x068e, 0x00},
	{0x068f, 0x08},
	{0x0690, 0x00},
	{0x0691, 0x02},
	{0xAFD0, 0x03},
	{0xAFD3, 0x18},
	{0xAFD4, 0x04},
	{0xAFD5, 0xB8},
	{0xAFD6, 0x02},
	{0xAFD7, 0x44},
	{0xAFD8, 0x02},
	{0x0000, 0x01},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0005, 0x01}, //Turn on rolling shutter

};

const uint16_t sensor_hm1055_720P_30fps_talbe[][2] =
{
	{0x0005, 0x00},  //Turn off rolling shutter

	{0x0025, 0x00}, //PLL ON
	{0x0026, 0x97}, //PCLK 72Mhz
	{0x002B, 0x00},
	{0x002C, 0x00}, //CKCFG2[7]=1

	{0x0012, 0x0B},
	{0x0013, 0x00},

	{0x038F, 0x02}, //756 , 30fps
	{0x0390, 0xF4}, //

	{0x0540, 0x00},
	{0x0541, 0xBE},
	{0x0542, 0x00},
	{0x0543, 0xE4},

	{0x0000, 0x01},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0005, 0x01}, //Turn on rolling shutter
};

const uint16_t sensor_hm1055_720P_25fps_talbe[][2] =
{
	{0x0005, 0x00},  //Turn off rolling shutter

	{0x0025, 0x00}, //PLL ON
	{0x0026, 0x93}, //MCLK=PCLK= 60Mhz
	{0x002B, 0x00},
	{0x002C, 0x00}, //CKCFG2[7]=0

	{0x0012, 0x0D},
	{0x0013, 0x02},

	{0x038F, 0x02}, //
	{0x0390, 0xEA}, //

	{0x0540, 0x00},
	{0x0541, 0x9B},
	{0x0542, 0x00},
	{0x0543, 0xDA},

	{0x0000, 0x01},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0005, 0x01}, //Turn on rolling shutter
};

const uint16_t sensor_hm1055_720P_20fps_talbe[][2] =
{
	{0x0005, 0x00}, //Turn off rolling shutter

	{0x0025, 0x00}, //PLL ON
	{0x0026, 0x97}, //PCLK=48MHz,
	{0x002B, 0x01},
	{0x002C, 0x00}, //PLL2[7]=0

	{0x0012, 0x01},
	{0x0013, 0x02},

	{0x038F, 0x02},
	{0x0390, 0xEA},

	{0x0540, 0x00},
	{0x0541, 0x7D},
	{0x0542, 0x00},
	{0x0543, 0x96},

	{0x0000, 0x01},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0005, 0x01}, //Turn on rolling shutter
};

const uint16_t sensor_hm1055_720P_15fps_talbe[][2] =
{
	{0x0005, 0x00}, //Turn off rolling shutter

	{0x0025, 0x00}, //PLL ON
	{0x0026, 0x97}, //MCLK 36MHz,
	{0x002B, 0x02}, //1/6 * 1/2
	{0x002C, 0x00}, //PLL2[7]=0

	{0x0012, 0x0b},
	{0x0013, 0x00},

	{0x038F, 0x02},
	{0x0390, 0xEA},

	{0x0540, 0x00},
	{0x0541, 0x5f},
	{0x0542, 0x00},
	{0x0543, 0x72},

	{0x0000, 0x01},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0005, 0x01}, //Turn on rolling shutter
};

const uint16_t sensor_hm1055_720P_10fps_talbe[][2] =
{
	{0x0005, 0x00}, //Turn off rolling shutter

	{0x0025, 0x02}, //PLL ON, clock_div=(2+1)
	{0x0026, 0x37}, //PCLK 24Mhz
	{0x002B, 0x00},
	{0x002C, 0x80}, //PLL2[7]=1

	{0x0012, 0x03},
	{0x0013, 0x06},

	{0x038F, 0x02},
	{0x0390, 0xEA},

	{0x0540, 0x00},
	{0x0541, 0x3C},
	{0x0542, 0x00},
	{0x0543, 0x48},

	{0x0000, 0x01},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0005, 0x01}, //Turn on rolling shutter
};

const uint16_t sensor_hm1055_720P_5fps_talbe[][2] =
{
	{0x0005, 0x00}, //Turn off rolling shutter

	{0x0025, 0x02}, //PLL ON, clock_div=(2+1)
	{0x0026, 0x37}, //PCLK 12Mhz
	{0x002B, 0x02}, //CKCFG3[1]=1
	{0x002C, 0x80}, //PLL2[7]=1

	{0x0012, 0x03},
	{0x0013, 0x06},

	{0x038F, 0x02},
	{0x0390, 0xEA},

	{0x0540, 0x00},
	{0x0541, 0x1E},
	{0x0542, 0x00},
	{0x0543, 0x24},

	{0x0000, 0x01},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0005, 0x01}, //Turn on rolling shutter
};

bool hm1055_detect(void)
{
	uint8_t data[2] = {0};

	SENSOR_I2C_READ(0x0001, &data[0]);
	SENSOR_I2C_READ(0x0002, &data[1]);

	LOGI("%s, id: 0x%02X%02X\n", __func__, data[0], data[1]);

	if (data[0] == (HM1055_CHIP_ID >> 8)
	    && data[1] == (HM1055_CHIP_ID & 0xFF))
	{
		LOGI("%s success\n", __func__);
		return true;
	}

	return false;
}

void hm1055_read_register(uint16_t addr, uint8_t data)
{
	if (hm1055_read_flag)
	{
		uint8_t value = 0;
		rtos_delay_milliseconds(2);
		SENSOR_I2C_READ(addr, &value);
		if (value != data)
		{
			LOGI("0x%04x, 0x%02x-0x%02x\r\n", addr, data, value);
		}
	}
}


int hm1055_init(void)
{
	uint32_t size = sizeof(sensor_hm1055_init_talbe) / 4;

	for (uint32_t i = 0; i < size; i++)
	{
		SENSOR_I2C_WRITE(sensor_hm1055_init_talbe[i][0],
		                 (uint8_t)sensor_hm1055_init_talbe[i][1]);

		hm1055_read_register(sensor_hm1055_init_talbe[i][0],
		                     (uint8_t)sensor_hm1055_init_talbe[i][1]);
	}

	return 0;
}

int hm1055_set_ppi(media_ppi_t ppi)
{
	return 0;
}

int hm1055_set_fps(frame_fps_t fps)
{
	uint32_t size, i;
	int ret = -1;

	switch (fps)
	{
		case FPS5:
		{
			size = sizeof(sensor_hm1055_720P_5fps_talbe) / 4;

			for (i = 0; i < size; i++)
			{
				SENSOR_I2C_WRITE(sensor_hm1055_720P_5fps_talbe[i][0],
				                 (uint8_t)sensor_hm1055_720P_5fps_talbe[i][1]);

				hm1055_read_register(sensor_hm1055_720P_5fps_talbe[i][0],
				                     (uint8_t)sensor_hm1055_720P_5fps_talbe[i][1]);
			}

			ret = 0;
		}
		break;
		case FPS10:
		{
			size = sizeof(sensor_hm1055_720P_10fps_talbe) / 4;

			for (i = 0; i < size; i++)
			{
				SENSOR_I2C_WRITE(sensor_hm1055_720P_10fps_talbe[i][0],
				                 (uint8_t)sensor_hm1055_720P_10fps_talbe[i][1]);

				hm1055_read_register(sensor_hm1055_720P_10fps_talbe[i][0],
				                     (uint8_t)sensor_hm1055_720P_10fps_talbe[i][1]);
			}

			ret = 0;
		}
		break;
		case FPS15:
		{
			size = sizeof(sensor_hm1055_720P_15fps_talbe) / 4;

			for (i = 0; i < size; i++)
			{
				SENSOR_I2C_WRITE(sensor_hm1055_720P_15fps_talbe[i][0],
				                 (uint8_t)sensor_hm1055_720P_15fps_talbe[i][1]);

				hm1055_read_register(sensor_hm1055_720P_15fps_talbe[i][0],
				                     (uint8_t)sensor_hm1055_720P_15fps_talbe[i][1]);
			}

			ret = 0;
		}
		break;

		case FPS20:
		{
			//          size = sizeof(sensor_hm1055_720P_20fps_talbe) / 4;
			//
			//          for (i = 0; i < size; i++)
			//          {
			//              SENSOR_I2C_WRITE(sensor_hm1055_720P_20fps_talbe[i][0],
			//                               (uint8_t)sensor_hm1055_720P_20fps_talbe[i][1]);
			//          }

			ret = 0;
		}
		break;
		case FPS25:
		{
			size = sizeof(sensor_hm1055_720P_25fps_talbe) / 4;

			for (i = 0; i < size; i++)
			{
				SENSOR_I2C_WRITE(sensor_hm1055_720P_25fps_talbe[i][0],
				                 (uint8_t)sensor_hm1055_720P_25fps_talbe[i][1]);

				hm1055_read_register(sensor_hm1055_720P_25fps_talbe[i][0],
				                     (uint8_t)sensor_hm1055_720P_25fps_talbe[i][1]);
			}

			ret = 0;
		}
		break;
		case FPS30:
		{
			size = sizeof(sensor_hm1055_720P_30fps_talbe) / 4;

			for (i = 0; i < size; i++)
			{
				SENSOR_I2C_WRITE(sensor_hm1055_720P_30fps_talbe[i][0],
				                 (uint8_t)sensor_hm1055_720P_30fps_talbe[i][1]);

				hm1055_read_register(sensor_hm1055_720P_30fps_talbe[i][0],
				                     (uint8_t)sensor_hm1055_720P_30fps_talbe[i][1]);
			}

			ret = 0;
		}
		break;
		default:
			LOGI("default 20fps");
	}

	return ret;
}

int hm1055_power_down(void)
{
	SENSOR_I2C_WRITE(0x0004, 0x14);
	return 0;
}

int hm1055_dump(media_ppi_t ppi)
{
	uint32_t size, i;
	int ret = -1;
	uint8_t value = 0;

	LOGI("%s\n", __func__);

	size = sizeof(sensor_hm1055_init_talbe) / 4;

	for (i = 0; i < size; i++)
	{
		SENSOR_I2C_READ(sensor_hm1055_init_talbe[i][0], &value);
		LOGI("[0x%04x, 0x%02x]\r\n", sensor_hm1055_init_talbe[i][0], value);
	}

	ret = kNoErr;

	return ret;

}


void hm1055_read_enable(bool enable)
{
	hm1055_read_flag = enable;
}


const dvp_sensor_config_t dvp_sensor_hm1055 =
{
	.name = "hm1055",
	.clk = MCLK_30M,
	.fmt = PIXEL_FMT_YUYV,
	.vsync = SYNC_HIGH_LEVEL,
	.hsync = SYNC_HIGH_LEVEL,
	/* default config */
	.def_ppi = PPI_1280X720,
	.def_fps = FPS15,
	/* capability config */
	.fps_cap = FPS5 | FPS10 | FPS15 | FPS20 | FPS25 | FPS30,
	.ppi_cap = PPI_CAP_1280X720,
	.id = ID_HM1055,
	.address = (HM1055_WRITE_ADDRESS >> 1),
	.init = hm1055_init,
	.detect = hm1055_detect,
	.set_ppi = hm1055_set_ppi,
	.set_fps = hm1055_set_fps,
	.power_down = hm1055_power_down,
	.dump_register = hm1055_dump,
	.read_register = hm1055_read_enable,
};

