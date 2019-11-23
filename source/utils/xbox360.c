#ifdef HW_RVL
#include <gccore.h>
#include <ogc/usb.h>

#define USB_CLASS_XBOX360 0xFF

static bool xbox360Setup = false;
static bool replugRequired = false;
static s32 deviceIdXBOX360 = 0;
static u8 endpointXBOX360_IN = 0x81;
static u8 endpointXBOX360_OUT = 0x01;
static u8 bMaxPacketSizeXBOX360 = 20;
static u8 bConfigurationValueXBOX360 = 1;
static u8 ATTRIBUTE_ALIGN(32) buf[20];
static bool xbox360_read = false;
static u32 jp;
static u8 player = 0;
static u32 xboxButtonCount = 0;
static bool nextPlayer = false;

static u8 getEndpoint(usb_devdesc devdesc)
{
	if (devdesc.configurations == NULL || devdesc.configurations->interfaces == NULL ||
			devdesc.configurations->interfaces->endpoints == NULL)
	{
		return -1;
	}
	return devdesc.configurations->interfaces->endpoints->bEndpointAddress;
}

static bool isXBOX360(usb_devdesc devdesc)
{
    return (devdesc.idVendor == 0x045e && devdesc.idProduct == 0x028e && getEndpoint(devdesc) == endpointXBOX360_IN);
}

static int read_XBOX360(s32 device_id, u8 endpoint, u8 bMaxPacketSize0);

static void start_reading_XBOX360(s32 device_id, u8 endpoint, u8 bMaxPacketSize0)
{
    if (xbox360_read)
    {
        // already reading
        return;
    }
    xbox360_read = true;
    read_XBOX360(deviceIdXBOX360, endpointXBOX360_IN, bMaxPacketSize0);
}

static void stop_reading_XBOX360()
{
    xbox360_read = false;
}

static int read_XBOX360_cb(int res, void *usrdata)
{
    if (!xbox360_read)
    {
        // stop reading
        return 1;
    }

    // NOTE: The four startup messages have res = 3 and can be ignored

    // Button layout
    // A=3,10
    // B=3,20
    // X=3,40
    // Y=3,80
    // Up=2,01
    // Right=2,08
    // Left=2,04
    // Down=2,02
    // L=3,01
    // R=3,02
    // L2=4,FF ; analog trigger
    // R3=5,FF ; analog trigger
    // L3=2,40 ; left hat button
    // L3=2,80 ; right hat button
    // XBOX=3,04
    // Start=2,10
    // Back=2,20
    // LStickX=6/7,FF ; <low byte>,<high byte>
    // LStickY=8/9,FF
    // RStickX=10/11,FF
    // RStickY=12/13,FF

    if (res == 20)
    {
        jp = 0;
        jp |= ((buf[2] & 0x01) == 0x01) ? PAD_BUTTON_UP    : 0;
        jp |= ((buf[2] & 0x02) == 0x02) ? PAD_BUTTON_DOWN  : 0;
        jp |= ((buf[2] & 0x04) == 0x04) ? PAD_BUTTON_LEFT  : 0;
        jp |= ((buf[2] & 0x08) == 0x08) ? PAD_BUTTON_RIGHT : 0;

        jp |= ((buf[3] & 0x10) == 0x10) ? PAD_BUTTON_B : 0; // XBOX360 A button maps to B
        jp |= ((buf[3] & 0x20) == 0x20) ? PAD_BUTTON_A : 0; // XBOX360 B button maps to A
        jp |= ((buf[3] & 0x40) == 0x40) ? PAD_BUTTON_Y : 0; // XBOX360 X button maps to Y
        jp |= ((buf[3] & 0x80) == 0x80) ? PAD_BUTTON_X : 0; // XBOX360 Y button maps to X

        jp |= ((buf[3] & 0x01) == 0x01) ? PAD_TRIGGER_L : 0;
        jp |= ((buf[3] & 0x02) == 0x02) ? PAD_TRIGGER_R : 0;

        jp |= ((buf[2] & 0x10) == 0x10) ? PAD_BUTTON_START : 0;
        jp |= ((buf[2] & 0x20) == 0x20) ? PAD_TRIGGER_Z    : 0; // XBOX360 back button maps to Z

        // triggers
        jp |= (buf[4] > 128) ? PAD_TRIGGER_L : 0;
        jp |= (buf[5] > 128) ? PAD_TRIGGER_R : 0;

        // left stick
        int16_t lx = (buf[7] << 8) | buf[6]; // [-32768, 32767]
        int16_t ly = (buf[9] << 8) | buf[8]; // [-32768, 32767]
        jp |= (ly >  16384) ? PAD_BUTTON_UP    : 0;
        jp |= (ly < -16384) ? PAD_BUTTON_DOWN  : 0;
        jp |= (lx < -16384) ? PAD_BUTTON_LEFT  : 0;
        jp |= (lx >  16384) ? PAD_BUTTON_RIGHT : 0;

        // right stick
        int16_t rx = (buf[11] << 8) | buf[10]; // [-32768, 32767]
        int16_t ry = (buf[13] << 8) | buf[12]; // [-32768, 32767]
        jp |= (ry >  16384) ? PAD_BUTTON_X : 0; // XBOX360 Y button maps to X
        jp |= (ry < -16384) ? PAD_BUTTON_B : 0; // XBOX360 A button maps to B
        jp |= (rx < -16384) ? PAD_BUTTON_Y : 0; // XBOX360 X button maps to Y
        jp |= (rx >  16384) ? PAD_BUTTON_A : 0; // XBOX360 B button maps to A

		// XBOX button to switch to next player
        if ((buf[3] & 0x04) == 0x04)
        {
            xboxButtonCount++;
            // count =  2 means you have to push the button 1x to switch players
            // count = 10 means you have to push the button 5x to switch players
            if (xboxButtonCount >= 2)
            {
                nextPlayer = true;
                xboxButtonCount = 0;
            }
        }
    }

    // read again
    read_XBOX360(deviceIdXBOX360, endpointXBOX360_IN, bMaxPacketSizeXBOX360);

    return 1;
}

// never call directly
static int read_XBOX360(s32 device_id, u8 endpoint, u8 bMaxPacketSize0)
{
    // need to use async, because USB_ReadIntrMsg() blocks until a button is pressed
    return USB_ReadIntrMsgAsync(device_id, endpoint, sizeof(buf), buf, &read_XBOX360_cb, NULL);
}

static void turnOnLED()
{
    uint8_t ATTRIBUTE_ALIGN(32) buf[] = { 0x01, 0x03, 0x06 + player };
    USB_WriteIntrMsg(deviceIdXBOX360, endpointXBOX360_OUT, sizeof(buf), buf);
}

static void increasePlayer()
{
    player++;
    if (player > 3)
    {
        player = 0;
    }
    turnOnLED();
}

void rumble_XBOX360(s32 device_id, u8 left, u8 right)
{
    uint8_t ATTRIBUTE_ALIGN(32) buf[] = { 0x00, 0x08, 0x00, left, right, 0x00, 0x00, 0x00 };
    USB_WriteIntrMsg(deviceIdXBOX360, endpointXBOX360_OUT, sizeof(buf), buf);
}

static int removal_cb(int result, void *usrdata)
{
    s32 fd = (s32) usrdata;
    if (fd == deviceIdXBOX360)
    {
        stop_reading_XBOX360();
        deviceIdXBOX360 = 0;
    }
    return 1;
}

static void openXBOX360()
{
    if (deviceIdXBOX360 != 0)
    {
        return;
    }

    usb_device_entry dev_entry[8];
    u8 dev_count;
    if (USB_GetDeviceList(dev_entry, 8, USB_CLASS_XBOX360, &dev_count) < 0)
    {
        return;
    }

    int i;
    for	(i = 0; i < dev_count; ++i)
    {
        s32 fd;
        if (USB_OpenDevice(dev_entry[i].device_id, dev_entry[i].vid, dev_entry[i].pid, &fd) < 0)
        {
            continue;
        }

        usb_devdesc devdesc;
        if (USB_GetDescriptors(fd, &devdesc) < 0)
        {
            // You have to replug the XBOX360 controller!
			replugRequired = true;
            USB_CloseDevice(&fd);
            continue;
        }

        if (isXBOX360(devdesc) && USB_SetConfiguration(fd, bConfigurationValueXBOX360) >= 0)
        {
            deviceIdXBOX360 = fd;
			replugRequired = false;
			turnOnLED();
			USB_DeviceRemovalNotifyAsync(fd, &removal_cb, (void*) fd);
        }
        else
        {
            USB_CloseDevice(&fd);
        }
    }

    xbox360Setup = true;
}

void XBOX360_ScanPads()
{
	if(!xbox360Setup)
	{
		openXBOX360();
	}

	if (deviceIdXBOX360 == 0)
	{
		return;
	}

	start_reading_XBOX360(deviceIdXBOX360, endpointXBOX360_IN, bMaxPacketSizeXBOX360);
}

u32 XBOX360_ButtonsHeld(int chan)
{
    if (deviceIdXBOX360 == 0)
    {
        return 0;
    }
    if (nextPlayer)
    {
        nextPlayer = false;
        increasePlayer();
    }
	if (chan != player)
	{
		return 0;
	}
	return jp;
}

char* XBOX360_Status()
{
	openXBOX360();
	if (replugRequired)
		return "please replug";
	return deviceIdXBOX360 ? "connected" : "not found";
}

#endif
