#!/bin/bash
#script para execução do projeto

# Caminho do diretório do NS-3
NS3_DIR=~/ns-allinone-3.44/ns-3.44

# Nome base do script (sem .cc)
SCRIPT_NAME=manet_video_compare._v3

# Caminho do executável
EXEC="$NS3_DIR/build/scratch/ns3.44-${SCRIPT_NAME}-default"

# Parâmetros com valores padrão
PROTO=${1:-AODV}
NODES=${2:-10}
FLOWS=${3:-2}
SIM_TIME=${4:-30}
FAIL_NODE=${5:-5}
FAIL_TIME=${6:-15}

# Entrar no diretório do NS-3
cd "$NS3_DIR"

# Verificar se o executável existe
if [ ! -f "$EXEC" ]; then
  echo "Erro: Executável $EXEC não encontrado!"
  echo "Compile com: cmake --build build -j$(nproc)"
  exit 1
fi

# Executar simulação
echo "   Rodando simulação:"
echo "   Protocolo: $PROTO"
echo "   Nós: $NODES"
echo "   Fluxos: $FLOWS"
echo "   Duração: $SIM_TIME s"
echo "   Falha no nó $FAIL_NODE aos $FAIL_TIME s"
$EXEC --protocol=$PROTO --nNodes=$NODES --nFlows=$FLOWS --simTime=$SIM_TIME --failNode=$FAIL_NODE --failTime=$FAIL_TIME

# Arquivos esperados
FLOWMON_FILE="${PROTO}_flowmon.xml"
ANIM_FILE="manet_anim.xml"

# Verificar saída do FlowMonitor
if [ -f "$FLOWMON_FILE" ]; then
  echo "Resultado do FlowMonitor salvo em: $FLOWMON_FILE"
else
  echo "FlowMonitor não gerou $FLOWMON_FILE"
fi

# Abrir visualização no NetAnim, se disponível
#  echo "Abrindo o NetAnim..."
#    cd /home/username_regex/ns-allinone-3.44/netanim-3.109/build/bin
#    ./netanim
