#pragma once

/*-----------------------------------------------------------------------
IT8951 Command defines
------------------------------------------------------------------------*/

// Built in I80 command code
#define IT8951_TCON_SYS_RUN         0x0001
#define IT8951_TCON_STANDBY         0x0002
#define IT8951_TCON_SLEEP           0x0003
#define IT8951_TCON_REG_RD          0x0010
#define IT8951_TCON_REG_WR          0x0011

#define IT8951_TCON_MEM_BST_RD_T    0x0012
#define IT8951_TCON_MEM_BST_RD_S    0x0013
#define IT8951_TCON_MEM_BST_WR      0x0014
#define IT8951_TCON_MEM_BST_END     0x0015

#define IT8951_TCON_LD_IMG          0x0020
#define IT8951_TCON_LD_IMG_AREA     0x0021
#define IT8951_TCON_LD_IMG_END      0x0022

// I80 user defined command code
#define IT8951_I80_CMD_DPY_AREA     0x0034
#define IT8951_I80_CMD_GET_DEV_INFO 0x0302
#define IT8951_I80_CMD_DPY_BUF_AREA 0x0037
#define IT8951_I80_CMD_VCOM         0x0039

/*-----------------------------------------------------------------------
 IT8951 Mode defines
------------------------------------------------------------------------*/
// Pixel mode (Bit per Pixel)
#define IT8951_2BPP                 0
#define IT8951_3BPP                 1
#define IT8951_4BPP                 2
#define IT8951_8BPP                 3

// Endian type
#define IT8951_LDIMG_L_ENDIAN       0
#define IT8951_LDIMG_B_ENDIAN       1

/*-----------------------------------------------------------------------
IT8951 Register defines
------------------------------------------------------------------------*/
// Register base address
#define IT8951_DISPLAY_REG_BASE     0x1000  // Register RW access

// Base address of Basic LUT registers
#define IT8951_LUT0EWHR             (IT8951_DISPLAY_REG_BASE + 0x00)
#define IT8951_LUT0XYR              (IT8951_DISPLAY_REG_BASE + 0x40)
#define IT8951_LUT0BADDR            (IT8951_DISPLAY_REG_BASE + 0x80)
#define IT8951_LUT0MFN              (IT8951_DISPLAY_REG_BASE + 0xC0)
#define IT8951_LUT01AF              (IT8951_DISPLAY_REG_BASE + 0x114)

// Update parameter setting registers
#define IT8951_UP0SR                (IT8951_DISPLAY_REG_BASE + 0x134)
#define IT8951_UP1SR                (IT8951_DISPLAY_REG_BASE + 0x138)
#define IT8951_LUT0ABFRV            (IT8951_DISPLAY_REG_BASE + 0x13C)
#define IT8951_UPBBADDR             (IT8951_DISPLAY_REG_BASE + 0x17C)
#define IT8951_LUT0IMXY             (IT8951_DISPLAY_REG_BASE + 0x180)
#define IT8951_LUTAFSR              (IT8951_DISPLAY_REG_BASE + 0x224)
#define IT8951_BGVR                 (IT8951_DISPLAY_REG_BASE + 0x250)

// System registers
#define IT8951_SYS_REG_BASE         0x0000

// Address of system registers
#define IT8951_I80CPCR              (IT8951_SYS_REG_BASE + 0x04)

// Memory converter registers
#define IT8951_MCSR_BASE_ADDR       0x0200
#define IT8951_MCSR                 (IT8951_MCSR_BASE_ADDR + 0x0000)
#define IT8951_LISAR                (IT8951_MCSR_BASE_ADDR + 0x0008)
