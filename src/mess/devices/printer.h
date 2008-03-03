/*
 * functions for the IODevice entry IO_PRINTER
 *
 * Currently only a simple port which only supports status
 * ("online/offline") and output of bytes is supported.
 */

#ifndef PRINTER_H
#define PRINTER_H

#include "device.h"
#include "image.h"

#ifdef __cplusplus
extern "C" {
#endif

	
#define MAX_PRINTER	(4)


int printer_status(mess_image *img, int newstatus);
void printer_output(mess_image *img, int data);

void printer_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);


#ifdef __cplusplus
}
#endif

#endif /* PRINTER_H */
