rm -Rf build
mkdir build && cd build
git clone https://github.com/pnikiel/PrepareEcosystem.git
bash PrepareEcosystem/prepare.sh --with_LogIt --with_o6compat --tag_o6compat OPCUA-2079_support_full_nodeclasses 
cmake ../
make
