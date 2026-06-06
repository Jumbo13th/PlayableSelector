// PS_LobbyManager — Central lobby state manager.
// Lives on the GameMode entity as a SCR_BaseGameModeComponent.
// Owns ALL lobby data: slot assignments, player mappings, vehicle data.
// Replication: RPCs for live updates + RplSave/RplLoad for JIP streaming.
// NO [RplProp()] on collections — follows vanilla SCR_MapMarkerManagerComponent pattern.
// Pattern reference: Arma-Reforger-Script-Diff/scripts/Game/Map/Markers/SCR_MapMarkerManagerComponent.c

class PS_LobbyManagerClass : SCR_BaseGameModeComponentClass
{
}

class PS_LobbyManager : SCR_BaseGameModeComponent
{
	// =====================================================================
	// SINGLETON
	// =====================================================================

	protected static PS_LobbyManager s_Instance;

	static PS_LobbyManager GetInstance()
	{
		return s_Instance;
	}

	// =====================================================================
	// DATA — plain collections, NOT [RplProp()]. Synced via RPCs + RplSave/RplLoad.
	// =====================================================================

	// All registered playable slots. Keyed by character RplId (as int).
	// WHY array not map: RplSave/RplLoad is simpler with arrays. Lookup by RplId
	// uses helper method. At 127 slots, linear search is negligible.
	protected ref array<ref PS_SlotData> m_aSlots = {};

	// All registered vehicles. Keyed by vehicle RplId (as int).
	protected ref array<ref PS_VehicleData> m_aVehicles = {};

	// Player names cache. Index = playerId, used for UI display.
	// WHY separate from slots: a player may not have a slot yet but still needs
	// their name displayed in the player list.
	protected ref map<int, string> m_mPlayerNames = new map<int, string>();

	// Server-only: disconnected player tracking for reconnect.
	// Maps player GUID → slot RplId they had. Not replicated — server-only concern.
	protected ref map<string, int> m_mDisconnectedPlayers = new map<string, int>();

	// =====================================================================
	// EVENTS — UI and other systems subscribe to these. Manager doesn't know about UI.
	// =====================================================================

	// WHY ScriptInvoker: vanilla pattern for decoupling. UI subscribes, manager invokes.
	protected ref ScriptInvoker m_OnSlotRegistered = new ScriptInvoker();		// (PS_SlotData slot)
	protected ref ScriptInvoker m_OnSlotUnregistered = new ScriptInvoker();		// (int rplId)
	protected ref ScriptInvoker m_OnSlotUpdated = new ScriptInvoker();			// (PS_SlotData slot)
	protected ref ScriptInvoker m_OnPlayerAssigned = new ScriptInvoker();		// (int playerId, int slotRplId)
	protected ref ScriptInvoker m_OnPlayerUnassigned = new ScriptInvoker();		// (int playerId, int slotRplId)
	protected ref ScriptInvoker m_OnVehicleRegistered = new ScriptInvoker();	// (PS_VehicleData vehicle)
	protected ref ScriptInvoker m_OnVehicleUnregistered = new ScriptInvoker();	// (int rplId)
	protected ref ScriptInvoker m_OnPlayerNameUpdated = new ScriptInvoker();	// (int playerId, string name)

	// Public getters — UI subscribes via these.
	ScriptInvoker GetOnSlotRegistered()		{ return m_OnSlotRegistered; }
	ScriptInvoker GetOnSlotUnregistered()	{ return m_OnSlotUnregistered; }
	ScriptInvoker GetOnSlotUpdated()		{ return m_OnSlotUpdated; }
	ScriptInvoker GetOnPlayerAssigned()		{ return m_OnPlayerAssigned; }
	ScriptInvoker GetOnPlayerUnassigned()	{ return m_OnPlayerUnassigned; }
	ScriptInvoker GetOnVehicleRegistered()	{ return m_OnVehicleRegistered; }
	ScriptInvoker GetOnVehicleUnregistered(){ return m_OnVehicleUnregistered; }
	ScriptInvoker GetOnPlayerNameUpdated()	{ return m_OnPlayerNameUpdated; }

	// =====================================================================
	// LIFECYCLE
	// =====================================================================

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		s_Instance = this;

		Print("[PS_Lobby] LobbyManager initialized", LogLevel.NORMAL);
	}

	override void OnDelete(IEntity owner)
	{
		super.OnDelete(owner);
		if (s_Instance == this)
			s_Instance = null;
	}

	// =====================================================================
	// PUBLIC READ API — UI and components query these. No replication cost.
	// =====================================================================

	// Get all registered slots.
	array<ref PS_SlotData> GetSlots()
	{
		return m_aSlots;
	}

	// Find a slot by its character RplId. Returns null if not found.
	PS_SlotData FindSlotByRplId(int rplId)
	{
		foreach (PS_SlotData slot : m_aSlots)
		{
			if (slot.m_iRplId == rplId)
				return slot;
		}
		return null;
	}

	// Find a slot by the player occupying it. Returns null if player has no slot.
	PS_SlotData FindSlotByPlayerId(int playerId)
	{
		foreach (PS_SlotData slot : m_aSlots)
		{
			if (slot.m_iPlayerId == playerId)
				return slot;
		}
		return null;
	}

	// Get all registered vehicles.
	array<ref PS_VehicleData> GetVehicles()
	{
		return m_aVehicles;
	}

	// Find a vehicle by RplId.
	PS_VehicleData FindVehicleByRplId(int rplId)
	{
		foreach (PS_VehicleData vehicle : m_aVehicles)
		{
			if (vehicle.m_iRplId == rplId)
				return vehicle;
		}
		return null;
	}

	// Get player display name.
	string GetPlayerName(int playerId)
	{
		string name;
		if (m_mPlayerNames.Find(playerId, name))
			return name;
		return "";
	}

	// Get all slots for a given faction.
	array<ref PS_SlotData> GetSlotsForFaction(string factionKey)
	{
		array<ref PS_SlotData> result = {};
		foreach (PS_SlotData slot : m_aSlots)
		{
			if (slot.m_sFactionKey == factionKey)
				result.Insert(slot);
		}
		return result;
	}

	// Count players currently assigned to a faction.
	int CountPlayersInFaction(string factionKey)
	{
		int count = 0;
		foreach (PS_SlotData slot : m_aSlots)
		{
			if (slot.m_sFactionKey == factionKey && slot.m_iPlayerId >= 0)
				count++;
		}
		return count;
	}

	// =====================================================================
	// SERVER-SIDE MUTATION — called by PS_PlayableComponent and PS_LobbyPlayerComponent
	// These execute on authority, then broadcast via RPC to all clients.
	// Pattern: mutate local → broadcast RPC → clients apply same mutation.
	// =====================================================================

	// --- Slot Registration (called by PS_PlayableComponent.OnPostInit on server) ---

	void RegisterSlot_S(PS_SlotData slot)
	{
		if (!Replication.IsServer())
			return;

		// Deduplicate — slot may already exist from a previous registration.
		if (FindSlotByRplId(slot.m_iRplId))
		{
			Print(string.Format("[PS_Lobby] Slot %1 already registered, skipping", slot.m_iRplId), LogLevel.WARNING);
			return;
		}

		Print(string.Format("[PS_Lobby] Registering slot: rplId=%1 name=%2 faction=%3",
			slot.m_iRplId, slot.m_sName, slot.m_sFactionKey), LogLevel.NORMAL);

		// Apply locally on server.
		ApplyRegisterSlot(slot);

		// WHY: decompose into primitives for RPC transport instead of passing PS_SlotData.
		// Enfusion codec auto-discovery for custom types in RPCs is not guaranteed.
		// Primitives (int, string, bool) always work.
		Rpc(RpcDo_RegisterSlot,
			slot.m_iRplId, slot.m_sName, slot.m_sFactionKey,
			slot.m_iPlayerId, slot.m_iGroupId, slot.m_sGroupName,
			slot.m_iDamageState, slot.m_bLocked);
	}

	// --- Slot Unregistration (character entity deleted) ---

	void UnregisterSlot_S(int rplId)
	{
		if (!Replication.IsServer())
			return;

		if (!FindSlotByRplId(rplId))
			return;

		Print(string.Format("[PS_Lobby] Unregistering slot: rplId=%1", rplId), LogLevel.NORMAL);

		ApplyUnregisterSlot(rplId);
		Rpc(RpcDo_UnregisterSlot, rplId);
	}

	// --- Take Slot (player selects a character) ---

	bool TakeSlot_S(int playerId, int slotRplId)
	{
		if (!Replication.IsServer())
			return false;

		PS_SlotData slot = FindSlotByRplId(slotRplId);
		if (!slot)
		{
			Print(string.Format("[PS_Lobby] TakeSlot failed: slot %1 not found", slotRplId), LogLevel.WARNING);
			return false;
		}

		if (!slot.IsAvailable())
		{
			Print(string.Format("[PS_Lobby] TakeSlot failed: slot %1 not available (player=%2, locked=%3, destroyed=%4)",
				slotRplId, slot.m_iPlayerId, slot.m_bLocked, slot.IsDestroyed()), LogLevel.WARNING);
			return false;
		}

		// If player already has a slot, leave it first.
		PS_SlotData currentSlot = FindSlotByPlayerId(playerId);
		if (currentSlot)
			LeaveSlot_S(playerId);

		Print(string.Format("[PS_Lobby] Player %1 taking slot %2 (%3)", playerId, slotRplId, slot.m_sName), LogLevel.NORMAL);

		ApplyTakeSlot(playerId, slotRplId);
		Rpc(RpcDo_TakeSlot, playerId, slotRplId);
		return true;
	}

	// --- Leave Slot (player deselects or disconnects) ---

	void LeaveSlot_S(int playerId)
	{
		if (!Replication.IsServer())
			return;

		PS_SlotData slot = FindSlotByPlayerId(playerId);
		if (!slot)
			return;

		int slotRplId = slot.m_iRplId;
		Print(string.Format("[PS_Lobby] Player %1 leaving slot %2", playerId, slotRplId), LogLevel.NORMAL);

		ApplyLeaveSlot(playerId, slotRplId);
		Rpc(RpcDo_LeaveSlot, playerId, slotRplId);
	}

	// --- Update Damage State (character took damage or died) ---

	void SetSlotDamageState_S(int slotRplId, int damageState)
	{
		if (!Replication.IsServer())
			return;

		PS_SlotData slot = FindSlotByRplId(slotRplId);
		if (!slot)
			return;

		if (slot.m_iDamageState == damageState)
			return;

		Print(string.Format("[PS_Lobby] Slot %1 damage state: %2", slotRplId, damageState), LogLevel.NORMAL);

		ApplySetDamageState(slotRplId, damageState);
		Rpc(RpcDo_SetDamageState, slotRplId, damageState);
	}

	// --- Lock/Unlock Slot ---

	void SetSlotLocked_S(int slotRplId, bool locked)
	{
		if (!Replication.IsServer())
			return;

		PS_SlotData slot = FindSlotByRplId(slotRplId);
		if (!slot)
			return;

		if (slot.m_bLocked == locked)
			return;

		ApplySetSlotLocked(slotRplId, locked);
		Rpc(RpcDo_SetSlotLocked, slotRplId, locked);
	}

	// --- Player Name ---

	void SetPlayerName_S(int playerId, string name)
	{
		if (!Replication.IsServer())
			return;

		ApplySetPlayerName(playerId, name);
		Rpc(RpcDo_SetPlayerName, playerId, name);
	}

	// --- Vehicle Registration ---

	void RegisterVehicle_S(PS_VehicleData vehicle)
	{
		if (!Replication.IsServer())
			return;

		if (FindVehicleByRplId(vehicle.m_iRplId))
			return;

		Print(string.Format("[PS_Lobby] Registering vehicle: rplId=%1 name=%2", vehicle.m_iRplId, vehicle.m_sName), LogLevel.NORMAL);

		ApplyRegisterVehicle(vehicle);
		Rpc(RpcDo_RegisterVehicle,
			vehicle.m_iRplId, vehicle.m_sName, vehicle.m_sFactionKey,
			vehicle.m_iGroupId, vehicle.m_bLocked);
	}

	void UnregisterVehicle_S(int rplId)
	{
		if (!Replication.IsServer())
			return;

		if (!FindVehicleByRplId(rplId))
			return;

		ApplyUnregisterVehicle(rplId);
		Rpc(RpcDo_UnregisterVehicle, rplId);
	}

	// =====================================================================
	// APPLY METHODS — shared logic for server + client. Mutate local state.
	// WHY separate: avoids duplicating mutation logic in RPC handlers.
	// Server calls Apply + Rpc, clients call Apply from RPC handler.
	// =====================================================================

	protected void ApplyRegisterSlot(PS_SlotData slot)
	{
		m_aSlots.Insert(slot);
		m_OnSlotRegistered.Invoke(slot);
	}

	protected void ApplyUnregisterSlot(int rplId)
	{
		for (int i = m_aSlots.Count() - 1; i >= 0; i--)
		{
			if (m_aSlots[i].m_iRplId == rplId)
			{
				m_aSlots.Remove(i);
				break;
			}
		}
		m_OnSlotUnregistered.Invoke(rplId);
	}

	protected void ApplyTakeSlot(int playerId, int slotRplId)
	{
		PS_SlotData slot = FindSlotByRplId(slotRplId);
		if (!slot)
			return;

		slot.m_iPlayerId = playerId;
		m_OnPlayerAssigned.Invoke(playerId, slotRplId);
		m_OnSlotUpdated.Invoke(slot);
	}

	protected void ApplyLeaveSlot(int playerId, int slotRplId)
	{
		PS_SlotData slot = FindSlotByRplId(slotRplId);
		if (!slot)
			return;

		slot.m_iPlayerId = -1;
		m_OnPlayerUnassigned.Invoke(playerId, slotRplId);
		m_OnSlotUpdated.Invoke(slot);
	}

	protected void ApplySetDamageState(int slotRplId, int damageState)
	{
		PS_SlotData slot = FindSlotByRplId(slotRplId);
		if (!slot)
			return;

		slot.m_iDamageState = damageState;
		m_OnSlotUpdated.Invoke(slot);
	}

	protected void ApplySetSlotLocked(int slotRplId, bool locked)
	{
		PS_SlotData slot = FindSlotByRplId(slotRplId);
		if (!slot)
			return;

		slot.m_bLocked = locked;
		m_OnSlotUpdated.Invoke(slot);
	}

	protected void ApplySetPlayerName(int playerId, string name)
	{
		m_mPlayerNames.Set(playerId, name);
		m_OnPlayerNameUpdated.Invoke(playerId, name);
	}

	protected void ApplyRegisterVehicle(PS_VehicleData vehicle)
	{
		m_aVehicles.Insert(vehicle);
		m_OnVehicleRegistered.Invoke(vehicle);
	}

	protected void ApplyUnregisterVehicle(int rplId)
	{
		for (int i = m_aVehicles.Count() - 1; i >= 0; i--)
		{
			if (m_aVehicles[i].m_iRplId == rplId)
			{
				m_aVehicles.Remove(i);
				break;
			}
		}
		m_OnVehicleUnregistered.Invoke(rplId);
	}

	// =====================================================================
	// RPCs — Broadcast (server → all clients)
	// WHY RplRcver.Broadcast: all clients need the same lobby state.
	// Each RPC carries only the delta — NOT the full collection.
	// =====================================================================

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_RegisterSlot(int rplId, string name, string factionKey,
		int playerId, int groupId, string groupName,
		int damageState, bool locked)
	{
		PS_SlotData slot = new PS_SlotData();
		slot.m_iRplId = rplId;
		slot.m_sName = name;
		slot.m_sFactionKey = factionKey;
		slot.m_iPlayerId = playerId;
		slot.m_iGroupId = groupId;
		slot.m_sGroupName = groupName;
		slot.m_iDamageState = damageState;
		slot.m_bLocked = locked;
		ApplyRegisterSlot(slot);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_UnregisterSlot(int rplId)
	{
		ApplyUnregisterSlot(rplId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_TakeSlot(int playerId, int slotRplId)
	{
		ApplyTakeSlot(playerId, slotRplId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_LeaveSlot(int playerId, int slotRplId)
	{
		ApplyLeaveSlot(playerId, slotRplId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SetDamageState(int slotRplId, int damageState)
	{
		ApplySetDamageState(slotRplId, damageState);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SetSlotLocked(int slotRplId, bool locked)
	{
		ApplySetSlotLocked(slotRplId, locked);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SetPlayerName(int playerId, string name)
	{
		ApplySetPlayerName(playerId, name);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_RegisterVehicle(int rplId, string name, string factionKey,
		int groupId, bool locked)
	{
		PS_VehicleData vehicle = new PS_VehicleData();
		vehicle.m_iRplId = rplId;
		vehicle.m_sName = name;
		vehicle.m_sFactionKey = factionKey;
		vehicle.m_iGroupId = groupId;
		vehicle.m_bLocked = locked;
		ApplyRegisterVehicle(vehicle);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_UnregisterVehicle(int rplId)
	{
		ApplyUnregisterVehicle(rplId);
	}

	// =====================================================================
	// JIP STREAMING — RplSave/RplLoad
	// WHY: when a client joins mid-game, they need the full lobby state.
	// RPCs are fire-and-forget — they don't replay for late joiners.
	// RplSave (server) serializes everything, RplLoad (client) rebuilds it.
	// Pattern: SCR_MapMarkerManagerComponent — write count, then each item.
	// =====================================================================

	override event protected bool RplSave(ScriptBitWriter writer)
	{
		// --- Slots ---
		int slotCount = m_aSlots.Count();
		writer.WriteInt(slotCount);

		foreach (PS_SlotData slot : m_aSlots)
		{
			writer.WriteInt(slot.m_iRplId);
			writer.WriteString(slot.m_sName);
			writer.WriteString(slot.m_sFactionKey);
			writer.WriteInt(slot.m_iPlayerId);
			writer.WriteInt(slot.m_iGroupId);
			writer.WriteString(slot.m_sGroupName);
			writer.WriteInt(slot.m_iDamageState);
			writer.WriteBool(slot.m_bLocked);
		}

		// --- Vehicles ---
		int vehicleCount = m_aVehicles.Count();
		writer.WriteInt(vehicleCount);

		foreach (PS_VehicleData vehicle : m_aVehicles)
		{
			writer.WriteInt(vehicle.m_iRplId);
			writer.WriteString(vehicle.m_sName);
			writer.WriteString(vehicle.m_sFactionKey);
			writer.WriteInt(vehicle.m_iGroupId);
			writer.WriteBool(vehicle.m_bLocked);
		}

		// --- Player Names ---
		int nameCount = m_mPlayerNames.Count();
		writer.WriteInt(nameCount);

		for (int i = 0; i < nameCount; i++)
		{
			writer.WriteInt(m_mPlayerNames.GetKey(i));
			writer.WriteString(m_mPlayerNames.GetElement(i));
		}

		Print(string.Format("[PS_Lobby] RplSave: %1 slots, %2 vehicles, %3 names",
			slotCount, vehicleCount, nameCount), LogLevel.NORMAL);

		return true;
	}

	override event protected bool RplLoad(ScriptBitReader reader)
	{
		// --- Slots ---
		int slotCount;
		reader.ReadInt(slotCount);

		for (int i = 0; i < slotCount; i++)
		{
			PS_SlotData slot = new PS_SlotData();
			reader.ReadInt(slot.m_iRplId);
			reader.ReadString(slot.m_sName);
			reader.ReadString(slot.m_sFactionKey);
			reader.ReadInt(slot.m_iPlayerId);
			reader.ReadInt(slot.m_iGroupId);
			reader.ReadString(slot.m_sGroupName);
			reader.ReadInt(slot.m_iDamageState);
			reader.ReadBool(slot.m_bLocked);

			m_aSlots.Insert(slot);
		}

		// --- Vehicles ---
		int vehicleCount;
		reader.ReadInt(vehicleCount);

		for (int i = 0; i < vehicleCount; i++)
		{
			PS_VehicleData vehicle = new PS_VehicleData();
			reader.ReadInt(vehicle.m_iRplId);
			reader.ReadString(vehicle.m_sName);
			reader.ReadString(vehicle.m_sFactionKey);
			reader.ReadInt(vehicle.m_iGroupId);
			reader.ReadBool(vehicle.m_bLocked);

			m_aVehicles.Insert(vehicle);
		}

		// --- Player Names ---
		int nameCount;
		reader.ReadInt(nameCount);

		for (int i = 0; i < nameCount; i++)
		{
			int playerId;
			string name;
			reader.ReadInt(playerId);
			reader.ReadString(name);
			m_mPlayerNames.Set(playerId, name);
		}

		Print(string.Format("[PS_Lobby] RplLoad: %1 slots, %2 vehicles, %3 names",
			slotCount, vehicleCount, nameCount), LogLevel.NORMAL);

		// WHY: fire events so UI rebuilds from JIP state.
		// Use CallLater(0) to defer until after all RplLoad is complete.
		GetGame().GetCallqueue().CallLater(FireJIPEvents, 0, false);

		return true;
	}

	// WHY deferred: during RplLoad, not all components may be initialized yet.
	// Firing events after the current frame ensures listeners are ready.
	protected void FireJIPEvents()
	{
		foreach (PS_SlotData slot : m_aSlots)
		{
			m_OnSlotRegistered.Invoke(slot);
		}

		foreach (PS_VehicleData vehicle : m_aVehicles)
		{
			m_OnVehicleRegistered.Invoke(vehicle);
		}

		for (int i = 0; i < m_mPlayerNames.Count(); i++)
		{
			m_OnPlayerNameUpdated.Invoke(m_mPlayerNames.GetKey(i), m_mPlayerNames.GetElement(i));
		}

		Print("[PS_Lobby] JIP events fired", LogLevel.NORMAL);
	}

	// =====================================================================
	// PLAYER LIFECYCLE — called by game mode component callbacks
	// =====================================================================

	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		Print(string.Format("[PS_Lobby] LobbyManager: player %1 connected", playerId), LogLevel.NORMAL);

		if (!Replication.IsServer())
			return;

		// WHY: check if this is a reconnecting player with a reserved slot.
		// If so, automatically reassign them to their old slot.
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;

		string guid = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		if (guid == "")
			return;

		int reservedSlotRplId = GetReconnectSlot(guid);
		if (reservedSlotRplId < 0)
			return;

		// Found a reservation — try to reassign.
		PS_SlotData slot = FindSlotByRplId(reservedSlotRplId);
		if (slot && slot.IsEmpty() && !slot.m_bLocked)
		{
			Print(string.Format("[PS_Lobby] Reconnect: reassigning player %1 to slot %2 (GUID: %3)",
				playerId, reservedSlotRplId, guid), LogLevel.NORMAL);

			// WHY: defer slightly so the player's controller component is ready.
			GetGame().GetCallqueue().CallLater(ReconnectPlayer, 500, false, playerId, reservedSlotRplId);
		}
		else
		{
			Print(string.Format("[PS_Lobby] Reconnect: slot %1 no longer available for player %2",
				reservedSlotRplId, playerId), LogLevel.WARNING);
		}
	}

	// Deferred reconnect — called after player's controller is initialized.
	protected void ReconnectPlayer(int playerId, int slotRplId)
	{
		TakeSlot_S(playerId, slotRplId);

		// WHY: also assign VoN channel based on current game state.
		PS_VoNChannelsManager vonMgr = PS_VoNChannelsManager.GetInstance();
		PS_GameModeCoop gameMode = PS_GameModeCoop.GetInstance();
		if (vonMgr && gameMode)
		{
			PS_SlotData slot = FindSlotByRplId(slotRplId);
			if (slot)
			{
				SCR_EGameModeState state = gameMode.GetLobbyState();
				string channelKey;

				switch (state)
				{
					case SCR_EGameModeState.SLOTSELECTION:
						channelKey = PS_VoNChannelsManager.GetFactionChannelKey(slot.m_sFactionKey);
						break;
					case SCR_EGameModeState.BRIEFING:
					case SCR_EGameModeState.GAME:
						channelKey = PS_VoNChannelsManager.GetGroupChannelKey(slot.m_iGroupId, slot.m_sFactionKey);
						break;
					default:
						channelKey = PS_VoNChannelsManager.GetFactionChannelKey(slot.m_sFactionKey);
						break;
				}

				vonMgr.SetPlayerChannel_S(playerId, channelKey);
			}
		}
	}

	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);

		if (!Replication.IsServer())
			return;

		// WHY: reserve the player's slot for reconnection.
		PS_SlotData slot = FindSlotByPlayerId(playerId);
		if (slot)
		{
			// Get the player's GUID for reconnect tracking.
			PlayerManager pm = GetGame().GetPlayerManager();
			string guid = "";
			if (pm)
				guid = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);

			if (guid != "")
			{
				m_mDisconnectedPlayers.Set(guid, slot.m_iRplId);
				Print(string.Format("[PS_Lobby] Reserved slot %1 for reconnecting player %2 (GUID: %3)",
					slot.m_iRplId, playerId, guid), LogLevel.NORMAL);

				// WHY: schedule cleanup after reconnect timeout.
				PS_GameModeCoop gameMode = PS_GameModeCoop.GetInstance();
				int reconnectTime = 120000; // default 2 min
				if (gameMode)
					reconnectTime = gameMode.GetReconnectTime();

				if (reconnectTime > 0)
				{
					GetGame().GetCallqueue().CallLater(ClearReconnectReservation, reconnectTime, false, guid);
				}
			}

			// Release the slot assignment but keep the reservation.
			LeaveSlot_S(playerId);
		}

		Print(string.Format("[PS_Lobby] LobbyManager: player %1 disconnected", playerId), LogLevel.NORMAL);
	}

	// WHY: called when reconnect timer expires. If the player hasn't rejoined, free the slot.
	protected void ClearReconnectReservation(string guid)
	{
		if (m_mDisconnectedPlayers.Contains(guid))
		{
			Print(string.Format("[PS_Lobby] Reconnect reservation expired for GUID: %1", guid), LogLevel.NORMAL);
			m_mDisconnectedPlayers.Remove(guid);
		}
	}

	// Check if a reconnecting player has a reserved slot. Returns slot RplId or -1.
	int GetReconnectSlot(string playerGUID)
	{
		int slotRplId;
		if (m_mDisconnectedPlayers.Find(playerGUID, slotRplId))
		{
			m_mDisconnectedPlayers.Remove(playerGUID);
			return slotRplId;
		}
		return -1;
	}
}
