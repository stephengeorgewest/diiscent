#.PHONY: sdl_wii gl2gx libogg libtheora libvorbis memtracer netport openal-soft profiler fs2_open all clean gnulib

default:
all: descent
#fs2_open 

sdl_wii:
	@echo "sdl_wii:"
	make -C sdl_wii
	
gl2gx:
	@echo "gl2gx"
	make -C gl2gx
	
libogg:
	@echo "libogg"
	make -C libogg
	
libtheora:
	@echo "libtheora"
	make -C libtheora
	
libvorbis:
	@echo "libvorbis"
	make -C libvorbis
	
memtracer:
	@echo "memtracer"
	make -C memtracer
	#doesn't compile with devkit... missing unix libraries
	#make -C memtracer/src/parser
	
netport:
	@echo "netport"
	make -C netport
	
openal-soft:
	@echo "openal-soft"
	make -C openal-soft

profiler:
	@echo "profiler"
	make -C profile_tools/binutil_dll
	make -C profile_tools/wiitrace/tracer
	make -C profile_tools/wiitrace/parser
	
gnulib:
	@echo "gnulib"
	make -C gnulib
	
#descent: sdl_wii gl2gx netport
descent:
	@echo "descent"
	make -C "D2X-XL"
	
#fs2_open: sdl_wii gl2gx libogg libtheora libvorbis memtracer netport openal-soft profiler gnulib
#	@echo "fs2_open"
#	-rm fs2_open/code/freespace2.elf
#	make -C fs2_open/code

clean:
	@echo "cleaning ..."
	make -C sdl_wii clean
	make -C gl2gx clean
	#make -C libogg clean
	#make -C libtheora clean
	#make -C libvorbis clean
	#make -C memtracer clean
	#make -C memtracer/src/parser clean
	#make -C netport clean
	#make -C openal-soft clean
	#make -C profile_tools/wiitrace/tracer clean
	#make -C profile_tools/wiitrace/parser clean
	#make -C profile_tools/binutil_dll clean
	#make -C gnulib clean
	#make -C fs2_open/code clean
	make -C descent clean