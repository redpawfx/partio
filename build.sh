#!/bin/bash

build_partio_lib=1
build_partio_maya=1
build_partio_arnold=1

build_type=Release

#mayaVersions=(2015.sp6) #2014.sp4 do not build
#mayaVersions=(2016.sp6) #2014.sp4 do not build
mayaVersions=(ext2.2016.sp1)
#mayaVersions=(2017.0.0)


#####################################################################
#    for MAYA 2016:                                                 #
# rez env gxx-4.6 cmake swig-3.0.5 glew-2.0 python-2.7 mayaAPI-2016 #
#																	#
#####################################################################


arnold_version='4.2.16.2'
mtoa_version='1.4.2.1.16'
compiler_path='/usr/bin/g++-4.6'

swig_executable='/s/apps/packages/dev/swig/3.0.5/platform-linux/bin/swig'


glew_include_dir=$REZ_GLEW_ROOT'/include'
glew_static_library=$REZ_GLEW_ROOT'/lib64/libGLEW.a'

#glew_include_dir='/s/apps/lin/vfx_test_apps/glew/1.11.0/include'
#glew_static_library='/s/apps/lin/vfx_test_apps/glew/1.11.0/lib/libGLEW.a'




export ARNOLD_HOME=/s/apps/packages/cg/arnold/$arnold_version/platform-linux
export MTOA_ROOT=/s/apps/packages/mikros/mayaModules/mimtoa/$mtoa_version/platform-linux/arnold-4.2/maya-2016
export PARTIO_HOME=/datas/hda/build/partio/build-Linux-x86_64

rm -fr /datas/hda/build/partio/partio.build
rm -fr /datas/hda/build/partio/build-Linux-x86_64

mkdir partio.build
mkdir build-Linux-x86_64

cd partio.build

if [ "$build_partio_lib" == 1 ]; then

	echo "BUILD PARTIO LIB"

	cmake .. \
	  -DBUILD_PARTIO_LIBRARY=1 \
	  -DBUILD_PARTIO_MAYA=0 \
	  -DBUILD_PARTIO_MTOA=0

	make -j12
	make install
fi

### Build Partio Maya plugin
echo $build_partio_maya

if [ "$build_partio_maya" == 1 ]; then

	echo "BUILD PARTIO MAYA"

	for mv in "${mayaVersions[@]}"
	do
		maya_executable='/s/apps/packages/cg/maya/'$mv'/platform-linux/bin/maya'	
		
		cmake .. \
		-DCMAKE_BUILD_TYPE=$build_type \
		-DGLEW_INCLUDE_DIR=$glew_include_dir \
		-DGLEW_STATIC_LIBRARY=$glew_static_library \
		-DMAYA_EXECUTABLE=$maya_executable \
		-DBUILD_PARTIO_LIBRARY=1 \
		-DBUILD_PARTIO_MAYA=1 \
		-DBUILD_PARTIO_MTOA=0
		
		make -j12
		make install
	done	
fi

### Build Partio Arnold procedural
if [ "$build_partio_arnold" == 1 ]; then
cmake .. \
-DCMAKE_BUILD_TYPE=$build_type \
-DSWIG_EXECUTABLE=$swig_executable \
-DGLEW_INCLUDE_DIR=$glew_include_dir \
-DGLEW_STATIC_LIBRARY=$glew_static_library \
-DBUILD_PARTIO_LIBRARY=1 \
-DBUILD_PARTIO_MAYA=0 \
-DBUILD_PARTIO_MTOA=1

#-DCMAKE_CXX_COMPILER=$compiler_path \

make -j12
make install
fi

# cp /s/apps/users/hda/build/partio/build-Linux-x86_64/maya/2016/plug-ins/Linux-x86_64/partio4Maya.so /s/apps/users/hda/packages/cgDev/partioMaya/dev/platform-linux/maya-2016/plug-ins


