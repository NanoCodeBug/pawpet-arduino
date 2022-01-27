#ifndef PawPet_FlashTransport_SPI_H_
#define PawPet_FlashTransport_SPI_H_

/**
 * Fork of adafruit flash transport
 * only purpose is to remove _spi->begin() from begin()
 * so that it stops resetting the sercom pin mappings
 */

#include "Arduino.h"
#include "SPI.h"

#include "Adafruit_FlashTransport.h"

class PawPet_FlashTransport_SPI : public Adafruit_FlashTransport
{
  private:
    SPIClass *_spi;
    uint8_t _ss;

    // SAMD21 M0 can write up to 24 Mhz, but can only read reliably with 12 MHz
    uint32_t _clock_wr;
    uint32_t _clock_rd;

  public:
    PawPet_FlashTransport_SPI(uint8_t ss, SPIClass *spiinterface);
    PawPet_FlashTransport_SPI(uint8_t ss, SPIClass &spiinterface);

    virtual void begin(void);
    virtual void end(void);

    virtual bool supportQuadMode(void)
    {
        return false;
    }

    virtual void setClockSpeed(uint32_t write_hz, uint32_t read_hz);

    virtual bool runCommand(uint8_t command);
    virtual bool readCommand(uint8_t command, uint8_t *response, uint32_t len);
    virtual bool writeCommand(uint8_t command, uint8_t const *data, uint32_t len);
    virtual bool eraseCommand(uint8_t command, uint32_t addr);

    virtual bool readMemory(uint32_t addr, uint8_t *data, uint32_t len);
    virtual bool writeMemory(uint32_t addr, uint8_t const *data, uint32_t len);

  private:
    void fillAddress(uint8_t *buf, uint32_t addr);

    void beginTransaction(uint32_t clock_hz)
    {
        _spi->beginTransaction(SPISettings(clock_hz, MSBFIRST, SPI_MODE0));
        digitalWrite(_ss, LOW);
    }

    void endTransaction(void)
    {
        digitalWrite(_ss, HIGH);
        _spi->endTransaction();
    }
};

#endif /* PawPet_FlashTransport_SPI_H_ */
