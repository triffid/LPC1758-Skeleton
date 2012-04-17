#define	DT_CDC_DESCRIPTOR           36
#define	DT_CDC_ENDPOINT             37

#define USB_CDC_SUBCLASS_ACM        0x02
#define USB_CDC_SUBCLASS_ETHERNET   0x06
#define USB_CDC_SUBCLASS_WHCM       0x08
#define USB_CDC_SUBCLASS_DMM        0x09
#define USB_CDC_SUBCLASS_MDLM       0x0a
#define USB_CDC_SUBCLASS_OBEX       0x0b
#define USB_CDC_SUBCLASS_EEM        0x0c
#define USB_CDC_SUBCLASS_NCM        0x0d

#define USB_CDC_SUBTYPE_HEADER           0x00
#define USB_CDC_SUBTYPE_CALL_MANAGEMENT  0x01
#define USB_CDC_SUBTYPE_ACM              0x02
#define USB_CDC_SUBTYPE_UNION            0x06
#define USB_CDC_SUBTYPE_COUNTRY          0x07
#define USB_CDC_SUBTYPE_NETWORK_TERMINAL 0x0a
#define USB_CDC_SUBTYPE_ETHERNET         0x0f
#define USB_CDC_SUBTYPE_WHCM             0x11
#define USB_CDC_SUBTYPE_MDLM             0x12
#define USB_CDC_SUBTYPE_MDLM_DETAIL      0x13
#define USB_CDC_SUBTYPE_DMM              0x14
#define USB_CDC_SUBTYPE_OBEX             0x15
#define USB_CDC_SUBTYPE_NCM              0x1a

typedef struct __attribute__ ((packed)) {
	U8	bLength;      // 5
	U8	bDescType;    // DT_CDC_DESCRIPTOR      (0x24)
	U8	bDescSubType; // USB_CDC_SUBTYPE_HEADER (0x00)
	U16	bcdCDC;
} usbcdc_header;
#define	USB_CDC_LENGTH_HEADER sizeof(usbcdc_header)

typedef struct __attribute__ ((packed)) {
	U8	bLength;      // 5
	U8	bDescType;    // DT_CDC_DESCRIPTOR      (0x24)
	U8	bDescSubType; // USB_CDC_SUBTYPE_CALL_MANAGEMENT (0x01)

	U8	bmCapabilities;
#define	USB_CDC_CALLMGMT_CAP_CALLMGMT	0x01
#define USB_CDC_CALLMGMT_CAP_DATAINTF	0x02

	U8	bDataInterface;
} usbcdc_callmgmt;
#define	USB_CDC_LENGTH_CALLMGMT sizeof(usbcdc_callmgmt)

typedef struct __attribute__ ((packed)) {
	U8	bLength;      // 4
	U8	bDescType;    // DT_CDC_DESCRIPTOR      (0x24)
	U8	bDescSubType; // USB_CDC_SUBTYPE_ACM    (0x02)

	U8	bmCapabilities;
#define	USB_CDC_ACM_CAP_COMM	0x01
#define USB_CDC_ACM_CAP_LINE	0x02
#define	USB_CDC_ACM_CAP_BRK		0x04
#define USB_CDC_ACM_CAP_NOTIFY	0x08
} usbcdc_acm;
#define	USB_CDC_LENGTH_ACM sizeof(usbcdc_acm)

typedef struct __attribute__ ((packed)) {
	U8	bLength;      // 5+
	U8	bDescType;    // DT_CDC_DESCRIPTOR      (0x24)
	U8	bDescSubType; // USB_CDC_SUBTYPE_UNION  (0x06)

	U8	bMasterInterface;
	U8	bSlaveInterface0;
} usbcdc_union;
#define	USB_CDC_LENGTH_UNION sizeof(usbcdc_union)

typedef struct __attribute__ ((packed)) {
	U8	bLength;							// 13
	U8	bDescType;						// DT_CDC_DESCRIPTOR      (0x24)
	U8	bDescSubType; 				// USB_CDC_SUBTYPE_ETHERNET (0x0F)

	U8	iMacAddress;					// index of MAC address string
	U32	bmEthernetStatistics;
	U16	wMaxSegmentSize;			// 1514?
	U16	wNumberMCFilters;			// 0
	U8	bNumberPowerFilters;	// 0
} usbcdc_ether;
#define USB_CDC_LENGTH_ETHER sizeof(usbcdc_ether)
