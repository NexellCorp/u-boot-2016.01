#define VENDORID			0x04e8	/* Samsung Vendor ID */
#define PRODUCTID			0x1234	/* Nexell Product ID */
#define NXP4330_USBD_VID		0x2375
#define NXP4330_USBD_PID		0x4330

/* @brief ECID Module's Register List */
struct  nx_ecid_registerset {
	u32 ecid[4];           /* 0x00 ~ 0x0C	: 128bit ECID Register */
	u8  chipname[48];      /* 0x10 ~ 0x3C	: Chip Name Register */
	u32 reserved;          /* 0x40		: Reserved Region */
	u32 guid0;             /* 0x44		: GUID 0 Register */
	u16 guid1;             /* 0x48		: GUID 1 Register */
	u16 guid2;             /* 0x4A		: GUID 2 Register */
	u8  guid3[8];          /* 0x4C ~ x50    : GUID 3-0 ~ 3-7 Register */
	u32 ec[3];             /* 0x54 ~ 0x5C	: EC 0 ~ 3 Register */
};

void cal_usbid(u16 *vid, u16 *pid, u32 ecid);
void get_usbid(u16 *vid, u16 *pid);
