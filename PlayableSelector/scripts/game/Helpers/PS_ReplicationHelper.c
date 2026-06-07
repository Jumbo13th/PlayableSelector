class PS_ReplicationHelper
{
	// I want template functions, this seems ugly :(
	// Read* methods return bool (fail-fast): every Read is checked, a bad count or short read
	// returns false so RplLoad can propagate a clean failure instead of silently loading garbage.
	// Mirrors vanilla SCR_BaseScoringSystemComponent.RplLoad.
	static void WriteMapIntInt(ScriptBitWriter writer, map<int, int> mapToWrite)
	{
		int count = mapToWrite.Count();
		writer.WriteInt(count);
		for (int i = 0; i < count; i++)
		{
			writer.WriteInt(mapToWrite.GetKey(i));
			writer.WriteInt(mapToWrite.GetElement(i));
		}
	}
	static bool ReadMapIntInt(ScriptBitReader reader, map<int, int> mapToWrite)
	{
		int count;
		if (!reader.ReadInt(count)) return false;
		for (int i = 0; i < count; i++)
		{
			int key;
			int value;
			if (!reader.ReadInt(key)) return false;
			if (!reader.ReadInt(value)) return false;

			mapToWrite.Insert(key, value);
		}
		return true;
	}

	static void WriteMapRplIdInt(ScriptBitWriter writer, map<RplId, int> mapToWrite)
	{
		int count = mapToWrite.Count();
		writer.WriteInt(count);
		for (int i = 0; i < count; i++)
		{
			writer.WriteInt(mapToWrite.GetKey(i));
			writer.WriteInt(mapToWrite.GetElement(i));
		}
	}
	static bool ReadMapRplIdInt(ScriptBitReader reader, map<RplId, int> mapToWrite)
	{
		int count;
		if (!reader.ReadInt(count)) return false;
		for (int i = 0; i < count; i++)
		{
			RplId key;
			int value;
			if (!reader.ReadInt(key)) return false;
			if (!reader.ReadInt(value)) return false;

			mapToWrite.Insert(key, value);
		}
		return true;
	}

	static void WriteMapIntRplId(ScriptBitWriter writer, map<int, RplId> mapToWrite)
	{
		int count = mapToWrite.Count();
		writer.WriteInt(count);
		for (int i = 0; i < count; i++)
		{
			writer.WriteInt(mapToWrite.GetKey(i));
			writer.WriteInt(mapToWrite.GetElement(i));
		}
	}
	static bool ReadMapIntRplId(ScriptBitReader reader, map<int, RplId> mapToWrite)
	{
		int count;
		if (!reader.ReadInt(count)) return false;
		for (int i = 0; i < count; i++)
		{
			int key;
			RplId value;
			if (!reader.ReadInt(key)) return false;
			if (!reader.ReadInt(value)) return false;

			mapToWrite.Insert(key, value);
		}
		return true;
	}

	static void WriteMapIntBool(ScriptBitWriter writer, map<int, bool> mapToWrite)
	{
		int count = mapToWrite.Count();
		writer.WriteInt(count);
		for (int i = 0; i < count; i++)
		{
			writer.WriteInt(mapToWrite.GetKey(i));
			writer.WriteBool(mapToWrite.GetElement(i));
		}
	}
	static bool ReadMapIntBool(ScriptBitReader reader, map<int, bool> mapToWrite)
	{
		int count;
		if (!reader.ReadInt(count)) return false;
		for (int i = 0; i < count; i++)
		{
			int key;
			bool value;
			if (!reader.ReadInt(key)) return false;
			if (!reader.ReadBool(value)) return false;

			mapToWrite.Insert(key, value);
		}
		return true;
	}

	static void WriteMapIntString(ScriptBitWriter writer, map<int, string> mapToWrite)
	{
		int count = mapToWrite.Count();
		writer.WriteInt(count);
		for (int i = 0; i < count; i++)
		{
			writer.WriteInt(mapToWrite.GetKey(i));
			writer.WriteString(mapToWrite.GetElement(i));
		}
	}
	static bool ReadMapIntString(ScriptBitReader reader, map<int, string> mapToWrite)
	{
		int count;
		if (!reader.ReadInt(count)) return false;
		for (int i = 0; i < count; i++)
		{
			int key;
			string value;
			if (!reader.ReadInt(key)) return false;
			if (!reader.ReadString(value)) return false;

			mapToWrite.Insert(key, value);
		}
		return true;
	}

	static void WriteMapIntFactionKey(ScriptBitWriter writer, map<int, FactionKey> mapToWrite)
	{
		int count = mapToWrite.Count();
		writer.WriteInt(count);
		for (int i = 0; i < count; i++)
		{
			writer.WriteInt(mapToWrite.GetKey(i));
			writer.WriteString(mapToWrite.GetElement(i));
		}
	}
	static bool ReadMapIntFactionKey(ScriptBitReader reader, map<int, FactionKey> mapToWrite)
	{
		int count;
		if (!reader.ReadInt(count)) return false;
		for (int i = 0; i < count; i++)
		{
			int key;
			FactionKey value;
			if (!reader.ReadInt(key)) return false;
			if (!reader.ReadString(value)) return false;

			mapToWrite.Insert(key, value);
		}
		return true;
	}

	static void WriteMapFactionKeyInt(ScriptBitWriter writer, map<FactionKey, int> mapToWrite)
	{
		int count = mapToWrite.Count();
		writer.WriteInt(count);
		for (int i = 0; i < count; i++)
		{
			writer.WriteString(mapToWrite.GetKey(i));
			writer.WriteInt(mapToWrite.GetElement(i));
		}
	}
	static bool ReadMapFactionKeyInt(ScriptBitReader reader, map<FactionKey, int> mapToWrite)
	{
		int count;
		if (!reader.ReadInt(count)) return false;
		for (int i = 0; i < count; i++)
		{
			FactionKey key;
			int value;
			if (!reader.ReadString(key)) return false;
			if (!reader.ReadInt(value)) return false;

			mapToWrite.Insert(key, value);
		}
		return true;
	}

	static void WriteMapRplIdString(ScriptBitWriter writer, map<RplId, string> mapToWrite)
	{
		int count = mapToWrite.Count();
		writer.WriteInt(count);
		for (int i = 0; i < count; i++)
		{
			writer.WriteInt(mapToWrite.GetKey(i));
			writer.WriteString(mapToWrite.GetElement(i));
		}
	}
	static bool ReadMapRplIdString(ScriptBitReader reader, map<RplId, string> mapToWrite)
	{
		int count;
		if (!reader.ReadInt(count)) return false;
		for (int i = 0; i < count; i++)
		{
			RplId key;
			string value;
			if (!reader.ReadInt(key)) return false;
			if (!reader.ReadString(value)) return false;

			mapToWrite.Insert(key, value);
		}
		return true;
	}
}
