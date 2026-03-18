/*
   -------------------------------------------------------------------
   Board-specific compile-time hardware profiles
   -------------------------------------------------------------------
*/

#ifndef _EMONESP_BOARD_PROFILE_H
#define _EMONESP_BOARD_PROFILE_H

#define CS_BOARD_PROFILE_NODEMCU32S 1
#define CS_BOARD_PROFILE_LILYGO_T_ETH_LITE_S3 2
#define CS_BOARD_PROFILE_WAVESHARE_ESP32_S3_ETH 3

#ifndef CS_BOARD_PROFILE
#define CS_BOARD_PROFILE CS_BOARD_PROFILE_NODEMCU32S
#endif

#define BOARD_PROFILE_NUM_BOARDS 7

#if CS_BOARD_PROFILE == CS_BOARD_PROFILE_NODEMCU32S

static constexpr const char *BOARD_PROFILE_NAME = "nodemcu32s";
#define BOARD_PROFILE_SUPPORTS_ETHERNET 0
#define BOARD_PROFILE_HAS_WIFI_STA 1
#define BOARD_PROFILE_ENABLE_BUTTON_ACTIONS 1
static constexpr int BOARD_PROFILE_METER_SPI_SCK = 18;
static constexpr int BOARD_PROFILE_METER_SPI_MISO = 19;
static constexpr int BOARD_PROFILE_METER_SPI_MOSI = 23;
static constexpr int BOARD_PROFILE_ETH_SPI_SCK = -1;
static constexpr int BOARD_PROFILE_ETH_SPI_MISO = -1;
static constexpr int BOARD_PROFILE_ETH_SPI_MOSI = -1;
static constexpr int BOARD_PROFILE_ETH_CS = -1;
static constexpr int BOARD_PROFILE_ETH_INT = -1;
static constexpr int BOARD_PROFILE_ETH_RST = -1;
static constexpr int BOARD_PROFILE_ETH_SPI_FREQ_MHZ = 30;
static constexpr int BOARD_PROFILE_CS1_PINS[BOARD_PROFILE_NUM_BOARDS] = {5, 0, 27, 2, 13, 14, 15};
static constexpr int BOARD_PROFILE_CS2_PINS[BOARD_PROFILE_NUM_BOARDS] = {4, 16, 17, 21, 22, 25, 26};

#elif CS_BOARD_PROFILE == CS_BOARD_PROFILE_LILYGO_T_ETH_LITE_S3

static constexpr const char *BOARD_PROFILE_NAME = "lilygo-t-eth-lite-s3";
#define BOARD_PROFILE_SUPPORTS_ETHERNET 1
#define BOARD_PROFILE_HAS_WIFI_STA 0
#define BOARD_PROFILE_ENABLE_BUTTON_ACTIONS 0
static constexpr int BOARD_PROFILE_METER_SPI_SCK = 7;
static constexpr int BOARD_PROFILE_METER_SPI_MISO = 5;
static constexpr int BOARD_PROFILE_METER_SPI_MOSI = 6;
static constexpr int BOARD_PROFILE_ETH_SPI_SCK = 10;
static constexpr int BOARD_PROFILE_ETH_SPI_MISO = 11;
static constexpr int BOARD_PROFILE_ETH_SPI_MOSI = 12;
static constexpr int BOARD_PROFILE_ETH_CS = 9;
static constexpr int BOARD_PROFILE_ETH_INT = 13;
static constexpr int BOARD_PROFILE_ETH_RST = 14;
static constexpr int BOARD_PROFILE_ETH_SPI_FREQ_MHZ = 30;
static constexpr int BOARD_PROFILE_CS1_PINS[BOARD_PROFILE_NUM_BOARDS] = {8, 0, 18, 2, 3, 41, 15};
static constexpr int BOARD_PROFILE_CS2_PINS[BOARD_PROFILE_NUM_BOARDS] = {4, 16, 17, 21, 20, 39, 38};

#elif CS_BOARD_PROFILE == CS_BOARD_PROFILE_WAVESHARE_ESP32_S3_ETH

static constexpr const char *BOARD_PROFILE_NAME = "waveshare-esp32-s3-eth";
#define BOARD_PROFILE_SUPPORTS_ETHERNET 1
#define BOARD_PROFILE_HAS_WIFI_STA 0
#define BOARD_PROFILE_ENABLE_BUTTON_ACTIONS 0
static constexpr int BOARD_PROFILE_METER_SPI_SCK = 44;
static constexpr int BOARD_PROFILE_METER_SPI_MISO = 45;
static constexpr int BOARD_PROFILE_METER_SPI_MOSI = 43;
static constexpr int BOARD_PROFILE_ETH_SPI_SCK = 13;
static constexpr int BOARD_PROFILE_ETH_SPI_MISO = 12;
static constexpr int BOARD_PROFILE_ETH_SPI_MOSI = 11;
static constexpr int BOARD_PROFILE_ETH_CS = 14;
static constexpr int BOARD_PROFILE_ETH_INT = 10;
static constexpr int BOARD_PROFILE_ETH_RST = 9;
static constexpr int BOARD_PROFILE_ETH_SPI_FREQ_MHZ = 30;
static constexpr int BOARD_PROFILE_CS1_PINS[BOARD_PROFILE_NUM_BOARDS] = {47, 1, 18, 2, 33, 41, 15};
static constexpr int BOARD_PROFILE_CS2_PINS[BOARD_PROFILE_NUM_BOARDS] = {48, 16, 17, 40, 42, 39, 38};

#else

#error Unsupported CS_BOARD_PROFILE

#endif

#endif // _EMONESP_BOARD_PROFILE_H
