/*
 * sound/midi_synth.c
 *
 * High level midi sequencer manager for dumb MIDI interfaces.
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

#define USE_SEQ_MACROS
#define USE_SIMPLE_MACROS

#include "sound_config.h"

#if defined(CONFIGURE_SOUNDCARD) && !defined(EXCLUDE_MIDI)

#define _MIDI_SYNTH_C_

DEFINE_WAIT_QUEUE (sysex_sleeper, sysex_sleep_flag);

#include "midi_synth.h"

static int      midi2synth[MAX_MIDI_DEV];
static unsigned char prev_out_status[MAX_MIDI_DEV];

#define STORE(cmd) \
{ \
  int len; \
  unsigned char obuf[8]; \
  cmd; \
  seq_input_event(obuf, len); \
}
#define _seqbuf obuf
#define _seqbufptr 0
#define _SEQ_ADVBUF(x) len=x

void
do_midi_msg (int synthno, unsigned char *msg, int mlen)
{
  switch (msg[0] & 0xf0)
    {
    case 0x90:
      if (msg[2] != 0)
	{
	  STORE (SEQ_START_NOTE (synthno, msg[0] & 0x0f, msg[1], msg[2]));
	  break;
	}
      msg[2] = 64;

    case 0x80:
      STORE (SEQ_STOP_NOTE (synthno, msg[0] & 0x0f, msg[1], msg[2]));
      break;

    case 0xA0:
      STORE (SEQ_KEY_PRESSURE (synthno, msg[0] & 0x0f, msg[1], msg[2]));
      break;

    case 0xB0:
      STORE (SEQ_CONTROL (synthno, msg[0] & 0x0f,
			  msg[1], msg[2]));
      break;

    case 0xC0:
      STORE (SEQ_SET_PATCH (synthno, msg[0] & 0x0f, msg[1]));
      break;

    case 0xD0:
      STORE (SEQ_CHN_PRESSURE (synthno, msg[0] & 0x0f, msg[1]));
      break;

    case 0xE0:
      STORE (SEQ_BENDER (synthno, msg[0] & 0x0f,
			 (msg[1] % 0x7f) | ((msg[2] & 0x7f) << 7)));
      break;

    default:
      /* printk ("MPU: Unknown midi channel message %02x\n", msg[0]); */
    }
}

static void
midi_outc (int midi_dev, int data)
{
  int             timeout;

  for (timeout = 0; timeout < 32000; timeout++)
    if (midi_devs[midi_dev]->putc (midi_dev, (unsigned char) (data & 0xff)))
      {
	if (data & 0x80)	/*
				 * Status byte
				 */
	  prev_out_status[midi_dev] =
	    (unsigned char) (data & 0xff);	/*
						 * Store for running status
						 */
	return;			/*
				 * Mission complete
				 */
      }

  /*
   * Sorry! No space on buffers.
   */
  printk ("Midi send timed out\n");
}

static int
prefix_cmd (int midi_dev, unsigned char status)
{
  if ((char *) midi_devs[midi_dev]->prefix_cmd == NULL)
    return 1;

  return midi_devs[midi_dev]->prefix_cmd (midi_dev, status);
}

static void
midi_synth_input (int orig_dev, unsigned char data)
{
  int             dev;
  struct midi_input_info *inc;

  static unsigned char len_tab[] =	/* # of data bytes following a status
					 */
  {
    2,				/* 8x */
    2,				/* 9x */
    2,				/* Ax */
    2,				/* Bx */
    1,				/* Cx */
    1,				/* Dx */
    2,				/* Ex */
    0				/* Fx */
  };

  if (orig_dev < 0 || orig_dev > num_midis)
    return;

  if (data == 0xfe)		/* Ignore active sensing */
    return;

  dev = midi2synth[orig_dev];
  inc = &midi_devs[orig_dev]->in_info;

  switch (inc->m_state)
    {
    case MST_INIT:
      if (data & 0x80)		/* MIDI status byte */
	{
	  if ((data & 0xf0) == 0xf0)	/* Common message */
	    {
	      switch (data)
		{
		case 0xf0:	/* Sysex */
		  inc->m_state = MST_SYSEX;
		  break;	/* Sysex */

		case 0xf1:	/* MTC quarter frame */
		case 0xf3:	/* Song select */
		  inc->m_state = MST_DATA;
		  inc->m_ptr = 1;
		  inc->m_left = 1;
		  inc->m_buf[0] = data;
		  break;

		case 0xf2:	/* Song position pointer */
		  inc->m_state = MST_DATA;
		  inc->m_ptr = 1;
		  inc->m_left = 2;
		  inc->m_buf[0] = data;
		  break;

		default:
		  inc->m_buf[0] = data;
		  inc->m_ptr = 1;
		  do_midi_msg (dev, inc->m_buf, inc->m_ptr);
		  inc->m_ptr = 0;
		  inc->m_left = 0;
		}
	    }
	  else
	    {
	      inc->m_state = MST_DATA;
	      inc->m_ptr = 1;
	      inc->m_left = len_tab[(data >> 4) - 8];
	      inc->m_buf[0] = inc->m_prev_status = data;
	    }
	}
      else if (inc->m_prev_status & 0x80)	/* Ignore if no previous status (yet) */
	{			/* Data byte (use running status) */
	  inc->m_state = MST_DATA;
	  inc->m_ptr = 2;
	  inc->m_left = len_tab[(data >> 4) - 8] - 1;
	  inc->m_buf[0] = inc->m_prev_status;
	  inc->m_buf[1] = data;
	}
      break;			/* MST_INIT */

    case MST_DATA:
      inc->m_buf[inc->m_ptr++] = data;
      if (--inc->m_left <= 0)
	{
	  inc->m_state = MST_INIT;
	  do_midi_msg (dev, inc->m_buf, inc->m_ptr);
	  inc->m_ptr = 0;
	}
      break;			/* MST_DATA */

    case MST_SYSEX:
      if (data == 0xf7)		/* Sysex end */
	{
	  inc->m_state = MST_INIT;
	  inc->m_left = 0;
	  inc->m_ptr = 0;
	}
      break;			/* MST_SYSEX */

    default:
      printk ("MIDI%d: Unexpected state %d (%02x)\n", orig_dev, inc->m_state,
	      (int) data);
      inc->m_state = MST_INIT;
    }
}

static void
midi_synth_output (int dev)
{
  /*
   * Currently NOP
   */
}

int
midi_synth_ioctl (int dev,
		  unsigned int cmd, unsigned int arg)
{
  /*
   * int orig_dev = synth_devs[dev]->midi_dev;
   */

  switch (cmd)
    {

    case SNDCTL_SYNTH_INFO:
      IOCTL_TO_USER ((char *) arg, 0, synth_devs[dev]->info,
		     sizeof (struct synth_info));

      return 0;
      break;

    case SNDCTL_SYNTH_MEMAVL:
      return 0x7fffffff;
      break;

    default:
      return RET_ERROR (EINVAL);
    }
}

int
midi_synth_kill_note (int dev, int channel, int note, int velocity)
{
  int             orig_dev = synth_devs[dev]->midi_dev;
  int             msg, chn;

  if (note < 0 || note > 127)
    return 0;
  if (channel < 0 || channel > 15)
    return 0;
  if (velocity < 0)
    velocity = 0;
  if (velocity > 127)
    velocity = 127;

  msg = prev_out_status[orig_dev] & 0xf0;
  chn = prev_out_status[orig_dev] & 0x0f;

  if (chn == channel && ((msg == 0x90 && velocity == 64) || msg == 0x80))
    {				/*
				 * Use running status
				 */
      if (!prefix_cmd (orig_dev, note))
	return 0;

      midi_outc (orig_dev, note);

      if (msg == 0x90)		/*
				 * Running status = Note on
				 */
	midi_outc (orig_dev, 0);	/*
					   * Note on with velocity 0 == note
					   * off
					 */
      else
	midi_outc (orig_dev, velocity);
    }
  else
    {
      if (velocity == 64)
	{
	  if (!prefix_cmd (orig_dev, 0x90 | (channel & 0x0f)))
	    return 0;
	  midi_outc (orig_dev, 0x90 | (channel & 0x0f));	/*
								 * Note on
								 */
	  midi_outc (orig_dev, note);
	  midi_outc (orig_dev, 0);	/*
					 * Zero G
					 */
	}
      else
	{
	  if (!prefix_cmd (orig_dev, 0x80 | (channel & 0x0f)))
	    return 0;
	  midi_outc (orig_dev, 0x80 | (channel & 0x0f));	/*
								 * Note off
								 */
	  midi_outc (orig_dev, note);
	  midi_outc (orig_dev, velocity);
	}
    }

  return 0;
}

int
midi_synth_set_instr (int dev, int channel, int instr_no)
{
  int             orig_dev = synth_devs[dev]->midi_dev;

  if (instr_no < 0 || instr_no > 127)
    return 0;
  if (channel < 0 || channel > 15)
    return 0;

  if (!prefix_cmd (orig_dev, 0xc0 | (channel & 0x0f)))
    return 0;
  midi_outc (orig_dev, 0xc0 | (channel & 0x0f));	/*
							 * Program change
							 */
  midi_outc (orig_dev, instr_no);

  return 0;
}

int
midi_synth_start_note (int dev, int channel, int note, int velocity)
{
  int             orig_dev = synth_devs[dev]->midi_dev;
  int             msg, chn;

  if (note < 0 || note > 127)
    return 0;
  if (channel < 0 || channel > 15)
    return 0;
  if (velocity < 0)
    velocity = 0;
  if (velocity > 127)
    velocity = 127;

  msg = prev_out_status[orig_dev] & 0xf0;
  chn = prev_out_status[orig_dev] & 0x0f;

  if (chn == channel && msg == 0x90)
    {				/*
				 * Use running status
				 */
      if (!prefix_cmd (orig_dev, note))
	return 0;
      midi_outc (orig_dev, note);
      midi_outc (orig_dev, velocity);
    }
  else
    {
      if (!prefix_cmd (orig_dev, 0x90 | (channel & 0x0f)))
	return 0;
      midi_outc (orig_dev, 0x90 | (channel & 0x0f));	/*
							 * Note on
							 */
      midi_outc (orig_dev, note);
      midi_outc (orig_dev, velocity);
    }
  return 0;
}

void
midi_synth_reset (int dev)
{
}

int
midi_synth_open (int dev, int mode)
{
  int             orig_dev = synth_devs[dev]->midi_dev;
  int             err;
  unsigned long   flags;
  struct midi_input_info *inc;

  if (orig_dev < 0 || orig_dev > num_midis)
    return RET_ERROR (ENXIO);

  midi2synth[orig_dev] = dev;
  prev_out_status[orig_dev] = 0;

  if ((err = midi_devs[orig_dev]->open (orig_dev, mode,
				  midi_synth_input, midi_synth_output)) < 0)
    return err;

  inc = &midi_devs[orig_dev]->in_info;

  DISABLE_INTR (flags);
  inc->m_busy = 0;
  inc->m_state = MST_INIT;
  inc->m_ptr = 0;
  inc->m_left = 0;
  inc->m_prev_status = 0x00;
  RESTORE_INTR (flags);

  return 1;
}

void
midi_synth_close (int dev)
{
  int             orig_dev = synth_devs[dev]->midi_dev;

  /*
     * Shut up the synths by sending just single active sensing message.
   */
  midi_devs[orig_dev]->putc (orig_dev, 0xfe);

  midi_devs[orig_dev]->close (orig_dev);
}

void
midi_synth_hw_control (int dev, unsigned char *event)
{
}

int
midi_synth_load_patch (int dev, int format, snd_rw_buf * addr,
		       int offs, int count, int pmgr_flag)
{
  int             orig_dev = synth_devs[dev]->midi_dev;

  struct sysex_info sysex;
  int             i;
  unsigned long   left, src_offs, eox_seen = 0;
  int             first_byte = 1;
  int             hdr_size = (unsigned long) &sysex.data[0] - (unsigned long) &sysex;

  if (!prefix_cmd (orig_dev, 0xf0))
    return 0;

  if (format != SYSEX_PATCH)
    {
      printk ("MIDI Error: Invalid patch format (key) 0x%x\n", format);
      return RET_ERROR (EINVAL);
    }

  if (count < hdr_size)
    {
      printk ("MIDI Error: Patch header too short\n");
      return RET_ERROR (EINVAL);
    }

  count -= hdr_size;

  /*
   * Copy the header from user space but ignore the first bytes which have
   * been transferred already.
   */

  COPY_FROM_USER (&((char *) &sysex)[offs], addr, offs, hdr_size - offs);

  if (count < sysex.len)
    {
      printk ("MIDI Warning: Sysex record too short (%d<%d)\n",
	      count, (int) sysex.len);
      sysex.len = count;
    }

  left = sysex.len;
  src_offs = 0;

  RESET_WAIT_QUEUE (sysex_sleeper, sysex_sleep_flag);

  for (i = 0; i < left && !PROCESS_ABORTING (sysex_sleeper, sysex_sleep_flag); i++)
    {
      unsigned char   data;

      GET_BYTE_FROM_USER (data, addr, hdr_size + i);

      eox_seen = (i > 0 && data & 0x80);	/* End of sysex */

      if (eox_seen && data != 0xf7)
	data = 0xf7;

      if (i == 0)
	{
	  if (data != 0xf0)
	    {
	      printk ("Error: Sysex start missing\n");
	      return RET_ERROR (EINVAL);
	    }
	}

      while (!midi_devs[orig_dev]->putc (orig_dev, (unsigned char) (data & 0xff)) &&
	     !PROCESS_ABORTING (sysex_sleeper, sysex_sleep_flag))
	DO_SLEEP (sysex_sleeper, sysex_sleep_flag, 1);	/* Wait for timeout */

      if (!first_byte && data & 0x80)
	return 0;
      first_byte = 0;
    }

  if (!eox_seen)
    midi_outc (orig_dev, 0xf7);
  return 0;
}

void
midi_synth_panning (int dev, int channel, int pressure)
{
}

void
midi_synth_aftertouch (int dev, int channel, int pressure)
{
  int             orig_dev = synth_devs[dev]->midi_dev;
  int             msg, chn;

  if (pressure < 0 || pressure > 127)
    return;
  if (channel < 0 || channel > 15)
    return;

  msg = prev_out_status[orig_dev] & 0xf0;
  chn = prev_out_status[orig_dev] & 0x0f;

  if (msg != 0xd0 || chn != channel)	/*
					 * Test for running status
					 */
    {
      if (!prefix_cmd (orig_dev, 0xd0 | (channel & 0x0f)))
	return;
      midi_outc (orig_dev, 0xd0 | (channel & 0x0f));	/*
							 * Channel pressure
							 */
    }
  else if (!prefix_cmd (orig_dev, pressure))
    return;

  midi_outc (orig_dev, pressure);
}

void
midi_synth_controller (int dev, int channel, int ctrl_num, int value)
{
  int             orig_dev = synth_devs[dev]->midi_dev;
  int             chn, msg;

  if (ctrl_num < 1 || ctrl_num > 127)
    return;			/* NOTE! Controller # 0 ignored */
  if (channel < 0 || channel > 15)
    return;

  msg = prev_out_status[orig_dev] & 0xf0;
  chn = prev_out_status[orig_dev] & 0x0f;

  if (msg != 0xb0 || chn != channel)
    {
      if (!prefix_cmd (orig_dev, 0xb0 | (channel & 0x0f)))
	return;
      midi_outc (orig_dev, 0xb0 | (channel & 0x0f));
    }
  else if (!prefix_cmd (orig_dev, ctrl_num))
    return;

  midi_outc (orig_dev, ctrl_num);
  midi_outc (orig_dev, value & 0x7f);
}

int
midi_synth_patchmgr (int dev, struct patmgr_info *rec)
{
  return RET_ERROR (EINVAL);
}

void
midi_synth_bender (int dev, int channel, int value)
{
  int             orig_dev = synth_devs[dev]->midi_dev;
  int             msg, prev_chn;

  if (channel < 0 || channel > 15)
    return;

  if (value < 0 || value > 16383)
    return;

  msg = prev_out_status[orig_dev] & 0xf0;
  prev_chn = prev_out_status[orig_dev] & 0x0f;

  if (msg != 0xd0 || prev_chn != channel)	/*
						 * Test for running status
						 */
    {
      if (!prefix_cmd (orig_dev, 0xe0 | (channel & 0x0f)))
	return;
      midi_outc (orig_dev, 0xe0 | (channel & 0x0f));
    }
  else if (!prefix_cmd (orig_dev, value & 0x7f))
    return;

  midi_outc (orig_dev, value & 0x7f);
  midi_outc (orig_dev, (value >> 7) & 0x7f);
}

void
midi_synth_setup_voice (int dev, int voice, int channel)
{
}

#endif
