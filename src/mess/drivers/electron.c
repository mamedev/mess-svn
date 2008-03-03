/******************************************************************************
    Acorn Electron driver

    MESS Driver By:

    Wilbert Pol

I don't have a real system to verify the behaviour of the emulation. The things
that can be done through BASIC programs seem to behave properly (most of the time :).

Incomplete:
    - Sound (sound is too high?)
    - Graphics (seems to be wrong for several games)
    - 1MHz bus is not emulated
    - Bus claiming by ULA is not implemented

Missing:
    - Support for ROM images
    - Support for floppy disks
    - Other peripherals

******************************************************************************/

#include "driver.h"
#include "includes/electron.h"
#include "devices/cassette.h"
#include "formats/uef_cas.h"

static const unsigned short electron_colour_table[8]=
{
	0,1,2,3,4,5,6,7
};

static const rgb_t electron_palette[8]=
{
	MAKE_RGB(0x0ff,0x0ff,0x0ff),
	MAKE_RGB(0x000,0x0ff,0x0ff),
	MAKE_RGB(0x0ff,0x000,0x0ff),
	MAKE_RGB(0x000,0x000,0x0ff),
	MAKE_RGB(0x0ff,0x0ff,0x000),
	MAKE_RGB(0x000,0x0ff,0x000),
	MAKE_RGB(0x0ff,0x000,0x000),
	MAKE_RGB(0x000,0x000,0x000)
};

static PALETTE_INIT( electron )
{
	palette_set_colors(machine, 0, electron_palette, ARRAY_LENGTH(electron_palette));
}

static ADDRESS_MAP_START(electron_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x7fff) AM_RAM AM_REGION(REGION_CPU1,  0x00000)	/* 32KB of RAM */
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(2)								/* Banked ROM pages */
	AM_RANGE(0xc000, 0xfbff) AM_ROM AM_REGION(REGION_USER1, 0x40000)	/* OS ROM */
	AM_RANGE(0xfc00, 0xfcff) AM_READWRITE( electron_jim_r, electron_jim_w )			/* JIM pages */
	AM_RANGE(0xfd00, 0xfdff) AM_READWRITE( electron_1mhz_r, electron_1mhz_w )		/* 1MHz bus */
	AM_RANGE(0xfe00, 0xfeff) AM_READWRITE( electron_ula_r, electron_ula_w )			/* Electron ULA */
	AM_RANGE(0xff00, 0xffff) AM_ROM AM_REGION(REGION_USER1, 0x43f00)	/* OS ROM continued */
ADDRESS_MAP_END

static INPUT_PORTS_START( electron )
	/* Keyboard line 0 */
	PORT_START
	PORT_BIT(0x01,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RIGHT CURSOR") PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT(0x02,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("COPY") PORT_CODE(KEYCODE_END)
	/* PORT_BIT(0x04,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("<NONE>") PORT_CODE(KEYCODE_) */
	PORT_BIT(0x08,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE)

	/* Keyboard line 1 */
	PORT_START
	PORT_BIT(0x01,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("LEFT CURSOR") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0x02,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("DOWN CURSOR") PORT_CODE(KEYCODE_DOWN)
	PORT_BIT(0x04,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x08,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("DELETE") PORT_CODE(KEYCODE_BACKSPACE)

	/* Keyboard line 2 */
	PORT_START
	PORT_BIT(0x01,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT(0x02,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("UP CURSOR") PORT_CODE(KEYCODE_UP)
	PORT_BIT(0x04,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(":") PORT_CODE(KEYCODE_QUOTE)
	/* PORT_BIT(0x08,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("<NONE>") PORT_CODE(KEYCODE_) */

	/* Keyboard line 3 */
	PORT_START
	PORT_BIT(0x01,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x02,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x04,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON)
	PORT_BIT(0x08,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH)

	/* Keyboard line 4 */
	PORT_START
	PORT_BIT(0x01,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x02,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x04,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x08,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)

	/* Keyboard line 5 */
	PORT_START
	PORT_BIT(0x01,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x02,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
	PORT_BIT(0x04,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
	PORT_BIT(0x08,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA)

	/* Keyboard line 6 */
	PORT_START
	PORT_BIT(0x01,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x02,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x04,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
	PORT_BIT(0x08,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)

	/* Keyboard line 7 */
	PORT_START
	PORT_BIT(0x01,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x02,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x04,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x08,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)

	/* Keyboard line 8 */
	PORT_START
	PORT_BIT(0x01,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x02,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x04,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
	PORT_BIT(0x08,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)

	/* Keyboard line 9 */
	PORT_START
	PORT_BIT(0x01,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x02,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x04,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x08,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)

	/* Keyboard line 10 */
	PORT_START
	PORT_BIT(0x01,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x02,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x04,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x08,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)

	/* Keyboard line 11 */
	PORT_START
	PORT_BIT(0x01,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x02,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x04,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x08,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)

	/* Keyboard line 12 */
	PORT_START
	PORT_BIT(0x01,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x02,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x04,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x08,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)

	/* Keyboard line 13 */
	PORT_START
	PORT_BIT(0x01,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ESCAPE") PORT_CODE(KEYCODE_ESC)
	PORT_BIT(0x02,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("FUNC") PORT_CODE(KEYCODE_TAB)
	PORT_BIT(0x04,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(0x04,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_RCONTROL)
	PORT_BIT(0x08,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT(0x08,  IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT)

INPUT_PORTS_END

/* Electron Rom Load */
ROM_START(electron)
	ROM_REGION( 0x10000, REGION_CPU1, ROMREGION_ERASEFF )
	ROM_REGION( 0x44000, REGION_USER1, 0 ) /* OS Rom */
	ROM_LOAD( "os.rom", 0x40000, 0x4000, CRC(bf63fb1f) SHA1(a48b8fa0cfb09140e808ac8a187316c605a0b32e) ) /* Os rom */
	/* 00000 0 */
        /* 04000 1 */
        /* 08000 2 */
        /* 0c000 3 */
        /* 10000 4 */
        /* 14000 5 */
        /* 18000 6 */
        /* 1c000 7 */
        /* 20000 8 */
        /* 24000 9 */
	ROM_LOAD( "basic.rom", 0x28000, 0x4000, CRC(79434781) SHA1(4a7393f3a45ea309f744441c16723e2ef447a281) ) /* page 10, Basic rom */
	ROM_COPY( REGION_USER1, 0x28000, 0x2c000, 0x4000 ) /* page 11, Basic rom mirror */
        /* 30000 12 */
        /* 34000 13 */
        /* 38000 14 */
        /* 3c000 15 */
ROM_END

static MACHINE_DRIVER_START( electron )
	MDRV_CPU_ADD_TAG( "main", M6502, 2000000 )
	MDRV_CPU_PROGRAM_MAP( electron_mem, 0 )
	MDRV_CPU_VBLANK_INT( electron_scanline_interrupt, 312 ) /* scanline interrupt */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE( 50.08 )

	MDRV_MACHINE_START( electron )

	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE( 640, 312 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 640-1, 0, 256-1 )
	MDRV_PALETTE_LENGTH( 16 )
	MDRV_PALETTE_INIT(electron)

	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( generic_bitmapped )

	MDRV_SPEAKER_STANDARD_MONO( "mono" )
	MDRV_SOUND_ADD( BEEP, 0 )
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "mono", 1.00 )
MACHINE_DRIVER_END

static void electron_cassette_getinfo( const mess_device_class *devclass, UINT32 state, union devinfo *info ) {
	switch( state ) {
	case MESS_DEVINFO_INT_COUNT:
		info->i = 1;
		break;
	case MESS_DEVINFO_PTR_CASSETTE_FORMATS:
		info->p = (void *)uef_cassette_formats;
		break;
	default:
		cassette_device_getinfo( devclass, state, info );
		break;
	}
}

SYSTEM_CONFIG_START(electron)
	CONFIG_DEVICE(electron_cassette_getinfo)
SYSTEM_CONFIG_END

/*     YEAR  NAME      PARENT COMPAT    MACHINE   INPUT     INIT  CONFIG    COMPANY  FULLNAME */
COMP ( 1983, electron, 0,     0,        electron, electron, 0,    electron, "Acorn", "Acorn Electron", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
