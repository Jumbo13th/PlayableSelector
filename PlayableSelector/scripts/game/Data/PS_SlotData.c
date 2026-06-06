// PS_SlotData — Lightweight data struct representing one playable character slot.
// Used as RPC argument type and for RplSave/RplLoad serialization.
// NOT a [RplProp()] — collections of these are managed by PS_LobbyManager.
// Codec pattern follows SCR_MapMarkerBase (vanilla).

class PS_SlotData
{
	// --- Fields ---

	// RplId of the character entity, serialized as int for network transport.
	int m_iRplId;

	// Display name of the slot (e.g. "Rifleman", "Team Leader").
	// Extracted from character prefab at registration time, then never changes.
	string m_sName;

	// Faction key this slot belongs to (e.g. "US", "USSR").
	string m_sFactionKey;

	// Player ID currently occupying this slot. -1 means empty.
	int m_iPlayerId;

	// Group ID this slot belongs to. -1 means no group.
	// WHY int: stored as key for fast lookup. May overflow signed int for large RplId values —
	// only used as a dictionary key, never displayed directly.
	int m_iGroupId;

	// Display name for the group (callsign), e.g. "Alpha 1" or "Bravo 2".
	// Resolved on the server from SCR_CallsignGroupComponent at registration time.
	string m_sGroupName;

	// Damage state of the character (EDamageState cast to int).
	// 0 = UNDAMAGED, 2 = DESTROYED. We only care about alive vs dead.
	int m_iDamageState;

	// Whether this slot is locked (admin/scenario designer locked it).
	bool m_bLocked;

	// --- Fixed-size portion: 4 ints (4 bytes each) + 1 bool (1 byte) = 17 bytes ---
	// Variable-size portion: 3 strings (m_sName, m_sFactionKey, m_sGroupName)
	protected static const int SERIALIZED_BYTES_FIXED = 17;

	// --- Constructor ---

	void PS_SlotData()
	{
		m_iRplId = -1;
		m_sName = "";
		m_sFactionKey = "";
		m_iPlayerId = -1;
		m_iGroupId = -1;
		m_sGroupName = "";
		m_iDamageState = 0;
		m_bLocked = false;
	}

	// --- Helpers ---

	bool IsEmpty()
	{
		return m_iPlayerId == -1;
	}

	bool IsDestroyed()
	{
		return m_iDamageState == EDamageState.DESTROYED;
	}

	bool IsAvailable()
	{
		return IsEmpty() && !m_bLocked && !IsDestroyed();
	}

	// --- Codec: Extract (instance → snapshot) ---
	// WHY: Captures the current live state of this slot into a binary snapshot.
	// Used by the replication system when it needs to serialize this data.

	static bool Extract(PS_SlotData instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.SerializeInt(instance.m_iRplId);
		snapshot.SerializeInt(instance.m_iPlayerId);
		snapshot.SerializeInt(instance.m_iGroupId);
		snapshot.SerializeInt(instance.m_iDamageState);
		snapshot.SerializeBool(instance.m_bLocked);
		snapshot.SerializeString(instance.m_sName);
		snapshot.SerializeString(instance.m_sFactionKey);
		snapshot.SerializeString(instance.m_sGroupName);
		return true;
	}

	// --- Codec: Inject (snapshot → instance) ---
	// WHY: Restores slot state from a binary snapshot.
	// Used on clients receiving replicated data or JIP streaming.

	static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, PS_SlotData instance)
	{
		snapshot.SerializeInt(instance.m_iRplId);
		snapshot.SerializeInt(instance.m_iPlayerId);
		snapshot.SerializeInt(instance.m_iGroupId);
		snapshot.SerializeInt(instance.m_iDamageState);
		snapshot.SerializeBool(instance.m_bLocked);
		snapshot.SerializeString(instance.m_sName);
		snapshot.SerializeString(instance.m_sFactionKey);
		snapshot.SerializeString(instance.m_sGroupName);
		return true;
	}

	// --- Codec: Encode (snapshot → packet) ---
	// WHY: Converts snapshot bytes into a network packet for transmission.
	// Fixed-size fields go as a bulk block, strings are encoded separately.

	static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet)
	{
		// 4 ints (16 bytes) + 1 bool (1 byte) = 17 bytes fixed
		// WHY 17 not 21: SerializeBool uses 1 byte in snapshot but we encode it
		// separately to match the Extract order precisely.
		snapshot.Serialize(packet, 16);		// m_iRplId, m_iPlayerId, m_iGroupId, m_iDamageState
		snapshot.EncodeBool(packet);		// m_bLocked
		snapshot.EncodeString(packet);		// m_sName
		snapshot.EncodeString(packet);		// m_sFactionKey
		snapshot.EncodeString(packet);		// m_sGroupName
	}

	// --- Codec: Decode (packet → snapshot) ---
	// WHY: Reconstructs snapshot from a received network packet.
	// Must mirror Encode exactly in field order and types.

	static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.Serialize(packet, 16);		// m_iRplId, m_iPlayerId, m_iGroupId, m_iDamageState
		snapshot.DecodeBool(packet);		// m_bLocked
		snapshot.DecodeString(packet);		// m_sName
		snapshot.DecodeString(packet);		// m_sFactionKey
		snapshot.DecodeString(packet);		// m_sGroupName
		return true;
	}

	// --- Codec: SnapCompare (snapshot vs snapshot) ---
	// WHY: Compares two snapshots to determine if the data changed.
	// If this returns true, no replication update is sent — saves bandwidth.

	static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs, ScriptCtx ctx)
	{
		return lhs.CompareSnapshots(rhs, 16)		// 4 ints
			&& lhs.CompareSnapshots(rhs, 1)			// m_bLocked (bool = 1 byte)
			&& lhs.CompareStringSnapshots(rhs)		// m_sName
			&& lhs.CompareStringSnapshots(rhs)		// m_sFactionKey
			&& lhs.CompareStringSnapshots(rhs);		// m_sGroupName
	}

	// --- Codec: PropCompare (instance vs snapshot) ---
	// WHY: Compares the live instance against its last snapshot.
	// Used by the replication system to detect changes on the authority.

	static bool PropCompare(PS_SlotData instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
		return snapshot.CompareInt(instance.m_iRplId)
			&& snapshot.CompareInt(instance.m_iPlayerId)
			&& snapshot.CompareInt(instance.m_iGroupId)
			&& snapshot.CompareInt(instance.m_iDamageState)
			&& snapshot.CompareBool(instance.m_bLocked)
			&& snapshot.CompareString(instance.m_sName)
			&& snapshot.CompareString(instance.m_sFactionKey)
			&& snapshot.CompareString(instance.m_sGroupName);
	}
}
