#!/usr/bin/env python3
from pathlib import Path


def replace_once(path: str, old: str, new: str, label: str) -> None:
    p = Path(path)
    text = p.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match, found {count}")
    p.write_text(text.replace(old, new, 1), encoding="utf-8")


# Keep the compatibility decoder declaration public, but do not redefine a stb
# symbol in a widely included header. That could redirect unrelated image loads.
replace_once(
    "include/gameLayer/rendering/model.h",
    '''// v0.9.6 keeps the established 128x128 renderer/network contract while the
// decoder accepts Mie and Minecraft PNG layouts. model.cpp is the only legacy
// translation unit that decodes player skins through stb_image, so redirect
// that call to the compatibility adapter without duplicating the renderer.
unsigned char *mieLoadPlayerSkinImageFromMemory(const unsigned char *buffer, int len,
\tint *x, int *y, int *channelsInFile, int desiredChannels);
#define stbi_load_from_memory mieLoadPlayerSkinImageFromMemory
''',
    '''// v0.9.6 keeps the established 128x128 renderer/network contract while the
// player-skin decoder accepts Mie and Minecraft PNG layouts. Keep this adapter
// explicit so unrelated stb_image users are never redirected by a global macro.
unsigned char *mieLoadPlayerSkinImageFromMemory(const unsigned char *buffer, int len,
\tint *x, int *y, int *channelsInFile, int desiredChannels);
''',
    "remove global stb macro",
)

replace_once(
    "src/gameLayer/rendering/model.cpp",
    '''\t\tconst unsigned char *decodedImage = stbi_load_from_memory(fileData, (int)fileSize, &width, &height, &channels, 4);''',
    '''\t\tunsigned char *decodedImage = mieLoadPlayerSkinImageFromMemory(fileData, (int)fileSize, &width, &height, &channels, 4);''',
    "scope skin decoder to loadPlayerSkin",
)

# Preflight PNG dimensions before full decode. Only the four supported atlas
# sizes can ever be valid skins, so this also blocks oversized/decompression
# bomb inputs from reaching the expensive pixel allocation path.
replace_once(
    "src/gameLayer/gameplay/skinImageDecode.cpp",
    '''\tint sourceWidth = 0;
\tint sourceHeight = 0;
\tint sourceChannels = 0;
\tstbi_set_flip_vertically_on_load(false);
\tunsigned char *decoded = stbi_load_from_memory(buffer, len, &sourceWidth,
\t\t&sourceHeight, &sourceChannels, 4);
\tif (!decoded) { return nullptr; }
''',
    '''\tint sourceWidth = 0;
\tint sourceHeight = 0;
\tint sourceChannels = 0;
\tif (!stbi_info_from_memory(buffer, len, &sourceWidth, &sourceHeight, &sourceChannels) ||
\t\tmie::skins::detectSourceFormat(sourceWidth, sourceHeight) == mie::skins::SourceFormat::Invalid)
\t{
\t\treturn nullptr;
\t}

\tstbi_set_flip_vertically_on_load(false);
\tunsigned char *decoded = stbi_load_from_memory(buffer, len, &sourceWidth,
\t\t&sourceHeight, &sourceChannels, 4);
\t// loadPlayerSkin historically requests bottom-up data. Restore that global
\t// stb preference after our top-down decode; the normalized atlas is flipped
\t// explicitly below.
\tstbi_set_flip_vertically_on_load(true);
\tif (!decoded) { return nullptr; }
''',
    "preflight skin PNG dimensions",
)

print("v0.9.6 skin decoder scope fix applied")
# Touch marker: workflow created after the first script commit.
