#!/bin/bash

# Caminhos
NS3_DIR=~/ns-allinone-3.44/ns-3.44/
NETANIM_DIR=~/ns-allinone-3.44/netanim-3.109

echo "Atualizando dependências..."
sudo apt update
sudo apt install -y g++ cmake qt5-qmake qtbase5-dev libsqlite3-dev qttools5-dev qttools5-dev-tools

echo "Entrando na pasta do NS-3..."
cd "$NS3_DIR"

echo "Limpando builds anteriores..."
rm -rf build
./ns3 clean

echo "Configurando CMake com os módulos utilizados no código..."
cmake -S . -B build \
  -DNS3_ENABLE_EXAMPLES=OFF \
  -DNS3_ENABLE_TESTS=OFF \
  -DNS3_MODULES_TO_ENABLE="core;network;internet;mobility;wifi;olsr;aodv;applications;flow-monitor;netanim"

echo "Compilando apenas os módulos necessários..."
cmake --build build -j$(nproc)

echo "Compilando NetAnim (versão 3.109)..."
cd "$NETANIM_DIR"

# Verifica se a pasta build já existe
if [ ! -d build ]; then
  mkdir build
fi

cd build
cmake ..
make -j$(nproc)

echo "Build otimizado concluído!"
