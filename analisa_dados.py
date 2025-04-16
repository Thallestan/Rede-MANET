import os
import xml.etree.ElementTree as ET
import pandas as pd
import matplotlib.pyplot as plt

def processar_flowmon(xml_path, pasta_saida):
    tree = ET.parse(xml_path)
    root = tree.getroot()

    flow_data = []
    delay_hist = []
    jitter_hist = []
    size_hist = []

    for flow in root.findall(".//FlowStats/Flow"):
        flow_id = int(flow.get('flowId'))
        tx_bytes = int(flow.get('txBytes'))
        rx_bytes = int(flow.get('rxBytes'))
        tx_packets = int(flow.get('txPackets'))
        rx_packets = int(flow.get('rxPackets'))
        lost_packets = int(flow.get('lostPackets'))
        delay_sum = float(flow.get('delaySum').replace('+', '').replace('ns', '')) / 1e6
        jitter_sum = float(flow.get('jitterSum').replace('+', '').replace('ns', '')) / 1e6

        flow_data.append({
            'flowId': flow_id,
            'txBytes': tx_bytes,
            'rxBytes': rx_bytes,
            'txPackets': tx_packets,
            'rxPackets': rx_packets,
            'lostPackets': lost_packets,
            'delaySum_ms': delay_sum,
            'jitterSum_ms': jitter_sum
        })

        for bin in flow.find('.//delayHistogram').findall('bin'):
            delay_hist.append({
                'flowId': flow_id,
                'delayStart': float(bin.get('start')),
                'delayWidth': float(bin.get('width')),
                'count': int(bin.get('count'))
            })

        for bin in flow.find('.//jitterHistogram').findall('bin'):
            jitter_hist.append({
                'flowId': flow_id,
                'jitterStart': float(bin.get('start')),
                'jitterWidth': float(bin.get('width')),
                'count': int(bin.get('count'))
            })

        for bin in flow.find('.//packetSizeHistogram').findall('bin'):
            size_hist.append({
                'flowId': flow_id,
                'packetSizeStart': float(bin.get('start')),
                'packetSizeWidth': float(bin.get('width')),
                'count': int(bin.get('count'))
            })

    # Convertendo para DataFrames
    df_flows = pd.DataFrame(flow_data)
    df_delay = pd.DataFrame(delay_hist)
    df_jitter = pd.DataFrame(jitter_hist)
    df_size = pd.DataFrame(size_hist)

    # Cria pasta de saÃ­da, se necessÃ¡rio
    os.makedirs(pasta_saida, exist_ok=True)

    # Salvando Excel
    excel_path = os.path.join(pasta_saida, "flowmon_resultados.xlsx")
    with pd.ExcelWriter(excel_path, engine='openpyxl') as writer:
        df_flows.to_excel(writer, sheet_name="Resumo_Fluxos", index=False)
        df_delay.to_excel(writer, sheet_name="Delay_Hist", index=False)
        df_jitter.to_excel(writer, sheet_name="Jitter_Hist", index=False)
        df_size.to_excel(writer, sheet_name="Size_Hist", index=False)

    # GrÃ¡ficos
    plt.figure(figsize=(8, 4))
    plt.bar(df_delay['delayStart'], df_delay['count'], width=df_delay['delayWidth'])
    plt.title('Histograma de Delay')
    plt.xlabel('Delay (s)')
    plt.ylabel('NÃºmero de Pacotes')
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(os.path.join(pasta_saida, "delay_histograma.png"))

    plt.figure(figsize=(8, 4))
    plt.bar(df_jitter['jitterStart'], df_jitter['count'], width=df_jitter['jitterWidth'], color='orange')
    plt.title('Histograma de Jitter')
    plt.xlabel('Jitter (s)')
    plt.ylabel('NÃºmero de Pacotes')
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(os.path.join(pasta_saida, "jitter_histograma.png"))

    plt.figure(figsize=(8, 4))
    plt.bar(df_size['packetSizeStart'], df_size['count'], width=df_size['packetSizeWidth'], color='green')
    plt.title('Histograma de Tamanho de Pacotes')
    plt.xlabel('Tamanho do Pacote (bytes)')
    plt.ylabel('NÃºmero de Pacotes')
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(os.path.join(pasta_saida, "packet_size_histograma.png"))

    # GrÃ¡fico: Pacotes Transmitidos vs Recebidos
    plt.figure(figsize=(8, 4))
    x = df_flows['flowId'].astype(str)
    plt.bar(x, df_flows['txPackets'], label='Transmitidos', alpha=0.7)
    plt.bar(x, df_flows['rxPackets'], label='Recebidos', alpha=0.7)
    plt.title('Pacotes Transmitidos vs Recebidos por Fluxo')
    plt.xlabel('ID do Fluxo')
    plt.ylabel('NÃºmero de Pacotes')
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(os.path.join(pasta_saida, "tx_rx_packets.png"))

    # GrÃ¡fico: Taxa de Perda de Pacotes
    plt.figure(figsize=(8, 4))
    df_flows['taxa_perda'] = df_flows['lostPackets'] / (df_flows['txPackets'] + df_flows['lostPackets'])
    plt.bar(df_flows['flowId'].astype(str), df_flows['taxa_perda'] * 100, color='red')
    plt.title('Taxa de Perda de Pacotes por Fluxo (%)')
    plt.xlabel('ID do Fluxo')
    plt.ylabel('Taxa de Perda (%)')
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(os.path.join(pasta_saida, "taxa_perda_fluxo.png"))

    # GrÃ¡fico: Delay MÃ©dio vs Jitter MÃ©dio
    plt.figure(figsize=(8, 4))
    df_flows['delay_medio'] = df_flows['delaySum_ms'] / df_flows['rxPackets']
    df_flows['jitter_medio'] = df_flows['jitterSum_ms'] / (df_flows['rxPackets'] - 1)
    plt.plot(df_flows['flowId'], df_flows['delay_medio'], marker='o', label='Delay MÃ©dio (ms)')
    plt.plot(df_flows['flowId'], df_flows['jitter_medio'], marker='s', label='Jitter MÃ©dio (ms)')
    plt.title('Delay e Jitter MÃ©dios por Fluxo')
    plt.xlabel('ID do Fluxo')
    plt.ylabel('Tempo (ms)')
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(os.path.join(pasta_saida, "delay_jitter_medio.png"))

    print(f"AnÃ¡lise finalizada e salva em: {pasta_saida}")

# Caminho base
base_dir = "simulacoes"
protocolos = ['AODV', 'OLSR']

for cenario in os.listdir(base_dir):
    cenario_path = os.path.join(base_dir, cenario)
    if not os.path.isdir(cenario_path):
        continue
    for protocolo in protocolos:
        protocolo_path = os.path.join(cenario_path, protocolo)
        xml_files = [f for f in os.listdir(protocolo_path) if f.endswith('.xml')]
        if xml_files:
            xml_path = os.path.join(protocolo_path, xml_files[0])
            pasta_saida = os.path.join("analises", cenario, protocolo)
            processar_flowmon(xml_path, pasta_saida)