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

#define DEBUG 1

#define ARRAY_SIZE(a) (sizeof(a) / sizeof *(a))

static snd_seq_t *seq;
static volatile sig_atomic_t signal_received = 0;

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

static void list_ports(void)
{
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
	       "  -R, --realtime             use realtime scheduling (default: no)\n"
	       "  -P, --priority=int         scheduling priority, use with -R\n"
	       "                             (default: maximum)\n\n"
	       "  -S, --samples=# of samples to take for the measurement (default: 10000)\n"
	       "  -s, --skip=# of samples    to skip at the beginning (default: 0)\n\n"
	       "  -h, --help                 this help\n"
	       "  -V, --version              print current version\n"
	       "\n", argv0);
}

static void version(void)
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

static void sighandler(int sig)
{
	signal_received = 1;
}

int main(int argc, char *argv[])
{
	static char short_options[] = "hVlo:i:RP:s:S:";
	static struct option long_options[] = {
		{"help", 0, NULL, 'h'},
		{"version", 0, NULL, 'V'},
		{"list", 0, NULL, 'l'},
		{"output", 1, NULL, 'o'},
		{"input", 1, NULL, 'i'},
		{"realtime", 0, NULL, 'R'},
		{"priority", 1, NULL, 'P'},
		{"skip", 1, NULL, 's'},
		{"samples", 1, NULL, 'S'},
		{}
	};
	int do_list = 0;
	int do_realtime = 0;
	int rt_prio = sched_get_priority_max(SCHED_FIFO);
	int skip_samples = 0;
	int nr_samples = 10000;
	const char *output_name = NULL;
	const char *input_name = NULL;
	snd_seq_addr_t output_addr;
	snd_seq_addr_t input_addr;
	int c, err;

	while ((c = getopt_long(argc, argv, short_options,
				long_options, NULL)) != -1) {
		switch (c) {
		case 'h':
			usage(argv[0]);
			return EXIT_SUCCESS;
		case 'V':
			version();
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
		default:
			usage(argv[0]);
			return EXIT_FAILURE;
		}
	}
	if (argc == 1 || argv[optind]) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	err = snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0);
	check_snd("open sequencer", err);

	if (do_list) {
		list_ports();
		return 0;
	}

	if (!output_name)
		fatal("Please specify an output port with --output.  Use -l to get a list.");
	if (!input_name)
		fatal("Please specify an input port with --input.  Use -l to get a list.");
	err = snd_seq_parse_address(seq, &output_addr, output_name);
	check_snd("parse output port", err);
	err = snd_seq_parse_address(seq, &input_addr, input_name);
	check_snd("parse input port", err);

	err = snd_seq_set_client_name(seq, "alsa-midi-latency-test");
	check_snd("set client name", err);
	int client = snd_seq_client_id(seq);
	check_snd("get client id", client);
	int port = snd_seq_create_simple_port(seq, "alsa-midi-latency-test",
					      SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SYNC_WRITE,
					      SND_SEQ_PORT_TYPE_APPLICATION);
	check_snd("create port", port);
	err = snd_seq_connect_to(seq, port, output_addr.client, output_addr.port);
	check_snd("connect output port", err);
	err = snd_seq_connect_from(seq, port, input_addr.client, input_addr.port);
	check_snd("connect input port", err);

        version();

	if (do_realtime) {
		printf("> set_realtime_priority(SCHED_FIFO, %d).. ", rt_prio);
		set_realtime_priority(SCHED_FIFO, rt_prio);
		printf("done.\n");
	}

	struct timespec begin, end;
	if (clock_gettime(CLOCK_MONOTONIC, &begin) < 0)
		fatal("monotonic clock not supported");
	if (clock_getres(CLOCK_MONOTONIC, &begin) < 0)
		fatal("monotonic clock not supported");
	printf("> clock resolution: %d.%09ld s\n", (int)begin.tv_sec, begin.tv_nsec);
	if (begin.tv_sec || begin.tv_nsec > 1000000)
		puts("WARNING: You do not have a high-resolution clock!");

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

	if (DEBUG)
		printf("\nsample; latency_ms; latency_ms_worst\n");

	snd_seq_event_t ev;
	snd_seq_ev_clear(&ev);
	snd_seq_ev_set_dest(&ev, output_addr.client, output_addr.port);
	snd_seq_ev_set_source(&ev, port);
	snd_seq_ev_set_direct(&ev);
	snd_seq_ev_set_noteon(&ev, 0, 60, 127);

	int pollfds_count = snd_seq_poll_descriptors_count(seq, POLLIN);
	struct pollfd *pollfds = alloca(pollfds_count * sizeof *pollfds);
	err = snd_seq_poll_descriptors(seq, pollfds, pollfds_count, POLLIN);
	check_snd("get poll descriptors", err);
	pollfds_count = err;

	unsigned int sample_nr = 0;
	unsigned int min_delay = UINT_MAX, max_delay = 0;
	for (c = 0; c < nr_samples; ++c) {
		clock_gettime(CLOCK_MONOTONIC, &begin);

		err = snd_seq_event_output_direct(seq, &ev);
		check_snd("output MIDI event", err);

		snd_seq_event_t *rec_ev;
		for (;;) {
			rec_ev = NULL;
			err = poll(pollfds, pollfds_count, 1000);
			if (signal_received)
			       break;
			if (err == 0)
				fatal("timeout: there seems to be no connection between ports %s and %s", output_name, input_name);
			if (err < 0)
				fatal("poll error: %s", strerror(errno));
			unsigned short revents;
			err = snd_seq_poll_descriptors_revents(seq, pollfds, pollfds_count, &revents);
			check_snd("get poll events", err);
			if (revents & (POLLERR | POLLNVAL))
				break;
			if (!(revents & POLLIN))
				continue;
			err = snd_seq_event_input(seq, &rec_ev);
			check_snd("input MIDI event", err);
			if (rec_ev->type == SND_SEQ_EVENT_NOTEON)
				break;
		}
		if (!rec_ev)
			break;

		clock_gettime(CLOCK_MONOTONIC, &end);

		unsigned int delay_ns = timespec_sub(&end, &begin);
		if (sample_nr < skip_samples) {
			//if (DEBUG) printf("skipping sample %d\n", sample_nr);
		} else if (delay_ns > max_delay) {
			max_delay = delay_ns;
			if (DEBUG)
				printf("%6u; %10.2f; %10.2f     \n",
				       sample_nr, delay_ns / 1000000.0, max_delay / 1000000.0);
		} else {
			if (DEBUG)
				printf("%6u; %10.2f; %10.2f     \r",
				       sample_nr, delay_ns / 1000000.0, max_delay / 1000000.0);
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

	unsigned int delay_hist[1000];
	unsigned int i, j;
	for (i = 0; i < ARRAY_SIZE(delay_hist); ++i)
		delay_hist[i] = 0;
	for (i = skip_samples; i < sample_nr; ++i) {
		unsigned int index = (delays[i] + 50000) / 100000;
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
			printf("%5.1f -%5.1f ms: %8u ", i/10.0, i/10.0 + 0.09, delay_hist[i]);
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

		snd_seq_close(seq);
		return EXIT_FAILURE;

	} else {
		printf("\n> SUCCESS\n");
		printf("\n best latency was %.2f ms\n", min_delay / 1000000.0);
		printf(" worst latency was %.2f ms, which is great.\n\n", max_delay/1000000.0);

		snd_seq_close(seq);
		return EXIT_SUCCESS;
	}
}
