/**********************************************************************

    Commodore 1581/1563 Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __C1581__
#define __C1581__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "imagedev/flopdrv.h"
#include "formats/d81_dsk.h"
#include "machine/6526cia.h"
#include "machine/cbmiec.h"
#include "machine/wd17xx.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define C1581_TAG			"c1581"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_C1581_ADD(_tag, _address) \
    MCFG_DEVICE_ADD(_tag, C1581, 0) \
	c1581_device_config::static_set_config(device, _address, "c1581");

#define MCFG_C1563_ADD(_tag, _address) \
    MCFG_DEVICE_ADD(_tag, C1563, 0) \
	c1581_device_config::static_set_config(device, _address, "c1563");



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> c1581_device_config

class c1581_device_config :   public device_config,
							  public device_config_cbm_iec_interface
{
    friend class c1581_device;
    friend class c1563_device;

    // construction/destruction
    c1581_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
    // allocators
    static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
    virtual device_t *alloc_device(running_machine &machine) const;

	// inline configuration helpers
	static void static_set_config(device_config *device, int address, const char *rom_region);

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;

protected:
    // device_config overrides
    virtual void device_config_complete();

private:
	int m_address;
	const char *m_rom_region;
};


// ======================> c1581_device

class c1581_device :  public device_t,
					  public device_cbm_iec_interface
{
    friend class c1581_device_config;

    // construction/destruction
    c1581_device(running_machine &_machine, const c1581_device_config &_config);

public:
	// not really public
	DECLARE_WRITE_LINE_MEMBER( cnt_w );
	DECLARE_WRITE_LINE_MEMBER( sp_w );
	DECLARE_READ8_MEMBER( cia_pa_r );
	DECLARE_WRITE8_MEMBER( cia_pa_w );
	DECLARE_READ8_MEMBER( cia_pb_r );
	DECLARE_WRITE8_MEMBER( cia_pb_w );

protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_reset();

	// device_cbm_iec_interface overrides
	void cbm_iec_srq(int state);
	void cbm_iec_atn(int state);
	void cbm_iec_data(int state);
	void cbm_iec_reset(int state);

private:
	inline void set_iec_data();
	inline void set_iec_srq();

	required_device<cpu_device> m_maincpu;
	required_device<mos6526_device> m_cia;
	required_device<device_t> m_fdc;
	required_device<device_t> m_image;
	required_device<cbm_iec_device> m_bus;

	int m_data_out;				// serial data out
	int m_atn_ack;				// attention acknowledge
	int m_fast_ser_dir;			// fast serial direction
	int m_sp_out;				// fast serial data out
	int m_cnt_out;				// fast serial clock out

    const c1581_device_config &m_config;
};


// ======================> c1563_device

class c1563_device :  public c1581_device
{
    friend class c1581_device_config;

    // construction/destruction
    c1563_device(running_machine &_machine, const c1581_device_config &_config);
};


// device type definition
extern const device_type C1581;
extern const device_type C1563;



#endif
