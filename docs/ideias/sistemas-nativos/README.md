# Sistemas Nativos do Mie — visão de longo prazo

Esta pasta existe **somente como documentação de design e implementação futura**. Nenhum arquivo daqui deve ser incluído no build, carregado em runtime ou tratado como configuração do jogo.

## Objetivo

Transformar ideias conhecidas de grandes expansões de jogos de blocos em **sistemas nativos do próprio Mie**, integrados entre si, com identidade, nomes, conteúdo, balanceamento, código e assets próprios.

As referências de inspiração são apenas conceituais:

- sistemas sociais profundos de villagers e famílias, lembrando a função que Minecraft Comes Alive cumpre;
- ecossistema de máquinas, energia, processamento e progressão industrial, lembrando IndustrialCraft 2;
- logística por tubos, bombas e automação de recursos, lembrando BuildCraft;
- energia mecânica, eixos, engrenagens, esteiras e fábricas físicas, lembrando Create;
- grande escala de criaturas, bosses, estruturas, minérios, equipamentos e exploração, lembrando OreSpawn.

**Nada disso deve entrar no Mie como “mod”.** A intenção é que essas funções façam parte do jogo base e pareçam ter sido projetadas para o Mie desde o início.

## Princípios obrigatórios

1. **Identidade própria.** Não copiar código, nomes, modelos, texturas, sons, interfaces, criaturas, receitas ou assets das referências.
2. **Integração.** Villagers, indústria, criaturas, estruturas, exploração e progressão devem conversar entre si.
3. **Servidor autoritativo.** Em multiplayer, estado relevante de NPCs, máquinas, redes, loot e bosses deve ser decidido pelo servidor.
4. **Save persistente e versionado.** Sistemas novos precisam suportar carregamento de mundos antigos e evolução de schema.
5. **Conteúdo dirigido por dados quando fizer sentido.** Receitas, profissões, loot, variantes e estruturas devem evitar código duplicado.
6. **Low-first.** Todo recurso deve ser desenhado primeiro para funcionar no pior preset gráfico/simulacional suportado pelo Mie. Presets maiores adicionam qualidade visual, distância e densidade; nunca podem ser requisito para a mecânica existir.
7. **Sem custo invisível por frame.** Sistemas distantes, ociosos ou fora de chunks ativos não podem executar IA, pathfinding, buscas de rede ou física completa continuamente.
8. **Escalabilidade.** Uma base pequena e uma megafábrica devem usar a mesma arquitetura, com atualizações incrementais, cache e time slicing.

## Estrutura desta documentação

- `01-villagers-sociedade.md`: sistema nativo de villagers, profissões, trocas, relações e famílias.
- `02-tecnologia-industria.md`: energia, máquinas, logística, automação e mecânica.
- `03-aventura-criaturas-mundo.md`: criaturas, bosses, estruturas, recursos e futuras dimensões/áreas especiais.
- `04-arquitetura-integracao.md`: como os sistemas compartilham dados, eventos, saves e multiplayer.
- `05-performance-low-first.md`: regras obrigatórias de performance e escalabilidade.
- `06-roadmap-implementacao.md`: ordem segura de desenvolvimento.
- `07-plano-v0.6.md`: auditoria da v0.5, contratos, pacotes de trabalho e
  rastreabilidade usados para iniciar a v0.6.

## Estado de execução

A Fase 0 foi implementada na v0.6: compatibilidade, IDs estáveis, schemas de save,
scheduler, dirty flags, interesse multiplayer, profiling e uma máquina piloto
interna. O plano e o estado de validação estão em
[`07-plano-v0.6.md`](07-plano-v0.6.md). As demais mecânicas continuam organizadas
por dependência e não devem avançar antes da validação distributiva dessa fundação.

## Regra de decisão

Se uma ideia for bonita no Ultra, mas inviável no Low, ela **não está pronta**. Primeiro deve existir uma versão funcional, legível e eficiente no Low; depois vêm efeitos, partículas, animações adicionais, maior distância de simulação e densidade visual.
