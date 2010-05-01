/*****************************************************************************
 *
 * includes/nes.h
 *
 * Nintendo Entertainment System (Famicom)
 *
 ****************************************************************************/

#ifndef NES_H_
#define NES_H_


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define NTSC_CLOCK           N2A03_DEFAULTCLOCK     /* 1.789772 MHz */
#define PAL_CLOCK	           (26601712.0/16)        /* 1.662607 MHz */

#define NES_BATTERY_SIZE 0x2000


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

struct nes_input
{
	UINT32 shift;
	UINT32 i0, i1, i2;
};

class nes_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, nes_state(machine)); }

	nes_state(running_machine &machine) { }

	/* input_related - this part has to be cleaned up (e.g. in_2 and in_3 are not really necessary here...) */
	nes_input in_0, in_1, in_2, in_3;

	/* video-related */
	int nes_vram_sprite[8]; /* Used only by mmc5 for now */
	int last_frame_flip;
	double scanlines_per_frame;
	
	/* devices */
	running_device *ppu;
	running_device *sound;
	running_device *cart;

	/* misc region to be allocated at init */
	// variables which don't change at run-time
	UINT8 *rom;
	UINT8 *vrom;
	UINT8 *vram;
	UINT8 *wram;
	UINT8 *ciram; //PPU nametable RAM - external to PPU!
	// Variables which can change
	UINT8 mid_ram_enable;

	/* SRAM-related (we have two elements due to the init order, but it would be better to verify they both are still needed) */
	UINT8 *battery_ram;
	UINT8 battery_data[NES_BATTERY_SIZE];

	/***** NES-cart related *****/

	/* load-time cart variables which remain constant */
	UINT8 trainer;
	UINT8 battery;
	UINT16 prg_chunks;	// a recently dumped multigame cart has 256 chunks of both PRG & CHR!
	UINT16 chr_chunks;
	
	UINT8 format;	// 1 = iNES, 2 = UNIF
	
	/* system variables which don't change at run-time */
	UINT16 mapper;		// for iNES
	const char *board;	// for UNIF
	UINT8 four_screen_vram;
	UINT8 hard_mirroring;
	UINT8 slow_banking;
	UINT8 crc_hack;	// this is needed to detect different boards sharing the same Mappers (shame on .nes format)
	
	
	/***** FDS-floppy related *****/

	UINT8 *fds_data;
	UINT8 fds_sides;
	UINT8 *fds_ram;
	
	/* Variables which can change */
	UINT8 fds_motor_on;
	UINT8 fds_door_closed;
	UINT8 fds_current_side;
	UINT32 fds_head_position;
	UINT8 fds_status0;
	UINT8 fds_read_mode;
	UINT8 fds_write_reg;
};


/*----------- defined in machine/nes.c -----------*/

WRITE8_HANDLER ( nes_IN0_w );
WRITE8_HANDLER ( nes_IN1_w );

extern unsigned char *nes_battery_ram;

/* protos */

DEVICE_IMAGE_LOAD(nes_cart);
DEVICE_START(nes_disk);
DEVICE_IMAGE_LOAD(nes_disk);
DEVICE_IMAGE_UNLOAD(nes_disk);

MACHINE_START( nes );
MACHINE_RESET( nes );

DRIVER_INIT( famicom );

READ8_HANDLER( nes_IN0_r );
READ8_HANDLER( nes_IN1_r );

int nes_ppu_vidaccess( running_device *device, int address, int data );

void nes_partialhash(char *dest, const unsigned char *data, unsigned long length, unsigned int functions);

/*----------- defined in video/nes.c -----------*/

extern int nes_vram_sprite[8];

PALETTE_INIT( nes );
VIDEO_START( nes_ntsc );
VIDEO_START( nes_pal );
VIDEO_UPDATE( nes );


#endif /* NES_H_ */
