#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <native/contentRegistry.h>

namespace mie::native
{
	struct ProcessingStack
	{
		ContentKind kind = ContentKind::Item;
		std::string stableKey;
		std::uint32_t count = 1;
	};

	struct ProcessingRecipe
	{
		std::string stableKey;
		std::string machineKey;
		std::vector<ProcessingStack> inputs;
		std::vector<ProcessingStack> outputs;
		std::uint32_t durationTicks = 0;
	};

	class ProcessingRecipeRegistry
	{
	public:
		bool registerRecipe(ProcessingRecipe recipe, const ContentRegistry &content,
			std::string *error = nullptr);
		const ProcessingRecipe *find(const std::string &stableKey) const;
		const std::vector<ProcessingRecipe> &entries() const { return recipes; }
		std::size_t size() const { return recipes.size(); }

	private:
		std::vector<ProcessingRecipe> recipes;
		std::unordered_map<std::string, std::size_t> keyToIndex;
	};

	ProcessingRecipeRegistry createV07ProcessingRecipeRegistry(const ContentRegistry &content);
}
