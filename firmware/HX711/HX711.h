#ifndef _HX711_H
#define _HX711_H

#define HX711_X_DAT 12
#define HX711_X_SCK 13
#define HX711_P_DAT 14
#define HX711_P_SCK 15
#define HX711_Y_DAT 16
#define HX711_Y_SCK 17
#define HX711_PIO pio1

#define DMA_RELOAD_VAL (0xffffffff-1)

extern int32_t Xtilt, Ytilt, Press;

void HX711_update();
void HX711_init();

#endif