#!/bin/bash
build=${build:-debug}
VERBOSE=${VERBOSE:-0}

cd $(dirname $0)
CLEAN=0
echo "$(pwd)/build.sh"
while [[ $@ > 0 ]]; do
case $1 in
--clean)
    echo "rm $(pwd)/build/$build"
    rm -rf $(pwd)/build/$build
    shift
esac
shift
done
ROQ_CONDA=${ROQ_CONDA:-$CONDA_PREFIX}
ROQ_ARCH=${ROQ_ARCH:-"Linux-x86_64"}
echo "ROQ_CONDA=$ROQ_CONDA"
if [ ! -d $ROQ_CONDA ]; then
echo "please install roq first"
fi
# note: for M1 osx ROQ_ARCH="MacOSX-arm64"
. $ROQ_CONDA/bin/activate
CXXFLAGS="-isystem $ROQ_CONDA/include"
if [[ "$ROQ_ARCH" =~ MacOSX.* ]]; then
CXX=${CXX:-$(which clang++)}
CC=${CC:-$(which clang)}
CXXFLAGS="$CXXFLAGS -mmacosx-version-min=12.6 -mmacos-version-min=12.6"
CXXFLAGS="$CXXFLAGS -D_LIBCPP_DISABLE_AVAILABILITY"
#CXXFLAGS="$CXXFLAGS -nostdinc++ -nostdlib++"
# https://github.com/catchorg/Catch2/issues/2779
else
CXX=${CXX:-$(which g++)}
CC=${CC:-$(which gcc)}
fi

#rm -rf build/$build
echo "running cmake... CXX=$CXX CC=$CC"
cmake -H. -Bbuild/$build -DCMAKE_INSTALL_PREFIX=$ROQ_CONDA\
	-DCMAKE_CXX_COMPILER=$CXX -DCMAKE_BUILD_TYPE=$build\
	-DCMAKE_EXPORT_COMPILE_COMMANDS=YES\
	-DCMAKE_CXX_FLAGS_DEBUG="-O0 -g $CXXFLAGS"\
        -DCMAKE_CXX_FLAGS="$CXXFLAGS"\
        -DCMAKE_C_FLAGS="$CXXFLAGS"\
        -DCMAKE_C_FLAGS_DEBUG="-O0 -g"\
	-DUSE_LQS=ON\
		|| exit 1

echo "running make..."
cd build/$build && make install -s -j8 VERBOSE=$VERBOSE || exit 2
touch compile_commands.json
cat compile_commands.json > ../../../compile_commands.json && sed -i -e ':a;N;$!ba;s/\]\n\n\[/,/g' ../../../compile_commands.json
echo "built roq-apps"
