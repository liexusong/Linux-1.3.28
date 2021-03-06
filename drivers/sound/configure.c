#define DISABLED_OPTIONS 	0
/*
 * sound/configure.c  - Configuration program for the Linux Sound Driver
 *
 * Copyright by Hannu Savolainen 1993
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <stdio.h>

#define B(x)	(1 << (x))

/*
 * Option numbers
 */

#define OPT_PAS		0
#define OPT_SB		1
#define OPT_ADLIB	2
#define OPT_LAST_MUTUAL	2

#define OPT_GUS		3
#define OPT_MPU401	4
#define OPT_UART6850	5
#define OPT_PSS		6
#define OPT_GUS16	7
#define OPT_GUSMAX	8
#define OPT_MSS		9
#define OPT_SSCAPE	10
#define OPT_TRIX	11
#define OPT_MAD16	12

#define OPT_HIGHLEVEL   13	/* This must be same than the next one */
#define OPT_SBPRO	13
#define OPT_SB16	14
#define OPT_AEDSP16     15
#define OPT_AUDIO	16
#define OPT_MIDI_AUTO	17
#define OPT_MIDI	18
#define OPT_YM3812_AUTO	19
#define OPT_YM3812	20
#define OPT_SEQUENCER	21
#define OPT_LAST	21	/* Last defined OPT number */

#define ANY_DEVS (B(OPT_AUDIO)|B(OPT_MIDI)|B(OPT_SEQUENCER)|B(OPT_GUS)| \
		  B(OPT_MPU401)|B(OPT_PSS)|B(OPT_GUS16)|B(OPT_GUSMAX)| \
		  B(OPT_MSS)|B(OPT_SSCAPE)|B(OPT_UART6850)|B(OPT_TRIX)| \
		  B(OPT_MAD16))
#define AUDIO_CARDS (B (OPT_PSS) | B (OPT_SB) | B (OPT_PAS) | B (OPT_GUS) | \
		B (OPT_MSS) | B (OPT_GUS16) | B (OPT_GUSMAX) | B (OPT_TRIX) | \
		B (OPT_SSCAPE)| B(OPT_MAD16))
#define MIDI_CARDS (B (OPT_PSS) | B (OPT_SB) | B (OPT_PAS) | B (OPT_MPU401) | \
		    B (OPT_GUS) | B (OPT_TRIX) | B (OPT_SSCAPE)|B(OPT_MAD16))
/*
 * Options that have been disabled for some reason (incompletely implemented
 * and/or tested). Don't remove from this list before looking at file
 * experimental.txt for further info.
 */

typedef struct
  {
    unsigned long   conditions;
    unsigned long   exclusive_options;
    char            macro[20];
    int             verify;
    int             alias;
    int             default_answ;
  }

hw_entry;


/*
 * The rule table for the driver options. The first field defines a set of
 * options which must be selected before this entry can be selected. The
 * second field is a set of options which are not allowed with this one. If
 * the fourth field is zero, the option is selected without asking
 * confirmation from the user.
 *
 * With this version of the rule table it is possible to select just one type of
 * hardware.
 *
 * NOTE!        Keep the following table and the questions array in sync with the
 * option numbering!
 */

hw_entry        hw_table[] =
{
/*
 * 0
 */
  {0, 0, "PAS", 1, 0, 0},
  {0, 0, "SB", 1, 0, 0},
  {0, B (OPT_PAS) | B (OPT_SB), "ADLIB", 1, 0, 0},

  {0, 0, "GUS", 1, 0, 0},
  {0, 0, "MPU401", 1, 0, 0},
  {0, 0, "UART6850", 1, 0, 0},
  {0, 0, "PSS", 1, 0, 0},
  {B (OPT_GUS), 0, "GUS16", 1, 0, 0},
  {B (OPT_GUS), B (OPT_GUS16), "GUSMAX", 1, 0, 0},
  {0, 0, "MSS", 1, 0, 0},
  {0, 0, "SSCAPE", 1, 0, 0},
  {0, 0, "TRIX", 1, 0, 0},
  {0, 0, "MAD16", 1, 0, 0},

  {B (OPT_SB), B (OPT_PAS), "SBPRO", 1, 0, 1},
  {B (OPT_SB) | B (OPT_SBPRO), B (OPT_PAS), "SB16", 1, 0, 1},
  {B (OPT_SBPRO) | B (OPT_MSS) | B (OPT_MPU401), 0, "AEDSP16", 1, 0, 0},
  {AUDIO_CARDS, 0, "AUDIO", 1, 0, 1},
  {B (OPT_MPU401), 0, "MIDI_AUTO", 0, OPT_MIDI, 0},
  {MIDI_CARDS, 0, "MIDI", 1, 0, 1},
  {B (OPT_ADLIB), 0, "YM3812_AUTO", 0, OPT_YM3812, 0},
  {B (OPT_PSS) | B (OPT_SB) | B (OPT_PAS) | B (OPT_ADLIB) | B (OPT_MSS) | B (OPT_PSS), B (OPT_YM3812_AUTO), "YM3812", 1, 0, 1},
  {B (OPT_MIDI) | B (OPT_YM3812) | B (OPT_YM3812_AUTO) | B (OPT_GUS), 0, "SEQUENCER", 0, 0, 1}
};

char           *questions[] =
{
  "ProAudioSpectrum 16 support",
  "SoundBlaster support",
  "Generic OPL2/OPL3 FM synthesizer support",
  "Gravis Ultrasound support",
  "MPU-401 support (NOT for SB16)",
  "6850 UART Midi support",
  "PSS (ECHO-ADI2111) support",
  "16 bit sampling option of GUS (_NOT_ GUS MAX)",
  "GUS MAX support",
  "Microsoft Sound System support",
  "Ensoniq Soundscape support",
  "MediaTriX AudioTriX Pro support",
  "Support for MAD16 and/or Mozart based cards",

  "SoundBlaster Pro support",
  "SoundBlaster 16 support",
  "Audio Excel DSP 16 initialization support",
  "/dev/dsp and /dev/audio supports (usually required)",
  "This should not be asked",
  "MIDI interface support",
  "This should not be asked",
  "FM synthesizer (YM3812/OPL-3) support",
  "/dev/sequencer support",
  "Is the sky really falling"
};

unsigned long   selected_options = 0;
int             sb_dma = 0;

#include "hex2hex.h"
int             bin2hex (char *path, char *target, char *varname);

int
can_select_option (int nr)
{
#if 0
  switch (nr)
    {
    case 0:
      fprintf (stderr, "The SoundBlaster, AdLib and ProAudioSpectrum\n"
	       "CARDS cannot be installed at the same time.\n\n"
	       "However the PAS16 has a SB emulator so you could select"
	       "the SoundBlaster DRIVER with it.\n");
      fprintf (stderr, "	- ProAudioSpectrum 16\n");
      fprintf (stderr, "	- SoundBlaster / SB Pro\n");
      fprintf (stderr, "          (Could be selected with a PAS16 also)\n");
      fprintf (stderr, "	- AdLib\n");
      fprintf (stderr, "\nDon't enable SoundBlaster if you have GUS at 0x220!\n\n");
      break;

    case OPT_LAST_MUTUAL + 1:
      fprintf (stderr, "\nThe following cards should work with any other cards.\n"
	       "CAUTION! Don't enable MPU-401 if you don't have it.\n");
      break;

    case OPT_HIGHLEVEL:
      fprintf (stderr, "\nSelect one or more of the following options\n");
      break;


    }
#endif

  if (hw_table[nr].conditions)
    if (!(hw_table[nr].conditions & selected_options))
      return 0;

  if (hw_table[nr].exclusive_options)
    if (hw_table[nr].exclusive_options & selected_options)
      return 0;

  if (DISABLED_OPTIONS & B (nr))
    return 0;

  return 1;
}

int
think_positively (int def_answ)
{
  char            answ[512];
  int             len;

  if ((len = read (0, &answ, sizeof (answ))) < 1)
    {
      fprintf (stderr, "\n\nERROR! Cannot read stdin\n");

      perror ("stdin");
      printf ("#undef CONFIGURE_SOUNDCARD\n");
      printf ("#undef KERNEL_SOUNDCARD\n");
      exit (-1);
    }

  if (len < 2)			/*
				 * There is an additional LF at the end
				 */
    return def_answ;

  answ[len - 1] = 0;

  if (!strcmp (answ, "y") || !strcmp (answ, "Y"))
    return 1;

  return 0;
}

int
ask_value (char *format, int default_answer)
{
  char            answ[512];
  int             len, num;

play_it_again_Sam:

  if ((len = read (0, &answ, sizeof (answ))) < 1)
    {
      fprintf (stderr, "\n\nERROR! Cannot read stdin\n");

      perror ("stdin");
      printf ("#undef CONFIGURE_SOUNDCARD\n");
      printf ("#undef KERNEL_SOUNDCARD\n");
      exit (-1);
    }

  if (len < 2)			/*
				 * There is an additional LF at the end
				 */
    return default_answer;

  answ[len - 1] = 0;

  if (sscanf (answ, format, &num) != 1)
    {
      fprintf (stderr, "Illegal format. Try again: ");
      goto play_it_again_Sam;
    }

  return num;
}

int
main (int argc, char *argv[])
{
  int             i, num, def_size, full_driver = 1;
  char            answ[10];

  printf ("/*\tGenerated by configure. Don't edit!!!!\t*/\n\n");

  fprintf (stderr, "\nConfiguring the sound support\n\n");
#if 0
  /*
     * The full driver appeared to be impossible to compile and boot.
     * There are too much supported cards now.
   */
  fprintf (stderr, "Do you want to include full version of the sound driver (n/y) ? ");

  if (think_positively (0))
    {
      /*
         * Select all but some most dangerous cards. These cards are difficult to
         * detect reliably or conflict with some other cards (SCSI, Mitsumi)
       */
      selected_options = 0xffffffff &
	~(B (OPT_MPU401) | B (OPT_UART6850) | B (OPT_PSS)) &
	~DISABLED_OPTIONS;

      fprintf (stderr, "Note! MPU-401, PSS and 6850 UART drivers not enabled\n");
      full_driver = 1;
    }
  else
#endif
    {
#if 0
      fprintf (stderr, "Do you want to DISABLE the Sound Driver (n/y) ?");
      if (think_positively (0))
	{
	  printf ("#undef CONFIGURE_SOUNDCARD\n");
	  printf ("#undef KERNEL_SOUNDCARD\n");
	  exit (0);
	}
#endif
      /*
       * Partial driver
       */

      full_driver = 0;

      for (i = 0; i <= OPT_LAST; i++)
	if (can_select_option (i))
	  {
	    if (!(selected_options & B (i)))	/*
						 * Not selected yet
						 */
	      if (!hw_table[i].verify)
		{
		  if (hw_table[i].alias)
		    selected_options |= B (hw_table[i].alias);
		  else
		    selected_options |= B (i);
		}
	      else
		{
		  int             def_answ = hw_table[i].default_answ;

		  fprintf (stderr,
			   def_answ ? "  %s (y/n) ? " : "  %s (n/y) ? ",
			   questions[i]);
		  if (think_positively (def_answ))
		    if (hw_table[i].alias)
		      selected_options |= B (hw_table[i].alias);
		    else
		      selected_options |= B (i);
		}
	  }
    }

  if (selected_options & B (OPT_SBPRO))
    {
      fprintf (stderr, "Do you want support for the mixer of SG NX Pro ? ");
      if (think_positively (0))
	printf ("#define __SGNXPRO__\n");
    }

  if (selected_options & B (OPT_SB))
    {
      fprintf (stderr, "Do you want support for the MV Jazz16 (ProSonic etc.) ? ");
      if (think_positively (0))
	{
	  printf ("#define JAZZ16\n");
	  do
	    {
	      fprintf (stderr, "\tValid 16 bit DMA channels for ProSonic/Jazz 16 are\n");
	      fprintf (stderr, "\t1, 3, 5 (default), 7\n");
	      fprintf (stderr, "\tEnter 16bit DMA channel for Prosonic : ");
	      num = ask_value ("%d", 5);
	    }
	  while (num != 1 && num != 3 && num != 5 && num != 7);
	  fprintf (stderr, "ProSonic 16 bit DMA set to %d\n", num);
	  printf ("#define JAZZ_DMA16 %d\n", num);

	  fprintf (stderr, "Do you have SoundMan Wave (n/y) ? ");

	  if (think_positively (0))
	    {
	      printf ("#define SM_WAVE\n");

	    midi0001_again:
	      fprintf
		(stderr,
		 "Logitech SoundMan Wave has a microcontroller which must be initialized\n"
		 "before MIDI emulation works. This is possible only if the microcode\n"
		 "file is compiled into the driver.\n"
		 "Do you have access to the MIDI0001.BIN file (y/n) ? ");
	      if (think_positively (1))
		{
		  char            path[512];

		  fprintf (stderr,
			   "Enter full name of the MIDI0001.BIN file (pwd is sound): ");
		  scanf ("%s", path);
		  fprintf (stderr, "including microcode file %s\n", path);

		  if (!bin2hex (path, "smw-midi0001.h", "smw_ucode"))
		    {
		      fprintf (stderr, "couldn't open %s file\n",
			       path);
		      fprintf (stderr, "try again with correct path? ");
		      if (think_positively (1))
			goto midi0001_again;
		    }
		  else
		    printf ("#define SMW_MIDI0001_INCLUDED\n");
		}
	    }
	}
    }

  if (selected_options & B (OPT_SBPRO))
    {
      fprintf (stderr, "\n\nThe Logitech SoundMan Games supports 44 kHz in stereo\n"
	       "while the standard SB Pro supports just 22 kHz/stereo\n"
	       "You have an option to enable the SM Games mode.\n"
	       "However do enable it only if you are _sure_ that your\n"
	       "card is a SM Games. Enabling this feature with a\n"
	       "plain old SB Pro _will_ cause troubles with stereo mode.\n"
	       "\n"
	       "DANGER! Read the above once again before answering 'y'\n"
	       "Answer 'n' in case you are unsure what to do!\n");
      fprintf (stderr, "Do you have a Logitech SoundMan Games (n/y) ? ");
      if (think_positively (0))
	printf ("#define SM_GAMES\n");
    }

  if (selected_options & B (OPT_SB16))
    selected_options |= B (OPT_SBPRO);

  if (selected_options & B (OPT_AEDSP16))
    {
      int             sel1 = 0;

      if (selected_options & B (OPT_SBPRO))
	{
	  fprintf (stderr, "Do you want support for the Audio Excel SoundBlaster pro mode ? ");
	  if (think_positively (1))
	    {
	      printf ("#define AEDSP16_SBPRO\n");
	      sel1 = 1;
	    }
	}

      if ((selected_options & B (OPT_MSS)) && (sel1 == 0))
	{
	  fprintf (stderr, "Do you want support for the Audio Excel Microsoft Sound System mode? ");
	  if (think_positively (1))
	    {
	      printf ("#define AEDSP16_MSS\n");
	      sel1 = 1;
	    }
	}

      if (sel1 == 0)
	{
	  printf ("#undef CONFIGURE_SOUNDCARD\n");
	  printf ("#undef KERNEL_SOUNDCARD\n");
	  fprintf (stderr, "ERROR!!!!!\nYou loose: you must select at least one mode when using Audio Excel!\n");
	  exit (-1);
	}
      if (selected_options & B (OPT_MPU401))
	printf ("#define AEDSP16_MPU401\n");
    }

  if (selected_options & B (OPT_PSS))
    {
    genld_again:
      fprintf
	(stderr,
       "if you wish to emulate the soundblaster and you have a DSPxxx.LD.\n"
	 "then you must include the LD in the kernel.\n"
	 "Do you wish to include a LD (y/n) ? ");
      if (think_positively (1))
	{
	  char            path[512];

	  fprintf (stderr,
		   "Enter the path to your LD file (pwd is sound): ");
	  scanf ("%s", path);
	  fprintf (stderr, "including LD file %s\n", path);

	  if (!bin2hex (path, "synth-ld.h", "pss_synth"))
	    {
	      fprintf (stderr, "couldn't open %s as the ld file\n",
		       path);
	      fprintf (stderr, "try again with correct path? ");
	      if (think_positively (1))
		goto genld_again;
	    }
	}
      else
	{
	  FILE           *sf = fopen ("synth-ld.h", "w");

	  fprintf (sf, "/* automaticaly generated by configure */\n");
	  fprintf (sf, "unsigned char pss_synth[1];\n"
		   "#define pss_synthLen 0\n");
	  fclose (sf);
	}
    }

  if (selected_options & B (OPT_TRIX))
    {
    hex2hex_again:
      fprintf (stderr, "MediaTriX audioTriX Pro has a onboard microcontroller\n"
	       "which needs to be initialized by downloading\n"
	       "the code from file TRXPRO.HEX in the DOS driver\n"
	       "directory. If you don't have the TRXPRO.HEX handy\n"
	       "you may skip this step. However SB and MPU-401\n"
	       "modes of AudioTriX Pro will not work without\n"
	       "this file!\n"
	       "\n"
	       "Do you want to include TRXPRO.HEX in your kernel (y/n) ? ");

      if (think_positively (1))
	{
	  char            path[512];

	  fprintf (stderr,
		 "Enter the path to your TRXPRO.HEX file (pwd is sound): ");
	  scanf ("%s", path);
	  fprintf (stderr, "including HEX file %s\n", path);

	  if (!hex2hex (path, "trix_boot.h", "static unsigned char trix_boot"))
	    goto hex2hex_again;
	  printf ("#define INCLUDE_TRIX_BOOT\n");
	}
    }

  if (!(selected_options & ANY_DEVS))
    {
      printf ("#undef CONFIGURE_SOUNDCARD\n");
      printf ("#undef KERNEL_SOUNDCARD\n");
      fprintf (stderr, "\n*** This combination is useless. Sound driver disabled!!! ***\n\n");
      exit (0);
    }
  else
    printf ("#define KERNEL_SOUNDCARD\n");

  for (i = 0; i <= OPT_LAST; i++)
    if (!hw_table[i].alias)
      if (selected_options & B (i))
	printf ("#undef  EXCLUDE_%s\n", hw_table[i].macro);
      else
	printf ("#define EXCLUDE_%s\n", hw_table[i].macro);


  /*
   * IRQ and DMA settings
   */
  printf ("\n");

#if defined(linux)
  if (selected_options & B (OPT_AEDSP16))
    {
      fprintf (stderr, "\nI/O base for Audio Excel DSP 16 ?\n"
	       "Warning:\n"
	       "If you are using Audio Excel SoundBlaster emulation,\n"
	"you must use the same I/O base for Audio Excel and SoundBlaster.\n"
	       "The factory default is 220 (other possible value is 240)\n"
	       "Enter the Audio Excel DSP 16 I/O base: ");

      num = ask_value ("%x", 0x220);
      fprintf (stderr, "Audio Excel DSP 16 I/O base set to %03x\n", num);
      printf ("#define AEDSP16_BASE 0x%03x\n", num);
    }

  if ((selected_options & B (OPT_SB)) && selected_options & (B (OPT_AUDIO) | B (OPT_MIDI)))
    {
      fprintf (stderr, "\nI/O base for SB?\n"
	       "The factory default is 220\n"
	       "Enter the SB I/O base: ");

      num = ask_value ("%x", 0x220);
      fprintf (stderr, "SB I/O base set to %03x\n", num);
      printf ("#define SBC_BASE 0x%03x\n", num);

      fprintf (stderr, "\nIRQ number for SoundBlaster?\n"
	       "The IRQ address is defined by the jumpers on your card.\n"
	  "The factory default is either 5 or 7 (depending on the model).\n"
	       "Valid values are 9(=2), 5, 7 and 10.\n"
	       "Enter the value: ");

      num = ask_value ("%d", 7);
      if (num != 9 && num != 5 && num != 7 && num != 10)
	{

	  fprintf (stderr, "*** Illegal input! ***\n");
	  num = 7;
	}
      fprintf (stderr, "SoundBlaster IRQ set to %d\n", num);

      printf ("#define SBC_IRQ %d\n", num);

      if (selected_options & (B (OPT_SBPRO) | B (OPT_PAS)))
	{
	  fprintf (stderr, "\nDMA channel for SoundBlaster?\n"
		   "For SB 1.0, 1.5 and 2.0 this MUST be 1\n"
		   "SB Pro supports DMA channels 0, 1 and 3 (jumper)\n"
		   "For SB16 give the 8 bit DMA# here\n"
		   "The default value is 1\n"
		   "Enter the value: ");

	  num = ask_value ("%d", 1);
	  if (num < 0 || num > 3)
	    {

	      fprintf (stderr, "*** Illegal input! ***\n");
	      num = 1;
	    }
	  fprintf (stderr, "SoundBlaster DMA set to %d\n", num);
	  printf ("#define SBC_DMA %d\n", num);
	  sb_dma = num;
	}

      if (selected_options & B (OPT_SB16))
	{

	  fprintf (stderr, "\n16 bit DMA channel for SoundBlaster 16?\n"
		   "Possible values are 5, 6 or 7\n"
		   "The default value is 6\n"
		   "Enter the value: ");

	  num = ask_value ("%d", 6);
	  if ((num < 5 || num > 7) && (num != sb_dma))
	    {

	      fprintf (stderr, "*** Illegal input! ***\n");
	      num = 6;
	    }
	  fprintf (stderr, "SoundBlaster DMA set to %d\n", num);
	  printf ("#define SB16_DMA %d\n", num);

	  fprintf (stderr, "\nI/O base for SB16 Midi?\n"
		   "Possible values are 300 and 330\n"
		   "The factory default is 330\n"
		   "Enter the SB16 Midi I/O base: ");

	  num = ask_value ("%x", 0x330);
	  fprintf (stderr, "SB16 Midi I/O base set to %03x\n", num);
	  printf ("#define SB16MIDI_BASE 0x%03x\n", num);
	}
    }

  if (selected_options & B (OPT_PAS))
    {

      if (selected_options & (B (OPT_AUDIO) | B (OPT_MIDI)))
	{
	  fprintf (stderr, "\nIRQ number for ProAudioSpectrum?\n"
		   "The recommended value is the IRQ used under DOS.\n"
		   "Please refer to the ProAudioSpectrum User's Guide.\n"
		   "The default value is 10.\n"
		   "Enter the value: ");

	  num = ask_value ("%d", 10);
	  if (num == 6 || num < 3 || num > 15 || num == 2)	/*
								 * Illegal
								 */
	    {

	      fprintf (stderr, "*** Illegal input! ***\n");
	      num = 10;
	    }
	  fprintf (stderr, "ProAudioSpectrum IRQ set to %d\n", num);
	  printf ("#define PAS_IRQ %d\n", num);
	}

      if (selected_options & B (OPT_AUDIO))
	{
	  fprintf (stderr, "\nDMA number for ProAudioSpectrum?\n"
		   "The recommended value is the DMA channel under DOS.\n"
		   "Please refer to the ProAudioSpectrum User's Guide.\n"
		   "The default value is 3\n"
		   "Enter the value: ");

	  num = ask_value ("%d", 3);
	  if (num == 4 || num < 0 || num > 7)
	    {

	      fprintf (stderr, "*** Illegal input! ***\n");
	      num = 3;
	    }
	  fprintf (stderr, "\nProAudioSpectrum DMA set to %d\n", num);
	  printf ("#define PAS_DMA %d\n", num);
	}
      fprintf (stderr, "\nEnable Joystick port on ProAudioSpectrum (n/y) ? ");
      if (think_positively (0))
	printf ("#define PAS_JOYSTICK_ENABLE\n");

      fprintf (stderr, "PAS16 could be noisy with some mother boards\n"
	       "There is a command line switch (was it :T?)\n"
	       "in the DOS driver for PAS16 which solves this.\n"
	       "Don't enable this feature unless you have problems!\n"
	       "Do you have to use this switch with DOS (y/n) ?");
      if (think_positively (0))
	printf ("#define BROKEN_BUS_CLOCK\n");
    }

  if (selected_options & B (OPT_GUS))
    {
      fprintf (stderr, "\nI/O base for Gravis Ultrasound?\n"
	       "Valid choices are 210, 220, 230, 240, 250 or 260\n"
	       "The factory default is 220\n"
	       "Enter the GUS I/O base: ");

      num = ask_value ("%x", 0x220);
      if ((num > 0x260) || ((num & 0xf0f) != 0x200) || ((num & 0x0f0) > 0x060))
	{

	  fprintf (stderr, "*** Illegal input! ***\n");
	  num = 0x220;
	}

      if ((selected_options & B (OPT_SB)) && (num == 0x220))
	{
	  fprintf (stderr, "FATAL ERROR!!!!!!!!!!!!!!\n"
		   "\t0x220 cannot be used if SoundBlaster is enabled.\n"
		   "\tRun the config again.\n");
	  printf ("#undef CONFIGURE_SOUNDCARD\n");
	  printf ("#undef KERNEL_SOUNDCARD\n");
	  exit (-1);
	}
      fprintf (stderr, "GUS I/O base set to %03x\n", num);
      printf ("#define GUS_BASE 0x%03x\n", num);

      fprintf (stderr, "\nIRQ number for Gravis UltraSound?\n"
	       "The recommended value is the IRQ used under DOS.\n"
	       "Please refer to the Gravis Ultrasound User's Guide.\n"
	       "The default value is 15.\n"
	       "Enter the value: ");

      num = ask_value ("%d", 15);
      if (num == 6 || num < 3 || num > 15 || num == 2)	/*
							 * Invalid
							 */
	{

	  fprintf (stderr, "*** Illegal input! ***\n");
	  num = 15;
	}
      fprintf (stderr, "Gravis UltraSound IRQ set to %d\n", num);
      printf ("#define GUS_IRQ %d\n", num);

      fprintf (stderr, "\nDMA number for Gravis UltraSound?\n"
	       "The recommended value is the DMA channel under DOS.\n"
	       "Please refer to the Gravis Ultrasound User's Guide.\n"
	       "The default value is 6\n"
	       "Enter the value: ");

      num = ask_value ("%d", 6);
      if (num == 4 || num < 0 || num > 7)
	{
	  fprintf (stderr, "*** Illegal input! ***\n");
	  num = 6;
	}
      fprintf (stderr, "\nGravis UltraSound DMA set to %d\n", num);
      printf ("#define GUS_DMA %d\n", num);

      if (selected_options & B (OPT_GUSMAX))
	{
	  fprintf (stderr, "\nSecond DMA channel for GUS MAX (optional)?\n"
		   "The default value is 7 (0 disables)\n"
		   "Enter the value: ");

	  num = ask_value ("%d", 7);
	  if (num > 0)
	    {
	      if (num > 7)
		{
		  fprintf (stderr, "*** Illegal input! ***\n");
		  num = 7;
		}

	      fprintf (stderr, "\nGUSMAX DMA set to %d\n", num);
	      printf ("#define GUSMAX_DMA %d\n", num);
	    }
	}

      if (selected_options & B (OPT_GUS16))
	{
	  fprintf (stderr, "\nI/O base for GUS16 (GUS 16 bit sampling option)?\n"
		   "The factory default is 530\n"
		   "Other possible values are  604, E80 or F40\n"
		   "Enter the GUS16 I/O base: ");

	  num = ask_value ("%x", 0x530);
	  fprintf (stderr, "GUS16 I/O base set to %03x\n", num);
	  printf ("#define GUS16_BASE 0x%03x\n", num);

	  fprintf (stderr, "\nIRQ number for GUS16?\n"
		   "Valid numbers are: 3, 4, 5, 7, or 9(=2).\n"
		   "The default value is 7.\n"
		   "Enter the value: ");

	  num = ask_value ("%d", 7);
	  if (num == 6 || num < 3 || num > 15)
	    {
	      fprintf (stderr, "*** Illegal input! ***\n");
	      num = 7;
	    }
	  fprintf (stderr, "GUS16 IRQ set to %d\n", num);
	  printf ("#define GUS16_IRQ %d\n", num);

	  fprintf (stderr, "\nDMA number for GUS16?\n"
		   "The default value is 3\n"
		   "Enter the value: ");

	  num = ask_value ("%d", 3);
	  if (num < 0 || num > 3)
	    {
	      fprintf (stderr, "*** Illegal input! ***\n");
	      num = 3;
	    }
	  fprintf (stderr, "\nGUS16 DMA set to %d\n", num);
	  printf ("#define GUS16_DMA %d\n", num);
	}
    }

  if (selected_options & B (OPT_MPU401))
    {
      fprintf (stderr, "\nI/O base for MPU-401?\n"
	       "The factory default is 330\n"
	       "Enter the MPU-401 I/O base: ");

      num = ask_value ("%x", 0x330);
      fprintf (stderr, "MPU-401 I/O base set to %03x\n", num);
      printf ("#define MPU_BASE 0x%03x\n", num);

      fprintf (stderr, "\nIRQ number for MPU-401?\n"
	       "Valid numbers are: 3, 4, 5, 7 and 9(=2).\n"
	       "The default value is 9.\n"
	       "Enter the value: ");

      num = ask_value ("%d", 9);
      if (num == 6 || num < 3 || num > 15)
	{

	  fprintf (stderr, "*** Illegal input! ***\n");
	  num = 5;
	}
      fprintf (stderr, "MPU-401 IRQ set to %d\n", num);
      printf ("#define MPU_IRQ %d\n", num);
    }

  if (selected_options & B (OPT_UART6850))
    {
      fprintf (stderr, "\nI/O base for 6850 UART Midi?\n"
	       "Be carefull. No defaults.\n"
	       "Enter the 6850 UART I/O base: ");

      num = ask_value ("%x", 0);
      if (num == 0)
	{
	  /*
	     * Invalid value entered
	   */
	  printf ("#define EXCLUDE_UART6850\n");
	}
      else
	{
	  fprintf (stderr, "6850 UART I/O base set to %03x\n", num);
	  printf ("#define U6850_BASE 0x%03x\n", num);

	  fprintf (stderr, "\nIRQ number for 6850 UART?\n"
		   "Valid numbers are: 3, 4, 5, 7 and 9(=2).\n"
		   "The default value is 5.\n"
		   "Enter the value: ");

	  num = ask_value ("%d", 5);
	  if (num == 6 || num < 3 || num > 15)
	    {

	      fprintf (stderr, "*** Illegal input! ***\n");
	      num = 5;
	    }
	  fprintf (stderr, "6850 UART IRQ set to %d\n", num);
	  printf ("#define U6850_IRQ %d\n", num);
	}
    }

  if (selected_options & B (OPT_PSS))
    {
      fprintf (stderr, "\nI/O base for PSS?\n"
	       "The factory default is 220 (240 also possible)\n"
	       "Enter the PSS I/O base: ");

      num = ask_value ("%x", 0x220);
      fprintf (stderr, "PSS I/O base set to %03x\n", num);
      printf ("#define PSS_BASE 0x%03x\n", num);

      fprintf (stderr, "\nIRQ number for PSS?\n"
	       "Valid numbers are: 3, 4, 5, 7, 9(=2) or 10.\n"
	       "The default value is 10.\n"
	       "Enter the value: ");

      num = ask_value ("%d", 10);
      if (num == 6 || num < 3 || num > 15)
	{
	  fprintf (stderr, "*** Illegal input! ***\n");
	  num = 7;
	}
      fprintf (stderr, "PSS IRQ set to %d\n", num);
      printf ("#define PSS_IRQ %d\n", num);

      fprintf (stderr, "\nDMA number for ECHO-PSS?\n"
	       "The default value is 5\n"
	       "Valid values are 5, 6 and 7\n"
	       "Enter the value: ");

      num = ask_value ("%d", 5);
      if (num < 5 || num > 7)
	{
	  fprintf (stderr, "*** Illegal input! ***\n");
	  num = 5;
	}
      fprintf (stderr, "\nECHO-PSS DMA set to %d\n", num);
      printf ("#define PSS_DMA %d\n", num);

      fprintf (stderr, "\nMSS (MS Sound System) I/O base for the PSS card?\n"
	       "The factory default is 530\n"
	       "Other possible values are  604, E80 or F40\n"
	       "Enter the MSS I/O base: ");

      num = ask_value ("%x", 0x530);
      fprintf (stderr, "PSS/MSS I/O base set to %03x\n", num);
      printf ("#define PSS_MSS_BASE 0x%03x\n", num);

      fprintf (stderr, "\nIRQ number for the MSS mode of PSS ?\n"
	       "Valid numbers are: 7, 9(=2), 10 and 11.\n"
	       "The default value is 11.\n"
	       "Enter the value: ");

      num = ask_value ("%d", 11);
      if (num == 6 || num < 3 || num > 15)
	{
	  fprintf (stderr, "*** Illegal input! ***\n");
	  num = 11;
	}
      fprintf (stderr, "PSS/MSS IRQ set to %d\n", num);
      printf ("#define PSS_MSS_IRQ %d\n", num);

      fprintf (stderr, "\nMSS DMA number for PSS?\n"
	       "Valid values are 0, 1 and 3.\n"
	       "The default value is 3\n"
	       "Enter the value: ");

      num = ask_value ("%d", 3);
      if (num == 4 || num < 0 || num > 7)
	{
	  fprintf (stderr, "*** Illegal input! ***\n");
	  num = 3;
	}
      fprintf (stderr, "\nPSS/MSS DMA set to %d\n", num);
      printf ("#define PSS_MSS_DMA %d\n", num);

      fprintf (stderr, "\nMIDI I/O base for PSS?\n"
	       "The factory default is 330\n"
	       "Enter the PSS MIDI I/O base: ");

      num = ask_value ("%x", 0x330);
      fprintf (stderr, "PSS/MIDI I/O base set to %03x\n", num);
      printf ("#define PSS_MPU_BASE 0x%03x\n", num);

      fprintf (stderr, "\nIRQ number for PSS MIDI?\n"
	       "Valid numbers are: 3, 4, 5, 7 and 9(=2).\n"
	       "The default value is 9.\n"
	       "Enter the value: ");

      num = ask_value ("%d", 9);
      if (num == 6 || num < 3 || num > 15)
	{

	  fprintf (stderr, "*** Illegal input! ***\n");
	  num = 5;
	}
      fprintf (stderr, "PSS MIDI IRQ set to %d\n", num);
      printf ("#define PSS_MPU_IRQ %d\n", num);
    }

  if (selected_options & B (OPT_MSS))
    {
      fprintf (stderr, "\nI/O base for MSS (MS Sound System)?\n"
	       "The factory default is 530\n"
	       "Other possible values are  604, E80 or F40\n"
	       "Enter the MSS I/O base: ");

      num = ask_value ("%x", 0x530);
      fprintf (stderr, "MSS I/O base set to %03x\n", num);
      printf ("#define MSS_BASE 0x%03x\n", num);

      fprintf (stderr, "\nIRQ number for MSS?\n"
	       "Valid numbers are: 7, 9(=2), 10 and 11.\n"
	       "The default value is 10.\n"
	       "Enter the value: ");

      num = ask_value ("%d", 10);
      if (num == 6 || num < 3 || num > 15)
	{
	  fprintf (stderr, "*** Illegal input! ***\n");
	  num = 7;
	}
      fprintf (stderr, "MSS IRQ set to %d\n", num);
      printf ("#define MSS_IRQ %d\n", num);

      fprintf (stderr, "\nDMA number for MSS?\n"
	       "Valid values are 0, 1 and 3.\n"
	       "The default value is 3\n"
	       "Enter the value: ");

      num = ask_value ("%d", 3);
      if (num == 4 || num < 0 || num > 7)
	{
	  fprintf (stderr, "*** Illegal input! ***\n");
	  num = 3;
	}
      fprintf (stderr, "\nMSS DMA set to %d\n", num);
      printf ("#define MSS_DMA %d\n", num);
    }

  if (selected_options & B (OPT_SSCAPE))
    {
      int             reveal_spea;

      fprintf (stderr, "\n(MIDI) I/O base for Ensoniq Soundscape?\n"
	       "The factory default is 330\n"
	       "Other possible values are 320, 340 or 350\n"
	       "Enter the Soundscape I/O base: ");

      num = ask_value ("%x", 0x330);
      fprintf (stderr, "Soundscape I/O base set to %03x\n", num);
      printf ("#define SSCAPE_BASE 0x%03x\n", num);

      fprintf (stderr, "Is your SoundScape card made/marketed by Reveal or Spea? ");
      reveal_spea = think_positively (0);
      if (reveal_spea)
	printf ("#define REVEAL_SPEA\n");

      fprintf (stderr, "\nIRQ number for Soundscape?\n");

      if (reveal_spea)
	fprintf (stderr, "Check valid interrupts from the manual of your card.\n");
      else
	fprintf (stderr, "Valid numbers are: 5, 7, 9(=2) and 10.\n");

      fprintf (stderr, "The default value is 9.\n"
	       "Enter the value: ");

      num = ask_value ("%d", 9);
      if (num == 6 || num < 3 || num > 15)
	{
	  fprintf (stderr, "*** Illegal input! ***\n");
	  num = 9;
	}
      fprintf (stderr, "Soundscape IRQ set to %d\n", num);
      printf ("#define SSCAPE_IRQ %d\n", num);

      fprintf (stderr, "\nDMA number for Soundscape?\n"
	       "Valid values are 1 and 3 (sometimes 0)"
	       "The default value is 3\n"
	       "Enter the value: ");

      num = ask_value ("%d", 3);
      if (num == 4 || num < 0 || num > 7)
	{
	  fprintf (stderr, "*** Illegal input! ***\n");
	  num = 3;
	}
      fprintf (stderr, "\nSoundscape DMA set to %d\n", num);
      printf ("#define SSCAPE_DMA %d\n", num);

      fprintf (stderr, "\nMSS (MS Sound System) I/O base for the SSCAPE card?\n"
	       "The factory default is 534\n"
	       "Other possible values are  608, E84 or F44\n"
	       "Enter the MSS I/O base: ");

      num = ask_value ("%x", 0x534);
      fprintf (stderr, "SSCAPE/MSS I/O base set to %03x\n", num);
      printf ("#define SSCAPE_MSS_BASE 0x%03x\n", num);

      fprintf (stderr, "\nIRQ number for the MSS mode of SSCAPE ?\n");
      if (reveal_spea)
	fprintf (stderr, "Valid numbers are: 5, 7, 9(=2) and 15.\n");
      else
	fprintf (stderr, "Valid numbers are: 5, 7, 9(=2) and 10.\n");
      fprintf (stderr, "The default value is 5.\n"
	       "Enter the value: ");

      num = ask_value ("%d", 5);
      if (num == 6 || num < 3 || num > 15)
	{
	  fprintf (stderr, "*** Illegal input! ***\n");
	  num = 10;
	}
      fprintf (stderr, "SSCAPE/MSS IRQ set to %d\n", num);
      printf ("#define SSCAPE_MSS_IRQ %d\n", num);

      fprintf (stderr, "\nMSS DMA number for SSCAPE?\n"
	       "Valid values are 0, 1 and 3.\n"
	       "The default value is 3\n"
	       "Enter the value: ");

      num = ask_value ("%d", 3);
      if (num == 4 || num < 0 || num > 7)
	{
	  fprintf (stderr, "*** Illegal input! ***\n");
	  num = 3;
	}
      fprintf (stderr, "\nSSCAPE/MSS DMA set to %d\n", num);
      printf ("#define SSCAPE_MSS_DMA %d\n", num);
    }
  if (selected_options & B (OPT_TRIX))
    {

      fprintf (stderr, "\nWindows Sound System I/O base for the AudioTriX card?\n"
	       "The factory default is 530\n"
	       "Other possible values are  604, E80 or F40\n"
	       "Enter the MSS I/O base: ");

      num = ask_value ("%x", 0x530);
      fprintf (stderr, "AudioTriX MSS I/O base set to %03x\n", num);
      printf ("#define TRIX_BASE 0x%03x\n", num);

      fprintf (stderr, "\nIRQ number for the WSS mode of AudioTriX ?\n"
	       "Valid numbers are: 5, 7, 9(=2), 10 and 11.\n"
	       "The default value is 11.\n"
	       "Enter the value: ");

      num = ask_value ("%d", 11);
      if (num != 5 && num != 7 && num != 9 && num != 10 && num != 11)
	{
	  fprintf (stderr, "*** Illegal input! ***\n");
	  num = 11;
	}
      fprintf (stderr, " AudioTriX WSS IRQ set to %d\n", num);
      printf ("#define TRIX_IRQ %d\n", num);

      fprintf (stderr, "\nWSS DMA number for AudioTriX?\n"
	       "Valid values are 0, 1 and 3.\n"
	       "The default value is 3\n"
	       "Enter the value: ");

      num = ask_value ("%d", 3);
      if (num != 0 && num != 1 && num != 3)
	{
	  fprintf (stderr, "*** Illegal input! ***\n");
	  num = 3;
	}
      fprintf (stderr, "\nAudioTriX/WSS DMA set to %d\n", num);
      printf ("#define TRIX_DMA %d\n", num);

      fprintf (stderr, "\nSoundBlaster I/O address for the AudioTriX card?\n"
	       "The factory default is 220\n"
	  "Other possible values are 200, 210, 230, 240, 250, 260 and 270\n"
	       "Enter the MSS I/O base: ");

      num = ask_value ("%x", 0x220);
      fprintf (stderr, "AudioTriX SB I/O base set to %03x\n", num);
      printf ("#define TRIX_SB_BASE 0x%03x\n", num);

      fprintf (stderr, "\nIRQ number for the SB mode of AudioTriX ?\n"
	       "Valid numbers are: 3, 4, 5 and 7.\n"
	       "The default value is 7.\n"
	       "Enter the value: ");

      num = ask_value ("%d", 7);
      if (num != 3 && num != 4 && num != 5 && num != 7)
	{
	  fprintf (stderr, "*** Illegal input! ***\n");
	  num = 7;
	}
      fprintf (stderr, " AudioTriX SB IRQ set to %d\n", num);
      printf ("#define TRIX_SB_IRQ %d\n", num);

      fprintf (stderr, "\nSB DMA number for AudioTriX?\n"
	       "Valid values are 1 and 3.\n"
	       "The default value is 1\n"
	       "Enter the value: ");

      num = ask_value ("%d", 1);
      if (num != 1 && num != 3)
	{
	  fprintf (stderr, "*** Illegal input! ***\n");
	  num = 1;
	}
      fprintf (stderr, "\nAudioTriX/SB DMA set to %d\n", num);
      printf ("#define TRIX_SB_DMA %d\n", num);

      fprintf (stderr, "\nMIDI (MPU-401) I/O address for the AudioTriX card?\n"
	       "The factory default is 330\n"
	       "Other possible values are 330, 370, 3B0 and 3F0\n"
	       "Enter the MPU I/O base: ");

      num = ask_value ("%x", 0x330);
      fprintf (stderr, "AudioTriX MIDI I/O base set to %03x\n", num);
      printf ("#define TRIX_MPU_BASE 0x%03x\n", num);

      fprintf (stderr, "\nMIDI IRQ number for the AudioTriX ?\n"
	       "Valid numbers are: 3, 4, 5, 7 and 9(=2).\n"
	       "The default value is 5.\n"
	       "Enter the value: ");

      num = ask_value ("%d", 5);
      if (num != 3 && num != 4 && num != 5 && num != 7 && num != 9)
	{
	  fprintf (stderr, "*** Illegal input! ***\n");
	  num = 5;
	}
      fprintf (stderr, " AudioTriX MIDI IRQ set to %d\n", num);
      printf ("#define TRIX_MPU_IRQ %d\n", num);
    }

  if (selected_options & B (OPT_MAD16))
    {
      fprintf (stderr, "\n*** Options for the MAD16 and Mozart based cards ***\n\n");

      fprintf (stderr, "\nWindows Sound System I/O base for the MAD16/Mozart card?\n"
	       "The factory default is 530\n"
	       "Other possible values are  604, E80 or F40\n"
	       "(Check which ones are supported by your card!!!!!!)\n"
	       "Enter the MSS I/O base: ");

      num = ask_value ("%x", 0x530);
      fprintf (stderr, "MAD16 MSS I/O base set to %03x\n", num);
      printf ("#define MAD16_BASE 0x%03x\n", num);

      fprintf (stderr, "\nIRQ number for the WSS mode of MAD16/Mozart ?\n"
	       "Valid numbers are: 7, 9(=2), 10 and 11.\n"
	       "The default value is 11.\n"
	       "Enter the value: ");

      num = ask_value ("%d", 11);
      if (num != 7 && num != 9 && num != 10 && num != 11)
	{
	  fprintf (stderr, "*** Illegal input! ***\n");
	  num = 11;
	}
      fprintf (stderr, " MAD16 WSS IRQ set to %d\n", num);
      printf ("#define MAD16_IRQ %d\n", num);

      fprintf (stderr, "\nWSS DMA number for MAD16/Mozart?\n"
	       "Valid values are 0, 1 and 3.\n"
	       "The default value is 3\n"
	       "Enter the value: ");

      num = ask_value ("%d", 3);
      if (num != 0 && num != 1 && num != 3)
	{
	  fprintf (stderr, "*** Illegal input! ***\n");
	  num = 3;
	}
      fprintf (stderr, "\nMAD16/WSS DMA set to %d\n", num);
      printf ("#define MAD16_DMA %d\n", num);

      fprintf (stderr, "\nMIDI (MPU-401) I/O address for the MAD16 card?\n"
	       "(MPU401 is not supported by Mozart and 82C928)\n"
	       "(This is the second MIDI port in TB Tropez)\n"
	       "The factory default is 330 (use 0 to disable)\n"
	       "Other possible values are 330, 320, 310 and 300\n"
	       "Enter the MPU I/O base: ");

      num = ask_value ("%x", 0x330);
      if (num == 0)
	fprintf (stderr, "MAD16/Mozart MIDI port disabled\n");
      else
	{
	  fprintf (stderr, "MAD16 MIDI I/O base set to %03x\n", num);
	  printf ("#define MAD16_MPU_BASE 0x%03x\n", num);

	  fprintf (stderr, "\nMIDI IRQ number for the MAD16 ?\n"
		   "Valid numbers are: 5, 7, 9(=2) and 10.\n"
		   "The default value is 5.\n"
		   "Enter the value: ");

	  num = ask_value ("%d", 5);
	  if (num != 3 && num != 4 && num != 5 && num != 7 && num != 9)
	    {
	      fprintf (stderr, "*** Illegal input! ***\n");
	      num = 5;
	    }
	  fprintf (stderr, " MAD16 MIDI IRQ set to %d\n", num);
	  printf ("#define MAD16_MPU_IRQ %d\n", num);
	}
    }
#endif

  if (selected_options & B (OPT_AUDIO))
    {
      def_size = 65536;

      fprintf (stderr, "\nSelect the DMA buffer size (4096, 16384, 32768 or 65536 bytes)\n"
	       "%d is recommended value for this configuration.\n"
	       "Enter the value: ", def_size);

      num = ask_value ("%d", def_size);
      if (num != 4096 && num != 16384 && num != 32768 && num != 65536)
	{

	  fprintf (stderr, "*** Illegal input! ***\n");
	  num = def_size;
	}
      fprintf (stderr, "The DMA buffer size set to %d\n", num);
      printf ("#define DSP_BUFFSIZE %d\n", num);
    }

  printf ("#define SELECTED_SOUND_OPTIONS\t0x%08x\n", selected_options);
  fprintf (stderr, "The sound driver is now configured.\n");

#if defined(SCO) || defined(ISC) || defined(SYSV)
  fprintf (stderr, "Remember to update the System file\n");
#endif

  exit (0);
}

int
bin2hex (char *path, char *target, char *varname)
{
  int             fd;
  int             count;
  char            c;
  int             i = 0;

  if ((fd = open (path, 0)) > 0)
    {
      FILE           *sf = fopen (target, "w");

      fprintf (sf, "/* automaticaly generated by configure */\n");
      fprintf (sf, "static unsigned char %s[] = {\n", varname);
      while (1)
	{
	  count = read (fd, &c, 1);
	  if (count == 0)
	    break;
	  if (i != 0 && (i % 10) == 0)
	    fprintf (sf, "\n");
	  fprintf (sf, "0x%02x,", c & 0xFFL);
	  i++;
	}
      fprintf (sf, "};\n"
	       "#define %sLen %d\n", varname, i);
      fclose (sf);
      close (fd);
      return 1;
    }

  return 0;
}
