#include <gameplay/items.h>


const char *itemsDescriptions[] =
{
	"Sturdy. Surprisingly versatile.",                             // stick
	"Soft. Could be a bandage... or fashion.",                     // cloth
	"Sharp. Someone lost a tooth.",                                // fang
	"Brittle. Still has some fight left in it.",                   // bone

	"Shiny and slightly warm.",                                    // copperIngot
	"Heavy. Don’t lick it.",                                       // leadIngot
	"Reliable. For serious crafting.",                             // ironIngot
	"Classy and conductive.",                                      // silverIngot
	"Fancy and overpriced.",                                       // goldIngot

	"Gets the job done.",                                          // copper pickaxe
	"It chops.",                                                   // copper axe
	"Dirt beware.",                                                // copper shovel
	"Heavy hitter.",                                               // lead pickaxe
	"Trees tremble.",                                              // lead axe
	"Might dig more than holes.",                                  // lead shovel
	"Classic miner’s choice.",                                     // iron pickaxe
	"Cuts like a dream.",                                          // iron axe
	"Efficient and stylish.",                                      // iron shovel
	"Almost too pretty to use.",                                   // silver pickaxe
	"Sparkles while chopping.",                                    // silver axe
	"Dig with elegance.",                                          // silver shovel
	"Digs fast, breaks faster.",                                   // gold pickaxe
	"Swing with style.",                                           // gold axe
	"For luxurious holes.",                                        // gold shovel

	"Won’t impress goblins.",                                      // copper sword
	"Packs a punch.",                                              // lead sword
	"Trusty and true.",                                            // iron sword
	"Elegant and sharp.",                                          // silver sword
	"Flashy, but flimsy.",                                         // gold sword

	"For safe training (probably).",                               // trainingScythe
	"For practice, not pride.",                                    // trainingSword
	"Massive but harmless.",                                       // trainingWarHammer
	"Pointy, but padded.",                                         // trainingSpear
	"Won’t cut deep—hopefully.",                                   // trainingKnife
	"All bark, no bite.",                                          // trainingBattleAxe

	"", "", "", "", // copper weapons
	"", "", "", "", // lead weapons
	"", "", "", "", // iron weapons
	"", "", "", "", // silver weapons
	"", "", "", "", // gold weapons

	"Hope you brought a sword.",                                   // zombie spawn egg
	"Oinks included.",                                             // pig spawn egg
	"May ignore you.",                                             // cat spawn egg
	"Good luck.",                                                  // goblin spawn egg
	"",                                                            // scare crow

	"Keeps you going.",                                            // apple
	"A bit tart.",                                                 // blackBerrie
	"Nature’s candy.",                                             // blueBerrie
	"Double the fun.",                                             // cherries
	"Why did you eat that?",                                       // chilliPepper
	"Shake it first.",                                             // cocconut
	"Don’t slip on them.",                                         // grapes
	"Pucker up.",                                                  // lime
	"Soft and sweet.",                                             // peach
	"Spiky outside, sweet inside.",                                // pinapple
	"Smells like summer",                                         // strawberry
	"Restores decent health",                                      // apple pie

	"Better than barefoot.", "Not just for looks.", "Keeps your head warm.",
	"Clink with every step.", "Slightly protective, very loud.", "Stylish head protection.",
	"Don’t try to swim.", "Heavy-duty defense.", "Thicc hat energy.",
	"For the serious adventurer.", "Reliable bodyguard.", "Keeps brain safe-ish.",
	"Fashion meets function.", "Gleams in sunlight.", "Shiny and snug.",
	"Impractical, but fabulous.", "Flex with protection.", "Crown-adjacent.",

	"Used to wash paint away from blocks.",
	"Blank canvas starter.", "Moody but soft.", "Serious and stormy.", "Embrace the void.",
	"Dirt, but artsy.", "Danger? Passion? You decide.", "Zesty and bold.", "Sunshine in a can.",
	"Loud and proud.", "Nature vibes.", "Somewhere between cool and cooler.",
	"Fresh like ocean breeze.", "Classic and calming.", "Royal pick.", "Hot and dramatic.",
	"Loudly lovely.",

	"Don’t spend it all at once.", "Feels richer already.", "Now you're getting somewhere.",
	"You're filthy rich.",

	"Basic but useful", "What could go wrong?", "Pointy and petty", "Creepy but effective",
	"Used for baking stuff",

	"heals you", "restores your mana",
	"", "", "", "", "Don't drink it lol", "", "", "", "", "", "",
	"Mele damage inflics poison on enemies", "Don't drink it lol",

	"Reduces the satiety effect by 5 secconds",
	"Starts healing faster",
	"All fruit good effects are longer",
	"Stealthy like a cat",
	"ALl healing items will heal a little more",

	// v0.7 survival progression. Keep these aligned with IDs 2184-2192.
	"A practical guide to survival, sieges, charcoal, bronze and village life.",
	"Fuel made from wood. Used for smelting and early metallurgy.",
	"Tin-bearing concentrate prepared from cassiterite for refining.",
	"Refined tin used with copper to make bronze.",
	"The first durable alloy of Mie's early metal age.",
	"A bronze pickaxe with 40% mining power.",
	"A bronze axe with 40% chopping power.",
	"A bronze shovel with 40% digging power.",
	"A balanced early-metal sword dealing 10 base damage.",

	// v0.9 home/subsistence content. Append-only after the shipped v0.7 range.
	"A portable resting place that marks a safe home and respawn point.", // bedroll (2193)

	// v0.10 subsistence loop. These descriptions follow the persisted item IDs.
	"A crisp root crop that can be eaten or cooked.",                    // carrot (2194)
	"A sturdy root crop. Better after cooking.",                         // potato (2195)
	"Warm, filling and ready for the trail.",                            // baked potato (2196)
	"A hearty mix of roots and grain.",                                 // vegetable stew (2197)
	"A sweet bowl of berries and grain."                                 // berry porridge (2198)
};

std::string Item::getItemDescription()
{
	static_assert(sizeof(itemsDescriptions) / sizeof(itemsDescriptions[0]) == lastItem - ItemsStartPoint, "item descriptions must match the persisted item range");

	if (isItem(type))
	{
		return itemsDescriptions[type - ItemsStartPoint];
	}
	return "";
}
