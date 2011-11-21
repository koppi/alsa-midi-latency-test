# alsa-midi-latency-test

alsa-midi-latency-test measures the roundtrip time of a MIDI message in the alsa subsystem of the linux kernel using a high precision timer. It calculates the worst case roundtrip time of all sent MIDI messages and displays a histogram of the rountrip time jitter:

![alsa-midi-latency-test](https://raw.github.com/koppi/alsa-midi-latency-test/master/alsa-midi-latency-test.gif "alsa midi latency test")

## Get the source code

```
 $ wget -O alsa-midi-latency-test.zip https://github.com/koppi/alsa-midi-latency-test/zipball/master
 $ unzip alsa-midi-latency-test.zip
 $ cd alsa-midi-latency-test*
```

or

```
 $ git clone https://koppi@github.com/koppi/alsa-midi-latency-test.git
 $ cd alsa-midi-latency-test/
```

## Compile / Install

The following packages are required to build alsa-midi-latency-test:

```
 $ sudo apt-get install debhelper autotools-dev automake libasound2-dev
```

Compile alsa-midi-latency-test as follows:

```
 $ sh autogen.sh
 $ ./configure
 $ make
```

Install alsa-midi-latency-test as follows:

```
 $ sudo make install
```

or use checkinstall:

```
 $ sudo apt-get install checkinstall
 $ sudo checkinstall
```

or build and install an Ubuntu / Debian package:

```
 $ debuild
 $ sudo dpkg -i ../alsa-midi-latency-test*.deb
```

## Run alsa-midi-latency-test

 * ``` $ alsa-midi-latency-test -l ```

    Lists available MIDI input and output ports.

 * ``` $ alsa-midi-latency-test -i [input port] -o [output port] ```

    This runs the benchmark with the given input and output port. Note, that the
    input and output ports have to be connected using a MIDI cable in the real
    hardware to loop the MIDI message back.

 * ``` $ man alsa-midi-latency-test ```

    The man page contains documentation for all available command line switches.

### Benchmark Results

Benchmark results for various MIDI adapters can be found in [benchmarks/](alsa-midi-latency-test/blob/master/benchmarks). The setup of the MIDI loop back chains is as follows:

 * [PC - USB - Elektron TM-1 OUT - MIDI loop - Elektron TM-1 IN - USB - PC](alsa-midi-latency-test/blob/master/benchmarks/elektron-tm1.txt)
 * [PC - USB - Polytec GM5 midibox OUT - MIDI loop - Polytec GM5 midibox IN - USB - PC](alsa-midi-latency-test/blob/master/benchmarks/gm5x5x5.txt)
 * [PC - Edirol UA-25EX - PC](alsa-midi-latency-test/blob/master/benchmarks/um2ex.txt)
 * [PC - M-Audio 2496 - PC](alsa-midi-latency-test/blob/master/benchmarks/m-audio-2496.txt)
 * [PC - USB - MidiSport 2x2 'Anniversary Edition' port B OUT - MidiSport 2x2 'Anniversary Edition' port B IN - USB - PC](alsa-midi-latency-test/blob/master/benchmarks/midisport2x2ann.txt)
 * [PC - MPU-401-Port on motherboard - PC](alsa-midi-latency-test/blob/master/benchmarks/mpu401.txt)
 * [PC - Edirol UM-2EX - PC](alsa-midi-latency-test/blob/master/benchmarks/um2ex.txt)
 * [PC - Yamaha UX256 (6-Port) - PC](alsa-midi-latency-test/blob/master/benchmarks/yamaha-ux256.txt)

If you have an interesting setup, please send [us](https://github.com/koppi) you results so we can include them here for further reference.

### User Experiences with alsa-midi-latency-test

 * [64studio-users](http://www.mail-archive.com/64studio-users@lists.64studio.com)
     * [Ralf Mardorf - USB Device 0x170b:0x11, TerraTec EWX24/96](http://www.mail-archive.com/64studio-users@lists.64studio.com/msg02047.html)

## See also

 * http://www.linuxaudio.org/mailarchive/lat/

    The linux-audio tuning (LAT) mailing list is to help GNU/Linux distribution
    maintainers  and  other interested users to share information on system
    performance tuning matters, especially with regard to real-time Linux
    kernels.

 * http://www.alsa-project.org/

    The Advanced Linux Sound Architecture.

 * http://earthvegaconnection.com/evc/products/miditest

    The MidiTest software for Microsoft Windows.

## Thanks

 * to Arnout Engelen for initial testing and giving feedback.
 * to Clemens Ladisch for a number of fixes with the high precision timer and
   alsa midi event handling.

## BUGS and AUTHORS

Please report bugs to the authors.

 * Jakob Flierl <jakob.flierl@gmail.com>

-- November, 2009, last updated November 2011.
