#include <common.h>
#include <console.h>
#include <asm/io.h>
#include <asm/arch/nexell.h>
#include <asm/arch/usbid.h>
#ifdef CONFIG_SYS_BURNING
#include <g_dnl.h>
#include <usb.h>
#endif

void cal_usbid(u16 *vid, u16 *pid, u32 ecid)
{
	if (ecid == 0) {   /* ecid is not burned */
		*vid = VENDORID;
		*pid = PRODUCTID;
		debug("\nECID Null!!\nVID %x, PID %x\n", *vid, *pid);
	} else {
		*vid = (ecid >> 16)&0xFFFF;
		*pid = (ecid >> 0)&0xFFFF;
		debug("VID %x, PID %x\n", *vid, *pid);
	}
}

void get_usbid(u16 *vid, u16 *pid)
{
	struct nx_ecid_registerset * const nx_ecidreg
		= (struct nx_ecid_registerset *)PHY_BASEADDR_ECID;
	char *cmp_name = "NEXELL-NXP4330-R0-LF3000";
	char name[49];
	int i;

	for (i = 0 ; i < 48 ; i++)
		name[i] = (char)nx_ecidreg->chipname[i];

	for (i = 0; i < 48; i++) {
		if ((name[i] == '-') && (name[i+1] == '-')) {
			name[i] = 0;
			name[i+1] = 0;
		}
	}

	if (!strcmp(name, cmp_name)) {
		*vid = NXP4330_USBD_VID;
		*pid = NXP4330_USBD_PID;
	} else {
		u32 id = readl(&nx_ecidreg->ecid[3]);

		cal_usbid(vid, pid, id);
	}
}

#ifdef CONFIG_SYS_BURNING
int g_dnl_bind_fixup(struct usb_device_descriptor *dev, const char *name)
{
	u16 vid, pid;

	get_usbid(&vid, &pid);
	debug("%s %x %x\n", __func__, vid, pid);

	put_unaligned(vid, &dev->idVendor);
	put_unaligned(pid, &dev->idProduct);

	return 0;
}
#endif
