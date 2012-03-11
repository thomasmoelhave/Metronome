#!/bin/bash
args=$@
args=$(seq -w -s' ' 10 10 300)
duration=120 #two minutes

idartist="Thomas Mølhave"
idalbum="Metronome"
idyear=2012
idcomment="Generated using Metronome"

tmpfile=$(mktemp)

trap "rm $tmpfile" EXIT

for bpm in $args; do
	
	wavfile=bpm-$bpm-$duration's'.wav
	mp3file="$(basename $wavfile .wav).mp3"

	idtitle="$bpm Beats per Minute"

	./metronome --bpm $bpm --output-file $wavfile --duration $duration 
	
	echo	lame --preset medium --tt \"$idtitle\" --ta \"$idartist\" --ty \" $idyear\" --tl \"$idalbum\" --tc \"$idcomment\" $wavfile $mp3file > $tmpfile
	source $tmpfile
done;

