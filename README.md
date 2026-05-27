https://www.notion.so/adalbertocarv/250520261545-Projeto-arduino-36b5265550c7807396f7ed9f0d67d7f2?source=copy_link#36c5265550c780f5ba54ca980eee7fdd
Estou desenvolvendo um sistema de telemetria integrada.
Funções:

- **Tacômetro Digital** (A parte que lê a bobina e mostra o RPM no leque).
- **Indicador Magnético de Marchas** (A placa do trambulador com os sensores Hall).
- **Medidor de Aceleração / Força G** (O sensor do eixo X).

Sugestões de nome:

- **GL-Dash:** Mistura a versão clássica do carro com a tecnologia moderna.
- **Telemetria AP:** Direto ao ponto, focado na mecânica lendária que está por baixo do capô.
- **RetroDash 90:** Um aceno ao ano de fabricação, mantendo o visual de fósforo/monocromático.
- **ShiftG-Tron:** Unindo o "Shift" (câmbio) com o "G" (Força G).

# 📄 Documentação Técnica: Sistema de Telemetria e Painel Digital Automotivo

## 1. Visão Geral do Projeto

O projeto consiste em um sistema embarcado de telemetria e exibição de dados automotivos, desenvolvido especificamente para operar no ecossistema elétrico de um motor AP (projetado e instalado em um VW Gol GL 1990).

O dispositivo unifica três funções principais em uma única interface gráfica monolítica:

- **Tacômetro Digital (Conta-giros):** Leitura em tempo real dos pulsos da bobina de ignição.
- **Indicador Magnético de Marchas:** Mapeamento da posição do trambulador via sensores de Efeito Hall.
- **Telemetria Dinâmica (Força G):** Medição de aceleração e frenagem no eixo longitudinal.

O design prioriza a **instalação não-invasiva**, garantindo a preservação estética e estrutural do veículo (padrão placa preta), utilizando fixações por dupla-face e derivações elétricas reversíveis.

## 2. Arquitetura de Hardware

O hardware é modular e fisicamente dividido em três zonas distintas para garantir a integridade dos sinais de dados e proteger o microcontrolador contra interferências eletromagnéticas (EMI) geradas pela alta tensão do motor.

### Módulo 1: Central Elétrica e Processamento (Abaixo do Painel)

O "cérebro" do sistema, enclausurado em uma caixa de isolamento (Patola ou PVC) para proteção mecânica e elétrica.

- **Microcontrolador:** Arduino Nano (ATmega328P).
- **Condicionamento de Energia:** Módulo Step-Down LM2596 (reduzindo os 12V/14.4V da linha pós-chave para 5.0V estabilizados), filtrado por capacitores (470µF na entrada, 100nF na saída).
- **Isolamento Galvânico (Circuito RPM):** Optoacoplador PC817 trabalhando em conjunto com um resistor de 10kΩ (1W) e um diodo 1N4007. Este bloco recebe o pulso de 400V do negativo da bobina, transforma em sinal luminoso interno e aciona o pino digital do Arduino, garantindo separação física entre a alta e a baixa tensão.

### Módulo 2: Interface Gráfica / Cockpit (Coluna de Direção)

Focado exclusivamente na exibição de dados e coleta de inércia, alojado em um *pod* impresso em 3D.

- **Display:** Módulo OLED 2.42" monocromático, interface I2C.
- **Acelerômetro/Giroscópio:** Módulo MPU-6050, interface I2C, fixado no eixo X do veículo.

### Módulo 3: Sensoriamento do Câmbio (Trambulador)

- **Placa:** PCB Customizada (Projeto UPIR) desenhada para abraçar a base da alavanca de câmbio.
- **Sensores:** 4x Sensores Magnéticos Lineares AH3503, acionados por ímãs de neodímio fixados na haste móvel.

## 3. Arquitetura de Software

O código (escrito em C++) foi otimizado para lidar com as limitações de processamento de 16MHz do Arduino, garantindo uma taxa de atualização visual de aproximadamente 25 FPS.

- **Leitura de Rotação (Interrupção de Hardware):** A leitura do motor não ocorre no `loop()` principal. O pino **D2** está configurado via `attachInterrupt`. Cada faísca da bobina gera um gatilho instantâneo que calcula o intervalo entre pulsos em microssegundos. Isso impede que a renderização da tela atrase a contagem de giros.
- **Renderização Gráfica (Geometria Dinâmica):** O leque do conta-giros não é desenhado com cálculos trigonométricos no Arduino. Matrizes de coordenadas (`PROGMEM`) pré-calculadas são armazenadas na memória Flash. A biblioteca `U8g2` renderiza os triângulos baseados nessas matrizes, liberando processamento e memória RAM.
- **Lógica Analógica (Marchas):** As portas `A0`, `A1`, `A2` e `A3` realizam o `analogRead` dos sensores Hall. Variáveis de limiar magnético (calibradas fisicamente) determinam a posição da alavanca na malha cartesiana (H0 a H3).
- **Suavização de Dados:** Aplicação de filtros de média exponencial no RPM para evitar saltos bruscos nos números exibidos na tela.

## 4. Diagrama Lógico de Roteamento (Pinagem)

| **Origem do Sinal** | **Protocolo / Tipo** | **Pino no Arduino** | **Destino / Componente** |
| --- | --- | --- | --- |
| PCB Câmbio (HALL0 - 1ª) | Analógico | `A0` | Entrada Sensorial |
| PCB Câmbio (HALL3 - 5ª) | Analógico | `A1` | Entrada Sensorial |
| PCB Câmbio (HALL1 - 2ª) | Analógico | `A2` | Entrada Sensorial |
| PCB Câmbio (HALL2 - Ré) | Analógico | `A3` | Entrada Sensorial |
| Barramento I2C | Dados (SDA) | `A4` | Display OLED + MPU-6050 |
| Barramento I2C | Clock (SCL) | `A5` | Display OLED + MPU-6050 |
| Negativo da Bobina | Digital (Interrupt) | `D2` | Optoacoplador PC817 (Saída) |

## 5. Medidas de Segurança e Proteção Elétrica

1. **Proteção de Sobrecarga:** Fusível automotivo de lâmina (1A ou 2A) instalado em série na alimentação 12V (Pós-chave / Linha 15).
2. **Blindagem Eletromagnética:** Utilização de Cabo Manga Blindado (6x22 AWG) para a conexão entre a PCB do câmbio e a Central Elétrica. A malha de aço externa é aterrada apenas na ponta da Central, atuando como Gaiola de Faraday contra os ruídos dos cabos de vela.
3. **Proteção Química:** Aplicação de verniz conformal (ou isolante similar) na camada inferior da Placa-Mãe de fenolite para prevenir oxidação das soldas devido a variações térmicas e umidade automotiva.
4. **Conectividade Modular:** Uso extensivo de bornes de parafuso (KRE) e soquetes (barra de pinos fêmea) para garantir que componentes vitais (como o próprio Arduino) possam ser rapidamente removidos, diagnosticados ou substituídos sem a necessidade de retrabalho com ferro de solda no interior do veículo.
https://www.notion.so/adalbertocarv/250520261545-Projeto-arduino-36b5265550c7807396f7ed9f0d67d7f2?source=copy_link#36c5265550c78029a047f73e224d9afd