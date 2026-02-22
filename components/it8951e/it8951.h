#pragma once

// Minimal IT8951 command/register definitions used by this component.
// Values are consistent with published IT8951 driver references.  [oai_citation:9‡docs.paperd.ink](https://docs.paperd.ink/PaperdInk-Library/GxEPD2__it60_8cpp.html?utm_source=chatgpt.com)

#define IT8951_TCON_SYS_RUN       0x0001
#define IT8951_TCON_STANDBY       0x0002
#define IT8951_TCON_SLEEP         0x0003
#define IT8951_TCON_REG_RD        0x0010
#define IT8951_TCON_REG_WR        0x0011
#define IT8951_TCON_LD_IMG        0x0020
#define IT8951_TCON_LD_IMG_AREA   0x0021
#define IT8951_TCON_LD_IMG_END    0x0022

// I80 user defined command code
#define IT8951_I80_CMD_DPY_AREA       0x0034
#define IT8951_I80_CMD_DPY_BUF_AREA   0x0037
#define IT8951_I80_CMD_VCOM           0x0039
#define IT8951_I80_CMD_GET_DEV_INFO   0x0302

// Register base / addresses
#define IT8951_SYS_REG_BASE       0x0000
#define IT8951_MCSR_BASE_ADDR     0x0200
#define IT8951_LISAR              (IT8951_MCSR_BASE_ADDR + 0x0008)

// Load image endian
#define IT8951_LDIMG_L_ENDIAN     0
#define IT8951_LDIMG_B_ENDIAN     1

// Bits-per-pixel
#define IT8951_2BPP               0
#define IT8951_3BPP               1
#define IT8951_4BPP               2
#define IT8951_8BPP               3
