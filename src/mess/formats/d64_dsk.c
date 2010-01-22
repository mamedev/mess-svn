/*

 Commodore GCR format 

Original    Encoded
4 bits      5 bits

0000    ->  01010 = 0x0a
0001    ->  01011 = 0x0b
0010    ->  10010 = 0x12
0011    ->  10011 = 0x13
0100    ->  01110 = 0x0e
0101    ->  01111 = 0x0f
0110    ->  10110 = 0x16
0111    ->  10111 = 0x17
1000    ->  01001 = 0x09
1001    ->  11001 = 0x19
1010    ->  11010 = 0x1a
1011    ->  11011 = 0x1b
1100    ->  01101 = 0x0d
1101    ->  11101 = 0x1d
1110    ->  11110 = 0x1e
1111    ->  10101 = 0x15

We use the encoded values in bytes because we use them to encode
groups of 4 bytes into groups of 5 bytes, below.

*/

/*

    TODO:

    - disk errors
	- variable gaps

*/

#include "emu.h"
#include "formats/flopimg.h"
#include "formats/d64_dsk.h"
#include "devices/flopdrv.h"

#define LOG 1

#define MAX_HEADS			2
#define MAX_TRACKS			84
#define MAX_ERROR_SECTORS	802
#define SECTOR_SIZE			256
#define SECTOR_SIZE_GCR		368

#define INVALID_OFFSET		0xbadbad

enum
{
	DOS1,
	DOS2,
	DOS27
};

enum
{
	ERROR_00 = 1,
	ERROR_20,		/* header block not found */
	ERROR_21,		/* no sync character */
	ERROR_22,		/* data block not present */
	ERROR_23,		/* checksum error in data block */
	ERROR_24,		/* write verify (on format) */
	ERROR_25,		/* write verify error */
	ERROR_26,		/* write protect on */
	ERROR_27,		/* checksum error in header block */
	ERROR_28,		/* write error */
	ERROR_29,		/* disk ID mismatch */
	ERROR_74,		/* disk not ready (no device 1) */
};

#define D64_SIZE_35_TRACKS				 174848
#define D64_SIZE_35_TRACKS_WITH_ERRORS	 175531
#define D64_SIZE_40_TRACKS				 196608
#define D64_SIZE_40_TRACKS_WITH_ERRORS	 197376
#define D64_SIZE_42_TRACKS				 205312
#define D64_SIZE_42_TRACKS_WITH_ERRORS	 206114
#define D67_SIZE_35_TRACKS				 176640
#define D71_SIZE_70_TRACKS				 349696
#define D71_SIZE_70_TRACKS_WITH_ERRORS	 351062
#define D80_SIZE_77_TRACKS				 533248
#define D82_SIZE_154_TRACKS				1066496

static const int DOS1_SECTORS_PER_TRACK[] =
{
	21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
	20, 20, 20, 20, 20, 20, 20,
	18, 18, 18, 18, 18, 18,
	17, 17, 17, 17, 17,
	17, 17, 17, 17, 17,
	17, 17
};

static const int DOS1_SPEED_ZONE[] =
{
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	2, 2, 2, 2, 2, 2, 2,
	1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0
};

static const int DOS2_SECTORS_PER_TRACK[] =
{
	21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
	19, 19, 19, 19, 19, 19, 19,
	18, 18, 18, 18, 18, 18,
	17, 17, 17, 17, 17,
	17, 17, 17, 17, 17,
	17, 17
};

static const int DOS27_SECTORS_PER_TRACK[] =
{
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,		/* 1-39 */
	27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,							/* 40-53 */
	25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,										/* 54-64 */
	23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,								/* 65-77 */
	23, 23, 23, 23, 23, 23, 23														/* 78-84 */
};

static const int DOS27_SPEED_ZONE[] =
{
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,	/* 1-39 */
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,					/* 40-53 */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,							/* 54-64 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,						/* 65-77 */
	0, 0, 0, 0, 0, 0, 0											/* 78-84 */
};

struct d64dsk_tag
{
	int dos;									/* CBM DOS version */
	int heads;									/* number of physical heads */
	int tracks;									/* number of physical tracks */
	int dos_tracks;								/* number of logical tracks */
	int track_offset[MAX_HEADS][MAX_TRACKS];	/* offset within image for each physical track */
	UINT32 speed_zone[MAX_TRACKS];				/* speed zone for each physical track */
	int error[MAX_ERROR_SECTORS];				/* error code for each logical sector */

	UINT8 id1, id2;								/* DOS disk format ID */
};

INLINE float get_dos_track(int track)
{
	return ((float)track / 2) + 1;
}

static struct d64dsk_tag *get_tag(floppy_image *floppy)
{
	struct d64dsk_tag *tag;
	tag = (d64dsk_tag *)floppy_tag(floppy);
	return tag;
}

static int d64_get_heads_per_disk(floppy_image *floppy)
{
	return get_tag(floppy)->heads;
}

static int d64_get_tracks_per_disk(floppy_image *floppy)
{
	return get_tag(floppy)->tracks;
}

static int d64_get_sectors_per_track(floppy_image *floppy, int head, int track)
{
	int sectors_per_track = 0; 
	
	switch (get_tag(floppy)->dos)
	{
	case DOS1:	sectors_per_track = DOS1_SECTORS_PER_TRACK[track / 2]; break;
	case DOS2:	sectors_per_track = DOS2_SECTORS_PER_TRACK[track / 2]; break;
	case DOS27:	sectors_per_track = DOS27_SECTORS_PER_TRACK[track];     break;
	}

	return sectors_per_track;
}

static floperr_t get_track_offset(floppy_image *floppy, int head, int track, UINT64 *offset)
{
	struct d64dsk_tag *tag = get_tag(floppy);
	UINT64 offs = 0;

	if ((track < 0) || (track >= tag->tracks))
		return FLOPPY_ERROR_SEEKERROR;

	offs = tag->track_offset[head][track];

	if (offset)
		*offset = offs;

	return FLOPPY_ERROR_SUCCESS;
}

static const int bin_2_gcr[] =
{
	0x0a, 0x0b, 0x12, 0x13, 0x0e, 0x0f, 0x16, 0x17,
	0x09, 0x19, 0x1a, 0x1b, 0x0d, 0x1d, 0x1e, 0x15
};

/* This could be of use if we ever implement saving in .d64 format, to convert back GCR -> d64 */
/*
static const int gcr_2_bin[] =
{
    -1, -1,   -1,   -1,
    -1, -1,   -1,   -1,
    -1, 0x08, 0x00, 0x01,
    -1, 0x0c, 0x04, 0x05,
    -1, -1,   0x02, 0x03,
    -1, 0x0f, 0x06, 0x07,
    -1, 0x09, 0x0a, 0x0b,
    -1, 0x0d, 0x0e, -1
};
*/

/* gcr_double_2_gcr takes 4 bytes (a, b, c, d) and shuffles their nibbles to obtain 5 bytes in dest */
/* The result is basically res = (enc(a) << 15) | (enc(b) << 10) | (enc(c) << 5) | enc(d)
 * with res being 5 bytes long and enc(x) being the GCR encode of x.
 * In fact, we store the result as five separate bytes in the dest argument
 */
static void gcr_double_2_gcr(UINT8 a, UINT8 b, UINT8 c, UINT8 d, UINT8 *dest)
{
	UINT8 gcr[8];

	/* Encode each nibble to 5 bits */
	gcr[0] = bin_2_gcr[a >> 4];
	gcr[1] = bin_2_gcr[a & 0x0f];
	gcr[2] = bin_2_gcr[b >> 4];
	gcr[3] = bin_2_gcr[b & 0x0f];
	gcr[4] = bin_2_gcr[c >> 4];
	gcr[5] = bin_2_gcr[c & 0x0f];
	gcr[6] = bin_2_gcr[d >> 4];
	gcr[7] = bin_2_gcr[d & 0x0f];

	/* Re-order the encoded data to only keep the 5 lower bits of each byte */
	dest[0] = (gcr[0] << 3) | (gcr[1] >> 2);
	dest[1] = (gcr[1] << 6) | (gcr[2] << 1) | (gcr[3] >> 4);
	dest[2] = (gcr[3] << 4) | (gcr[4] >> 1);
	dest[3] = (gcr[4] << 7) | (gcr[5] << 2) | (gcr[6] >> 3);
	dest[4] = (gcr[6] << 5) | gcr[7];
}

static floperr_t d64_read_track(floppy_image *floppy, int head, int track, UINT64 offset, void *buffer, size_t buflen)
{
	struct d64dsk_tag *tag = get_tag(floppy);
	floperr_t err;
	UINT64 track_offset;

	/* get track offset */
	err = get_track_offset(floppy, head, track, &track_offset);

	if (err)
		return err;

	if (track_offset != INVALID_OFFSET)
	{
		UINT8 id1 = tag->id1;
		UINT8 id2 = tag->id2;

		/* determine logical track number */
		int dos_track = get_dos_track(track);
		
		if (tag->dos == DOS27)
		{
			dos_track = track + 1;
		}

		/* logical track numbers continue on the flip side */
		if (head == 1) dos_track += tag->dos_tracks;

		/* determine number of sectors per track */
		int sectors_per_track = d64_get_sectors_per_track(floppy, head, track); 

		/* allocate D64 track data buffer */
		UINT16 d64_track_size = sectors_per_track * SECTOR_SIZE;
		UINT8 d64_track_data[d64_track_size];

		/* allocate temporary GCR track data buffer */
		UINT16 gcr_track_size = 2 + (sectors_per_track * SECTOR_SIZE_GCR);
		UINT8 gcr_track_data[gcr_track_size];
		UINT64 gcr_pos = 2;
		
		if (buflen < gcr_track_size) fatalerror("D64 track buffer too small: %u!\n", (UINT32)buflen);

		/* prepend GCR track data buffer with GCR track size */
		gcr_track_data[0] = gcr_track_size & 0xff;
		gcr_track_data[1] = gcr_track_size >> 8;

		/* read D64 track data */
		floppy_image_read(floppy, d64_track_data, track_offset, d64_track_size);

		/* GCR encode D64 sector data */
		for (int sector = 0; sector < sectors_per_track; sector++)
		{
			// here we convert the sector data to gcr directly!
			// IMPORTANT: we shall implement errors in reading sectors!
			// these can modify e.g. header info $01 & $05

			// first we set the position at which sector data starts in the image
			UINT64 d64_pos = sector * SECTOR_SIZE;
			int i;

			/*
                1. Header sync       FF FF FF FF FF (40 'on' bits, not GCR encoded)
                2. Header info       52 54 B5 29 4B 7A 5E 95 55 55 (10 GCR bytes)
                3. Header gap        55 55 55 55 55 55 55 55 55 (9 bytes, never read)
                4. Data sync         FF FF FF FF FF (40 'on' bits, not GCR encoded)
                5. Data block        55...4A (325 GCR bytes)
                6. Inter-sector gap  55 55 55 55...55 55 (4 to 19 bytes, never read)
            */

			/* Header sync */
			for (i = 0; i < 5; i++)
				gcr_track_data[gcr_pos + i] = 0xff;
			gcr_pos += 5;

			/* Header info */
			/* These are 8 bytes unencoded, which become 10 bytes encoded */
			// $00 - header block ID ($08)                      // this byte can be modified by error code 20 -> 0xff
			// $01 - header block checksum (EOR of $02-$05)     // this byte can be modified by error code 27 -> ^ 0xff
			// $02 - Sector# of data block
			// $03 - Track# of data block
			gcr_double_2_gcr(0x08, sector ^ dos_track ^ id2 ^ id1, sector, dos_track, gcr_track_data + gcr_pos);
			gcr_pos += 5;

			// $04 - Format ID byte #2
			// $05 - Format ID byte #1
			// $06 - $0F ("off" byte)
			// $07 - $0F ("off" byte)
			gcr_double_2_gcr(id2, id1, 0x0f, 0x0f, gcr_track_data + gcr_pos);
			gcr_pos += 5;

			/* Header gap */
			for (i = 0; i < 9; i++)
				gcr_track_data[gcr_pos + i] = 0x55;
			gcr_pos += 9;

			/* Data sync */
			for (i = 0; i < 5; i++)
				gcr_track_data[gcr_pos + i] = 0xff;
			gcr_pos += 5;

			/* Data block */
			// we first need to calculate the checksum of the 256 bytes of the sector
			UINT8 sector_checksum = d64_track_data[d64_pos];
			for (i = 1; i < 256; i++)
				sector_checksum ^= d64_track_data[d64_pos + i];

			/*
                $00      - data block ID ($07)
                $01-100  - 256 bytes sector data
                $101     - data block checksum (EOR of $01-100)
                $102-103 - $00 ("off" bytes, to make the sector size a multiple of 5)
            */
			gcr_double_2_gcr(0x07, d64_track_data[d64_pos], d64_track_data[d64_pos + 1], d64_track_data[d64_pos + 2], gcr_track_data + gcr_pos);
			gcr_pos += 5;

			for (i = 1; i < 64; i++)
			{
				gcr_double_2_gcr(d64_track_data[d64_pos + 4 * i - 1], d64_track_data[d64_pos + 4 * i],
									d64_track_data[d64_pos + 4 * i + 1], d64_track_data[d64_pos + 4 * i + 2], gcr_track_data + gcr_pos);
				gcr_pos += 5;
			}

			gcr_double_2_gcr(d64_track_data[d64_pos + 255], sector_checksum, 0x00, 0x00, gcr_track_data + gcr_pos);
			gcr_pos += 5;

			/* Inter-sector gap */
			// "In tests that the author conducted on a real 1541 disk, gap sizes of 8  to  19 bytes were seen."
			// Here we put 14 as an average...
			for (i = 0; i < 14; i++)
				gcr_track_data[gcr_pos + i] = 0x55;
			gcr_pos += 14;
		}

		/* copy GCR track data to buffer */
		memcpy(buffer, gcr_track_data, gcr_track_size);
	}
	else	/* half tracks */
	{
		/* set track length to 0 */
		memset(buffer, 0, buflen);
	}

	return FLOPPY_ERROR_SUCCESS;
}

static floperr_t d64_write_track(floppy_image *floppy, int head, int track, UINT64 offset, const void *buffer, size_t buflen)
{
	return FLOPPY_ERROR_UNSUPPORTED;
}

static void d64_identify(floppy_image *floppy, int *dos, int *heads, int *tracks, bool *has_errors)
{
	switch (floppy_image_size(floppy))
	{
	/* 2040/3040 */
	case D67_SIZE_35_TRACKS:				*dos = DOS1;  *heads = 1; *tracks = 35; *has_errors = false; break;

	/* 4040/2031/1541/1551 */
	case D64_SIZE_35_TRACKS:				*dos = DOS2;  *heads = 1; *tracks = 35; *has_errors = false; break;
	case D64_SIZE_35_TRACKS_WITH_ERRORS:	*dos = DOS2;  *heads = 1; *tracks = 35; *has_errors = true;  break;
	case D64_SIZE_40_TRACKS:				*dos = DOS2;  *heads = 1; *tracks = 40; *has_errors = false; break;
	case D64_SIZE_40_TRACKS_WITH_ERRORS:	*dos = DOS2;  *heads = 1; *tracks = 40; *has_errors = true;  break;
	case D64_SIZE_42_TRACKS:				*dos = DOS2;  *heads = 1; *tracks = 42; *has_errors = false; break;
	case D64_SIZE_42_TRACKS_WITH_ERRORS:	*dos = DOS2;  *heads = 1; *tracks = 42; *has_errors = true;  break;

	/* 1571 */
	case D71_SIZE_70_TRACKS:				*dos = DOS2;  *heads = 2; *tracks = 35; *has_errors = false; break;
	case D71_SIZE_70_TRACKS_WITH_ERRORS:	*dos = DOS2;  *heads = 2; *tracks = 35; *has_errors = true;  break;

	/* 8050 */
	case D80_SIZE_77_TRACKS:				*dos = DOS27; *heads = 1; *tracks = 77; *has_errors = false; break;

	/* 8250/SFD1001 */
	case D82_SIZE_154_TRACKS:				*dos = DOS27; *heads = 2; *tracks = 77; *has_errors = false; break;
	}
}

FLOPPY_IDENTIFY( d64_dsk_identify )
{
	int dos = 0, heads, tracks;
	bool has_errors = false;

	*vote = 0;

	d64_identify(floppy, &dos, &heads, &tracks, &has_errors);

	if (dos == DOS2 && heads == 1)
	{
		*vote = 100;
	}

	return FLOPPY_ERROR_SUCCESS;
}

FLOPPY_IDENTIFY( d67_dsk_identify )
{
	*vote = 0;

	if (floppy_image_size(floppy) == D67_SIZE_35_TRACKS)
	{
		*vote = 100;
	}

	return FLOPPY_ERROR_SUCCESS;
}

FLOPPY_IDENTIFY( d71_dsk_identify )
{
	int heads = 0, tracks = 0, dos;
	bool has_errors = false;

	*vote = 0;

	d64_identify(floppy, &dos, &heads, &tracks, &has_errors);

	if (dos == DOS2 && heads == 2)
	{
		*vote = 100;
	}

	return FLOPPY_ERROR_SUCCESS;
}

FLOPPY_IDENTIFY( d80_dsk_identify )
{
	*vote = 0;

	if (floppy_image_size(floppy) == D80_SIZE_77_TRACKS)
	{
		*vote = 100;
	}

	return FLOPPY_ERROR_SUCCESS;
}

FLOPPY_IDENTIFY( d82_dsk_identify )
{
	*vote = 0;

	if (floppy_image_size(floppy) == D82_SIZE_154_TRACKS)
	{
		*vote = 100;
	}

	return FLOPPY_ERROR_SUCCESS;
}

FLOPPY_CONSTRUCT( d64_dsk_construct )
{
	struct FloppyCallbacks *callbacks;
	struct d64dsk_tag *tag;
	UINT8 id[2];

	int track_offset = 0;
	int head, track;
	
	int heads = 0, dos_tracks = 0, dos = 0;
	bool has_errors;

	if (params)
	{
		/* create not supported */
		return FLOPPY_ERROR_UNSUPPORTED;
	}

	tag = (struct d64dsk_tag *) floppy_create_tag(floppy, sizeof(struct d64dsk_tag));

	if (!tag) return FLOPPY_ERROR_OUTOFMEMORY;

	/* identify image type */
	d64_identify(floppy, &dos, &heads, &dos_tracks, &has_errors);

	tag->dos = dos;
	tag->heads = heads;
	tag->tracks = MAX_TRACKS;
	tag->dos_tracks = dos_tracks;

	if (LOG)
	{
		logerror("D64 size: %04x\n", (UINT32)floppy_image_size(floppy));
		logerror("D64 heads: %u\n", heads);
		logerror("D64 tracks: %u\n", dos_tracks);
	}

	/* determine track data offsets */
	for (head = 0; head < heads; head++)
	{
		for (track = 0; track < tag->tracks; track++)
		{
			if (dos == DOS27)
			{
				if (track >= dos_tracks)
				{
					/* track out of range */
					tag->track_offset[head][track] = INVALID_OFFSET;
				}
				else
				{
					tag->track_offset[head][track] = track_offset;
					track_offset += DOS27_SECTORS_PER_TRACK[track] * SECTOR_SIZE;
	
					if (LOG) logerror("D64 head %u track %u data offset: %04x\n", head, track, tag->track_offset[head][track]);
				}
			}
			else
			{
				if ((track % 2) || ((track / 2) >= dos_tracks))
				{
					/* half track or out of range */
					tag->track_offset[head][track] = INVALID_OFFSET;
				}
				else
				{
					/* full track */
					tag->track_offset[head][track] = track_offset;
					
					if (dos == DOS1)
						track_offset += DOS1_SECTORS_PER_TRACK[track / 2] * SECTOR_SIZE;
					else
						track_offset += DOS2_SECTORS_PER_TRACK[track / 2] * SECTOR_SIZE;

					if (LOG) logerror("D64 head %u track %.1f data offset: %04x\n", head, get_dos_track(track), tag->track_offset[head][track]);
				}
			}
		}
	}

	/* determine speed zones */
	for (track = 0; track < tag->tracks; track++)
	{
		if (dos == DOS27)
		{
			tag->speed_zone[track] = DOS27_SPEED_ZONE[track];

			if (LOG) logerror("D64 track %u speed zone: %u\n", track, tag->speed_zone[track]);
		}
		else
		{
			tag->speed_zone[track] = DOS1_SPEED_ZONE[track / 2];

			if (LOG) logerror("D64 track %.1f speed zone: %u\n", get_dos_track(track), tag->speed_zone[track]);
		}
	}

	/* read format ID from directory */
	if (dos == DOS27)
		floppy_image_read(floppy, id, tag->track_offset[0][39] + 0x18, 2);
	else
		floppy_image_read(floppy, id, tag->track_offset[0][34] + 0xa2, 2);
	
	tag->id1 = id[0];
	tag->id2 = id[1];

	if (LOG) logerror("D64 format ID: %02x%02x\n", id[0], id[1]);

	/* set callbacks */
	callbacks = floppy_callbacks(floppy);

	callbacks->read_track = d64_read_track;
	callbacks->write_track = d64_write_track;
	callbacks->get_heads_per_disk = d64_get_heads_per_disk;
	callbacks->get_tracks_per_disk = d64_get_tracks_per_disk;

	return FLOPPY_ERROR_SUCCESS;
}

/*
id1, id2 are the same for extended d64 (i.e. with error tables), for d67 and for d71

for d81 they are at track 40 bytes 0x17 & 0x18
for d80 & d82 they are at track 39 bytes 0x18 & 0x19
*/
