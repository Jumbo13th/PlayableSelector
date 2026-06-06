void PS_ScriptInvokerFactionChangeMethod(int playerId, FactionKey factionKey, FactionKey factionKeyOld);
typedef func PS_ScriptInvokerFactionChangeMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerFactionChangeMethod> PS_ScriptInvokerFactionChange;

void PS_ScriptInvokerPlayableMethod(RplId id, PS_PlayableContainer playableComponent);
typedef func PS_ScriptInvokerPlayableMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerPlayableMethod> PS_ScriptInvokerPlayable;

void PS_ScriptInvokerPinChangeMethod(int playerId, bool pin);
typedef func PS_ScriptInvokerPinChangeMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerPinChangeMethod> PS_ScriptInvokerPinChange;

void PS_ScriptInvokerPlayerStateChangeMethod(int playerId, PS_EPlayableControllerState state);
typedef func PS_ScriptInvokerPlayerStateChangeMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerPlayerStateChangeMethod> PS_ScriptInvokerPlayerStateChange;

void PS_ScriptInvokerPlayerPlayableChangeMethod(int playerId, RplId playbleId);
typedef func PS_ScriptInvokerPlayerPlayableChangeMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerPlayerPlayableChangeMethod> PS_ScriptInvokerPlayerPlayableChange;

void PS_ScriptInvokerPlayableChangeGroupMethod(RplId id, PS_PlayableContainer playableComponent, SCR_AIGroup aiGroup);
typedef func PS_ScriptInvokerPlayableChangeGroupMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerPlayableChangeGroupMethod> PS_ScriptInvokerPlayableChangeGroup;

void PS_ScriptInvokerFactionReadyChangeMethod(FactionKey factionKey, int readyValue);
typedef func PS_ScriptInvokerFactionReadyChangeMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerFactionReadyChangeMethod> PS_ScriptInvokerFactionReadyChangeGroup;

[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_PlayableManagerClass : ScriptComponentClass
{

}

class PS_PlayableManager : ScriptComponent
{
	// ============================================================================================
	// ======================================= Replicated State ===================================
	// ============================================================================================

	[RplProp(onRplName: "OnPlayablesReplicated")]
	ref PS_ReplicatedClassMap<RplId, ref PS_PlayableContainer> m_aPlayables = new PS_ReplicatedClassMap<RplId, ref PS_PlayableContainer>();

	[RplProp()]
	ref PS_ReplicatedClassMap<RplId, ref PS_PlayableVehicleContainer> m_mPlayableVehicles = new PS_ReplicatedClassMap<RplId, ref PS_PlayableVehicleContainer>();

	[RplProp()]
	ref PS_ReplicatedBasicMap<int, int> m_playersStates = new PS_ReplicatedBasicMap<int, int>();

	[RplProp()]
	ref PS_ReplicatedBasicMap<int, RplId> m_playersPlayable = new PS_ReplicatedBasicMap<int, RplId>();

	[RplProp()]
	ref PS_ReplicatedBasicMap<RplId, int> m_playablePlayers = new PS_ReplicatedBasicMap<RplId, int>();

	[RplProp()]
	ref PS_ReplicatedBasicMap<int, bool> m_playersPin = new PS_ReplicatedBasicMap<int, bool>();

	[RplProp()]
	ref PS_ReplicatedBasicMap<int, FactionKey> m_playersFaction = new PS_ReplicatedBasicMap<int, FactionKey>();

	[RplProp()]
	ref PS_ReplicatedBasicMap<int, FactionKey> m_playersFactionRemembered = new PS_ReplicatedBasicMap<int, FactionKey>();

	[RplProp()]
	ref PS_ReplicatedBasicMap<RplId, int> m_playablePlayerGroupId = new PS_ReplicatedBasicMap<RplId, int>();

	[RplProp()]
	ref PS_ReplicatedBasicMap<int, string> m_playersLastName = new PS_ReplicatedBasicMap<int, string>();

	[RplProp()]
	ref PS_ReplicatedBasicMap<int, RplId> m_playersPlayableRemembered = new PS_ReplicatedBasicMap<int, RplId>();

	[RplProp()]
	ref PS_ReplicatedBasicMap<RplId, int> m_playablePlayersRemembered = new PS_ReplicatedBasicMap<RplId, int>();

	[RplProp()]
	ref PS_ReplicatedBasicMap<FactionKey, int> m_mFactionReady = new PS_ReplicatedBasicMap<FactionKey, int>();

	[RplProp()]
	ref PS_ReplicatedBasicMap<RplId, string> m_mPlayablePrefabs = new PS_ReplicatedBasicMap<RplId, string>();

	[RplProp()]
	ref PS_ReplicatedBasicMap<RplId, string> m_mPlayableNames = new PS_ReplicatedBasicMap<RplId, string>();

	[RplProp()]
	int m_iMaxPlayersCount = 1;

	[RplProp(onRplName: "OnStartTimerCounterChanged")]
	int m_iStartTimerCounter = -1;

	// ============================================================================================
	// ======================================= Non-Replicated =====================================
	// ============================================================================================

	ref array<PS_PlayableContainer> m_aPlayablesSorted = {};

	protected ref map<string, int> m_mGUIDtoPlayerId = new map<string, int>();
	protected ref map<int, string> m_mPlayerIdToGUID = new map<int, string>();
	protected ref map<string, int> m_mDisconnectedGUIDs = new map<string, int>();

	protected ref array<PS_PlayableComponent> m_aRegistrationQueue = {};
	protected static const int REGISTRATION_INTERVAL_MS = 50;

	bool m_bFactionsReadySended;
	bool m_bRplLoaded = false;

	protected PS_GameModeCoop m_GameModeCoop;
	protected ScriptCallQueue m_CallQueue;
	protected PlayerManager m_PlayerManager;
	protected SCR_PlayerController m_CurrentPlayerController;
	static protected PS_PlayableControllerComponent s_CurrentPlayableController;
	protected static PS_PlayableManager s_Instance;

	// ============================================================================================
	// ======================================= Invokers ===========================================
	// ============================================================================================

	ref ScriptInvokerInt m_eOnPlayerConnected = new ScriptInvokerInt();
	ScriptInvokerInt GetOnPlayerConnected() { return m_eOnPlayerConnected; }

	ref ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected> m_eOnPlayerDisconnected = new ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected>();
	ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected> GetOnPlayerDisconnected() { return m_eOnPlayerDisconnected; }

	ref PS_ScriptInvokerFactionChange m_eOnFactionChange = new PS_ScriptInvokerFactionChange();
	PS_ScriptInvokerFactionChange GetOnFactionChange() { return m_eOnFactionChange; }

	ref PS_ScriptInvokerPlayable m_eOnPlayableRegistered = new PS_ScriptInvokerPlayable();
	PS_ScriptInvokerPlayable GetOnPlayableRegistered() { return m_eOnPlayableRegistered; }

	ref PS_ScriptInvokerPlayable m_eOnPlayableUnregistered = new PS_ScriptInvokerPlayable();
	PS_ScriptInvokerPlayable GetOnPlayableUnregistered() { return m_eOnPlayableUnregistered; }

	ref PS_ScriptInvokerPinChange m_eOnPlayerPinChange = new PS_ScriptInvokerPinChange();
	PS_ScriptInvokerPinChange GetOnPlayerPinChange() { return m_eOnPlayerPinChange; }

	ref PS_ScriptInvokerPlayerStateChange m_eOnPlayerStateChange = new PS_ScriptInvokerPlayerStateChange();
	PS_ScriptInvokerPlayerStateChange GetOnPlayerStateChange() { return m_eOnPlayerStateChange; }

	ref PS_ScriptInvokerPlayerPlayableChange m_eOnPlayerPlayableChange = new PS_ScriptInvokerPlayerPlayableChange();
	PS_ScriptInvokerPlayerPlayableChange GetOnPlayerPlayableChange() { return m_eOnPlayerPlayableChange; }

	ref PS_ScriptInvokerPlayableChangeGroup m_eOnPlayableChangeGroup = new PS_ScriptInvokerPlayableChangeGroup();
	PS_ScriptInvokerPlayableChangeGroup GetOnPlayableChangeGroup() { return m_eOnPlayableChangeGroup; }

	ref ScriptInvokerInt m_eOnStartTimerCounterChanged = new ScriptInvokerInt();
	ScriptInvokerInt GetOnStartTimerCounterChanged() { return m_eOnStartTimerCounterChanged; }

	ref PS_ScriptInvokerFactionReadyChangeGroup m_eFactionReadyChanged = new PS_ScriptInvokerFactionReadyChangeGroup();
	PS_ScriptInvokerFactionReadyChangeGroup GetOnFactionReadyChanged() { return m_eFactionReadyChanged; }

	// ============================================================================================
	// ======================================= Lifecycle ==========================================
	// ============================================================================================

	static PS_PlayableManager GetInstance() { return s_Instance; }
	bool IsReplicated() { return m_bRplLoaded; }

	override protected void OnPostInit(IEntity owner)
	{
		s_Instance = this;

		m_GameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		m_CallQueue = GetGame().GetCallqueue();
		m_PlayerManager = GetGame().GetPlayerManager();

		if (Replication.IsServer())
			m_bRplLoaded = true;
		if (RplSession.Mode() == RplMode.Dedicated)
			ForceGetSessionMaxPlayersCount();

		m_GameModeCoop.GetOnPlayerConnected().Insert(OnPlayerConnected);
		m_GameModeCoop.GetOnPlayerDisconnected().Insert(OnPlayerDisconnected);
		m_GameModeCoop.GetOnPlayerRoleChange().Insert(OnPlayerRoleChange);
		m_CallQueue.Call(LateInit, owner);
	}

	protected void LateInit(IEntity owner)
	{
		if (RplSession.Mode() == RplMode.Dedicated)
			return;

		m_CurrentPlayerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!m_CurrentPlayerController)
		{
			m_CallQueue.Call(LateInit, owner);
			return;
		}
		s_CurrentPlayableController = m_CurrentPlayerController.PS_GetPLayableComponent();

		if (!m_bRplLoaded)
			m_CallQueue.CallLater(MarkRplLoaded, 500, false);
	}

	protected void MarkRplLoaded()
	{
		m_bRplLoaded = true;
	}

	protected void OnPlayablesReplicated()
	{
		m_bRplLoaded = true;
		UpdatePlayablesSorted();

		foreach (RplId playableId, PS_PlayableContainer container : m_aPlayables.GetRawMap())
		{
			m_CallQueue.Call(OnPlayableRegisteredLateInvoke, playableId, container);
		}
	}

	protected void ForceGetSessionMaxPlayersCount()
	{
		DSSession dSSession = GetGame().GetBackendApi().GetDSSession();
		if (dSSession)
		{
			int playerLimit = dSSession.PlayerLimit();
			if (m_iMaxPlayersCount != playerLimit)
			{
				m_iMaxPlayersCount = playerLimit;
				Replication.BumpMe();
			}
		}
		else
			m_CallQueue.Call(ForceGetSessionMaxPlayersCount);
	}

	void OnStartTimerCounterChanged()
	{
		m_eOnStartTimerCounterChanged.Invoke(m_iStartTimerCounter);
	}

	void StartTime()
	{
		m_iStartTimerCounter -= 1;
		Replication.BumpMe();
		OnStartTimerCounterChanged();
		if (m_iStartTimerCounter == 0)
		{
			PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			gameModeCoop.AdvanceGameState(SCR_EGameModeState.SLOTSELECTION);
			m_CallQueue.Remove(StartTime);
		}
	}

	// ============================================================================================
	// ==================================== Main Entry Point ======================================
	// ============================================================================================

	void ApplyPlayable(int playerId)
	{
		SCR_PlayerController playerController = SCR_PlayerController.Cast(m_PlayerManager.GetPlayerController(playerId));
		if (!playerController)
			return;
		PS_PlayableControllerComponent playableController = playerController.PS_GetPLayableComponent();
		SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();

		SetPlayerState(playerId, PS_EPlayableControllerState.Playing);

		RplId playableId = GetPlayableByPlayer(playerId);
		if (playableId != RplId.Invalid())
		{
			PS_PlayableContainer playableContainer = GetPlayableById(playableId);
			if (playableContainer && playableContainer.GetDamageState() == EDamageState.DESTROYED)
				m_CallQueue.CallLater(DelayedSwitchToInitialEntity, 1000, false, playerId);
		}

		IEntity entity;
		if (playableId == RplId.Invalid())
		{
			SCR_AIGroup currentGroup = groupsManagerComponent.GetPlayerGroup(playableId);
			if (currentGroup)
				currentGroup.RemovePlayer(playerId);

			m_CallQueue.CallLater(SetPlayerFactionKey, 200, false, playerId, "");

			entity = playableController.GetInitialEntity();
			if (!entity)
			{
				Resource resource = Resource.Load("{ADDE38E4119816AB}Prefabs/InitialPlayer_Version2.et");
				EntitySpawnParams params = new EntitySpawnParams();
				Math3D.MatrixIdentity4(params.Transform);
				params.Transform[3] = Vector(5000, 100000, 5000) + Vector(1000 * Math.Mod(playerId, 10), 5000 * Math.Floor(Math.Mod(playerId, 100) / 10), 5000 * Math.Floor(playerId / 100));
				entity = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
				playableController.SetInitialEntity(entity);
			}

			playerController.SetInitialMainEntity(entity);
			return;
		}
		else
			entity = GetPlayableById(playableId).GetPlayableComponent().GetOwner();

		IEntity initialEntity = playableController.GetInitialEntity();
		if (initialEntity)
			m_CallQueue.Call(SCR_EntityHelper.DeleteEntityAndChildren, initialEntity);

		playerController.SetInitialMainEntity(entity);

		SCR_ChimeraCharacter playableCharacter = SCR_ChimeraCharacter.Cast(entity);
		SCR_Faction faction = SCR_Faction.Cast(playableCharacter.GetFaction());
		SetPlayerFactionKey(playerId, faction.GetFactionKey());

		m_CallQueue.CallLater(ChangeGroup, 0, false, playerId, playableId);
	}

	protected void DelayedSwitchToInitialEntity(int playerId)
	{
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		gameModeCoop.SwitchToInitialEntity(playerId);
	}

	void ChangeGroup(int playerId, RplId playableId)
	{
		SCR_PlayerController playerController = SCR_PlayerController.Cast(m_PlayerManager.GetPlayerController(playerId));
		PS_PlayableControllerComponent playableController = playerController.PS_GetPLayableComponent();

		SCR_AIGroup playerGroup = GetPlayerGroupByPlayable(playableId);
		SCR_ChimeraCharacter leaderCharacter = null;
		if (playerGroup)
			leaderCharacter = SCR_ChimeraCharacter.Cast(playerGroup.GetLeaderEntity());
		PS_PlayableContainer playableContainerLeader;
		if (leaderCharacter)
			playableContainerLeader = leaderCharacter.PS_GetPlayable().GetPlayableContainer();

		SCR_PlayerControllerGroupComponent playerControllerGroupComponent = SCR_PlayerControllerGroupComponent.Cast(playerController.FindComponent(SCR_PlayerControllerGroupComponent));
		SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		if (playerGroup)
			playerControllerGroupComponent.PS_AskJoinGroup(playerGroup.GetGroupID());

		if (playerGroup && playerGroup.GetNameAuthorID() == -1)
			playerGroup.SetCustomName(playerGroup.GetCustomName(), playerId);

		if (playableContainerLeader)
			if (playableContainerLeader.GetRplId() > playableId)
				groupsManagerComponent.SetGroupLeader(playerGroup.GetGroupID(), playerId);
	}

	// ============================================================================================
	// ===================================== Registration =========================================
	// ============================================================================================

	void RegisterPlayable(PS_PlayableComponent playableComponent)
	{
		RplId playableId = playableComponent.GetRplId();
		if (m_aPlayables.Contains(playableId))
			return;
		SCR_ChimeraCharacter playableCharacter = playableComponent.GetCharacter();
		if (!playableCharacter.PS_GetChimeraAIControlComponent())
			return;

		m_aRegistrationQueue.Insert(playableComponent);
		if (m_aRegistrationQueue.Count() == 1)
			m_CallQueue.CallLater(ProcessRegistrationQueue, REGISTRATION_INTERVAL_MS, false);
	}

	protected void ProcessRegistrationQueue()
	{
		if (m_aRegistrationQueue.IsEmpty())
			return;

		PS_PlayableComponent playableComponent = m_aRegistrationQueue[0];
		m_aRegistrationQueue.RemoveOrdered(0);

		if (!playableComponent || !playableComponent.GetOwner())
		{
			if (!m_aRegistrationQueue.IsEmpty())
				m_CallQueue.CallLater(ProcessRegistrationQueue, REGISTRATION_INTERVAL_MS, false);
			return;
		}

		RplId playableId = playableComponent.GetRplId();
		if (m_aPlayables.Contains(playableId))
		{
			if (!m_aRegistrationQueue.IsEmpty())
				m_CallQueue.CallLater(ProcessRegistrationQueue, REGISTRATION_INTERVAL_MS, false);
			return;
		}

		RegisterPlayableImmediate(playableComponent);

		if (!m_aRegistrationQueue.IsEmpty())
			m_CallQueue.CallLater(ProcessRegistrationQueue, REGISTRATION_INTERVAL_MS, false);
	}

	protected void RegisterPlayableImmediate(PS_PlayableComponent playableComponent)
	{
		RplId playableId = playableComponent.GetRplId();
		SCR_ChimeraCharacter playableCharacter = playableComponent.GetCharacter();

		PS_PlayableContainer container = playableComponent.GetPlayableContainer();
		m_aPlayables.Set(playableId, container);
		SetPlayablePrefab(playableId, playableComponent.GetOwner().GetPrefabData().GetPrefabName());
		SetPlayableName(playableId, playableComponent.GetName());

		if (Replication.IsServer())
		{
			AIControlComponent aiControl = playableCharacter.PS_GetChimeraAIControlComponent();
			SCR_AIGroup playableGroup = SCR_AIGroup.Cast(aiControl.GetControlAIAgent().GetParentGroup());
			SCR_AIGroup playerGroup;

			if (!playableGroup)
				return;

			if (!playableGroup.m_PlayersGroup)
			{
				SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
				playerGroup = groupsManagerComponent.CreateNewPlayableGroup(playableGroup.GetFaction());

				playerGroup.m_BotsGroup = playableGroup;
				playableGroup.m_PlayersGroup = playerGroup;

				playerGroup.SetMaxMembers(playableGroup.m_aUnitPrefabSlots.Count());
				playerGroup.SetCustomName(playableGroup.GetCustomName(), -1);
				playableGroup.SetCanDeleteIfNoPlayer(false);
				playerGroup.SetCanDeleteIfNoPlayer(false);
				playableGroup.SetDeleteWhenEmpty(false);
				playerGroup.SetDeleteWhenEmpty(false);
			} else {
				playerGroup = playableGroup.m_PlayersGroup;
			}
			SetPlayablePlayerGroupId(playableId, playerGroup.GetGroupID());
			m_CallQueue.Call(UpdateGroupCallsign, playableId, playerGroup, playableGroup)
		}

		Replication.BumpMe();

		UpdatePlayablesSortedDelayed();
		m_CallQueue.Call(OnPlayableRegisteredLateInvoke, playableId, container);
		Rpc(RPC_NotifyPlayableRegistered, playableId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_NotifyPlayableRegistered(RplId playableId)
	{
		m_CallQueue.Call(OnPlayableRegisteredClient, playableId, 0);
	}

	protected void OnPlayableRegisteredClient(RplId playableId, int retryCount)
	{
		PS_PlayableContainer container = m_aPlayables.Get(playableId);
		if (!container && retryCount < 30)
		{
			m_CallQueue.CallLater(OnPlayableRegisteredClient, 100, false, playableId, retryCount + 1);
			return;
		}
		if (!container)
			return;

		UpdatePlayablesSortedDelayed();
		m_CallQueue.Call(OnPlayableRegisteredLateInvoke, playableId, container);
	}

	protected void OnPlayableRegisteredLateInvoke(RplId playableId, PS_PlayableContainer playableComponent)
	{
		m_CallQueue.Call(OnPlayableRegisteredLateInvoke2, playableId, playableComponent);
	}
	protected void OnPlayableRegisteredLateInvoke2(RplId playableId, PS_PlayableContainer playableComponent)
	{
		m_eOnPlayableRegistered.Invoke(playableId, playableComponent);
	}

	protected void UpdateGroupCallsign(RplId playableId, SCR_AIGroup playerGroup, SCR_AIGroup playableGroup)
	{
		PS_GroupCallsignAssigner groupCallsignAssigner = PS_GroupCallsignAssigner.Cast(playableGroup.FindComponent(PS_GroupCallsignAssigner));
		int company, platoon, squad;
		if (groupCallsignAssigner) {
			groupCallsignAssigner.GetCallsign(company, platoon, squad);
		} else {
			SCR_CallsignGroupComponent callsignComponent = SCR_CallsignGroupComponent.Cast(playableGroup.FindComponent(SCR_CallsignGroupComponent));
			callsignComponent.GetCallsignIndexes(company, platoon, squad);
		}
		SCR_CallsignGroupComponent callsignComponent = SCR_CallsignGroupComponent.Cast(playerGroup.FindComponent(SCR_CallsignGroupComponent));
		callsignComponent.ReAssignGroupCallsign(company, platoon, squad);

		m_CallQueue.CallLater(RegisterGroupName, 0, false, playableId, playerGroup)
	}

	protected void RegisterGroupName(RplId playableId, SCR_AIGroup playerGroup)
	{
		SCR_CallsignGroupComponent callsignComponent = SCR_CallsignGroupComponent.Cast(playerGroup.FindComponent(SCR_CallsignGroupComponent));
		int company, platoon, squad;
		callsignComponent.GetCallsignIndexes(company, platoon, squad);
		int groupCallsign = 1000000 * company + 1000 * platoon + 1 * squad;

		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		VoNRoomsManager.GetOrCreateRoomWithFaction(playerGroup.GetFaction().GetFactionKey(), groupCallsign.ToString());
		VoNRoomsManager.GetOrCreateRoomWithFaction(playerGroup.GetFaction().GetFactionKey(), "#PS-VoNRoom_Command");
		VoNRoomsManager.GetOrCreateRoomWithFaction(playerGroup.GetFaction().GetFactionKey(), "#PS-VoNRoom_Faction");
	}

	void UnRegisterPlayable(RplId playableId)
	{
		if (!m_aPlayables.Contains(playableId))
			return;
		PS_PlayableContainer playableContainer = m_aPlayables.Get(playableId);

		Rpc(RPC_NotifyPlayableUnregistered, playableId);
		m_eOnPlayableUnregistered.Invoke(playableId, playableContainer);
		playableContainer.m_eOnUnregister.Invoke();

		m_aPlayables.Remove(playableId);
		Replication.BumpMe();
		UpdatePlayablesSorted();
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_NotifyPlayableUnregistered(RplId playableId)
	{
		PS_PlayableContainer container = m_aPlayables.Get(playableId);
		if (container)
		{
			m_eOnPlayableUnregistered.Invoke(playableId, container);
			container.m_eOnUnregister.Invoke();
		}
		UpdatePlayablesSorted();
	}

	// ============================================================================================
	// ==================================== Vehicle Registration ==================================
	// ============================================================================================

	void RegisterGroupVehicle(RplId rplId, SCR_AIGroup group, IEntity vehicle)
	{
		if (!Replication.IsServer())
			return;
		if (!group.m_PlayersGroup)
		{
			m_CallQueue.Call(RegisterGroupVehicle, rplId, group, vehicle);
			return;
		}
		int groupCallsign = group.GetCallsignNum();
		PS_PlayableVehicleContainer playableVehicleContainer = new PS_PlayableVehicleContainer();
		SCR_EditableVehicleComponent editableVehicleComponent = SCR_EditableVehicleComponent.Cast(vehicle.FindComponent(SCR_EditableVehicleComponent));
		SCR_VehicleFactionAffiliationComponent vehicleFactionAffiliationComponent = SCR_VehicleFactionAffiliationComponent.Cast(vehicle.FindComponent(SCR_VehicleFactionAffiliationComponent));
		SCR_UIInfo uIInfo = editableVehicleComponent.GetInfo();
		ResourceName prefab = vehicle.GetPrefabData().GetPrefabName();
		if (prefab == "")
			prefab = vehicle.GetPrefabData().GetPrefab().GetAncestor().GetResourceName();
		if (prefab == "")
			prefab = vehicle.GetPrefabData().GetPrefab().GetAncestor().GetAncestor().GetResourceName();
		playableVehicleContainer.Init(rplId, prefab, uIInfo.GetIconPath(), groupCallsign, group.m_PlayersGroup.GetGroupID(), vehicleFactionAffiliationComponent.GetDefaultFactionKey());

		m_mPlayableVehicles.Set(rplId, playableVehicleContainer);
		Replication.BumpMe();
	}

	void UnRegisterGroupVehicle(RplId rplId)
	{
		if (!Replication.IsServer())
			return;
		m_mPlayableVehicles.Remove(rplId);
		Replication.BumpMe();
	}

	// ============================================================================================
	// ======================================= Accessors ==========================================
	// ============================================================================================

	PS_PlayableContainer GetPlayableById(RplId PlayableId)
	{
		return m_aPlayables.Get(PlayableId);
	}

	map<RplId, ref PS_PlayableContainer> GetPlayables()
	{
		return m_aPlayables.GetRawMap();
	}

	array<PS_PlayableContainer> GetPlayablesSorted()
	{
		return m_aPlayablesSorted;
	}

	map<RplId, ref PS_PlayableVehicleContainer> GetPlayableVehicles()
	{
		return m_mPlayableVehicles.GetRawMap();
	}

	// ============================================================================================
	// ==================================== Player Faction ========================================
	// ============================================================================================

	FactionKey GetPlayerFactionKey(int playerId)
	{
		if (!m_playersFaction.Contains(playerId))
			return "";
		return m_playersFaction.Get(playerId);
	}

	FactionKey GetPlayerFactionKeyRemembered(int playerId)
	{
		if (!m_playersFactionRemembered.Contains(playerId))
			return "";
		return m_playersFactionRemembered.Get(playerId);
	}

	void SetPlayerFactionKey(int playerId, FactionKey factionKey)
	{
		FactionKey factionKeyOld = GetPlayerFactionKey(playerId);
		m_playersFaction.Set(playerId, factionKey);
		if (factionKey != "")
			m_playersFactionRemembered.Set(playerId, factionKey);
		Replication.BumpMe();

		m_eOnFactionChange.Invoke(playerId, factionKey, factionKeyOld);
		Rpc(RPC_NotifyFactionChanged, playerId, factionKey, factionKeyOld);

		PlayerController playerController = m_PlayerManager.GetPlayerController(playerId);
		if (!playerController)
			return;
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		SCR_PlayerFactionAffiliationComponent playerFactionAffiliation = SCR_PlayerFactionAffiliationComponent.Cast(playerController.FindComponent(SCR_PlayerFactionAffiliationComponent));
		playerFactionAffiliation.SetAffiliatedFactionByKey(factionKey);
		factionManager.UpdatePlayerFaction_S(playerFactionAffiliation);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_NotifyFactionChanged(int playerId, FactionKey factionKey, FactionKey factionKeyOld)
	{
		m_eOnFactionChange.Invoke(playerId, factionKey, factionKeyOld);
	}

	// ============================================================================================
	// ==================================== Player State ==========================================
	// ============================================================================================

	PS_EPlayableControllerState GetPlayerState(int playerId)
	{
		if (!m_playersStates.Contains(playerId))
			return PS_EPlayableControllerState.NotReady;
		return m_playersStates.Get(playerId);
	}

	void SetPlayerState(int playerId, PS_EPlayableControllerState state)
	{
		m_playersStates.Set(playerId, state);
		Replication.BumpMe();

		m_eOnPlayerStateChange.Invoke(playerId, state);
		RplId playableId = GetPlayableByPlayer(playerId);
		if (playableId != RplId.Invalid())
		{
			PS_PlayableContainer playableContainer = m_aPlayables.Get(playableId);
			if (playableContainer)
				playableContainer.GetOnPlayerStateChange().Invoke(state);
		}

		Rpc(RPC_NotifyPlayerStateChanged, playerId, state);

		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		SCR_EGameModeState gameModeState = gameModeCoop.GetState();
		if (gameModeState == SCR_EGameModeState.SLOTSELECTION)
		{
			m_CallQueue.Remove(StartTime);
			bool adminExist = !gameModeCoop.IsAdminMode();
			array<int> players = {};
			GetGame().GetPlayerManager().GetPlayers(players);
			foreach (int otherPlayerId : players)
			{
				if (!adminExist)
					adminExist = SCR_Global.IsAdmin(otherPlayerId);

				PS_EPlayableControllerState playerState = GetPlayerState(otherPlayerId);
				if (playerState != PS_EPlayableControllerState.Ready)
				{
					if (m_iStartTimerCounter != -1)
					{
						m_iStartTimerCounter = -1;
						Replication.BumpMe();
						OnStartTimerCounterChanged();
					}
					return;
				}
			}

			if (adminExist)
			{
				m_iStartTimerCounter = 3;
				Replication.BumpMe();
				OnStartTimerCounterChanged();
				m_CallQueue.CallLater(StartTime, 1000, true);
			}
		}
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_NotifyPlayerStateChanged(int playerId, PS_EPlayableControllerState state)
	{
		m_eOnPlayerStateChange.Invoke(playerId, state);
		RplId playableId = GetPlayableByPlayer(playerId);
		if (playableId != RplId.Invalid())
		{
			PS_PlayableContainer playableContainer = m_aPlayables.Get(playableId);
			if (playableContainer)
				playableContainer.GetOnPlayerStateChange().Invoke(state);
		}
	}

	// ============================================================================================
	// ==================================== Player Name ===========================================
	// ============================================================================================

	string GetPlayerName(int playerId)
	{
		if (!m_playersLastName.Contains(playerId))
			return "";
		return m_playersLastName.Get(playerId);
	}

	void SetPlayerName(int playerId, string playerName)
	{
		m_playersLastName.Set(playerId, playerName);
		Replication.BumpMe();

		m_eOnPlayerConnected.Invoke(playerId);
		Rpc(RPC_NotifyPlayerName, playerId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_NotifyPlayerName(int playerId)
	{
		m_eOnPlayerConnected.Invoke(playerId);
	}

	// ============================================================================================
	// ==================================== Faction Ready =========================================
	// ============================================================================================

	int GetFactionReady(FactionKey factionKey)
	{
		if (!m_mFactionReady.Contains(factionKey))
			return 0;
		return m_mFactionReady.Get(factionKey);
	}

	void SetFactionReady(FactionKey factionKey, int readyValue)
	{
		m_mFactionReady.Set(factionKey, readyValue);
		Replication.BumpMe();

		m_eFactionReadyChanged.Invoke(factionKey, readyValue);
		Rpc(RPC_NotifyFactionReady, factionKey, readyValue);

		if (m_bFactionsReadySended)
			return;

		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		m_bFactionsReadySended = true;
		foreach (int playerId : players)
		{
			FactionKey playerFaction = GetPlayerFactionKey(playerId);
			if (playerFaction == "")
				continue;
			if (GetFactionReady(playerFaction))
				continue;
			m_bFactionsReadySended = false;
			break;
		}

		if (m_bFactionsReadySended)
		{
			SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
			ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("tmsg");
			invoker.Invoke(null, "Factions ready");
		}
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_NotifyFactionReady(FactionKey factionKey, int readyValue)
	{
		m_eFactionReadyChanged.Invoke(factionKey, readyValue);
	}

	// ============================================================================================
	// ==================================== Playable Prefab/Name ==================================
	// ============================================================================================

	string GetPlayablePrefab(RplId playableId)
	{
		if (!m_mPlayablePrefabs.Contains(playableId))
			return "";
		return m_mPlayablePrefabs.Get(playableId);
	}

	void SetPlayablePrefab(RplId playableId, ResourceName prefab)
	{
		m_mPlayablePrefabs.Set(playableId, prefab);
		Replication.BumpMe();
	}

	string GetPlayableName(RplId playableId)
	{
		if (!m_mPlayableNames.Contains(playableId))
			return "";
		return m_mPlayableNames.Get(playableId);
	}

	void SetPlayableName(RplId playableId, string name)
	{
		m_mPlayableNames.Set(playableId, name);
		Replication.BumpMe();
	}

	// ============================================================================================
	// ============================== Player <-> Playable Links ===================================
	// ============================================================================================

	int GetPlayerByPlayable(RplId PlayableId)
	{
		if (!m_playablePlayers.Contains(PlayableId))
			return -1;
		return m_playablePlayers.Get(PlayableId);
	}

	int GetPlayerByPlayableRemembered(RplId PlayableId)
	{
		if (!m_playablePlayersRemembered.Contains(PlayableId))
			return -1;
		return m_playablePlayersRemembered.Get(PlayableId);
	}

	RplId GetPlayableByPlayer(int playerId)
	{
		if (!m_playersPlayable.Contains(playerId))
			return RplId.Invalid();
		return m_playersPlayable.Get(playerId);
	}

	RplId GetPlayableByPlayerRemembered(int playerId)
	{
		if (!m_playersPlayableRemembered.Contains(playerId))
			return RplId.Invalid();
		return m_playersPlayableRemembered.Get(playerId);
	}

	void SetPlayablePlayer(RplId playableId, int playerId)
	{
		RplId oldPlayable = RplId.Invalid();
		if (playerId > 0) {
			oldPlayable = GetPlayableByPlayer(playerId);
			if (oldPlayable != RplId.Invalid())
			{
				m_playablePlayers.Set(oldPlayable, -1);
			}
			PS_PlayableContainer playableComponent = m_aPlayables.Get(oldPlayable);
			if (playableComponent)
				playableComponent.InvokeOnPlayerChanged(playerId, -1);
		}

		m_playersPlayable.Set(playerId, playableId);
		int oldPlayerId = m_playablePlayers.Get(playableId);
		m_playablePlayers.Set(playableId, playerId);
		if (oldPlayerId > 0 && oldPlayerId != playerId)
			m_playersPlayable.Set(oldPlayerId, RplId.Invalid());

		if (playableId != RplId.Invalid())
			m_playersPlayableRemembered.Set(playerId, playableId);

		if (playerId > 0)
		{
			m_playablePlayersRemembered.Set(playableId, playerId);
			m_eOnPlayerPlayableChange.Invoke(playerId, playableId);
		}

		PS_PlayableContainer playableContainer = m_aPlayables.Get(playableId);
		if (playableContainer)
			playableContainer.InvokeOnPlayerChanged(oldPlayerId, playerId);

		Replication.BumpMe();
		Rpc(RPC_NotifyPlayerPlayableChanged, playerId, playableId, oldPlayerId, oldPlayable);
	}

	void SetPlayerPlayable(int playerId, RplId playableId)
	{
		RplId oldPlayable = GetPlayableByPlayer(playerId);
		if (oldPlayable != RplId.Invalid()) {
			m_playablePlayers.Set(oldPlayable, -1);
			PS_PlayableContainer playableComponent = m_aPlayables.Get(oldPlayable);
			if (playableComponent)
				playableComponent.InvokeOnPlayerChanged(playerId, -1);
		}

		m_playersPlayable.Set(playerId, playableId);
		int oldPlayerId = m_playablePlayers.Get(playableId);
		m_playablePlayers.Set(playableId, playerId);
		if (oldPlayerId > 0 && oldPlayerId != playerId)
			m_playersPlayable.Set(oldPlayerId, RplId.Invalid());

		if (playableId != RplId.Invalid())
			m_playersPlayableRemembered.Set(playerId, playableId);

		if (playerId > 0)
		{
			m_eOnPlayerPlayableChange.Invoke(playerId, playableId);
			m_playablePlayersRemembered.Set(playableId, playerId);
		}

		PS_PlayableContainer playableComponent = m_aPlayables.Get(playableId);
		if (playableComponent)
			playableComponent.InvokeOnPlayerChanged(oldPlayerId, playerId);

		Replication.BumpMe();
		Rpc(RPC_NotifyPlayerPlayableChanged, playerId, playableId, oldPlayerId, oldPlayable);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_NotifyPlayerPlayableChanged(int playerId, RplId playableId, int oldPlayerId, RplId oldPlayable)
	{
		if (oldPlayable != RplId.Invalid())
		{
			PS_PlayableContainer oldContainer = m_aPlayables.Get(oldPlayable);
			if (oldContainer)
				oldContainer.InvokeOnPlayerChanged(playerId, -1);
		}

		if (playerId > 0)
			m_eOnPlayerPlayableChange.Invoke(playerId, playableId);

		PS_PlayableContainer playableContainer = m_aPlayables.Get(playableId);
		if (playableContainer)
			playableContainer.InvokeOnPlayerChanged(oldPlayerId, playerId);
	}

	// ============================================================================================
	// =========================== Playable Group / Vehicle / Pin =================================
	// ============================================================================================

	static PS_PlayableControllerComponent GetPlayableController()
	{
		return s_CurrentPlayableController;
	}

	SCR_AIGroup GetPlayerGroupByPlayable(RplId PlayableId)
	{
		if (!m_playablePlayerGroupId.Contains(PlayableId))
			return null;
		SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		return groupsManagerComponent.FindGroup(m_playablePlayerGroupId.Get(PlayableId));
	}

	int GetGroupCallsignByPlayable(RplId PlayableId)
	{
		SCR_AIGroup group = GetPlayerGroupByPlayable(PlayableId);
		if (!group)
			return -1;
		return group.GetCallsignNum();
	}

	void SetPlayablePlayerGroupId(RplId PlayableId, int groupId)
	{
		m_playablePlayerGroupId.Set(PlayableId, groupId);
		Replication.BumpMe();

		UpdatePlayablesSorted();
		SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		m_eOnPlayableChangeGroup.Invoke(PlayableId, GetPlayableById(PlayableId), groupsManagerComponent.FindGroup(groupId));

		Rpc(RPC_NotifyGroupChanged, PlayableId, groupId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_NotifyGroupChanged(RplId PlayableId, int groupId)
	{
		UpdatePlayablesSorted();
		SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		m_eOnPlayableChangeGroup.Invoke(PlayableId, GetPlayableById(PlayableId), groupsManagerComponent.FindGroup(groupId));
	}

	SCR_AIGroup GetPlayerGroupByVehicle(PS_PlayableVehicleContainer playableVehicleContainer)
	{
		SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		return groupsManagerComponent.FindGroup(playableVehicleContainer.m_iGroupId);
	}

	void SetPlayableVehicleLocked(RplId vehicleId, bool lock)
	{
		PS_PlayableVehicleContainer container = m_mPlayableVehicles.Get(vehicleId);
		if (!container)
			return;
		container.SetLock(lock);
		Replication.BumpMe();
	}

	bool GetPlayerPin(int playerId)
	{
		if (!m_playersPin.Contains(playerId))
			return false;
		return m_playersPin.Get(playerId);
	}

	void SetPlayerPin(int playerId, bool pined)
	{
		m_playersPin.Set(playerId, pined);
		Replication.BumpMe();

		m_eOnPlayerPinChange.Invoke(playerId, pined);
		RplId playableId = GetPlayableByPlayer(playerId);
		if (playableId != RplId.Invalid())
		{
			PS_PlayableContainer playableComponent = m_aPlayables.Get(playableId);
			if (playableComponent)
				playableComponent.GetOnPlayerPinChange().Invoke(pined);
		}

		Rpc(RPC_NotifyPinChanged, playerId, pined);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_NotifyPinChanged(int playerId, bool pined)
	{
		m_eOnPlayerPinChange.Invoke(playerId, pined);
		RplId playableId = GetPlayableByPlayer(playerId);
		if (playableId != RplId.Invalid())
		{
			PS_PlayableContainer playableComponent = m_aPlayables.Get(playableId);
			if (playableComponent)
				playableComponent.GetOnPlayerPinChange().Invoke(pined);
		}
	}

	int GetMaxPlayers()
	{
		return m_iMaxPlayersCount;
	}

	// ============================================================================================
	// ================================== Client Requests =========================================
	// ============================================================================================

	void NotifyKick(int playerId)
	{
		SCR_PlayerController playerController = SCR_PlayerController.Cast(m_PlayerManager.GetPlayerController(playerId));
		if (!playerController)
			return;
		PS_PlayableControllerComponent playableController = playerController.PS_GetPLayableComponent();
		if (!playableController)
			return;
		playableController.NotifyKickOwner();
	}

	void ForceSwitch(int playerId)
	{
		SCR_PlayerController playerController = SCR_PlayerController.Cast(m_PlayerManager.GetPlayerController(playerId));
		if (!playerController)
			return;
		PS_PlayableControllerComponent playableController = playerController.PS_GetPLayableComponent();
		if (!playableController)
			return;
		playableController.SwitchToMenuServer(SCR_EGameModeState.GAME);
	}

	// ============================================================================================
	// ======================================= Events =============================================
	// ============================================================================================

	protected void OnPlayerConnected(int playerId)
	{
		RplId playableId = GetPlayableByPlayer(playerId);
		PS_PlayableContainer playableContainer = GetPlayableById(playableId);
		if (playableContainer)
			playableContainer.GetOnPlayerConnected().Invoke(playerId);
	}

	protected void OnPlayerDisconnected(int playerId, KickCauseCode cause = KickCauseCode.NONE, int timeout = -1)
	{
		Rpc(RPC_OnPlayerDisconnected, playerId, cause, timeout);
		RPC_OnPlayerDisconnected(playerId, cause, timeout);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		RplId playableId = GetPlayableByPlayer(playerId);
		PS_PlayableContainer playableContainer = GetPlayableById(playableId);
		if (playableContainer)
			playableContainer.GetOnPlayerDisconnected().Invoke(playerId);
		m_eOnPlayerDisconnected.Invoke(playerId, cause, timeout);
	}

	protected void OnPlayerRoleChange(int playerId, EPlayerRole roleFlags)
	{
		RplId playableId = GetPlayableByPlayer(playerId);
		PS_PlayableContainer playableContainer = GetPlayableById(playableId);
		if (playableContainer)
			playableContainer.GetOnPlayerRoleChange().Invoke(playerId, roleFlags);
	}

	// ============================================================================================
	// =================================== Damage State ===========================================
	// ============================================================================================

	void OnPlayableDamageStateChanged(RplId playableId, EDamageState damageState)
	{
		Rpc(RPC_OnPlayableDamageStateChanged, playableId, damageState);
		RPC_OnPlayableDamageStateChanged(playableId, damageState);
	}
	[RplRpc(RplChannel.Unreliable, RplRcver.Broadcast)]
	protected void RPC_OnPlayableDamageStateChanged(RplId playableId, EDamageState damageState)
	{
		PS_PlayableContainer container = m_aPlayables.Get(playableId);
		if (!container)
			return;
		container.OnDamageStateChanged(damageState);
	}

	// ============================================================================================
	// ================================ Reconnect ID Remapping ====================================
	// ============================================================================================

	void TrackPlayerGUID(int playerId)
	{
		if (!Replication.IsServer())
			return;
		string guid = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
		if (guid == "")
			return;
		m_mGUIDtoPlayerId[guid] = playerId;
		m_mPlayerIdToGUID[playerId] = guid;
	}

	void TrackPlayerDisconnect(int playerId)
	{
		if (!Replication.IsServer())
			return;
		string guid;
		if (!m_mPlayerIdToGUID.Find(playerId, guid))
			return;
		m_mDisconnectedGUIDs[guid] = playerId;
	}

	int TryHandleReconnect(int newPlayerId)
	{
		if (!Replication.IsServer())
			return -1;
		string guid = GetGame().GetBackendApi().GetPlayerIdentityId(newPlayerId);
		if (guid == "")
			return -1;
		int oldPlayerId;
		if (!m_mDisconnectedGUIDs.Find(guid, oldPlayerId))
			return -1;

		m_mDisconnectedGUIDs.Remove(guid);
		m_mGUIDtoPlayerId[guid] = newPlayerId;
		m_mPlayerIdToGUID.Remove(oldPlayerId);
		m_mPlayerIdToGUID[newPlayerId] = guid;

		if (oldPlayerId != newPlayerId)
			RemapPlayerIds(oldPlayerId, newPlayerId);

		return oldPlayerId;
	}

	protected void RemapPlayerIds(int oldPlayerId, int newPlayerId)
	{
		RPC_RemapPlayerIds(oldPlayerId, newPlayerId);
		Replication.BumpMe();
		Rpc(RPC_RemapPlayerIds, oldPlayerId, newPlayerId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_RemapPlayerIds(int oldPlayerId, int newPlayerId)
	{
		RemapKey(m_playersStates, oldPlayerId, newPlayerId);
		RemapKey(m_playersPlayable, oldPlayerId, newPlayerId);
		RemapKey(m_playersPlayableRemembered, oldPlayerId, newPlayerId);
		RemapKey(m_playersPin, oldPlayerId, newPlayerId);
		RemapKey(m_playersFaction, oldPlayerId, newPlayerId);
		RemapKey(m_playersFactionRemembered, oldPlayerId, newPlayerId);
		RemapKey(m_playersLastName, oldPlayerId, newPlayerId);

		for (int i = 0; i < m_playablePlayers.Count(); i++)
		{
			if (m_playablePlayers.GetElement(i) == oldPlayerId)
				m_playablePlayers.Set(m_playablePlayers.GetKey(i), newPlayerId);
		}
		for (int i = 0; i < m_playablePlayersRemembered.Count(); i++)
		{
			if (m_playablePlayersRemembered.GetElement(i) == oldPlayerId)
				m_playablePlayersRemembered.Set(m_playablePlayersRemembered.GetKey(i), newPlayerId);
		}

		PS_VoNRoomsManager vonManager = PS_VoNRoomsManager.GetInstance();
		if (vonManager)
			vonManager.RemapPlayerId(oldPlayerId, newPlayerId);
	}

	private void RemapKey(PS_ReplicatedBasicMap<int, int> m, int oldKey, int newKey) { int val; if (m.Find(oldKey, val)) { m.Set(newKey, val); m.Remove(oldKey); } }
	private void RemapKey(PS_ReplicatedBasicMap<int, RplId> m, int oldKey, int newKey) { RplId val; if (m.Find(oldKey, val)) { m.Set(newKey, val); m.Remove(oldKey); } }
	private void RemapKey(PS_ReplicatedBasicMap<int, bool> m, int oldKey, int newKey) { bool val; if (m.Find(oldKey, val)) { m.Set(newKey, val); m.Remove(oldKey); } }
	private void RemapKey(PS_ReplicatedBasicMap<int, FactionKey> m, int oldKey, int newKey) { FactionKey val; if (m.Find(oldKey, val)) { m.Set(newKey, val); m.Remove(oldKey); } }
	private void RemapKey(PS_ReplicatedBasicMap<int, string> m, int oldKey, int newKey) { string val; if (m.Find(oldKey, val)) { m.Set(newKey, val); m.Remove(oldKey); } }

	// ============================================================================================
	// ======================================= Utilities ==========================================
	// ============================================================================================

	void RemoveRedundantUnits()
	{
		for (int i = 0; i < m_aPlayables.Count(); i++)
		{
			PS_PlayableContainer playable = m_aPlayables.GetElement(i);
			if (GetPlayerByPlayable(playable.GetRplId()) == -2 || (GetPlayerByPlayable(playable.GetRplId()) <= 0 && m_GameModeCoop.GetRemoveRedundantUnits()))
			{
				SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playable.GetPlayableComponent().GetOwner());
				if (character)
				{
					SCR_EntityHelper.DeleteEntityAndChildren(character);
					m_CallQueue.Call(RemoveRedundantUnits);
					return;
				}
			}
		}

		foreach (RplId vehicleId, PS_PlayableVehicleContainer playableVehicleContainer : m_mPlayableVehicles.GetRawMap())
		{
			if (playableVehicleContainer.GetLock())
			{
				IEntity entity = IEntity.Cast(Replication.FindItem(playableVehicleContainer.GetRplId()));
				if (entity)
				{
					SCR_EntityHelper.DeleteEntityAndChildren(entity);
					m_CallQueue.Call(RemoveRedundantUnits);
					return;
				}
			}
		}
	}

	void HolsterWeapons()
	{
		if (!Replication.IsServer())
			return;

		foreach (RplId id, PS_PlayableContainer playable : m_aPlayables.GetRawMap())
		{
			playable.GetPlayableComponent().HolsterWeapon();
		}
	}

	protected void UpdatePlayablesSorted()
	{
		array<PS_PlayableContainer> playablesSorted = {};
		map<RplId, ref PS_PlayableContainer> playables = GetPlayables();

		foreach (RplId playableId, PS_PlayableContainer playable : playables)
		{
			if (!playable)
				continue;
			int callSign = GetGroupCallsignByPlayable(playable.GetRplId());
			bool isInserted = false;
			for (int s = 0; s < playablesSorted.Count(); s++)
			{
				PS_PlayableContainer playableS = playablesSorted[s];
				int callSignS = GetGroupCallsignByPlayable(playableS.GetRplId());

				bool rplIdGreater = playableS.GetRplId() > playable.GetRplId();
				bool rankEquival = playable.GetCharacterRank() == playableS.GetCharacterRank();
				bool rankGreater = playable.GetCharacterRank() > playableS.GetCharacterRank();
				bool callSignEquival = callSignS == callSign;
				bool callSignGreater = callSignS > callSign;

				if ((((rplIdGreater && rankEquival) || rankGreater) && callSignEquival) || callSignGreater) {
					playablesSorted.InsertAt(playable, s);
					isInserted = true;
					break;
				}
			}
			if (!isInserted)
				playablesSorted.Insert(playable);
		}

		m_aPlayablesSorted = playablesSorted;
	}

	protected void UpdatePlayablesSortedDelayed()
	{
		m_CallQueue.Remove(UpdatePlayablesSorted);
		m_CallQueue.Call(UpdatePlayablesSorted);
	}

	bool IsPlayerGroupLeader(int thisPlayerId)
	{
		if (thisPlayerId == -1)
			return false;

		RplId thisPlayableId = GetPlayableByPlayer(thisPlayerId);
		if (thisPlayableId == RplId.Invalid())
			return false;

		int thisGroupCallsign = GetGroupCallsignByPlayable(thisPlayableId);

		array<PS_PlayableContainer> playables = GetPlayablesSorted();
		foreach (PS_PlayableContainer playable : playables)
		{
			RplId playableId = playable.GetRplId();
			int playerId = GetPlayerByPlayable(playable.GetRplId());
			if (playerId <= 0)
				continue;
			if (playerId == thisPlayerId)
				return true;
			if (GetPlayerFactionKey(playerId) != GetPlayerFactionKey(thisPlayerId))
				continue;
			int groupCallsign = GetGroupCallsignByPlayable(playableId);
			if (thisGroupCallsign != groupCallsign)
				continue;
			return false;
		}

		return true;
	}
}
