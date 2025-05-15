``alsa-midi-latency-test`` is a Linux™-based tool used to measure the roundtrip delay of MIDI messages within the ALSA (Advanced Linux™ Sound Architecture) subsystem. The tool utilizes a high-precision timer to calculate the time it takes for a MIDI message to travel from the ALSA source to the ALSA sink, and back. It's a helpful way to understand and troubleshoot MIDI latency issues, often in situations where noticeable delay is encountered between playing a MIDI keyboard and hearing the sound. 

![alsa-midi-latency-test](https://raw.github.com/koppi/alsa-midi-latency-test/master/alsa-midi-latency-test.gif "alsa midi latency test")

* Roundtrip Measurement:

  The tool measures the total time it takes for a MIDI message to complete a round trip, meaning from the output of one device to the input of another.

* High-Precision Timer:

  It uses a high-precision timer to accurately measure the delay, which is crucial for precise latency measurements. 

* ALSA Subsystem Focus:
  
  The tool specifically focuses on the ALSA subsystem, allowing users to diagnose latency issues related to Linux's™ audio/MIDI infrastructure.

How alsa-midi-latency-test can help:

* Diagnosing MIDI Latency:
The tool can help users pinpoint the source of latency, whether it's related to the MIDI hardware, software drivers, or system configuration. 

* Optimizing ALSA Settings:

  The measurements can guide users in adjusting ALSA settings, such as buffer sizes, to minimize latency.

* Identifying System Bottlenecks:

  In some cases, latency can be a result of system limitations, and ``alsa-midi-latency-test`` can help identify those bottlenecks.

In summary, ``alsa-midi-latency-test`` is a valuable tool for anyone working with MIDI and ALSA on Linux™, allowing them to measure and troubleshoot latency issues effectively. 


## Install from source code
```bash
git clone https://github.com/koppi/alsa-midi-latency-test.git
cd alsa-midi-latency-test/
```
The following packages are required to build alsa-midi-latency-test:
```bash
sudo apt -y install debhelper autotools-dev automake libasound2-dev
```
Compile alsa-midi-latency-test as follows:
```bash
sh autogen.sh
./configure
make
```
Install alsa-midi-latency-test as follows:
```bash
sudo make install
```
or build and install an Ubuntu / Debian package:
```bash
sudo apt -y install devscripts
debuild -uc -us
sudo dpkg -i ../alsa-midi-latency-test*.deb
```

– Tested on Ubuntu 25.04 [![.github/workflows/ubuntu.yml](https://github.com/koppi/alsa-midi-latency-test/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/koppi/alsa-midi-latency-test/actions/workflows/ubuntu.yml)

## Run alsa-midi-latency-test
 * ``` alsa-midi-latency-test -l ```

   Lists available MIDI input and output ports.

 * ``` alsa-midi-latency-test -i [input port] -o [output port] ```

   This runs the benchmark with the given input and output port. Note, that the
   input and output ports have to be connected using a MIDI cable in the real
   hardware to loop the MIDI message back.

 * ``` man alsa-midi-latency-test ```

   The man page contains documentation for all available command line switches.

### Benchmark Results

   Please share your results in the [Wiki](../../wiki/).

### Definitions

 * [Wikipedia EN - Latency in engineering](http://tinyurl.com/wikipedia-latency-engineering)
 * [Wikipedia EN - Latency with audio](http://tinyurl.com/wikipedia-latency-audio)

## See also

 * http://lists.linuxaudio.org/listinfo/linux-audio-tuning

   The linux-audio tuning (LAT) mailing list is to help GNU/Linux™ distribution
   maintainers  and  other interested users to share information on system
   performance tuning matters, especially with regard to real-time Linux™
   kernels.

 * http://www.alsa-project.org/

   The Advanced Linux™ Sound Architecture.

 * [http://www.evc-soft.nl/evc/products/miditest/](http://web.archive.org/web/20061128213256/http://www.evc-soft.nl/evc/products/miditest/)

   The MidiTest software for Microsoft Windows.

 * https://codeberg.org/rtcqs/rtcqs

   rtcqs is a Python utility to analyze your system and detect possible bottlenecks that could have a negative impact on the performance of your system when working with Linux™ audio.

## Thanks to

 * [Arnout Engelen](https://github.com/raboof) for initial testing.
 * - [Clemens Ladisch](https://github.com/cladisch) for a number of fixes with the high precision timer and ALSA midi event handling.
 * - [Giulio Moro](https://github.com/giuliomoro) for code cleanup, various fixes and support for UART I/O.

## BUGS and AUTHORS

Please [report bugs](https://github.com/koppi/alsa-midi-latency-test/issues) to the authors:

 * [Jakob Flierl](https://github.com/koppi)

Last updated May 2025.
