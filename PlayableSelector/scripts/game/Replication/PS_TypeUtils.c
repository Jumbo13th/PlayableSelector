class PS_SizeOf<Class T>
{
	private static int SizeOfImpl(int value)
	{
		return 4;
	}

	private static int SizeOfImpl(float value)
	{
		return 4;
	}

	private static int SizeOfImpl(bool value)
	{
		return 1;
	}

	private static int SizeOfImpl(vector value)
	{
		return 12;
	}

	private static int SizeOfImpl(string value)
	{
		return value.Length() + 1;
	}

	private static int SizeOfImpl(Managed value)
	{
		if (!value)
			return 0;
		return value.GetSizeOf();
	}

	static int Get(T value)
	{
		return SizeOfImpl(value);
	}
}

class PS_Serializer<Class TSerializer, Class T>
{
	private static void SerializeImpl(inout TSerializer serializer, inout int value)
	{
		serializer.SerializeInt(value);
	}

	private static void SerializeImpl(inout TSerializer serializer, inout float value)
	{
		serializer.SerializeFloat(value);
	}

	private static void SerializeImpl(inout TSerializer serializer, inout bool value)
	{
		serializer.SerializeBool(value);
	}

	private static void SerializeImpl(inout TSerializer serializer, inout vector value)
	{
		serializer.SerializeVector(value);
	}

	private static void SerializeImpl(inout TSerializer serializer, inout string value)
	{
		serializer.SerializeString(value);
	}

	static void Serialize(inout TSerializer serializer, inout T value)
	{
		SerializeImpl(serializer, value);
	}
}

class PS_Comparator<Class T>
{
	private static bool CompareSnapImpl(inout SSnapSerializerBase lhs, inout SSnapSerializerBase rhs, int anyVal, int sizeOfAnyVal)
	{
		return lhs.CompareSnapshots(rhs, sizeOfAnyVal);
	}

	private static bool CompareSnapImpl(inout SSnapSerializerBase lhs, inout SSnapSerializerBase rhs, float anyVal, int sizeOfAnyVal)
	{
		return lhs.CompareSnapshots(rhs, sizeOfAnyVal);
	}

	private static bool CompareSnapImpl(inout SSnapSerializerBase lhs, inout SSnapSerializerBase rhs, bool anyVal, int sizeOfAnyVal)
	{
		return lhs.CompareSnapshots(rhs, sizeOfAnyVal);
	}

	private static bool CompareSnapImpl(inout SSnapSerializerBase lhs, inout SSnapSerializerBase rhs, vector anyVal, int sizeOfAnyVal)
	{
		return lhs.CompareSnapshots(rhs, sizeOfAnyVal);
	}

	private static bool CompareSnapImpl(inout SSnapSerializerBase lhs, inout SSnapSerializerBase rhs, string anyVal, int sizeOfAnyVal)
	{
		return lhs.CompareStringSnapshots(rhs);
	}

	static bool CompareSnap(inout SSnapSerializerBase lhs, inout SSnapSerializerBase rhs, T anyVal)
	{
		int sizeOf = PS_SizeOf<T>.Get(anyVal);
		return CompareSnapImpl(lhs, rhs, anyVal, sizeOf);
	}
}
