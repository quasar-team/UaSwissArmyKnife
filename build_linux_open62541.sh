rm -Rf build
mkdir build && cd build
git clone ssh://git@gitlab.cern.ch:7999/atlas-dcs-opcua-servers/PrepareEcosystem.git 
bash PrepareEcosystem/prepare.sh --with_LogIt --with_o6compat --tag_o6compat master 
cmake ../
make -j4
