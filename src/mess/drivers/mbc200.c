/***************************************************************************
   
        Sanyo MBC-200
		
		Machine MBC-1200 is identicaly but sold outside of Japan

		16 x HM6116P-3 2K x 8 SRAM soldered onboard (so 32k ram)
		4 x HM6116P-3 2K x 8 SRAM socketed (so 8k ram)
		4 x MB83256 32K x 8 socketed (128k ram)
		Dual 5.25" floppys.

		On back side:
			- keyboard DIN connector
			- Centronics printer port			
			- RS-232C 25pin connector
			
        31/10/2011 Skeleton driver.

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/i8255.h"
#include "machine/i8251.h"
#include "video/mc6845.h"
#include "machine/wd17xx.h"
#include "formats/basicdsk.h"
#include "imagedev/flopdrv.h"

class mbc200_state : public driver_device
{
public:
	mbc200_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_6845(*this, "crtc"),
		m_fdc(*this, "fdc"),
		m_floppy0(*this, FLOPPY_0),
		m_floppy1(*this, FLOPPY_1)
	{ }

	virtual void machine_start();
	
	required_device<mc6845_device> m_6845;
	required_device<device_t> m_fdc;
	required_device<device_t> m_floppy0;
	required_device<device_t> m_floppy1;		
};

static ADDRESS_MAP_START(mbc200_sub_mem, AS_PROGRAM, 8, mbc200_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x2fff ) AM_ROM
	AM_RANGE( 0x3000, 0xffff ) AM_RAM 
ADDRESS_MAP_END

static ADDRESS_MAP_START( mbc200_sub_io , AS_IO, 8, mbc200_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x70, 0x73) AM_DEVREADWRITE("ppi8255_1", i8255_device, read, write)
	AM_RANGE(0xb0, 0xb0) AM_DEVWRITE("crtc", mc6845_device, address_w)
	AM_RANGE(0xb1, 0xb1) AM_DEVREADWRITE("crtc", mc6845_device, register_r, register_w)	
	//AM_RANGE(0xd0, 0xd3) AM_DEVREADWRITE("ppi8255_2", i8255_device, read, write)
ADDRESS_MAP_END

static ADDRESS_MAP_START(mbc200_mem, AS_PROGRAM, 8, mbc200_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x0fff ) AM_ROM
	AM_RANGE( 0x1000, 0xffff ) AM_RAM 
ADDRESS_MAP_END

static ADDRESS_MAP_START( mbc200_io , AS_IO, 8, mbc200_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xe0, 0xe0) AM_DEVREADWRITE("i8251_1", i8251_device, data_r, data_w)
	AM_RANGE(0xe1, 0xe1) AM_DEVREADWRITE("i8251_1", i8251_device, status_r, control_w)	
	AM_RANGE(0xe4, 0xe7) AM_DEVREADWRITE_LEGACY("fdc", wd17xx_r, wd17xx_w)
	AM_RANGE(0xe8, 0xeb) AM_DEVREADWRITE("ppi8255_2", i8255_device, read, write)
	AM_RANGE(0xec, 0xec) AM_DEVREADWRITE("i8251_2", i8251_device, data_r, data_w)	
	AM_RANGE(0xed, 0xed) AM_DEVREADWRITE("i8251_2", i8251_device, status_r, control_w)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( mbc200 )
INPUT_PORTS_END

void mbc200_state::machine_start()
{
	floppy_mon_w(m_floppy0, CLEAR_LINE);
	floppy_mon_w(m_floppy1, CLEAR_LINE);
	floppy_drive_set_ready_state(m_floppy0, 1, 1);
	floppy_drive_set_ready_state(m_floppy1, 1, 1);
}

MC6845_UPDATE_ROW( mbc200_update_row )
{
}

static const mc6845_interface mbc200_crtc = {
	"screen",			/* name of screen */
	8,			/* number of dots per character */
	NULL,
	mbc200_update_row,		/* handler to display a scanline */
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};

static VIDEO_START( mbc200 )
{
}

static SCREEN_UPDATE( mbc200 )
{
    return 0;
}

static I8255_INTERFACE( mbc200_ppi8255_interface )
{
	DEVCB_NULL,	/* port A read */
	DEVCB_NULL,	/* port A write */
	DEVCB_NULL,	/* port B read */
	DEVCB_NULL,	/* port B write */
	DEVCB_NULL,	/* port C read */
	DEVCB_NULL	/* port C write */
};

static const wd17xx_interface mbc200_mb8876_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	{FLOPPY_0, FLOPPY_1, NULL, NULL}
};
static const floppy_interface mbc200_floppy_interface =
{
    DEVCB_NULL,
	DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    FLOPPY_STANDARD_5_25_SSDD_40,
    LEGACY_FLOPPY_OPTIONS_NAME(default),
    "floppy_5_25",
	NULL
};

static MACHINE_CONFIG_START( mbc200, mbc200_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",Z80, XTAL_8MHz/2) // NEC D780C-1
    MCFG_CPU_PROGRAM_MAP(mbc200_mem)
    MCFG_CPU_IO_MAP(mbc200_io)

    MCFG_CPU_ADD("subcpu",Z80, XTAL_8MHz/2) // NEC D780C-1
    MCFG_CPU_PROGRAM_MAP(mbc200_sub_mem)
    MCFG_CPU_IO_MAP(mbc200_sub_io)	
	
    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 400)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 400-1)
    MCFG_SCREEN_UPDATE(mbc200)

    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(mbc200)
	
	MCFG_MC6845_ADD("crtc", H46505, XTAL_8MHz / 4, mbc200_crtc) // HD46505SP	
	MCFG_I8255_ADD("ppi8255_1", mbc200_ppi8255_interface) // i8255AC-5
	MCFG_I8255_ADD("ppi8255_2", mbc200_ppi8255_interface) // i8255AC-5
	MCFG_I8251_ADD("i8251_1", default_i8251_interface) // INS8251N	
	MCFG_I8251_ADD("i8251_2", default_i8251_interface) // INS8251A	
	MCFG_MB8876_ADD("fdc",mbc200_mb8876_interface) // MB8876A
	MCFG_LEGACY_FLOPPY_2_DRIVES_ADD(mbc200_floppy_interface)	
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( mbc200 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "d2732a.bin",  0x0000, 0x1000, CRC(bf364ce8) SHA1(baa3a20a5b01745a390ef16628dc18f8d682d63b))
	ROM_REGION( 0x10000, "subcpu", ROMREGION_ERASEFF )
	ROM_LOAD( "m5l2764.bin", 0x0000, 0x2000, CRC(377300a2) SHA1(8563172f9e7f84330378a8d179f4138be5fda099))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1982, mbc200,  0,       0, 	mbc200, 	mbc200, 	 0,	 "Sanyo",   "MBC-200",		GAME_NOT_WORKING | GAME_NO_SOUND)

