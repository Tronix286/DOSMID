#ifndef cms_h_sentinel
#define cms_h_sentinel

#define MAX_CMS_CHANNELS 12

typedef struct {
        unsigned char note;
        unsigned char priority;
} mid_channel;

void cmsReset(unsigned short port);

void cmsNoteOn(unsigned char channel, unsigned char note, unsigned char velocity);

void cmsNoteOff(unsigned char channel, unsigned char note);

void cmsTick(void);

void cmsController(unsigned char channel, unsigned char id, unsigned char val);

#endif
