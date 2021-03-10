#ifdef CMS

//#define CMS_DEBUG 1

#ifdef CMS_DEBUG
#include <stdio.h>
#include <stdarg.h>
#endif

#include "cms.h"

#ifdef CMS_DEBUG
void debug_log(const char *fmt, ...)
{
   FILE *f_log;
   va_list ap;

	f_log = fopen("cmslog.log", "a");
	va_start(ap, fmt);
        vfprintf(f_log, fmt, ap);
	va_end(ap);
        fclose(f_log);
}
#endif

mid_channel cms_synth[MAX_CMS_CHANNELS];	// CMS synth

// CMS IO port address
unsigned short cmsPort;


// The 12 note-within-an-octave values for the SAA1099, starting at B
static unsigned char noteAdr[] = {5, 32, 60, 85, 110, 132, 153, 173, 192, 210, 227, 243};

// Volume
static unsigned char atten[128] = {
                 0,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,
                 3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,
                 5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,
                 7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,
                 9,9,9,9,9,9,9,9,10,10,10,10,10,10,10,10,
                 11,11,11,11,11,11,11,11,12,12,12,12,12,12,12,12,
                 13,13,13,13,13,13,13,13,14,14,14,14,14,14,14,14,
        	 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
        };

// Logic channel - first chip/second chip
static unsigned char ChanReg[12] =  {000,001,002,003,004,005,000,001,002,003,004,005};

// Set octave command
static unsigned char OctavReg[12] = {0x10,0x10,0x11,0x11,0x12,0x12,0x10,0x10,0x11,0x11,0x12,0x12};

unsigned char CmsOctaveStore[12];

unsigned char ChanEnableReg[2] = {0,0};

// Note priority
unsigned char NotePriority;


void __declspec( naked ) cmsWrite(void)
{
/*
	parameters
	dx = port base+offset
	ah = register
	al = data
*/
_asm
    {
	inc  dx
	xchg al,ah
	out  dx,al
	dec  dx
	xchg al,ah
	out  dx,al
        ret
    }
}

void __declspec( naked ) cmsNull(unsigned short port)
{
/*
	parameters
	dx = port offset to null
*/
_asm
    {
	add   dx,cmsPort // FIXME! CMSPortAddr
	mov   cx,20h
	xor   ax,ax
loop_nul:           // null all 20 registers 
	call  cmsWrite
	inc   ah
	loop  loop_nul

	mov   ax,1C02h // reset chip 
	call  cmsWrite

	mov   ax,1C01h // enable this chip 
	call  cmsWrite

	ret
    }
}

void cmsReset(unsigned short port)
{
   unsigned char i;
   cmsPort = port;
   _asm
    {
	mov  dx,0
	call cmsNull
	mov  dx,2
	call cmsNull
    }
	for (i=0;i<11;i++)CmsOctaveStore[i]=0;
	ChanEnableReg[0]=0;
	ChanEnableReg[1]=0;
        for (i=0;i<MAX_CMS_CHANNELS;i++)
        {
                cms_synth[i].note=0;
                cms_synth[i].priority=0;
        }
	NotePriority = 0;
}

void cmsDisableVoice(unsigned char voice)
{
_asm
   {
		xor   bh,bh
		mov   bl,voice

		mov   dx,cmsPort	// FIXME !!!

		mov   bl,ChanReg[bx]	; bl = true channel (0 - 5)

		xor   di,di
		mov   cl,voice
		cmp   cl,06h
                jl    skip_inc
		inc   di
                add   dx,2
skip_inc:
		mov   al,14h
		inc   dx
		out   dx,al
		dec   dx

		mov   al,ChanEnableReg[di]
		mov   ah,01h
		mov   cl,bl
		shl   ah,cl
		not   ah
		and   al,ah		; al = voice enable reg

		out   dx,al
		mov   ChanEnableReg[di],al
    }
}


void cmsSetVolume(unsigned char voice,unsigned char amplitudeLeft,unsigned char amplitudeRight)
{
_asm
   {
		xor   bh,bh
		mov   bl,voice
		
		mov   dx,cmsPort	// FIXME !!!
		cmp   bl,06h		; check channel num > 5?
		jl    setVol	; yes - set port = port + 2
		add   dx,2
setVol:
		mov   bl,ChanReg[bx]	; bx = true channel (0 - 5)
		mov   al,byte ptr amplitudeLeft
		mov   ah,byte ptr amplitudeRight
		mov   cl,4
		shl   ah,cl
		or    al,ah
		mov   ah,bl
		call cmsWrite
   }
}

void cmsSound(unsigned char voice,unsigned char freq,unsigned char octave,unsigned char amplitudeLeft,unsigned char amplitudeRight)
{
_asm
   {
		xor   bh,bh
		mov   bl,voice
		
		mov   dx,cmsPort	// FIXME !!!
		cmp   bl,06h		; check channel num > 5?
		jl    setOctave	; yes - set port = port + 2
		add   dx,2
setOctave:
		mov   bl,ChanReg[bx]	; bx = true channel (0 - 5)
		mov   ah,OctavReg[bx]   ; ah = Set octave command
;
;	ah now = register
;		0,1,6,7=$10
;		2,3,8,9=$11
;		4,5,10,11=$12
;
;	CMS octave regs are write only, so we have to track
;	the values in adjoining voices manually

		mov   al,ah
		xor   ah,ah		; ax = set octave cmd (10h - 12h)
		mov   di,ax		; di = ax
		sub   di,010h		; di = octave cmd - 10h (0..2 index)
		mov   cl,voice
		cmp   cl,06h
                jl    skip_inc
		add   di,3
skip_inc:
		mov   ah,al		; set ah back to octave cmd

		mov   al,byte ptr CmsOctaveStore[di]
		mov   bh,octave
		test  bl,01h
		jnz   shiftOctave
		and   al,0F0h
		jmp   outOctave
shiftOctave:
		and   al,0Fh
		mov   cl,4
		shl   bh,cl
outOctave:
		or    al,bh
		mov   byte ptr CmsOctaveStore[di],al
		call cmsWrite		; set octave to CMS
setAmp:
		mov   al,byte ptr amplitudeLeft
		mov   ah,byte ptr amplitudeRight
		;and   al,0Fh
		mov   cl,4
		shl   ah,cl
		or    al,ah
		mov   ah,bl
		call cmsWrite
setFreq:
		mov   al,byte ptr freq
		or    ah,08h


		call cmsWrite
voiceEnable:
		mov   al,14h
		inc   dx
		out   dx,al
		dec   dx

		xor   di,di
		mov   cl,voice
		cmp   cl,06h
                jl    skip_inc2
		inc   di
skip_inc2:
		mov   al,ChanEnableReg[di]
		mov   ah,01h
		mov   cl,bl
		shl   ah,cl
		or    al,ah
		out   dx,al
		mov   ChanEnableReg[di],al
    }
}

// ****
// High-level CMS synth procedures
// ****

void cmsNoteOff(unsigned char channel, unsigned char note)
{
  unsigned char i;
  unsigned char voice;

    voice = MAX_CMS_CHANNELS;
    for(i=0; i<MAX_CMS_CHANNELS; i++)
    {
      	if(cms_synth[i].note==note)
      	{
        	voice = i;
        	break;
      	}
    }


    // Note not found, ignore note off command
    if(voice==MAX_CMS_CHANNELS)
    {
#ifdef CMS_DEBUG
	debug_log("not found Ch=%u,note=%u\n",channel & 0xFF,note & 0xFF);
#endif
    	return;
    }

    // decrease priority for all notes greater than current
    for (i=0; i<MAX_CMS_CHANNELS; i++)
	if (cms_synth[i].priority > cms_synth[voice].priority )
		cms_synth[i].priority = cms_synth[i].priority - 1;

    
    if (NotePriority != 0) NotePriority--;

    cmsDisableVoice(voice);
    cms_synth[voice].note = 0;
    cms_synth[voice].priority = 0;
}

void cmsNoteOn(unsigned char channel, unsigned char note, unsigned char velocity)
{
  unsigned char octave;
  unsigned char noteVal;
  unsigned char i;
  unsigned char voice;
  unsigned char note_cms;

  if (velocity != 0)
  {
	note_cms = note+1;

	octave = (note_cms / 12) - 1; //Some fancy math to get the correct octave
  	noteVal = note_cms - ((octave + 1) * 12); //More fancy math to get the correct note

	NotePriority++;

        voice = MAX_CMS_CHANNELS;
    	for(i=0; i<MAX_CMS_CHANNELS; i++)
    	{
      		if(cms_synth[i].note==0)
      		{
        		voice = i;
        		break;
      		}
    	}


  	// We run out of voices, find low priority voice
  	if(voice==MAX_CMS_CHANNELS)
  	{
		unsigned char min_prior = cms_synth[0].priority;

		// find note with min prioryty
		voice = 0;
    		for (i=1; i<MAX_CMS_CHANNELS; i++)
		  if (cms_synth[i].priority < min_prior) 
		  {
			voice = i;
                        min_prior = cms_synth[i].priority;
                  }

		// decrease all notes priority by one
    		for (i=0; i<MAX_CMS_CHANNELS; i++)
		  if (cms_synth[i].priority != 0)
			cms_synth[i].priority = cms_synth[i].priority - 1;

		// decrease current priority
		if (NotePriority != 0) NotePriority--;

  	}

	cmsSound(voice,noteAdr[noteVal],octave,atten[velocity],atten[velocity]); 

	cms_synth[voice].note = note;
	cms_synth[voice].priority = NotePriority;

  } 
  else
        cmsNoteOff(channel,note);
}


void cmsTick(void)
{
#if 0
  unsigned char i;
  unsigned char noteVal;

  for (i=0;i<MAX_CMS_CHANNELS;i++) 
    if (cms_synth[i].note != 0)
	{
		noteVal = cms_synth[i].volume-1;
		if (noteVal != 0)
			{
				cmsSetVolume(i,atten[noteVal],atten[noteVal]);
				cms_synth[i].volume = noteVal;
			}
		else
			{
    				cmsDisableVoice(i);
				cms_synth[i].volume = 0;
				cms_synth[i].note = 0;
			}
	}
#endif
}

void cmsController(unsigned char channel, unsigned char id, unsigned char val)
{
  unsigned char i;
    if ((id==121) || (id==123)) // All Sound/Notes Off
    {
  		for (i=0;i<MAX_CMS_CHANNELS;i++) 
    		  if (cms_synth[i].note != 0)
		 	{
    				cmsDisableVoice(i);
				cms_synth[i].note = 0;
				cms_synth[i].priority = 0;
		 	}
    }
}

#endif
