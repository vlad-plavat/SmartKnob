#include "bsp/board.h"
#include "tusb.h"

#include "usb_descriptors.h"
enum  {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

typedef struct TU_ATTR_PACKED
{
  union{
    struct{
      int32_t  Xtilt, Ytilt, Press;
      uint32_t angle;
      int32_t speed;
    };
    uint8_t filler[64];
  };
} hid_smartknob_report_t;


void (*usb_packetCallback)(uint8_t*, uint8_t) = NULL;
void (*usb_fill_descriptor_callback)(hid_smartknob_report_t *rep) = NULL;

void led_blinking_task(void);
void cdc_task(void);
void hid_task(void);

struct repeating_timer timer;

unsigned long prevtm=0,crtm;

unsigned long millis = 0;

bool repeating_timer_callback(struct repeating_timer *t) {
    (void) t;
    millis++;
    
    return true;
}

void service_usb(){
    tud_task(); // tinyusb device task
    
    /*crtm = to_ms_since_boot(get_absolute_time());
    if(crtm - prevtm > 100){
        checkBIOSloaded();
    }*/
    cdc_task();
    hid_task();
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+
void cdc_task(void)
{
  // connected() check for DTR bit
  // Most but not all terminal client set this when making connection
  // if ( tud_cdc_connected() )
  {
    // connected and there are data available
    if ( tud_cdc_available() )
    {
      // read data
      char buf[64];
      uint32_t count = tud_cdc_read(buf, sizeof(buf));
      (void) count;
        for(uint i=0;i<count;i++)buf[i]+='A'-'a';
      // Echo back
      // Note: Skip echo by commenting out write() and write_flush()
      // for throughput test e.g
      //    $ dd if=/dev/zero of=/dev/ttyACM0 count=10000
      tud_cdc_write(buf, count);
      tud_cdc_write_flush();
    }
  }
}

// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
  (void) itf;
  (void) rts;

  // TODO set some indicator
  if ( dtr )
  {
    // Terminal connected
  }else
  {
    // Terminal disconnected
  }
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf)
{
  (void) itf;
}


int abs(int x){
  return x<0?-x:x;
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

static void send_hid_report(uint8_t report_id)
{
  // skip if hid is not ready yet
  if ( !tud_hid_ready() ) return;

  if (report_id == REPORT_ID_GAMEPAD)
  {
      // use to avoid send multiple consecutive zero report for keyboard
      //static bool has_gamepad_key = false;

    if(usb_mode == USB_JOYSTICK){
      //extern int32_t Xtilt, Ytilt, Press;
      //dbgprintf("%ld %ld %ld\n",Xtilt, Ytilt, Press);
      int16_t Yval = (Ytilt>0?-Ytilt/20:-Ytilt/16);
      int16_t Xval = -Xtilt/16;
      if(!(abs(-Xtilt/24)>64 || abs(Yval)>64)){
        Xval = Yval = 0;
      }
      if ( Xval > 127 ) Xval=127;
      if ( Yval > 127 ) Yval=127;
      if ( Xval < -128) Xval=-128;
      if ( Yval < -128) Yval=-128;

      static uint32_t prevprtm = 0;
      static uint8_t prevpr = 0;
      static float prev_knob_angle;
      float cr_angle = 360.0-knob_angle*360.0/1024/16;
      float delta = cr_angle - prev_knob_angle;
      if(delta > 180){delta-=360;}if(delta < -180){delta+=360;}
      int32_t rx = delta*4;if(rx>127){rx=127;}if(rx<-127){rx=-127;}
      prev_knob_angle = cr_angle;

      uint8_t crpr = Press>PressLimit1;
      uint8_t btn2 = true;
      if(!prevpr && crpr){
        prevprtm  = time_us_32();
      }
      if(time_us_32() - prevprtm > 100*1000){
        btn2=false;
        prevprtm = time_us_32()+101*1000;
      }
      prevpr = crpr;

      hid_gamepad_report_t report =
      {
        .x   = Xval, .y = Yval, .z = rx, .rz = rx, .rx = rx, .ry = rx,
        .hat = 0, .buttons = (Press>PressLimit2?GAMEPAD_BUTTON_A:0)|(btn2?GAMEPAD_BUTTON_1:0)
      };

      tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));
      return;
    }
    if(usb_mode == USB_MOUSE){

      int16_t Yval = (Ytilt>0?-Ytilt/20:-Ytilt/16);
      int16_t Xval = -Xtilt/16;
      if(!(abs(Xval)>64 || abs(Yval)>64)){
        Xval = Yval = 0;
      }
      if ( Xval > 127 ) Xval=127;
      if ( Yval > 127 ) Yval=127;
      if ( Xval < -128) Xval=-128;
      if ( Yval < -128) Yval=-128;

      static float prev_knob_angle;
      float cr_angle = 360.01-knob_angle*360.0/1024/16;
      float delta = cr_angle - prev_knob_angle;
      if(delta > 180){delta-=360;}if(delta < -180){delta+=360;}
      int32_t rx = delta/18;if(rx>127){rx=127;}if(rx<-127){rx=-127;}
      if(rx){
        prev_knob_angle = prev_knob_angle + rx*18;
        if(prev_knob_angle > 360){prev_knob_angle-=360;}
        if(prev_knob_angle < 0){prev_knob_angle+=360;}
      }

      hid_mouse_report_t report =
      {
        .x   = Xval/10, .y = Yval/10, .wheel = -rx, .pan = 0, 
        .buttons = (Press>PressLimit2?MOUSE_BUTTON_LEFT:0)
      };

      tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));
      return;
    }
    if(usb_mode == USB_SMART){
      
      hid_smartknob_report_t report;

      if(usb_fill_descriptor_callback != NULL)
        usb_fill_descriptor_callback(&report);

      tud_hid_report(0, &report, sizeof(report));
      return;
    }
  }
}

// Every 10ms, we will sent 1 report for each HID profile (keyboard, mouse etc ..)
// tud_hid_report_complete_cb() is used to send the next report after previous one is complete
void hid_task(void)
{
  // Poll every 10ms
  uint32_t interval_ms = 10;
  if(usb_mode == USB_SMART)
    interval_ms = 1;
  static uint32_t start_ms = 0;

  if ( board_millis() - start_ms < interval_ms) return; // not enough time
  start_ms += interval_ms;

const uint8_t btn = 0;
  // Remote wakeup
  if ( tud_suspended() && btn )
  {
    // Wake up host if we are in suspend mode
    // and REMOTE_WAKEUP feature is enabled by host
    tud_remote_wakeup();
  }else
  {
    // Send the 1st of report chain, the rest will be sent by tud_hid_report_complete_cb()
    send_hid_report(REPORT_ID_GAMEPAD);
  }
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len)
{
  (void) instance;
  (void) len;

  uint8_t next_report_id = report[0] + 1u;

  if (next_report_id < REPORT_ID_COUNT)
  {
    send_hid_report(next_report_id);
  }
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  // TODO not Implemented
  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;

  return 64;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) bufsize;

  if(usb_packetCallback != NULL)
    usb_packetCallback((uint8_t*)buffer, bufsize);
  // echo back anything we received from host
  //tud_hid_report(0, buffer, bufsize);
}



















//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // blink is disabled
  if (!blink_interval_ms) return;

  // Blink every interval ms
  if ( board_millis() - start_ms < blink_interval_ms) return; // not enough time
  start_ms += blink_interval_ms;

  //board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}