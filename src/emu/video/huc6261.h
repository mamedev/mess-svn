/**********************************************************************

    Hudson/NEC HuC6261 interface and definitions

**********************************************************************/


#ifndef __HUC6261_H_
#define __HUC6261_H_

#include "emu.h"
#include "machine/devhelpr.h"
#include "video/huc6270.h"


/* Screen timing stuff */
#define HUC6261_WPF			1365	/* width of a line in frame including blanking areas */
#define HUC6261_LPF			263		/* max number of lines in a single frame */


#define MCFG_HUC6261_ADD( _tag, clock, _intrf )	\
	MCFG_DEVICE_ADD( _tag, HUC6261, clock )		\
	MCFG_DEVICE_CONFIG( _intrf )


typedef struct _huc6261_interface huc6261_interface;
struct _huc6261_interface
{
	/* Tag for the screen we will be drawing on */
	const char *screen_tag;

	/* Tags for the 2 HuC6270 devices */
	const char *huc6270_a_tag;
	const char *huc6270_b_tag;
};


class huc6261_device :	public device_t,
						public huc6261_interface
{
public:
	// construction/destruction
	huc6261_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	void video_update(bitmap_rgb32 &bitmap, const rectangle &cliprect);
	DECLARE_READ16_MEMBER( read );
	DECLARE_WRITE16_MEMBER( write );

protected:
	// device-level overrides
	virtual void device_config_complete();
	virtual void device_start();
	virtual void device_reset();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);

private:
	screen_device *m_screen;
	huc6270_device *m_huc6270_a;
	huc6270_device *m_huc6270_b;
	int		m_last_h;
	int		m_last_v;
	int		m_height;

	UINT16	m_palette[512];
	UINT16	m_address;
	UINT16	m_palette_latch;
	UINT16	m_register;
	UINT16	m_control;
	UINT8	m_priority[7];

	UINT8	m_pixels_per_clock;	/* Number of pixels to output per colour clock */
	UINT16	m_pixel_data;
	UINT8	m_pixel_clock;

	emu_timer	*m_timer;
	bitmap_rgb32	*m_bmp;
};


extern const device_type HUC6261;


#endif
