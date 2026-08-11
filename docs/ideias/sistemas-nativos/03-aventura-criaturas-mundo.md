# 03 — Aventura, criaturas, bosses, estruturas e mundo

## Visão

O Mie deve possuir uma camada de aventura própria e expansível: novas criaturas, variantes, bosses, estruturas, recursos raros e, no futuro, regiões ou dimensões especiais.

A referência de escala pode lembrar OreSpawn, mas **nenhuma criatura, nome, modelo, item, estrutura ou progressão deve ser copiada**. O objetivo é obter a sensação de descoberta e abundância de conteúdo usando identidade própria.

## Filosofia

O mundo deve oferecer motivos para sair da base.

O ciclo desejado:

`explorar -> descobrir -> combater/resolver -> obter recurso único -> desbloquear nova possibilidade -> explorar mais longe`

Nem toda descoberta deve ser combate. Algumas podem ser:

- estruturas abandonadas;
- ruínas;
- aldeias;
- recursos raros;
- eventos ambientais;
- puzzles simples;
- pontos de mineração;
- criaturas passivas raras;
- comerciantes especiais.

## Framework de criaturas

Antes de dezenas de mobs, criar um framework orientado por componentes/variantes que permita compartilhar:

- vida;
- armadura;
- movimento;
- percepção;
- agressividade;
- faction/tag;
- ataques;
- loot;
- resistências;
- animação;
- spawn rules;
- variante visual.

Evitar uma classe gigante e única para cada criatura se o comportamento puder ser composto.

## Categorias sugeridas

### Fauna

- passiva;
- territorial;
- domesticável futuramente;
- predador natural.

### Hostis comuns

- inimigos de superfície;
- noturnos;
- cavernas;
- biomas específicos;
- grupos/facções.

### Elites

Versões incomuns com:

- mais vida;
- comportamento adicional;
- loot melhor;
- sinal visual simples.

### Minibosses

Encontrados em estruturas ou eventos específicos.

### Bosses

Encontros desenhados, não apenas inimigos com muita vida.

## Variantes

Variantes devem preferir reutilizar modelo/esqueleto quando possível.

Podem alterar:

- textura/paleta;
- escala limitada;
- velocidade;
- vida;
- ataque;
- loot;
- comportamento;
- região de spawn.

Isso multiplica conteúdo sem multiplicar proporcionalmente memória e draw calls.

## Boss framework

Boss futuro deve possuir:

- fases explícitas;
- telegraph de ataques;
- cooldowns;
- arena ou espaço mínimo;
- regras de reset;
- multiplayer scaling controlado;
- loot único;
- persistência de estado somente quando necessário.

### IA de boss

Usar máquina de estados ou árvore de comportamento compacta.

Não executar decisões caras por frame. Ataques podem ser agendados por timers/eventos e movimento pode reutilizar sistemas comuns.

## Estruturas procedurais

Criar estrutura em camadas:

1. marcador/regra de geração;
2. blueprint/template;
3. variações de peças;
4. loot table;
5. pontos de spawn;
6. metadados especiais.

Primeiras estruturas úteis:

- cabana abandonada;
- acampamento hostil;
- cemitério;
- ruína;
- mina abandonada;
- torre pequena;
- fortificação;
- laboratório/oficina antiga futuramente;
- dungeon de boss.

## Geração eficiente

Estruturas não devem fazer buscas globais ou tentativas caras repetidas por chunk.

Usar seed determinística e regiões de geração. A decisão “existe estrutura aqui?” deve ser barata e reproduzível sem precisar gerar todos os chunks vizinhos.

Para estruturas grandes:

- registrar um anchor/região;
- gerar partes conforme chunks entram;
- impedir duplicação por identificador determinístico.

## Recursos e minérios

Novos minérios só devem existir quando tiverem papel claro em progressão.

Categorias possíveis:

- construção;
- ferramentas/armas;
- tecnologia;
- alquimia;
- boss/endgame.

Evitar adicionar dez minérios equivalentes que apenas aumentem números.

## Loot e raridade

Criar tabelas reutilizáveis com:

- peso/chance;
- quantidade;
- tier;
- condições;
- região;
- dificuldade;
- fonte.

Possível classificação visual própria:

- comum;
- incomum;
- raro;
- excepcional;
- relíquia.

A raridade deve ter função real, não apenas cor.

## Itens únicos

Bosses e estruturas podem liberar:

- componentes tecnológicos;
- materiais especiais;
- armas com comportamento próprio;
- cosméticos;
- mapas/chaves;
- itens sociais para villagers;
- recursos para novas áreas.

Assim aventura, indústria e sociedade formam um único jogo.

## Regiões e futuras dimensões

Não começar por dimensões completas.

Ordem recomendada:

1. biomas/regiões mais distintos no mundo atual;
2. estruturas e eventos regionais;
3. áreas instanciadas ou regiões especiais se necessário;
4. somente depois, mundos/dimensões separados.

Uma dimensão futura deve ter:

- identificador próprio;
- regras de geração;
- save separado ou namespace no save;
- portal/transição;
- spawn rules;
- iluminação/ambiente;
- recursos exclusivos;
- estratégia clara de unload.

## Spawn system

Spawn deve ser controlado por orçamento, não por tentativas ilimitadas.

Considerar:

- cap global do servidor;
- cap por região/chunk ativo;
- cap por categoria;
- distância dos jogadores;
- densidade do bioma;
- horário/evento;
- dificuldade.

Entidades muito distantes entram em dormant ou são removidas quando não precisam de persistência.

## Multiplayer

Servidor decide:

- spawn;
- comportamento;
- dano;
- fases de boss;
- loot;
- estrutura válida;
- persistência.

Clientes não devem gerar independentemente resultados que afetem gameplay.

## Regra Low-first

No Low:

- reduzir partículas e efeitos de boss;
- simplificar sombras e materiais;
- limitar criaturas renderizadas à distância;
- usar LOD/culling;
- IA distante roda em frequência reduzida;
- animação distante pode ser simplificada;
- efeitos de área usam representação lógica barata;
- estruturas devem usar os mesmos blocos/batching do mundo sempre que possível.

Nunca desenhar um boss cujo ataque só seja compreensível com partículas Ultra. Telegraphs precisam permanecer claros no Low.
