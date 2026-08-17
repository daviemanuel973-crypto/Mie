# 07 — Plano de execução da v0.6

## Estado

- **Base:** `main` na v0.5, commit `06ae7d6`.
- **Branch de trabalho:** `agent/native-systems-foundation-v0.6`.
- **Situação histórica:** implementação executável concluída. A validação de
  distribuição Windows/Flatpak foi concluída pelo CI nas versões posteriores.
- **Runtime:** identificado como `0.6.0`.

Este documento transforma o roadmap de longo prazo em trabalho implementável. A
entrega não adiciona blocos, itens, entidades, pacotes, receitas ou assets e não
altera os formatos de save existentes; seus novos payloads são independentes.

## Resultado pretendido para a v0.6

A v0.6 deve entregar os contratos da Fase 0 e provar esses contratos com um único
piloto de máquina. O piloto existe para validar arquitetura; ele não autoriza a
implementação simultânea de energia, logística, villagers e bosses.

Ao final da versão, o projeto deve conseguir adicionar conteúdo persistente sem:

- renumerar conteúdo existente;
- depender do FPS para simulação;
- atualizar objetos ociosos globalmente por frame;
- enviar estado irrelevante para todos os clientes;
- regravar o mundo inteiro após uma alteração local;
- quebrar mundos, jogadores ou inventários da v0.5.

## Limites desta versão

### Dentro da v0.6

- congelar e testar os IDs legados da v0.5;
- introduzir registro estável de conteúdo com adaptação para IDs numéricos legados;
- criar manifesto de schema do mundo e caminho explícito de migração;
- criar scheduler de gameplay com orçamento e níveis de simulação;
- uniformizar dirty flags, eventos internos e escopos de interesse;
- expor métricas de subsistemas sem depender da janela gráfica de profiler;
- validar a fundação com um piloto interno de máquina persistente e autoritativa;
- manter testes de compatibilidade Windows/Linux e saves antigos.

### Fora da v0.6

- pacote completo de villagers, profissões, relações ou famílias;
- economia local e eventos de vila;
- rede elétrica jogável, tubos, fluidos ou energia mecânica;
- contraptions móveis;
- grande pacote de criaturas, minibosses ou bosses;
- regiões especiais ou dimensões;
- produção em massa de receitas, blocos, modelos, texturas ou efeitos.

Esses sistemas permanecem aprovados como direção de longo prazo. Eles apenas não
podem avançar antes de seus contratos compartilhados estarem testados.

## Auditoria da base v0.5

| Área | O que já existe | Lacuna para os sistemas nativos | Decisão da v0.6 |
|---|---|---|---|
| Blocos | `BlockTypes` numérico e append-only | posição no enum ainda é identidade persistente | congelar mapa legado e adicionar chave estável sem renumerar |
| Itens | `ItemTypes` numérico a partir de `2048` | registro e validação continuam centralizados em código | adaptar conteúdo legado a um registro validado |
| Entidades | oito tipos codificados no byte alto do EID | adicionar tipo exige alterar macros, containers e serialização | preservar namespace legado e criar descritor explícito por tipo |
| Save de jogador | snapshot v1 separado do layout runtime, `safeSave` e sanitize | não há dispatcher comum de migrações | manter formato v1 e registrar migrações por schema |
| Progresso do mundo | relógio/cercos v1 com checksum e recuperação | cada subsistema ainda define seu formato isoladamente | adicionar manifesto de schemas por mundo |
| Chunks e block data | dirty flags e salvamento por chunk | formatos não têm um catálogo central de versões | versionar novos payloads sem reescrever chunks antigos |
| Tick do servidor | 20 Hz e atualização dividida por regiões | trabalhos futuros não têm fila/orçamento comum | scheduler sobre o tick existente, sem segundo loop concorrente |
| Multiplayer | autoridade do servidor e streaming por chunks | falta contrato reutilizável de interesse e deltas | definir escopos por chunk, jogador, interação e evento global |
| Profiling | CPU/GPU profiler interno e seções do tick | métricas dependem muito da UI interna e não cobrem filas futuras | contadores headless por subsistema e cenários de stress |
| Loot | tabelas reutilizáveis em C++ | conteúdo ainda é pequeno e hardcoded | preservar API e preparar validação data-driven posterior |
| Estruturas | templates em pastas e geração existente | catálogo e carregamento são hardcoded | registrar metadados antes de ampliar o conteúdo |
| Aventura | variantes, spawn natural, estruturas e cercos | não há framework genérico completo de criaturas/bosses | tratar como fundação parcial, sem reescrever na v0.6 |

## Contratos que a v0.6 deve fixar

### 1. Identidade de conteúdo

- IDs numéricos atuais nunca mudam de significado.
- Novos dados usam uma chave estável com namespace, por exemplo
  `mie:block/<nome>`, sem tornar strings obrigatórias no hot path.
- O registro resolve chave estável para ID runtime compacto.
- Chaves duplicadas, IDs fora da faixa e conteúdo ausente falham de forma clara.
- Conteúdo desconhecido em save é preservado ou substituído por fallback explícito;
  nunca é reinterpretado silenciosamente como outro item.

### 2. Save e migração

- Cada payload novo declara magic, versão, limites e validação.
- O mundo mantém um manifesto pequeno com as versões de cada subsistema.
- Migrações são sequenciais e testáveis: `N -> N+1`.
- A leitura nunca grava automaticamente antes de concluir parse, sanitize e migração.
- Saves da v0.5 entram nos testes como fixtures imutáveis.

### 3. Simulação e scheduler

- O servidor continua como fonte da verdade.
- O scheduler é alimentado pelo tick de 20 Hz existente.
- Trabalhos declaram categoria, prioridade, custo estimado e próxima execução.
- Níveis mínimos: `Full`, `Reduced`, `Dormant/Aggregated` e `Unloaded`.
- Exceder o orçamento adia trabalho não crítico; não altera regras de gameplay.
- Combate próximo e interação têm prioridade sobre rotinas e catch-up distante.

### 4. Dirty flags e eventos

- Dirty flags descrevem o que mudou: persistência, rede, topologia, inventário,
  processo ou apresentação.
- Eventos comunicam fatos consumados; não substituem autoridade nem viram um
  event bus global usado para tudo.
- Eventos não confiáveis ou puramente visuais nunca decidem loot, dano ou save.

### 5. Interesse multiplayer

- `Chunk`: estado espacial observado.
- `Player`: dados privados/relevantes a um jogador.
- `Interaction`: UI aberta ou operação em andamento.
- `Party/Settlement`: grupo lógico futuro.
- `Global`: apenas eventos realmente globais.

Cada novo pacote deve declarar um desses escopos. Broadcast global é exceção
justificada, não comportamento padrão.

### 6. Observabilidade Low-first

Cada subsistema escalável deve fornecer pelo menos:

- tempo de CPU acumulado e pico;
- quantidade total, ativa, reduzida e dormant;
- trabalhos executados, adiados e descartados;
- bytes salvos e replicados quando pertinente;
- tamanho das filas e caches;
- contador de sanitize/migração/fallback.

## Quadro de execução da v0.6

| Pacote | Entrega | Dependências | Critério de saída | Estado |
|---|---|---|---|---|
| `V06-00` | organização, auditoria e rastreabilidade | v0.5 integrada | escopo e ordem documentados sem mudar runtime | **Concluído nesta etapa** |
| `V06-01` | baseline de compatibilidade | `V06-00` | fixtures e testes congelam IDs/protocolos/saves v0.5 | **Concluído** |
| `V06-02` | registro estável de conteúdo | `V06-01` | legado resolve para os mesmos IDs e rejeita colisões | **Concluído** |
| `V06-03` | manifesto e migrações de mundo | `V06-01`, `V06-02` | mundo v0.5 abre sem regravação destrutiva | **Concluído** |
| `V06-04` | scheduler e níveis de simulação | `V06-01` | orçamento determinístico testado sem depender do FPS | **Concluído** |
| `V06-05` | dirty flags, eventos e interesse | `V06-02`, `V06-04` | deltas locais não viram broadcast nem full save | **Concluído** |
| `V06-06` | métricas e stress harness | `V06-04`, `V06-05` | execução headless mede custo e filas | **Concluído** |
| `V06-07` | piloto interno de máquina | `V06-02` a `V06-06` | processa, salva, carrega e replica em SP/MP | **Concluído** |
| `V06-08` | hardening e empacotamento | todos | MSVC/Flatpak, migração e Low aprovados | **Concluído e validado pelo CI** |

Os pacotes `V06-01` a `V06-07` estão implementados. Nenhum bloco de máquina foi
adicionado; expansões jogáveis continuam condicionadas à validação distributiva de
`V06-08`.

## Rastreabilidade das mecânicas de longo prazo

### Fundação compartilhada

| ID | Mecânica | Situação atual | Porta de entrada |
|---|---|---|---|
| `ARC-01` | IDs estáveis e registro | legado parcial | v0.6 `V06-01/02` |
| `ARC-02` | schemas e migrações | parcial em jogador/relógio | v0.6 `V06-03` |
| `ARC-03` | scheduler com orçamento | tick/regions parciais | v0.6 `V06-04` |
| `ARC-04` | dirty flags e eventos | dirty de chunk parcial | v0.6 `V06-05` |
| `ARC-05` | interest management | streaming de chunk parcial | v0.6 `V06-05` |
| `ARC-06` | profiling por subsistema | profiler interno parcial | v0.6 `V06-06` |
| `ARC-07` | conteúdo dirigido por dados | estruturas/loot parciais | incremental após o registro |

### Tecnologia e indústria

| ID | Mecânica | Depende de | Ordem |
|---|---|---|---|
| `TEC-01` | framework genérico de máquinas | `ARC-01` a `ARC-06` | piloto v0.6 |
| `TEC-02` | receitas de processamento | `TEC-01`, `ARC-07` | após piloto |
| `TEC-03` | energia elétrica e cabos | `TEC-01`, `ARC-03/04` | antes das máquinas elétricas |
| `TEC-04` | gerador, acumulador e consumidores | `TEC-03` | primeiro pacote elétrico |
| `TEC-05` | logística de itens | `TEC-01`, `ARC-03/05` | após energia estável |
| `TEC-06` | fluidos, tanque e bomba | `TEC-05` | água como primeiro caso |
| `TEC-07` | energia mecânica | `ARC-03/04`, `TEC-01` | após fluidos |
| `TEC-08` | esteiras e transferidores | `TEC-05`, `TEC-07` | após redes básicas |
| `TEC-09` | automação e controle | `TEC-03/05/07` | após integração das redes |
| `TEC-10` | contraptions móveis | todos acima + save/MP maduros | penúltima etapa tecnológica |

### Villagers e sociedade

| ID | Mecânica | Depende de | Ordem |
|---|---|---|---|
| `SOC-01` | entidade Villager persistente | `ARC-01` a `ARC-05` | após fundações industriais básicas |
| `SOC-02` | agenda, casa e trabalho | `SOC-01`, scheduler | primeira vertical social |
| `SOC-03` | agricultor, ferreiro e comerciante | `SOC-02`, conteúdo validado | profissões iniciais |
| `SOC-04` | estoque e economia local | `SOC-03`, eventos | após trocas básicas |
| `SOC-05` | guarda, curandeiro e eventos | `SOC-04`, aventura | vila viva |
| `SOC-06` | confiança, afinidade e presentes | `SOC-01`, save social | relações opcionais |
| `SOC-07` | romance, família e crescimento | `SOC-06`, relógio do mundo | depois da estabilidade social |

### Aventura, criaturas e mundo

| ID | Mecânica | Situação/dependência | Ordem |
|---|---|---|---|
| `ADV-01` | framework de criaturas/variantes | parcial com zumbis e goblins; requer `ARC-01/03/05` | fortalecer antes de novos pacotes |
| `ADV-02` | spawn por orçamento/categoria | parcial desde v0.4/v0.5 | generalizar com métricas |
| `ADV-03` | fauna e hostis regionais | `ADV-01/02`, loot validado | pacotes pequenos |
| `ADV-04` | estruturas com anchor e metadados | templates atuais parciais; requer `ARC-01/07` | antes de estruturas grandes |
| `ADV-05` | loot/raridade orientados por dados | loot atual parcial; requer `ARC-07` | antes de minibosses |
| `ADV-06` | miniboss e telegraphs | `ADV-01/04/05` | prova do boss framework |
| `ADV-07` | bosses completos | `ADV-06`, multiplayer e profiling | após prova de fases/reset |
| `ADV-08` | regiões especiais | geração/save maduros | antes de dimensões |
| `ADV-09` | dimensões | todos os contratos de unload/namespace | última etapa |

## Ordem de dependência preservada

1. Fundação da v0.6.
2. Máquina genérica e receitas de processamento.
3. Energia elétrica.
4. Logística de itens.
5. Fluidos.
6. Energia mecânica.
7. Villagers base, economia e relações.
8. Expansão do framework de criaturas e spawn.
9. Estruturas, minibosses e bosses.
10. Automação avançada e contraptions.
11. Regiões especiais e dimensões.

O conteúdo de aventura que já existe não é removido nem bloqueado por essa ordem. A
sequência define onde investir a próxima expansão estrutural.

## Matriz mínima de testes

| Área | Teste obrigatório |
|---|---|
| IDs | snapshot de todos os IDs v0.5 e detecção de colisão |
| Save | fixture v0.5, truncamento, versão desconhecida e migração interrompida |
| Scheduler | orçamento, prioridade, adiamento, catch-up limitado e determinismo |
| Dirty flags | nenhuma gravação/replicação sem mudança; delta correto após mudança |
| Interesse | dois jogadores em chunks distintos não recebem estado irrelevante |
| Máquina piloto | processo SP/MP, unload/reload, reinício e entrada tardia do cliente |
| Low | centenas de objetos ociosos sem varredura global cara |
| Distribuição | MSVC sem console, instalador/update e Flatpak com save preservado |

## Regras de mudança durante a v0.6

- Cada pacote vira um commit pequeno e reversível.
- Nenhuma etapa posterior começa com teste obrigatório anterior vermelho.
- Mudança de ID, save ou pacote de rede exige teste de compatibilidade no mesmo commit.
- Conteúdo novo entra somente depois da infraestrutura que o torna seguro.
- O plano pode ser refinado com evidência de profiling, mas os pilares e as garantias
  Low-first não podem ser removidos para acelerar uma entrega.
