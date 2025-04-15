#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/aodv-helper.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/ipv4-flow-classifier.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ManetVideoCompare");

int main(int argc, char *argv[])
{


  // Parâmetros configuráveis com valores padrão
  std::string protocol = "AODV";
  uint32_t nNodes = 10;
  double simTime = 240.0;
  uint32_t nFlows = 1;
  double errorRate = 0.01;
  double txPower = 20.0; // dBm
  double maxSpeed = 5.0; // m/s
  double pktPerSec = 20.0; // pacotes/segundo
  double failTime = 25.0; // segundos
  uint32_t failNode = 1; // nó que falhará
  bool enableCsv = true; // exportar CSV

  CommandLine cmd;
  cmd.AddValue("protocol", "Protocolo de roteamento: AODV ou OLSR", protocol);
  cmd.AddValue("nNodes", "Número de nós", nNodes);
  cmd.AddValue("simTime", "Tempo de simulação (segundos)", simTime);
  cmd.AddValue("nFlows", "Número de fluxos de vídeo (pares cliente-servidor)", nFlows);
  cmd.AddValue("errorRate", "Taxa de erro do canal (0-1)", errorRate);
  cmd.AddValue("txPower", "Potência de transmissão WiFi (dBm)", txPower);
  cmd.AddValue("maxSpeed", "Velocidade máxima de mobilidade (m/s)", maxSpeed);
  cmd.AddValue("pktPerSec", "Intensidade do tráfego (pacotes/seg)", pktPerSec);
  cmd.AddValue("failTime", "Tempo para simular falha (segundos)", failTime);
  cmd.AddValue("failNode", "Nó que falhará", failNode);
  cmd.AddValue("enableCsv", "Exportar métricas para CSV (0/1)", enableCsv);
  cmd.Parse(argc, argv);

  // Validação de parâmetros
  if (nNodes < nFlows * 2) {
    NS_FATAL_ERROR("Número de nós deve ser pelo menos o dobro do número de fluxos.");
  }
  if (errorRate < 0 || errorRate > 1) {
    NS_FATAL_ERROR("Taxa de erro deve estar entre 0 e 1");
  }
  if (failNode >= nNodes) {
    NS_FATAL_ERROR("Nó de falha deve ser menor que o número total de nós");
  }


  // Normalizar protocolo para maiúsculas
  std::transform(protocol.begin(), protocol.end(), protocol.begin(), ::toupper);

  // Criar nós
  NodeContainer nodes;
  nodes.Create(nNodes);

  // Configurar mobilidade
  MobilityHelper mobility;
  Ptr<PositionAllocator> positionAlloc = CreateObjectWithAttributes<GridPositionAllocator>(
      "MinX", DoubleValue(0.0),
      "MinY", DoubleValue(0.0),
      "DeltaX", DoubleValue(20.0),
      "DeltaY", DoubleValue(20.0),
      "GridWidth", UintegerValue(5),
      "LayoutType", StringValue("RowFirst"));

  mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
                          "Speed", StringValue("ns3::UniformRandomVariable[Min=1.0|Max=" + std::to_string(maxSpeed) + "]"),
                          "Pause", StringValue("ns3::ConstantRandomVariable[Constant=2.0]"),
                          "PositionAllocator", PointerValue(positionAlloc));
  mobility.Install(nodes);

  // Configurar WiFi
  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211b);
  wifi.SetRemoteStationManager("ns3::AarfWifiManager");

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
  YansWifiPhyHelper phy;
  phy.SetChannel(channel.Create());
  phy.Set("TxPowerStart", DoubleValue(txPower));
  phy.Set("TxPowerEnd", DoubleValue(txPower));

  // Configurar erro de recepção
  Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
  em->SetAttribute("ErrorRate", DoubleValue(errorRate));
  phy.SetErrorRateModel("ns3::NistErrorRateModel");

  WifiMacHelper mac;
  mac.SetType("ns3::AdhocWifiMac");

  NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

  // Instalar pilha de Internet e roteamento
  InternetStackHelper stack;
  if (protocol == "AODV") {
    AodvHelper aodv;
    stack.SetRoutingHelper(aodv);
  } else if (protocol == "OLSR") {
    OlsrHelper olsr;
    stack.SetRoutingHelper(olsr);
  } else {
    NS_FATAL_ERROR("Protocolo inválido. Use AODV ou OLSR.");
  }
  
  try {
    stack.Install(nodes);
  } catch (const std::exception &e) {
    NS_FATAL_ERROR("Falha na instalação da pilha de rede: " << e.what());
  }

  Ipv4AddressHelper address;
  address.SetBase("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign(devices);

  // Configurar aplicações (modelo VBR para vídeo)
  for (uint32_t i = 0; i < nFlows; ++i) {
    uint32_t sender = i * 2;
    uint32_t receiver = i * 2 + 1;
    
    if (sender >= nNodes || receiver >= nNodes) {
      NS_FATAL_ERROR("Configuração de fluxo inválida: remetente ou receptor além do número de nós");
    }
    
    uint16_t port = 9000 + i;

    // Servidor UDP
    UdpServerHelper server(port);
    ApplicationContainer serverApp = server.Install(nodes.Get(receiver));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(simTime));

    // Cliente OnOff (VBR)
    OnOffHelper onoff("ns3::UdpSocketFactory", 
                     InetSocketAddress(interfaces.GetAddress(receiver), port));
    onoff.SetAttribute("OnTime", StringValue("ns3::ExponentialRandomVariable[Mean=1.0]"));
    onoff.SetAttribute("OffTime", StringValue("ns3::ExponentialRandomVariable[Mean=0.5]"));
    onoff.SetAttribute("DataRate", DataRateValue(DataRate("500kbps")));
    onoff.SetAttribute("PacketSize", UintegerValue(1024));
    
    ApplicationContainer clientApp = onoff.Install(nodes.Get(sender));
    clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(simTime));
  }

  // Configurar monitoramento
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  // Configurar animação
  AnimationInterface anim("manet_anim.xml");
  anim.EnablePacketMetadata(true);
  anim.EnableIpv4RouteTracking("routingtable.xml", Seconds(0), Seconds(5), Seconds(0.5));
  
  for (uint32_t i = 0; i < nodes.GetN(); ++i) {
    anim.UpdateNodeDescription(i, "Dispositivo" + std::to_string(i));
    anim.UpdateNodeColor(i, 128, 128, 128); // Cinza padrão
    
    for (uint32_t j = 0; j < nFlows; ++j) {
      if (i == j * 2)
        anim.UpdateNodeColor(i, 0, 255, 0); // Cliente: verde
      else if (i == j * 2 + 1)
        anim.UpdateNodeColor(i, 255, 0, 0); // Servidor: vermelho
    }
  }

  // Configurar evento de falha e recuperação
  Simulator::Schedule(Seconds(failTime), [failNode, failTime, nodes]() {
    NS_LOG_UNCOND("Falha simulada: Desligando a interface WiFi do nó " << failNode << " aos " << failTime << "s.");
    Ptr<Node> node = NodeList::GetNode(failNode);
    
    for (uint32_t i = 0; i < node->GetNDevices(); ++i) {
      Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice>(node->GetDevice(i));
      if (wifiDev && wifiDev->GetPhy()) {
        wifiDev->GetPhy()->SetSleepMode(true);
      }
    }
  });

  Simulator::Schedule(Seconds(failTime + 5.0), [failNode]() {
    NS_LOG_UNCOND("Recuperação simulada: Reativando a interface WiFi do nó " << failNode);
    Ptr<Node> node = NodeList::GetNode(failNode);
    
    for (uint32_t i = 0; i < node->GetNDevices(); ++i) {
      Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice>(node->GetDevice(i));
      if (wifiDev && wifiDev->GetPhy()) {
        wifiDev->GetPhy()->SetSleepMode(false);
      }
    }
  });

  // Executar simulação
  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  // Analisar resultados
  monitor->CheckForLostPackets();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

  // Variáveis para estatísticas agregadas
  double totalThroughput = 0.0;
  double totalPacketLoss = 0.0;
  double totalDelay = 0.0;
  double totalJitter = 0.0;
  uint32_t validFlows = 0;

  // Arquivo CSV para exportação
  std::ofstream csvFile;
  if (enableCsv) {
    csvFile.open(protocol + "_metrics.csv");
    csvFile << "FlowID,Source,Destination,Throughput(Kbps),PacketLoss(%),Delay(ms),Jitter(ms),Efficiency(%)\n";
  }

  std::cout << "\n========== MÉTRICAS COLETADAS ==========\n";
  for (auto &flow : stats) {
    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);
    
    std::cout << "Fluxo " << flow.first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
    std::cout << "  Pacotes transmitidos: " << flow.second.txPackets << "\n";
    std::cout << "  Pacotes recebidos:    " << flow.second.rxPackets << "\n";
    
    uint32_t lost = flow.second.txPackets - flow.second.rxPackets;
    double lossPercent = (flow.second.txPackets > 0) ? (lost * 100.0 / flow.second.txPackets) : 0;
    std::cout << "  Perda de pacotes:     " << lost << " (" << lossPercent << "%)\n";

    if (flow.second.rxPackets > 0) {
      double throughput = (flow.second.rxBytes * 8.0) / 
                         (flow.second.timeLastRxPacket.GetSeconds() - flow.second.timeFirstTxPacket.GetSeconds());
      double throughputKbps = throughput / 1024;
      double avgDelay = (flow.second.delaySum.GetSeconds() / flow.second.rxPackets) * 1000;
      double avgJitter = (flow.second.rxPackets > 1) ? 
                        (flow.second.jitterSum.GetSeconds() / (flow.second.rxPackets - 1)) * 1000 : 0;
      double efficiency = (flow.second.rxBytes * 100.0 / flow.second.txBytes);

      std::cout << "  Vazão (throughput):   " << throughputKbps << " Kbps\n";
      std::cout << "  Atraso médio:         " << avgDelay << " ms\n";
      if (flow.second.rxPackets > 1) {
        std::cout << "  Jitter médio:         " << avgJitter << " ms\n";
      }
      std::cout << "  Eficiência:           " << efficiency << "%\n";

      // Acumular para estatísticas agregadas
      totalThroughput += throughputKbps;
      totalPacketLoss += lossPercent;
      totalDelay += avgDelay;
      totalJitter += avgJitter;
      validFlows++;

      // CSV
      if (enableCsv) {
        csvFile << flow.first << "," << t.sourceAddress << "," << t.destinationAddress << ","
                << throughputKbps << "," << lossPercent << "," << avgDelay << "," 
                << avgJitter << "," << efficiency << "\n";
      }
    }
    std::cout << std::endl;
  }

  // Calcular e mostrar estatísticas agregadas
  if (validFlows > 0) {
    std::cout << "\n========== ESTATÍSTICAS GERAIS ==========\n";
    std::cout << "Throughput médio: " << (totalThroughput / validFlows) << " Kbps\n";
    std::cout << "Perda de pacotes média: " << (totalPacketLoss / validFlows) << "%\n";
    std::cout << "Atraso médio: " << (totalDelay / validFlows) << " ms\n";
    std::cout << "Jitter médio: " << (totalJitter / validFlows) << " ms\n";
  }

  // Salvar relatório do FlowMonitor
  std::string fileName = protocol + "_flowmon.xml";
  monitor->SerializeToXmlFile(fileName, true, true);

  if (enableCsv) {
    csvFile.close();
    std::cout << "\nMétricas exportadas para: " << protocol << "_metrics.csv\n";
  }

  Simulator::Destroy();
  return 0;
}