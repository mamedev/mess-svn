/***************************************************************************

    machine/pc.c

    Functions to emulate general aspects of the machine
    (RAM, ROM, interrupts, I/O ports)

    The information herein is heavily based on
    'Ralph Browns Interrupt List'
    Release 52, Last Change 20oct96

***************************************************************************/

#include "emu.h"
#include "includes/pc.h"

#include "machine/i8255a.h"
#include "machine/ins8250.h"
#include "machine/mc146818.h"
#include "machine/pic8259.h"
#include "machine/pc_turbo.h"

#include "video/pc_video_mess.h"
#include "video/pc_vga.h"
#include "video/pc_cga.h"
#include "video/pc_aga.h"
#include "video/pc_mda.h"
#include "video/pc_t1t.h"

#include "machine/pit8253.h"

#include "includes/pc_mouse.h"
#include "machine/pckeybrd.h"
#include "machine/pc_lpt.h"
#include "machine/pc_fdc.h"
#include "machine/pc_hdc.h"
#include "machine/upd765.h"
#include "includes/amstr_pc.h"
#include "includes/europc.h"
#include "includes/ibmpc.h"
#include "machine/pcshare.h"
#include "imagedev/cassette.h"
#include "sound/speaker.h"

#include "machine/8237dma.h"

#include "machine/kb_keytro.h"
#include "machine/ram.h"

#define VERBOSE_PIO 0	/* PIO (keyboard controller) */

#define PIO_LOG(N,M,A) \
	do { \
		if(VERBOSE_PIO>=N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s",machine->time().as_double(),(char*)M ); \
			logerror A; \
		} \
	} while (0)

/*************************************************************************
 *
 *      PC DMA stuff
 *
 *************************************************************************/

READ8_HANDLER(pc_page_r)
{
	return 0xFF;
}


WRITE8_HANDLER(pc_page_w)
{
	pc_state *st = space->machine->driver_data<pc_state>();
	switch(offset % 4)
	{
	case 1:
		st->dma_offset[0][2] = data;
		break;
	case 2:
		st->dma_offset[0][3] = data;
		break;
	case 3:
		st->dma_offset[0][0] = st->dma_offset[0][1] = data;
		break;
	}
}


static WRITE_LINE_DEVICE_HANDLER( pc_dma_hrq_changed )
{
	pc_state *st = device->machine->driver_data<pc_state>();
	cpu_set_input_line(st->maincpu, INPUT_LINE_HALT, state ? ASSERT_LINE : CLEAR_LINE);

	/* Assert HLDA */
	i8237_hlda_w( device, state );
}


static READ8_HANDLER( pc_dma_read_byte )
{
	UINT8 result;
	pc_state *st = space->machine->driver_data<pc_state>();
	offs_t page_offset = (((offs_t) st->dma_offset[0][st->dma_channel]) << 16)
		& 0x0F0000;

	result = space->read_byte( page_offset + offset);
	return result;
}


static WRITE8_HANDLER( pc_dma_write_byte )
{
	pc_state *st = space->machine->driver_data<pc_state>();
	offs_t page_offset = (((offs_t) st->dma_offset[0][st->dma_channel]) << 16)
		& 0x0F0000;

	space->write_byte( page_offset + offset, data);
}


static READ8_DEVICE_HANDLER( pc_dma8237_fdc_dack_r )
{
	return pc_fdc_dack_r(device->machine);
}


static READ8_DEVICE_HANDLER( pc_dma8237_hdc_dack_r )
{
	return pc_hdc_dack_r( device->machine );
}


static WRITE8_DEVICE_HANDLER( pc_dma8237_fdc_dack_w )
{
	pc_fdc_dack_w( device->machine, data );
}


static WRITE8_DEVICE_HANDLER( pc_dma8237_hdc_dack_w )
{
	pc_hdc_dack_w( device->machine, data );
}


static WRITE8_DEVICE_HANDLER( pc_dma8237_0_dack_w )
{
	pc_state *st = device->machine->driver_data<pc_state>();
	st->u73_q2 = 0;
	i8237_dreq0_w( st->dma8237, st->u73_q2 );
}


static WRITE_LINE_DEVICE_HANDLER( pc_dma8237_out_eop )
{
	pc_fdc_set_tc_state( device->machine, state == ASSERT_LINE ? 0 : 1 );
}

static void set_dma_channel(device_t *device, int channel, int state)
{
	pc_state *st = device->machine->driver_data<pc_state>();

	if (!state) st->dma_channel = channel;
}

static WRITE_LINE_DEVICE_HANDLER( pc_dack0_w ) { set_dma_channel(device, 0, state); }
static WRITE_LINE_DEVICE_HANDLER( pc_dack1_w ) { set_dma_channel(device, 1, state); }
static WRITE_LINE_DEVICE_HANDLER( pc_dack2_w ) { set_dma_channel(device, 2, state); }
static WRITE_LINE_DEVICE_HANDLER( pc_dack3_w ) { set_dma_channel(device, 3, state); }

I8237_INTERFACE( ibm5150_dma8237_config )
{
	DEVCB_LINE(pc_dma_hrq_changed),
	DEVCB_LINE(pc_dma8237_out_eop),
	DEVCB_MEMORY_HANDLER("maincpu", PROGRAM, pc_dma_read_byte),
	DEVCB_MEMORY_HANDLER("maincpu", PROGRAM, pc_dma_write_byte),
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_HANDLER(pc_dma8237_fdc_dack_r), DEVCB_HANDLER(pc_dma8237_hdc_dack_r) },
	{ DEVCB_HANDLER(pc_dma8237_0_dack_w), DEVCB_NULL, DEVCB_HANDLER(pc_dma8237_fdc_dack_w), DEVCB_HANDLER(pc_dma8237_hdc_dack_w) },
	{ DEVCB_LINE(pc_dack0_w), DEVCB_LINE(pc_dack1_w), DEVCB_LINE(pc_dack2_w), DEVCB_LINE(pc_dack3_w) }
};


/*************************************************************
 *
 * pic8259 configuration
 *
 *************************************************************/

const struct pic8259_interface ibm5150_pic8259_config =
{
	DEVCB_CPU_INPUT_LINE("maincpu", 0)
};


/*************************************************************
 *
 * PCJR pic8259 configuration
 *
 * Part of the PCJR CRT POST test at address F0452/F0454 writes
 * to the PIC enabling an IRQ which is then immediately fired,
 * however it is expected that the actual IRQ is taken one
 * instruction later (the irq bit is reset by the instruction
 * at F0454). Delaying taking of an IRQ by one instruction for
 * all cases breaks floppy emulation. This seems to be a really
 * tight corner case. For now we delay the IRQ by one instruction
 * only for the PCJR and only when it's inside the POST checks.
 *
 *************************************************************/

static emu_timer	*pc_int_delay_timer;

static TIMER_CALLBACK( pcjr_delayed_pic8259_irq )
{
    cpu_set_input_line(machine->firstcpu, 0, param ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_LINE_DEVICE_HANDLER( pcjr_pic8259_set_int_line )
{
	if ( cpu_get_reg( device->machine->firstcpu, STATE_GENPC ) == 0xF0454 )
	{
		pc_int_delay_timer->adjust( device->machine->firstcpu->cycles_to_attotime(1), state );
	}
	else
	{
		cpu_set_input_line(device->machine->firstcpu, 0, state ? ASSERT_LINE : CLEAR_LINE);
	}
}

const struct pic8259_interface pcjr_pic8259_config =
{
	DEVCB_LINE(pcjr_pic8259_set_int_line)
};


/*************************************************************************
 *
 *      PC Speaker related
 *
 *************************************************************************/
UINT8 pc_speaker_get_spk(running_machine *machine)
{
	pc_state *st = machine->driver_data<pc_state>();
	return st->pc_spkrdata & st->pc_input;
}


void pc_speaker_set_spkrdata(running_machine *machine, UINT8 data)
{
	device_t *speaker = machine->device("speaker");
	pc_state *st = machine->driver_data<pc_state>();
	st->pc_spkrdata = data ? 1 : 0;
	speaker_level_w( speaker, pc_speaker_get_spk(machine) );
}


void pc_speaker_set_input(running_machine *machine, UINT8 data)
{
	device_t *speaker = machine->device("speaker");
	pc_state *st = machine->driver_data<pc_state>();
	st->pc_input = data ? 1 : 0;
	speaker_level_w( speaker, pc_speaker_get_spk(machine) );
}


/*************************************************************
 *
 * pit8253 configuration
 *
 *************************************************************/

static WRITE_LINE_DEVICE_HANDLER( ibm5150_pit8253_out1_changed )
{
	pc_state *st = device->machine->driver_data<pc_state>();
	/* Trigger DMA channel #0 */
	if ( st->out1 == 0 && state == 1 && st->u73_q2 == 0 )
	{
		st->u73_q2 = 1;
		i8237_dreq0_w( st->dma8237, st->u73_q2 );
	}
	st->out1 = state;
}


static WRITE_LINE_DEVICE_HANDLER( ibm5150_pit8253_out2_changed )
{
	pc_speaker_set_input( device->machine, state );
}


const struct pit8253_config ibm5150_pit8253_config =
{
	{
		{
			XTAL_14_31818MHz/12,				/* heartbeat IRQ */
			DEVCB_NULL,
			DEVCB_DEVICE_LINE("pic8259", pic8259_ir0_w)
		}, {
			XTAL_14_31818MHz/12,				/* dram refresh */
			DEVCB_NULL,
			DEVCB_LINE(ibm5150_pit8253_out1_changed)
		}, {
			XTAL_14_31818MHz/12,				/* pio port c pin 4, and speaker polling enough */
			DEVCB_NULL,
			DEVCB_LINE(ibm5150_pit8253_out2_changed)
		}
	}
};


/*
  On the PC Jr the input for clock 1 seems to be selectable
  based on bit 4(/5?) written to output port A0h. This is not
  supported yet.
 */

const struct pit8253_config pcjr_pit8253_config =
{
	{
		{
			XTAL_14_31818MHz/12,              /* heartbeat IRQ */
			DEVCB_NULL,
			DEVCB_DEVICE_LINE("pic8259", pic8259_ir0_w)
		}, {
			XTAL_14_31818MHz/12,              /* dram refresh */
			DEVCB_NULL,
			DEVCB_NULL
		}, {
			XTAL_14_31818MHz/12,              /* pio port c pin 4, and speaker polling enough */
			DEVCB_NULL,
			DEVCB_LINE(ibm5150_pit8253_out2_changed)
		}
	}
};

/**********************************************************
 *
 * COM hardware
 *
 **********************************************************/

/* called when a interrupt is set/cleared from com hardware */
static INS8250_INTERRUPT( pc_com_interrupt_1 )
{
	pc_state *st = device->machine->driver_data<pc_state>();
	pic8259_ir4_w(st->pic8259, state);
}

static INS8250_INTERRUPT( pc_com_interrupt_2 )
{
	pc_state *st = device->machine->driver_data<pc_state>();
	pic8259_ir3_w(st->pic8259, state);
}

/* called when com registers read/written - used to update peripherals that
are connected */
static void pc_com_refresh_connected_common(device_t *device, int n, int data)
{
	/* mouse connected to this port? */
	if (input_port_read(device->machine, "DSW2") & (0x80>>n))
		pc_mouse_handshake_in(device,data);
}

static INS8250_HANDSHAKE_OUT( pc_com_handshake_out_0 ) { pc_com_refresh_connected_common( device, 0, data ); }
static INS8250_HANDSHAKE_OUT( pc_com_handshake_out_1 ) { pc_com_refresh_connected_common( device, 1, data ); }
static INS8250_HANDSHAKE_OUT( pc_com_handshake_out_2 ) { pc_com_refresh_connected_common( device, 2, data ); }
static INS8250_HANDSHAKE_OUT( pc_com_handshake_out_3 ) { pc_com_refresh_connected_common( device, 3, data ); }

/* PC interface to PC-com hardware. Done this way because PCW16 also
uses PC-com hardware and doesn't have the same setup! */
const ins8250_interface ibm5150_com_interface[4]=
{
	{
		1843200,
		pc_com_interrupt_1,
		NULL,
		pc_com_handshake_out_0,
		NULL
	},
	{
		1843200,
		pc_com_interrupt_2,
		NULL,
		pc_com_handshake_out_1,
		NULL
	},
	{
		1843200,
		pc_com_interrupt_1,
		NULL,
		pc_com_handshake_out_2,
		NULL
	},
	{
		1843200,
		pc_com_interrupt_2,
		NULL,
		pc_com_handshake_out_3,
		NULL
	}
};


/**********************************************************
 *
 * NMI handling
 *
 **********************************************************/

static UINT8	nmi_enabled;

WRITE8_HANDLER( pc_nmi_enable_w )
{
	logerror( "%08X: changing NMI state to %s\n", cpu_get_pc(space->cpu), data & 0x80 ? "enabled" : "disabled" );

	nmi_enabled = data & 0x80;
}

/*************************************************************
 *
 * PCJR NMI and raw keybaord handling
 *
 * raw signals on the keyboard cable:
 * ---_-b0b1b2b3b4b5b6b7pa----------------------
 *    | | | | | | | | | | |
 *    | | | | | | | | | | *--- 11 stop bits ( -- = 1 stop bit )
 *    | | | | | | | | | *----- parity bit ( 0 = _-, 1 = -_ )
 *    | | | | | | | | *------- bit 7 ( 0 = _-, 1 = -_ )
 *    | | | | | | | *--------- bit 6 ( 0 = _-, 1 = -_ )
 *    | | | | | | *----------- bit 5 ( 0 = _-, 1 = -_ )
 *    | | | | | *------------- bit 4 ( 0 = _-, 1 = -_ )
 *    | | | | *--------------- bit 3 ( 0 = _-, 1 = -_ )
 *    | | | *----------------- bit 2 ( 0 = _-, 1 = -_ )
 *    | | *------------------- bit 1 ( 0 = _-, 1 = -_ )
 *    | *--------------------- bit 0 ( 0 = _-, 1 = -_ )
 *    *----------------------- start bit (always _- )
 *
 * An entire bit lasts for 440 uSec, half bit time is 220 uSec.
 * Transferring an entire byte takes 21 x 440uSec. The extra
 * time of the stop bits is to allow the CPU to do other things
 * besides decoding keyboard signals.
 *
 * These signals get inverted before going to the PCJR
 * handling hardware. The sequence for the start then
 * becomes:
 *
 * __-_b0b1.....
 *   |
 *   *---- on the 0->1 transition of the start bit a keyboard
 *         latch signal is set to 1 and an NMI is generated
 *         when enabled.
 *         The keyboard latch is reset by reading from the
 *         NMI enable port (A0h).
 *
 *************************************************************/

static struct {
	UINT8		transferring;
	UINT8		latch;
	UINT32		raw_keyb_data;
	int			signal_count;
	emu_timer	*keyb_signal_timer;
} pcjr_keyb;


READ8_HANDLER( pcjr_nmi_enable_r )
{
	pcjr_keyb.latch = 0;

	return nmi_enabled;
}


static TIMER_CALLBACK( pcjr_keyb_signal_callback )
{
	pcjr_keyb.raw_keyb_data = pcjr_keyb.raw_keyb_data >> 1;
	pcjr_keyb.signal_count--;

	if ( pcjr_keyb.signal_count <= 0 )
	{
		pcjr_keyb.keyb_signal_timer->adjust( attotime::never, 0, attotime::never );
		pcjr_keyb.transferring = 0;
	}
}


static void pcjr_set_keyb_int(running_machine *machine, int state)
{
	pc_state *st = machine->driver_data<pc_state>();
	if ( state )
	{
		UINT8	data = pc_keyb_read();
		UINT8	parity = 0;
		int		i;

		/* Calculate the raw data */
		for( i = 0; i < 8; i++ )
		{
			if ( ( 1 << i ) & data )
			{
				parity ^= 1;
			}
		}
		pcjr_keyb.raw_keyb_data = 0;
		pcjr_keyb.raw_keyb_data = ( pcjr_keyb.raw_keyb_data << 2 ) | ( parity ? 1 : 2 );
		for( i = 0; i < 8; i++ )
		{
			pcjr_keyb.raw_keyb_data = ( pcjr_keyb.raw_keyb_data << 2 ) | ( ( data & 0x80 ) ? 1 : 2 );
			data <<= 1;
		}
		/* Insert start bit */
		pcjr_keyb.raw_keyb_data = ( pcjr_keyb.raw_keyb_data << 2 ) | 1;
		pcjr_keyb.signal_count = 20 + 22;

		/* we are now transferring a byte of keyboard data */
		pcjr_keyb.transferring = 1;

		/* Set timer */
		pcjr_keyb.keyb_signal_timer->adjust( attotime::from_usec(220), 0, attotime::from_usec(220) );

		/* Trigger NMI */
		if ( ! pcjr_keyb.latch )
		{
			pcjr_keyb.latch = 1;
			if ( nmi_enabled & 0x80 )
			{
				cpu_set_input_line( st->pit8253->machine->firstcpu, INPUT_LINE_NMI, PULSE_LINE );
			}
		}
	}
}


static void pcjr_keyb_init(running_machine *machine)
{
	pcjr_keyb.transferring = 0;
	pcjr_keyb.latch = 0;
	pcjr_keyb.raw_keyb_data = 0;
	pc_keyb_set_clock( 1 );
}



/**********************************************************
 *
 * PPI8255 interface
 *
 *
 * PORT A (input)
 *
 * Directly attached to shift register which stores data
 * received from the keyboard.
 *
 * PORT B (output)
 * 0 - PB0 - TIM2GATESPK - Enable/disable counting on timer 2 of the 8253
 * 1 - PB1 - SPKRDATA    - Speaker data
 * 2 - PB2 -             - Enable receiving data from the keyboard when keyboard is not locked.
 * 3 - PB3 -             - Dipsswitch set selector
 * 4 - PB4 - ENBRAMPCK   - Enable ram parity check
 * 5 - PB5 - ENABLEI/OCK - Enable expansion I/O check
 * 6 - PB6 -             - Connected to keyboard clock signal
 *                         0 = ignore keyboard signals
 *                         1 = accept keyboard signals
 * 7 - PB7 -             - Clear/disable shift register and IRQ1 line
 *                         0 = normal operation
 *                         1 = clear and disable shift register and clear IRQ1 flip flop
 *
 * PORT C
 * 0 - PC0 -         - Dipswitch 0/4 SW1
 * 1 - PC1 -         - Dipswitch 1/5 SW1
 * 2 - PC2 -         - Dipswitch 2/6 SW1
 * 3 - PC3 -         - Dipswitch 3/7 SW1
 * 4 - PC4 - SPK     - Speaker/cassette data
 * 5 - PC5 - I/OCHCK - Expansion I/O check result
 * 6 - PC6 - T/C2OUT - Output of 8253 timer 2
 * 7 - PC7 - PCK     - Parity check result
 *
 * IBM5150 SW1:
 * 0   - OFF - One or more floppy drives
 *       ON  - Diskless operation
 * 1   - OFF - 8087 present
 *       ON  - No 8087 present
 * 2+3 - Used to determine on board memory configuration
 *       OFF OFF - 64KB
 *       ON  OFF - 48KB
 *       OFF ON  - 32KB
 *       ON  ON  - 16KB
 * 4+5 - Used to select display
 *       OFF OFF - Monochrome
 *       ON  OFF - CGA, 80 column
 *       OFF ON  - CGA, 40 column
 *       ON  ON  - EGA/VGA display
 * 6+7 - Used to select number of disk drives
 *       OFF OFF - four disk drives
 *       ON  OFF - three disk drives
 *       OFF ON  - two disk drives
 *       ON  ON  - one disk drive
 *
 **********************************************************/

static READ8_DEVICE_HANDLER (ibm5150_ppi_porta_r)
{
	int data = 0xFF;
	running_machine *machine = device->machine;
	pc_state *st = device->machine->driver_data<pc_state>();

	/* KB port A */
	if (st->ppi_keyboard_clear)
	{
		/*   0  0 - no floppy drives
         *   1  Not used
         * 2-3  The number of memory banks on the system board
         * 4-5  Display mode
         *      11 = monochrome
         *      10 - color 80x25
         *      01 - color 40x25
         * 6-7  The number of floppy disk drives
         */
		data = input_port_read(device->machine, "DSW0") & 0xF3;
		switch ( ram_get_size(machine->device(RAM_TAG)) )
		{
		case 16 * 1024:
			data |= 0x00;
			break;
		case 32 * 1024:	/* Need to verify if this is correct */
			data |= 0x04;
			break;
		case 48 * 1024:	/* Need to verify if this is correct */
			data |= 0x08;
			break;
		default:
			data |= 0x0C;
			break;
		}
	}
	else
	{
		data = st->ppi_shift_register;
	}
    PIO_LOG(1,"PIO_A_r",("$%02x\n", data));
    return data;
}


static READ8_DEVICE_HANDLER (ibm5150_ppi_portb_r )
{
	int data;
	running_machine *machine = device->machine;

	data = 0xff;
	PIO_LOG(1,"PIO_B_r",("$%02x\n", data));
	return data;
}


static READ8_DEVICE_HANDLER ( ibm5150_ppi_portc_r )
{
	pc_state *st = device->machine->driver_data<pc_state>();
	int timer2_output = pit8253_get_output( st->pit8253, 2 );
	int data=0xff;
	running_machine *machine = device->machine;

	data&=~0x80; // no parity error
	data&=~0x40; // no error on expansion board
	/* KB port C: equipment flags */
	if (st->ppi_portc_switch_high)
	{
		/* read hi nibble of SW2 */
		data = data & 0xf0;

		switch ( ram_get_size(machine->device(RAM_TAG)) - 64 * 1024 )
		{
		case 64 * 1024:		data |= 0x00; break;
		case 128 * 1024:	data |= 0x02; break;
		case 192 * 1024:	data |= 0x04; break;
		case 256 * 1024:	data |= 0x06; break;
		case 320 * 1024:	data |= 0x08; break;
		case 384 * 1024:	data |= 0x0A; break;
		case 448 * 1024:	data |= 0x0C; break;
		case 512 * 1024:	data |= 0x0E; break;
		case 576 * 1024:	data |= 0x01; break;
		case 640 * 1024:	data |= 0x03; break;
		case 704 * 1024:	data |= 0x05; break;
		case 768 * 1024:	data |= 0x07; break;
		case 832 * 1024:	data |= 0x09; break;
		case 896 * 1024:	data |= 0x0B; break;
		case 960 * 1024:	data |= 0x0D; break;
		}
		if ( ram_get_size(machine->device(RAM_TAG)) > 960 * 1024 )
			data |= 0x0D;

		PIO_LOG(1,"PIO_C_r (hi)",("$%02x\n", data));
	}
	else
	{
		/* read lo nibble of S2 */
		data = (data & 0xf0) | (input_port_read(device->machine, "DSW0") & 0x0f);
		PIO_LOG(1,"PIO_C_r (lo)",("$%02x\n", data));
	}

	if ( ! ( st->ppi_portb & 0x08 ) )
	{
		double tap_val = cassette_input( device->machine->device("cassette") );

		if ( tap_val < 0 )
		{
			data &= ~0x10;
		}
		else
		{
			data |= 0x10;
		}
	}
	else
	{
		if ( st->ppi_portb & 0x01 )
		{
			data = ( data & ~0x10 ) | ( timer2_output ? 0x10 : 0x00 );
		}
	}
	data = ( data & ~0x20 ) | ( timer2_output ? 0x20 : 0x00 );

	return data;
}


static WRITE8_DEVICE_HANDLER ( ibm5150_ppi_porta_w )
{
	running_machine *machine = device->machine;

	/* KB controller port A */
	PIO_LOG(1,"PIO_A_w",("$%02x\n", data));
}


static WRITE8_DEVICE_HANDLER ( ibm5150_ppi_portb_w )
{
	pc_state *st = device->machine->driver_data<pc_state>();
	device_t *keyboard = device->machine->device("keyboard");

	/* KB controller port B */
	st->ppi_portb = data;
	st->ppi_portc_switch_high = data & 0x08;
	st->ppi_keyboard_clear = data & 0x80;
	st->ppi_keyb_clock = data & 0x40;
	pit8253_gate2_w(st->pit8253, BIT(data, 0));
	pc_speaker_set_spkrdata( device->machine, data & 0x02 );

	cassette_change_state( device->machine->device("cassette"), ( data & 0x08 ) ? CASSETTE_MOTOR_DISABLED : CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);

	st->ppi_clock_signal = ( st->ppi_keyb_clock ) ? 1 : 0;
	kb_keytronic_clock_w(keyboard, st->ppi_clock_signal);

	/* If PB7 is set clear the shift register and reset the IRQ line */
	if ( st->ppi_keyboard_clear )
	{
		pic8259_ir1_w(st->pic8259, 0);
		st->ppi_shift_register = 0;
		st->ppi_shift_enable = 1;
	}
}


static WRITE8_DEVICE_HANDLER ( ibm5150_ppi_portc_w )
{
	running_machine *machine = device->machine;

	/* KB controller port C */
	PIO_LOG(1,"PIO_C_w",("$%02x\n", data));
}


WRITE8_HANDLER( ibm5150_kb_set_clock_signal )
{
	pc_state *st = space->machine->driver_data<pc_state>();
	device_t *keyboard = space->machine->device("keyboard");

	if ( st->ppi_clock_signal != data )
	{
		if ( st->ppi_keyb_clock && st->ppi_shift_enable )
		{
			st->ppi_clock_signal = data;
			if ( ! st->ppi_keyboard_clear )
			{
				/* Data is clocked in on a high->low transition */
				if ( ! data )
				{
					UINT8	trigger_irq = st->ppi_shift_register & 0x01;

					st->ppi_shift_register = ( st->ppi_shift_register >> 1 ) | ( st->ppi_data_signal << 7 );
					if ( trigger_irq )
					{
						pic8259_ir1_w(st->pic8259, 1);
						st->ppi_shift_enable = 0;
						st->ppi_clock_signal = 0;
						kb_keytronic_clock_w(keyboard, st->ppi_clock_signal);
					}
				}
			}
		}
	}

	kb_keytronic_clock_w(keyboard, st->ppi_clock_signal);
}


WRITE8_HANDLER( ibm5150_kb_set_data_signal )
{
	pc_state *st = space->machine->driver_data<pc_state>();
	device_t *keyboard = space->machine->device("keyboard");

	st->ppi_data_signal = data;

	kb_keytronic_data_w(keyboard, st->ppi_data_signal);
}


/* IBM PC has a 8255 which is connected to keyboard and other
status information */
I8255A_INTERFACE( ibm5150_ppi8255_interface )
{
	DEVCB_HANDLER(ibm5150_ppi_porta_r),
	DEVCB_HANDLER(ibm5150_ppi_portb_r),
	DEVCB_HANDLER(ibm5150_ppi_portc_r),
	DEVCB_HANDLER(ibm5150_ppi_porta_w),
	DEVCB_HANDLER(ibm5150_ppi_portb_w),
	DEVCB_HANDLER(ibm5150_ppi_portc_w)
};


static READ8_DEVICE_HANDLER (ibm5160_ppi_porta_r)
{
	int data = 0xFF;
	running_machine *machine = device->machine;
	pc_state *st = device->machine->driver_data<pc_state>();

	/* KB port A */
	if (st->ppi_keyboard_clear)
	{
		/*   0  0 - no floppy drives
         *   1  Not used
         * 2-3  The number of memory banks on the system board
         * 4-5  Display mode
         *      11 = monochrome
         *      10 - color 80x25
         *      01 - color 40x25
         * 6-7  The number of floppy disk drives
         */
		data = input_port_read(device->machine, "DSW0");
	}
	else
	{
		data = st->ppi_shift_register;
	}
    PIO_LOG(1,"PIO_A_r",("$%02x\n", data));
    return data;
}


static READ8_DEVICE_HANDLER ( ibm5160_ppi_portc_r )
{
	pc_state *st = device->machine->driver_data<pc_state>();
	int timer2_output = pit8253_get_output( st->pit8253, 2 );
	int data=0xff;
	running_machine *machine = device->machine;

	data&=~0x80; // no parity error
	data&=~0x40; // no error on expansion board
	/* KB port C: equipment flags */
//  if (pc_port[0x61] & 0x08)
	if (st->ppi_portc_switch_high)
	{
		/* read hi nibble of S2 */
		data = (data & 0xf0) | ((input_port_read(device->machine, "DSW0") >> 4) & 0x0f);
		PIO_LOG(1,"PIO_C_r (hi)",("$%02x\n", data));
	}
	else
	{
		/* read lo nibble of S2 */
		data = (data & 0xf0) | (input_port_read(device->machine, "DSW0") & 0x0f);
		PIO_LOG(1,"PIO_C_r (lo)",("$%02x\n", data));
	}

	if ( st->ppi_portb & 0x01 )
	{
		data = ( data & ~0x10 ) | ( timer2_output ? 0x10 : 0x00 );
	}
	data = ( data & ~0x20 ) | ( timer2_output ? 0x20 : 0x00 );

	return data;
}


static WRITE8_DEVICE_HANDLER( ibm5160_ppi_portb_w )
{
	pc_state *st = device->machine->driver_data<pc_state>();
	device_t *keyboard = device->machine->device("keyboard");

	/* PPI controller port B*/
	st->ppi_portb = data;
	st->ppi_portc_switch_high = data & 0x08;
	st->ppi_keyboard_clear = data & 0x80;
	st->ppi_keyb_clock = data & 0x40;
	pit8253_gate2_w(st->pit8253, BIT(data, 0));
	pc_speaker_set_spkrdata( device->machine, data & 0x02 );

	st->ppi_clock_signal = ( st->ppi_keyb_clock ) ? 1 : 0;
	kb_keytronic_clock_w(keyboard, st->ppi_clock_signal);

	/* If PB7 is set clear the shift register and reset the IRQ line */
	if ( st->ppi_keyboard_clear )
	{
		pic8259_ir1_w(st->pic8259, 0);
		st->ppi_shift_register = 0;
		st->ppi_shift_enable = 1;
	}
}


I8255A_INTERFACE( ibm5160_ppi8255_interface )
{
	DEVCB_HANDLER(ibm5160_ppi_porta_r),
	DEVCB_HANDLER(ibm5150_ppi_portb_r),
	DEVCB_HANDLER(ibm5160_ppi_portc_r),
	DEVCB_HANDLER(ibm5150_ppi_porta_w),
	DEVCB_HANDLER(ibm5160_ppi_portb_w),
	DEVCB_HANDLER(ibm5150_ppi_portc_w)
};


static READ8_DEVICE_HANDLER (pc_ppi_porta_r)
{
	int data = 0xFF;
	running_machine *machine = device->machine;
	pc_state *st = device->machine->driver_data<pc_state>();

	/* KB port A */
	if (st->ppi_keyboard_clear)
	{
		/*   0  0 - no floppy drives
         *   1  Not used
         * 2-3  The number of memory banks on the system board
         * 4-5  Display mode
         *      11 = monochrome
         *      10 - color 80x25
         *      01 - color 40x25
         * 6-7  The number of floppy disk drives
         */
		data = input_port_read(device->machine, "DSW0");
	}
	else
	{
		data = pc_keyb_read();
	}
	PIO_LOG(1,"PIO_A_r",("$%02x\n", data));
	return data;
}


static WRITE8_DEVICE_HANDLER( pc_ppi_portb_w )
{
	pc_state *st = device->machine->driver_data<pc_state>();
	/* PPI controller port B*/
	st->ppi_portb = data;
	st->ppi_portc_switch_high = data & 0x08;
	st->ppi_keyboard_clear = data & 0x80;
	st->ppi_keyb_clock = data & 0x40;
	pit8253_gate2_w(st->pit8253, BIT(data, 0));
	pc_speaker_set_spkrdata( device->machine, data & 0x02 );
	pc_keyb_set_clock( st->ppi_keyb_clock );

	if ( st->ppi_keyboard_clear )
		pc_keyb_clear();
}


I8255A_INTERFACE( pc_ppi8255_interface )
{
	DEVCB_HANDLER(pc_ppi_porta_r),
	DEVCB_HANDLER(ibm5150_ppi_portb_r),
	DEVCB_HANDLER(ibm5160_ppi_portc_r),
	DEVCB_HANDLER(ibm5150_ppi_porta_w),
	DEVCB_HANDLER(pc_ppi_portb_w),
	DEVCB_HANDLER(ibm5150_ppi_portc_w)
};


static WRITE8_DEVICE_HANDLER ( pcjr_ppi_portb_w )
{
	pc_state *st = device->machine->driver_data<pc_state>();
	/* KB controller port B */
	st->ppi_portb = data;
	st->ppi_portc_switch_high = data & 0x08;
	pit8253_gate2_w(device->machine->device("pit8253"), BIT(data, 0));
	pc_speaker_set_spkrdata( device->machine, data & 0x02 );

	cassette_change_state( device->machine->device("cassette"), ( data & 0x08 ) ? CASSETTE_MOTOR_DISABLED : CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);
}


/*
 * On a PCJR none of the port A bits are connected.
 */
static READ8_DEVICE_HANDLER (pcjr_ppi_porta_r )
{
	int data;
	running_machine *machine = device->machine;

	data = 0xff;
	PIO_LOG(1,"PIO_A_r",("$%02x\n", data));
	return data;
}


/*
 * Port C connections on a PCJR (notes from schematics):
 * PC0 - KYBD LATCH
 * PC1 - MODEM CD INSTALLED
 * PC2 - DISKETTE CD INSTALLED
 * PC3 - ATR CD IN
 * PC4 - cassette audio
 * PC5 - OUT2 from 8253
 * PC6 - KYBD IN
 * PC7 - (keyboard) CABLE CONNECTED
 */
static READ8_DEVICE_HANDLER ( pcjr_ppi_portc_r )
{
	pc_state *st = device->machine->driver_data<pc_state>();
	int timer2_output = pit8253_get_output( device->machine->device("pit8253"), 2 );
	int data=0xff;

	data&=~0x80;
	data &= ~0x04;		/* floppy drive installed */
	if ( ram_get_size(device->machine->device(RAM_TAG)) > 64 * 1024 )	/* more than 64KB ram installed */
		data &= ~0x08;
	data = ( data & ~0x01 ) | ( pcjr_keyb.latch ? 0x01: 0x00 );
	if ( ! ( st->ppi_portb & 0x08 ) )
	{
		double tap_val = cassette_input( device->machine->device("cassette") );

		if ( tap_val < 0 )
		{
			data &= ~0x10;
		}
		else
		{
			data |= 0x10;
		}
	}
	else
	{
		if ( st->ppi_portb & 0x01 )
		{
			data = ( data & ~0x10 ) | ( timer2_output ? 0x10 : 0x00 );
		}
	}
	data = ( data & ~0x20 ) | ( timer2_output ? 0x20 : 0x00 );
	data = ( data & ~0x40 ) | ( ( pcjr_keyb.raw_keyb_data & 0x01 ) ? 0x40 : 0x00 );

	return data;
}


I8255A_INTERFACE( pcjr_ppi8255_interface )
{
	DEVCB_HANDLER(pcjr_ppi_porta_r),
	DEVCB_HANDLER(ibm5150_ppi_portb_r),
	DEVCB_HANDLER(pcjr_ppi_portc_r),
	DEVCB_HANDLER(ibm5150_ppi_porta_w),
	DEVCB_HANDLER(pcjr_ppi_portb_w),
	DEVCB_HANDLER(ibm5150_ppi_portc_w)
};


/**********************************************************
 *
 * NEC uPD765 floppy interface
 *
 **********************************************************/

static void pc_fdc_interrupt(running_machine *machine, int state)
{
	pc_state *st = machine->driver_data<pc_state>();
	if (st->pic8259)
	{
		pic8259_ir6_w(st->pic8259, state);
	}
}

static void pc_fdc_dma_drq(running_machine *machine, int state)
{
	pc_state *st = machine->driver_data<pc_state>();
	i8237_dreq2_w( st->dma8237, state);
}

static device_t * pc_get_device(running_machine *machine )
{
	return machine->device("upd765");
}

static const struct pc_fdc_interface fdc_interface_nc =
{
	pc_fdc_interrupt,
	pc_fdc_dma_drq,
	NULL,
	pc_get_device
};


static const struct pc_fdc_interface pcjr_fdc_interface_nc =
{
	pc_fdc_interrupt,
	NULL,
	NULL,
	pc_get_device
};


static void pc_set_irq_line(running_machine *machine,int irq, int state)
{
	pc_state *st = machine->driver_data<pc_state>();

	switch (irq)
	{
	case 0: pic8259_ir0_w(st->pic8259, state); break;
	case 1: pic8259_ir1_w(st->pic8259, state); break;
	case 2: pic8259_ir2_w(st->pic8259, state); break;
	case 3: pic8259_ir3_w(st->pic8259, state); break;
	case 4: pic8259_ir4_w(st->pic8259, state); break;
	case 5: pic8259_ir5_w(st->pic8259, state); break;
	case 6: pic8259_ir6_w(st->pic8259, state); break;
	case 7: pic8259_ir7_w(st->pic8259, state); break;
	}
}

static void pc_set_keyb_int(running_machine *machine, int state)
{
	pc_set_irq_line( machine, 1, state );
}


/**********************************************************
 *
 * Initialization code
 *
 **********************************************************/

void mess_init_pc_common(running_machine *machine, UINT32 flags, void (*set_keyb_int_func)(running_machine *, int), void (*set_hdc_int_func)(running_machine *,int,int))
{
	if ( set_keyb_int_func != NULL )
		init_pc_common(machine, flags, set_keyb_int_func);

	/* MESS managed RAM */
	if ( ram_get_ptr(machine->device(RAM_TAG)) )
		memory_set_bankptr( machine, "bank10", ram_get_ptr(machine->device(RAM_TAG)) );

	/* FDC/HDC hardware */
	pc_hdc_setup(machine, set_hdc_int_func);

	/* serial mouse */
	pc_mouse_initialise(machine);
}


DRIVER_INIT( ibm5150 )
{
	mess_init_pc_common(machine, PCCOMMON_KEYBOARD_PC, NULL, pc_set_irq_line);
	pc_rtc_init(machine);
}


DRIVER_INIT( pccga )
{
	mess_init_pc_common(machine, PCCOMMON_KEYBOARD_PC, NULL, pc_set_irq_line);
	pc_rtc_init(machine);
}


DRIVER_INIT( bondwell )
{
	mess_init_pc_common(machine, PCCOMMON_KEYBOARD_PC, NULL, pc_set_irq_line);
	pc_turbo_setup(machine, machine->firstcpu, "DSW2", 0x02, 4.77/12, 1);
}

DRIVER_INIT( pcmda )
{
	mess_init_pc_common(machine, PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);
}

DRIVER_INIT( europc )
{
	UINT8 *gfx = &machine->region("gfx1")->base()[0x8000];
	UINT8 *rom = &machine->region("maincpu")->base()[0];
	int i;

    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	/*
      fix century rom bios bug !
      if year <79 month (and not CENTURY) is loaded with 0x20
    */
	if (rom[0xff93e]==0xb6){ // mov dh,
		UINT8 a;
		rom[0xff93e]=0xb5; // mov ch,
		for (i=0xf8000, a=0; i<0xfffff; i++ ) a+=rom[i];
		rom[0xfffff]=256-a;
	}

	mess_init_pc_common(machine, PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);

	europc_rtc_init(machine);
//  europc_rtc_set_time(machine);
}

DRIVER_INIT( t1000hx )
{
	mess_init_pc_common(machine, PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);
	pc_turbo_setup(machine, machine->firstcpu, "DSW2", 0x02, 4.77/12, 1);
}

DRIVER_INIT( pc200 )
{
	address_space *space = cpu_get_address_space( machine->firstcpu, ADDRESS_SPACE_PROGRAM );
	UINT8 *gfx = &machine->region("gfx1")->base()[0x8000];
	int i;

    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	memory_install_read16_handler( space, 0xb0000, 0xbffff, 0, 0, pc200_videoram16le_r );
	memory_install_write16_handler( space, 0xb0000, 0xbffff, 0, 0, pc200_videoram16le_w );
	pc_videoram_size = 0x10000;
	pc_videoram = machine->region("maincpu")->base()+0xb0000;
	mess_init_pc_common(machine, PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);
}

DRIVER_INIT( ppc512 )
{
	address_space *space = cpu_get_address_space( machine->firstcpu, ADDRESS_SPACE_PROGRAM );
	UINT8 *gfx = &machine->region("gfx1")->base()[0x8000];
	int i;

    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	memory_install_read16_handler( space, 0xb0000, 0xbffff, 0, 0, pc200_videoram16le_r );
	memory_install_write16_handler( space, 0xb0000, 0xbffff, 0, 0, pc200_videoram16le_w );
	pc_videoram_size = 0x10000;
	pc_videoram = machine->region("maincpu")->base()+0xb0000;
	mess_init_pc_common(machine, PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);
}
DRIVER_INIT( pc1512 )
{
	address_space *space = cpu_get_address_space( machine->firstcpu, ADDRESS_SPACE_PROGRAM );
	address_space *io_space = cpu_get_address_space( machine->firstcpu, ADDRESS_SPACE_IO );
	UINT8 *gfx = &machine->region("gfx1")->base()[0x8000];
	int i;

    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	memory_install_read_bank( space, 0xb8000, 0xbbfff, 0, 0x0C000, "bank1" );
	memory_install_write16_handler( space, 0xb8000, 0xbbfff, 0, 0x0C000, pc1512_videoram16le_w );

	memory_install_read16_handler( io_space, 0x3d0, 0x3df, 0, 0, pc1512_16le_r );
	memory_install_write16_handler( io_space, 0x3d0, 0x3df, 0, 0, pc1512_16le_w );

	mess_init_pc_common(machine, PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);
}


DRIVER_INIT( pcjr )
{
	mess_init_pc_common(machine, PCCOMMON_KEYBOARD_PC, pcjr_set_keyb_int, pc_set_irq_line);
}

static READ8_HANDLER( input_port_0_r ) { return input_port_read(space->machine, "IN0"); }

static const struct pc_vga_interface vga_interface =
{
	NULL,
	NULL,

	input_port_0_r,

	ADDRESS_SPACE_IO,
	0x0000
};



DRIVER_INIT( pc1640 )
{
	address_space *io_space = cpu_get_address_space( machine->firstcpu, ADDRESS_SPACE_IO );

	memory_install_read16_handler(io_space, 0x278, 0x27b, 0, 0, pc1640_16le_port278_r );
	memory_install_read16_handler(io_space, 0x4278, 0x427b, 0, 0, pc1640_16le_port4278_r );

	mess_init_pc_common(machine, PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);
}

DRIVER_INIT( pc_vga )
{
	mess_init_pc_common(machine, PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);

	pc_vga_init(machine, &vga_interface, NULL);
}

static IRQ_CALLBACK(pc_irq_callback)
{
	pc_state *st = device->machine->driver_data<pc_state>();
	return pic8259_acknowledge( st->pic8259 );
}


MACHINE_START( pc )
{
	pc_state *st = machine->driver_data<pc_state>();

	st->pic8259 = machine->device("pic8259");
	st->dma8237 = machine->device("dma8237");
	st->pit8253 = machine->device("pit8253");
	pc_fdc_init( machine, &fdc_interface_nc );
}


MACHINE_RESET( pc )
{
	device_t *speaker = machine->device("speaker");
	pc_state *st = machine->driver_data<pc_state>();
	st->maincpu = machine->device("maincpu" );
	cpu_set_irq_callback(st->maincpu, pc_irq_callback);

	st->u73_q2 = 0;
	st->out1 = 0;
	st->pc_spkrdata = 0;
	st->pc_input = 0;
	st->dma_channel = 0;
	memset(st->dma_offset,0,sizeof(st->dma_offset));
	st->ppi_portc_switch_high = 0;
	st->ppi_speaker = 0;
	st->ppi_keyboard_clear = 0;
	st->ppi_keyb_clock = 0;
	st->ppi_portb = 0;
	st->ppi_clock_signal = 0;
	st->ppi_data_signal = 0;
	st->ppi_shift_register = 0;
	st->ppi_shift_enable = 0;

	pc_mouse_set_serial_port( machine->device("ins8250_0") );
	pc_hdc_set_dma8237_device( st->dma8237 );
	speaker_level_w( speaker, 0 );
}


MACHINE_START( pcjr )
{
	pc_state *st = machine->driver_data<pc_state>();
	pc_fdc_init( machine, &pcjr_fdc_interface_nc );
	pcjr_keyb.keyb_signal_timer = machine->scheduler().timer_alloc(FUNC(pcjr_keyb_signal_callback));
	pc_int_delay_timer = machine->scheduler().timer_alloc(FUNC(pcjr_delayed_pic8259_irq));
	st->maincpu = machine->device("maincpu" );
	cpu_set_irq_callback(st->maincpu, pc_irq_callback);

	st->pic8259 = machine->device("pic8259");
	st->dma8237 = NULL;
	st->pit8253 = machine->device("pit8253");
}


MACHINE_RESET( pcjr )
{
	device_t *speaker = machine->device("speaker");
	pc_state *st = machine->driver_data<pc_state>();
	st->u73_q2 = 0;
	st->out1 = 0;
	st->pc_spkrdata = 0;
	st->pc_input = 0;
	st->dma_channel = 0;
	memset(st->dma_offset,0,sizeof(st->dma_offset));
	st->ppi_portc_switch_high = 0;
	st->ppi_speaker = 0;
	st->ppi_keyboard_clear = 0;
	st->ppi_keyb_clock = 0;
	st->ppi_portb = 0;
	st->ppi_clock_signal = 0;
	st->ppi_data_signal = 0;
	st->ppi_shift_register = 0;
	st->ppi_shift_enable = 0;
	pc_mouse_set_serial_port( machine->device("ins8250_0") );
	pc_hdc_set_dma8237_device( st->dma8237 );
	speaker_level_w( speaker, 0 );

	pcjr_keyb_init(machine);
}


DEVICE_IMAGE_LOAD( pcjr_cartridge )
{
	UINT32	address;
	UINT32	size;

	address = ( ! strcmp( "cart2", image.device().tag() ) ) ? 0xd0000 : 0xe0000;

	if ( image.software_entry() )
	{
		UINT8 *cart = image.get_software_region( "rom" );

		size = image.get_software_region_length("rom" );

		memcpy( image.device().machine->region("maincpu")->base() + address, cart, size );
	}
	else
	{
		UINT8	header[0x200];

		unsigned size = image.length();

		/* Check for supported image sizes */
		switch( size )
		{
		case 0x2200:
		case 0x4200:
		case 0x8200:
		case 0x10200:
			break;
		default:
			image.seterror(IMAGE_ERROR_UNSUPPORTED, "Invalid rom file size" );
			return IMAGE_INIT_FAIL;
		}

		/* Read and verify the header */
		if ( 512 != image.fread( header, 512 ) )
		{
			image.seterror(IMAGE_ERROR_UNSUPPORTED, "Unable to read header" );
			return IMAGE_INIT_FAIL;
		}

		/* Read the cartridge contents */
		if ( ( size - 0x200 ) != image.fread(image.device().machine->region("maincpu")->base() + address, size - 0x200 ) )
		{
			image.seterror(IMAGE_ERROR_UNSUPPORTED, "Unable to read cartridge contents" );
			return IMAGE_INIT_FAIL;
		}
	}

	return IMAGE_INIT_PASS;
}


/**************************************************************************
 *
 *      Interrupt handlers.
 *
 **************************************************************************/

INTERRUPT_GEN( pc_frame_interrupt )
{
	pc_keyboard();
}

INTERRUPT_GEN( pc_vga_frame_interrupt )
{
	//vga_timer();
	pc_keyboard();
}

INTERRUPT_GEN( pcjr_frame_interrupt )
{
	if ( pcjr_keyb.transferring == 0 )
	{
		pc_keyboard();
	}
}

