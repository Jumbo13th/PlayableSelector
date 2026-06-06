// PS_PlayableComponent — Character bridge component. Attached to every playable character entity.
// ZERO REPLICATION — no [RplProp()], no RplSave/RplLoad, no BumpMe.
//
// WHY zero replication: the old PS had RplSave/RplLoad + [RplProp()] + BumpMe on each of
// 127 character entities. This was the ROOT CAUSE of STALLED kicks — 127x replication overhead.
// Echo solved this by moving ALL data to the central manager. We follow Echo's approach.
//
// This component's only job is to REGISTER the character with PS_LobbyManager on the server
// and provide a local bridge for code that has a character entity and needs its slot data.

class PS_PlayableComponentClass : ScriptComponentClass
{
}

class PS_PlayableComponent : ScriptComponent
{
	// Cached RplId as int — set once at registration, never changes.
	protected int m_iRplId = -1;

	// WHY: fallback counter for Workbench offline mode where RplIds are invalid.
	// On a real server, RplIds are always valid and this counter is unused.
	protected static int s_iFallbackIdCounter = 100000;

	// =====================================================================
	// LIFECYCLE
	// =====================================================================

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode())
			return;

		// WHY: server-only registration. The server discovers all playable characters
		// and registers them with PS_LobbyManager. Clients learn about slots via RPCs.
		if (Replication.IsServer())
		{
			// WHY CallLater(500): at OnPostInit time, RplComponent may not have a valid
			// RplId yet — the entity hasn't been fully registered with the replication
			// system. Deferring gives the engine time to assign IDs.
			GetGame().GetCallqueue().CallLater(RegisterWithManager, 500, false);
		}
	}

	override void OnDelete(IEntity owner)
	{
		super.OnDelete(owner);

		// WHY: if the character entity is deleted (killed and removed, or scenario cleanup),
		// tell the manager to unregister it. Server-only — clients get notified via RPC.
		if (Replication.IsServer() && m_iRplId > 0)
		{
			PS_LobbyManager mgr = PS_LobbyManager.GetInstance();
			if (mgr)
				mgr.UnregisterSlot_S(m_iRplId);
		}
	}

	// =====================================================================
	// REGISTRATION — server-only, called once at init
	// =====================================================================

	protected void RegisterWithManager()
	{
		// WHY: get RplId NOW (deferred), not at OnPostInit when it's not ready yet.
		IEntity owner = GetOwner();
		if (!owner)
			return;

		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (rpl)
			m_iRplId = rpl.Id();

		// WHY: if RplId is still invalid (Workbench offline mode), assign a unique fallback ID.
		// On a real dedicated server, RplId will be valid.
		if (m_iRplId <= 0)
		{
			m_iRplId = s_iFallbackIdCounter;
			s_iFallbackIdCounter++;
		}

		PS_LobbyManager mgr = PS_LobbyManager.GetInstance();
		if (!mgr)
		{
			Print(string.Format("[PS_Lobby] PlayableComponent: manager not found, cannot register id=%1", m_iRplId), LogLevel.WARNING);
			return;
		}

		// Build slot data from the character entity.
		PS_SlotData slot = BuildSlotData();
		if (!slot)
			return;

		mgr.RegisterSlot_S(slot);

		Print(string.Format("[PS_Lobby] PlayableComponent registered: id=%1 name=%2 faction=%3 group=%4",
			slot.m_iRplId, slot.m_sName, slot.m_sFactionKey, slot.m_sGroupName), LogLevel.NORMAL);
	}

	// Extract slot data from the character entity's components.
	// WHY: we read name, faction, group from the entity ONCE at registration.
	// After that, the manager owns the data — the entity doesn't replicate it.
	protected PS_SlotData BuildSlotData()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return null;

		PS_SlotData slot = new PS_SlotData();
		slot.m_iRplId = m_iRplId;

		// --- Faction ---
		// WHY: use GetAffiliatedFactionKey() (proto external on the base class) instead of
		// GetAffiliatedFaction().GetFactionKey(). GetAffiliatedFaction() queries the
		// FactionManager which may not have registered the entity yet at 500ms. The key
		// string is stored directly on the component and is always available.
		FactionAffiliationComponent factionComp = FactionAffiliationComponent.Cast(
			owner.FindComponent(FactionAffiliationComponent));
		if (factionComp)
			slot.m_sFactionKey = factionComp.GetAffiliatedFactionKey();

		// --- Name / Role ---
		// WHY: read the character's display name from its loadout or entity name.
		// This gives us "Rifleman", "Team Leader", etc.
		SCR_EditableEntityComponent editableComp = SCR_EditableEntityComponent.Cast(
			owner.FindComponent(SCR_EditableEntityComponent));
		if (editableComp)
		{
			SCR_EditableEntityUIInfo uiInfo = SCR_EditableEntityUIInfo.Cast(editableComp.GetInfo());
			if (uiInfo)
				slot.m_sName = uiInfo.GetName();
		}

		// Fallback: if no editable entity name, use a generic name.
		if (slot.m_sName == "")
			slot.m_sName = "Unknown";

		// --- Group ---
		// WHY: read group ID from the character's AI agent's parent group.
		// AIControlComponent.GetControlAIAgent() → AIAgent.GetParentGroup() → AIGroup.
		SCR_AIGroup group = null;
		AIControlComponent aiControl = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
		if (aiControl)
		{
			AIAgent agent = aiControl.GetControlAIAgent();
			if (agent)
				group = SCR_AIGroup.Cast(agent.GetParentGroup());
		}

		if (group)
		{
			RplComponent groupRpl = RplComponent.Cast(group.FindComponent(RplComponent));
			if (groupRpl)
				slot.m_iGroupId = groupRpl.Id();

			// WHY: SCR_CallsignGroupComponent holds the squad callsign assigned by the
			// callsign manager (e.g. "Alpha 1", "Bravo 2"). Reading it here on the server
			// and storing as a display string avoids sending raw RplId integers to the UI.
			// GetCallsignNames returns already-translated name strings (not format keys),
			// so no WidgetManager needed — safe to call on dedicated server.
			SCR_CallsignGroupComponent callsignComp = SCR_CallsignGroupComponent.Cast(
				group.FindComponent(SCR_CallsignGroupComponent));
			if (callsignComp)
			{
				string company, platoon, squad, character, format;
				if (callsignComp.GetCallsignNames(company, platoon, squad, character, format))
				{
					if (platoon != "" && squad != "")
						slot.m_sGroupName = platoon + " " + squad;
					else if (squad != "")
						slot.m_sGroupName = squad;
					else if (platoon != "")
						slot.m_sGroupName = platoon;
				}
			}

			// Fallback: entity name set in the editor, or a generic label.
			if (slot.m_sGroupName == "")
			{
				string entityName = group.GetName();
				if (entityName != "")
					slot.m_sGroupName = entityName;
			}
		}

		// --- Damage State ---
		SCR_DamageManagerComponent dmgMgr = SCR_DamageManagerComponent.Cast(
			owner.FindComponent(SCR_DamageManagerComponent));
		if (dmgMgr)
			slot.m_iDamageState = dmgMgr.GetState();

		return slot;
	}

	// =====================================================================
	// PUBLIC API — local bridge for code that has an entity reference
	// =====================================================================

	// Get this character's RplId (as int).
	int GetRplId()
	{
		return m_iRplId;
	}

	// Get this character's slot data from the manager. Returns null if not registered.
	PS_SlotData GetSlotData()
	{
		PS_LobbyManager mgr = PS_LobbyManager.GetInstance();
		if (!mgr)
			return null;

		return mgr.FindSlotByRplId(m_iRplId);
	}

	// Convenience: get the player ID occupying this character. -1 if empty.
	int GetAssignedPlayerId()
	{
		PS_SlotData slot = GetSlotData();
		if (!slot)
			return -1;

		return slot.m_iPlayerId;
	}

	// Convenience: is this character available for selection?
	bool IsAvailable()
	{
		PS_SlotData slot = GetSlotData();
		if (!slot)
			return false;

		return slot.IsAvailable();
	}
}
