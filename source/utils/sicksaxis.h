/* SickSaxis lib 1.0 by xerpi */

#ifndef _SICKSAXIS_H_
#define _SICKSAXIS_H_

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <gcutil.h>
#include <ogcsys.h>
#include <ogc/ipc.h>
#include <ogc/usb.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>

#define SS_DEBUG

#ifdef SS_DEBUG
	#define ss_debug_printf printf
#else
	#define ss_debug_printf(...) ((void)0)
#endif

#define swap16(s) ((s >> 8) | (s << 8))

#define SS_DATA_LEN        53 //Should be 49
#define SS_LEDS_RUMBLE_LEN 32
#define SS_VENDOR_ID       0x054C //Sony Corp.
#define SS_PRODUCT_ID      0x0268 //Sixaxis and DS3
#define	SS_HEAP_SIZE       4096
#define SS_MAX_DEV         8 //Was 7
#define SS_PAYLOAD_SIZE 49 //New
#define SS_SLEEP_USECONDS  1000
#define SS_THREAD_PRIORITY 20


typedef enum
{
	SS_STATUS_NOT_INITIALIZED = 0x0,
	SS_STATUS_INITIALIZED
}ss_status_t;

/*struct SS_BUTTONS    //Little endian
{
    uint8_t select   : 1;
    uint8_t L3       : 1;
    uint8_t R3       : 1;
    uint8_t start    : 1;
    uint8_t up       : 1;
    uint8_t right    : 1;
    uint8_t down     : 1;
    uint8_t left     : 1;
    uint8_t L2       : 1;
    uint8_t R2       : 1;
    uint8_t L1       : 1;
    uint8_t R1       : 1;
    uint8_t triangle : 1;
    uint8_t circle   : 1;
    uint8_t cross    : 1;
    uint8_t square   : 1;
    uint8_t PS       : 1;
    uint8_t not_used : 7;
};*/

struct SS_BUTTONS    //Big endian
{
	uint8_t left     : 1;
	uint8_t down     : 1;
	uint8_t right    : 1;
	uint8_t up       : 1;
	uint8_t start    : 1;
	uint8_t R3       : 1;
	uint8_t L3       : 1;
	uint8_t select   : 1;
	
	uint8_t square   : 1;
	uint8_t cross    : 1;
	uint8_t circle   : 1;
	uint8_t triangle : 1;
	uint8_t R1       : 1;
	uint8_t L1       : 1;
	uint8_t R2       : 1;
	uint8_t L2       : 1;
	 
    uint8_t not_used : 7;
    uint8_t PS       : 1;
};

struct SS_ANALOG
{
    uint8_t x;
    uint8_t y;
};

struct SS_DPAD_SENSITIVE
{
    uint8_t up;
    uint8_t right;
    uint8_t down;
    uint8_t left;
};

struct SS_SHOULDER_SENSITIVE
{
    uint8_t L2;
    uint8_t R2;
    uint8_t L1;
    uint8_t R1;
};

struct SS_BUTTON_SENSITIVE
{
    uint8_t triangle;
    uint8_t circle;
    uint8_t cross;
    uint8_t square;
};

struct SS_MOTION
{
    uint16_t accX;
    uint16_t accY;
    uint16_t accZ;
    uint16_t Zgyro;
};

struct SS_GAMEPAD
{
    uint8_t                        HIDdata;
    uint8_t                        unk0;
    struct SS_BUTTONS              buttons;
    uint8_t                        unk1;
    struct SS_ANALOG               leftAnalog;
    struct SS_ANALOG               rightAnalog;
    uint32_t                       unk2;
    struct SS_DPAD_SENSITIVE       dpad_sens;
    struct SS_SHOULDER_SENSITIVE   shoulder_sens;
    struct SS_BUTTON_SENSITIVE     button_sens;
    uint16_t                       unk3;
    uint8_t                        unk4;
    uint8_t                        status;
    uint8_t                        power_rating;
    uint8_t                        comm_status;
    uint32_t                       unk5;
    uint32_t                       unk6;
    uint8_t                        unk7;
    struct SS_MOTION               motion;
    uint8_t                        padding[3];
}__attribute__((packed));


typedef struct
{
	uint8_t connected, continue_reading;
	int32_t fd, dev_id;
	struct SS_GAMEPAD __attribute__ ((aligned(32))) gamepad;
	uint8_t __attribute__ ((aligned(32))) leds_rumble[SS_LEDS_RUMBLE_LEN];
}ss_instance_t;



//Functions
	int  ss_init();
	int  ss_end();
	int  ss_open(ss_instance_t *ss);
	int  ss_close(ss_instance_t *ss);
	void ss_start_reading(ss_instance_t *ss);
	void ss_stop_reading(ss_instance_t *ss);
	int  ss_set_operational_mode(ss_instance_t *ss);
	void ss_set_led(ss_instance_t *ss, uint8_t led);
	void ss_set_rumble(ss_instance_t *ss, uint8_t duration_right, uint8_t power_right, uint8_t duration_left, uint8_t power_left);
	void ss_get_mac(ss_instance_t *ss, uint8_t *mac);
	void ss_set_mac(ss_instance_t *ss, uint8_t *mac);
	
//Lib only functions
	int  _ss_read(ss_instance_t *ss);
	int  _ss_instance_list_dev_exists(int32_t dev_id);
	void _ss_instance_list_delete(ss_instance_t *ss);
	void _ss_instance_list_add(ss_instance_t *ss);
//Callback/Thread functions
	int      _ss_read_function(ss_instance_t *ss);
	void    *_ss_thread_function(void *arg);
	int32_t  _ss_disconnect_cb(int32_t result, void *usrdata);
	int32_t  _ss_usbcallback(int32_t result, void *usrdata);


#endif
