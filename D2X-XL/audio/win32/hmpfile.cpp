/* This is HMP file playing code by Arne de Bruijn */
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>

#include "descent.h"
#include "hmpfile.h"
#include "byteswap.h"
#include "u_mem.h"

//------------------------------------------------------------------------------

hmp_file *hmp_open(const char *filename, int bUseD1Hog) 
{
	int i;
	char buf [256];
	long data = 0;
	CFile cf;
	hmp_file *hmp;
	int num_tracks, midi_div;
	ubyte *p;

	if (!cf.Open (const_cast<char*> (filename), gameFolders.szDataDir, "rb", bUseD1Hog))
		return NULL;
	hmp = new hmp_file;
	if (!hmp) {
		cf.Close ();
		return NULL;
	}
	memset(hmp, 0, sizeof(*hmp));
	if ((cf.Read (buf, 1, 8) != 8) || (memcmp(buf, "HMIMIDIP", 8)))
		goto err;
	if (cf.Seek (0x30, SEEK_SET))
		goto err;
	if (cf.Read (&num_tracks, 4, 1) != 1)
		goto err;
	if (cf.Seek (0x38, SEEK_SET))
		goto err;
	if (cf.Read (&midi_div, 4, 1) != 1)
		goto err;
	if ((num_tracks < 1) || (num_tracks > HMP_TRACKS))
		goto err;
	hmp->num_trks = num_tracks;
	hmp->midi_division = midi_div;
   hmp->tempo = 120;
	if (cf.Seek(0x308, SEEK_SET))
		goto err;
    for (i = 0; i < num_tracks; i++) {
		if ((cf.Seek(4, SEEK_CUR)) || (cf.Read(&data, 4, 1) != 1))
			goto err;
		data -= 12;
#if 0
		if (i == 0)  /* track 0: reserve length for tempo */
		    data += sizeof(hmp_tempo);
#endif
		hmp->trks [i].len = data;
		if (!(p = hmp->trks [i].data = new ubyte [data]))
			goto err;
#if 0
		if (i == 0) { /* track 0: add tempo */
			memcpy(p, hmp_tempo, sizeof(hmp_tempo));
			p += sizeof(hmp_tempo);
			data -= sizeof(hmp_tempo);
		}
#endif
					     /* finally, read track data */
		if ((cf.Seek(4, SEEK_CUR)) || (cf.Read(p, data, 1) != 1))
            goto err;
   }
   cf.Close ();
   return hmp;

err:
   cf.Close ();
   hmp_close(hmp);
   return NULL;
}

//------------------------------------------------------------------------------

#ifdef _WIN32

void hmp_stop(hmp_file *hmp) 
{
	MIDIHDR *mhdr;
	
if (!hmp->stop) {
	hmp->stop = 1;
	//PumpMessages();
	midiStreamStop (hmp->hmidi);
   while (hmp->bufs_in_mm)
      {
		//PumpMessages();
		G3_SLEEP(0);
      }
	}
while ((mhdr = hmp->evbuf)) {
	midiOutUnprepareHeader (reinterpret_cast<HMIDIOUT> (hmp->hmidi), mhdr, sizeof (MIDIHDR));
	hmp->evbuf = mhdr->lpNext;
	delete[] mhdr;
	mhdr = NULL;
	}

if (hmp->hmidi) {
	midiStreamClose(hmp->hmidi);
	hmp->hmidi = NULL;
	}
}

//------------------------------------------------------------------------------

void hmp_close(hmp_file *hmp) 
{
	int i;

	hmp_stop(hmp);
	for (i = 0; i < hmp->num_trks; i++)
		if (hmp->trks [i].data)
			delete[] hmp->trks [i].data;
	delete hmp;
}

//------------------------------------------------------------------------------
/*
 * read a HMI nType variable length number
 */
static int get_var_num_hmi(ubyte *data, int datalen, ulong *value) 
{
	ubyte *p;
	ulong v = 0;
	int shift = 0;

	p = data;
	while ((datalen > 0) && !(*p & 0x80)) {
		v += *(p++) << shift;
		shift += 7;
		datalen --;
    }
	if (!datalen)
		return 0;
    v += (*(p++) & 0x7f) << shift;
	if (value) *value = v;
    return (int) (p - data);
}

//------------------------------------------------------------------------------
/*
 * read a MIDI nType variable length number
 */
static int get_var_num(ubyte *data, int datalen, ulong *value) 
{
	ubyte *orgdata = data;
	ulong v = 0;

	while ((datalen > 0) && (*data & 0x80))
		v = (v << 7) + (*(data++) & 0x7f);
	if (!datalen)
		return 0;
    v = (v << 7) + *(data++);
    if (value) *value = v;
    return (int) (data - orgdata);
}

//------------------------------------------------------------------------------

static int get_event(hmp_file *hmp, event *ev) 
{
    static int cmdlen [7] ={3,3,3,3,2,2,3};
	ulong got;
	ulong mindelta, delta;
	int i, ev_num;
	hmp_track *trk, *fndtrk;

	mindelta = INT_MAX;
	fndtrk = NULL;
	for (trk = hmp->trks, i = hmp->num_trks; (i--) > 0; trk++) {
		if (!trk->left)
			continue;
		if (!(got = get_var_num_hmi(trk->cur, trk->left, &delta)))
			return HMP_INVALID_FILE;
		if (trk->left > got + 2 && *(trk->cur + got) == 0xff
			&& *(trk->cur + got + 1) == 0x2f) {/* end of track */
			trk->left = 0;
			continue;
		}
        delta += trk->curTime - hmp->curTime;
		if (delta < mindelta) {
			mindelta = delta;
			fndtrk = trk;
		}
	}
	if (!(trk = fndtrk))
			return HMP_EOF;

	got = get_var_num_hmi(trk->cur, trk->left, &delta);

	trk->curTime += delta;
	ev->delta = trk->curTime - hmp->curTime;
	hmp->curTime = trk->curTime;

	if ((trk->left -= got) < 3)
			return HMP_INVALID_FILE;
	trk->cur += got;
	/*memset(ev, 0, sizeof(*ev));*/ev->datalen = 0;
	ev->msg [0] = ev_num = *(trk->cur++);
	trk->left--;
	if (ev_num < 0x80)
	    return HMP_INVALID_FILE; /* invalid command */
	if (ev_num < 0xf0) {
		ev->msg [1] = *(trk->cur++);
		trk->left--;
		if (cmdlen [((ev_num) >> 4) - 8] == 3) {
			ev->msg [2] = *(trk->cur++);
			trk->left--;
		}
	} else if (ev_num == 0xff) {
		ev->msg [1] = *(trk->cur++);
		trk->left--;
		if (!(got = get_var_num(ev->data = trk->cur, trk->left, reinterpret_cast<ulong*> (&ev->datalen))))
			return HMP_INVALID_FILE;
	    trk->cur += ev->datalen;
		if (trk->left <= ev->datalen)
			return HMP_INVALID_FILE;
		trk->left -= ev->datalen;
	} else /* sysex -> error */
	    return HMP_INVALID_FILE;
	return 0;
}

//------------------------------------------------------------------------------

static int fill_buffer(hmp_file *hmp) 
{
	MIDIHDR *mhdr = hmp->evbuf;
	uint *p = reinterpret_cast<uint*> (mhdr->lpData + mhdr->dwBytesRecorded);
	uint *pend = reinterpret_cast<uint*> (mhdr->lpData + mhdr->dwBufferLength);
	uint i;
	event ev;

	while (p + 4 <= pend) {
		if (hmp->pending_size) {
			i = (int) (p - pend) * 4;
			if (i > hmp->pending_size)
				i = hmp->pending_size;
			*(p++) = hmp->pending_event | i;
			*(p++) = 0;
			memcpy (p, hmp->pending, i);
			hmp->pending_size -= i;
			p += (i + 3) / 4;
		} else {
			if ((i = get_event(hmp, &ev))) {
            mhdr->dwBytesRecorded = (int) (reinterpret_cast<ubyte*> (p) - reinterpret_cast<ubyte*> (mhdr->lpData));
				return i;
			}
			if (ev.datalen) {
				hmp->pending_size = ev.datalen;
				hmp->pending = ev.data;
				hmp->pending_event = ev.msg [0] << 24;
			} else {
				*(p++) = ev.delta;
				*(p++) = 0;
				*(p++) = (((DWORD)MEVT_SHORTMSG) << 24) |
					((DWORD)ev.msg [0]) |
					(((DWORD)ev.msg [1]) << 8) |
					(((DWORD)ev.msg [2]) << 16);
			}
		}
	}
        mhdr->dwBytesRecorded = (int) (reinterpret_cast<ubyte*> (p) - reinterpret_cast<ubyte*> (mhdr->lpData));
	return 0;
}

//------------------------------------------------------------------------------

static int setup_buffers(hmp_file *hmp) 
{
	int i;
	MIDIHDR *buf, *lastbuf;

	lastbuf = NULL;
	for (i = 0; i < HMP_BUFFERS; i++) {
		if (!(buf = reinterpret_cast<MIDIHDR*> (new ubyte [HMP_BUFSIZE + sizeof(MIDIHDR)])))
			return HMP_OUT_OF_MEM;
		memset (buf, 0, sizeof (MIDIHDR));
		buf->lpData = reinterpret_cast<char*> (buf + 1);
		buf->dwBufferLength = HMP_BUFSIZE;
		buf->dwUser = (DWORD)(size_t)hmp;
		buf->lpNext = lastbuf;
		lastbuf = buf;
	}
	hmp->evbuf = lastbuf;
	return 0;
}

//------------------------------------------------------------------------------

static void reset_tracks(struct hmp_file *hmp) 
{
	int i;

	for (i = 0; i < hmp->num_trks; i++) {
		hmp->trks [i].cur = hmp->trks [i].data;
		hmp->trks [i].left = hmp->trks [i].len;
		hmp->trks [i].curTime = 0;
	}
	hmp->curTime=0;
}

//------------------------------------------------------------------------------

static void _stdcall midi_callback(HMIDISTRM hms, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2) 
{
	MIDIHDR *mhdr;
	hmp_file *hmp;
	int rc;

	if (uMsg != MOM_DONE)
		return;

	mhdr = reinterpret_cast<MIDIHDR*> ((size_t)dw1);
	mhdr->dwBytesRecorded = 0;
	hmp = reinterpret_cast<hmp_file*> (mhdr->dwUser);
	mhdr->lpNext = hmp->evbuf;
	hmp->evbuf = mhdr;
	hmp->bufs_in_mm--;

	if (!hmp->stop) {
		while (fill_buffer(hmp) == HMP_EOF) {
			if (!hmp->bLoop)
				hmp->stop=1;
			reset_tracks(hmp);
		}
		if ((rc = midiStreamOut(hmp->hmidi, hmp->evbuf, 
			sizeof(MIDIHDR))) != MMSYSERR_NOERROR) {
			/* ??? */
		} else {
			hmp->evbuf = hmp->evbuf->lpNext;
			hmp->bufs_in_mm++;
		}
	}
}

//------------------------------------------------------------------------------

static void setup_tempo(hmp_file *hmp, ulong tempo) 
{
	MIDIHDR *mhdr = hmp->evbuf;
if (mhdr) {
	uint *p = reinterpret_cast<uint*> (mhdr->lpData + mhdr->dwBytesRecorded);
	*(p++) = 0;
	*(p++) = 0;
	*(p++) = (((DWORD)MEVT_TEMPO)<<24) | tempo;
	mhdr->dwBytesRecorded += 12;
	}
}

//------------------------------------------------------------------------------

int hmp_play(hmp_file *hmp, int bLoop) 
{
	int rc;
	MIDIPROPTIMEDIV mptd;
#if 0
	uint    numdevs;
   int i=0;

numdevs = midiOutGetNumDevs();
hmp->devid = -1;
do {
	MIDIOUTCAPS devcaps;
	midiOutGetDevCaps (i, &devcaps, sizeof (MIDIOUTCAPS));
	if ((devcaps.wTechnology == MOD_FMSYNTH) || (devcaps.wTechnology==MOD_SYNTH))
//			if ((devcaps.dwSupport & (MIDICAPS_VOLUME | MIDICAPS_STREAM)) == (MIDICAPS_VOLUME | MIDICAPS_STREAM))
		hmp->devid=i;
	i++;
	} while ((i < (int)numdevs) && (hmp->devid == -1));
if (hmp->devid == -1)
	return -1;
#endif
	hmp->bLoop = bLoop;
	hmp->devid = MIDI_MAPPER;

	if ((rc = setup_buffers(hmp)))
		return rc;
	if ((midiStreamOpen(&hmp->hmidi, &hmp->devid,1, (DWORD_PTR) midi_callback,
								0, CALLBACK_FUNCTION)) != MMSYSERR_NOERROR) {
		hmp->hmidi = NULL;
		return HMP_MM_ERR;
	}
	mptd.cbStruct  = sizeof(mptd);
	mptd.dwTimeDiv = hmp->tempo;
	if ((midiStreamProperty(hmp->hmidi,
         (LPBYTE)&mptd,
         MIDIPROP_SET|MIDIPROP_TIMEDIV)) != MMSYSERR_NOERROR) {
		/* FIXME: cleanup... */
		return HMP_MM_ERR;
	}

	reset_tracks(hmp);
	setup_tempo(hmp, 0x0f4240);

	hmp->stop = 0;
	while (hmp->evbuf) {
		if ((rc = fill_buffer(hmp))) {
			if (rc == HMP_EOF) {
				reset_tracks(hmp);
				continue;
			} else
				return rc;
		}
#if 0
	 {  FILE *f = fopen("dump","wb"); fwrite(hmp->evbuf->lpData, 
 hmp->evbuf->dwBytesRecorded,1,f); fclose(f); exit(1);}
#endif
 		if ((rc = midiOutPrepareHeader((HMIDIOUT)hmp->hmidi, hmp->evbuf, 
			sizeof(MIDIHDR))) != MMSYSERR_NOERROR) {
			/* FIXME: cleanup... */
			return HMP_MM_ERR;
		}
		if ((rc = midiStreamOut(hmp->hmidi, hmp->evbuf, 
			sizeof(MIDIHDR))) != MMSYSERR_NOERROR) {
			/* FIXME: cleanup... */
			return HMP_MM_ERR;
		}
		hmp->evbuf = hmp->evbuf->lpNext;
		hmp->bufs_in_mm++;
	}
	midiStreamRestart(hmp->hmidi);
	return 0;
}

#endif //_WIN32

//------------------------------------------------------------------------------
// ripped from the JJFFE project

static int hmp_track_to_midi (ubyte* track, int size, FILE *f)
{
	ubyte *pt = track;
	ubyte lc1 = 0,lastcom = 0;
	uint t = 0, d;
	int n1, n2;
	int startOffs = ftell (f);

while (track < pt + size) {	
	if (track [0] &0x80) {
		ubyte b = track [0] & 0x7F;
		fwrite (&b, sizeof (b), 1, f);
		t+=b;
		}
	else {
		d = (track [0]) &0x7F;
		n1 = 0;
		while ((track [n1] &0x80) == 0) {
			n1++;
			d += (track [n1] &0x7F) << (n1 * 7);
			}
		t += d;
		n1 = 1;
		while((track [n1] & 0x80) == 0) {
			n1++;
			if (n1 == 4)
				return 0;
			}
		for(n2 = 0;n2 <= n1; n2++) {
			ubyte b = track [n1 - n2] & 0x7F;
		
			if (n2 != n1) 
				b |= 0x80;
			fwrite (&b, sizeof (b), 1, f);
			}
		track += n1;
		}
	track++;
	if (*track == 0xFF) {//meta
		fwrite (track, 3 + track [2], 1, f);
		if (track [1] == 0x2F) 
			break;
		}
	else {
		lc1=track [0] ;
		if ((lc1&0x80) == 0) 
			return 0;
		switch (lc1 & 0xF0) {
			case 0x80:
			case 0x90:
			case 0xA0:
			case 0xB0:
			case 0xE0:
				if (lc1 != lastcom)
				fwrite (&lc1, sizeof (lc1), 1, f);
				fwrite (track + 1, 2, 1, f);
				track += 3;
				break;
			case 0xC0:
			case 0xD0:
				if (lc1 != lastcom)
					fwrite (&lc1, sizeof (lc1), 1, f);
				fwrite (track + 1, 1, 1, f);
				track += 2;
				break;
			default:
				return 0;
			}
		lastcom=lc1;
		}
	}
return ftell (f) - startOffs;
}

//------------------------------------------------------------------------------

static ubyte midiSetTempo [19] = {'M','T','r','k',0,0,0,11,0,0xFF,0x51,0x03,0x18,0x80,0x00,0,0xFF,0x2F,0};

int hmp_to_midi (hmp_file *hmp, char *pszFn)
{
	FILE	*f;
	int	i, j, nLenPos;
	short	s;

if (!(f = fopen (pszFn, "wb")))
	return 0;
// midi nSignature & header
fwrite ("MThd", 4, 1, f);
i = 6;
i = BE_INT (i);
fwrite (&i, sizeof (i), 1, f);
s = 1;
s = BE_SHORT (s);
fwrite (&s, sizeof (s), 1, f);	//format
s = BE_SHORT (hmp->num_trks);
fwrite (&s, sizeof (s), 1, f);
s = 0xC0;								//hmp->midi_division;
s = BE_SHORT (s);
fwrite (&s, sizeof (s), 1, f);	//tempo
fwrite (midiSetTempo, sizeof (midiSetTempo), 1, f);

for (j = 1; j < hmp->num_trks; j++) {
	// midi nSignature & track length & track data
	fwrite ("MTrk", 4, 1, f);
	nLenPos = ftell (f);
	i = 0;
	fwrite (&i, sizeof (i), 1, f);
	i = hmp_track_to_midi (hmp->trks [j].data, hmp->trks [j].len, f);
	i = BE_INT (i);
	fseek (f, nLenPos, SEEK_SET);
	fwrite (&i, sizeof (i), 1, f);
	fseek (f, 0, SEEK_END);
	}
return !fclose (f);
}

//------------------------------------------------------------------------------
//eof

