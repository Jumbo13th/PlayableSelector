// Lightweight per-faction ready value. Replicated as an element of an [RplProp] array on PS_PlayableManager.
class PS_FactionReadyInfo
{
	FactionKey m_sFactionKey;
	int        m_iReady;

	static bool Extract(PS_FactionReadyInfo instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.SerializeString(instance.m_sFactionKey);
		snapshot.SerializeInt(instance.m_iReady);
		return true;
	}

	static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, PS_FactionReadyInfo instance)
	{
		snapshot.SerializeString(instance.m_sFactionKey);
		snapshot.SerializeInt(instance.m_iReady);
		return true;
	}

	static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet)
	{
		snapshot.EncodeString(packet);
		snapshot.EncodeInt(packet);
	}

	static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.DecodeString(packet);
		snapshot.DecodeInt(packet);
		return true;
	}

	static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs, ScriptCtx ctx)
	{
		return lhs.CompareStringSnapshots(rhs)
			&& lhs.CompareSnapshots(rhs, 4);
	}

	static bool PropCompare(PS_FactionReadyInfo instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
		return snapshot.CompareString(instance.m_sFactionKey)
			&& snapshot.CompareInt(instance.m_iReady);
	}
}
