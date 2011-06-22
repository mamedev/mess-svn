/**********************************************************************

    Morrow Designs Disk Jockey 2D/B floppy controller board emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "s100_dj2db.h"



//**************************************************************************
//  MACROS/CONSTANTS
//**************************************************************************

#define MB8866_TAG		"14b"
#define S1602_TAG		"14d"
#define BR1941_TAG		"13d"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type S100_DJ2DB = &device_creator<s100_dj2db_device>;


//-------------------------------------------------
//  ROM( dj2db )
//-------------------------------------------------

ROM_START( dj2db )
	ROM_REGION( 0x400, "dj2db", ROMREGION_INVERT )
	ROM_LOAD( "bv-2 f8.11d", 0x000, 0x400, CRC(b6218d0b) SHA1(e4b2ae886c0dd7717e2e02ae2e202115d8ec2def) )

	ROM_REGION( 0x220, "proms", 0 )
	ROM_LOAD( "8c-b f8.8c", 0x000, 0x200, NO_DUMP ) // 6301
	ROM_LOAD( "3d.3d", 0x200, 0x20, NO_DUMP ) // 6331
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *s100_dj2db_device::device_rom_region() const
{
	return ROM_NAME( dj2db );
}


//-------------------------------------------------
//  COM8116_INTERFACE( brg_intf )
//-------------------------------------------------

static COM8116_INTERFACE( brg_intf )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	{ 6336, 4224, 2880, 2355, 2112, 1056, 528, 264, 176, 158, 132, 88, 66, 44, 33, 16 }, // from WD1943-00 datasheet
	{ 6336, 4224, 2880, 2355, 2112, 1056, 528, 264, 176, 158, 132, 88, 66, 44, 33, 16 },
};


//-------------------------------------------------
//  wd17xx_interface fdc_intf
//-------------------------------------------------

static const floppy_interface dj2db_floppy_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_8_DSDD,
	FLOPPY_OPTIONS_NAME(default),
	"floppy_8",
	NULL
};

static const wd17xx_interface fdc_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( s100_dj2db )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( s100_dj2db )
	MCFG_COM8116_ADD(BR1941_TAG, XTAL_5_0688MHz, brg_intf)
	MCFG_WD179X_ADD(MB8866_TAG, fdc_intf) // FD1791
	MCFG_FLOPPY_2_DRIVES_ADD(dj2db_floppy_interface)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor s100_dj2db_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( s100_dj2db );
}


//-------------------------------------------------
//  INPUT_PORTS( mm65k16s )
//-------------------------------------------------

static INPUT_PORTS_START( dj2db )
	PORT_START("SW1")
	PORT_DIPNAME( 0xf8, 0xf8, "Power-On Jump Address" ) PORT_DIPLOCATION("SW1:1,2,3,4,5") PORT_CONDITION("s100_3:dj2db:SW1", 0x01, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0xf8, "F800H" )
	PORT_DIPSETTING(    0xf0, "F000H" )
	PORT_DIPSETTING(    0xe8, "E800H" )
	PORT_DIPSETTING(    0xe0, "E000H" )
	PORT_DIPSETTING(    0xd8, "D800H" )
	PORT_DIPSETTING(    0xd0, "D000H" )
	PORT_DIPSETTING(    0xc8, "C800H" )
	PORT_DIPSETTING(    0xc0, "C000H" )
	PORT_DIPSETTING(    0xb8, "B800H" )
	PORT_DIPSETTING(    0xb0, "B000H" )
	PORT_DIPSETTING(    0xa8, "A800H" )
	PORT_DIPSETTING(    0xa0, "A000H" )
	PORT_DIPSETTING(    0x98, "9800H" )
	PORT_DIPSETTING(    0x90, "9000H" )
	PORT_DIPSETTING(    0x88, "8800H" )
	PORT_DIPSETTING(    0x80, "8000H" )
	PORT_DIPSETTING(    0x78, "7800H" )
	PORT_DIPSETTING(    0x70, "7000H" )
	PORT_DIPSETTING(    0x68, "6800H" )
	PORT_DIPSETTING(    0x60, "6000H" )
	PORT_DIPSETTING(    0x58, "5800H" )
	PORT_DIPSETTING(    0x50, "5000H" )
	PORT_DIPSETTING(    0x48, "4800H" )
	PORT_DIPSETTING(    0x40, "4000H" )
	PORT_DIPSETTING(    0x38, "3800H" )
	PORT_DIPSETTING(    0x30, "3000H" )
	PORT_DIPSETTING(    0x28, "2800H" )
	PORT_DIPSETTING(    0x20, "2000H" )
	PORT_DIPSETTING(    0x18, "1800H" )
	PORT_DIPSETTING(    0x10, "1000H" )
	PORT_DIPSETTING(    0x08, "0800H" )
	PORT_DIPSETTING(    0x00, "0000H" )
	PORT_DIPNAME( 0x04, 0x04, "Phantom Line" ) PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(    0x04, "Disabled" )
	PORT_DIPSETTING(    0x00, "Enabled" )
	PORT_DIPNAME( 0x02, 0x02, "Bus Speed" ) PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(    0x02, "2 MHz" )
	PORT_DIPSETTING(    0x00, "4/6 MHz" )
	PORT_DIPNAME( 0x01, 0x01, "Power-On Jump" ) PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x01, "Disabled" )
	PORT_DIPSETTING(    0x00, "Enabled" )

	PORT_START("SW2")
	PORT_DIPNAME( 0x0f, 0x0f, "Baud Rate" ) PORT_DIPLOCATION("SW2:1,2,3,4")
	PORT_DIPSETTING(    0x08, "110" )
	PORT_DIPSETTING(    0x0e, "1200" )
	PORT_DIPSETTING(    0x07, "9600" )
	PORT_DIPSETTING(    0x0f, "19200" )
	PORT_DIPNAME( 0x10, 0x00, "Word Length" ) PORT_DIPLOCATION("SW2:5")
	PORT_DIPSETTING(    0x10, "8 Bits" )
	PORT_DIPSETTING(    0x00, "7 Bits" )
	PORT_DIPNAME( 0x20, 0x20, "Stop Bit Count" ) PORT_DIPLOCATION("SW2:6")
	PORT_DIPSETTING(    0x20, "2 Stop Bits" )
	PORT_DIPSETTING(    0x00, "1 Stop Bit" )
	PORT_DIPNAME( 0x40, 0x40, "Parity" ) PORT_DIPLOCATION("SW2:7") PORT_CONDITION("s100_3:dj2db:SW2", 0x80, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x40, "Even Parity" )
	PORT_DIPSETTING(    0x00, "Odd Parity" )
	PORT_DIPNAME( 0x80, 0x80, "Parity" ) PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("J4")
	PORT_DIPNAME( 0x01, 0x00, "Power Up" )
	PORT_DIPSETTING(    0x01, "Inactive" )
	PORT_DIPSETTING(    0x00, "Active" )

	PORT_START("J2")
	PORT_DIPNAME( 0x01, 0x01, "Generate PHANTOM Signal" )
	PORT_DIPSETTING(    0x01, "Disabled" )
	PORT_DIPSETTING(    0x00, "Enabled" )
	
	PORT_START("J3A")
	PORT_DIPNAME( 0x01, 0x00, "Bank Select Bit 0" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPNAME( 0x02, 0x00, "Bank Select Bit 1" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPNAME( 0x04, 0x00, "Bank Select Bit 2" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPNAME( 0x08, 0x00, "Bank Select Bit 3" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPNAME( 0x10, 0x00, "Bank Select Bit 4" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPNAME( 0x20, 0x00, "Bank Select Bit 5" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPNAME( 0x40, 0x00, "Bank Select Bit 6" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x40, "1" )
	PORT_DIPNAME( 0x80, 0x00, "Bank Select Bit 7" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x80, "1" )
	
	PORT_START("J1A")
	PORT_DIPNAME( 0x0f, 0x09, "Interrupt" )
	PORT_DIPSETTING(    0x09, "Disabled")
	PORT_DIPSETTING(    0x00, "VI0")
	PORT_DIPSETTING(    0x01, "VI1")
	PORT_DIPSETTING(    0x02, "VI2")
	PORT_DIPSETTING(    0x03, "VI3")
	PORT_DIPSETTING(    0x04, "VI4")
	PORT_DIPSETTING(    0x05, "VI5")
	PORT_DIPSETTING(    0x06, "VI6")
	PORT_DIPSETTING(    0x07, "VI7")
	PORT_DIPSETTING(    0x08, "PINT")
INPUT_PORTS_END


//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

ioport_constructor s100_dj2db_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( dj2db );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  s100_dj2db_device - constructor
//-------------------------------------------------

s100_dj2db_device::s100_dj2db_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, S100_DJ2DB, "DJ2DB", tag, owner, clock),
	device_s100_card_interface(mconfig, *this),
	device_slot_card_interface(mconfig, *this),
	m_fdc(*this, MB8866_TAG),
	m_dbrg(*this, BR1941_TAG)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void s100_dj2db_device::device_start()
{
	m_s100 = machine().device<s100_device>("s100");

	m_rom = subregion("dj2db")->base();
	m_ram = auto_alloc_array(machine(), UINT8, 0x400);
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void s100_dj2db_device::device_reset()
{
}


//-------------------------------------------------
//  s100_smemr_r - memory read
//-------------------------------------------------

UINT8 s100_dj2db_device::s100_smemr_r(offs_t offset)
{
	UINT8 data = 0;
	
	if ((offset >= 0xf800) && (offset < 0xfbf8))
	{
		data = m_rom[offset & 0x3ff];
	}
	else if (offset == 0xfbf8)
	{
		// UART inverted data
	}
	else if (offset == 0xfbf9)
	{
		// UART inverted status
	}
	else if (offset == 0xfbfa)
	{
		// disk jockey status
	}
	else if ((offset >= 0xfbfc) && (offset < 0xfc00))
	{
		data = wd17xx_r(m_fdc, offset & 0x03);
	}
	else if ((offset >= 0xfc00) && (offset < 0x10000))
	{
		data = m_ram[offset & 0x3ff];
	}
	
	return data;
}


//-------------------------------------------------
//  s100_mwrt_w - memory write
//-------------------------------------------------

void s100_dj2db_device::s100_mwrt_w(offs_t offset, UINT8 data)
{
	if (offset == 0xfbf8)
	{
		// UART inverted data
	}
	else if (offset == 0xfbf9)
	{
		// disk jockey function
	}
	else if (offset == 0xfbfa)
	{
		// drive control register
	}
	else if ((offset >= 0xfbfc) && (offset < 0xfc00))
	{
		wd17xx_w(m_fdc, offset & 0x03, data);
	}
	else if ((offset >= 0xfc00) && (offset < 0x10000))
	{
		m_ram[offset & 0x3ff] = data;
	}
}


//-------------------------------------------------
//  s100_sinp_r - I/O read
//-------------------------------------------------

UINT8 s100_dj2db_device::s100_sinp_r(offs_t offset)
{
	return 0;
}


//-------------------------------------------------
//  s100_sout_w - I/O write
//-------------------------------------------------

void s100_dj2db_device::s100_sout_w(offs_t offset, UINT8 data)
{
}


//-------------------------------------------------
//  s100_phantom_w - phantom
//-------------------------------------------------

void s100_dj2db_device::s100_phantom_w(int state)
{
}


//-------------------------------------------------
//  s100_terminal_w - terminal write
//-------------------------------------------------

void s100_dj2db_device::s100_terminal_w(UINT8 data)
{
}