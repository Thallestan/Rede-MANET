# Rede-MANET
Projeto da disciplina de redes de computadores, simulação no NS-3 de uma rede do tipo MANET para streaming de vídeo;


*Simulation Parameters*
-Simulator: NS-3
-Network Type:	Rede MANET (Ad-hoc)
-Routing Protocols:	AODV ou OLSR (configurável)
-Area:	Grid 5x5 (MinX=0, MinY=0, DeltaX=20, DeltaY=20)
-Fluxos: 5
-Number of Nodes:	10, 20, 30, 40 
-Duration:	240 segundos
-Mobility Model:	Random Waypoint Mobility Model
-Pause Time:	2.0 segundos (constante)
-Mobility:	5.0 m/s
-Traffic Type:	Tráfego UDP VBR (Exponential On/Off)
-Data Packet Size:	1024 bytes
-Data rate:	500 kbps
-Address Mode:	IPv4 (base: 10.0.0.0/24)
-Physical Characteristics:	WiFi 802.11b (TxPower: configurável, padrão: 20 dBm)
-Fragmentation threshold:	2346 bytes
-Buffer size:	1500 packets (por interface)


Desenvolvido por Thalles Stanziola
