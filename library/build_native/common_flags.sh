
FLAGS="$FLAGS --disable-doc"
FLAGS="$FLAGS --disable-programs"
FLAGS="$FLAGS --disable-avdevice"
FLAGS="$FLAGS --disable-swscale"
FLAGS="$FLAGS --disable-avfilter"
FLAGS="$FLAGS --enable-avresample" 
if (( $FFMPEG ));  then
FLAGS="$FLAGS --disable-swresample"
fi
FLAGS="$FLAGS --disable-bsfs"
FLAGS="$FLAGS --disable-indevs"
FLAGS="$FLAGS --disable-outdevs"
FLAGS="$FLAGS --disable-devices"
FLAGS="$FLAGS --disable-filters"
FLAGS="$FLAGS --disable-muxers"
FLAGS="$FLAGS --disable-encoders"
FLAGS="$FLAGS --disable-parsers"
FLAGS="$FLAGS --disable-decoders"
FLAGS="$FLAGS --disable-protocols"
FLAGS="$FLAGS --disable-demuxers"
FLAGS="$FLAGS --disable-debug"
FLAGS="$FLAGS --enable-small --enable-runtime-cpudetect"
FLAGS="$FLAGS --extra-ldexeflags=-pie" 

FLAGS="$FLAGS --enable-demuxer=asf,ac3,aac,flac,m4v,mp3,ogg,pcm_alaw,pcm_f32be,pcm_f32le,pcm_f64be,pcm_f64le,pcm_mulaw,pcm_s16be,pcm_s16le,pcm_s24be"
FLAGS="$FLAGS --enable-demuxer=aac,ac3,asf,au,avi,flac,flv,pcm_u32be,pcm_u32le,pcm_u8,m4v,avi,matroska,rtp,rtsp,mov,mp3,ogg,flac,pcm_alaw,pcm_f32be,pcm_f32le,pcm_f64be,pcm_f64le,pcm_mulaw,wav,pcm_s16be,pcm_s16le,pcm_s24be,pcm_s24le,pcm_s32be,pcm_s32le,pcm_s8,pcm_u16be,pcm_u16le,pcm_u24be,pcm_u24le"
FLAGS="$FLAGS --enable-parser=ac3,aac,aac_latm,flac,mpegaudio,vorbis,vp3,vp8,vp9"
FLAGS="$FLAGS --enable-decoder=aac,aac_latm,flac,mjpeg,mpeg4,mp3,vorbis,wmalossless,wmapro,wmav1,wmav2,wmavoice"
FLAGS="$FLAGS --enable-protocol=pipe,file,http,https,mmsh,mmst,rtmps,rtmpt,concat,tcp,udp,rtp,srtp,icecast"





