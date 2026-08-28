#
# ArepoVTK
# Dylan Nelson
#

# user-configurable
# -----------------

EXECNAME = ArepoRT

#OPT += -DDEBUG          # enable verbose diagnostics and checks
#OPT  += -DENABLE_OPENGL # unused
#OPT  += -DENABLE_CUDA   # unused

# system
# -------

ARCH = $(shell uname)

OPTIMIZE = -Wall -g -m64 -O3 #-pg #enable profiler
INCL     = -I ./arepo/

CC       = mpicxx
LIBS     = -fopenmp -lm -L ./arepo/ 

CFLAGS   = $(OPTIMIZE) -DH5_USE_16_API

# module load gcc gsl fftw-serial hdf5-serial impi
CFLAGS += -I${GSL_HOME}/include -I${HDF5_HOME}/include -I./libpng/
LIBS += -L${GSL_HOME}/lib -L${HDF5_HOME}/lib -L./libpng/

OBJS = ArepoRT.o camera.o fileio.o fileio_img.o geometry.o integrator.o keyframe.o renderer.o sampler.o stellar_camera_path_v055.o transfer.o transform.o util.o volume.o snapio.o
HEAD = ArepoRT.h camera.h fileio.h fileio_img.h geometry.h integrator.h keyframe.h renderer.h sampler.h spectrum.h stellar_camera_path_v055.h stellar_camera_v054.h stellar_feature_framing_v067.h stellar_feature_landmarks_v066.h stellar_feature_profile_v065.h stellar_palette_v057.h transfer.h transform.h util.h volume.h snapio.h
MISC_RM = frame.raw.txt frame.tga

# ENABLE_AREPO
OBJS += arepo.o arepoTree.o arepoInterp.o stellar_gpu_scene_export_v052a.o voronoi_3db.o
HEAD += arepo.h arepoTree.h
LIBS += -larepo -lgsl -lgslcblas -lgmp -lhdf5 -pthread -lpng16 #-lhwloc

OBJS := $(addprefix build/,$(OBJS))
INCL := $(addprefix src/,$(INCL))

.PHONY: libarepo.a

$(EXECNAME): libarepo.a $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(OPT) -o $(EXECNAME) $(LIBS)

stellar_camera_director_v056: src/stellar_camera_director_v056.cpp src/stellar_cinematic_director_v056.cpp src/stellar_cinematic_director_v056.h src/stellar_camera_v054.h
	$(CC) $(CFLAGS) -Isrc src/stellar_cinematic_director_v056.cpp src/stellar_camera_director_v056.cpp -o $@

stellar_camera_director_v059: src/stellar_camera_director_v059.cpp src/stellar_cinematic_director_v059.cpp src/stellar_cinematic_director_v059.h src/stellar_camera_v054.h
	$(CC) $(CFLAGS) -Isrc src/stellar_cinematic_director_v059.cpp src/stellar_camera_director_v059.cpp -o $@

test_stellar_cinematic_director_v059: tests/test_stellar_cinematic_director_v059.cpp src/stellar_cinematic_director_v059.cpp src/stellar_cinematic_director_v059.h src/stellar_camera_path_v055.cpp src/stellar_camera_path_v055.h src/stellar_camera_v054.h
	$(CC) $(CFLAGS) -Isrc src/stellar_cinematic_director_v059.cpp src/stellar_camera_path_v055.cpp tests/test_stellar_cinematic_director_v059.cpp -o $@

stellar_camera_director_v060: src/stellar_camera_director_v060.cpp src/stellar_cinematic_director_v060.cpp src/stellar_cinematic_director_v060.h src/stellar_camera_v054.h
	$(CC) $(CFLAGS) -Isrc src/stellar_cinematic_director_v060.cpp src/stellar_camera_director_v060.cpp -o $@

test_stellar_cinematic_director_v060: tests/test_stellar_cinematic_director_v060.cpp src/stellar_cinematic_director_v060.cpp src/stellar_cinematic_director_v060.h src/stellar_camera_path_v055.cpp src/stellar_camera_path_v055.h src/stellar_camera_v054.h
	$(CC) $(CFLAGS) -Isrc src/stellar_cinematic_director_v060.cpp src/stellar_camera_path_v055.cpp tests/test_stellar_cinematic_director_v060.cpp -o $@

stellar_camera_director_v061: src/stellar_camera_director_v061.cpp src/stellar_cinematic_director_v061.cpp src/stellar_cinematic_director_v061.h src/stellar_camera_v054.h
	$(CC) $(CFLAGS) -Isrc src/stellar_cinematic_director_v061.cpp src/stellar_camera_director_v061.cpp -o $@

test_stellar_cinematic_director_v061: tests/test_stellar_cinematic_director_v061.cpp src/stellar_cinematic_director_v061.cpp src/stellar_cinematic_director_v061.h src/stellar_camera_path_v055.cpp src/stellar_camera_path_v055.h src/stellar_camera_v054.h
	$(CC) $(CFLAGS) -Isrc src/stellar_cinematic_director_v061.cpp src/stellar_camera_path_v055.cpp tests/test_stellar_cinematic_director_v061.cpp -o $@

stellar_camera_director_v062: src/stellar_camera_director_v062.cpp src/stellar_cinematic_director_v062.cpp src/stellar_cinematic_director_v062.h src/stellar_camera_v054.h
	$(CC) $(CFLAGS) -Isrc src/stellar_cinematic_director_v062.cpp src/stellar_camera_director_v062.cpp -o $@

test_stellar_cinematic_director_v062: tests/test_stellar_cinematic_director_v062.cpp src/stellar_cinematic_director_v062.cpp src/stellar_cinematic_director_v062.h src/stellar_camera_path_v055.cpp src/stellar_camera_path_v055.h src/stellar_camera_v054.h
	$(CC) $(CFLAGS) -Isrc src/stellar_cinematic_director_v062.cpp src/stellar_camera_path_v055.cpp tests/test_stellar_cinematic_director_v062.cpp -o $@

stellar_camera_director_v063: src/stellar_camera_director_v063.cpp src/stellar_cinematic_director_v063.cpp src/stellar_cinematic_director_v063.h src/stellar_camera_v054.h
	$(CC) $(CFLAGS) -Isrc src/stellar_cinematic_director_v063.cpp src/stellar_camera_director_v063.cpp -o $@

test_stellar_cinematic_director_v063: tests/test_stellar_cinematic_director_v063.cpp src/stellar_cinematic_director_v063.cpp src/stellar_cinematic_director_v063.h src/stellar_camera_path_v055.cpp src/stellar_camera_path_v055.h src/stellar_camera_v054.h
	$(CC) $(CFLAGS) -Isrc src/stellar_cinematic_director_v063.cpp src/stellar_camera_path_v055.cpp tests/test_stellar_cinematic_director_v063.cpp -o $@

stellar_camera_director_v069: src/stellar_camera_director_v069.cpp src/stellar_cinematic_director_v069.cpp src/stellar_cinematic_director_v069.h src/stellar_camera_v054.h
	$(CC) $(CFLAGS) -Isrc src/stellar_cinematic_director_v069.cpp src/stellar_camera_director_v069.cpp -o $@

test_stellar_cinematic_director_v069: tests/test_stellar_cinematic_director_v069.cpp src/stellar_cinematic_director_v069.cpp src/stellar_cinematic_director_v069.h src/stellar_camera_path_v055.cpp src/stellar_camera_path_v055.h src/stellar_camera_v054.h
	$(CC) $(CFLAGS) -Isrc src/stellar_cinematic_director_v069.cpp src/stellar_camera_path_v055.cpp tests/test_stellar_cinematic_director_v069.cpp -o $@

test_stellar_palette_v057: tests/test_stellar_palette_v057.cpp src/stellar_render_model_v052a.h src/stellar_palette_v057.h
	$(CC) $(CFLAGS) -Isrc tests/test_stellar_palette_v057.cpp -o $@

test_stellar_palette_v058: tests/test_stellar_palette_v058.cpp src/stellar_render_model_v052a.h src/stellar_palette_v057.h
	$(CC) $(CFLAGS) -Isrc tests/test_stellar_palette_v058.cpp -o $@

test_stellar_optical_profile_v068: tests/test_stellar_optical_profile_v068.cpp src/stellar_render_model_v052a.h src/stellar_feature_profile_v065.h src/stellar_palette_v057.h
	$(CC) $(CFLAGS) -Isrc tests/test_stellar_optical_profile_v068.cpp -o $@

test_stellar_gpu_ray_status_v053b: tests/test_stellar_gpu_ray_status_v053b.cpp src/stellar_gpu_ray_status_v053b.h
	$(CC) $(CFLAGS) -Isrc tests/test_stellar_gpu_ray_status_v053b.cpp -o $@

test_stellar_gpu_output_parity_v053b: tests/test_stellar_gpu_output_parity_v053b.cpp src/stellar_display_encoding_v053b.h
	$(CC) $(CFLAGS) -Isrc tests/test_stellar_gpu_output_parity_v053b.cpp -o $@

test_stellar_gpu_geometry_v053b: tests/test_stellar_gpu_geometry_v053b.cpp src/stellar_gpu_geometry_v053b.h
	$(CC) $(CFLAGS) -Isrc tests/test_stellar_gpu_geometry_v053b.cpp -o $@

test_stellar_gpu_profile_contract_v053c: tests/test_stellar_gpu_profile_contract_v053c.cpp src/stellar_gpu_profile_contract_v053c.h src/stellar_render_model_v052a.h src/stellar_feature_profile_v065.h src/stellar_palette_v057.h
	$(CC) $(CFLAGS) -Isrc tests/test_stellar_gpu_profile_contract_v053c.cpp -o $@

stellar_feature_probe_v064: src/stellar_feature_probe_v064.cpp src/stellar_feature_diagnostics_v064.cpp src/stellar_feature_diagnostics_v064.h src/stellar_feature_framing_v067.cpp src/stellar_feature_framing_v067.h src/stellar_feature_landmarks_v066.cpp src/stellar_feature_landmarks_v066.h src/stellar_gpu_scene_format_v052.h src/stellar_render_model_v052a.h src/stellar_feature_profile_v065.h src/stellar_palette_v057.h
	$(CC) $(CFLAGS) -Isrc src/stellar_feature_diagnostics_v064.cpp src/stellar_feature_framing_v067.cpp src/stellar_feature_landmarks_v066.cpp src/stellar_feature_probe_v064.cpp -o $@

test_stellar_feature_diagnostics_v064: tests/test_stellar_feature_diagnostics_v064.cpp src/stellar_feature_diagnostics_v064.cpp src/stellar_feature_diagnostics_v064.h src/stellar_feature_landmarks_v066.cpp src/stellar_feature_landmarks_v066.h src/stellar_gpu_scene_format_v052.h src/stellar_render_model_v052a.h src/stellar_feature_profile_v065.h src/stellar_palette_v057.h
	$(CC) $(CFLAGS) -Isrc src/stellar_feature_diagnostics_v064.cpp src/stellar_feature_landmarks_v066.cpp tests/test_stellar_feature_diagnostics_v064.cpp -o $@

test_stellar_feature_landmarks_v066: tests/test_stellar_feature_landmarks_v066.cpp src/stellar_feature_landmarks_v066.cpp src/stellar_feature_landmarks_v066.h src/stellar_feature_diagnostics_v064.h src/stellar_render_model_v052a.h src/stellar_feature_profile_v065.h src/stellar_palette_v057.h
	$(CC) $(CFLAGS) -Isrc src/stellar_feature_landmarks_v066.cpp tests/test_stellar_feature_landmarks_v066.cpp -o $@

test_stellar_feature_framing_v067: tests/test_stellar_feature_framing_v067.cpp src/stellar_feature_framing_v067.cpp src/stellar_feature_framing_v067.h src/stellar_feature_diagnostics_v064.h src/stellar_render_model_v052a.h src/stellar_feature_profile_v065.h src/stellar_palette_v057.h
	$(CC) $(CFLAGS) -Isrc src/stellar_feature_framing_v067.cpp tests/test_stellar_feature_framing_v067.cpp -o $@

test_stellar_feature_profile_v065: tests/test_stellar_feature_profile_v065.cpp src/stellar_render_model_v052a.h src/stellar_feature_profile_v065.h src/stellar_palette_v057.h
	$(CC) $(CFLAGS) -Isrc tests/test_stellar_feature_profile_v065.cpp -o $@

libarepo.a:
	+$(MAKE) -C arepo libarepo.a

$(OBJS): | libarepo.a

clean:
	+$(MAKE) -C arepo clean
	rm -f $(OBJS) $(EXECNAME) stellar_camera_director_v056 stellar_camera_director_v059 stellar_camera_director_v060 stellar_camera_director_v061 stellar_camera_director_v062 stellar_camera_director_v063 stellar_camera_director_v069 stellar_feature_probe_v064 test_stellar_cinematic_director_v059 test_stellar_cinematic_director_v060 test_stellar_cinematic_director_v061 test_stellar_cinematic_director_v062 test_stellar_cinematic_director_v063 test_stellar_cinematic_director_v069 test_stellar_palette_v057 test_stellar_palette_v058 test_stellar_optical_profile_v068 test_stellar_gpu_ray_status_v053b test_stellar_gpu_output_parity_v053b test_stellar_gpu_geometry_v053b test_stellar_gpu_profile_contract_v053c test_stellar_feature_diagnostics_v064 test_stellar_feature_profile_v065 test_stellar_feature_landmarks_v066 test_stellar_feature_framing_v067 $(MISC_RM)

build/%.o: src/%.cpp
	$(CC) $(CFLAGS) $(OPT) -c $< -o $@
