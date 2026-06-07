// Playable replicatable container
class PS_PlayableContainer
{
	protected RplId m_RplId;
	protected string m_sName;
	protected FactionKey m_FactionKey;
	protected SCR_ECharacterRank m_eCharacterRank;
	// R3: role icon/name are NOT serialized (they were duplicated across 100+ slots). Cached here,
	// resolved lazily: from the live component on the server, from the prefab on the client.
	protected string m_sRoleIconPath;
	protected string m_sRoleIconQuad;
	protected string m_sRoleName;
	protected bool m_bRoleResolved;
	protected EDamageState m_eDamageState;

	void Init(PS_PlayableComponent playableComponent) // Rpc workaround
	{
		m_PlayableComponent = playableComponent;
		m_RplId = playableComponent.GetRplId();
		m_sName = playableComponent.GetName();
		m_FactionKey = playableComponent.GetFactionKey();
		m_eCharacterRank = playableComponent.GetCharacterRank();
		m_sRoleIconPath = playableComponent.GetRoleIconPath();
		m_sRoleIconQuad = playableComponent.GetRoleIconQuad();
		m_sRoleName = playableComponent.GetRoleName();
		m_bRoleResolved = true; // server has authoritative role meta from the component
		m_eDamageState = playableComponent.GetDamageState();
	}

	// -------------------------- Replication ----------------------------
	static bool Extract(PS_PlayableContainer instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.SerializeInt(instance.m_RplId);
		snapshot.SerializeString(instance.m_sName);
		snapshot.SerializeString(instance.m_FactionKey);
		snapshot.SerializeInt(instance.m_eCharacterRank);
		snapshot.SerializeInt(instance.m_eDamageState);
		return true;
	}

	static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, PS_PlayableContainer instance)
	{
		snapshot.SerializeInt(instance.m_RplId);
		snapshot.SerializeString(instance.m_sName);
		snapshot.SerializeString(instance.m_FactionKey);
		snapshot.SerializeInt(instance.m_eCharacterRank);
		snapshot.SerializeInt(instance.m_eDamageState);
		return true;
	}

	static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet)
	{
		snapshot.EncodeInt(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeInt(packet);
	}

	static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.DecodeInt(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeInt(packet);
		snapshot.DecodeInt(packet);
		return true;
	}

	static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs, ScriptCtx ctx)
	{
		return lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareStringSnapshots(rhs)
			&& lhs.CompareStringSnapshots(rhs)
			&& lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareSnapshots(rhs, 4);
	}

	static bool PropCompare(PS_PlayableContainer instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
		return snapshot.CompareInt(instance.m_RplId)
			&& snapshot.CompareString(instance.m_sName)
			&& snapshot.CompareString(instance.m_FactionKey)
			&& snapshot.CompareInt(instance.m_eCharacterRank)
			&& snapshot.CompareInt(instance.m_eDamageState);
	}

	void Save(ScriptBitWriter writer)
	{
		writer.WriteInt(m_RplId);
		writer.WriteString(m_sName);
		writer.WriteString(m_FactionKey);
		writer.WriteInt(m_eCharacterRank);
		writer.WriteInt(m_eDamageState);
	}

	bool Load(ScriptBitReader reader)
	{
		if (!reader.ReadInt(m_RplId)) return false;
		if (!reader.ReadString(m_sName)) return false;
		if (!reader.ReadString(m_FactionKey)) return false;
		if (!reader.ReadInt(m_eCharacterRank)) return false;
		if (!reader.ReadInt(m_eDamageState)) return false;
		return true;
	}

	// -------------------------- Get server ----------------------------
	protected PS_PlayableComponent m_PlayableComponent;
	PS_PlayableComponent GetPlayableComponent()
	{
		return m_PlayableComponent;
	}
	PS_PlayableComponent GetPlayableComponentTemp()
	{
		return m_PlayableComponent;
	}

	// ---------------------------- Events ------------------------------
	protected ref ScriptInvokerInt2 m_eOnPlayerChange = new ScriptInvokerInt2(); // int playerId
	ScriptInvokerInt2 GetOnPlayerChange()
	{
		return m_eOnPlayerChange;
	}
	void InvokeOnPlayerChanged(int oldPlayerId, int playerId)
	{
		m_eOnPlayerChange.Invoke(oldPlayerId, playerId);
	}

	ref ScriptInvokerInt m_eOnDamageStateChanged = new ScriptInvokerInt();
	ScriptInvokerInt GetOnDamageStateChanged()
	{
		return m_eOnDamageStateChanged;
	}
	ref ScriptInvokerVoid m_eOnUnregister = new ScriptInvokerVoid();
	ScriptInvokerVoid GetOnUnregister()
	{
		return m_eOnUnregister;
	}
	protected ref ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected> m_eOnPlayerDisconnected = new ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected>();
	ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected> GetOnPlayerDisconnected()
	{
		return m_eOnPlayerDisconnected;
	}
	protected ref ScriptInvokerBase<SCR_BaseGameMode_PlayerId> m_eOnPlayerConnected = new ScriptInvokerBase<SCR_BaseGameMode_PlayerId>();
	ScriptInvokerBase<SCR_BaseGameMode_PlayerId> GetOnPlayerConnected()
	{
		return m_eOnPlayerConnected;
	}
	protected ref ScriptInvokerBase<SCR_BaseGameMode_OnPlayerRoleChanged> m_eOnPlayerRoleChange = new ScriptInvokerBase<SCR_BaseGameMode_OnPlayerRoleChanged>();
	ScriptInvokerBase<SCR_BaseGameMode_OnPlayerRoleChanged> GetOnPlayerRoleChange()
	{
		return m_eOnPlayerRoleChange;
	}
	protected ref ScriptInvokerInt m_eOnPlayerStateChange = new ScriptInvokerInt();
	ScriptInvokerInt GetOnPlayerStateChange()
	{
		return m_eOnPlayerStateChange;
	}
	protected ref ScriptInvokerBool m_eOnPlayerPinChange = new ScriptInvokerBool();
	ScriptInvokerBool GetOnPlayerPinChange()
	{
		return m_eOnPlayerPinChange;
	}

	void OnDamageStateChanged(EDamageState damageState)
	{
		m_eDamageState = damageState;
		m_eOnDamageStateChanged.Invoke(damageState);
	}

	// -------------------------- Get client ----------------------------
	RplId GetRplId()
	{
		return m_RplId;
	}
	string GetName()
	{
		return m_sName;
	}
	FactionKey GetFactionKey()
	{
		return m_FactionKey;
	}
	SCR_Faction GetFaction()
	{
		return SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey(GetFactionKey()));
	}
	// R3: lazily resolve role icon/name (no longer serialized). Role meta is static per prefab/role,
	// so it is resolved once and cached. Server reads the live component; client derives from the
	// prefab via the preview manager (independent of entity scope — works for out-of-scope corpses).
	protected void ResolveRoleMeta()
	{
		if (m_bRoleResolved)
			return;

		// Server / listen-server host: the playable component (server-only) is the authority
		if (m_PlayableComponent)
		{
			m_sRoleIconPath = m_PlayableComponent.GetRoleIconPath();
			m_sRoleIconQuad = m_PlayableComponent.GetRoleIconQuad();
			m_sRoleName = m_PlayableComponent.GetRoleName();
			m_bRoleResolved = true;
			return;
		}

		// Pure client: derive from the prefab via the preview manager
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (!playableManager)
			return;
		ResourceName prefab = playableManager.GetPlayablePrefab(m_RplId);
		if (prefab == "")
			return; // prefab not replicated yet — retry on next access

		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return;
		ItemPreviewManagerEntity previewManager = world.GetItemPreviewManager();
		if (!previewManager)
			return;
		IEntity entity = previewManager.ResolvePreviewEntityForPrefab(prefab);
		if (!entity)
			return;
		SCR_EditableCharacterComponent editableCharacterComponent = SCR_EditableCharacterComponent.Cast(entity.FindComponent(SCR_EditableCharacterComponent));
		if (!editableCharacterComponent)
			return;
		SCR_UIInfo uiInfo = editableCharacterComponent.GetInfo();
		if (!uiInfo)
			return;

		// Mirror PS_PlayableComponent.GetRoleIconPath/Quad/RoleName
		if (uiInfo.GetIconSetName() == "")
			m_sRoleIconPath = uiInfo.GetIconPath();
		else
			m_sRoleIconPath = uiInfo.GetImageSetPath();
		m_sRoleIconQuad = uiInfo.GetIconSetName();
		m_sRoleName = uiInfo.GetName();
		m_bRoleResolved = true;
	}

	string GetRoleIconPath()
	{
		ResolveRoleMeta();
		return m_sRoleIconPath;
	}
	string GetRoleIconQuad()
	{
		ResolveRoleMeta();
		return m_sRoleIconQuad;
	}

	bool SetIconTo(ImageWidget imageWidget)
	{
		ResolveRoleMeta();
		if (!imageWidget || m_sRoleIconPath.IsEmpty())
			return false;

		if (m_sRoleIconQuad != "")
			imageWidget.LoadImageFromSet(0, m_sRoleIconPath, m_sRoleIconQuad);
		else
			imageWidget.LoadImageTexture(0, m_sRoleIconPath);

		return true;
	}

	string GetRoleName()
	{
		ResolveRoleMeta();
		return m_sRoleName;
	}
	SCR_ECharacterRank GetCharacterRank()
	{
		return m_eCharacterRank;
	}
	EDamageState GetDamageState()
	{
		return m_eDamageState;
	}
}
