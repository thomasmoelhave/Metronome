Metronome
===================

This program is tested on GNU/Linux (using Ubuntu 11.10) only, but will probably work in most places.
It requires boost, and LAME for converting WAVE files to mp3.

To use, run cmake, then make to build the metronome binary. You can run the program like this:


		./metronome   --bpm 160 --duration 100 --output-file bpm160.wav 


This produces an output WAVE file (bpm160.wav) with a pulse every 160 beats for 100 seconds.


