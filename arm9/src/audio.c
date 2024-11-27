#include <nds.h>
#include <maxmod9.h>
#include <stdio.h>
#include "audio.h"
#include "soundbank.h"
#include "soundbank_bin.h"

mm_sound_effect STARTUP = {
	{ SFX_STARTUP } ,			// id
	(int)(1.0f * (1<<10)),	// rate
	0,		// handle
	255,	// volume
	127,	// panning
};
mm_sound_effect SELECT = {
	{ SFX_SELECT } ,			// id
	(int)(1.0f * (1<<10)),	// rate
	0,		// handle
	255,	// volume
	127,	// panning
};
mm_sound_effect BACK = {
	{ SFX_BACK } ,			// id
	(int)(1.0f * (1<<10)),	// rate
	0,		// handle
	255,	// volume
	127,	// panning
};
mm_sound_effect OK = {
	{ SFX_OK } ,			// id
	(int)(1.0f * (1<<10)),	// rate
	0,		// handle
	255,	// volume
	127,	// panning
};
mm_sound_effect NG = {
	{ SFX_NG } ,			// id
	(int)(1.0f * (1<<10)),	// rate
	0,		// handle
	255,	// volume
	127,	// panning
};
mm_sound_effect CHIME = {
	{ SFX_CHIME } ,			// id
	(int)(1.0f * (1<<10)),	// rate
	0,		// handle
	255,	// volume
	127,	// panning
};

void soundInit() {

	mmInitDefaultMem((mm_addr)soundbank_bin);
	mmLoadEffect( SFX_STARTUP );
	mmLoadEffect( SFX_SELECT );
	mmLoadEffect( SFX_BACK );
	mmLoadEffect( SFX_OK );
	mmLoadEffect( SFX_NG );
	
}

void soundPlayStartup() {
	mmEffectEx(&STARTUP);
}
void soundPlaySelect() {
	mmEffectEx(&SELECT);
}
void soundPlayBack() {
	mmEffectEx(&BACK);
}
void soundPlayPass() {
	mmEffectEx(&OK);
}
void soundPlayFail() {
	mmEffectEx(&NG);
}
void soundPlayChime() {
	mmEffectEx(&CHIME);
}