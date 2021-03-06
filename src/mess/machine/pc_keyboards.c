
#include "emu.h"
#include "machine/pc_keyboards.h"
#include "machine/kb_keytro.h"
#include "machine/kb_msnat.h"

SLOT_INTERFACE_START(pc_xt_keyboards)
	SLOT_INTERFACE(STR_KBD_KEYTRONIC_PC3270, PC_KBD_KEYTRONIC_PC3270)
SLOT_INTERFACE_END


SLOT_INTERFACE_START(pc_at_keyboards)
	SLOT_INTERFACE(STR_KBD_KEYTRONIC_PC3270, PC_KBD_KEYTRONIC_PC3270_AT)
	SLOT_INTERFACE(STR_KBD_MICROSOFT_NATURAL, PC_KBD_MICROSOFT_NATURAL)
SLOT_INTERFACE_END

