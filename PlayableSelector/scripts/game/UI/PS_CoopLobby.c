// PS_CoopLobby — Main lobby menu.
// Menu preset: ChimeraMenuPreset.CoopLobby
// Layout: {9DECCA625D345B35}UI/Lobby/CoopLobby.layout
//
// Reads data from PS_LobbyManager. Sends actions via PS_LobbyPlayerComponent.
// Builds widget tree dynamically: factions → groups → character slots.
// Subscribes to manager events for live updates.

modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	CoopLobby
}

class PS_CoopLobby : MenuBase
{
	// Layout prefab paths for dynamically created widgets.
	protected ResourceName m_sRolesGroupPrefab = "{B45A0FA6883A7A0E}UI/Lobby/RolesGroup.layout";
	protected ResourceName m_sCharacterSelectorPrefab = "{3F761F63F1DF29D1}UI/Lobby/CharacterSelector.layout";
	protected ResourceName m_sFactionSelectorPrefab = "{DA22ED7112FA8028}UI/Lobby/FactionSelector.layout";
	protected ResourceName m_sPlayerSelectorPrefab = "{B55DD7054C5892AE}UI/Lobby/PlayerSelector.layout";

	// --- Cached systems ---
	protected PS_GameModeCoop m_GameModeCoop;
	protected PS_LobbyManager m_LobbyManager;
	protected PlayerManager m_PlayerManager;
	protected PlayerController m_PlayerController;
	protected SCR_FactionManager m_FactionManager;
	protected PS_LobbyPlayerComponent m_LobbyPlayerComponent;
	protected WorkspaceWidget m_wWorkspaceWidget;
	protected InputManager m_InputManager;
	protected int m_iPlayerId;

	// --- Widgets ---
	protected Widget m_wRoot;
	protected VerticalLayoutWidget m_wFactionList;
	protected VerticalLayoutWidget m_wRolesList;
	protected VerticalLayoutWidget m_wPlayersList;
	protected TextWidget m_wPlayersCounter;
	protected ButtonWidget m_wNavigationStart;
	protected ButtonWidget m_wNavigationClose;
	protected FrameWidget m_wGameModeHeader;
	protected ScrollLayoutWidget m_wRolesScroll;

	// --- Handlers ---
	protected PS_GameModeHeader m_GameModeHeader;
	protected SCR_InputButtonComponent m_NavigationStartComp;
	protected SCR_InputButtonComponent m_NavigationCloseComp;

	// --- State ---
	protected ref map<string, PS_FactionSelector> m_mFactions = new map<string, PS_FactionSelector>();
	protected ref map<int, PS_RolesGroup> m_mGroups = new map<int, PS_RolesGroup>();
	protected ref map<int, PS_PlayerSelector> m_mPlayers = new map<int, PS_PlayerSelector>();
	protected string m_sCurrentFactionKey;

	// =====================================================================
	// MENU LIFECYCLE
	// =====================================================================

	override void OnMenuOpen()
	{
		// WHY: dedicated servers have no UI.
		if (RplSession.Mode() == RplMode.Dedicated)
		{
			Close();
			return;
		}

		// Cache systems.
		m_GameModeCoop = PS_GameModeCoop.GetInstance();
		m_LobbyManager = PS_LobbyManager.GetInstance();
		m_PlayerManager = GetGame().GetPlayerManager();
		m_PlayerController = GetGame().GetPlayerController();
		m_FactionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		m_wWorkspaceWidget = GetGame().GetWorkspace();
		m_InputManager = GetGame().GetInputManager();
		m_iPlayerId = m_PlayerController.GetPlayerId();
		m_LobbyPlayerComponent = PS_LobbyPlayerComponent.Cast(
			m_PlayerController.FindComponent(PS_LobbyPlayerComponent));

		// Find widgets.
		m_wRoot = GetRootWidget();
		m_wFactionList = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("FactionList"));
		m_wRolesList = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("RolesList"));
		m_wPlayersList = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("PlayersList"));
		m_wPlayersCounter = TextWidget.Cast(m_wRoot.FindAnyWidget("PlayersCounter"));
		m_wNavigationStart = ButtonWidget.Cast(m_wRoot.FindAnyWidget("NavigationStart"));
		m_wNavigationClose = ButtonWidget.Cast(m_wRoot.FindAnyWidget("NavigationClose"));
		m_wGameModeHeader = FrameWidget.Cast(m_wRoot.FindAnyWidget("GameModeHeader"));
		m_wRolesScroll = ScrollLayoutWidget.Cast(m_wRoot.FindAnyWidget("RolesScroll"));

		// WHY: PlayersSearch starts hidden (VoiceChat tab is active by default in the layout).
		// We always want the players list visible — no voice channels implemented.
		Widget wPlayersSearch = m_wRoot.FindAnyWidget("PlayersSearch");
		if (wPlayersSearch)
			wPlayersSearch.SetVisible(true);

		// WHY: hide VoiceSwitch tab — no voice channel feature.
		// Show PlayersSwitch as the only (and active) tab.
		Widget wVoiceSwitch = m_wRoot.FindAnyWidget("VoiceSwitch");
		if (wVoiceSwitch)
			wVoiceSwitch.SetVisible(false);

		// WHY: hide VoiceChat frame if it exists — nothing to show there.
		Widget wVoiceChat = m_wRoot.FindAnyWidget("VoiceChat");
		if (wVoiceChat)
			wVoiceChat.SetVisible(false);

		// WHY: hide voice-related navigation buttons — not implemented.
		Widget wNavChat = m_wRoot.FindAnyWidget("NavigationChat");
		if (wNavChat)
			wNavChat.SetVisible(false);

		Widget wNavRoomVoice = m_wRoot.FindAnyWidget("NavigationRoomVoice");
		if (wNavRoomVoice)
			wNavRoomVoice.SetVisible(false);

		// Find handlers.
		if (m_wGameModeHeader)
			m_GameModeHeader = PS_GameModeHeader.Cast(m_wGameModeHeader.FindHandler(PS_GameModeHeader));

		if (m_wNavigationStart)
			m_NavigationStartComp = SCR_InputButtonComponent.Cast(m_wNavigationStart.FindHandler(SCR_InputButtonComponent));

		if (m_wNavigationClose)
			m_NavigationCloseComp = SCR_InputButtonComponent.Cast(m_wNavigationClose.FindHandler(SCR_InputButtonComponent));

		// Wire button handlers.
		if (m_NavigationStartComp)
			m_NavigationStartComp.m_OnActivated.Insert(Action_Ready);

		if (m_NavigationCloseComp)
			m_NavigationCloseComp.m_OnActivated.Insert(Action_Exit);

		// Subscribe to manager events for live updates.
		if (m_LobbyManager)
		{
			m_LobbyManager.GetOnSlotRegistered().Insert(OnSlotRegistered);
			m_LobbyManager.GetOnSlotUnregistered().Insert(OnSlotUnregistered);
			m_LobbyManager.GetOnSlotUpdated().Insert(OnSlotUpdated);
			m_LobbyManager.GetOnPlayerAssigned().Insert(OnPlayerAssigned);
			m_LobbyManager.GetOnPlayerUnassigned().Insert(OnPlayerUnassigned);
			m_LobbyManager.GetOnPlayerNameUpdated().Insert(OnPlayerNameUpdated);
		}

		// Build UI from current state.
		BuildUI();

		Print("[PS_Lobby] CoopLobby opened", LogLevel.NORMAL);
	}

	override void OnMenuClose()
	{
		// Unsubscribe from events.
		if (m_LobbyManager)
		{
			m_LobbyManager.GetOnSlotRegistered().Remove(OnSlotRegistered);
			m_LobbyManager.GetOnSlotUnregistered().Remove(OnSlotUnregistered);
			m_LobbyManager.GetOnSlotUpdated().Remove(OnSlotUpdated);
			m_LobbyManager.GetOnPlayerAssigned().Remove(OnPlayerAssigned);
			m_LobbyManager.GetOnPlayerUnassigned().Remove(OnPlayerUnassigned);
			m_LobbyManager.GetOnPlayerNameUpdated().Remove(OnPlayerNameUpdated);
		}

		if (m_NavigationStartComp)
			m_NavigationStartComp.m_OnActivated.Remove(Action_Ready);
		if (m_NavigationCloseComp)
			m_NavigationCloseComp.m_OnActivated.Remove(Action_Exit);

		Print("[PS_Lobby] CoopLobby closed", LogLevel.NORMAL);
	}

	override void OnMenuUpdate(float tDelta)
	{
		if (m_GameModeHeader)
			m_GameModeHeader.TryUpdate();
	}

	// =====================================================================
	// BUILD UI — construct widget tree from current lobby state
	// =====================================================================

	protected void BuildUI()
	{
		if (!m_LobbyManager)
			return;

		// Clear existing widgets.
		ClearChildWidgets(m_wFactionList);
		ClearChildWidgets(m_wRolesList);
		ClearChildWidgets(m_wPlayersList);
		m_mFactions.Clear();
		m_mGroups.Clear();
		m_mPlayers.Clear();

		// Build slot widgets grouped by faction and group.
		array<ref PS_SlotData> slots = m_LobbyManager.GetSlots();

		// WHY: collect unique factions first so we can create faction tabs.
		ref map<string, ref array<ref PS_SlotData>> factionSlots = new map<string, ref array<ref PS_SlotData>>();

		foreach (PS_SlotData slot : slots)
		{
			string fKey = slot.m_sFactionKey;
			if (!factionSlots.Contains(fKey))
				factionSlots.Set(fKey, new array<ref PS_SlotData>());

			factionSlots[fKey].Insert(slot);
		}

		// Create faction selector widgets.
		for (int i = 0; i < factionSlots.Count(); i++)
		{
			string factionKey = factionSlots.GetKey(i);
			array<ref PS_SlotData> fSlots = factionSlots.GetElement(i);
			AddFaction(factionKey, fSlots);
		}

		// Select first faction by default, or the player's current faction.
		PS_SlotData mySlot = m_LobbyManager.FindSlotByPlayerId(m_iPlayerId);
		if (mySlot)
			SwitchCurrentFaction(mySlot.m_sFactionKey);
		else if (factionSlots.Count() > 0)
			SwitchCurrentFaction(factionSlots.GetKey(0));

		// Build player list.
		BuildPlayerList();

		// Update player counter.
		UpdatePlayerCounter();
	}

	protected void AddFaction(string factionKey, array<ref PS_SlotData> slots)
	{
		if (!m_wFactionList || !m_wWorkspaceWidget)
			return;

		// Create faction selector widget.
		Widget factionWidget = m_wWorkspaceWidget.CreateWidgets(m_sFactionSelectorPrefab, m_wFactionList);
		if (!factionWidget)
			return;

		PS_FactionSelector factionSelector = PS_FactionSelector.Cast(factionWidget.FindHandler(PS_FactionSelector));
		if (!factionSelector)
			return;

		// Initialize the faction selector.
		SCR_Faction faction = null;
		if (m_FactionManager)
			faction = SCR_Faction.Cast(m_FactionManager.GetFactionByKey(factionKey));

		factionSelector.Init(faction, factionKey, this);
		m_mFactions.Set(factionKey, factionSelector);

		// Create role groups for this faction.
		// WHY: group slots by their groupId — each group becomes a RolesGroup widget.
		ref map<int, ref array<ref PS_SlotData>> groupedSlots = new map<int, ref array<ref PS_SlotData>>();

		foreach (PS_SlotData slot : slots)
		{
			int gId = slot.m_iGroupId;
			if (!groupedSlots.Contains(gId))
				groupedSlots.Set(gId, new array<ref PS_SlotData>());

			groupedSlots[gId].Insert(slot);
		}

		// Create a RolesGroup for each group.
		for (int i = 0; i < groupedSlots.Count(); i++)
		{
			int groupId = groupedSlots.GetKey(i);
			array<ref PS_SlotData> groupSlots = groupedSlots.GetElement(i);
			AddRolesGroup(factionKey, groupId, groupSlots);
		}

		// Update counts.
		UpdateFactionCounts(factionKey);
	}

	protected void AddRolesGroup(string factionKey, int groupId, array<ref PS_SlotData> slots)
	{
		if (!m_wRolesList || !m_wWorkspaceWidget)
			return;

		Widget groupWidget = m_wWorkspaceWidget.CreateWidgets(m_sRolesGroupPrefab, m_wRolesList);
		if (!groupWidget)
			return;

		PS_RolesGroup rolesGroup = PS_RolesGroup.Cast(groupWidget.FindHandler(PS_RolesGroup));
		if (!rolesGroup)
			return;

		string groupName = "";
		if (slots.Count() > 0)
			groupName = slots[0].m_sGroupName;
		rolesGroup.Init(factionKey, groupId, groupName, this);
		m_mGroups.Set(groupId, rolesGroup);

		// Add character selectors for each slot.
		foreach (PS_SlotData slot : slots)
		{
			rolesGroup.AddSlot(slot);
		}

		// Set visibility based on current faction.
		groupWidget.SetVisible(factionKey == m_sCurrentFactionKey);
	}

	protected void BuildPlayerList()
	{
		if (!m_wPlayersList || !m_PlayerManager)
			return;

		ClearChildWidgets(m_wPlayersList);
		m_mPlayers.Clear();

		// WHY: iterate all connected players and create a PlayerSelector for each.
		array<int> playerIds = {};
		m_PlayerManager.GetPlayers(playerIds);

		foreach (int playerId : playerIds)
		{
			AddPlayer(playerId);
		}
	}

	protected void AddPlayer(int playerId)
	{
		if (!m_wPlayersList || !m_wWorkspaceWidget)
			return;

		if (m_mPlayers.Contains(playerId))
			return;

		Widget playerWidget = m_wWorkspaceWidget.CreateWidgets(m_sPlayerSelectorPrefab, m_wPlayersList);
		if (!playerWidget)
			return;

		PS_PlayerSelector playerSelector = PS_PlayerSelector.Cast(playerWidget.FindHandler(PS_PlayerSelector));
		if (!playerSelector)
			return;

		string playerName = m_LobbyManager.GetPlayerName(playerId);
		if (playerName == "")
			playerName = m_PlayerManager.GetPlayerName(playerId);

		playerSelector.Init(playerId, playerName, this);
		m_mPlayers.Set(playerId, playerSelector);
	}

	// =====================================================================
	// FACTION SWITCHING
	// =====================================================================

	void SwitchCurrentFaction(string factionKey)
	{
		m_sCurrentFactionKey = factionKey;

		// Show/hide role groups based on selected faction.
		for (int i = 0; i < m_mGroups.Count(); i++)
		{
			PS_RolesGroup group = m_mGroups.GetElement(i);
			Widget groupWidget = group.GetRootWidget();
			if (groupWidget)
				groupWidget.SetVisible(group.GetFactionKey() == factionKey);
		}

		// Highlight selected faction tab.
		for (int i = 0; i < m_mFactions.Count(); i++)
		{
			m_mFactions.GetElement(i).SetSelected(m_mFactions.GetKey(i) == factionKey);
		}
	}

	string GetCurrentFactionKey()
	{
		return m_sCurrentFactionKey;
	}

	// =====================================================================
	// EVENT HANDLERS — live updates from PS_LobbyManager
	// =====================================================================

	protected void OnSlotRegistered(PS_SlotData slot)
	{
		// WHY: slots register ~500ms after menu opens (deferred RplId assignment in
		// PS_PlayableComponent). BuildUI() runs at open time with an empty list, so
		// ALL faction tabs and role groups must be created on demand here too.
		string fKey = slot.m_sFactionKey;
		int gId = slot.m_iGroupId;

		// Create faction tab if not yet present.
		if (!m_mFactions.Contains(fKey) && m_wFactionList && m_wWorkspaceWidget)
		{
			Widget factionWidget = m_wWorkspaceWidget.CreateWidgets(m_sFactionSelectorPrefab, m_wFactionList);
			if (factionWidget)
			{
				PS_FactionSelector factionSelector = PS_FactionSelector.Cast(factionWidget.FindHandler(PS_FactionSelector));
				if (factionSelector)
				{
					SCR_Faction faction = null;
					if (m_FactionManager)
						faction = SCR_Faction.Cast(m_FactionManager.GetFactionByKey(fKey));
					factionSelector.Init(faction, fKey, this);
					m_mFactions.Set(fKey, factionSelector);

					// Select first faction that appears.
					if (m_sCurrentFactionKey == "")
						SwitchCurrentFaction(fKey);
				}
			}
		}

		// Create role group if not yet present.
		PS_RolesGroup group;
		if (!m_mGroups.Find(gId, group) && m_wRolesList && m_wWorkspaceWidget)
		{
			Widget groupWidget = m_wWorkspaceWidget.CreateWidgets(m_sRolesGroupPrefab, m_wRolesList);
			if (groupWidget)
			{
				group = PS_RolesGroup.Cast(groupWidget.FindHandler(PS_RolesGroup));
				if (group)
				{
					group.Init(fKey, gId, slot.m_sGroupName, this);
					m_mGroups.Set(gId, group);
					groupWidget.SetVisible(fKey == m_sCurrentFactionKey);
				}
			}
		}

		if (group)
			group.AddSlot(slot);

		UpdateFactionCounts(fKey);
		UpdatePlayerCounter();
	}

	protected void OnSlotUnregistered(int rplId)
	{
		// WHY: character entity deleted. Remove it from the UI.
		// RolesGroups handle their own cleanup via PS_CharacterSelector.
		UpdatePlayerCounter();
	}

	protected void OnSlotUpdated(PS_SlotData slot)
	{
		// WHY: slot data changed (player assigned/left, damage, lock).
		// CharacterSelectors update themselves via direct data binding.
		UpdateFactionCounts(slot.m_sFactionKey);
	}

	protected void OnPlayerAssigned(int playerId, int slotRplId)
	{
		UpdatePlayerCounter();
		UpdatePlayerInList(playerId);
	}

	protected void OnPlayerUnassigned(int playerId, int slotRplId)
	{
		UpdatePlayerCounter();
		UpdatePlayerInList(playerId);
	}

	protected void OnPlayerNameUpdated(int playerId, string name)
	{
		// Add player to list if not present.
		if (!m_mPlayers.Contains(playerId))
			AddPlayer(playerId);

		PS_PlayerSelector playerSel;
		if (m_mPlayers.Find(playerId, playerSel))
			playerSel.UpdateName(name);
	}

	// =====================================================================
	// UPDATE HELPERS
	// =====================================================================

	protected void UpdateFactionCounts(string factionKey)
	{
		PS_FactionSelector factionSel;
		if (!m_mFactions.Find(factionKey, factionSel))
			return;

		if (!m_LobbyManager)
			return;

		array<ref PS_SlotData> slots = m_LobbyManager.GetSlotsForFaction(factionKey);
		int total = slots.Count();
		int occupied = 0;
		int locked = 0;

		foreach (PS_SlotData slot : slots)
		{
			if (slot.m_iPlayerId >= 0)
				occupied++;
			if (slot.m_bLocked)
				locked++;
		}

		factionSel.SetCounts(occupied, total, locked);
	}

	protected void UpdatePlayerCounter()
	{
		if (!m_wPlayersCounter || !m_PlayerManager || !m_LobbyManager)
			return;

		int playerCount = m_PlayerManager.GetPlayerCount();
		int slotCount = m_LobbyManager.GetSlots().Count();
		m_wPlayersCounter.SetTextFormat("%1/%2", playerCount, slotCount);
	}

	protected void UpdatePlayerInList(int playerId)
	{
		PS_PlayerSelector playerSel;
		if (m_mPlayers.Find(playerId, playerSel))
			playerSel.Refresh();
	}

	// =====================================================================
	// ACTIONS — button callbacks
	// =====================================================================

	protected void Action_Ready()
	{
		// WHY: for now, Ready = admin advance state. No per-player ready tracking yet.
		if (m_LobbyPlayerComponent)
			m_LobbyPlayerComponent.AskAdvanceState();

		Print("[PS_Lobby] Ready/AdvanceState pressed", LogLevel.NORMAL);
	}

	protected void Action_Exit()
	{
		Close();
	}

	// =====================================================================
	// UTILITY
	// =====================================================================

	protected void ClearChildWidgets(Widget parent)
	{
		if (!parent)
			return;

		while (parent.GetChildren())
		{
			parent.GetChildren().RemoveFromHierarchy();
		}
	}

	// Public accessor for child components.
	PS_LobbyManager GetLobbyManager()
	{
		return m_LobbyManager;
	}

	PS_LobbyPlayerComponent GetLobbyPlayerComponent()
	{
		return m_LobbyPlayerComponent;
	}

	int GetLocalPlayerId()
	{
		return m_iPlayerId;
	}
}
