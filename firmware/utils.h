#ifndef _UTILS_H
#define _UTILS_H

#define CHECK_SERIAL_QUIT if(getchar_timeout_us(1) != PICO_ERROR_TIMEOUT){\
            printf("\n\nBOOTING FROM USB\n\n");\
            sleep_ms(20);\
            reset_usb_boot(0,0);\
        }

#define CHECKBUTTON gpio_set_function(11,GPIO_FUNC_SIO);\
                    gpio_set_dir(11, false);\
                    gpio_set_pulls(11,true,false);\
                    sleep_ms(1);\
                    if(gpio_get(11) == 0){\
                        reset_usb_boot(0,0);\
                    }

char _tmp_string_dprintf[100];
#define dprintf(...) {sprintf(_tmp_string_dprintf, __VA_ARGS__);\
                     tud_cdc_write(_tmp_string_dprintf, strlen(_tmp_string_dprintf));}

#define D sleep_ms(1);

#define REP10(X) X X X X X X X X X X
#define REP100(X) REP10(REP10(X))

#endif