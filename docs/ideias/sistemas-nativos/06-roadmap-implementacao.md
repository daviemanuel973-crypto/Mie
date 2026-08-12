# 06 — Roadmap de implementação futura

## Objetivo

Implementar os sistemas nativos sem transformar o Mie em um conjunto de features frágeis. A ordem abaixo prioriza fundações reutilizáveis, compatibilidade multiplayer/save e performance Low-first.

## Estado após a v0.5

A v0.4 e a v0.5 já entregaram partes reutilizáveis: save de jogador versionado,
relógio persistente, autoridade de servidor para spawn/cercos, limites de inimigos,
dirty flags de chunks e profiling interno. Isso não conclui a Fase 0: IDs de
conteúdo ainda são posicionais e não existem manifesto central de schemas,
scheduler comum nem contrato genérico de interesse.

A v0.6 começa formalmente por essas lacunas. O quadro de execução, a auditoria e a
rastreabilidade de todas as mecânicas estão em
[`07-plano-v0.6.md`](07-plano-v0.6.md). Este roadmap continua sendo a ordem de longo
prazo; o plano da v0.6 é a decomposição operacional da primeira fase.

## Fase 0 — contratos técnicos

Antes de conteúdo grande:

- definir IDs persistentes seguros;
- definir versionamento/migração de save;
- revisar sistema de registro de itens/blocos/entidades;
- definir tick/scheduler para simulações não ligadas ao FPS;
- definir dirty flags;
- definir interest management de multiplayer;
- adicionar profiling básico por subsistema.

**Saída:** infraestrutura que permite crescer sem quebrar mundos antigos.

## Fase 1 — máquinas básicas

Criar framework genérico de máquinas:

- inventário de entrada/saída;
- progresso;
- receita;
- estado ativo;
- save;
- sincronização cliente/servidor;
- UI comum.

Primeira máquina de teste deve ser simples e provar o framework, não inaugurar vinte receitas.

**Validação Low:** centenas de máquinas ociosas não podem custar atualização pesada por frame.

## Fase 2 — energia elétrica

- gerador;
- acumulador;
- consumidor;
- cabo;
- rede cacheada;
- conexão/desconexão incremental;
- UI de energia;
- persistência;
- multiplayer.

Depois:

- forno elétrico;
- triturador;
- prensa;
- serraria.

**Validação Low:** redes grandes só recalculam topologia quando mudam.

## Fase 3 — logística

- endpoint de inventário;
- tubo básico;
- extrator;
- destino;
- cache de rota;
- filtros;
- prioridades.

Itens transportados devem ser pacotes lógicos, não entidades físicas completas.

Depois:

- esteiras;
- braços/transferidores;
- interfaces com máquinas.

## Fase 4 — fluidos

- tanque;
- bomba;
- pipe;
- endpoint;
- rede agregada;
- água como primeiro caso simples.

Adicionar outros fluidos somente quando tiverem função de gameplay.

## Fase 5 — energia mecânica

- eixo;
- fonte de rotação;
- engrenagens;
- correias/polias;
- roda d'água;
- moinho;
- motor elétrico;
- máquinas mecânicas.

A física deve ser lógica/de rede, e a rotação visual deriva do resultado.

## Fase 6 — villagers base

- entidade persistente;
- nome/visual seed;
- agenda;
- casa;
- estação de trabalho;
- profissão;
- inventário simples;
- reputação do jogador;
- interação básica.

Começar com três profissões:

- agricultor;
- ferreiro;
- comerciante.

## Fase 7 — economia e vila viva

- estoque agregado da vila;
- preços locais;
- demanda/oferta;
- mais profissões;
- guarda;
- curandeiro;
- eventos simples;
- ataques;
- recuperação.

**Validação Low:** vila descarregada é simulada de forma agregada.

## Fase 8 — relações e famílias

- confiança/afinidade;
- relações relevantes persistentes;
- presentes/interações;
- romance opcional;
- família;
- crescimento por tempo do mundo;
- herança/parentesco se trouxer gameplay.

Evitar simulação social global cara.

## Fase 9 — expansão de criaturas

Primeiro fortalecer framework de variantes e spawn.

Depois adicionar conteúdo em pacotes:

- fauna;
- hostis de superfície;
- cavernas;
- elites;
- criaturas regionais.

Cada pacote precisa trazer um motivo de gameplay: loot, recurso, risco ou comportamento novo.

## Fase 10 — estruturas e minibosses

- blueprint procedural;
- anchor determinístico;
- loot table;
- pontos de spawn;
- persistência.

Primeiros exemplos:

- acampamento hostil;
- ruína;
- mina abandonada;
- torre;
- dungeon pequena.

Miniboss deve demonstrar framework de fases/telegraphs antes de bosses gigantes.

## Fase 11 — bosses completos

- arenas;
- fases;
- multiplayer;
- reset;
- loot único;
- integração com tecnologia e villagers.

Bosses não devem ser apenas “mob com HP alto”.

## Fase 12 — automação avançada

- sensores;
- lógica por eventos;
- filtros avançados;
- controle de estoque;
- automação de linhas;
- integração vila/fábrica.

## Fase 13 — contraptions móveis

Somente quando:

- save está robusto;
- multiplayer está estável;
- redes estão maduras;
- profiling prova folga no Low.

Implementar estruturas móveis como conjuntos lógicos, nunca um rigid body por bloco.

## Fase 14 — regiões especiais e dimensões

Antes, expandir o mundo atual com conteúdo suficiente.

Depois avaliar:

- regiões especiais;
- áreas instanciadas;
- finalmente dimensões completas.

Cada nova área deve justificar custo de geração, assets e save.

## Ordem de integração entre pilares

A sequência ideal de experiência é:

`sobrevivência -> recursos -> máquinas -> automação -> comércio -> vila -> exploração perigosa -> materiais raros -> tecnologia avançada -> bosses/regiões novas`

Isso cria progressão contínua em vez de sistemas paralelos sem conexão.

## Definition of Done de qualquer feature grande

Uma feature só está pronta quando:

- funciona em single-player;
- funciona em multiplayer;
- salva/carrega corretamente;
- não quebra saves antigos suportados;
- possui caminho Low completo;
- usa simulação escalável/dormancy quando necessário;
- foi perfilada;
- possui comportamento em erro/sanitize;
- conteúdo visual é original do Mie;
- está documentada.

## Regra final

Não implementar toda a lista de uma vez.

Cada fase deve produzir uma base jogável e testável. Quando uma fundação estiver estável, ela vira ferramenta para criar muito conteúdo com custo menor. Esse é o caminho para o Mie chegar à escala de grandes conjuntos de mods sem carregar a fragilidade de vários sistemas independentes.
