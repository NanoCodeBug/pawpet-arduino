#pragma once

/**
 * pin mappings and chip specific configurations
 * documents unused pins and changes to major board revisions
 */

#define M0_VER_5 1

const unsigned char BUILD_STRING[]
{
	__TIME__[0],
	__TIME__[1],
	__TIME__[3],
	__TIME__[4],
	__DATE__[0],
	__DATE__[1],
	__DATE__[2],
	__DATE__[4],
	__DATE__[5],
	'\0'
};

#define DISP_WIDTH 128
#define DISP_HEIGHT 128

#if defined(M0_VER_5) || defined(M0_VER_6)
	// 0 RX
	// 1 TX
	#define FLASH_MISO 2 // PORTA, 14
	#define FLASH_SCK 3  // PORTA, 9
	#define FLASH_MOSI 4 // PORTA, 8
	#define PIN_BUTTON_C 5
	#define PIN_RIGHT 6
	#define PIN_LEFT 7
	#define SHARP_SS 8
	#define PIN_VBAT 9 // A7
	#define PIN_BUTTON_A 10
	#define PIN_BUTTON_P 11
	#define PIN_DOWN 12
	#define PIN_BUTTON_B 13
	// 14 A0
	#define PIN_BEEPER 15 // A1
	// 16 A2
	// 17 A3
	// 18 A4
	#define PIN_UP 19 // A5
	// SDA 20 no pullup?
	// SCL 21 no pullup?
  #define SHARP_MISO 22 // MISO 
	#define SHARP_MOSI 23 // MOSI
	#define SHARP_SCK 24  // SCK
	#define FLASH_CS 38 // PORTA, 13

	#define RAM_SIZE 32000

	#define DISP_ROTATION 1
#endif

#define MX25R1635F                                                             \
  {                                                                            \
    .total_size = (1 << 21), /* 2 MB / 16 Mb */                                \
    .start_up_time_us = 800, .manufacturer_id = 0xc2,                          \
    .memory_type = 0x28, .capacity = 0x15, .max_clock_speed_mhz = 33 /*8*/,    \
    .quad_enable_bit_mask = 0x40, .has_sector_protection = false,              \
    .supports_fast_read = true, .supports_qspi = true,                         \
    .supports_qspi_writes = true, .write_status_register_split = false,        \
    .single_status_byte = true, \
  }



