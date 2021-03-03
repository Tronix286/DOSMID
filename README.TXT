DOSMid Copyright (C) 2014-2018 Mateusz Viste
Creative Music System / Game Blaster output support version (C) Tronix 2021

Used piece of software codes from third-party peoples, eg:
- DEATHSHADOW'S Paku-Paku game by Jason M. Knight
- SAATunes v 1.04 lib by Bobcatmodder Jacob Field
- Arduino YM2149 MIDI Synth by yukimizake

New DOSMid options:

 /cms=XXX  Force dosmid to use Creative Music System / Game Blaster
           on port XXX. The port part is optional, that means you can 
           use "/cms" to just force CMS usage on default 220h port.


============================= ORIGINAL DOSMID.TXT
           
         DOSMid - a low-requirements MIDI and MUS player for DOS
                    http://dosmid.sourceforge.net


*** INTRO ***

DOSMid is a MIDI, MUS and RMID player for DOS. It's a real mode application
designed to run on very modest hardware configurations. It plays standard MIDI
files (both format 0 and format 1), as well as MIDI in RIFF (ie. RMID), and
even MUS tunes (as used by Id Software in numerous games like Doom, Heretic,
Hexen, etc).
DOSMid requires MIDI-capable hardware of some sort. Note, that some sound
cards have an MPU-401 interface, although many need an additional 'wavetable' daughterboard to produce actual MIDI sound.
DOSMid can also emulate MIDI through FM synthesis using an OPL chip (that is
one of the Yamaha YM3812 or YMF262 chips, found on most sound cards from the
nineties) - be warned however that, most of the time, such MIDI-over-OPL
emulation will yeld less than desirable results, unless the MIDI file was
specifically crafted for OPL.

Minimum requirements:
 - a compatible synthesizer (wavetable, OPL or external - see the compat list)
 - an 8086-compatible CPU
 - 100K of available conventional memory and some XMS
 - 250K or so available conventional memory if you do not have XMS
 - a video card able to display an 80x25 monochrome text mode

Highly recommended:
 - 80286 CPU for a guaranteed lag-free experience even on complex MIDI files
 - VGA graphic with a color monitor
 - 512K of available XMS memory


*** USAGE ***

During runtime, DOSMid can be controlled with the keyboard:
 ESC       Quits to DOS
 +/-       Volume up/down
 SPACE     Pause the song (press any key to resume)
 ENTER     Skip to next song of the playlist
 BKSPC     Jump to previous song of the playlist (doesn't work with /random)

DOSMid accepts several command-line options, as listed below:

DOSMID [options] file.mid (or m3u playlist)

 /noxms    Use conventional memory instead of XMS. This is obviously useful
           only if you don't have XMS. Don't use this option otherwise, since
           without XMS you won't be able to load big MIDI files.
 /xmsdelay Wait 2ms before each XMS access. Such waiting is required sometimes
           when the MPU controller is emulated by a TSR driver (specifically,
           the AWEUTIL driver used with SoundBlaster AWE 32/64 cards happens
           to crash if XMS accesses are not slightly delayed).
 /mpu=XXX  Force dosmid to use MPU-401 on port XXX. If not forced, DOSMID
           scans the BLASTER environment variable for the MPU port, and if not
           found, it fallbacks to port 330h. The port part is optional, that
           means you can use "/mpu" to just force MPU usage.
 /awe=XXX  Use the EMU8000 synth chip found on SoundBlaster AWE32/AWE64 cards
           on port XXX (the port is optional, you can specify just "/awe").
           KNOWN BUG: On some AWE cards, the FM music module becomes muted or
           noisy after using the EMU8000 chip. This is not a bug in DOSMid,
           and happens with other applications using AWE as well. If you have
           this problem, execute AWEUTIL /S after using DOSMid to reinit FM.
           I observed this problem on an AWE64 CT4390 ("Gold"), but not on an
           AWE32 CT2760.
 /opl=XXX  Use an OPL2-compatible chip on port XXX. This should be a last
           resort option if you don't have any wavetable device. Do NOT expect
           pleasing results. The port part is optional ("/opl" will default to
           port 388h).
 /sbmidi=XXX Drives an external synth connected to the gameport of your Sound
           Blaster card. The port part is optional ("/sbmidi" will use the
           port read from BLASTER, or fallback to 220h).
 /com=XXX  Send MIDI messages out via the RS232 port at I/O port XXX. This can
  or /com1 be used to hook a hardware synth to a computer with no MIDI
  or /com2 interface, only a standard serial port. DOSMid does NOT reconfigure
  or /com3 the COM port, so you should take care of setting it correctly, for
  or /com4 example using the 'MODE COM1: ...' command. The port part is not
           optional, you are expected to pass the hexadecimal I/O address of
           the RS232 port you wish to use (for example "/com=3f8" is pointing
           to COM1 on most BIOS implementations).
           It is also possible to use simpler "/com1", "/com2", "/com3" and
           "/com4" switches. These will autodetect the correct I/O port.
 /gus      Use the Gravis UltraSound card, relying on its ULTRAMID API.
 /syx=FILE Uses SYSEX instructions stored in FILE for initializing the MIDI
           device. FILE must be in "SYX" format, and can contain one or more
           SYSEX messages.
 /delay=X  Insert an extra delay of X ms (X being in the range 1..9000) before
           playing the MIDI file. Setting this to 100 or 200 might help in
           some cases where the sound hardware needs more time to initialize
           completely, or when complex sysex data is fed via the /syx option.
           You can also use this simply to make the silence longer between
           the files of your playlist.
 /sbnk=FILE Makes DOSMid load a custom sound bank. This is supported only on
           OPL and AWE hardware. The sound bank file must be in the IBK format
           when used with OPL, and SBK format for AWE. OPL accepts one or two
           IBK files (eg. /sbnk=file1.ibk,file2.ibk). If two are provided,
           then the first one will be used for the standard 128-instruments GM
           set, and the second one for defining percussion instruments.
 /log=FILE Logs all DOSMid activity to FILE. This is a debugging feature that
           you shouldn't be interested in. Beware, the log file can get pretty
           big (MUCH bigger than the MIDI file you are playing).
 /fullcpu  Do not let DOSMid being CPU-friendly. By default DOSMid issues an
           INT 28h when idle, to let the system be gentler on the CPU, but on
           some hardware this might lead to degraded sound performance.
 /dontstop Never ask the user to press a key after an error occurs. This is
           useful if you want to play a long playlist and don't care about
           bad MIDI files, simply skipping them (or if you play a single file
           and wish that DOSMid exit immediately if the file is unplayable).
 /random   randomize playlist order
 /nosound  Disable sound (not very useful for a music player!)

Note: All the above options can also be written to the DOSMID.CFG file. This
file is loaded by DOSMID and used as default parameters that can still be
overloaded by command-line options.


*** THE BLASTER VARIABLE ***

When not forced into a specific configuration via command-line switches,
DOSMid scans the BLASTER environment variable to find out the most desirable
settings. A BLASTER environment variable usually looks similar to this:

  SET BLASTER=A220 I5 D1 T3 P330 H6 E620

The bits DOSMid is interested in are "A220", "P330" and "E620". P330 provides
the port address of the MPU-401 MIDI interface, while E620 tells the port
address of the EMU8000 onboard synth (available only on 'AWE' models). A220,
on the other hand, provides the base I/O address of the SoundBlaster card, so
DOSMid can output directly to the card's MIDI port. If not instructed
otherwise, DOSMid will always try to use the EMU8000 synth if found in the
BLASTER string, and if not, it will use the MPU-401. If neither of them are
found in the BLASTER string, or if there is no BLASTER variable at all, then
DOSMid will try to fall back to FM synthesis on port 388h, unless an ULTRADIR
environment variable is present, in which case it will try to detect a GUS
through the ULTRAMID TSR API.


*** COMPATIBLE SOUND HARDWARE ***

DOSMid supports a variety of MIDI hardware. Here below I list the types of
hardware that DOSMid is able to talk to:

External MIDI synthesizers:
 - Connected through a MIDI/game port: either using an industry standard
   MPU-401 interface, or SoundBlaster MIDI port,
 - Connected through a "COM" (RS232) port: many synthesizers come with an
   RS-232 port that can be used instead of the standard MIDI port. This is
   supported by Roland SoundCanvas models, Yamaha PSR series, the Miracle
   Piano, many Korg devices... You could even use a software solution based
   on ttymidi...

Internal MIDI synthesizers:
 - Available through a virtual MPU-401 interface: some AzTech Waverider 32
   models, some versions of the HighScreen SoundBoostar 16, SoundBlaster 64
   cards using the 'AWEUTIL' MPU emulator,
 - Based on the EMU8000 chip (SoundBlaster 32, SoundBlaster 64),
 - Gravis UltraSound (GUS) cards, through their ULTRAMID driver,
 - Based on an OPL2 or OPL3-compatible chip (most ISA sound cards from the
   nineties: Adlib, all SoundBlaster ISA models, Opti, AzTech...).


*** BUILDING ***

DOSMid is compiled with OpenWatcom 1.9. The entire build process is automated
through a Makefile file, so if you wish to rebuild DOSMid, all you have to do
is type "wmake". A few compile-time options are available to disable features
you might not want: edit the Makefile and adapt FEATURES to your likeness. The
FEATURES list is documented in the Makefile. The default build comes with all
features compiled in, so you really should fiddle with FEATURES only in case
you desperately need to lower DOSMid's memory footprint.


*** CONTACT ***

If you enjoy DOSMid, or noticed any bugs, I'd love to hear about it! You won't
see my e-mail address here, but you will surely find some contact pointers on
my home page: http://mateusz.viste.fr


*** LICENSE ***

Copyright (C) 2014-2018 Mateusz Viste
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
