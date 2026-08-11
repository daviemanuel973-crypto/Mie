# 01 — Villagers, sociedade, trocas e famílias

## Visão

Os villagers do Mie devem ser um **sistema social nativo do jogo base**, não uma camada opcional ou “mod embutido”. Eles devem fazer parte da exploração, economia, progressão e sensação de mundo vivo.

A inspiração é a profundidade social de mods como Comes Alive, mas o resultado deve ser próprio do Mie.

## Objetivos de gameplay

- vilas com moradores persistentes e identificáveis;
- nomes, aparência, personalidade e relações simples;
- profissões úteis ao ecossistema do jogo;
- sistema de trocas baseado em oferta, demanda e reputação;
- amizade, confiança, rivalidade e romance de forma leve e opcional;
- casamento e família como progressão social, não como obrigação;
- moradores capazes de trabalhar, descansar, conversar, proteger-se e reagir ao estado da vila;
- eventos locais: falta de alimento, ataques, festivais, pedidos, construção e recuperação da vila.

## Entidade base

Criar no futuro uma entidade genérica `Villager` com componentes de dados em vez de subclasses para cada profissão.

Estado persistente sugerido:

- `villagerId` estável;
- nome e seed visual;
- idade/faixa etária;
- profissão;
- nível de profissão;
- casa/bed assignment;
- local de trabalho;
- necessidades básicas simplificadas;
- reputação por jogador;
- relações com outros villagers;
- inventário pessoal pequeno;
- agenda atual;
- flags narrativas/eventos;
- parentesco;
- estado vivo/morto/desaparecido.

Evitar salvar estados transitórios desnecessários, como path completo atual.

## Profissões iniciais

Começar pequeno:

- agricultor;
- ferreiro;
- comerciante;
- lenhador;
- minerador;
- curandeiro;
- guarda.

Cada profissão deve compartilhar o mesmo framework e trocar apenas:

- conjunto de tarefas;
- tabela de produtos/necessidades;
- estação de trabalho;
- progressão de habilidade;
- itens aceitos/oferecidos.

## Economia e trocas

As trocas não devem ser uma lista completamente fixa.

Modelo sugerido:

`preço final = preço base × disponibilidade × reputação × dificuldade × modificador regional`

A economia deve ser **local e limitada**, sem simular um mercado global pesado.

Cada vila pode manter contadores agregados de estoque por categoria:

- comida;
- madeira;
- pedra;
- metais;
- ferramentas;
- remédios;
- itens raros.

Esses contadores podem atualizar em baixa frequência e alimentar preços sem varrer inventários de todos os NPCs a cada compra.

## Relações

Usar valores compactos, por exemplo:

- confiança;
- afinidade;
- medo;
- vínculo familiar.

Evitar uma matriz completa N×N para milhares de NPCs. Persistir apenas relações relevantes ou acima de um limiar.

Interações possíveis:

- conversar;
- presentear;
- negociar;
- ajudar em eventos;
- contratar/acompanhante futuramente;
- formar família.

## Família e crescimento

Famílias devem ser persistentes, mas a simulação pode ser abstrata quando longe do jogador.

Filhos não precisam executar IA completa até estarem próximos de um jogador. Crescimento pode ser calculado por timestamp do mundo.

## IA em camadas

### Próximo do jogador

- navegação real;
- animações;
- percepção simples;
- combate/fuga;
- tarefas visíveis.

### Distância média

- IA reduzida;
- decisões em baixa frequência;
- sem pathfinding contínuo;
- posição aproximada/objetivo agregado.

### Vila descarregada

- nenhuma entidade física atualizada;
- produção, consumo e envelhecimento processados por eventos ou diferença de tempo quando a vila volta a ser carregada.

## Pathfinding

Não recalcular caminho completo por NPC a cada frame.

Usar:

- cache de rotas comuns dentro da vila;
- cooldown para novos paths;
- fila de pathfinding com orçamento por tick;
- navegação local barata para pequenos desvios;
- cancelamento quando objetivo deixa de ser relevante.

## Multiplayer

Servidor decide:

- posição lógica;
- profissão;
- inventário;
- relações;
- preços;
- trocas;
- nascimento/morte;
- combate.

Cliente recebe snapshots/deltas e cuida de apresentação.

## Relação com outros sistemas

Villagers devem consumir e produzir recursos do próprio jogo.

Exemplos futuros:

- agricultor vende comida para o jogador ou para a vila;
- ferreiro compra lingotes produzidos em fábrica;
- minerador pode gerar contratos por determinados minérios;
- ataques de criaturas podem reduzir população e estoque;
- máquinas podem ser vendidas, reparadas ou operadas por profissões futuras;
- bosses e estruturas podem desbloquear bens exóticos no comércio.

## Regra Low-first

No preset Low:

- reduzir distância visual de NPCs;
- reduzir frequência de animações secundárias;
- limitar quantidade de NPCs renderizados simultaneamente;
- usar LOD/modelos simples quando disponível;
- manter IA lógica correta, mas com ticks escalonados;
- entidades fora da área ativa devem entrar em modo agregado/dormant.

A jogabilidade social deve permanecer completa no Low; só a apresentação e a frequência de simulação distante podem ser reduzidas.
