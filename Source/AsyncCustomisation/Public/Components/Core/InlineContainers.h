#pragma once

#include "CoreMinimal.h"

namespace Util
{
	template<class ValueType, uint32 ElementsOnStack>
	using TInlineArray = TArray<ValueType, TInlineAllocator<ElementsOnStack>>;

	template<class ValueType, uint32 ElementsOnStack>
	using TFixedArray = TArray<ValueType, TFixedAllocator<ElementsOnStack>>;
	
	template<class ValueType, uint32 ElementsOnStack>
	using TInlineSet = TSet<ValueType, DefaultKeyFuncs<ValueType>, TInlineSetAllocator<ElementsOnStack>>;
	
	template<class ValueType, uint32 ElementsOnStack>
	using TFixedSet = TSet<ValueType, DefaultKeyFuncs<ValueType>, TFixedSetAllocator<ElementsOnStack>>;

	template<class KeyType, class ValueType, uint32 ElementsOnStack>
	using TInlineMap = TMap<KeyType, ValueType, TInlineSetAllocator<ElementsOnStack>>;
	
	template<class KeyType, class ValueType, uint32 ElementsOnStack>
	using TFixedMap = TMap<KeyType, ValueType, TFixedSetAllocator<ElementsOnStack>>;
}
