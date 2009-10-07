/*
    Corvus Concept driver

    Relatively simple 68k-based system

    * 256 or 512 kbytes of DRAM
    * 4kbytes of SRAM
    * 8kbyte boot ROM
    * optional MacsBugs ROM
    * two serial ports, keyboard, bitmapped display, simple sound, omninet
      LAN port (seems more or less similar to AppleTalk)
    * 4 expansion ports enable to add expansion cards, namely floppy disk
      and hard disk controllers (the expansion ports are partially compatible
      with Apple 2 expansion ports)

    Video: monochrome bitmapped display, 720*560 visible area (bitmaps are 768
      pixels wide in memory).  One interesting feature is the fact that the
      monitor can be rotated to give a 560*720 vertical display (you need to
      throw a switch and reset the machine for the display rotation to be taken
      into account, though).  One oddity is that the video hardware scans the
      display from the lower-left corner to the upper-left corner (or from the
      upper-right corner to the lower-left if the screen is flipped).
    Sound: simpler buzzer connected to the via shift register
    Keyboard: intelligent controller, connected through an ACIA.  See CCOS
      manual pp. 76 through 78. and User Guide p. 2-1 through 2-9.
    Clock: mm58174 RTC

    Raphael Nabet, Brett Wyer, 2003-2005
*/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "includes/concept.h"
#include "devices/flopdrv.h"
#include "formats/basicdsk.h"
#include "devices/harddriv.h"
#include "machine/mm58274c.h"
#include "machine/wd17xx.h"

static ADDRESS_MAP_START(concept_memmap, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x000000, 0x000007) AM_ROM AM_REGION("maincpu", 0x010000) 	/* boot ROM mirror */
	AM_RANGE(0x000008, 0x000fff) AM_RAM										/* static RAM */
	AM_RANGE(0x010000, 0x011fff) AM_ROM AM_REGION("maincpu", 0x010000)	/* boot ROM */
	AM_RANGE(0x020000, 0x021fff) AM_ROM										/* macsbugs ROM (optional) */
	AM_RANGE(0x030000, 0x03ffff) AM_READWRITE(concept_io_r,concept_io_w)	/* I/O space */

	AM_RANGE(0x080000, 0x0fffff) AM_RAM AM_BASE(&videoram16)/* AM_RAMBANK(2) */	/* DRAM */
ADDRESS_MAP_END

/* init with simple, fixed, B/W palette */
/* Is the palette black on white or white on black??? */
static PALETTE_INIT( concept )
{
	palette_set_color_rgb(machine, 0, 0xff, 0xff, 0xff);
	palette_set_color_rgb(machine, 1, 0x00, 0x00, 0x00);
}

static const mm58274c_interface concept_mm58274c_interface =
{
	0,	/*  mode 24*/
	1   /*  first day of week */
};



static FLOPPY_OPTIONS_START(concept)
#if 1
	/* SSSD 8" */
	FLOPPY_OPTION(concept, "img", "Corvus Concept 8\" SSSD disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([77])
		SECTORS([26])
		SECTOR_LENGTH([128])
		FIRST_SECTOR_ID([1]))
#elif 0
	/* SSDD 8" (according to ROMs) */
	FLOPPY_OPTION(concept, "img", "Corvus Concept 8\" SSDD disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([77])
		SECTORS([26])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
#elif 0
	/* Apple II DSDD 5"1/4 (according to ROMs) */
	FLOPPY_OPTION(concept, "img", "Corvus Concept Apple II 5\"1/4 DSDD disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([35])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
#elif 0
	/* actual formats found */
	FLOPPY_OPTION(concept, "img", "Corvus Concept 5\"1/4 DSDD disk image (256-byte sectors)", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
#else
	FLOPPY_OPTION(concept, "img", "Corvus Concept 5\"1/4 DSDD disk image (512-byte sectors)", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([9])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
#endif
FLOPPY_OPTIONS_END

static const floppy_config concept_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(concept),
	DO_NOT_KEEP_GEOMETRY
};

/* concept machine */
static MACHINE_DRIVER_START( concept )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M68000, 8182000)        /* 16.364 MHz / 2 */
	MDRV_CPU_PROGRAM_MAP(concept_memmap)
	MDRV_CPU_VBLANK_INT("screen", concept_interrupt)

	MDRV_QUANTUM_TIME(HZ(60))
	MDRV_MACHINE_START(concept)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)			/* 50 or 60, jumper-selectable */
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(720, 560)
	MDRV_SCREEN_VISIBLE_AREA(0, 720-1, 0, 560-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(concept)

	MDRV_VIDEO_START(concept)
	MDRV_VIDEO_UPDATE(concept)

	/* no sound? */

	MDRV_HARDDISK_ADD( "harddisk1" )

	/* rtc */
	MDRV_MM58274C_ADD("mm58274c", concept_mm58274c_interface)

	/* via */
	MDRV_VIA6522_ADD("via6522_0", 1022750, concept_via6522_intf)

	MDRV_WD179X_ADD("wd179x", concept_wd17xx_interface )

	MDRV_FLOPPY_4_DRIVES_ADD(concept_floppy_config)
MACHINE_DRIVER_END


static INPUT_PORTS_START( concept )

	PORT_START("KEY0")	/* port 0: keys 0x00 through 0x0f */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(right)") PORT_CODE(KEYCODE_RIGHT)
		PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3_PAD)
		PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9_PAD)
		PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("HOME") PORT_CODE(KEYCODE_HOME)
		PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6_PAD)
		PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_PLUS_PAD)
		PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS_PAD)
		PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER_PAD)
		PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(left)") PORT_CODE(KEYCODE_LEFT)
		PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1_PAD)
		PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7_PAD)
		PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(down)") PORT_CODE(KEYCODE_DOWN)
		PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4_PAD)
		PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8_PAD)
		PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5_PAD)
		PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2_PAD)

	PORT_START("KEY1")	/* port 1: keys 0x10 through 0x1f */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("= +") PORT_CODE(KEYCODE_EQUALS)

		PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[ {") PORT_CODE(KEYCODE_OPENBRACE)
		PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("BACKSPACE") PORT_CODE(KEYCODE_BACKSPACE)
		PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER)
		PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("] }") PORT_CODE(KEYCODE_CLOSEBRACE)
		PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\\ |") PORT_CODE(KEYCODE_BACKSLASH)

		PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0 )") PORT_CODE(KEYCODE_0)
		PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH)
		PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
		PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("- _") PORT_CODE(KEYCODE_MINUS)
		PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("; :") PORT_CODE(KEYCODE_COLON)
		PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("` ~") PORT_CODE(KEYCODE_BACKSLASH2)
		PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("' \"") PORT_CODE(KEYCODE_QUOTE)
		PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SHIFT (r)") PORT_CODE(KEYCODE_RSHIFT)

	PORT_START("KEY2")	/* port 2: keys 0x20 through 0x2f */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
		PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
		PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)
		PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)
		PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5)

		PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4 $") PORT_CODE(KEYCODE_4)
		PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5 %") PORT_CODE(KEYCODE_5)
		PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
		PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
		PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
		PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
		PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
		PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)

	PORT_START("KEY3")	/* port 3: keys 0x30 through 0x3f */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2 @") PORT_CODE(KEYCODE_2)
		PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3 #") PORT_CODE(KEYCODE_3)
		PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
		PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
		PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
		PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
		PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
		PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
		PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC)
		PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1 !") PORT_CODE(KEYCODE_1)
		PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB)
		PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
		PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CAPS LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE
		PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
		PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SHIFT (l)") PORT_CODE(KEYCODE_LSHIFT)
		PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)

	PORT_START("KEY4")	/* port 4: keys 0x40 through 0x4f */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6 ^") PORT_CODE(KEYCODE_6)
		PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7 &") PORT_CODE(KEYCODE_7)
		PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
		PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
		PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
		PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
		PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
		PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
		PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CONTROL") PORT_CODE(KEYCODE_LCONTROL)
		PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("FAST") PORT_CODE(KEYCODE_TILDE)
		PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("COMMAND") PORT_CODE(KEYCODE_LALT)
		PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(space)") PORT_CODE(KEYCODE_SPACE)
		PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ALT") PORT_CODE(KEYCODE_RALT)
		PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0_PAD)
		PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("00") PORT_CODE(KEYCODE_ASTERISK)
		PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_DEL_PAD)

	PORT_START("KEY5")	/* port 5: keys 0x50 through 0x5f */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8 *") PORT_CODE(KEYCODE_8)
		PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9 (") PORT_CODE(KEYCODE_9)
		PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
		PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
		PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
		PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA)
		PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP)
		PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F6") PORT_CODE(KEYCODE_F6)
		PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F7") PORT_CODE(KEYCODE_F7)
		PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F8") PORT_CODE(KEYCODE_F8)
		PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F9") PORT_CODE(KEYCODE_F9)
		PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F10") PORT_CODE(KEYCODE_F10)
		PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(up)") PORT_CODE(KEYCODE_UP)
		PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("???") PORT_CODE(KEYCODE_SLASH_PAD)
		PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("BREAK") PORT_CODE(KEYCODE_PAUSE)

	PORT_START("DSW0")	/* port 6: on-board DIP switches */
		PORT_DIPNAME(0x01, 0x00, "Omninet Address bit 0")
		PORT_DIPSETTING(0x00, DEF_STR( Off ))
		PORT_DIPSETTING(0x01, DEF_STR( On ))
		PORT_DIPNAME(0x02, 0x02, "Omninet Address bit 1")
		PORT_DIPSETTING(0x00, DEF_STR( Off ))
		PORT_DIPSETTING(0x02, DEF_STR( On ))
		PORT_DIPNAME(0x04, 0x00, "Omninet Address bit 2")
		PORT_DIPSETTING(0x00, DEF_STR( Off ))
		PORT_DIPSETTING(0x04, DEF_STR( On ))
		PORT_DIPNAME(0x08, 0x00, "Omninet Address bit 3")
		PORT_DIPSETTING(0x00, DEF_STR( Off ))
		PORT_DIPSETTING(0x08, DEF_STR( On ))
		PORT_DIPNAME(0x10, 0x00, "Omninet Address bit 4")
		PORT_DIPSETTING(0x00, DEF_STR( Off ))
		PORT_DIPSETTING(0x10, DEF_STR( On ))
		PORT_DIPNAME(0x20, 0x00, "Omninet Address bit 5")
		PORT_DIPSETTING(0x00, DEF_STR( Off ))
		PORT_DIPSETTING(0x20, DEF_STR( On ))
		PORT_DIPNAME(0xc0, 0x00, "Type of Boot")
		PORT_DIPSETTING(0x00, "Prompt fo type of Boot")		// Documentation has 0x00 and 0xc0 reversed per boot PROM
		PORT_DIPSETTING(0x40, "Boot from Omninet")
		PORT_DIPSETTING(0x80, "Boot from Local Disk")
		PORT_DIPSETTING(0xc0, "Boot from Diskette")

#if 0
	PORT_START("DISPLAY")	/* port 7: Display orientation */
		PORT_DIPNAME(0x01, 0x00, "Screen Orientation")
		PORT_DIPSETTING(0x00, "Horizontal")
		PORT_DIPSETTING(0x01, "Vertical")
#endif

INPUT_PORTS_END


ROM_START( concept )
	ROM_REGION16_BE(0x100000,"maincpu",0)	/* 68k rom and ram */

	// concept boot ROM
#if 0
	// version 0 level 6 release
	ROM_LOAD16_BYTE("bootl06h", 0x010000, 0x1000, CRC(66b6b259))
	ROM_LOAD16_BYTE("bootl06l", 0x010001, 0x1000, CRC(600940d3))
#elif 0
	// version 1 lvl 7 release
	ROM_LOAD16_BYTE("bootl17h", 0x010000, 0x1000, CRC(6dd9718f))
	ROM_LOAD16_BYTE("bootl17l", 0x010001, 0x1000, CRC(107a3830))
#elif 1
	// version 0 lvl 8 release
	ROM_LOAD16_BYTE("bootl08h", 0x010000, 0x1000, CRC(ee479f51) SHA1(b20ba18564672196076e46507020c6d69a640a2f))
	ROM_LOAD16_BYTE("bootl08l", 0x010001, 0x1000, CRC(acaefd07) SHA1(de0c7eaacaf4c0652aa45e523cebce2b2993c437))
#else
	// version $F lvl 8 (development version found on a floppy disk along with
	// the source code)
	ROM_LOAD16_WORD("cc.prm", 0x010000, 0x2000, CRC(b5a87dab) SHA1(0da59af6cfeeb38672f71731527beac323d9c3d6))
#endif

#if 0
	// only known MACSbug release for the concept, with reset vector and
	// entry point (the reset vector seems to be bogus: is the ROM dump bad,
	// or were the ROMs originally loaded with buggy code?)
	ROM_LOAD16_BYTE("macsbugh", 0x020000, 0x1000, CRC(aa357112))
	ROM_LOAD16_BYTE("macsbugl", 0x020001, 0x1000, CRC(b4b59de9))
#endif

ROM_END

/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT    INIT  CONFIG   COMPANY           FULLNAME */
COMP( 1982, concept,  0,		0,		concept,  concept, 0,    0, "Corvus Systems", "Concept" , 0)
