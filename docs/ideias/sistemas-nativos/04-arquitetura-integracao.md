# 04 — Arquitetura compartilhada e integração

## Objetivo

Evitar que villagers, tecnologia, logística, criaturas e estruturas virem sistemas isolados. Eles devem compartilhar fundações e conversar por interfaces estáveis.

## Princípio central

Separar:

- **dados persistentes**;
- **simulação autoritativa**;
- **apresentação cliente**;
- **conteúdo dirigido por dados**.

Nenhum sistema deve depender de efeitos gráficos para funcionar.

## IDs estáveis

Criar identificadores estáveis para conteúdo persistente:

- tipo de item;
- tipo de bloco;
- tipo de entidade;
- profissão;
- receita;
- máquina;
- estrutura;
- loot table;
- região/dimensão.

Evitar depender exclusivamente de posição em enum quando mudanças futuras puderem quebrar saves. Quando possível, usar versionamento/migração ou IDs registrados explicitamente.

## Event bus / eventos de gameplay

Sistemas devem se comunicar por eventos bem definidos em vez de referências cruzadas excessivas.

Exemplos conceituais:

- `ItemCrafted`;
- `MachineCompletedRecipe`;
- `VillagerTradeCompleted`;
- `StructureDiscovered`;
- `BossDefeated`;
- `VillageAttacked`;
- `ResourceDelivered`;
- `NetworkTopologyChanged`.

Não é obrigatório criar um event bus genérico imediatamente. A regra é evitar acoplamento circular e polling global.

## Componentes reutilizáveis

Possíveis interfaces/componentes futuros:

- `InventoryHolder`;
- `EnergyProducer`;
- `EnergyConsumer`;
- `EnergyStorage`;
- `ItemTransportEndpoint`;
- `FluidEndpoint`;
- `MechanicalNode`;
- `TradeProvider`;
- `FactionMember`;
- `LootProvider`;
- `PersistentWorldObject`.

Usar composição quando reduzir duplicação sem criar abstrações inúteis.

## Dirty flags

Objetos persistentes e sincronizados devem marcar alterações.

Exemplo:

- inventário mudou -> dirty inventory;
- energia mudou significativamente -> dirty energy;
- relação mudou -> dirty social;
- máquina mudou de estado -> dirty process;
- rede foi alterada -> dirty topology.

Save e rede podem processar apenas dados sujos quando seguro.

## Simulação em ticks

Não atrelar tudo ao FPS.

Categorias sugeridas:

- movimento/física: frequência alta onde necessário;
- combate: tick fixo;
- máquinas: tick fixo ou event-driven;
- energia/logística: tick fixo e incremental;
- IA próxima: frequência moderada;
- IA distante: frequência baixa;
- economia/vilas: segundos ou eventos;
- mundo descarregado: cálculo por diferença de tempo.

As frequências concretas devem ser perfiladas antes de se tornarem contrato permanente.

## Scheduler com orçamento

Criar futuramente um scheduler de trabalho de gameplay com filas por categoria.

Objetivo:

- impedir centenas de pathfinds no mesmo frame;
- repartir recomputação de redes;
- repartir atualizações de máquinas;
- evitar spikes ao carregar chunks;
- permitir que o preset Low use orçamento mais conservador.

Trabalhos adiáveis podem ser distribuídos ao longo de vários frames/ticks.

## Spatial partitioning

Consultas de mundo devem preferir chunk/região/célula espacial.

Não procurar “todos os NPCs”, “todas as máquinas” ou “todos os mobs” do mundo para resolver uma interação local.

## Save

Cada grande sistema deve ter:

- versão de schema;
- valores padrão para campos novos;
- validação/sanitize ao carregar;
- tolerância a conteúdo desconhecido quando possível;
- migração explícita para mudanças incompatíveis.

Salvar estado mínimo necessário. Estado derivável deve ser reconstruído.

## Chunks carregados e descarregados

### Carregado e próximo

Simulação completa necessária.

### Carregado e distante

Simulação reduzida.

### Descarregado

Somente estado persistente/agregado.

Ao recarregar, calcular progresso offline por timestamps quando apropriado, por exemplo produção de vila ou máquina simples. Para sistemas complexos, aplicar um limite máximo de catch-up para evitar travamentos após longos períodos.

## Multiplayer e replicação

Servidor é fonte da verdade.

Preferir mensagens por delta/evento em vez de snapshots enormes frequentes.

Exemplos:

- máquina inicia receita: enviar receita/tempo inicial;
- cliente anima progresso localmente;
- servidor envia correção apenas se necessário;
- item entra em tubo: enviar rota/segmento e tempo;
- cliente interpola posição visual;
- relação social muda: enviar somente para jogadores relevantes.

## Interesse de rede

Não replicar tudo para todos.

Filtrar por:

- distância;
- chunk observado;
- ownership/interação;
- relevância de UI;
- evento global realmente necessário.

## Conteúdo orientado por dados

Bons candidatos:

- receitas;
- loot;
- profissões;
- ofertas base;
- variantes;
- spawn rules;
- blueprint de estruturas;
- stats de máquinas;
- tiers tecnológicos.

Código fica responsável por comportamento; dados descrevem conteúdo.

## Integrações desejadas

### Sociedade + indústria

- villagers demandam produtos;
- profissões compram/vendem materiais industriais;
- máquinas podem abastecer economia local;
- trabalhos futuros podem operar estações simples.

### Aventura + indústria

- estruturas fornecem componentes;
- bosses desbloqueiam materiais/técnicas;
- máquinas refinam recursos raros.

### Aventura + sociedade

- vilas sofrem ataques/eventos;
- reputação muda por defesa/ajuda;
- comerciantes especiais surgem após descobertas.

### Tudo junto

O jogador deve sentir que está jogando um único Mie, não três mods colados.
