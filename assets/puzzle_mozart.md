Puzzle background: Mozart, Symphony No. 40 in G minor, K. 550, first movement,
performed by Musopen Symphony. The recording was released into the public domain
by Musopen: https://commons.wikimedia.org/wiki/File:Mozart_-_Symphony_No._40_in_G_minor,_K550_-_I._Molto_allegro_(Musopen_Symphony).flac

The embedded file contains the first 20 seconds, slowed to 85% without changing
pitch, dynamically compressed and normalized, faded at the loop boundaries, and encoded as mono 22.05 kHz
IMA ADPCM to fit the console's flash. Source copy:
https://www.orangefreesounds.com/wp-content/uploads/2016/08/Mozart-symphony-40.mp3

Conversion:
ffmpeg -i source.mp3 -t 20 -af 'atempo=0.85,acompressor=threshold=0.08:ratio=3:attack=20:release=250:makeup=2,loudnorm=I=-12:TP=-2:LRA=6,alimiter=limit=0.68:level=false,afade=t=in:st=0:d=0.35,afade=t=out:st=19.2:d=0.8' -ar 22050 -ac 1 -c:a adpcm_ima_wav puzzle_mozart.wav
