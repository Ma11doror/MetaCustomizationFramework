#pragma once


template <class ObjectType> class TCache
{
protected:
	ObjectType Value;
	bool bReady;

public:
	TCache() : Value(), bReady(false) {}

	template <typename... InArgTypes> TCache(InArgTypes&&... Args) : Value(Forward<InArgTypes>(Args)...), bReady(true) {}

	friend FArchive& operator<<(FArchive& Ar, TCache<ObjectType>& Cache)
	{
		Ar << Cache.Value;
		Ar << Cache.bReady;
		return Ar;
	}

	void operator=(const ObjectType& NewValue)
	{
		Value = NewValue;
		bReady = true;
	}

	void operator+=(const ObjectType& NewValue)
	{
		ensure(bReady);
		Value += NewValue;
	}

	ObjectType* operator->()
	{
		ensure(bReady);
		return &Value;
	}

	operator ObjectType&() { return Value; }

	const ObjectType& operator*() const { return Value; }

	ObjectType& operator*() { return Value; }

	void SetReady() { bReady = true; }

	bool IsValid() const { return bReady; }

	void Empty() { bReady = false; }
};
