/***************************************************************************

    Jugend+Technik CompJU+TEr

    15/07/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "includes/jtc.h"
#include "cpu/z8/z8.h"
#include "devices/cassette.h"
#include "machine/ctronics.h"
#include "sound/speaker.h"
#include "sound/wave.h"
#include "devices/messram.h"

/* Read/Write Handlers */

static WRITE8_HANDLER( p2_w )
{
	/*

        bit     description

        P20
        P21
        P22
        P23
        P24
        P25     centronics strobe output
        P26     V4093 pins 1,2
        P27     DL299 pin 18

    */

	jtc_state *state = space->machine->driver_data;

	centronics_strobe_w(state->centronics, BIT(data, 5));
}

static READ8_HANDLER( p3_r )
{
	/*

        bit     description

        P30     tape input
        P31
        P32
        P33     centronics busy input
        P34
        P35
        P36     tape output
        P37     speaker output

    */

	jtc_state *state = space->machine->driver_data;

	UINT8 data = 0;

	data |= (cassette_input(state->cassette) < 0.0) ? 1 : 0;
	data |= centronics_busy_r(state->centronics) << 3;

	return data;
}

static WRITE8_HANDLER( p3_w )
{
	/*

        bit     description

        P30     tape input
        P31
        P32
        P33     centronics busy input
        P34
        P35
        P36     tape output
        P37     speaker output

    */

	jtc_state *state = space->machine->driver_data;

	/* tape */
	cassette_output(state->cassette, BIT(data, 6) ? +1.0 : -1.0);

	/* speaker */
	speaker_level_w(state->speaker, BIT(data, 7));
}

static READ8_HANDLER( es40_videoram_r )
{
	jtc_state *state = space->machine->driver_data;

	UINT8 data = 0;

	if (state->video_bank & 0x80) data |= state->color_ram_r[offset];
	if (state->video_bank & 0x40) data |= state->color_ram_g[offset];
	if (state->video_bank & 0x20) data |= state->color_ram_b[offset];
	if (state->video_bank & 0x10) data |= state->video_ram[offset];

	return data;
}

static WRITE8_HANDLER( es40_videoram_w )
{
	jtc_state *state = space->machine->driver_data;

	if (state->video_bank & 0x80) state->color_ram_r[offset] = data;
	if (state->video_bank & 0x40) state->color_ram_g[offset] = data;
	if (state->video_bank & 0x20) state->color_ram_b[offset] = data;
	if (state->video_bank & 0x10) state->video_ram[offset] = data;
}

static WRITE8_HANDLER( es40_banksel_w )
{
	jtc_state *state = space->machine->driver_data;

	state->video_bank = offset & 0xf0;
}

/* Memory Maps */

static ADDRESS_MAP_START( jtc_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x7001, 0x7001) AM_MIRROR(0x0ff0) AM_READ_PORT("Y1")
	AM_RANGE(0x7002, 0x7002) AM_MIRROR(0x0ff0) AM_READ_PORT("Y2")
	AM_RANGE(0x7003, 0x7003) AM_MIRROR(0x0ff0) AM_READ_PORT("Y3")
	AM_RANGE(0x7004, 0x7004) AM_MIRROR(0x0ff0) AM_READ_PORT("Y4")
	AM_RANGE(0x7005, 0x7005) AM_MIRROR(0x0ff0) AM_READ_PORT("Y5")
	AM_RANGE(0x7006, 0x7006) AM_MIRROR(0x0ff0) AM_READ_PORT("Y6")
	AM_RANGE(0x7007, 0x7007) AM_MIRROR(0x0ff0) AM_READ_PORT("Y7")
	AM_RANGE(0x7008, 0x7008) AM_MIRROR(0x0ff0) AM_READ_PORT("Y8")
	AM_RANGE(0x7009, 0x7009) AM_MIRROR(0x0ff0) AM_READ_PORT("Y9")
	AM_RANGE(0x700a, 0x700a) AM_MIRROR(0x0ff0) AM_READ_PORT("Y10")
	AM_RANGE(0x700b, 0x700b) AM_MIRROR(0x0ff0) AM_READ_PORT("Y11")
	AM_RANGE(0x700c, 0x700c) AM_MIRROR(0x0ff0) AM_READ_PORT("Y12")
	AM_RANGE(0xe000, 0xfdff) AM_RAM
	AM_RANGE(0xfe00, 0xffff) AM_RAM AM_BASE_MEMBER(jtc_state, video_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( jtc_es1988_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0800, 0x0fff) AM_ROM
	AM_RANGE(0x2000, 0x27ff) AM_ROM
	AM_RANGE(0x7001, 0x7001) AM_MIRROR(0x0ff0) AM_READ_PORT("Y1")
	AM_RANGE(0x7002, 0x7002) AM_MIRROR(0x0ff0) AM_READ_PORT("Y2")
	AM_RANGE(0x7003, 0x7003) AM_MIRROR(0x0ff0) AM_READ_PORT("Y3")
	AM_RANGE(0x7004, 0x7004) AM_MIRROR(0x0ff0) AM_READ_PORT("Y4")
	AM_RANGE(0x7005, 0x7005) AM_MIRROR(0x0ff0) AM_READ_PORT("Y5")
	AM_RANGE(0x7006, 0x7006) AM_MIRROR(0x0ff0) AM_READ_PORT("Y6")
	AM_RANGE(0x7007, 0x7007) AM_MIRROR(0x0ff0) AM_READ_PORT("Y7")
	AM_RANGE(0x7008, 0x7008) AM_MIRROR(0x0ff0) AM_READ_PORT("Y8")
	AM_RANGE(0x7009, 0x7009) AM_MIRROR(0x0ff0) AM_READ_PORT("Y9")
	AM_RANGE(0x700a, 0x700a) AM_MIRROR(0x0ff0) AM_READ_PORT("Y10")
	AM_RANGE(0x700b, 0x700b) AM_MIRROR(0x0ff0) AM_READ_PORT("Y11")
	AM_RANGE(0x700c, 0x700c) AM_MIRROR(0x0ff0) AM_READ_PORT("Y12")
	AM_RANGE(0xe000, 0xfdff) AM_RAM
	AM_RANGE(0xfe00, 0xffff) AM_RAM AM_BASE_MEMBER(jtc_state, video_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( jtc_es23_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0800, 0x17ff) AM_ROM
	AM_RANGE(0x7000, 0x7000) AM_MIRROR(0x0ff0) AM_READ_PORT("Y0")
	AM_RANGE(0x7001, 0x7001) AM_MIRROR(0x0ff0) AM_READ_PORT("Y1")
	AM_RANGE(0x7002, 0x7002) AM_MIRROR(0x0ff0) AM_READ_PORT("Y2")
	AM_RANGE(0x7003, 0x7003) AM_MIRROR(0x0ff0) AM_READ_PORT("Y3")
	AM_RANGE(0x7004, 0x7004) AM_MIRROR(0x0ff0) AM_READ_PORT("Y4")
	AM_RANGE(0x7005, 0x7005) AM_MIRROR(0x0ff0) AM_READ_PORT("Y5")
	AM_RANGE(0x7006, 0x7006) AM_MIRROR(0x0ff0) AM_READ_PORT("Y6")
	AM_RANGE(0x7007, 0x7007) AM_MIRROR(0x0ff0) AM_READ_PORT("Y7")
	AM_RANGE(0x7008, 0x7008) AM_MIRROR(0x0ff0) AM_READ_PORT("Y8")
	AM_RANGE(0x7009, 0x7009) AM_MIRROR(0x0ff0) AM_READ_PORT("Y9")
	AM_RANGE(0x700a, 0x700a) AM_MIRROR(0x0ff0) AM_READ_PORT("Y10")
	AM_RANGE(0x700b, 0x700b) AM_MIRROR(0x0ff0) AM_READ_PORT("Y11")
	AM_RANGE(0x700c, 0x700c) AM_MIRROR(0x0ff0) AM_READ_PORT("Y12")
	AM_RANGE(0x700d, 0x700d) AM_MIRROR(0x0ff0) AM_READ_PORT("Y13")
	AM_RANGE(0x700e, 0x700e) AM_MIRROR(0x0ff0) AM_READ_PORT("Y14")
	AM_RANGE(0x700f, 0x700f) AM_MIRROR(0x0ff0) AM_READ_PORT("Y15")
	AM_RANGE(0xe000, 0xfdff) AM_RAM
	AM_RANGE(0xf800, 0xffff) AM_RAM AM_BASE_MEMBER(jtc_state, video_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( jtc_es40_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0800, 0x1fff) AM_ROM
	AM_RANGE(0x4000, 0x5fff) AM_READWRITE(es40_videoram_r, es40_videoram_w)
	AM_RANGE(0x6000, 0x63ff) AM_WRITE(es40_banksel_w)
	AM_RANGE(0x7001, 0x7001) AM_MIRROR(0x0ff0) AM_READ_PORT("Y1")
	AM_RANGE(0x7002, 0x7002) AM_MIRROR(0x0ff0) AM_READ_PORT("Y2")
	AM_RANGE(0x7003, 0x7003) AM_MIRROR(0x0ff0) AM_READ_PORT("Y3")
	AM_RANGE(0x7004, 0x7004) AM_MIRROR(0x0ff0) AM_READ_PORT("Y4")
	AM_RANGE(0x7005, 0x7005) AM_MIRROR(0x0ff0) AM_READ_PORT("Y5")
	AM_RANGE(0x7006, 0x7006) AM_MIRROR(0x0ff0) AM_READ_PORT("Y6")
	AM_RANGE(0x7007, 0x7007) AM_MIRROR(0x0ff0) AM_READ_PORT("Y7")
	AM_RANGE(0x7008, 0x7008) AM_MIRROR(0x0ff0) AM_READ_PORT("Y8")
	AM_RANGE(0x7009, 0x7009) AM_MIRROR(0x0ff0) AM_READ_PORT("Y9")
	AM_RANGE(0x700a, 0x700a) AM_MIRROR(0x0ff0) AM_READ_PORT("Y10")
	AM_RANGE(0x700b, 0x700b) AM_MIRROR(0x0ff0) AM_READ_PORT("Y11")
	AM_RANGE(0x700c, 0x700c) AM_MIRROR(0x0ff0) AM_READ_PORT("Y12")
	AM_RANGE(0x8000, 0xffff) AM_RAM//BANK(1)
ADDRESS_MAP_END

static ADDRESS_MAP_START( jtc_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00, 0x00) AM_NOP // A8-A15
	AM_RANGE(0x01, 0x01) AM_NOP // AD0-AD7
	AM_RANGE(0x02, 0x02) AM_WRITE(p2_w)
	AM_RANGE(0x03, 0x03) AM_READWRITE(p3_r, p3_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( jtc )
	PORT_START("Y1")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91 \xE2\x86\x93") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP)) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90 \xE2\x86\x92") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT)) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')

	PORT_START("Y2")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Y')

	PORT_START("Y3")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')

	PORT_START("Y4")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')

	PORT_START("Y5")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('?')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')

	PORT_START("Y6")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('*')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')

	PORT_START("Y7")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('%')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Z')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')

	PORT_START("Y8")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR(0x2030) // per mille
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')

	PORT_START("Y9")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')

	PORT_START("Y10")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')

	PORT_START("Y11")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR('*') PORT_CHAR(':')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')

	PORT_START("Y12")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CLR") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('+') PORT_CHAR(';')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
INPUT_PORTS_END

static INPUT_PORTS_START( jtces23 )
	PORT_START("Y0")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CONTROL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)

	PORT_START("Y1")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Y')

	PORT_START("Y2")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')

	PORT_START("Y3")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')

	PORT_START("Y4")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')

	PORT_START("Y5")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')

	PORT_START("Y6")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Z')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')

	PORT_START("Y7")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')

	PORT_START("Y8")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('@')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('[')

	PORT_START("Y9")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR(']')

	PORT_START("Y10")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')

	PORT_START("Y11")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('<')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('+') PORT_CHAR('\\')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RET") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)

	PORT_START("Y12")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('>')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) //PORT_CODE() PORT_CHAR('=')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) //PORT_CODE() PORT_CHAR('*') PORT_CHAR('^')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')

	PORT_START("Y13")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) // DBS
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) // DEL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) // SPA

	PORT_START("Y14")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) // INS
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) // HOM
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))

	PORT_START("Y15")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) // CLS
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) // SOL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) // RET
INPUT_PORTS_END

static INPUT_PORTS_START( jtces40 )
	PORT_START("Y1")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHT3") PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_MAMEKEY(LALT))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHT2") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHT1") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)

	PORT_START("Y2")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('y') PORT_CHAR('Y')

	PORT_START("Y3")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')

	PORT_START("Y4")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')

	PORT_START("Y5")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')

	PORT_START("Y6")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')

	PORT_START("Y7")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')

	PORT_START("Y8")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')

	PORT_START("Y9")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('@')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('[') PORT_CHAR('{')

	PORT_START("Y10")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR(']') PORT_CHAR('}')

	PORT_START("Y11")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ') PORT_CHAR('=') PORT_CHAR('_')

	PORT_START("Y12")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CLR") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('+') PORT_CHAR('-') PORT_CHAR('\\')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('*') PORT_CHAR('/') PORT_CHAR('|')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RET") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13) PORT_CHAR('?') PORT_CHAR('~')
INPUT_PORTS_END

/* Video */

static VIDEO_START( jtc )
{
}

static VIDEO_UPDATE( jtc )
{
	jtc_state *state = screen->machine->driver_data;

	int x, y, sx;

	for (y = 0; y < 64; y++)
	{
		for (sx = 0; sx < 8; sx++)
		{
			UINT8 data = state->video_ram[(y * 8) + sx];

			for (x = 0; x < 8; x++)
			{
				int color = BIT(data, x);
				*BITMAP_ADDR16(bitmap, y, (sx * 8) + x) = color;
			}
		}
	}

    return 0;
}

static VIDEO_START( jtc_es23 )
{
}

static VIDEO_UPDATE( jtc_es23 )
{
	jtc_state *state = screen->machine->driver_data;

	int x, y, sx;

	for (y = 0; y < 128; y++)
	{
		for (sx = 0; sx < 16; sx++)
		{
			UINT8 data = state->video_ram[(y * 16) + sx];

			for (x = 0; x < 8; x++)
			{
				int color = BIT(data, x);
				*BITMAP_ADDR16(bitmap, y, (sx * 8) + x) = color;
			}
		}
	}

    return 0;
}

static PALETTE_INIT( jtc_es40 )
{
}

static VIDEO_START( jtc_es40 )
{
	jtc_state *state = machine->driver_data;

	/* allocate memory */
	state->video_ram = auto_alloc_array(machine, UINT8, JTC_ES40_VIDEORAM_SIZE);
	state->color_ram_r = auto_alloc_array(machine, UINT8, JTC_ES40_VIDEORAM_SIZE);
	state->color_ram_g = auto_alloc_array(machine, UINT8, JTC_ES40_VIDEORAM_SIZE);
	state->color_ram_b = auto_alloc_array(machine, UINT8, JTC_ES40_VIDEORAM_SIZE);

	/* register for state saving */
	state_save_register_global(machine, state->video_bank);
	state_save_register_global_pointer(machine, state->video_ram, JTC_ES40_VIDEORAM_SIZE);
	state_save_register_global_pointer(machine, state->color_ram_r, JTC_ES40_VIDEORAM_SIZE);
	state_save_register_global_pointer(machine, state->color_ram_g, JTC_ES40_VIDEORAM_SIZE);
	state_save_register_global_pointer(machine, state->color_ram_b, JTC_ES40_VIDEORAM_SIZE);
}

static VIDEO_UPDATE( jtc_es40 )
{
	jtc_state *state = screen->machine->driver_data;

	int x, y, sx;

	for (y = 0; y < 192; y++)
	{
		for (sx = 0; sx < 40; sx++)
		{
			UINT8 data = state->video_ram[(y * 40) + sx];
			UINT8 color_r = state->color_ram_r[(y * 40) + sx];
			UINT8 color_g = state->color_ram_g[(y * 40) + sx];
			UINT8 color_b = state->color_ram_b[(y * 40) + sx];

			for (x = 0; x < 8; x++)
			{
				int color = (BIT(color_r, x) << 3) | (BIT(color_g, x) << 2) | (BIT(color_b, x) << 1) | BIT(data, x);

				*BITMAP_ADDR16(bitmap, y, (sx * 8) + x) = color;
			}
		}
	}

    return 0;
}

/* Machine Initialization */

static MACHINE_START( jtc )
{
	jtc_state *state = machine->driver_data;

	/* find devices */
	state->cassette = devtag_get_device(machine, CASSETTE_TAG);
	state->speaker = devtag_get_device(machine, SPEAKER_TAG);
	state->centronics = devtag_get_device(machine, CENTRONICS_TAG);

	/* register for state saving */
	//state_save_register_global(machine, state->);
}

/* Machine Driver */

static const cassette_config jtc_cassette_config =
{
	cassette_default_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_ENABLED
};

/* F4 Character Displayer */
static const gfx_layout jtces23_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	64,					/* 128 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static const gfx_layout jtces40_charlayout =
{
	8, 8,					/* 8 x 16 characters */
	128,					/* 128 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static GFXDECODE_START( jtces23 )
	GFXDECODE_ENTRY( UB8830D_TAG, 0x1000, jtces23_charlayout, 0, 1 )
GFXDECODE_END

static GFXDECODE_START( jtces40 )
	GFXDECODE_ENTRY( UB8830D_TAG, 0x1000, jtces40_charlayout, 0, 8 )
GFXDECODE_END

static MACHINE_DRIVER_START( basic )
	MDRV_DRIVER_DATA(jtc_state)

	/* basic machine hardware */
    MDRV_CPU_ADD(UB8830D_TAG, UB8830D, XTAL_8MHz)
    MDRV_CPU_PROGRAM_MAP(jtc_mem)
    MDRV_CPU_IO_MAP(jtc_io)

    MDRV_MACHINE_START(jtc)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SPEAKER_TAG, SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_WAVE_ADD("wave", CASSETTE_TAG)
	MDRV_SOUND_ROUTE(1, "mono", 0.25)

	/* cassette */
	MDRV_CASSETTE_ADD(CASSETTE_TAG, jtc_cassette_config)

	/* printer */
	MDRV_CENTRONICS_ADD(CENTRONICS_TAG, standard_centronics)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( jtc )
	MDRV_IMPORT_FROM(basic)

    /* video hardware */
    MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(64, 64)
    MDRV_SCREEN_VISIBLE_AREA(0, 64-1, 0, 64-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(jtc)
    MDRV_VIDEO_UPDATE(jtc)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("2K")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( jtces88 )
	MDRV_IMPORT_FROM(jtc)

    /* basic machine hardware */
    MDRV_CPU_MODIFY(UB8830D_TAG)
    MDRV_CPU_PROGRAM_MAP(jtc_es1988_mem)

	/* internal ram */
	MDRV_RAM_MODIFY("messram")
	MDRV_RAM_DEFAULT_SIZE("4K")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( jtces23 )
	MDRV_IMPORT_FROM(basic)

    /* basic machine hardware */
    MDRV_CPU_MODIFY(UB8830D_TAG)
    MDRV_CPU_PROGRAM_MAP(jtc_es23_mem)

    /* video hardware */
    MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(128, 128)
    MDRV_SCREEN_VISIBLE_AREA(0, 128-1, 0, 128-1)
	MDRV_GFXDECODE(jtces23)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(jtc_es23)
    MDRV_VIDEO_UPDATE(jtc_es23)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("4K")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( jtces40 )
	MDRV_IMPORT_FROM(basic)

    /* basic machine hardware */
    MDRV_CPU_MODIFY(UB8830D_TAG)
    MDRV_CPU_PROGRAM_MAP(jtc_es40_mem)

    /* video hardware */
    MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(320, 192)
    MDRV_SCREEN_VISIBLE_AREA(0, 320-1, 0, 192-1)
	MDRV_GFXDECODE(jtces40)
    MDRV_PALETTE_LENGTH(16)
    MDRV_PALETTE_INIT(jtc_es40)

    MDRV_VIDEO_START(jtc_es40)
    MDRV_VIDEO_UPDATE(jtc_es40)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("8K")
	MDRV_RAM_EXTRA_OPTIONS("16K,32K")
MACHINE_DRIVER_END

/* ROMs */

ROM_START( jtc )
	ROM_REGION( 0x10000, UB8830D_TAG, 0 )
  	ROM_LOAD( "u883rom.bin", 0x0000, 0x0800, CRC(2453c8c1) SHA1(816f5d08f8064b69b1779eb6661fde091aa58ba8) )
	ROM_LOAD( "u2716c1.bin", 0x0800, 0x0800, NO_DUMP )
	ROM_LOAD( "u2716c2.bin", 0x2000, 0x0800, NO_DUMP )
ROM_END

ROM_START( jtces88 )
	ROM_REGION( 0x10000, UB8830D_TAG, 0 )
  	ROM_LOAD( "u883rom.bin", 0x0000, 0x0800, CRC(2453c8c1) SHA1(816f5d08f8064b69b1779eb6661fde091aa58ba8) )
  	ROM_LOAD( "es1988_0800.bin", 0x0800, 0x0800, CRC(af3e882f) SHA1(65af0d0f5f882230221e9552707d93ed32ba794d) )
  	ROM_LOAD( "es1988_2000.bin", 0x2000, 0x0800, CRC(5ff87c1e) SHA1(fbd2793127048bd9706970b7bce84af2cb258dc5) )
ROM_END

ROM_START( jtces23 )
	ROM_REGION( 0x10000, UB8830D_TAG, 0 )
  	ROM_LOAD( "u883rom.bin", 0x0000, 0x0800, CRC(2453c8c1) SHA1(816f5d08f8064b69b1779eb6661fde091aa58ba8) )
  	ROM_LOAD( "es23_0800.bin", 0x0800, 0x1000, CRC(16128b64) SHA1(90fb0deeb5660f4a2bb38d51981cc6223d5ddf6b) )
ROM_END

ROM_START( jtces40 )
	ROM_REGION( 0x10000, UB8830D_TAG, 0 )
  	ROM_LOAD( "u883rom.bin", 0x0000, 0x0800, CRC(2453c8c1) SHA1(816f5d08f8064b69b1779eb6661fde091aa58ba8) )
  	ROM_LOAD( "es40_0800.bin", 0x0800, 0x1800, CRC(770c87ce) SHA1(1a5227ba15917f2a572cb6c27642c456f5b32b90) )
ROM_END

/* System Drivers */

/*    YEAR  NAME        PARENT  COMPAT  MACHINE INPUT   INIT    COMPANY                 FULLNAME                    FLAGS */
COMP( 1987, jtc,	0,       0, 	jtc, 	jtc, 	 0,		"Jugend+Technik",   "CompJU+TEr",					GAME_NOT_WORKING )
COMP( 1988, jtces88,	jtc,     0, 	jtces88,jtc, 	 0,		"Jugend+Technik",   "CompJU+TEr (EMR-ES 1988)",	GAME_NOT_WORKING )
COMP( 1989, jtces23,	jtc,     0, 	jtces23,jtces23, 0,		"Jugend+Technik",   "CompJU+TEr (ES 2.3)",		GAME_NOT_WORKING )
COMP( 1990, jtces40,	jtc,     0, 	jtces40,jtces40, 0,		"Jugend+Technik",   "CompJU+TEr (ES 4.0)",		GAME_NOT_WORKING )
