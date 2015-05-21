
# set this to 1 for ffmpeg or 0 for libav
FFMPEG := 0

#TODO: set to 1 to enable openssl support (not working as yet)
SSL := 0
include $(call all-subdir-makefiles)


