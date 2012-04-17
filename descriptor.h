#include <stdint.h>

#include	"descriptor_cdc.h"

#define DL_DEVICE                   0x12
#define DL_CONFIGURATION            0x09
#define DL_INTERFACE                0x09
#define DL_ENDPOINT                 0x07

#define mA                          /2

#define DT_DEVICE                   1
#define DT_CONFIGURATION            2
#define DT_STRING                   3
#define DT_INTERFACE                4
#define DT_ENDPOINT                 5

#define USB_VERSION_1_0             0x0100
#define USB_VERSION_1_1             0x0101
#define USB_VERSION_2_0             0x0200
#define USB_VERSION_3_0             0x0300

#define UC_PER_INTERFACE            0
#define UC_AUDIO                    1
#define UC_COMM                     2
#define UC_HID                      3
#define UC_PHYSICAL                 5
#define UC_STILL_IMAGE              6
#define UC_PRINTER                  7
#define UC_MASS_STORAGE             8
#define UC_HUB                      9
#define UC_CDC_DATA                 10
#define UC_CSCID                    11
#define UC_CONTENT_SEC              13
#define UC_VIDEO                    14
#define UC_WIRELESS_CONTROLLER      224
#define UC_MISC                     239
#define UC_APP_SPEC                 254
#define UC_VENDOR_SPEC              255

#define CA_BUSPOWERED               0x80
#define CA_SELFPOWERED              0x40
#define CA_REMOTEWAKEUP             0x20

#define EA_CONTROL                  0x00
#define EA_ISOCHRONOUS              0x01
#define EA_BULK                     0x02
#define EA_INTERRUPT                0x03

#define EA_ISO_NONE                 0x00
#define EA_ISO_ASYNC                0x04
#define EA_ISO_ADAPTIVE             0x08
#define EA_ISO_SYNC                 0x0C

#define EA_ISO_TYPE_DATA            0x00
#define EA_ISO_TYPE_FEEDBACK        0x10
#define EA_ISO_TYPE_EXPLICIT        0x20

#define SL_USENGLISH                0x0409
#define SL_AUENGLISH                0x0C09
#define SL_GERMAN                   0x0407



typedef struct {
	U8	bLength;			// descriptor length
	U8	bDescType;			// descriptor type: see enum DESCRIPTOR_TYPE
} usbdesc_base;

typedef struct {
	U8	bLength;			// Device descriptor length (0x12)
	U8	bDescType;			// DT_DEVICE (0x01)
	U16	bcdUSB;				// USB Specification Number which device complies to - see USB_Version_Enum
	U8	bDeviceClass;		// USB Device Class - see Device_Class_Enum
	U8	bDeviceSubClass;	// Subclass Code
	U8	bDeviceProtocol;	// Protocol Code
	U8	bMaxPacketSize;		// Maximum Packet Size for Zero Endpoint. Valid Sizes are 8, 16, 32, 64
	U16	idVendor;			// Vendor ID
	U16	idProduct;			// Product ID
	U16	bcdDevice;			// Device Release Number
	U8	iManufacturer;		// Index of Manufacturer String Descriptor
	U8	iProduct;			// Index of Product String Descriptor
	U8	iSerialNumber;		// Index of Serial Number String Descriptor
	U8	bNumConfigurations;	// Number of Possible Configurations
} usbdesc_device;

typedef struct __attribute__ ((packed)) {
	U8	bLength;				// Configuration Descriptor Length (0x09)
	U8	bDescType;				// DT_CONFIGURATION (0x02)
	U16	wTotalLength;			// Total length in bytes of this descriptor plus all this configuration's interfaces plus their endpoints, see http://www.beyondlogic.org/usbnutshell/confsize.gif
	U8	bNumInterfaces;			// Number of Interfaces
	U8	bConfigurationValue;	// Value to use as an argument to select this configuration
	U8	iConfiguration;			// Index of String Descriptor describing this configuration
	U8	bmAttributes;			// bitmap. see Config_Attributes_Enum
	U8	bMaxPower;				// Max. Current = bMaxPower * 2mA
} usbdesc_configuration;

typedef struct __attribute__ ((packed)) {
	U8	bLength;				// Interface Descriptor Length (0x09)
	U8	bDescType;				// DT_INTERFACE (0x04)
	U8	bInterfaceNumber;		// Number of Interface
	U8	bAlternateSetting;		// Value used to select alternative setting
	U8	bNumEndPoints;			// Number of Endpoints used for this interface
	U8	bInterfaceClass;		// Class Code - see Device_Class_Enum
	U8	bInterfaceSubClass;		// Subclass Code
	U8	bInterfaceProtocol;		// Protocol Code
	U8	iInterface;				// Index of String Descriptor Describing this interface
} usbdesc_interface;

typedef struct __attribute__ ((packed)) {
	U8	bLength;				// Endpoint Descriptor Length (0x07)
	U8	bDescType;				// DT_ENDPOINT (0x05)
	U8	bEndpointAddress;		// 0x00-0x0F = OUT endpoints, 0x80-0x8F = IN endpoints
	U8	bmAttributes;			// bitmap, see Endpoint_Attributes_Enum
	U16	wMaxPacketSize;			// Maximum Packet Size this endpoint is capable of sending or receiving
	U8	bInterval;				// Interval for polling endpoint data transfers. Value in frame counts. Ignored for Bulk & Control Endpoints. Isochronous must equal 1 and field may range from 1 to 255 for interrupt endpoints.
} usbdesc_endpoint;

typedef struct __attribute__ ((packed)) {
	U8	bLength;				// String Descriptor Length (2 + 2*nLang)
	U8	bDescType;				// DT_STRING (0x03)
	U16	wLangID[1];				// language code(s)
} usbdesc_language;

typedef struct __attribute__ ((packed)) {
	U8	bLength;				// 2 + strlen
	U8	bDescType;				// DT_STRING (0x03)
	U16	str[];					// UNICODE string
} usbdesc_string;

#define usbdesc_string_l(l) struct __attribute__ ((packed)) { U8 bLength; U8 bDescType; U16 str[l]; }
