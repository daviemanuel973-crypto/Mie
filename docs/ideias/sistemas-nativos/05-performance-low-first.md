# 05 — Performance: Low-first obrigatório

## Regra máxima

**Todo conteúdo novo do Mie deve ser projetado para funcionar primeiro no pior preset suportado.**

O preset Low não é uma versão incompleta do jogo. Ele deve manter todas as mecânicas, progressão, IA essencial, multiplayer, economia, máquinas, bosses e exploração.

Presets acima do Low podem aumentar qualidade, densidade visual e distância, mas não podem ser necessários para compreender ou utilizar uma mecânica.

## O que pode escalar por preset

- resolução e qualidade de sombras;
- distância de renderização;
- LOD;
- partículas;
- luzes dinâmicas visuais;
- reflexos;
- bloom;
- SSAO;
- animações secundárias;
- distância de efeitos decorativos;
- quantidade de detalhes cosméticos;
- frequência visual de elementos não críticos.

## O que não pode depender do preset

- regras de combate;
- dano;
- drops;
- receitas;
- funcionamento de máquinas;
- economia;
- comportamento lógico de villagers;
- funcionamento de tubos/cabos;
- fases de boss;
- persistência;
- resultado de geração do mundo;
- autoridade multiplayer.

## Orçamento de CPU

### Proibido

- varrer todos os NPCs por frame;
- varrer todas as máquinas por frame;
- recalcular redes inteiras sem mudança topológica;
- pathfinding ilimitado;
- milhares de itens logísticos como rigid bodies;
- reconstruir meshes sem dirty flag;
- fazer geração procedural pesada na thread principal sem orçamento;
- serializar o mundo inteiro após pequenas alterações.

### Obrigatório

- atualização em ticks;
- time slicing;
- filas de trabalho;
- spatial partitioning;
- caches;
- dirty flags;
- dormancy;
- simulação por nível de interesse;
- trabalho assíncrono apenas quando thread-safe e realmente benéfico.

## Níveis de simulação

Cada sistema que pode existir em grande escala deve possuir estados equivalentes a:

1. **Full** — próximo e relevante;
2. **Reduced** — carregado, mas distante;
3. **Dormant/Aggregated** — sem necessidade de comportamento detalhado;
4. **Unloaded** — apenas dados persistentes.

Exemplo de villager:

- Full: pathfinding, interação e animação;
- Reduced: agenda em baixa frequência;
- Dormant: somente estado lógico da vila;
- Unloaded: timestamps e dados persistentes.

Exemplo de máquina:

- Full: processo + efeitos visuais;
- Reduced: processo lógico sem efeitos;
- Dormant: cálculo por eventos/timestamps quando seguro;
- Unloaded: estado salvo.

## Orçamento de GPU

### Reutilizar

- atlas/arrays já suportados pelo renderer;
- batching;
- instancing quando aplicável;
- meshes compartilhadas;
- materiais compartilhados;
- culling por frustum/distância;
- LOD.

### Evitar

- material exclusivo por máquina idêntica;
- draw call por cabo individual quando puder haver batching;
- luz dinâmica em cada máquina;
- transparência excessiva;
- partículas permanentes de milhares de blocos;
- shaders obrigatórios que dependam de extensões não disponíveis no caminho compatível do Linux.

## Redes industriais

A topologia é um grafo cacheado.

Recalcular somente quando a estrutura muda.

Mudanças múltiplas no mesmo tick devem ser coalescidas quando possível.

Uma rede enorme pode ser processada por orçamento/tick, sem bloquear o frame.

## Logística

Itens em tubos/esteiras não devem consumir o custo de entidades completas do mundo.

Preferir structs compactas e pools.

Quando distantes, vários pacotes iguais podem ser agregados se isso não alterar gameplay observável.

## IA e pathfinding

Implementar fila global/regional de path requests.

Prioridades:

1. combate próximo ao jogador;
2. NPC em interação;
3. movimentação visível;
4. tarefas rotineiras;
5. simulação distante.

Se o orçamento acabar, tarefas de baixa prioridade esperam; o frame não deve travar.

## Bosses

Boss deve ser visualmente legível no Low.

Ataques importantes precisam de:

- animação clara;
- geometria/telegraph simples;
- som quando apropriado;
- indicação que sobreviva com partículas reduzidas.

## Estruturas e geração

Geração deve ser determinística e incremental.

Pré-calcular apenas o necessário. Estruturas grandes são divididas entre chunks e não devem forçar geração completa de regiões fora do interesse atual.

## Memória

- compartilhar assets;
- descarregar recursos não utilizados quando seguro;
- limitar caches;
- usar tipos compactos para estados em massa;
- não manter path histories completos;
- não manter cópias redundantes de inventários/redes.

## Multiplayer

Performance de rede é parte do Low-first.

- interest management;
- deltas;
- quantização onde não prejudicar precisão;
- IDs compactos;
- eventos em vez de spam por frame;
- servidor não deve simular efeitos puramente gráficos.

## Profiling obrigatório antes de considerar um sistema concluído

Medir pelo menos:

- tempo de CPU por sistema;
- picos/frame-time;
- número de entidades ativas;
- path requests;
- máquinas ativas;
- tamanho das redes;
- quantidade de pacotes logísticos;
- draw calls quando pertinente;
- tráfego multiplayer;
- tamanho/tempo de save.

## Cenários de stress sugeridos

- vila com muitos moradores;
- ataque à vila + combate simultâneo;
- base com centenas de máquinas;
- redes longas de cabo/tubos;
- milhares de itens processados ao longo do tempo;
- boss com vários jogadores;
- exploração rápida carregando chunks novos;
- mundo antigo com muitas estruturas persistentes.

## Critério de aprovação

Uma feature não deve ser considerada terminada apenas porque funciona numa máquina forte.

Ela deve:

1. ter caminho Low funcional;
2. possuir dormancy/redução quando escalável;
3. não criar trabalho global por frame;
4. ter comportamento consistente em multiplayer;
5. não depender de efeitos avançados;
6. ser perfilada em cenário normal e de stress;
7. degradar qualidade visual antes de degradar jogabilidade.

Enquanto a especificação mínima oficial de hardware do Mie não estiver fechada, os números absolutos de FPS/frame budget devem ser tratados como metas de profiling a definir. A arquitetura, porém, deve seguir estas regras desde o primeiro protótipo.
