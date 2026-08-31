# Mie Survival — sistema de skins v0.9.6

## Formato canônico: Mie Skin v1

O Mie mantém internamente um atlas **RGBA 128×128** para preservar o renderer, o multiplayer e o caminho de compatibilidade do Flatpak já existentes.

O desenho/UV do atlas segue o mesmo layout lógico das skins modernas **Minecraft Java 64×64**. Isso permite reutilizar skins do Minecraft sem inventar um segundo mapeamento corporal.

Formatos aceitos pelo carregador da v0.9.6:

- `128×128` — Mie Skin v1, carregada diretamente;
- `64×64` — skin moderna do Minecraft, ampliada 2× por nearest-neighbour para 128×128;
- `64×32` — skin legada do Minecraft, adaptada para o layout quadrado e depois ampliada;
- `128×64` — skin Mie legada, adaptada para o atlas 128×128.

Qualquer outra dimensão é rejeitada e o jogo volta para a skin padrão embutida.

## Proporções do corpo

A v0.9.6 usa a geometria **Classic** do jogador (braços de quatro pixels no layout lógico do Minecraft). O editor permite marcar uma skin como referência Slim, mas o renderer desta versão ainda a exibe sobre a geometria Classic. O PNG não é destruído ou recortado; suporte geométrico Slim real pode entrar numa versão posterior sem quebrar o formato Mie Skin v1.

## Skin padrão

Se nenhuma skin customizada estiver selecionada, ou se o arquivo customizado for inválido, o cliente usa a textura padrão empacotada em `resources/assets/models/steve.png`.

## Pasta de skins

O seletor existente `Change Skin` procura arquivos `.png` em:

- Windows portátil: `resources/skins/` ao lado do executável;
- build de desenvolvimento: `resources/skins/` do checkout;
- Flatpak: a pasta gravável de conteúdo do usuário criada pelo launcher.

Arquivos pessoais dessa pasta não devem ser incluídos em builds/releases.

## Editor incluído

A v0.9.6 inclui `resources/tools/MieSkinEditor.html`.

Ele funciona offline no navegador e oferece:

- lápis, borracha, balde e conta-gotas;
- cor RGBA/transparência;
- undo/redo;
- zoom, grade e guias do atlas;
- importação de 64×64, 64×32, 128×128 e 128×64;
- exportação PNG 64×64 compatível com Minecraft e importável pelo Mie.

Fluxo recomendado:

1. no menu principal, abra `Change Skin` e clique em `Open Skin Editor`;
2. crie uma skin ou importe uma skin do Minecraft;
3. exporte `mie-skin.png`;
4. volte ao Mie, clique em `Open Skins Folder` e coloque o PNG nessa pasta;
5. retorne ao seletor e escolha o arquivo;
6. a escolha fica salva para as próximas execuções; ao entrar em um mundo, o cliente normaliza a textura para 128×128 e a envia pelo sistema de skins multiplayer já existente.

Se o ambiente gráfico bloquear a abertura automática, o menu informa o caminho alternativo e o editor continua disponível em `resources/tools/MieSkinEditor.html`.

## Regras de compatibilidade

- O formato de rede continua transportando uma textura final 128×128; clientes/servidores não precisam negociar um novo tamanho.
- A conversão moderna 64×64 → 128×128 é determinística e sem filtragem, preservando pixel art.
- Skins legadas 64×32 não possuem braço/perna esquerdos independentes; a v0.9.6 gera essas regiões de forma determinística a partir dos membros disponíveis.
- O carregador valida dimensão antes de criar a textura OpenGL.
- Nenhuma skin customizada deve ser empacotada automaticamente nos artefatos de release.
