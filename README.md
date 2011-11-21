# alsa-midi-latency-test

alsa-midi-latency-test measures the roundtrip time of a MIDI message in the alsa subsystem of the linux kernel using a high precision timer. It calculates the worst case roundtrip time of all sent MIDI messages and displays a histogram of the rountrip time jitter.

## Compile / Install

The following libraries are required to build alsa-midi-latency-test:

 * debhelper (>= 5)
 * autotools-dev
 * automake
 * libasound2-dev (>= 1.0.13)

```
$ sudo apt-get install debhelper autotools-dev automake libasound2-dev
```

Compile and install alsa-midi-latency-test as follows:

```
 $ sh autogen.sh
 $ ./configure
 $ make
 $ sudo make install
```

Or build and install an Ubuntu / Debian package:

```
 $ debuild
 $ sudo dpkg -i ../alsa-midi-latency-test*.deb
```

## Run

Run alsa-midi-latency-test as follows:

 * ```$ alsa-midi-latency-test -l```

    Lists available MIDI input and output ports.

 * ```$ alsa-midi-latency-test -i [input port] -o [output port]```

    This runs the benchmark with the given input and output port. Note, that the
    input and output ports have to be connected using a MIDI cable in the real
    hardware to loop the MIDI message back.

 * ```$ man alsa-midi-latency-test```

    The man page contains documentation for all available command line switches.

## Benchmark Results

Results of some benchmarks created with alsa-midi-latency-test can be found in the benchmarks/ sub-directory. The setup of the MIDI-chains of the results is as follows:

 * [PC - Elektron TM-1 - PC](alsa-midi-latency-test/blob/master/benchmarks/elektron-tm1.txt)
 * [PC - Polytec GM5 midibox - PC](alsa-midi-latency-test/blob/master/benchmarks/gm5x5x5.txt)
 * [PC - Edirol UA-25EX - PC](alsa-midi-latency-test/blob/master/benchmarks/)
 * [PC - M-Audio 2496 - PC](alsa-midi-latency-test/blob/master/benchmarks/m-audio-2496.txt)
 * [PC - MidiSport 2x2 'Anniversary Edition' port B - PC](alsa-midi-latency-test/blob/master/benchmarks/midisport2x2ann.txt)
 * [PC - MPU-401-Port on motherboard - PC](alsa-midi-latency-test/blob/master/benchmarks/mpu401.txt)
 * [PC - Edirol UM-2EX - PC](alsa-midi-latency-test/blob/master/benchmarks/um2ex.txt)
 * [PC - Yamaha UX256 (6-Port) - PC](alsa-midi-latency-test/blob/master/benchmarks/yamaha-ux256.txt)

If you have an interesting setup, please send us you results so we can include them here for further reference.

## See also

 * http://www.linuxaudio.org/mailarchive/lat/

    The linux-audio tuning (LAT) mailing list is to help GNU/Linux distribution
    maintainers  and  other interested users to share information on system
    performance tuning matters, especially with regard to real-time Linux
    kernels.

 * http://www.alsa-project.org/

    The Advanced Linux Sound Architecture.

## Thanks

 * to Arnout Engelen for initial testing and giving feedback.
 * to Clemens Ladisch for a number of fixes with the high precision timer and
   alsa midi event handling.

## BUGS and AUTHORS

Please report bugs to the authors.

 * Jakob Flierl <jakob.flierl@gmail.com>

-- November, 2009, last updated November 2011.
