/****************************************************************************

    TI-99/4(A) and /8 Video subsystem
    See videowrp.c for documentation

    Michael Zapf
    October 2010
    January 2012: Rewritten as class

*****************************************************************************/

#ifndef __TIVIDEO__
#define __TIVIDEO__

#include "video/tms9928a.h"
#include "video/v9938.h"
#include "ti99defs.h"
#include "sound/sn76496.h"

class ti_video_device : public bus8z_device
{
public:
	virtual void	reset_vdp(int state) =0;

protected:
	address_space	*m_space;
	tms9928a_device *m_tms9928a;

	/* Constructor */
	ti_video_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock);
	virtual void device_start(void);
	virtual void device_reset(void);
	virtual DECLARE_READ8Z_MEMBER(readz) { };
	virtual DECLARE_WRITE8_MEMBER(write) { };
};

/*
    Used in the TI-99/4A and TI-99/8
*/
class ti_std_video_device : public ti_video_device
{
public:
	ti_std_video_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	DECLARE_READ8Z_MEMBER(readz);
	DECLARE_WRITE8_MEMBER(write);

	void	reset_vdp(int state) { m_tms9928a->reset_line(state); }
};

/*
    Used in the EVPC and Geneve
*/
class ti_exp_video_device : public ti_video_device
{
public:
	ti_exp_video_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	void video_update_mouse(int delta_x, int delta_y, int buttons);
	DECLARE_READ8Z_MEMBER(readz);
	DECLARE_WRITE8_MEMBER(write);
	DECLARE_READ16_MEMBER(read16);
	DECLARE_WRITE16_MEMBER(write16);
	void	reset_vdp(int state) { m_v9938->reset_line(state); }

protected:
	void			device_start(void);
	v9938_device	*m_v9938;
};

extern const device_type TI99VIDEO;
extern const device_type V9938VIDEO;

/****************************************************************************/
/*
    Sound device
*/

extern const device_type TISOUND_94624;
extern const device_type TISOUND_76496;

typedef struct _ti_sound_config
{
	devcb_write_line				ready;
} ti_sound_config;

#define TI_SOUND_CONFIG(name) \
	const ti_sound_config(name) =

class ti_sound_system_device : public bus8z_device
{
public:
	ti_sound_system_device(const machine_config &mconfig, device_type type, const char *tag, const char *name, device_t *owner, UINT32 clock)
	: bus8z_device(mconfig, type, name, tag, owner, clock) { };

	// Cannot read from sound; just ignore silently
	DECLARE_READ8Z_MEMBER(readz) { };
	DECLARE_WRITE8_MEMBER(write);
	DECLARE_WRITE_LINE_MEMBER( sound_ready );	// connect to console READY

protected:
	void device_start(void);
	virtual machine_config_constructor device_mconfig_additions() const =0;

private:
	sn76496_base_device*		m_sound_chip;
	devcb_resolved_write_line	m_console_ready;
};

/*
    The version that sits in the TI-99/4A
*/
class ti_sound_sn94624_device : public ti_sound_system_device
{
public:
	ti_sound_sn94624_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: ti_sound_system_device(mconfig, TISOUND_94624, tag, "Onboard sound (SN94624)", owner, clock) { }

protected:
	machine_config_constructor device_mconfig_additions() const;
};

/*
    The version that sits in the TI-99/8 and Geneve
*/
class ti_sound_sn76496_device : public ti_sound_system_device
{
public:
	ti_sound_sn76496_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: ti_sound_system_device(mconfig, TISOUND_76496, tag, "Onboard sound (SN76496)", owner, clock) { }

protected:
	machine_config_constructor device_mconfig_additions() const;
};


/****************************************************************************/

#define MCFG_TI_TMS991x_ADD_NTSC(_tag, _chip, _tmsparam)	\
	MCFG_DEVICE_ADD(_tag, TI99VIDEO, 0)										\
	MCFG_TMS9928A_ADD( VDP_TAG, _chip, _tmsparam )					\
	MCFG_TMS9928A_SCREEN_ADD_NTSC( SCREEN_TAG ) 							\
	MCFG_SCREEN_UPDATE_DEVICE( VDP_TAG, tms9928a_device, screen_update )

#define MCFG_TI_TMS991x_ADD_PAL(_tag, _chip, _tmsparam)		\
	MCFG_DEVICE_ADD(_tag, TI99VIDEO, 0)										\
	MCFG_TMS9928A_ADD( VDP_TAG, _chip, _tmsparam )						\
	MCFG_TMS9928A_SCREEN_ADD_PAL( SCREEN_TAG )								\
	MCFG_SCREEN_UPDATE_DEVICE( VDP_TAG, tms9928a_device, screen_update )

#define MCFG_TI998_ADD_NTSC(_tag, _chip, _tmsparam)	\
	MCFG_DEVICE_ADD(_tag, TI99VIDEO, 0)										\
	MCFG_TMS9928A_ADD( VDP_TAG, _chip, _tmsparam )					\
	MCFG_TMS9928A_SCREEN_ADD_NTSC( SCREEN_TAG ) 							\
	MCFG_SCREEN_UPDATE_DEVICE( VDP_TAG, tms9928a_device, screen_update )

#define MCFG_TI998_ADD_PAL(_tag, _chip, _tmsparam)		\
	MCFG_DEVICE_ADD(_tag, TI99VIDEO, 0)										\
	MCFG_TMS9928A_ADD( VDP_TAG, _chip, _tmsparam )						\
	MCFG_TMS9928A_SCREEN_ADD_PAL( SCREEN_TAG )								\
	MCFG_SCREEN_UPDATE_DEVICE( VDP_TAG, tms9928a_device, screen_update )

#define MCFG_TI_V9938_ADD(_tag, _rate, _screen, _blank, _x, _y, _devtag, _class, _int)		\
	MCFG_DEVICE_ADD(_tag, V9938VIDEO, 0)										\
	MCFG_V9938_ADD(VDP_TAG, _screen, 0x20000)							\
	MCFG_V99X8_INTERRUPT_CALLBACK_DEVICE(_devtag, _class, _int)			\
	MCFG_SCREEN_ADD(_screen, RASTER)										\
	MCFG_SCREEN_REFRESH_RATE(_rate)											\
	MCFG_SCREEN_UPDATE_DEVICE(VDP_TAG, v9938_device, screen_update)	\
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(_blank))					\
	MCFG_SCREEN_SIZE(_x, _y)												\
	MCFG_SCREEN_VISIBLE_AREA(0, _x - 1, 0, _y - 1)							\
	MCFG_PALETTE_LENGTH(512)												\
	MCFG_PALETTE_INIT(v9938)

#define MCFG_TI_SOUND_94624_ADD(_tag, _conf)			\
	MCFG_DEVICE_ADD(_tag, TISOUND_94624, 0)	\
	MCFG_DEVICE_CONFIG( _conf )

#define MCFG_TI_SOUND_76496_ADD(_tag, _conf)			\
	MCFG_DEVICE_ADD(_tag, TISOUND_76496, 0)	\
	MCFG_DEVICE_CONFIG( _conf )

#endif /* __TIVIDEO__ */

