
#include "oled.h"
#include "synth.h"
#include "state.h"

ssd1306_handle_t device_handle;

static uint8_t last_y[128] = {0};

void i2c_master_init() {
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x58,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    i2c_master_dev_handle_t dev_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
}

void oled_init() {
    i2c_master_init();

    ssd1306_config_t dev_cfg = {
        .i2c_address  				= I2C_SSD1306_DEV_ADDR,		
        .i2c_clock_speed    		= I2C_MASTER_FREQ_HZ,  
        .panel_size                 = SSD1306_PANEL_128x64,	    
        .offset_x                   = 0,						
        .flip_enabled               = false
    };
    i2c_master_bus_handle_t i2c0_bus_hdl;
    i2c_master_get_bus_handle(0, &i2c0_bus_hdl);

    ssd1306_init(i2c0_bus_hdl, &dev_cfg, &device_handle);
    if (device_handle == NULL) {
        ESP_LOGE("OLED", "ssd1306 handle init failed");
        assert(device_handle);
    }

}

void oled_update() {

    // 21.5 ms per write, max 46.5 fps at i2c = 700kHz
    // too slow to run on same core as the synth but fast enough to run conncurent with io, wifi, etc
    // i was able to get i2c up to 1.4MHz for 69fps
    int32_t* buffer;
    //get_buffer_values(buffer);
    buffer = i2s_buffer;

    uint32_t stride = (scope_wavelength << 16) / 128;
    uint32_t phase = scope_trigger << 16;

    uint8_t prev_x = 0;
    uint8_t prev_y = 32;

    for (uint8_t x = 0; x < 128; x++) {

        for (uint8_t y = 0; y < 64; y++) {
            ssd1306_set_pixel(device_handle, x, y, true); // sclear display
        }

        uint32_t val = phase >> 16; // where to index the buffer so that the screen is scaled to one period
        int32_t sample = buffer[val % BUFFER_LEN];

        uint8_t y = 32 + (sample >> (24+2)); // scale [-2^31, 2^31-1] to [0, 63]

        if(y > 63 || y <= 0) y = 0;
        y = 63 - y; // invert vertically 
        last_y[x] = y;

        //ssd1306_set_pixel(device_handle, x, y, false);
        if(x == 0) prev_y = y;
        ssd1306_set_line(device_handle, prev_x, prev_y, x, y, false);
        prev_x = x;
        prev_y = y;

        phase += stride;

    }
    ssd1306_display_pages(device_handle); // flush buffer to device over i2c

}
