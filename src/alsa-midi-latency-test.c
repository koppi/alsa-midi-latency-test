#include "aconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <getopt.h>
#include <alsa/asoundlib.h>

// begin: common stuff
#define DEBUG 1

double cpu_hz;
int opt_smp = 0;
double cpu_load = 0.0;

#define mygettime() (rdtsc() / cpu_hz)

unsigned long long int rdtsc() {
  unsigned long long int x;
  __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
  return x;
}

/* simple busywaiting loop used to waste CPU time, useful to simulate
   heavy CPU bound computations, since some classes of realtime processes
   need a huge amount of CPU, like software synthesizers for example */
static void mydelay(int loops) {
  int k = 0;
  int u;
  for(u = 0;u < loops; u++) k += 1;
}

/* sets the process to "policy" policy at given priority */
static int set_realtime_priority(int policy, int prio) {
  struct sched_param schp;
  memset(&schp, 0, sizeof(schp));
  if (prio == -1) {
    schp.sched_priority = sched_get_priority_max(policy);
  } else {
    schp.sched_priority = prio;
  }
  
  if (sched_setscheduler(0, policy, &schp) != 0) {
    perror("sched_setscheduler");
    return -1;
  }
  
  return 0;
}

/* determines first the CPU frequency which is then stored in a global
   variable (cpu_hz) which is needed to get accurate timing information
   through the Pentium RDTSC instruction determines how many loops per
   second the CPU is able to execute returns the number of loops per
   sec, this is used for busywaiting through the mydelay() function */
static int calibrate_loop(void) {
  FILE *f;
  char *res;
  char s1[100];
  double tmp_loops_per_sec;
  double mytime1,mytime2;
  int num_cpus;
  
  /* read the CPU frequency from /proc/cpuinfo  */
  f = fopen("/proc/cpuinfo","r");

  if (f==NULL) {
    perror("can't open /proc/cpuinfo, exiting. open");
    exit(1);
  }

  for (;;) {
    res = fgets(s1, 100, f);

    if (res == NULL) {
      break;
    }

    if (!memcmp(s1, "cpu MHz", 7)) {
      cpu_hz = atof(&s1[10])*1000000.0;
    }

    if (!memcmp(s1, "processor", 9)) {
      num_cpus=atoi(&s1[11]);
    }
  }

  fclose(f);
  if (cpu_hz < 1.0) {
    fprintf(stderr, "can't determine CPU clock frequency, exiting.\n");
    exit(EXIT_FAILURE);
  }

  if(DEBUG)
    printf("> calibrating high precision timer loop.. ");
  
#define CALIB_LOOPS 200000000
  
  /* run mydelay for CALIB_LOOPS times, and measure how long this takes */
  mytime1 = mygettime();
  mydelay(CALIB_LOOPS);
  mytime2 = mygettime();
  
  if (DEBUG) {
    printf("done.\n");
    printf("time diff = %30.2f\n", mytime2 - mytime1);
    printf("loops/sec = %30.2f\n", CALIB_LOOPS / (mytime2 - mytime1));
  }
  
  /* calculate the "loops per second" value  */
  tmp_loops_per_sec = CALIB_LOOPS / (mytime2 - mytime1);
  
  return tmp_loops_per_sec;
}
// end: common stuff

// begin: alsa foo
int port_in_count;
int port_out_count;

snd_seq_addr_t *port_in;
snd_seq_addr_t *port_out;

/* prints an error message to stderr */
static void errormsg(const char *msg, ...) {
  va_list ap;
  
  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);
  fputc('\n', stderr);
}

/* prints an error message to stderr, and dies */
static void fatal(const char *msg, ...) {
  va_list ap;

  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);
  fputc('\n', stderr);
  exit(EXIT_FAILURE);
}

/* memory allocation error handling */
static void check_mem(void *p) {
  if (!p) {
    fatal("Out of memory");
  }
}

/* error handling for ALSA functions */
static void check_snd(const char *operation, int err) {
  if (err < 0) {
    fatal("Cannot %s - %s", operation, snd_strerror(err));
  }
}

/* parses one port addresses from the string */
static void parse_port_in(snd_seq_t* seq, const char *arg) {
  char *buf, *s, *port_name;
  int err;
  
  /* make a copy of the string because we're going to modify it */
  buf = strdup(arg);
  check_mem(buf);
  
  for (port_name = s = buf; s; port_name = s + 1) {
    /* Assume that ports are separated by commas.  We don't use
     * spaces because those are valid in client names. */
    s = strchr(port_name, ',');
    if (s) {
      *s = '\0';
    }
    
    ++port_in_count;
    port_in = realloc(port_in, port_in_count * sizeof(snd_seq_addr_t));
    check_mem(port_in);
    
    err = snd_seq_parse_address(seq, &port_in[port_in_count - 1], port_name);
    if (err < 0) {
      fatal("Invalid port %s - %s", port_name, snd_strerror(err));
    }
  }
  
  free(buf);
}

/* parses one port addresses from the string */
static void parse_port_out(snd_seq_t* seq, const char *arg) {
  char *buf, *s, *port_name;
  int err;
  
  /* make a copy of the string because we're going to modify it */
  buf = strdup(arg);
  check_mem(buf);
  
  for (port_name = s = buf; s; port_name = s + 1) {
    /* Assume that ports are separated by commas.  We don't use
     * spaces because those are valid in client names. */
    s = strchr(port_name, ',');
    if (s) {
      *s = '\0';
    }
    
    ++port_out_count;
    port_out = realloc(port_out, port_out_count * sizeof(snd_seq_addr_t));
    check_mem(port_out);
    
    err = snd_seq_parse_address(seq, &port_out[port_out_count - 1], port_name);
    if (err < 0)
      fatal("Invalid port %s - %s", port_name, snd_strerror(err));
  }
  
  free(buf);
}

static void list_ports(snd_seq_t* seq) {
  snd_seq_client_info_t *cinfo;
  snd_seq_port_info_t *pinfo;

  snd_seq_client_info_alloca(&cinfo);
  snd_seq_port_info_alloca(&pinfo);

  puts(" Port    Client name                      Port name");

  snd_seq_client_info_set_client(cinfo, -1);
  while (snd_seq_query_next_client(seq, cinfo) >= 0) {
    int client = snd_seq_client_info_get_client(cinfo);

    snd_seq_port_info_set_client(pinfo, client);
    snd_seq_port_info_set_port(pinfo, -1);
    while (snd_seq_query_next_port(seq, pinfo) >= 0) {
      /* port must understand MIDI messages */
      if (!(snd_seq_port_info_get_type(pinfo)
	    & SND_SEQ_PORT_TYPE_MIDI_GENERIC))
	continue;
      /* we need both WRITE and SUBS_WRITE */
      if ((snd_seq_port_info_get_capability(pinfo)
	   & (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE))
	  != (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE))
	continue;
      printf("%3d:%-3d  %-32.32s %s\n",
	     snd_seq_port_info_get_client(pinfo),
	     snd_seq_port_info_get_port(pinfo),
	     snd_seq_client_info_get_name(cinfo),
	     snd_seq_port_info_get_name(pinfo));
    }
  }
}

static void dump_event(const snd_seq_event_t *ev) {
  printf("%3d:%-3d ", ev->source.client, ev->source.port);

  switch (ev->type) {
  case SND_SEQ_EVENT_NOTEON:
   printf("Note on                %2d %3d %3d\n",
	   ev->data.note.channel, ev->data.note.note, ev->data.note.velocity);
    break;
  case SND_SEQ_EVENT_NOTEOFF:
    printf("Note off               %2d %3d %3d\n",
	   ev->data.note.channel, ev->data.note.note, ev->data.note.velocity);
    break;
  case SND_SEQ_EVENT_KEYPRESS:
    printf("Polyphonic aftertouch  %2d %3d %3d\n",
	   ev->data.note.channel, ev->data.note.note, ev->data.note.velocity);
    break;
  case SND_SEQ_EVENT_CONTROLLER:
    printf("Control change         %2d %3d %3d\n",
	   ev->data.control.channel, ev->data.control.param, ev->data.control.value);
    break;
  case SND_SEQ_EVENT_PGMCHANGE:
    printf("Program change         %2d %3d\n",
	   ev->data.control.channel, ev->data.control.value);
    break;
  case SND_SEQ_EVENT_CHANPRESS:
    printf("Channel aftertouch     %2d %3d\n",
	   ev->data.control.channel, ev->data.control.value);
    break;
  case SND_SEQ_EVENT_PITCHBEND:
    printf("Pitch bend             %2d  %6d\n",
	   ev->data.control.channel, ev->data.control.value);
    break;
  case SND_SEQ_EVENT_CONTROL14:
    printf("Control change         %2d %3d %5d\n",
	   ev->data.control.channel, ev->data.control.param, ev->data.control.value);
    break;
  case SND_SEQ_EVENT_NONREGPARAM:
    printf("Non-reg. parameter     %2d %5d %5d\n",
	   ev->data.control.channel, ev->data.control.param, ev->data.control.value);
    break;
  case SND_SEQ_EVENT_REGPARAM:
    printf("Reg. parameter         %2d %5d %5d\n",
	   ev->data.control.channel, ev->data.control.param, ev->data.control.value);
    break;
  case SND_SEQ_EVENT_SONGPOS:
    printf("Song position pointer     %5d\n",
	   ev->data.control.value);
    break;
  case SND_SEQ_EVENT_SONGSEL:
    printf("Song select               %3d\n",
	   ev->data.control.value);
    break;
  case SND_SEQ_EVENT_QFRAME:
    printf("MTC quarter frame         %02xh\n",
	   ev->data.control.value);
    break;
  case SND_SEQ_EVENT_TIMESIGN:
    // XXX how is this encoded?
    printf("SMF time signature        (%#08x)\n",
	   ev->data.control.value);
    break;
  case SND_SEQ_EVENT_KEYSIGN:
    // XXX how is this encoded?
    printf("SMF key signature         (%#08x)\n",
	   ev->data.control.value);
    break;
  case SND_SEQ_EVENT_START:
    if (ev->source.client == SND_SEQ_CLIENT_SYSTEM &&
	ev->source.port == SND_SEQ_PORT_SYSTEM_TIMER)
      printf("Queue start               %d\n",
	     ev->data.queue.queue);
    else
      printf("Start\n");
    break;
  case SND_SEQ_EVENT_CONTINUE:
    if (ev->source.client == SND_SEQ_CLIENT_SYSTEM &&
	ev->source.port == SND_SEQ_PORT_SYSTEM_TIMER)
      printf("Queue continue            %d\n",
	     ev->data.queue.queue);
    else
      printf("Continue\n");
    break;
  case SND_SEQ_EVENT_STOP:
    if (ev->source.client == SND_SEQ_CLIENT_SYSTEM &&
	ev->source.port == SND_SEQ_PORT_SYSTEM_TIMER)
      printf("Queue stop                %d\n",
	     ev->data.queue.queue);
    else
      printf("Stop\n");
    break;
  case SND_SEQ_EVENT_SETPOS_TICK:
    printf("Set tick queue pos.       %d\n", ev->data.queue.queue);
    break;
  case SND_SEQ_EVENT_SETPOS_TIME:
    printf("Set rt queue pos.         %d\n", ev->data.queue.queue);
    break;
  case SND_SEQ_EVENT_TEMPO:
    printf("Set queue tempo           %d\n", ev->data.queue.queue);
    break;
  case SND_SEQ_EVENT_CLOCK:
    printf("Clock\n");
    break;
  case SND_SEQ_EVENT_TICK:
    printf("Tick\n");
    break;
  case SND_SEQ_EVENT_QUEUE_SKEW:
    printf("Queue timer skew          %d\n", ev->data.queue.queue);
    break;
  case SND_SEQ_EVENT_TUNE_REQUEST:
    printf("Tune request\n");
    break;
  case SND_SEQ_EVENT_RESET:
    printf("Reset\n");
    break;
  case SND_SEQ_EVENT_SENSING:
    printf("Active Sensing\n");
    break;
  case SND_SEQ_EVENT_CLIENT_START:
    printf("Client start              %d\n",
	   ev->data.addr.client);
    break;
  case SND_SEQ_EVENT_CLIENT_EXIT:
    printf("Client exit               %d\n",
	   ev->data.addr.client);
    break;
  case SND_SEQ_EVENT_CLIENT_CHANGE:
    printf("Client changed            %d\n",
	   ev->data.addr.client);
    break;
  case SND_SEQ_EVENT_PORT_START:
    printf("Port start                %d:%d\n",
	   ev->data.addr.client, ev->data.addr.port);
    break;
  case SND_SEQ_EVENT_PORT_EXIT:
    printf("Port exit                 %d:%d\n",
	   ev->data.addr.client, ev->data.addr.port);
    break;
  case SND_SEQ_EVENT_PORT_CHANGE:
    printf("Port changed              %d:%d\n",
	   ev->data.addr.client, ev->data.addr.port);
    break;
  case SND_SEQ_EVENT_PORT_SUBSCRIBED:
    printf("Port subscribed           %d:%d -> %d:%d\n",
	   ev->data.connect.sender.client, ev->data.connect.sender.port,
	   ev->data.connect.dest.client, ev->data.connect.dest.port);
    break;
  case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
    printf("Port unsubscribed         %d:%d -> %d:%d\n",
	   ev->data.connect.sender.client, ev->data.connect.sender.port,
	   ev->data.connect.dest.client, ev->data.connect.dest.port);
    break;
  case SND_SEQ_EVENT_SYSEX:
    {
      unsigned int i;
      printf("System exclusive         ");
      for (i = 0; i < ev->data.ext.len; ++i)
	printf(" %02X", ((unsigned char*)ev->data.ext.ptr)[i]);
      printf("\n");
    }
    break;
  default:
    printf("Event type %d\n",  ev->type);
  }
}

static void send_note(snd_seq_t* seq, int port, int queue, int tick) {
  snd_seq_event_t ev;

  snd_seq_ev_clear(&ev);
  snd_seq_ev_set_subs(&ev);
  snd_seq_ev_set_source(&ev, port);
  snd_seq_ev_schedule_tick(&ev, queue, tick, 4);
  snd_seq_ev_set_note(&ev, 0, 60, 127, 1);
  snd_seq_event_output(seq, &ev);
  snd_seq_drain_output(seq);
}

// end: alsa foo

static void usage(const char *argv0) {
  printf(" Usage: %s -o client:port -i client:port ...\n\n"
	 "  -o, --output=client:port   set port to send events to.\n"
	 "  -i, --input=client:port    set port to receive events from.\n"
	 "  -l, --list                 list available midi in/out ports.\n\n"
	 "  -R, --realtime             use realtime scheduling. (default: no)\n"
         "  -P, --priority=int         specify scheduling priority. Use with -R.\n"
	 "                             (default: max. realtime priority)\n\n"
	 "  -S, --samples=# of samples to take for the measurement. (default: 10000)\n"
	 "  -s, --skip=# of samples    to skip measuring latency of.\n"
	 "                             (default: 0)\n\n"
	 "  -h, --help                 print this help.\n"
	 "  -V, --version              print current version.\n"
	 "\n", argv0);
}

static void version(void) {
  printf("%s %s\n", PACKAGE, VERSION);
}

static volatile sig_atomic_t stop = 0;

static void sighandler(int sig) {
  stop = 1;
}

int main(int argc, char *argv[]) {
  int client;

  snd_seq_t* seq;
  int queue, i, j;
  int port;

  snd_seq_queue_tempo_t* tempo;

  long delay_hist[10000];
  for (i = 0; i < 9999; i++) {
    delay_hist[i] = 0;
  }

  int npfd, l1;
  struct pollfd *pfd;

  /* open sequencer */
  int err = snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0);
  check_snd("open sequencer", err);

  static char short_options[] = "hVlo:i:RP:s:S:";
  static struct option long_options[] = {
    {"help", 0, NULL, 'h'},
    {"version", 0, NULL, 'V'},
    {"list", 0, NULL, 'l'},
    {"output", 1, NULL, 'o'},
    {"input", 1, NULL, 'i'},
    {"realtime", 0, NULL, 'R'},
    {"priority", 0, NULL, 'P'},
    {"skip", 1, NULL, 's'},
    {"samples", 1, NULL, 'S'},
    {}
  };

  int c;
  int do_list = 0;
  int do_realtime = 0;
  int rt_prio = -1;
  int skip_samples = 0;
  int nr_samples = 10000;

  while ((c = getopt_long(argc, argv, short_options,
			  long_options, NULL)) != -1) {
    switch (c) {
    case 'h':
      usage(argv[0]);
      snd_seq_close(seq);
      return EXIT_SUCCESS;
    case 'V':
      version();
      snd_seq_close(seq);
      return EXIT_SUCCESS;
    case 'l':
      do_list = 1;
      break;
    case 'o':
      parse_port_out(seq, optarg);
      break;
    case 'i':
      parse_port_in(seq, optarg);
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

  if (do_list) {
    list_ports(seq);
    snd_seq_close(seq);
    return EXIT_SUCCESS;
  }

  if (port_out_count < 1) {
    errormsg("Please specify one output port with --output. Use -l to get a list.");
    return EXIT_FAILURE;
  }

  if (port_in_count < 1) {
    errormsg("Please specify one input port with --input. Use -l to get a list.");
    return EXIT_FAILURE;
  }

  /* set our name (otherwise it's "Client-xxx") */
  err = snd_seq_set_client_name(seq, "alsa-midi-latency-test");
  check_snd("set client name", err);

  /* find out who we actually are */
  client = snd_seq_client_id(seq);
  check_snd("get client id", client);

  if ((port = snd_seq_create_simple_port(seq, "alsa-midi-latency-test",
					 SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE |
					 SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
					 SND_SEQ_PORT_TYPE_APPLICATION)) < 0) {
    fprintf(stderr, "Error creating sequencer port.\n");
    exit(EXIT_FAILURE);
  }

  for (i = 0; i < port_out_count; ++i) {
    err = snd_seq_connect_to(seq, port, port_out[i].client, port_out[i].port);
    if (err < 0)
      fatal("Cannot connect output port %d:%d - %s",
	    port_out[i].client, port_out[i].port, snd_strerror(err));
  }

  for (i = 0; i < port_in_count; ++i) {
    err = snd_seq_connect_from(seq, port, port_in[i].client, port_in[i].port);
    if (err < 0)
      fatal("Cannot connect to input port %d:%d - %s",
	    port_in[i].client, port_in[i].port, snd_strerror(err));
  }

  queue = snd_seq_alloc_queue(seq);

  snd_seq_queue_tempo_alloca(&tempo);
  snd_seq_queue_tempo_set_tempo(tempo, 500000);
  snd_seq_queue_tempo_set_ppq(tempo, 8);
  snd_seq_set_queue_tempo(seq, queue, tempo);

  npfd = snd_seq_poll_descriptors_count(seq, POLLIN);
  pfd = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
  snd_seq_poll_descriptors(seq, pfd, npfd, POLLIN);

  snd_seq_start_queue(seq, queue, NULL);

  double mytime1, mytime2;
  double midi_delay, max_delay;
  max_delay = 0.0;

  if (do_realtime) {
    printf("> set_realtime_priority(SCHED_FIFO, %d).. ", rt_prio);
    set_realtime_priority(SCHED_FIFO, rt_prio);
    printf("done.\n");
  }

  calibrate_loop();

  snd_seq_event_t *mev;
  int gotnote = 1;
  int tick = 0;

  unsigned long sample_nr = 0;

  printf("\n> sampling %d midi latency values - please wait..\n", nr_samples);
  if (skip_samples > 0) {
    if (skip_samples == 1) {
      printf("> skipping first latency sample.\n");
    } else {
      printf("> skipping first %d latency samples.\n", skip_samples);
    }
  }
  sleep(1);

  if (DEBUG) {
    printf("\nsample; latency_ms; latency_ms_worst\n");
  }

  signal(SIGINT,  sighandler);
  signal(SIGTERM, sighandler);

  //send_note(seq, port_out[0].port, queue, 0);
  while (1) {
    if (gotnote == 1) {
      gotnote = 0;
      mytime1 = mygettime();
      send_note(seq, port_out[0].port, queue, 0);
    }

    usleep(150); // wtf!?!!
    if (poll(pfd, npfd, 1000) > 0) {
      for (l1 = 0; l1 < npfd; l1++) {
        if (pfd[l1].revents > 0) {
	  do {
	    snd_seq_event_input(seq, &mev);
	    mytime2 = mygettime();
	    
	    // DEBUG dump_event(mev);
	    if (mev->type ==  SND_SEQ_EVENT_NOTEON) {
	      sample_nr++;

	      midi_delay = mytime2 - mytime1;

	      int delay_hist_slot = (int)(midi_delay * 10000.0);
	      if (delay_hist_slot > 9999)
		delay_hist_slot = 9999;

	      delay_hist[delay_hist_slot]++;

	      if (sample_nr <= skip_samples) {
		//if (DEBUG) printf("skipping sample %d\n", sample_nr);
	      } else if (midi_delay > max_delay) {
		max_delay = midi_delay;
		if (DEBUG) {
		  printf("%6d; %10.2f; %10.2f     \n", sample_nr, midi_delay * 1000.0, max_delay * 1000.0);
		}
	      } else {
		if (DEBUG) {
		  printf("%6d; %10.2f; %10.2f     \r", sample_nr, midi_delay * 1000.0, max_delay * 1000.0);
		}
	      }
	      gotnote = 1;
	    }
	    snd_seq_free_event(mev);
	  } while (snd_seq_event_input_pending(seq, 0) > 0);
	}
      }

      if (stop) {
	break;
      }
    }

    if (sample_nr >= nr_samples) {

      printf("\n> done.\n\n> latency distribution:\n");

      // try to make ascii bars thinner than 50 columns
      int max_samples = 1;
      for (i = 0; i < 9999; ++i) {
	if (delay_hist[i] > max_samples) max_samples = delay_hist[i];
      }
      int graph_step = (int)((float)(max_samples / 50.0));

      // plot ascii bars
      int skip = 0;
      for (i = 0; i < max_delay * 10000; i++) {
	if (delay_hist[i] > 0) {
	  printf("%5.1f -%5.1f ms: %8d ", i/10.0, i/10.0 + 0.09, delay_hist[i]);
	  skip = 0;
	} else {
	  skip++;
	  if (skip == 1)
	    printf("...\n"); // skip intervals without samples, write "..." only once
	}
	for (j = 0; j < delay_hist[i]; j+= graph_step) {
	  printf("#");
	}
	if (skip == 0)
	  printf("\n");
      }

      if (max_delay * 1000.0 > 6.0) { // latencies <= 6ms are o.k. imho
	printf("\n> FAIL\n");
	printf("\n worst latency was %.2f ms, which is too much. Please check:\n\n", max_delay*1000.0);
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
	printf("\n worst latency was %.2f ms, which is great.\n\n", max_delay*1000.0);

	snd_seq_close(seq);
	return EXIT_SUCCESS;
      }

      break;
    }
  }

  snd_seq_close(seq);
  return EXIT_SUCCESS;
}
