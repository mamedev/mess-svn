/*
  pc parallel port/centronics interface

  started as some ttl's, is now available on several multi io chips
*/


#include "driver.h"
#include "memconv.h"
#include "includes/pclpt.h"
#include "machine/centroni.h"


#define LOG(LEVEL,N,M,A)  \
	do { \
		if( M ) \
			logerror("%11.6f: %-24s",attotime_to_double(timer_get_time(machine)),(char*)M ); \
		logerror A; \
	} while (0)


#define VERBOSE_LPT 1

#define LPT_LOG(n,m,a) LOG(VERBOSE_LPT,n,m,a)

#define STATUS_NOT_BUSY 0x80
#define STATUS_ACKNOWLEDGE 0x40
#define STATUS_NO_PAPER 0x20
#define STATUS_ONLINE 0x10
#define STATUS_NO_ERROR 8

#define CONTROL_SELECT 8
#define CONTROL_NO_RESET 4
#define CONTROL_AUTOLINEFEED 2
#define CONTROL_STROBE 1
#define CONTROL_IRQ 0x10

typedef struct {
	const CENTRONICS_DEVICE *device;
	const PC_LPT_CONFIG *config;
	int on;
	UINT8 data;
	UINT8 status;
	UINT8 control;
} PC_LPT;
static PC_LPT LPT[3]= {
	{ 0 }
};

void pc_lpt_config(int nr, const PC_LPT_CONFIG *config)
{
	PC_LPT *This=LPT+nr;
	This->config=config;
}

void pc_lpt_set_device(int nr, const CENTRONICS_DEVICE *device)
{
	PC_LPT *This=LPT+nr;
	This->device=device;
}

static void pc_LPT_w(running_machine *machine, int n, int offset, int data)
{
	PC_LPT *This=LPT+n;
	if ( !(input_port_read(machine, "DSW1") & (0x08>>n)) ) return;
//	if (!This->on) return;
	switch( offset )
	{
		case 0:
			if (This->device->write_data)
				This->device->write_data(machine, n, data);
			This->data = data;
			LPT_LOG(1,"LPT_data_w",("LPT%d $%02x\n", n, data));
			break;
		case 1:
			LPT_LOG(1,"LPT_status_w",("LPT%d $%02x\n", n, data));
			break;
		case 2:
			This->control = data;
			LPT_LOG(1,"LPT_control_w",("%d $%02x\n", n, data));
			if (This->device->handshake_out) {
				int lines=0;
				if (data&CONTROL_SELECT) lines|=CENTRONICS_SELECT;
				if (data&CONTROL_NO_RESET) lines|=CENTRONICS_NO_RESET;
				if (data&CONTROL_AUTOLINEFEED) lines|=CENTRONICS_AUTOLINEFEED;
				if (data&CONTROL_STROBE) lines|=CENTRONICS_STROBE;
				This->device->handshake_out(machine, n, lines, CENTRONICS_SELECT
											|CENTRONICS_NO_RESET
											|CENTRONICS_AUTOLINEFEED
											|CENTRONICS_STROBE);
			}
			break;
    }
}

static int pc_LPT_r(running_machine *machine, int n, int offset)
{
	PC_LPT *This=LPT+n;
    int data = 0xff;
	if ( !(input_port_read(machine, "DSW1") & (0x08>>n)) ) return data;
//	if (!This->on) return data;
	switch( offset )
	{
		case 0:
			if (This->device->read_data)
				data=This->device->read_data(machine,n);
			else
				data = This->data;
			LPT_LOG(1,"LPT_data_r",("LPT%d $%02x\n", n, data));
			break;
		case 1:
			if (This->device&&This->device->handshake_in) {
				int lines=This->device->handshake_in(machine,n);
				data=0;
				if (lines&CENTRONICS_NOT_BUSY) data|=STATUS_NOT_BUSY;
				if (lines&CENTRONICS_ACKNOWLEDGE) data|=STATUS_ACKNOWLEDGE;
				if (lines&CENTRONICS_NO_PAPER) data|=STATUS_NO_PAPER;
				if (lines&CENTRONICS_ONLINE) data|=STATUS_ONLINE;
				if (lines&CENTRONICS_NO_ERROR) data|=STATUS_NO_ERROR;
			} else {
				data=0x9c;

#if 0
				/* set status 'out of paper', '*no* error', 'IRQ has *not* occured' */
				LPT[n].status = 0x09c;	//0x2c;

#endif
			}
			data|=0x4; //?

			LPT_LOG(1,"LPT_status_r",("%d $%02x\n", n, data));
			break;
		case 2:
//			This->control = 0x0c; //?
			data = This->control;
			LPT_LOG(1,"LPT_control_r",("%d $%02x\n", n, data));
			break;
    }
	return data;
}

void pc_lpt_handshake_in(running_machine *machine,int nr, int data, int mask)
{
	PC_LPT *This=LPT+nr;
	int neu=(data&mask)|(This->status&~mask);

	if (!(This->control&CONTROL_IRQ) &&
		!(This->status&STATUS_ACKNOWLEDGE)&&
		(neu&CENTRONICS_ACKNOWLEDGE)) {
		if (This->config->irq)
			This->config->irq(nr, PULSE_LINE);
	}
	This->status=neu;
}

READ8_HANDLER ( pc_parallelport0_r) { return pc_LPT_r(space->machine, 0, offset); }
READ8_HANDLER ( pc_parallelport1_r) { return pc_LPT_r(space->machine, 1, offset); }
READ8_HANDLER ( pc_parallelport2_r) { return pc_LPT_r(space->machine, 2, offset); }
WRITE8_HANDLER ( pc_parallelport0_w ) { pc_LPT_w(space->machine, 0, offset, data); }
WRITE8_HANDLER ( pc_parallelport1_w ) { pc_LPT_w(space->machine, 1, offset, data); }
WRITE8_HANDLER ( pc_parallelport2_w ) { pc_LPT_w(space->machine, 2, offset, data); }

READ16_HANDLER ( pc16le_parallelport0_r ) { return read16le_with_read8_handler(pc_parallelport0_r, space, offset, mem_mask); }
READ16_HANDLER ( pc16le_parallelport1_r ) { return read16le_with_read8_handler(pc_parallelport1_r, space, offset, mem_mask); }
READ16_HANDLER ( pc16le_parallelport2_r ) { return read16le_with_read8_handler(pc_parallelport2_r, space, offset, mem_mask); }
WRITE16_HANDLER( pc16le_parallelport0_w ) { write16le_with_write8_handler(pc_parallelport0_w, space, offset, data, mem_mask); }
WRITE16_HANDLER( pc16le_parallelport1_w ) { write16le_with_write8_handler(pc_parallelport1_w, space, offset, data, mem_mask); }
WRITE16_HANDLER( pc16le_parallelport2_w ) { write16le_with_write8_handler(pc_parallelport2_w, space, offset, data, mem_mask); }

READ32_HANDLER ( pc32le_parallelport0_r ) { return read32le_with_read8_handler(pc_parallelport0_r, space, offset, mem_mask); }
READ32_HANDLER ( pc32le_parallelport1_r ) { return read32le_with_read8_handler(pc_parallelport1_r, space, offset, mem_mask); }
READ32_HANDLER ( pc32le_parallelport2_r ) { return read32le_with_read8_handler(pc_parallelport2_r, space, offset, mem_mask); }
WRITE32_HANDLER( pc32le_parallelport0_w ) { write32le_with_write8_handler(pc_parallelport0_w, space, offset, data, mem_mask); }
WRITE32_HANDLER( pc32le_parallelport1_w ) { write32le_with_write8_handler(pc_parallelport1_w, space, offset, data, mem_mask); }
WRITE32_HANDLER( pc32le_parallelport2_w ) { write32le_with_write8_handler(pc_parallelport2_w, space, offset, data, mem_mask); }
