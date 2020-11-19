rm -Rf build
mkdir build && cd build
git clone https://github.com/pnikiel/PrepareEcosystem.git
bash PrepareEcosystem/prepare.sh --with_LogIt --with_o6compat --tag_o6compat v1.3.5
cmake ../
make
