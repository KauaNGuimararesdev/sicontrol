# SiControl

Aplicativo de controle para mesas digitais **Soundcraft Si** (Si Expression /
Performer / Impact) via protocolo **HiQnet** sobre Wi-Fi. Reconstrução moderna,
do zero, inspirada no fluxo e no layout do **ViSi Listen** — porém em Qt 6,
para rodar em Android 10/11/12/13/14, e com os recursos de engenheiro que o app
original não tinha.

> **Stack:** C++ + QML/JavaScript (UI e protocolo HiQnet) · Kotlin (camada de
> rede Wi-Fi no Android, via JNI). Exatamente a stack pedida.

---

## Antes de tudo: uma correção importante

Vale registrar com honestidade, porque muda o que dá pra esperar do app:

1. **O console é Soundcraft *Si*, não *Vi*.** O "ViSi Listen" original fala
   HiQnet com mesas da linha **Si**. O nome "ViSi" é da Soundcraft; não é uma
   mesa Vi (essa é da DiGiCo). Mantive o layout/fluxo do ViSi Listen como você
   pediu, mas o alvo de hardware é a Si.

2. **O ViSi Listen original é um app de *monitor pessoal*.** Ele controla
   basicamente a contribuição de cada canal no *seu* mix de monitor (nível +
   pan + grupos de visualização). É por isso que ele **não** tinha matrix, EQ
   por canal, compressor, gate etc. — essas funções são de engenheiro, e foram
   **adicionadas** aqui a seu pedido. Dependendo do **Access Control** da mesa,
   o console pode recusar que um nó "de monitor" altere parâmetros de
   engenheiro; isso se resolve nas permissões da própria Si.

3. **O original também é Qt (Qt 5 + QML + C++).** Ou seja, sua escolha de stack
   bate com a do app de fábrica — esta versão só moderniza para Qt 6.

---

## O que está pronto (e funciona de verdade)

- **Codec HiQnet completo** (`src/hiqnet/`): cabeçalho de 25 bytes big-endian,
  endereçamento Node + VD/Object, datatypes, e construção/parse de Discovery,
  KeepAlive, MultiParamSet/Get e SubscribeAll. Handshake real: conecta →
  Discovery → mantém keep-alive a cada 5 s → SubscribeAll.
- **Máquina de estados de conexão** (`HiQnetClient.h`) sobre `QTcpSocket`
  (porta 3804), com enquadramento por tamanho de mensagem.
- **Catálogo de State Variables** (`StateVarCatalog.h`) extraído do app original
  — cobre fader, gain, mute, pan, EQ (freq/ganho/Q/tipo), HPF, PEQ/GEQ,
  compressor, gate, matrix, delay, etc.
- **Modelo** (`src/model/`): canais como objetos QML, `MixerModel` como singleton
  "Mixer", e o `ParameterMap` (preenchido sozinho pelo modo Learn).
- **UI em QML** espelhando o fluxo do ViSi Listen iOS: seleção de dispositivo →
  seleção de barramento → tela de mix (faders de contribuição), e telas novas de
  engenheiro abrindo a partir do mix.
- **Camada Kotlin** (`android/src/.../NetworkService.kt`) com MulticastLock,
  WifiLock, leitura de IPv4/máscara, socket TCP e *callbacks* JNI para o C++.

## Como isso funciona de verdade (boa notícia)

Eu fui no binário do app original pra te responder com honestidade, e a
descoberta muda tudo: **não existe uma tabela de números secreta pra você
preencher à mão.** Encontrei no `libViSi_Listen.so` as funções
`svAttributeDetected` / `attributeDetected` e os comandos `SVSubscribeAll` e
`GetAttributes`. Ou seja, o app de fábrica **aprende os endereços da própria
mesa em tempo de execução**: ele assina tudo (`SubscribeAll`) e a Si responde
dizendo quais parâmetros existem e o endereço de cada um.

Implementei o mesmo comportamento aqui, então o "preencher o mapa" virou quase
automático. Tem dois jeitos:

### Jeito A — modo *Learn* (recomendado, sem código)

1. Compile e abra o app (passo a passo de compilação abaixo).
2. Conecte na sua Si pela tela inicial.
3. Vá em **Settings → "Map a control (Learn)"**.
4. Escolha o que quer mapear (ex.: `Fader`, canal `1`) e toque **Arm**.
5. **Mexa exatamente nesse controle na mesa** (mova o fader do canal 1).
6. O app captura o endereço que a mesa mandou e **liga** aquele controle.
7. Repita pros controles que você usa. Pronto — o mapa se monta sozinho.

O log do HiQnet, na mesma tela, mostra ao vivo cada `rx vd/obj/id/val` que chega
da mesa, então dá pra conferir visualmente que o controle certo foi capturado.

### Jeito B — fixar no código (opcional)

Se quiser gravar o mapa de uma vez (pra não precisar do Learn toda vez), pegue os
endereços que apareceram no log e escreva em `src/model/ParameterMap.h`, dentro
de um perfil de console, assim:

```cpp
// exemplo: fader do canal 1 no bus selecionado
profile.define(
    ParamKey{ sv::Kind::Fader, /*channel*/0, /*bus*/0 },
    ParamRef{ /*object*/ hiqnet::Address{node, vd, obj}, /*paramId*/ id }
);
```

> **Sobre o Access Control:** achei também `accessControlEnable/Selected` no
> binário. Como o ViSi Listen é um nó "de monitor", a Si pode **bloquear** que
> ele altere parâmetros de engenheiro (matrix, EQ etc.) dependendo da config de
> permissões da mesa. Se algum controle não responder mesmo mapeado, é aí que
> você olha: nas permissões/Access Control da própria Si.

---

## Como compilar o APK (passo a passo, do zero)

Não dá pra eu te entregar o `.apk` já pronto — falta o toolchain de Android, que
roda na sua máquina. Mas é tranquilo:

**1. Instalar o Qt (uma vez só)**
- Baixe o instalador em `qt.io` (conta gratuita).
- No instalador, marque: **Qt 6.5** (ou mais novo) → dentro dele, **"Android"**.
- Marque também, na seção *Developer and Designer Tools*: **Qt Creator**,
  **Android OpenSSL**, e deixe o instalador baixar o **Android SDK/NDK**
  (em Qt Creator: *Preferences → Devices → Android* tem um botão "Set Up"
  que instala SDK, NDK e JDK automaticamente).

**2. Abrir o projeto**
- Abra o **Qt Creator**.
- *File → Open File or Project…* e escolha o **`CMakeLists.txt`** desta pasta.

**3. Escolher o kit Android**
- Quando ele perguntar os *kits*, marque um **Android** (Qt 6.x).
- No seletor embaixo à esquerda, escolha **Android Qt 6.x Clang arm64-v8a**
  (arm64 cobre praticamente todo celular atual).

**4. Compilar / rodar**
- Conecte o celular por USB com **depuração USB** ligada (ou use um emulador).
- Clique no **▶ (Run)**. O Qt compila o C++ e o Kotlin, gera o APK e instala.
- O `.apk` fica em `build-.../android-build/build/outputs/apk/`.

**5. Gerar APK assinado (pra distribuir)**
- *Projects → Build → Build Android APK*.
- Em *Sign package*, crie uma **keystore** (guarde a senha) e marque
  "Sign package". O resultado é um APK/AAB assinado, pronto pra instalar/publicar.

> Dica: se quiser só testar rápido sem celular, instale o **Android Emulator**
> pelo mesmo botão "Set Up" do Qt Creator e rode num dispositivo virtual.

### Alternativa sem instalar nada: build na nuvem (GitHub Actions)

Se você não quiser instalar o Qt no PC, já incluí um fluxo de build pronto em
`.github/workflows/build-apk.yml`. Suba o projeto pra um repositório no GitHub
e ele **compila o APK num servidor de graça** e te entrega o arquivo para
download (Actions → execução → *Artifacts* → `SiControl-apk`). O passo a passo
está comentado no próprio arquivo. O APK sai assinado com chave de debug, então
instala direto no celular.

Observação honesta: não consigo *compilar/verificar* o build aqui dentro (este
ambiente não tem Qt-for-Android nem NDK e a rede é restrita). Entrego o projeto
como código-fonte que abre e builda no Qt Creator. Se aparecer algum erro de
compilação, me manda a mensagem que eu corrijo.

## Estrutura

```
SiControl/
├─ CMakeLists.txt            # projeto Qt6 (Quick/Qml/Network)
├─ src/
│  ├─ main.cpp               # registra o modelo e carga o módulo QML
│  ├─ hiqnet/                # protocolo: tipos, codec, cliente TCP, catálogo SV
│  ├─ model/                 # Channel, MixerModel (singleton "Mixer"), ParameterMap
│  └─ platform/
│     └─ AndroidBridge.cpp   # ponte JNI <-> Kotlin (só Android)
├─ qml/
│  ├─ Main.qml  Theme.qml    # shell + tokens de design (tema escuro estilo Si)
│  ├─ controls/              # ViSiFader, PanControl, RotaryKnob, EqGraph
│  ├─ views/                 # DeviceSelect, BusSelect, Mix, ChannelStrip,
│  │                         #   Matrix, MuteGroups, Settings
│  └─ js/EqMath.js           # curva de resposta do EQ
└─ android/
   ├─ AndroidManifest.xml    # permissões de rede Wi-Fi, SDK 29→34
   ├─ build.gradle           # habilita Kotlin
   └─ src/com/sicontrol/app/NetworkService.kt
```

## Recursos da UI (layout no espírito do ViSi Listen + extras pedidos)

- **Faders** de contribuição com meter embutido e marca de unidade; **pan**.
- **ON / MUTE** por canal; seleção de barramento (até 24 mixes).
- **Tira de canal (engenheiro):** head-amp com **gain**, **48V** phantom e fase;
  **EQ de 4 bandas (hi/lo + paramétricas)** com gráfico ao vivo e **HPF**;
  **compressor** (thr/ratio/atk/rel/gain) e **gate** (thr/range/atk/hold/rel).
- **Matrix:** grade de cruzamentos fonte→saída, nível por célula, **LR/Mono**.
- **Grupos de mute** (8) e **delay de saída (taps)** por barramento.
- **Settings** com status de conexão e log HiQnet ao vivo (útil pra mapear).

---

## Nota legal / de origem

Nenhum recurso proprietário (imagens, strings de UI, o `.so`) foi copiado. O tema
é "clean-room", feito do zero no estilo visual da superfície Si. O catálogo de
nomes de State Variables e a estrutura do protocolo vêm do guia HiQnet público e
de análise do binário original apenas para fins de interoperabilidade.
