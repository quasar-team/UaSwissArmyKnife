rm -Rf build
mkdir build && cd build
git clone ssh://git@gitlab.cern.ch:7999/atlas-dcs-opcua-servers/PrepareEcosystem.git
sh PrepareEcosystem/prepare.sh --with_LogIt --with_o6compat
cmake ../
make
