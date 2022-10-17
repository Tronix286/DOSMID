#ifndef cms_h_sentinel
#define cms_h_sentinel

#define MAX_CMS_CHANNELS 12

typedef struct {
        unsigned char note;
        unsigned char priority;
        unsigned char ch;
        unsigned char voice;
	unsigned char velocity;
} mid_channel;

void cmsReset(unsigned short port);

void cms_pitchwheel(unsigned short oplport, int channel, int pitchwheel);

void cmsNoteOn(unsigned char channel, unsigned char note, unsigned char velocity);

void cmsNoteOff(unsigned char channel, unsigned char note);

void cmsTick(void);

void cmsController(unsigned char channel, unsigned char id, unsigned char val);

#endif
