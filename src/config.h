#pragma once
/**
 * FeatherM0 - global define for logic specific to that proccessor
 * 
 * Current configurations
 * M0_VER_0
 * - prototype board, hand wired
 * 
 * M0_VER_1
 * - pcb ver 1
 * 
 * M0_VER_2
 * - PIN_UP moved to a5 instead of scl
 * - i2c breakout header
 *
 **/

// try to figure out revision naively

#define M0_VER_5 1
#define SOFT_VERSION "1.2"

#define DISP_WIDTH 128
#define DISP_HEIGHT 128

#if defined(M0_VER_0)
    #define PIN_LEFT 0
    #define PIN_DOWN 1
    // 5  AVAILABLE
    #define SHARP_SS 6

    #define PIN_VBAT A7
    // 10 AVAILABLE
    // 11 AVAILABLE
    // 12 AVAILABLE
    #define PIN_LED 13      // unused but reserved to avoid power loss to LED
    #define PIN_BEEPER A0   // 14
    #define PIN_BUTTON_C 15 // A1
    #define PIN_BUTTON_P 16 // A2
    #define PIN_BUTTON_B 17 // A3
    #define PIN_BUTTON_A 18 // A4
    #define PIN_UP 19       // A5
    // SDA 20 no pullup?
    // SCL 21 no pullup?
    #define PIN_RIGHT 22  // MISO
    #define SHARP_MOSI 23 // MOSI
    #define SHARP_SCK 24  // SCK

    #define RAM_SIZE 32000
    
    SPIClass mySPI(&sercom4, 22, 24, 23, SPI_PAD_2_SCK_3, SERCOM_RX_PAD_0);

    #define DISP_ROTATION 2
#elif defined(M0_VER_1)

    // 0 RX 
    // 1 TX
    #define FLASH_MISO 2
    #define FLASH_SCK 3
    #define FLASH_MOSI 4
    #define PIN_BUTTON_C 5
    #define PIN_DOWN 6
    #define PIN_LEFT 7
    #define SHARP_SS 8
    #define PIN_VBAT A7
    #define PIN_BUTTON_A 10 
    #define PIN_BUTTON_P 11
    #define PIN_RIGHT 12
    #define PIN_BUTTON_B 13
    // 14 A0
    #define PIN_BEEPER 15 // A1
    // 16 A2
    // 17 A3
    // 18 A4
    // 19 A5
    // SDA 20 no pullup?
    #define PIN_UP 21 //scl
    // RESERVED 22 MISO
    #define SHARP_MOSI 23 // MOSI
    #define SHARP_SCK 24  // SCK
    #define FLASH_CS 38

    #define RAM_SIZE 32000
    
    SPIClass mySPI(&sercom4, 22, 24, 23, SPI_PAD_2_SCK_3, SERCOM_RX_PAD_0);

    #define DISP_ROTATION 1
	
#elif defined(M0_VER_2)
	// 0 RX
	// 1 TX
	#define FLASH_MISO 2 // PORTA, 14
	#define FLASH_SCK 3  // PORTA, 9
	#define FLASH_MOSI 4 // PORTA, 8
	#define PIN_BUTTON_C 5
	#define PIN_DOWN 6
	#define PIN_LEFT 7
	#define SHARP_SS 8
	#define PIN_VBAT 9 // A7
	#define PIN_BUTTON_A 10
	#define PIN_BUTTON_P 11
	#define PIN_RIGHT 12
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

	SPIClass mySPI(&sercom4, SHARP_MISO, SHARP_SCK, SHARP_MOSI, SPI_PAD_2_SCK_3, SERCOM_RX_PAD_0);

  SPIClass flashSPI(&sercom2, FLASH_MISO, FLASH_SCK, FLASH_MOSI, SPI_PAD_0_SCK_1, SERCOM_RX_PAD_2);

	#define DISP_ROTATION 1
#else if defined(M0_VER_5) || define(M0_VER_6)
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

static const SPIFlash_Device_t possible_devices[] = {
  MX25R1635F
};

