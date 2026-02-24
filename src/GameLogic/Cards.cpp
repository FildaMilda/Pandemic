#include "Cards.h"

std::array<CardData, 64> CardRegistry::registry;

const void CardRegistry::DebugPrint()
{
	for (int i = 0; i < registrySize; i++) {
		std::cout << registry[i].name << "\n";
	}
}
