#ifndef _UTILS_H
#define _UTILS_H

#include <string.h>
#include <stdarg.h>

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

char _tmp_string_dbgprintf[100];
void dbgprintf(const char* format, ...) {
    va_list argptr;
    va_start(argptr, format);
    sprintf(_tmp_string_dbgprintf, format, argptr);
    va_end(argptr);
    tud_cdc_write(_tmp_string_dbgprintf, strlen(_tmp_string_dbgprintf));
    tud_cdc_write_flush();
    }



#define D sleep_ms(1);

#define REP10(X) X X X X X X X X X X
#define REP100(X) REP10(REP10(X))

#endif