// PS_VehicleData — Lightweight data struct representing one vehicle assignment.
// Used as RPC argument type and for RplSave/RplLoad serialization.
// NOT a [RplProp()] — collections of these are managed by PS_LobbyManager.
// Codec pattern follows SCR_MapMarkerBase (vanilla).

class PS_VehicleData
{
	// --- Fields ---

	// RplId of the vehicle entity, serialized as int for network transport.
	int m_iRplId;

	// Display name of the vehicle (e.g. "M998", "BTR-70").
	// Extracted from vehicle prefab at registration time.
	string m_sName;

	// Faction key this vehicle belongs to (e.g. "US", "USSR").
	string m_sFactionKey;

	// Group ID that owns this vehicle. -1 means unassigned.
	int m_iGroupId;

	// Whether this vehicle is locked (admin locked it).
	bool m_bLocked;

	// --- Constructor ---

	void PS_VehicleData()
	{
		m_iRplId = -1;
		m_sName = "";
		m_sFactionKey = "";
		m_iGroupId = -1;
		m_bLocked = false;
	}

	// --- Codec: Extract (instance → snapshot) ---

	static bool Extract(PS_VehicleData instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.SerializeInt(instance.m_iRplId);
		snapshot.SerializeInt(instance.m_iGroupId);
		snapshot.SerializeBool(instance.m_bLocked);
		snapshot.SerializeString(instance.m_sName);
		snapshot.SerializeString(instance.m_sFactionKey);
		return true;
	}

	// --- Codec: Inject (snapshot → instance) ---

	static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, PS_VehicleData instance)
	{
		snapshot.SerializeInt(instance.m_iRplId);
		snapshot.SerializeInt(instance.m_iGroupId);
		snapshot.SerializeBool(instance.m_bLocked);
		snapshot.SerializeString(instance.m_sName);
		snapshot.SerializeString(instance.m_sFactionKey);
		return true;
	}

	// --- Codec: Encode (snapshot → packet) ---

	static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet)
	{
		snapshot.Serialize(packet, 8);		// m_iRplId, m_iGroupId (2 ints = 8 bytes)
		snapshot.EncodeBool(packet);		// m_bLocked
		snapshot.EncodeString(packet);		// m_sName
		snapshot.EncodeString(packet);		// m_sFactionKey
	}

	// --- Codec: Decode (packet → snapshot) ---

	static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.Serialize(packet, 8);		// m_iRplId, m_iGroupId
		snapshot.DecodeBool(packet);		// m_bLocked
		snapshot.DecodeString(packet);		// m_sName
		snapshot.DecodeString(packet);		// m_sFactionKey
		return true;
	}

	// --- Codec: SnapCompare (snapshot vs snapshot) ---

	static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs, ScriptCtx ctx)
	{
		return lhs.CompareSnapshots(rhs, 8)			// 2 ints
			&& lhs.CompareSnapshots(rhs, 1)			// m_bLocked
			&& lhs.CompareStringSnapshots(rhs)		// m_sName
			&& lhs.CompareStringSnapshots(rhs);		// m_sFactionKey
	}

	// --- Codec: PropCompare (instance vs snapshot) ---

	static bool PropCompare(PS_VehicleData instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
		return snapshot.CompareInt(instance.m_iRplId)
			&& snapshot.CompareInt(instance.m_iGroupId)
			&& snapshot.CompareBool(instance.m_bLocked)
			&& snapshot.CompareString(instance.m_sName)
			&& snapshot.CompareString(instance.m_sFactionKey);
	}
}
