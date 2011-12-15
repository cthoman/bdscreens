BDScreens - Blu-ray Screenshot Tool
===================================

At the moment this is more sample code than an actual program, but I thought someone might find 
this code snippet useful even in its current minimal form.  The current sample code extracts
all of the [I-frames][iframe] from an input Blu-ray [.M2TS][m2ts] stream file and saves them to 
disc as lossless .PNG using the [FFmpeg libavcodec][ffmpeg] libraries.

Usage
-----

Example of typical command-line usage:

     mkdir temp
     cd temp
     ..\bdscreens x:\bdmv\stream\00000.m2ts

Necessary libavcodec headers and binary files for Windows can be obtained from [ffmpeg.zeranoe.com][zeranoe].

Motivation
----------

I am sometimes asked how I create and select the Blu-ray screenshots found on [cinemasquid.com][screenshots].

The answer is somewhat long and convoluted. It's made more complicated by the unfortunate fact that 
most Blu-ray software players, both commercial and open-source, have certain limitations or outright problems 
that prevent them from being totally suited to the workflow and quality I prefer.

Some key points about this sample:

* Frames are extracted to disc, so can be browsed and culled using a veritable cornucopia of image viewers, which I find more convenient than fussing with jump, pause, rewind, framestep, snapshot, etc. in a media player.
* The colorspace matrix for RGB conversion is set to [BT.709][bt709] rather than [BT.601][bt601] to provide accurate colors.  Shockingly, this is still an issue going into 2012 with popular open-source players such as [VLC][vlc] and [mplayer][mplayer].
* Raw YUV 4:2:0 frames are first upsampled to YUV 4:4:4 to mitigate loss of information due to [chroma subsampling][chroma] during the conversion to RGB.  This is a bit more subtle than the colorspace issue, but without it artifacts such as chroma bleeding on reds are sometimes noticeable in the resultant RGB.

[iframe]: http://en.wikipedia.org/wiki/Video_compression_picture_types
[m2ts]: http://en.wikipedia.org/wiki/.m2ts
[ffmpeg]: http://ffmpeg.org/
[screenshots]: http://www.cinemasquid.com/blu-ray/movies/screenshots
[zeranoe]: http://ffmpeg.zeranoe.com/builds/
[chroma]: http://en.wikipedia.org/wiki/Chroma_subsampling
[bt709]: http://en.wikipedia.org/wiki/Rec._709
[bt601]: http://en.wikipedia.org/wiki/Rec._601
[vlc]: http://www.videolan.org/vlc/
[mplayer]: http://www.mplayerhq.hu/