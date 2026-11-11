// lcd.h  前提: config.h
class MyLGFX : public lgfx::LGFX_Device {
  lgfx::LCD_PANEL _panel_instance;   // config.h で指定
  lgfx::Bus_SPI   _bus_instance;
#ifdef LCD_BL
  lgfx::Light_PWM _light_instance;
#endif

public:
  MyLGFX(void) {
    auto cfg = _bus_instance.config();
    cfg.spi_host    = SPI2_HOST;
    cfg.spi_mode    = 0;
    cfg.freq_write  = 40000000;
    cfg.freq_read   = 16000000;
    cfg.dma_channel = SPI_DMA_CH_AUTO;
    cfg.pin_sclk    = LCD_SCLK;
    cfg.pin_mosi    = LCD_MOSI;
    cfg.pin_miso    = -1;
    cfg.pin_dc      = LCD_DC;
    _bus_instance.config(cfg);
    _panel_instance.setBus(&_bus_instance);

    auto panel_cfg = _panel_instance.config();
    panel_cfg.pin_cs       = -1;
    panel_cfg.pin_rst      = LCD_RST;
    panel_cfg.pin_busy     = -1;
    panel_cfg.panel_width  = LCD_PANEL_W;   // パネル本来の向き(横向きは setRotation で)
    panel_cfg.panel_height = LCD_PANEL_H;
    _panel_instance.config(panel_cfg);

#ifdef LCD_BL
    auto light_cfg = _light_instance.config();
    light_cfg.pin_bl      = LCD_BL;
    light_cfg.freq        = 44100;
    light_cfg.pwm_channel = 0;        // ESP32-C3 は 0〜5
    _light_instance.config(light_cfg);
    _panel_instance.setLight(&_light_instance);
#endif

    setPanel(&_panel_instance);
  }
};
