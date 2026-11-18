# CMake build of JonoF's TekWar port for the MiSTer hybrid core (the jfSW
# core), after jfSW_MiSTer's jfsw/CMakeLists.txt: classic software renderer
# only, SDL2 with the MiSTer drivers, the engine and sound library of
# ItsDanik's "mister" branches (jfbuild/, jfaudiolib/).
cmake_minimum_required(VERSION 3.16)
project(jftekwar C)

option(MISTER_HYBRID "Build for the MiSTer hybrid core (needs SDL2 with the MiSTer drivers)" ON)
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS ON)

find_package(SDL2 REQUIRED CONFIG)
get_target_property(SDL2_INCLUDES SDL2::SDL2-static INTERFACE_INCLUDE_DIRECTORIES)

set(ENGINE jfbuild/src)
set(AUDIO jfaudiolib/src)

add_executable(jftekwar
	${ENGINE}/a-c.c
	${ENGINE}/asmprot.c
	${ENGINE}/baselayer.c
	${ENGINE}/cache1d.c
	${ENGINE}/compat.c
	${ENGINE}/crc32.c
	${ENGINE}/defs.c
	${ENGINE}/engine.c
	${ENGINE}/kplib.c
	${ENGINE}/mmulti.c
	${ENGINE}/osd.c
	${ENGINE}/pragmas.c
	${ENGINE}/scriptfile.c
	${ENGINE}/textfont.c
	${ENGINE}/talltextfont.c
	${ENGINE}/smalltextfont.c
	${ENGINE}/sdlayer2.c
	${ENGINE}/startwin_stub.c
	${ENGINE}/version.c

	${AUDIO}/drivers.c
	${AUDIO}/fx_man.c
	${AUDIO}/cd.c
	${AUDIO}/multivoc.c
	${AUDIO}/mix.c
	${AUDIO}/mixst.c
	${AUDIO}/pitch.c
	${AUDIO}/music.c
	${AUDIO}/midi.c
	${AUDIO}/driver_nosound.c
	${AUDIO}/driver_sdl.c
	${AUDIO}/driver_misterfm.c
	${AUDIO}/gmtimbre.c
	${AUDIO}/asssys.c

	libsmacker/smacker_jftekwar.c

	src/b5compat.c
	src/config.c
	src/tekcdr.c
	src/tekchng.c
	src/tekgame.c
	src/tekgun.c
	src/tekldsv.c
	src/tekmap.c
	src/tekmsc.c
	src/tekprep.c
	src/teksmk.c
	src/teksnd.c
	src/tekspr.c
	src/tekstat.c
	src/tektag.c
	src/tektxt.c
	src/tekver.c
	src/datascan.c
	src/version.c
	src/mister_port.c
)

target_include_directories(jftekwar PRIVATE
	src jfbuild/include jfbuild/src jfaudiolib/include jfaudiolib/src libsmacker
	${SDL2_INCLUDES})
target_compile_definitions(jftekwar PRIVATE
	MISTER_HYBRID=1 RENDERTYPESDL=1 HAVE_SDL=2 _GNU_SOURCE)
target_compile_options(jftekwar PRIVATE -fno-strict-aliasing -w)
target_link_libraries(jftekwar PRIVATE SDL2::SDL2-static m)
