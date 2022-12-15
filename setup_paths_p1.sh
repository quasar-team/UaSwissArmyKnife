# This script must be sourced and NOT executed. 
# For instance do:
# 
#   source setup_paths_p1.sh

# Description:
#
# This script is coming out of the SCA-SW dependencies from a given LCG release
# and for a specific platform
#
# Authors: Paris

### To move to a different LCG edit the following variables accordingly
LCG_VERSION="LCG_100"
PLATFORM="centos7"
COMPILER="gcc8"
GCC_VERSION="8.3.0"
CMAKE_VERSION="3.20.0"
BOOST_VERSION="1.75.0"
PROTOBUF_VERSION="2.5.0"
###

PATH_TO_LCG="/sw/atlas/sw/lcg"
X_BINARY_TAG="x86_64-${PLATFORM}-${COMPILER}-opt"

usage()
{
	echo
	echo "Usage"
	echo
	echo "[-h]: help"
	echo
}

epilogue()
{
	gcc --version
	cmake --version
	echo "Dependencies configured. Please also source the setup.sh from your FELIX installation directory."
}
	
grep -e "CentOS" /etc/redhat-release
rc=$?
if [ $rc == 0 ]
then
	echo "We're on CentOS..."
	if [ "$1" == "-h" ]
	then
		usage
	else
		echo "Using ${LCG_VERSION} and ${COMPILER}..."
		#  First the compiler
		if [ -e ${PATH_TO_LCG}/releases/gcc/${GCC_VERSION}/x86_64-centos7/setup.sh ]; then
    		source ${PATH_TO_LCG}/releases/gcc/${GCC_VERSION}/x86_64-centos7/setup.sh
		else
			echo "Error: Cannot access ${PATH_TO_LCG}/releases/gcc/${GCC_VERSION}/x86_64-centos7/setup.sh"
			exit 1
		fi
		#  Then CMake
		## One way is to have a soft link names cmake pointing to cmake3 in home bin and then
		## export PATH=~/bin/:$PATH
		#  Then Boost
		export BOOST_ROOT="${PATH_TO_LCG}/releases/${LCG_VERSION}/Boost/${BOOST_VERSION}/${X_BINARY_TAG}" 
		epilogue
	fi
fi
