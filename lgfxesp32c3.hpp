#ifndef LGFX_ESP32C3_HPP
#define LGFX_ESP32C3_HPP

#include <LovyanGFX.hpp>

class MyLGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9341 _panel_instance;
  lgfx::Bus_SPI _bus_instance;

public:
  MyLGFX(void) {
    auto cfg = _bus_instance.config();
    cfg.spi_host = SPI2_HOST;
    cfg.spi_mode = 0;
    cfg.freq_write = 40000000;
    cfg.freq_read  = 16000000;
    cfg.dma_channel = SPI_DMA_CH_AUTO;
    cfg.pin_sclk = 6;
    cfg.pin_mosi = 7;
    cfg.pin_miso = -1;
    cfg.pin_dc   = 9;
    _bus_instance.config(cfg);
    _panel_instance.setBus(&_bus_instance);

    auto panel_cfg = _panel_instance.config();
    panel_cfg.pin_cs   = -1;
    panel_cfg.pin_rst  = 8;
    panel_cfg.pin_busy = -1;
    panel_cfg.panel_width  = 240;
    panel_cfg.panel_height = 320;
    _panel_instance.config(panel_cfg);

    setPanel(&_panel_instance);
  }
};

#endif
