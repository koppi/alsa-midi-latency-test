/*
 * alsa-midi-latency-test.c - measure the roundtrip time of MIDI messages
 *
 * Copyright (C) 2009 - 2021 Jakob Flierl <jakob.flierl@gmail.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */
#include "aconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <signal.h>
#include <sched.h>
#include <poll.h>
#include <getopt.h>
#include <alsa/asoundlib.h>

#include <sys/utsname.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof *(a))
#define ENABLE_RAW

static snd_seq_t *seq;
#ifdef ENABLE_RAW
static snd_rawmidi_t *raw_in;
static snd_rawmidi_t *raw_out;
#endif // ENABLE_RAW
static volatile sig_atomic_t signal_received = 0;

void print_uname()
{
  struct utsname u;
  uname (&u);
  printf ("> running on %s release %s (version %s) on %s\n",
	  u.sysname, u.release, u.version, u.machine);
}

/* prints an error message to stderr, and dies */
static void fatal(const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	fputc('\n', stderr);
	exit(EXIT_FAILURE);
}

/* memory allocation error handling */
static void check_mem(void *p)
{
	if (!p)
		fatal("out of memory");
}

/* error handling for ALSA functions */
static void check_snd(const char *operation, int err)
{
	if (err < 0)
		fatal("cannot %s - %s", operation, snd_strerror(err));
}

/* sets the process to "policy" policy at given priority */
static int set_realtime_priority(int policy, int prio)
{
	struct sched_param schp;
	memset(&schp, 0, sizeof(schp));

	schp.sched_priority = prio;
	if (sched_setscheduler(0, policy, &schp) != 0) {
		perror("sched_setscheduler");
		return -1;
	}
	return 0;
}

#ifdef ENABLE_RAW
static void quiet_error_handler(const char *file, int line, const char *function, int err, const char *fmt, ...)
{
	return;
}
// code below borrowed rom:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// https://ccrma.stanford.edu/~craig/articles/linuxmidi/alsa-1.0/alsarawportlist.c
//
//////////////////////////////
//
// print_midi_ports -- go through the list of available "soundcards",
//   checking them to see if there are devices/subdevices on them which
//   can read/write MIDI data.
//
static void list_midi_devices_on_card(int card);
static void list_subdevice_info(snd_ctl_t *ctl, int card, int device);

//////////////////////////////
//
// is_input -- returns true if specified card/device/sub can output MIDI data.
//

static int is_input(snd_ctl_t *ctl, int card, int device, int sub) {
   snd_rawmidi_info_t *info;
   int status;

   snd_rawmidi_info_alloca(&info);
   snd_rawmidi_info_set_device(info, device);
   snd_rawmidi_info_set_subdevice(info, sub);
   snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);

   if ((status = snd_ctl_rawmidi_info(ctl, info)) < 0 && status != -ENXIO) {
      return status;
   } else if (status == 0) {
      return 1;
   }

   return 0;
}

//////////////////////////////
//
// is_output -- returns true if specified card/device/sub can output MIDI data.
//

static int is_output(snd_ctl_t *ctl, int card, int device, int sub) {
   snd_rawmidi_info_t *info;
   int status;

   snd_rawmidi_info_alloca(&info);
   snd_rawmidi_info_set_device(info, device);
   snd_rawmidi_info_set_subdevice(info, sub);
   snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);

   if ((status = snd_ctl_rawmidi_info(ctl, info)) < 0 && status != -ENXIO) {
      return status;
   } else if (status == 0) {
      return 1;
   }

   return 0;
}

//////////////////////////////
//
// list_midi_devices_on_card -- For a particular "card" look at all
//   of the "devices/subdevices" on it and print information about it
//   if it can handle MIDI input and/or output.
//

static void list_midi_devices_on_card(int card) {
   snd_ctl_t *ctl;
   char name[32];
   int device = -1;
   int status;
   sprintf(name, "hw:%d", card);
   if ((status = snd_ctl_open(&ctl, name, 0)) < 0) {
      fatal("cannot open control for card %d: %s", card, snd_strerror(status));
   }
   do {
      status = snd_ctl_rawmidi_next_device(ctl, &device);
      if (status < 0) {
         fatal("cannot determine device number: %s", snd_strerror(status));
      }
      if (device >= 0) {
         list_subdevice_info(ctl, card, device);
      }
   } while (device >= 0);
   snd_ctl_close(ctl);
}

//////////////////////////////
//
// list_subdevice_info -- Print information about a subdevice
//   of a device of a card if it can handle MIDI input and/or output.
//

static void list_subdevice_info(snd_ctl_t *ctl, int card, int device) {
   snd_rawmidi_info_t *info;
   const char *name;
   const char *sub_name;
   int subs, subs_in, subs_out;
   int sub, in, out;
   int status;

   snd_rawmidi_info_alloca(&info);
   snd_rawmidi_info_set_device(info, device);

   snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);
   snd_ctl_rawmidi_info(ctl, info);
   subs_in = snd_rawmidi_info_get_subdevices_count(info);
   snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);
   snd_ctl_rawmidi_info(ctl, info);
   subs_out = snd_rawmidi_info_get_subdevices_count(info);
   subs = subs_in > subs_out ? subs_in : subs_out;

   sub = 0;
   in = out = 0;
   if ((status = is_output(ctl, card, device, sub)) < 0) {
      fatal("cannot get rawmidi information %d:%d: %s",
            card, device, snd_strerror(status));
   } else if (status)
      out = 1;

   if (status == 0) {
      if ((status = is_input(ctl, card, device, sub)) < 0) {
         fatal("cannot get rawmidi information %d:%d: %s",
               card, device, snd_strerror(status));
      }
   } else if (status)
      in = 1;

   if (status == 0)
      return;

   name = snd_rawmidi_info_get_name(info);
   sub_name = snd_rawmidi_info_get_subdevice_name(info);
   if (sub_name[0] == '\0') {
      if (subs == 1) {
         printf("%c%c  hw:%d,%d    %s\n",
                in  ? 'I' : ' ',
                out ? 'O' : ' ',
                card, device, name);
      } else
         printf("%c%c  hw:%d,%d    %s (%d subdevices)\n",
                in  ? 'I' : ' ',
                out ? 'O' : ' ',
                card, device, name, subs);
   } else {
      sub = 0;
      for (;;) {
         printf("%c%c  hw:%d,%d,%d  %s\n",
                in ? 'I' : ' ', out ? 'O' : ' ',
                card, device, sub, sub_name);
         if (++sub >= subs)
            break;

         in = is_input(ctl, card, device, sub);
         out = is_output(ctl, card, device, sub);
         snd_rawmidi_info_set_subdevice(info, sub);
         if (out) {
            snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);
            if ((status = snd_ctl_rawmidi_info(ctl, info)) < 0) {
               fatal("cannot get rawmidi information %d:%d:%d: %s",
                     card, device, sub, snd_strerror(status));
            }
         } else {
            snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);
            if ((status = snd_ctl_rawmidi_info(ctl, info)) < 0) {
               fatal("cannot get rawmidi information %d:%d:%d: %s",
                     card, device, sub, snd_strerror(status));
            }
         }
         sub_name = snd_rawmidi_info_get_subdevice_name(info);
      }
   }
}

static void list_ports_raw(void)
{
	int status;
	int card = -1;  // use -1 to prime the pump of iterating through card list
	char* longname  = NULL;
	char* shortname = NULL;
	puts("Rawmidi ports:");

	int found = 0;
	while (1) {
		if ((status = snd_card_next(&card)) < 0) {
			fatal("cannot determine card number: %s", snd_strerror(status));
		}
		if (card < 0)
			break;
		list_midi_devices_on_card(card);
	}
}
#endif // ENABLE_RAW

static void list_ports(void)
{
#ifdef ENABLE_RAW
	list_ports_raw();
	if (seq)
		puts("Sequencer ports:");
	else {
		fprintf(stderr, "ALSA sequencer disabled (load module and/or rebuild kernel to enable)\n");
		return;
	}
#endif // ENABLE_RAW
	snd_seq_client_info_t *cinfo;
	snd_seq_port_info_t *pinfo;

	snd_seq_client_info_alloca(&cinfo);
	snd_seq_port_info_alloca(&pinfo);

	puts(" Port    Client name                      Port name");

	snd_seq_client_info_set_client(cinfo, -1);
	while (snd_seq_query_next_client(seq, cinfo) >= 0) {
		unsigned int caps;
		int client = snd_seq_client_info_get_client(cinfo);

		snd_seq_port_info_set_client(pinfo, client);
		snd_seq_port_info_set_port(pinfo, -1);
		while (snd_seq_query_next_port(seq, pinfo) >= 0) {
			/* port must understand MIDI messages */
			if (!(snd_seq_port_info_get_type(pinfo)
			      & SND_SEQ_PORT_TYPE_MIDI_GENERIC))
				continue;
			/* we need both READ/WRITE and SUBS_READ/WRITE */
			caps = snd_seq_port_info_get_capability(pinfo);
			if ((caps & (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE))
			    != (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE) &&
			    (caps & (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ))
			    != (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ))
				continue;
			printf("%3d:%-3d  %-32.32s %s\n",
			       snd_seq_port_info_get_client(pinfo),
			       snd_seq_port_info_get_port(pinfo),
			       snd_seq_client_info_get_name(cinfo),
			       snd_seq_port_info_get_name(pinfo));
		}
	}
}

static void usage(const char *argv0)
{
	printf("Usage: %s -o client:port -i client:port ...\n\n"
	       "  -o, --output=client:port   port to send events to\n"
	       "  -i, --input=client:port    port to receive events from\n"
	       "  -l, --list                 list available midi input/output ports\n\n"
#ifdef ENABLE_RAW
	       "  -a, --raw                  interpret ports as snd_rawmidi names\n\n"
#endif // ENABLE_RAW
	       "  -R, --realtime             use realtime scheduling (default: no)\n"
	       "  -P, --priority=int         scheduling priority, use with -R\n"
	       "                             (default: maximum)\n\n"
	       "  -S, --samples=# of samples to take for the measurement (default: 10000)\n"
	       "  -s, --skip=# of samples    to skip at the beginning (default: 0)\n"
	       "  -w, --wait=ms              time interval between measurements\n"
	       "  -r, --random-wait          use random interval between wait and 2*wait\n"
           "  -x                         disable debug output of measurements,\n"
           "                             this improves timing accuracy with very low latencies\n"
           "                             use this with -w to avoid CPU saturation.\n"
           " group bins in histogram:\n"
           "  -1 -2 -3 -4 -5 -6          0.1ms, 0.01ms, 0.001ms.. 0.000001ms (default: 0.1ms)\n\n"
	       "  -h, --help                 this help\n"
	       "  -V, --version              print current version\n"
	       "\n", argv0);
}

static void print_version(void)
{
	printf("> %s %s\n", PACKAGE, VERSION);
}

static unsigned int timespec_sub(const struct timespec *a,
				 const struct timespec *b)
{
	unsigned int diff;
       
	if (a->tv_sec - b->tv_sec > 2)
		return UINT_MAX;
	diff = (a->tv_sec - b->tv_sec) * 1000000000;
	diff += a->tv_nsec - b->tv_nsec;
	return diff;
}

static void wait_ms(double t)
{
	struct timespec ts;

	ts.tv_sec = t / 1000;
	ts.tv_nsec = (t - ts.tv_sec * 1000) * 1000000;
	nanosleep(&ts, NULL);
}

/* many thanks to Randall Munroe (http://xkcd.com/221/) */
static int getRandomNumber(void)
{
	return 4; // chosen by fair dice roll.
	          // guaranteed to be random.
}

static void sighandler(int sig)
{
	signal_received = 1;
}

int main(int argc, char *argv[])
{
	static char short_options[] = "hVlao:i:RP:s:S:w:r123456x";
	static struct option long_options[] = {
		{"help", 0, NULL, 'h'},
		{"version", 0, NULL, 'V'},
		{"list", 0, NULL, 'l'},
		{"raw", 0, NULL, 'a'},
		{"output", 1, NULL, 'o'},
		{"input", 1, NULL, 'i'},
		{"realtime", 0, NULL, 'R'},
		{"priority", 1, NULL, 'P'},
		{"skip", 1, NULL, 's'},
		{"samples", 1, NULL, 'S'},
		{"wait", 1, NULL, 'w'},
		{"random-wait", 0, NULL, 'r'},
		{}
	};
	int do_list = 0;
	int do_realtime = 0;
	int rt_prio = sched_get_priority_max(SCHED_FIFO);
	int skip_samples = 0;
	int nr_samples = 10000;
	int random_wait = 0;
    int precision = 1;
    int high_precision_display = 1;
    int debug = 1;
	double wait = 0.0;
	const char *output_name = NULL;
	const char *input_name = NULL;
	snd_seq_addr_t output_addr;
	snd_seq_addr_t input_addr;
	int c, err;
#ifdef ENABLE_RAW
	int use_rawmidi = 0;
#endif // ENABLE_RAW

	while ((c = getopt_long(argc, argv, short_options,
				long_options, NULL)) != -1) {
		switch (c) {
#ifdef ENABLE_RAW
		case 'a':
			use_rawmidi = 1;
			break;
#endif // ENABLE_RAW
		case 'h':
			usage(argv[0]);
			return EXIT_SUCCESS;
		case 'V':
			print_version();
			return EXIT_SUCCESS;
		case 'l':
			do_list = 1;
			break;
		case 'o':
			output_name = optarg;
			break;
		case 'i':
			input_name = optarg;
			break;
		case 'R':
			do_realtime = 1;
			break;
		case 'P':
			rt_prio = atoi(optarg);
			if (rt_prio > sched_get_priority_max(SCHED_FIFO)) {
				printf("> Warning: Given priority:   %d > sched_get_priority_max(SCHED_FIFO)! ", rt_prio);
				printf("Setting priority to %d.\n", sched_get_priority_max(SCHED_FIFO));
				rt_prio = sched_get_priority_max(SCHED_FIFO);
			} else if (rt_prio < sched_get_priority_min(SCHED_FIFO)) {
				printf("> Warning: Given priority:   %d < sched_get_priority_min(SCHED_FIFO)! ", rt_prio);
				printf("Setting priority to %d.\n", sched_get_priority_min(SCHED_FIFO));
				rt_prio = sched_get_priority_min(SCHED_FIFO);
			}
			break;
		case 's':
			skip_samples = atoi(optarg);
			if (skip_samples < 0) {
				printf("> Warning: Given number of events to skip cannot be smaller than zero! ");
				printf("Setting nr of skip events to zero.\n");
				skip_samples = 0;
			}
			break;
		case 'S':
			nr_samples = atoi(optarg);
			if (nr_samples <= 0) {
				printf("> Warning: Given number of samples to take is less or equal zero! ");
				printf("Setting nr of samples to take to 1.\n");
				nr_samples = 1;
			}
			break;
		case 'w':
			wait = atof(optarg);
			if (wait < 0) {
				printf("> Warning: Wait time is negative; using zero.\n");
				wait = 0;
			}
			break;
		case 'r':
			random_wait = 1;
			break;
        case '1':
            precision = 1;
            high_precision_display = 1;
            break;
        case '2':
            precision = 2;
            high_precision_display = 10;
            break;
        case '3':
            precision = 3;
            high_precision_display = 100;
            break;
        case '4':
            precision = 4;
            high_precision_display = 1000;
            break;
        case '5':
            precision = 5;
            high_precision_display = 10000;
            break;
        case '6':
            precision = 6;
            high_precision_display = 100000;
            break;
        case 'x':
            debug = 0;
            break;
		default:
			usage(argv[0]);
			return EXIT_FAILURE;
		}
	}
	if (argc == 1 || argv[optind]) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	int use_seq = 1;
#ifdef ENABLE_RAW
	// temporarily change the ALSA error handler to silence warning in case
	// /dev/snd/seq doesn't exist (e.g.: lacks kernel support or module not
	// loaded)
	snd_lib_error_set_handler(quiet_error_handler);
#endif // ENABLE_RAW
	err = snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0);
#ifdef ENABLE_RAW
	snd_lib_error_set_handler(NULL); // restore default handler
	if (err)
		use_seq = 0;
#else // ENABLE_RAW
	check_snd("open sequencer", err);
#endif // ENABLE_RAW

	if (do_list) {
		list_ports();
		return 0;
	}

	if (!output_name)
		fatal("Please specify an output port with --output.  Use -l to get a list.");
	if (!input_name)
		fatal("Please specify an input port with --input.  Use -l to get a list.");
#ifdef ENABLE_RAW
	// ensure that exactly one of rawmidi or seq is enabled
	if (use_rawmidi)
		use_seq = 0;
	else
		use_rawmidi = !use_seq;
	if (use_rawmidi) {
		err = snd_rawmidi_open(&raw_in, NULL, input_name, SND_RAWMIDI_NONBLOCK);
		check_snd("open input", err);
		err = snd_rawmidi_open(NULL, &raw_out, output_name, SND_RAWMIDI_SYNC);
		check_snd("open output", err);
	}
#endif // ENABLE_RAW
	int port;
	int client;
	if (use_seq) {
		err = snd_seq_parse_address(seq, &output_addr, output_name);
		check_snd("parse output port", err);
		err = snd_seq_parse_address(seq, &input_addr, input_name);
		check_snd("parse input port", err);

		err = snd_seq_set_client_name(seq, "alsa-midi-latency-test");
		check_snd("set client name", err);
		client = snd_seq_client_id(seq);
		check_snd("get client id", client);
		port = snd_seq_create_simple_port(seq, "alsa-midi-latency-test",
						      SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SYNC_WRITE,
						      SND_SEQ_PORT_TYPE_APPLICATION);
		check_snd("create port", port);
		err = snd_seq_connect_to(seq, port, output_addr.client, output_addr.port);
		check_snd("connect output port", err);
		err = snd_seq_connect_from(seq, port, input_addr.client, input_addr.port);
		check_snd("connect input port", err);
	}

        print_version();
	print_uname();

	if (random_wait)
		srand(getRandomNumber());

	if (do_realtime) {
		printf("> set_realtime_priority(SCHED_FIFO, %d).. ", rt_prio);
		set_realtime_priority(SCHED_FIFO, rt_prio);
		printf("done.\n");
	}

#if defined(CLOCK_MONOTONIC_RAW)
#define HR_CLOCK CLOCK_MONOTONIC_RAW
#else
#define HR_CLOCK CLOCK_MONOTONIC
#endif

	struct timespec begin, end;
	if (clock_gettime(HR_CLOCK, &begin) < 0)
		fatal("monotonic raw clock not supported");
	if (clock_getres(HR_CLOCK, &begin) < 0)
		fatal("monotonic raw clock not supported");
	printf("> clock resolution: %d.%09ld s\n", (int)begin.tv_sec, begin.tv_nsec);
	if (begin.tv_sec || begin.tv_nsec > 1000000)
		puts("WARNING: You do not have a high-resolution clock!");
	if (wait) {
		if (random_wait)
			printf("> interval between measurements: %.3f .. %.3f ms\n", wait, wait * 2);
		else
			printf("> interval between measurements: %.3f ms\n", wait);
	}

	printf("\n> sampling %d midi latency values - please wait ...\n", nr_samples);
	printf("> press Ctrl+C to abort test\n");

	signal(SIGINT,  sighandler);
	signal(SIGTERM, sighandler);

	unsigned int *delays = calloc(nr_samples, sizeof *delays);
	check_mem(delays);

	if (skip_samples) {
		if (skip_samples == 1)
			printf("> skipping first latency sample\n");
		else
			printf("> skipping first %d latency samples\n", skip_samples);
	}

	if (debug == 1)
		printf("\nsample; latency_ms; latency_ms_worst\n");

	snd_seq_event_t ev;
	int pollfds_count;
	struct pollfd *pollfds;
	if (use_seq)
	{
		snd_seq_ev_clear(&ev);
		snd_seq_ev_set_dest(&ev, output_addr.client, output_addr.port);
		snd_seq_ev_set_source(&ev, port);
		snd_seq_ev_set_direct(&ev);
		snd_seq_ev_set_noteon(&ev, 0, 60, 127);

		pollfds_count = snd_seq_poll_descriptors_count(seq, POLLIN);
		pollfds = alloca(pollfds_count * sizeof *pollfds);
		err = snd_seq_poll_descriptors(seq, pollfds, pollfds_count, POLLIN);
	}
#ifdef ENABLE_RAW
	const char test_status_byte = 0x90;
	char msg[3] = { test_status_byte, 60, 127 };
	if (use_rawmidi)
	{
		pollfds_count = snd_rawmidi_poll_descriptors_count(raw_in);
		pollfds = alloca(pollfds_count * sizeof *pollfds);
		err = snd_rawmidi_poll_descriptors(raw_in, pollfds, pollfds_count);
		snd_rawmidi_drain(raw_in);
		snd_rawmidi_drain(raw_out);
		// not sure if this is documented anwhere, but in practical
		// applications we find that one needs to poll() at least once
		// before incoming messages start being queued.
		// skipping this dummy poll() here would result in the first
		// response message not being received if the roundtrip is so
		// fast that the first call to poll() happens after the
		// device has sent back its response
		poll(pollfds, pollfds_count, 0);
	}
#endif // ENABLE_RAW
	check_snd("get poll descriptors", err);
	pollfds_count = err;

	unsigned int sample_nr = 0;
	unsigned int min_delay = UINT_MAX, max_delay = 0;
	for (c = 0; c < nr_samples; ++c) {
		if (wait) {
			if (random_wait)
				wait_ms(wait + rand() * wait / RAND_MAX);
			else
				wait_ms(wait);
			if (signal_received)
				break;
		}

		clock_gettime(HR_CLOCK, &begin);

		if (use_seq)
			err = snd_seq_event_output_direct(seq, &ev);
#ifdef ENABLE_RAW
		char rec_msg[3];
		if (use_rawmidi)
			err = snd_rawmidi_write(raw_out, msg, sizeof(msg));
#endif // ENABLE_RAW
		check_snd("output MIDI event", err);

		snd_seq_event_t *rec_ev;
		int received_something = 0;
		for (;;) {
			if (use_seq)
				rec_ev = NULL;
#ifdef ENABLE_RAW
			if (use_rawmidi)
				memset(rec_msg, 0, sizeof(rec_msg));
#endif // ENABLE_RAW
			err = poll(pollfds, pollfds_count, 1000);
			if (signal_received)
			       break;
			if (err == 0)
				fatal("timeout: there seems to be no connection between ports %s and %s", output_name, input_name);
			if (err < 0)
				fatal("poll error: %s", strerror(errno));
			unsigned short revents;
			if (use_seq)
				err = snd_seq_poll_descriptors_revents(seq, pollfds, pollfds_count, &revents);
#ifdef ENABLE_RAW
			if (use_rawmidi)
				err = snd_rawmidi_poll_descriptors_revents(raw_in, pollfds, pollfds_count, &revents);
#endif // ENABLE_RAW
			check_snd("get poll events", err);
			if (revents & (POLLERR | POLLNVAL))
				break;
			if (!(revents & POLLIN))
				continue;
			if (use_seq) {
				err = snd_seq_event_input(seq, &rec_ev);
				check_snd("input MIDI event", err);
				received_something = 1;
				if (rec_ev->type == SND_SEQ_EVENT_NOTEON)
					break;
			}
#ifdef ENABLE_RAW
			if (use_rawmidi) {
				err = snd_rawmidi_read(raw_in, rec_msg, sizeof(rec_msg));
				check_snd("input MIDI event", err);
				received_something = 1;
				if (test_status_byte == rec_msg[0])
					break;
			}
#endif // ENABLE_RAW
		}
		if (!received_something)
			break;

		clock_gettime(HR_CLOCK, &end);

		unsigned int delay_ns = timespec_sub(&end, &begin);
		if (sample_nr < skip_samples) {
			//if (debug == 1) printf("skipping sample %d\n", sample_nr);
		} else if (delay_ns > max_delay) {
			max_delay = delay_ns;
			if (debug == 1)
				printf("%6u; %10.*f; %10.*f     \n",
				       sample_nr, 2 + precision, delay_ns / 1000000.0, 2 + precision, max_delay / 1000000.0);
		} else {
			if (debug == 1)
				printf("%6u; %10.*f; %10.*f     \r",
				       sample_nr, 2 + precision, delay_ns / 1000000.0, 2 + precision, max_delay / 1000000.0);
		}
		if (delay_ns < min_delay)
			min_delay = delay_ns;
		delays[sample_nr++] = delay_ns;

		ev.data.note.channel ^= 1; // prevent running status
	}

	printf("\n> done.\n\n> latency distribution:\n");

	if (!max_delay) {
		puts("no delay was measured; clock has too low resolution");
		return EXIT_FAILURE;
	}

	unsigned int delay_hist[1000000];
	unsigned int i, j;
	for (i = 0; i < ARRAY_SIZE(delay_hist); ++i)
		delay_hist[i] = 0;
	for (i = skip_samples; i < sample_nr; ++i) {
		unsigned int index = (delays[i] + 50000/high_precision_display) / (100000/high_precision_display);
		if (index >= ARRAY_SIZE(delay_hist))
			index = ARRAY_SIZE(delay_hist) - 1;
		delay_hist[index]++;
	}

	unsigned int max_samples = 0;
	for (i = 0; i < ARRAY_SIZE(delay_hist); ++i) {
		if (delay_hist[i] > max_samples)
			max_samples = delay_hist[i];
	}
	if (!max_samples) {
		puts("(no measurements)");
		return EXIT_FAILURE;
	}

	// plot ascii bars
	int skipped = 0;
	for (i = 0; i < ARRAY_SIZE(delay_hist); ++i) {
		if (delay_hist[i] > 0) {
			if (skipped) {
				puts("...");
				skipped = 0;
			}
			printf("%*.*f -%*.*f ms: %8u ", 4 + precision, precision, i/(10.0*high_precision_display), 4 + precision, precision, i/(10.0*high_precision_display) + (0.09999999/high_precision_display), delay_hist[i]);
			unsigned int bar_width = (delay_hist[i] * 50 + max_samples / 2) / max_samples;
			if (!bar_width && delay_hist[i])
				bar_width = 1;
			for (j = 0; j < bar_width; ++j)
				printf("#");
			puts("");
		} else {
			skipped = 1;
		}
	}

	if (use_seq)
		snd_seq_close(seq);
#ifdef ENABLE_RAW
	if (use_rawmidi) {
		snd_rawmidi_close(raw_in);
		snd_rawmidi_close(raw_out);
	}
#endif // ENABLE_RAW

	if (max_delay / 1000000.0 > 6.0) { // latencies <= 6ms are o.k. imho
		printf("\n> FAIL\n");
		printf("\n best latency was %.2f ms\n", min_delay / 1000000.0);
		printf(" worst latency was %.2f ms, which is too much. Please check:\n\n", max_delay/1000000.0);
		printf("  - if your hardware uses shared IRQs - `watch -n 1 cat /proc/interrupts`\n");
		printf("    while running this test to see, which IRQs the OS is using for your midi hardware,\n\n");
		printf("  - if you're running this test on a realtime OS - `uname -a` should contain '-rt',\n\n");
		printf("  - your OS' scheduling priorities - `chrt -p [pidof process name|IRQ-?]`.\n\n");
		printf(" Have a look at\n");
		printf("  http://www.linuxaudio.org/mailarchive/lat/\n");
		printf(" to find out, howto fix issues with high midi latencies.\n\n");

		return EXIT_FAILURE;

	} else {
		printf("\n> SUCCESS\n");
		printf("\n best latency was %.*f ms\n", precision, min_delay / 1000000.0);
		printf(" worst latency was %.*f ms, which is great.\n\n", precision, max_delay/1000000.0);

		return EXIT_SUCCESS;
	}
}
