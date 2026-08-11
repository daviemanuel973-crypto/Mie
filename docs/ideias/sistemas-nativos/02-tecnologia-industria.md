# 02 — Tecnologia, indústria, energia e automação

## Visão

O maquinário deve ser um **sistema tecnológico nativo do Mie**, integrado à progressão normal do jogo. A inspiração funcional pode lembrar IndustrialCraft 2, BuildCraft e Create, mas nomes, receitas, máquinas, lógica visual e balanceamento devem ser próprios.

A tecnologia deve nascer de uma infraestrutura comum para evitar três sistemas incompatíveis entre si.

## Camadas principais

1. máquinas;
2. energia elétrica;
3. redes de cabos;
4. transporte de itens;
5. fluidos;
6. energia mecânica;
7. automação e controle;
8. estruturas/contraptions móveis somente após a base estar madura.

## Interface genérica de máquina

Toda máquina futura deve reutilizar um contrato comum, por exemplo conceitualmente:

- inventário de entrada;
- inventário de saída;
- buffer de energia;
- consumo por operação;
- receita/processo atual;
- progresso;
- estado ativo/inativo/bloqueado;
- sides/ports configuráveis;
- serialização compacta;
- dirty flag para rede e save.

Máquinas não devem executar lógica completa todo frame. Idealmente trabalham em ticks ou por eventos.

## Receitas de processamento

Receitas devem ser orientadas por dados sempre que possível:

- itens de entrada;
- quantidade;
- energia necessária;
- tempo;
- saídas;
- subprodutos;
- máquina compatível;
- requisitos tecnológicos.

Isso permite criar conteúdo sem duplicar código.

## Primeiras máquinas

Conjunto inicial sugerido:

- gerador simples;
- bateria/acumulador;
- forno elétrico;
- triturador;
- prensa;
- serraria;
- bomba;
- tanque;
- mineradora automatizada de pequena escala;
- bancada/montadora industrial.

A mineradora automática deve ser balanceada por energia, velocidade, alcance e manutenção para não invalidar mineração manual cedo demais.

## Energia elétrica

Criar uma unidade própria do Mie, sem copiar nomenclatura de outras franquias.

Cada rede elétrica deve possuir:

- produtores;
- consumidores;
- armazenamento;
- capacidade de transferência;
- perdas opcionais somente se trouxerem gameplay real;
- prioridade/configuração futura.

### Regra crítica de performance

**Nunca percorrer toda a rede elétrica a cada frame.**

Usar componentes conectados com cache. A topologia só é recalculada quando:

- cabo é colocado/removido;
- máquina conecta/desconecta;
- chunk entra/sai de estado relevante;
- configuração de porta muda.

Distribuição de energia acontece por tick e pode trabalhar com valores agregados da rede.

## Cabos

Cabos devem funcionar como blocos conectáveis visualmente.

O renderer pode calcular máscara de conexões por vizinhança quando o bloco muda, não continuamente.

No Low:

- geometria simplificada;
- menos efeitos luminosos;
- sem partículas permanentes;
- nenhuma diferença de funcionalidade.

## Transporte de itens

Inspirado na função logística de tubos, mas implementado de forma própria.

Elementos futuros:

- tubo básico;
- entrada/extrator;
- saída;
- filtro;
- divisor/prioridade;
- tubo rápido;
- interface com baús e máquinas.

### Implementação eficiente

Evitar criar uma entidade física completa por item dentro de tubos.

Representar carga em trânsito como pacote lógico compacto:

- item;
- quantidade;
- origem;
- destino/rota ou próximo nó;
- progresso normalizado.

O cliente pode interpolar um modelo visual sem física real.

Rotas devem ser cacheadas e invalidadas apenas quando a rede muda.

## Fluidos

Tratar fluido industrial como quantidade numérica em tanques e segmentos lógicos, não como voxels fluidos simulados dentro dos tubos.

Rede de fluidos compartilha princípios de cache/topologia com energia.

Possíveis fluidos:

- água;
- combustível refinado futuro;
- óleo/recurso próprio do Mie futuro;
- vapor futuramente.

## Energia mecânica

Sistema próprio baseado em:

- fonte de rotação;
- eixo;
- engrenagem;
- caixa de transmissão;
- correia;
- polia;
- roda d'água;
- moinho;
- motor elétrico;
- máquinas mecânicas.

A simulação deve ser **de rede**, não física rígida por dente de engrenagem.

Cada rede pode calcular:

- velocidade angular lógica;
- capacidade/torque disponível;
- carga requerida;
- direção.

Animação visual deriva desses valores.

## Esteiras

Esteiras devem mover itens e talvez entidades, mas o conteúdo sobre elas deve ser gerenciado de forma agregada quando possível.

Para itens:

- slots/posições lógicas ao longo da correia;
- atualização em tick fixo;
- interpolação cliente.

## Contraptions e estruturas móveis

Só implementar depois de redes, máquinas e persistência estarem maduras.

Estratégia sugerida:

- converter conjunto de blocos conectado em uma estrutura lógica temporária;
- retirar/mascarar blocos do grid estático enquanto móvel;
- armazenar transforms locais;
- usar uma colisão simplificada ou conjunto limitado de colliders;
- sincronizar transform raiz em multiplayer;
- restaurar voxels quando a estrutura para.

Nunca começar simulando cada bloco como rigid body individual.

## Automação e controle

Depois da logística básica:

- sensores;
- filtros;
- comparadores lógicos próprios;
- temporizadores;
- portas controláveis;
- braços mecânicos;
- prioridades;
- gatilhos por estoque/energia.

A lógica deve ser baseada em eventos e estado sujo, não em polling global por frame.

## Progressão

Tecnologia deve conversar com mineração e exploração.

Exemplo:

`minério bruto -> triturador -> concentrado -> forno -> lingote -> peças -> máquina`

Bosses/estruturas podem fornecer materiais especiais para tiers avançados sem bloquear o começo da automação.

## Multiplayer

Servidor autoritativo para:

- inventários;
- energia;
- processamento;
- rotas;
- fluidos;
- redes mecânicas;
- contraptions.

Clientes recebem estados compactos e interpolam animação.

## Regra Low-first

Uma megafábrica precisa continuar jogável no Low.

Obrigatório:

- tick rate adaptativo;
- redes cacheadas;
- atualizações incrementais;
- batching de meshes repetitivas;
- culling de máquinas ocultas/distantes;
- partículas opcionais;
- animação reduzida fora de foco;
- limite de trabalho de simulação por frame;
- nunca usar física individual para milhares de itens de logística.

O preset Ultra pode aumentar efeitos e fidelidade, não a quantidade necessária de cálculos para a lógica principal funcionar.
