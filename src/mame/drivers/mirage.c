/* Mirage Youjuu Mahjongden

todo: inputs, eeprom?
      sound (is mbl-03.10a a bad dump?)
*/

/*

Mirage Youjuu Mahjongden
(c)1994 Mitchell

MT4001-2
DEC-22V0
all custom chips are surface scratched, but I believe they are DECO156 and mates.

Sound: M6295x2
OSC  : 28.0000MHz

MBL-00.7A    [2e258b7b]

MBL-01.11A   [895be69a]
MBL-02.12A   [474f6104]

MBL-03.10A   [4a599703]

MBL-04.12K   [b533123d]

MR_00-.2A    [3a53f33d]
MR_01-.3A    [a0b758aa]

*/

#include "driver.h"
#include "decocrpt.h"
#include "decoprot.h"
#include "deco16ic.h"
#include "sound/okim6295.h"

#include "deco16ic.h"

static void draw_sprites(running_machine *machine, bitmap_t *bitmap,const rectangle *cliprect)
{
	int offs;

	for (offs = 0;offs < 0x400;offs += 4)
	{
		int x,y,sprite,colour,multi,fx,fy,inc,flash,mult;

		sprite = spriteram16[offs+1];
		if (!sprite) continue;

		y = spriteram16[offs];
		flash=y&0x1000;
		if (flash && (video_screen_get_frame_number(machine->primary_screen) & 1)) continue;

		x = spriteram16[offs+2];
		colour = (x >>9) & 0x1f;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 320) x -= 512;
		if (y >= 256) y -= 512;
		y = 240 - y;
        x = 304 - x;

		if (x>320) continue;

		sprite &= ~multi;
		if (fy)
			inc = -1;
		else
		{
			sprite += multi;
			inc = 1;
		}

		if (flip_screen_get())
		{
			y=240-y;
			x=304-x;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
			mult=16;
		}
		else mult=-16;

		while (multi >= 0)
		{
			drawgfx(bitmap,machine->gfx[2],
					sprite - multi * inc,
					colour,
					fx,fy,
					x,y + mult * multi,
					cliprect,TRANSPARENCY_PEN,0);

			multi--;
		}
	}
}

static int mirage_bank_callback(const int bank)
{
	return ((bank>>4)&0x7) * 0x1000;
}

static VIDEO_START(mirage)
{
	deco16_1_video_init();

	deco16_set_tilemap_bank_callback(0, mirage_bank_callback);
	deco16_set_tilemap_bank_callback(1, mirage_bank_callback);
}

static VIDEO_UPDATE(mirage)
{
	flip_screen_set( deco16_pf12_control[0]&0x80 );
	deco16_pf12_update(deco16_pf1_rowscroll,deco16_pf2_rowscroll);

	fillbitmap(bitmap,256,cliprect); /* not verified */

	deco16_tilemap_2_draw(bitmap,cliprect,TILEMAP_DRAW_OPAQUE,0);
	deco16_tilemap_1_draw(bitmap,cliprect,0,0);

	draw_sprites(screen->machine,bitmap,cliprect);
	return 0;
}


static READ16_HANDLER( mirage_controls_r )
{
	return input_port_read(machine, "SYSTEM_IN");
}

static READ16_HANDLER( random_readers )
{
	return mame_rand(machine);
}

static READ16_HANDLER( mirage_input_r )
{
	UINT16 port = input_port_read(machine, "MIRAGE0");
	return port;
}

static ADDRESS_MAP_START( mirage_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(SMH_ROM)

	AM_RANGE(0x100000, 0x101fff) AM_READ(SMH_RAM)
	AM_RANGE(0x102000, 0x103fff) AM_READ(SMH_RAM)

	AM_RANGE(0x110000, 0x110bff) AM_READ(SMH_RAM)
	AM_RANGE(0x112000, 0x112bff) AM_READ(SMH_RAM)

	AM_RANGE(0x120000, 0x1207ff) AM_READ(SMH_RAM)

	AM_RANGE(0x130000, 0x1307ff) AM_READ(SMH_RAM)

	AM_RANGE(0x140006, 0x140007) AM_READ(random_readers)

	AM_RANGE(0x150006, 0x150007) AM_READ(SMH_NOP)

	AM_RANGE(0x16c006, 0x16c007) AM_READ(mirage_input_r)

	AM_RANGE(0x16e002, 0x16e003) AM_READ(mirage_controls_r)

	AM_RANGE(0x170000, 0x173fff) AM_READ(SMH_RAM)

ADDRESS_MAP_END

static ADDRESS_MAP_START( mirage_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(SMH_ROM)

	/* tilemaps */
	AM_RANGE(0x100000, 0x101fff) AM_WRITE(deco16_pf1_data_w) AM_BASE(&deco16_pf1_data) // 0x100000 - 0x101fff tested
	AM_RANGE(0x102000, 0x103fff) AM_WRITE(deco16_pf2_data_w) AM_BASE(&deco16_pf2_data) // 0x102000 - 0x102fff tested
	/* linescroll */
	AM_RANGE(0x110000, 0x110bff) AM_WRITE(SMH_RAM) AM_BASE(&deco16_pf1_rowscroll)
	AM_RANGE(0x112000, 0x112bff) AM_WRITE(SMH_RAM) AM_BASE(&deco16_pf2_rowscroll)

	AM_RANGE(0x120000, 0x1207ff) AM_WRITE(SMH_RAM) AM_BASE(&spriteram16)

	AM_RANGE(0x130000, 0x1307ff) AM_WRITE(paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)

	AM_RANGE(0x150000, 0x150001) AM_WRITE(SMH_NOP)
	AM_RANGE(0x150002, 0x150003) AM_WRITE(SMH_NOP)

	AM_RANGE(0x160000, 0x160001) AM_WRITE(SMH_NOP)

	AM_RANGE(0x168000, 0x16800f) AM_WRITE(SMH_RAM) AM_BASE(&deco16_pf12_control)

	AM_RANGE(0x16a000, 0x16a001) AM_WRITE(SMH_NOP)

	AM_RANGE(0x16c000, 0x16c001) AM_WRITE(SMH_NOP)
	AM_RANGE(0x16c002, 0x16c003) AM_WRITE(SMH_NOP) // input multiplex?
	AM_RANGE(0x16c004, 0x16c005) AM_WRITE(SMH_NOP)

	AM_RANGE(0x16e000, 0x16e001) AM_WRITE(SMH_NOP)

	AM_RANGE(0x170000, 0x173fff) AM_WRITE(SMH_RAM)
ADDRESS_MAP_END



static INPUT_PORTS_START( mirage )
	PORT_START_TAG("SYSTEM_IN")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0008, IP_ACTIVE_LOW )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Flip_Screen ) )
    PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
    PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START_TAG("MIRAGE0")
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_SERVICE1 ) /* Inputs start here???? */
    PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) ) /* Makes selections in "Test Mode" when changing from "Off" to "On" */
    PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

INPUT_PORTS_END


static const gfx_layout tile_8x8_layout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8,RGN_FRAC(1,2)+0,RGN_FRAC(0,2)+8,RGN_FRAC(0,2)+0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16
};

static const gfx_layout tile_16x16_layout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8,RGN_FRAC(1,2)+0,RGN_FRAC(0,2)+8,RGN_FRAC(0,2)+0 },
	{ 256,257,258,259,260,261,262,263,0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,8*16,9*16,10*16,11*16,12*16,13*16,14*16,15*16 },
	32*16
};

static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 24,8,16,0 },
	{ 512,513,514,515,516,517,518,519, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  8*32, 9*32,10*32,11*32,12*32,13*32,14*32,15*32},
	32*32
};

static GFXDECODE_START( mirage )
	GFXDECODE_ENTRY(REGION_GFX1, 0, tile_8x8_layout,     0x000, 32)	/* Tiles (8x8) */
	GFXDECODE_ENTRY(REGION_GFX1, 0, tile_16x16_layout,   0x000, 32)	/* Tiles (16x16) */
	GFXDECODE_ENTRY(REGION_GFX2, 0, spritelayout,        0x200, 32)	/* Sprites (16x16) */
GFXDECODE_END



static MACHINE_DRIVER_START( mirage )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 28000000/2)
	MDRV_CPU_PROGRAM_MAP(mirage_readmem,mirage_writemem)
	MDRV_CPU_VBLANK_INT("main", irq6_line_hold)

	/* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(58)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(529))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)

	MDRV_GFXDECODE(mirage)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(mirage)
	MDRV_VIDEO_UPDATE(mirage)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 1000000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1_pin7high) // clock frequency & pin 7 not verified
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD(OKIM6295, 1000000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1_pin7high) // clock frequency & pin 7 not verified
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END


ROM_START( mirage )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "mr_00-.2a", 0x00000, 0x40000, CRC(3a53f33d) SHA1(0f654021dcd64202b41e0ef5ef3cdf5dd274f8a5) )
	ROM_LOAD16_BYTE( "mr_01-.3a", 0x00001, 0x40000, CRC(a0b758aa) SHA1(7fb5faf6fb57cd72a3ac24b8af1f33e504ac8398) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE ) /* Tiles - Encrypted */
	ROM_LOAD( "mbl-00.7a", 0x000000, 0x100000, CRC(2e258b7b) SHA1(2dbd7d16a1eda97ae3de149b67e80e511aa9d0ba) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Sprites */
  	ROM_LOAD16_BYTE( "mbl-01.11a", 0x000001, 0x200000, CRC(895be69a) SHA1(541d8f37fb4cf99312b80a0eb0d729fbbeab5f4f) )
	ROM_LOAD16_BYTE( "mbl-02.12a", 0x000000, 0x200000, CRC(474f6104) SHA1(ff81b32b90192c3d5f27c436a9246aa6caaeeeee) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )	/* M6295 samples */
	ROM_LOAD( "mbl-03.10a", 0x000000, 0x200000, CRC(4a599703) SHA1(b49e84faa2d6acca952740d30fc8d1a33ac47e79) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 )	/* M6295 samples */
	ROM_LOAD( "mbl-04.12k", 0x000000, 0x100000, CRC(b533123d) SHA1(2cb2f11331d00c2d282113932ed2836805f4fc6e) )
ROM_END

static void descramble_sound( int region )
{
	UINT8 *rom = memory_region(region);
	int length = memory_region_length(region);
	UINT8 *buf1 = malloc_or_die(length);
	UINT32 x;

	for (x=0;x<length;x++)
	{
		UINT32 addr;

		addr = BITSWAP24 (x,23,22,21,0, 20,
		                    19,18,17,16,
		                    15,14,13,12,
		                    11,10,9, 8,
		                    7, 6, 5, 4,
		                    3, 2, 1 );

		buf1[addr] = rom[x];
	}

	memcpy(rom,buf1,length);

	free (buf1);

#if 0
	{
		FILE *fp;
		fp=fopen("sound.dmp", "w+b");
		if (fp)
		{
			fwrite(rom, length, 1, fp);
			fclose(fp);
		}
	}
#endif

}


static DRIVER_INIT( mirage )
{
	deco56_decrypt(REGION_GFX1);
	descramble_sound(REGION_SOUND1);
}

GAME( 1994, mirage, 0,        mirage, mirage, mirage, ROT0, "Mitchell", "Mirage Youjuu Mahjongden (Japan)", GAME_NOT_WORKING )
