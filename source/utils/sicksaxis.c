/* SickSaxis lib 1.0 by xerpi */
#ifdef HW_RVL
#include "sicksaxis.h"

/*
 * the total time the led is active (0xff means forever)
 * |     duty_length: how long a cycle is in deciseconds:
 * |     |                              (0 means "blink very fast")
 * |     |     ??? (Maybe a phase shift or duty_length multiplier?)
 * |     |     |     % of duty_length led is off (0xff means 100%)
 * |     |     |     |     % of duty_length led is on (0xff is 100%)
 * |     |     |     |     |
 * 0xff, 0x27, 0x10, 0x00, 0x32,
 */
static uint8_t leds_rumble_payload[SS_LEDS_RUMBLE_LEN] =
{
	0x52, //0x52
	0x01,
	0x00, 0x00, 0x00, //Rumble
	0xff, 0x80, //Gyro
	0x00, 0x00,
	0x00, //* LED_1 = 0x02, LED_2 = 0x04, ... */
    0xff, 0x27, 0x10, 0x00, 0x32, /* LED_4 */
    0xff, 0x27, 0x10, 0x00, 0x32, /* LED_3 */
    0xff, 0x27, 0x10, 0x00, 0x32, /* LED_2 */
    0xff, 0x27, 0x10, 0x00, 0x32, /* LED_1 */
};

static uint8_t ss_led_pattern[8] = {0x0, 0x02, 0x04, 0x08, 0x10, 0x12, 0x14, 0x18};

static ss_status_t     _ss_status = SS_STATUS_NOT_INITIALIZED;
static int32_t         _ss_heapid = -1;
static ss_instance_t * _ss_instance_list[SS_MAX_DEV];

//Functions
	int ss_init()
	{
		//Check if you have already initialized the lib
		if(_ss_status == SS_STATUS_INITIALIZED)
			return 2;

		//If not, check if the heap is already allocated
		if(_ss_heapid > 0)
			return 3;

		//If not, allocate it
		_ss_heapid = iosCreateHeap(SS_HEAP_SIZE);
		
		//Check for an allocation error
		if(_ss_heapid < 0)
			return -1;

		//Clean instance list
		uint8_t i;
		for(i = 0; i < SS_MAX_DEV; i++)
			_ss_instance_list[i] = NULL;

		_ss_status = SS_STATUS_INITIALIZED;
		ss_debug_printf("sicksaxis lib initialized: Heap created. "
						"hId: %i length: %i\n" ,_ss_heapid, SS_HEAP_SIZE);
		return 1;
	}


	int ss_end()
	{
		_ss_status = SS_STATUS_NOT_INITIALIZED;
		//Destroy heap ¿?
		return 1;
	}

	int ss_open(ss_instance_t *ss)
	{
		//Already connected
		if(ss->connected)
			return 2;
		//Heap not initialized
		if(_ss_heapid < 0)
			return -1;

		uint8_t usb_dev_count;
		usb_device_entry usb_dev_entry_list[SS_MAX_DEV];

		//Get USB device list
		if (USB_GetDeviceList(usb_dev_entry_list, SS_MAX_DEV, USB_CLASS_HID, &usb_dev_count) < 0)
		{
			ss_debug_printf("Error getting device list.\n");
			return -2;
		}
		ss_debug_printf("Found %i usb device(s).\n", usb_dev_count);
		if(usb_dev_count == 0)
			return -3;
			
		//Search for Sixaxis and DualShock3 controllers
		uint8_t i;
		usb_device_entry *de;
		for(i = 0; i < usb_dev_count; i++)
		{
			de = &usb_dev_entry_list[i];
			//Is it a 6AXIS or a DS3?
			if( de->vid == SS_VENDOR_ID && de->pid == SS_PRODUCT_ID)
			{
				//OK, it is. Now check whether we have opened it or not
				if(!_ss_instance_list_dev_exists(de->device_id))
				{
					//Yeah it hasn't been opened, so open it!
						if(USB_OpenDevice(de->device_id, SS_VENDOR_ID, SS_PRODUCT_ID, &ss->fd) < 0)
						{
							ss_debug_printf("Could not open the controller. fd: %i\n", ss->fd);
							goto exit_close;
						}
						ss->dev_id = de->device_id;
				}
			}
		}
		//Now the device is opened, so we have to set the disconnect callback
		if(USB_DeviceRemovalNotifyAsync(ss->fd, &_ss_disconnect_cb, (void *)ss) < 0)
		{
			ss_debug_printf("Error setting the device disconnect callback.\n");
			goto exit_close;
		}
		printf("dev_id: %i   fd: %i\n", ss->dev_id, ss->fd);

		//Setting operational mode
		ss_debug_printf("Setting operational mode...");
		if(ss_set_operational_mode(ss) < 0)
		{
			ss_debug_printf("Error.\n");
			goto exit_close;
		}
		ss_debug_printf("Done!\n");

		//Everything went OK!
		ss->connected = 1;
		ss->continue_reading = 0;
		ss_debug_printf("sicksaxis opened!\n");
		memcpy ((void *)ss->leds_rumble, (void *)leds_rumble_payload, SS_LEDS_RUMBLE_LEN);
		_ss_instance_list_add(ss);

		return 1;	
			
	exit_close:
		USB_CloseDevice(&ss->fd);
		return 0;
	}

	int ss_close(ss_instance_t *ss)
	{
		if(ss->connected)
		{
			_ss_instance_list_delete(ss);
			ss->connected = 0;
			ss->continue_reading = 0;
			USB_CloseDevice(&ss->fd);
		}
		return 1;
	}

	void ss_start_reading(ss_instance_t *ss)
	{
		if(!ss->continue_reading)
		{
			ss->continue_reading = 1;
			_ss_read(ss);
		}
	}
	
	void ss_stop_reading(ss_instance_t *ss)
	{
		ss->continue_reading = 0;
	}

	int ss_set_operational_mode(ss_instance_t *ss)
	{
		uint8_t ATTRIBUTE_ALIGN(32) buf[17];
		int ret = USB_WriteCtrlMsgAsync(ss->fd,
							 USB_REQTYPE_INTERFACE_GET,
							 USB_REQ_GETREPORT,
							 (USB_REPTYPE_FEATURE<<8) | 0xf2,
							 0x0,
							 17,
							 buf,
							_ss_usbcallback,
							NULL
							);

		return ret;
	}	

	int _ss_read(ss_instance_t *ss)
	{                           
		return USB_WriteCtrlMsgAsync(
							ss->fd,
							USB_REQTYPE_INTERFACE_GET,
							USB_REQ_GETREPORT,
							(USB_REPTYPE_INPUT<<8) | 0x1,
							0x0,
							SS_DATA_LEN,
							(uint8_t *)&(ss->gamepad),
							_ss_usbcallback,
							(void *)ss
							);
	}

	void ss_set_led(ss_instance_t *ss, uint8_t led)
	{
		ss->leds_rumble[9] = ss_led_pattern[led];
		USB_WriteCtrlMsgAsync(ss->fd,
						USB_REQTYPE_INTERFACE_SET,
						USB_REQ_SETREPORT,
						(USB_REPTYPE_OUTPUT<<8) | 0x01,
						0x0,
						SS_LEDS_RUMBLE_LEN,
						ss->leds_rumble,
						_ss_usbcallback,
						NULL
						);
												
	}


	void ss_set_rumble(ss_instance_t *ss, uint8_t duration_right, uint8_t power_right, uint8_t duration_left, uint8_t power_left)
	{
		ss->leds_rumble[1] = duration_right;
		ss->leds_rumble[2] = power_right;
		ss->leds_rumble[3] = duration_left;
		ss->leds_rumble[4] = power_left;
		USB_WriteCtrlMsgAsync(ss->fd,
						USB_REQTYPE_INTERFACE_SET,
						USB_REQ_SETREPORT,
						(USB_REPTYPE_OUTPUT<<8) | 0x01,
						0x0,
						SS_LEDS_RUMBLE_LEN,
						ss->leds_rumble,
						_ss_usbcallback,
						NULL
						);
	}

	void ss_get_mac(ss_instance_t *ss, uint8_t *mac)
	{
		uint8_t ATTRIBUTE_ALIGN(32) msg[8];
		USB_WriteCtrlMsgAsync(ss->fd,
					   USB_REQTYPE_INTERFACE_GET,
					   USB_REQ_GETREPORT,
					   (USB_REPTYPE_FEATURE<<8) | 0xf5,
					   0,
					   8,
					   msg,
					   _ss_usbcallback,
					   NULL);

		mac[0] = msg[2];
		mac[1] = msg[3];
		mac[2] = msg[4];
		mac[3] = msg[5];
		mac[4] = msg[6];
		mac[5] = msg[7];
	}
	
	void ss_set_mac(ss_instance_t *ss, uint8_t *mac)
	{
		uint8_t ATTRIBUTE_ALIGN(32) msg[8] = {0x01, 0x00, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]};
		USB_WriteCtrlMsgAsync(ss->fd,
					USB_REQTYPE_INTERFACE_SET,
					USB_REQ_SETREPORT,
					(USB_REPTYPE_FEATURE<<8) | 0xf2,
					0,
					8,
					msg,
					_ss_usbcallback,
					NULL);
	}

//Lib only functions
	int _ss_instance_list_dev_exists(int32_t dev_id)
	{
		uint8_t i;
		for(i = 0; i < SS_MAX_DEV; i++)
		{
			if(_ss_instance_list[i] != NULL)
			{
				if(_ss_instance_list[i]->dev_id == dev_id)
				{
					return 1;
				}
			}
		}
		return 0;
	}

	void _ss_instance_list_delete(ss_instance_t *ss)
	{
		uint8_t i;
		for(i = 0; i < SS_MAX_DEV; i++)
		{
			if(_ss_instance_list[i] == NULL)
			{
				_ss_instance_list[i] = ss;
			}
		}
	}

	void _ss_instance_list_add(ss_instance_t *ss)
	{
		uint8_t i;
		for(i = 0; i < SS_MAX_DEV; i++)
		{
			if(_ss_instance_list[i] == ss)
			{
				_ss_instance_list[i] = NULL;
			}
		}
	}

//Callback functions
	int32_t _ss_disconnect_cb(s32 result, void *usrdata)
	{
		ss_instance_t *ss = (ss_instance_t *)usrdata;
		ss_close(ss);
		return 1;
	}

	int32_t _ss_usbcallback(int32_t result, void *usrdata)
	{
		if(usrdata == NULL) return 0;
		ss_instance_t *ss = (ss_instance_t *)usrdata;
		//Read again?
		if(ss->continue_reading && ss->connected)
			_ss_read(ss);
		return 1;
	}

#endif
