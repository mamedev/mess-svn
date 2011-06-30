/*

XL 80 cartridge
(c) 1984 20 Data Corporation

PCB Layout
----------

		|==================================|
		|   LS175    LS20    LS139         |
		|   LS157                          |
|=======|                                  |
|=|                          RAM           |
|=|    LS157      LS157             LS165  |
|=|                                        |
|=|                                     CN1|
|=|          CRTC            ROM1          |
|=|                                        |
|=|    ROM0          LS245   LS151         |
|=|                             14.31818MHz|
|=======|            LS174   LS00          |
		|                           HCU04  |
		|    LS74    LS161   LS74          |
		|==================================|

Notes:
    All IC's shown.

	CRTC 	- Hitachi HD46505SP
	RAM  	- Toshiba TMM2016AP-12 2Kx8 Static RAM
	ROM0 	- GI 9433CS-0090 8Kx8 ROM?
	ROM1 	- GI 9316CS-F67 2Kx8 ROM? "DTC"
	CN1  	- RCA video output

*/

#include "c64_xl80.h"



//**************************************************************************
//  MACROS/CONSTANTS
//**************************************************************************

#define RAM_SIZE			0x800

#define HD46505SP_TAG		"mc6845"
#define MC6845_SCREEN_TAG	"screen80"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C64_XL80 = &device_creator<c64_xl80_device>;


//-------------------------------------------------
//  ROM( c64_xl80 )
//-------------------------------------------------

ROM_START( c64_xl80 )
	ROM_REGION( 0x2000, "roml", 0 )
	ROM_LOAD( "9433cs-0090.u3",	0x0000, 0x2000, NO_DUMP )

	ROM_REGION( 0x800, HD46505SP_TAG, 0 )
	ROM_LOAD( "dtc.u14", 0x000, 0x800, NO_DUMP )
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *c64_xl80_device::device_rom_region() const
{
	return ROM_NAME( c64_xl80 );
}

//-------------------------------------------------
//  mc6845_interface crtc_intf
//-------------------------------------------------

void c64_xl80_device::crtc_update_row(mc6845_device *device, bitmap_t *bitmap, const rectangle *cliprect, UINT16 ma, UINT8 ra, UINT16 y, UINT8 x_count, INT8 cursor_x, void *param)
{
}

static MC6845_UPDATE_ROW( c64_xl80_update_row )
{
	c64_xl80_device *xl80 = downcast<c64_xl80_device *>(device->owner());
	xl80->crtc_update_row(device,bitmap,cliprect,ma,ra,y,x_count,cursor_x,param);
}

static const mc6845_interface crtc_intf =
{
	MC6845_SCREEN_TAG,
	8,
	NULL,
	c64_xl80_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};


//-------------------------------------------------
//  GFXDECODE( c64_xl80 )
//-------------------------------------------------

static GFXDECODE_START( c64_xl80 )
	GFXDECODE_ENTRY(HD46505SP_TAG, 0x0000, gfx_8x8x1, 0, 1)
GFXDECODE_END


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( c64_xl80 )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( c64_xl80 )
	MCFG_SCREEN_ADD(MC6845_SCREEN_TAG, RASTER)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(80*8, 24*8)
	MCFG_SCREEN_VISIBLE_AREA(0, 80*8-1, 0, 24*8-1)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MCFG_SCREEN_REFRESH_RATE(50)
	
	//MCFG_GFXDECODE(c64_xl80)

	MCFG_MC6845_ADD(HD46505SP_TAG, H46505, XTAL_14_31818MHz, crtc_intf)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor c64_xl80_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( c64_xl80 );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c64_xl80_device - constructor
//-------------------------------------------------

c64_xl80_device::c64_xl80_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, C64_XL80, "XL 80", tag, owner, clock),
	device_c64_expansion_card_interface(mconfig, *this),
	device_slot_card_interface(mconfig, *this),
	m_crtc(*this, HD46505SP_TAG)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c64_xl80_device::device_start()
{
	m_slot = dynamic_cast<c64_expansion_slot_device *>(owner());

	m_rom = subregion("roml")->base();
	m_char_rom = subregion(HD46505SP_TAG)->base();
	m_video_ram = auto_alloc_array(machine(), UINT8, RAM_SIZE);;

	// state saving
	save_pointer(NAME(m_video_ram), RAM_SIZE);
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void c64_xl80_device::device_reset()
{
}


//-------------------------------------------------
//  c64_cd_r - cartridge data read
//-------------------------------------------------

UINT8 c64_xl80_device::c64_cd_r(offs_t offset, int roml, int romh, int io1, int io2)
{
	return 0;
}


//-------------------------------------------------
//  c64_cd_w - cartridge data write
//-------------------------------------------------

void c64_xl80_device::c64_cd_w(offs_t offset, UINT8 data, int roml, int romh, int io1, int io2)
{
}


//-------------------------------------------------
//  c64_game_r - GAME read
//-------------------------------------------------

int c64_xl80_device::c64_game_r()
{
	return 1;
}


//-------------------------------------------------
//  c64_exrom_r - EXROM read
//-------------------------------------------------

int c64_xl80_device::c64_exrom_r()
{
	return 0;
}


//-------------------------------------------------
//  c64_screen_update - update screen
//-------------------------------------------------

bool c64_xl80_device::c64_screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	m_crtc->update(&bitmap, &cliprect);

	return false;
}