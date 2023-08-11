#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bme280.h"
#include "lcd_i2c.h"
#include "driver/i2c.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "esp_adc_cal.h"
#include "esp_sleep.h"
#include <stdio.h>

#define SENSOR_PIN ADC1_CHANNEL_4
#define TRIGGER_PIN GPIO_NUM_31

void lcd_display(char *temp_str, char *humi_str, char  *mois_str){
        lcd1602_t lcdtmp = {0};
        lcd1602_set_pos(&lcdtmp, 0, 0);
        lcd1602_send_string(&lcdtmp, temp_str);
        lcd1602_set_pos(&lcdtmp, 0, 14);
        lcd1602_send_string(&lcdtmp, "°C");
        lcd1602_set_pos(&lcdtmp, 1, 0);
        lcd1602_send_string(&lcdtmp, "H/M =");
        lcd1602_set_pos(&lcdtmp, 1, 15);
        lcd1602_send_string(&lcdtmp, "%");
        lcd1602_set_pos(&lcdtmp, 1, 8);
        lcd1602_send_string(&lcdtmp, "%/");
        lcd1602_set_pos(&lcdtmp, 1, 6);
        lcd1602_send_string(&lcdtmp, humi_str);
        lcd1602_set_pos(&lcdtmp, 1, 10);
        lcd1602_send_string(&lcdtmp, mois_str);
}

void app_main(){
//setup bme280 sensor
        bme280_data_t bme280_data = {0};
        bme280_init();

//setup lcd
        lcd1602_t lcdtmp = {0};
        lcd1602_i2c_init(21, 22, 0);
        lcdtmp.i2caddr = 0x27;
        lcdtmp.backlight = 1;
        lcd1602_dcb_set(&lcdtmp, 1, 0, 0);
        lcd1602_init(&lcdtmp);

//setup MH-sensor-series
        static const adc_atten_t atten = ADC_ATTEN_DB_11;
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(SENSOR_PIN, atten);

//setup GPIO output pin
        gpio_pad_select_gpio(TRIGGER_PIN);
        gpio_set_direction(TRIGGER_PIN, GPIO_MODE_OUTPUT);

  while(1){
//lấy dữ liệu độ ẩm đất
        uint32_t sensor_analog = adc1_get_raw(SENSOR_PIN);
        float moisture = 100 - (float)sensor_analog /4095.0 * 100.0;
        char mois_str[16];
        sprintf(mois_str, "%.1f", moisture);

//lấy dữ liệu từ BME280 sensor
        bme280_get_data(&bme280_data);
        char temp_str[16];
        char humi_str[16];
        int temperature = bme280_data.temperature;
        int humidity = bme280_data.humidity;
        sprintf(temp_str, "Temp = %d", bme280_data.temperature);
        sprintf(humi_str, "%d", bme280_data.humidity);

//hiển thị các dữ liệu lên lcd
        lcd_display(temp_str, humi_str, mois_str);

//kịch bản tưới tiêu
        if (temperature > 30 && moisture < 70) {
                gpio_set_level(TRIGGER_PIN, 1);//bật led để báo rằng đã thỏa mãn điều kiện để tưới nước
                int count = 300;
                while(count){
                        //lấy dữ liệu độ ẩm đất
                                uint32_t sensor_analog = adc1_get_raw(SENSOR_PIN);
                                float moisture = 100 - (float)sensor_analog /4095.0 * 100.0;
                                char mois_str[16];
                                sprintf(mois_str, "%.1f", moisture);

                        //lấy dữ liệu từ BME280 sensor
                                bme280_get_data(&bme280_data);
                                char temp_str[16];
                                char humi_str[16];
                                int temperature = bme280_data.temperature;
                                int humidity = bme280_data.humidity;
                                sprintf(temp_str, "Temp = %d", bme280_data.temperature);
                                sprintf(humi_str, "%d", bme280_data.humidity);

                        //hiển thị các dữ liệu lên lcd
                                lcd_display(temp_str, humi_str, mois_str);
                        
                        //cập nhật biến count để thực hiện 300 lần mỗi lần cách nhau 1s tương đương 5'
                                count -= 1;
                                vTaskDelay(pdMS_TO_TICKS(1000));

                }
                gpio_set_level(TRIGGER_PIN, 0);//tắt led tương đương với ý nghĩa là dừng tưới cây sau 5'
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

