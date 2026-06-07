// Lightweight per-player lobby state. Over the wire — only primitives + short faction key.
// Replicated as an element of an [RplProp] array on PS_PlayableManager (vanilla SCR_PlayerFactionInfo style).
class PS_PlayerLobbyInfo
{
	int        m_iPlayerId;
	int        m_iState;                 // PS_EPlayableControllerState (enum -> int); default meaning = NotReady
	FactionKey m_sFactionKey;
	FactionKey m_sFactionKeyRemembered;
	RplId      m_PlayableId;
	RplId      m_PlayableIdRemembered;
	bool       m_bPin;

	static bool Extract(PS_PlayerLobbyInfo instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.SerializeInt(instance.m_iPlayerId);
		snapshot.SerializeInt(instance.m_iState);
		snapshot.SerializeString(instance.m_sFactionKey);
		snapshot.SerializeString(instance.m_sFactionKeyRemembered);
		snapshot.SerializeInt(instance.m_PlayableId);
		snapshot.SerializeInt(instance.m_PlayableIdRemembered);
		snapshot.SerializeBool(instance.m_bPin);
		return true;
	}

	static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, PS_PlayerLobbyInfo instance)
	{
		snapshot.SerializeInt(instance.m_iPlayerId);
		snapshot.SerializeInt(instance.m_iState);
		snapshot.SerializeString(instance.m_sFactionKey);
		snapshot.SerializeString(instance.m_sFactionKeyRemembered);
		snapshot.SerializeInt(instance.m_PlayableId);
		snapshot.SerializeInt(instance.m_PlayableIdRemembered);
		snapshot.SerializeBool(instance.m_bPin);
		return true;
	}

	static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet)
	{
		snapshot.EncodeInt(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeBool(packet);
	}

	static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.DecodeInt(packet);
		snapshot.DecodeInt(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeInt(packet);
		snapshot.DecodeInt(packet);
		snapshot.DecodeBool(packet);
		return true;
	}

	static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs, ScriptCtx ctx)
	{
		return lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareStringSnapshots(rhs)
			&& lhs.CompareStringSnapshots(rhs)
			&& lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareSnapshots(rhs, 1);
	}

	static bool PropCompare(PS_PlayerLobbyInfo instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
		return snapshot.CompareInt(instance.m_iPlayerId)
			&& snapshot.CompareInt(instance.m_iState)
			&& snapshot.CompareString(instance.m_sFactionKey)
			&& snapshot.CompareString(instance.m_sFactionKeyRemembered)
			&& snapshot.CompareInt(instance.m_PlayableId)
			&& snapshot.CompareInt(instance.m_PlayableIdRemembered)
			&& snapshot.CompareBool(instance.m_bPin);
	}
}
