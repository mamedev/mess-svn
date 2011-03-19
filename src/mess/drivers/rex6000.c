/***************************************************************************

        Xircom / Intel REX 6000

        Driver by Sandro Ronco

****************************************************************************/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/tc8521.h"
#include "imagedev/snapquik.h"
#include "rendlay.h"

#define MAKE_BANK_ADDRESS(lo, hi)		(((lo)<<13) | ((hi)<<21))

class rex6000_state : public driver_device
{
public:
	rex6000_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, "maincpu")
		{ }

	required_device<cpu_device> m_maincpu;

	UINT8 m_bank[4];
	UINT8 m_lcd_base[2];
	UINT8 m_lcd_enabled;
	UINT8 m_lcd_cmd;

	UINT32 m_lcd_addr;
	UINT8 *m_bank_base;
	UINT8 m_irq_mask;
	UINT8 m_irq_flag;
	UINT8 m_port6;

	virtual void machine_start();
	bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	DECLARE_READ8_MEMBER( bankswitch_r );
	DECLARE_WRITE8_MEMBER( bankswitch_w );
	DECLARE_READ8_MEMBER( lcd_base_r );
	DECLARE_WRITE8_MEMBER( lcd_base_w );
	DECLARE_READ8_MEMBER( lcd_io_r );
	DECLARE_WRITE8_MEMBER( lcd_io_w );
	DECLARE_READ8_MEMBER( irq_r );
	DECLARE_WRITE8_MEMBER( irq_w );
	DECLARE_READ8_MEMBER( touchscreen_r );
};

READ8_MEMBER( rex6000_state::bankswitch_r )
{
	return m_bank[offset];
}

WRITE8_MEMBER( rex6000_state::bankswitch_w )
{
	m_bank[offset&3] = data;

	switch (offset)
	{
		case 0:		//bank 1 low
		case 1:		//bank 1 high
		{
			UINT32 new_bank = MAKE_BANK_ADDRESS(m_bank[0], m_bank[1]&0x0f) + 0x8000;
			memory_set_bankptr(machine, "bank1", m_bank_base + new_bank);
			//printf("Bank1: %x %x %x\n", m_bank[0], m_bank[1], new_bank);
			break;
		}
		case 2:		//bank 2 low
		case 3:		//bank 2 high
		{
			UINT32 new_bank = MAKE_BANK_ADDRESS(m_bank[2], m_bank[3]&0x0f) + ((m_bank[3]&0x10) ? 0x200000 : 0);
			memory_set_bankptr(machine, "bank2", m_bank_base + new_bank);
			//printf("Bank2: %x %x %x\n", m_bank[2], m_bank[3], new_bank);
			break;
		}
	}
}

READ8_MEMBER( rex6000_state::lcd_base_r )
{
	return m_lcd_base[offset];
}

WRITE8_MEMBER( rex6000_state::lcd_base_w )
{
	m_lcd_base[offset&1] = data;
	m_lcd_addr = MAKE_BANK_ADDRESS(m_lcd_base[0], (m_lcd_base[1]&0x0f)) + ((m_lcd_base[1]&0x10) ? 0x200000 : 0);
	//printf("Set LCD base at %x\n", m_lcd_addr);
}

READ8_MEMBER( rex6000_state::lcd_io_r )
{
	return (offset == 0) ? m_lcd_enabled : m_lcd_cmd;
}

WRITE8_MEMBER( rex6000_state::lcd_io_w )
{
	switch (offset)
	{
		case 0:
			m_lcd_enabled = data;
			break;
		case 1:
			m_lcd_cmd = data;
			break;
	}
}

READ8_MEMBER( rex6000_state::irq_r )
{
	switch (offset)
	{
		case 0: //irq flag
			/*
                ---- ---x   input port
                ---x ----   sec timer
                --x- ----   timer
            */
			return m_irq_flag;

		case 1:
			return m_port6;

		case 2: //irq mask
			return m_irq_mask;

		default:
			return 0;
	}
}

WRITE8_MEMBER( rex6000_state::irq_w )
{
	switch (offset)
	{
		case 0: //irq flag
			m_irq_flag = data;
			break;
		case 1:	//clear irq flag
			m_port6 = data;
			m_irq_flag &= (~data);
			break;
		case 2: //irq mask
			m_irq_mask = data;
			break;
	}
}

READ8_MEMBER( rex6000_state::touchscreen_r )
{
	UINT16 x = input_port_read(space.machine, "PENX");
	UINT16 y = input_port_read(space.machine, "PENY");

	switch (offset)
	{
		case 0:
			return ((input_port_read(space.machine, "INPUT") & 0x40) ? 0x20 : 0x00) | 0X10;
		case 1:
			return (y>>0) & 0xff;
		case 2:
			return (y>>8) & 0xff;
		case 3:
			return (x>>0) & 0xff;
		case 4:
			return (x>>8) & 0xff;
	}

	return 0;
}

static ADDRESS_MAP_START(rex6000_mem, ADDRESS_SPACE_PROGRAM, 8, rex6000_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x7fff ) AM_ROM	AM_REGION("flash", 0)
	AM_RANGE( 0x8000, 0x9fff ) AM_RAMBANK("bank1")
	AM_RANGE( 0xa000, 0xbfff ) AM_RAMBANK("bank2")
	AM_RANGE( 0xc000, 0xffff ) AM_RAM				//system RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( rex6000_io, ADDRESS_SPACE_IO, 8, rex6000_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE( 0x01, 0x04 ) AM_READWRITE(bankswitch_r, bankswitch_w)
	AM_RANGE( 0x05, 0x07 ) AM_READWRITE(irq_r, irq_w)
	AM_RANGE( 0x10, 0x10 ) AM_READ_PORT("INPUT")
	AM_RANGE( 0x15, 0x19 ) AM_NOP	//beep
	AM_RANGE( 0x22, 0x23 ) AM_READWRITE(lcd_base_r, lcd_base_w)
	AM_RANGE( 0x30, 0x3f ) AM_DEVREADWRITE_LEGACY("rtc", tc8521_r, tc8521_w)
	AM_RANGE( 0x40, 0x47 ) AM_NOP	//SIO
	AM_RANGE( 0x50, 0x51 ) AM_READWRITE(lcd_io_r, lcd_io_w)
	AM_RANGE( 0x68, 0x6c ) AM_READ(touchscreen_r)
	//AM_RANGE( 0x00, 0xff ) AM_RAM
ADDRESS_MAP_END

static INPUT_CHANGED( trigger_irq )
{
	rex6000_state *state = field->port->machine->driver_data<rex6000_state>();

	state->m_irq_flag |= 0x01;

	cpu_set_input_line(state->m_maincpu, 0, HOLD_LINE);
}

/* Input ports */
INPUT_PORTS_START( rex6000 )
	PORT_START("INPUT")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD)  PORT_NAME("Home")	PORT_CODE(KEYCODE_ENTER)		PORT_CHANGED(trigger_irq, 0)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD)  PORT_NAME("Back")	PORT_CODE(KEYCODE_BACKSPACE)	PORT_CHANGED(trigger_irq, 0)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD)  PORT_NAME("Select")		PORT_CODE(KEYCODE_SPACE)		PORT_CHANGED(trigger_irq, 0)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD)  PORT_NAME("Up")		PORT_CODE(KEYCODE_UP)			PORT_CHANGED(trigger_irq, 0)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYPAD)  PORT_NAME("Down")	PORT_CODE(KEYCODE_DOWN)			PORT_CHANGED(trigger_irq, 0)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Pen")	PORT_CODE(MOUSECODE_BUTTON1)	PORT_CHANGED(trigger_irq, 0)

	PORT_START("PENX")
	PORT_BIT(0x3ff, 0x00, IPT_LIGHTGUN_X) PORT_NAME("Pen X") PORT_CROSSHAIR(X, 1, 0, 0) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_INVERT

	PORT_START("PENY")
	PORT_BIT(0x3ff, 0x00, IPT_LIGHTGUN_Y) PORT_NAME("Pen Y") PORT_CROSSHAIR(Y, 1, 0, 0) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_INVERT
INPUT_PORTS_END

void rex6000_state::machine_start()
{
	m_bank_base = (UINT8*)machine->region("flash")->base();

	memory_set_bankptr(machine, "bank1", m_bank_base + 0x8000);
	memory_set_bankptr(machine, "bank2", m_bank_base);
}

bool rex6000_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	UINT8 *lcd_base = m_bank_base + m_lcd_addr;

	if (m_lcd_enabled)
	{
		for (int y=0; y<120; y++)
			for (int x=0; x<30; x++)
			{
				UINT8 data = lcd_base[y*30 + x];

				for (int b=0; b<8; b++)
				{
					*BITMAP_ADDR16(&bitmap, y, (x * 8) + b) = BIT(data, 7);
					data <<= 1;
				}
			}
	}
	else
	{
		bitmap_fill(&bitmap, &cliprect, 0);
	}

	return 0;
}

static TIMER_DEVICE_CALLBACK( irq_timer )
{
	rex6000_state *state = timer.machine->driver_data<rex6000_state>();

	state->m_irq_flag |= 0x20;

	cpu_set_input_line(state->m_maincpu, 0, HOLD_LINE);
}

static TIMER_DEVICE_CALLBACK( sec_timer )
{
	rex6000_state *state = timer.machine->driver_data<rex6000_state>();

	state->m_irq_flag |= 0x10;

	cpu_set_input_line(state->m_maincpu, 0, HOLD_LINE);
}


static PALETTE_INIT( rex6000 )
{
	palette_set_color(machine, 0, MAKE_RGB(138, 146, 148));
	palette_set_color(machine, 1, MAKE_RGB(92, 83, 88));
}

static QUICKLOAD_LOAD(rex6000)
{
	static const char magic[] = "ApplicationName:Addin";
	running_machine *machine = image.device().machine;
	UINT32 img_start = 0;
	UINT8 *data;

	data = (UINT8*)auto_alloc_array(machine, UINT8, image.length());
	image.fread(data, image.length());

	if(strncmp((const char*)data, magic, 21))
		return IMAGE_INIT_FAIL;

	img_start = strlen((const char*)data) + 5;
	img_start += 0xa0;	//skip the icon (40x32 pixel)

	memcpy((UINT8*)machine->region("flash")->base() + 0x100000, data + img_start, image.length() - img_start);
	auto_free(machine, data);

	return IMAGE_INIT_PASS;
}


/* F4 Character Displayer */
static const gfx_layout rex6000_bold_charlayout =
{
	16, 11,					/* 16 x 11 characters */
	256,					/* 256 characters */
	1,						/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 },
	/* y offsets */
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16, 8*16, 9*16, 10*16 },
	8*24			/* every char takes 24 bytes, first 2 bytes are used for the char size */
};

static const gfx_layout rex6000_tiny_charlayout =
{
	16, 9,					/* 16 x 9 characters */
	256,					/* 256 characters */
	1,						/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 },
	/* y offsets */
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16, 8*16 },
	8*20			/* every char takes 20 bytes, first 2 bytes ared use for the char size */
};

static const gfx_layout rex6000_graph_charlayout =
{
	16, 13,					/* 16 x 13 characters */
	48,						/* 48 characters */
	1,						/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 },
	/* y offsets */
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16, 8*16, 9*16, 10*16 , 11*16 , 12*16 },
	8*28			/* every char takes 28 bytes, first 2 bytes are used for the char size */
};

static GFXDECODE_START( rex6000 )
	GFXDECODE_ENTRY( "flash", 0x0f0000, rex6000_bold_charlayout,  0, 0 )	//normal
	GFXDECODE_ENTRY( "flash", 0x0f2000, rex6000_bold_charlayout,  0, 0 )	//bold
	GFXDECODE_ENTRY( "flash", 0x0f4000, rex6000_tiny_charlayout,  0, 0 )	//tiny
	GFXDECODE_ENTRY( "flash", 0x0f6000, rex6000_graph_charlayout, 0, 0 )	//graphic
GFXDECODE_END


static const tc8521_interface rex6000_tc8521_interface =
{
	NULL
};

static MACHINE_CONFIG_START( rex6000, rex6000_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz) //Toshiba microprocessor Z80 compatible at 4.3MHz
    MCFG_CPU_PROGRAM_MAP(rex6000_mem)
    MCFG_CPU_IO_MAP(rex6000_io)

	MCFG_TIMER_ADD_PERIODIC("irq_timer", irq_timer, attotime::from_msec(25))
	MCFG_TIMER_ADD_PERIODIC("sec_timer", sec_timer, attotime::from_seconds(1))

    /* video hardware */
    MCFG_SCREEN_ADD("screen", LCD)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(240, 120)
    MCFG_SCREEN_VISIBLE_AREA(0, 240-1, 0, 120-1)
	MCFG_DEFAULT_LAYOUT(layout_lcd)
    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(rex6000)
	MCFG_GFXDECODE(rex6000)

	/* quickload */
	MCFG_QUICKLOAD_ADD("quickload", rex6000, "rex,ds2", 0)

	MCFG_TC8521_ADD("rtc", rex6000_tc8521_interface)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( rex6000 )
    ROM_REGION( 0x400000, "flash", ROMREGION_ERASE )
	ROM_LOAD( "rex6000.dat", 0x0000, 0x200000, CRC(b476f7e0) SHA1(46a56634576408a5b0dca80aea08513e856c3bb1))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 2000, rex6000,  0,       0,	rex6000,	rex6000,	 0,   "Xircom / Intel",   "REX 6000",		GAME_NOT_WORKING | GAME_NO_SOUND)
