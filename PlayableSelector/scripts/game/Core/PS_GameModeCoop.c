// PS_GameModeCoop — Lobby game mode with state machine.
// Extends SCR_BaseGameMode. Controls lobby lifecycle: SLOTSELECTION → BRIEFING → GAME → DEBRIEFING.
// Owns configuration (editor attributes). Does NOT own lobby data — that's PS_LobbyManager.
// Pattern: vanilla SCR_BaseGameMode + modded enum for lobby states.

// Extend vanilla game state enum with lobby-specific states.
// WHY modded enum: Enfusion requires extending existing enums this way.
// We add only the states we need — CUTSCENE and NULL from old PS are dropped (declined features).
modded enum SCR_EGameModeState
{
	SLOTSELECTION,
	BRIEFING,
	DEBRIEFING,
}

// --- Class definition (required by Enfusion for any entity class) ---
class PS_GameModeCoopClass : SCR_BaseGameModeClass
{
}

// --- Game Mode ---
class PS_GameModeCoop : SCR_BaseGameMode
{
	// =====================================================================
	// EDITOR ATTRIBUTES — scenario designer configures these in World Editor
	// =====================================================================

	[Attribute("120000", UIWidgets.EditBox, "Time (ms) disconnected players keep their slot reserved. -1 = infinite.", category: "PS Lobby")]
	protected int m_iReconnectTime;

	[Attribute("-1", UIWidgets.EditBox, "Reconnect reservation time (ms) after briefing ends. -1 = same as above.", category: "PS Lobby")]
	protected int m_iReconnectTimeAfterBriefing;

	[Attribute("1", UIWidgets.CheckBox, "Only admins can advance game state (start briefing, start game).", category: "PS Lobby")]
	protected bool m_bAdminMode;

	[Attribute("0", UIWidgets.CheckBox, "Players can reopen lobby during GAME state to switch roles.", category: "PS Lobby")]
	protected bool m_bAllowTeamSwitch;

	[Attribute("0", UIWidgets.CheckBox, "Map markers can only be placed by squad leaders during BRIEFING.", category: "PS Lobby")]
	protected bool m_bMarkersOnlyOnBriefing;

	[Attribute("60000", UIWidgets.EditBox, "Freeze time (ms) after GAME starts. Players can't leave spawn zone.", category: "PS Lobby")]
	protected int m_iFreezeTime;

	[Attribute("0", UIWidgets.CheckBox, "Remove AI units not occupied by players when GAME starts.", category: "PS Lobby")]
	protected bool m_bRemoveRedundantUnits;

	[Attribute("0", UIWidgets.CheckBox, "Remove default squad leader map markers.", category: "PS Lobby")]
	protected bool m_bRemoveSquadMarkers;

	[Attribute("0", UIWidgets.CheckBox, "Disable text chat for alive players during GAME. Admins always see chat.", category: "PS Lobby")]
	protected bool m_bDisableChat;

	[Attribute("0", UIWidgets.CheckBox, "Disable vanilla group menu (replaced by lobby UI).", category: "PS Lobby")]
	protected bool m_bDisableVanillaGroupMenu;

	[Attribute("1", UIWidgets.CheckBox, "Disable night vision during lobby phases.", category: "PS Lobby")]
	protected bool m_bDisableArmaVision;

	[Attribute("0", UIWidgets.CheckBox, "Spectator camera locked to own faction only.", category: "PS Lobby")]
	protected bool m_bFriendliesSpectatorOnly;

	// =====================================================================
	// REPLICATED STATE — simple scalars only, per replication rules
	// =====================================================================

	// WHY [RplProp()]: freeze time countdown is a simple float scalar.
	// Clients need it for the countdown UI. Updated once per second during freeze.
	[RplProp()]
	protected float m_fFreezeTimeRemaining;

	// WHY [RplProp()]: clients need to know when the game actually started
	// for elapsed time calculations.
	[RplProp()]
	protected float m_fGameStartTimestamp;

	// =====================================================================
	// EVENTS — UI and other components subscribe to these
	// =====================================================================

	// WHY ScriptInvoker: decouples UI from game mode. UI subscribes to state
	// changes without the game mode knowing about UI at all.
	protected ref ScriptInvokerInt m_OnGameStateChanged = new ScriptInvokerInt();

	ScriptInvokerInt GetOnGameStateChanged()
	{
		return m_OnGameStateChanged;
	}

	// =====================================================================
	// GETTERS — public read access to config
	// =====================================================================

	int GetReconnectTime()
	{
		// WHY: after briefing, reconnect time may be different (shorter or infinite).
		SCR_EGameModeState state = GetState();
		if (state == SCR_EGameModeState.GAME || state == SCR_EGameModeState.DEBRIEFING)
		{
			if (m_iReconnectTimeAfterBriefing >= 0)
				return m_iReconnectTimeAfterBriefing;
		}
		return m_iReconnectTime;
	}

	bool IsAdminMode()			{ return m_bAdminMode; }
	bool AllowTeamSwitch()		{ return m_bAllowTeamSwitch; }
	bool MarkersOnlyOnBriefing(){ return m_bMarkersOnlyOnBriefing; }
	int GetFreezeTimeDuration()	{ return m_iFreezeTime; }
	float GetFreezeTimeRemaining() { return m_fFreezeTimeRemaining; }
	bool RemoveRedundantUnits()	{ return m_bRemoveRedundantUnits; }
	bool RemoveSquadMarkers()	{ return m_bRemoveSquadMarkers; }
	bool IsChatDisabled()		{ return m_bDisableChat; }
	bool IsArmaVisionDisabled()	{ return m_bDisableArmaVision; }
	bool IsFriendliesSpectatorOnly() { return m_bFriendliesSpectatorOnly; }
	float GetGameStartTimestamp() { return m_fGameStartTimestamp; }

	// Current lobby state — wraps parent's GetState() for clarity.
	SCR_EGameModeState GetLobbyState()
	{
		return GetState();
	}

	// =====================================================================
	// SINGLETON — convenience accessor, since there's exactly one game mode
	// =====================================================================

	static PS_GameModeCoop GetInstance()
	{
		return PS_GameModeCoop.Cast(GetGame().GetGameMode());
	}

	// =====================================================================
	// LIFECYCLE
	// =====================================================================

	override void OnGameStart()
	{
		super.OnGameStart();

		Print("[PS_Lobby] GameMode OnGameStart", LogLevel.NORMAL);

		// WHY: disable vanilla group menu if configured — lobby has its own UI.
		if (m_bDisableVanillaGroupMenu)
		{
			InputManager inputManager = GetGame().GetInputManager();
			if (inputManager)
			{
				inputManager.RemoveActionListener("ShowScoreboard", EActionTrigger.DOWN, ArmaReforgerScripted.OnShowPlayerList);
				inputManager.RemoveActionListener("ShowGroupMenu", EActionTrigger.DOWN, ArmaReforgerScripted.OnShowGroupMenu);
			}
		}

		// WHY: server starts in SLOTSELECTION. Clients will get this via RplProp on m_eGameState.
		if (Replication.IsServer())
		{
			SetLobbyState(SCR_EGameModeState.SLOTSELECTION);
		}
	}

	// =====================================================================
	// STATE MACHINE
	// =====================================================================

	// WHY: called on ALL machines when m_eGameState [RplProp()] changes.
	// This is the vanilla callback pattern — parent class fires this automatically.
	override void OnGameStateChanged()
	{
		super.OnGameStateChanged();

		SCR_EGameModeState newState = GetState();

		Print(string.Format("[PS_Lobby] State changed to: %1", typename.EnumToString(SCR_EGameModeState, newState)), LogLevel.NORMAL);

		// Notify listeners (UI, VoN manager, etc.)
		m_OnGameStateChanged.Invoke(newState);
	}

	// Server-only: transition to a new state.
	// WHY protected: only the game mode itself or admin RPCs should change state.
	// External code calls AdvanceLobbyState() or specific transition methods.
	protected void SetLobbyState(SCR_EGameModeState newState)
	{
		if (!Replication.IsServer())
			return;

		SCR_EGameModeState oldState = GetState();
		if (oldState == newState)
			return;

		Print(string.Format("[PS_Lobby] State transition: %1 → %2",
			typename.EnumToString(SCR_EGameModeState, oldState),
			typename.EnumToString(SCR_EGameModeState, newState)), LogLevel.NORMAL);

		// WHY: SetGameModeState() is added via modded class in PS_Modded.c because
		// m_eGameState is private in vanilla SCR_BaseGameMode.
		SetGameModeState(newState);

		// Handle server-side effects of entering the new state.
		OnEnterState_S(newState);
	}

	// Server-only: advance to the next logical state.
	// WHY: provides a simple "next" button for the admin UI.
	void AdvanceLobbyState()
	{
		if (!Replication.IsServer())
			return;

		SCR_EGameModeState current = GetState();

		switch (current)
		{
			case SCR_EGameModeState.SLOTSELECTION:
				SetLobbyState(SCR_EGameModeState.BRIEFING);
				break;

			case SCR_EGameModeState.BRIEFING:
				SetLobbyState(SCR_EGameModeState.GAME);
				break;

			case SCR_EGameModeState.GAME:
				SetLobbyState(SCR_EGameModeState.DEBRIEFING);
				break;
		}
	}

	// Server-only: handle side effects when entering a state.
	protected void OnEnterState_S(SCR_EGameModeState state)
	{
		switch (state)
		{
			case SCR_EGameModeState.GAME:
				OnEnterGame_S();
				break;
		}
	}

	// Server-only: GAME state entered — start freeze timer, record timestamp.
	protected void OnEnterGame_S()
	{
		m_fGameStartTimestamp = System.GetTickCount();
		m_fFreezeTimeRemaining = m_iFreezeTime / 1000.0; // Convert ms to seconds for UI

		Replication.BumpMe();

		// WHY: start freeze countdown if freeze time > 0.
		// CallLater at 1s interval updates the countdown for client UI.
		if (m_iFreezeTime > 0)
		{
			GetGame().GetCallqueue().CallLater(UpdateFreezeTimer_S, 1000, true);
		}

		Print(string.Format("[PS_Lobby] Game started. Freeze time: %1s", m_fFreezeTimeRemaining), LogLevel.NORMAL);
	}

	// Server-only: tick the freeze timer once per second.
	protected void UpdateFreezeTimer_S()
	{
		m_fFreezeTimeRemaining -= 1.0;

		if (m_fFreezeTimeRemaining <= 0)
		{
			m_fFreezeTimeRemaining = 0;
			GetGame().GetCallqueue().Remove(UpdateFreezeTimer_S);
			OnFreezeTimeEnded_S();
		}

		// WHY: BumpMe so clients get the updated countdown value.
		// This is a simple scalar — exactly what [RplProp()] is for.
		Replication.BumpMe();
	}

	// Server-only: freeze time ended — players are free to move.
	protected void OnFreezeTimeEnded_S()
	{
		Print("[PS_Lobby] Freeze time ended", LogLevel.NORMAL);
		// TODO: remove freeze zones, enable movement
	}

	// =====================================================================
	// PLAYER LIFECYCLE — server-only callbacks from engine
	// =====================================================================

	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		Print(string.Format("[PS_Lobby] Player connected: %1", playerId), LogLevel.NORMAL);
	}

	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);
		Print(string.Format("[PS_Lobby] Player disconnected: %1 (cause: %2)", playerId, cause), LogLevel.NORMAL);

		// WHY: don't delete entity immediately — player might reconnect.
		// PS_LobbyManager handles reconnect reservation via its own logic.
	}

	// =====================================================================
	// RPC — Admin state control
	// =====================================================================

	// WHY RpcAsk_: client (admin) asks server to advance state.
	// Server validates admin permissions before executing.
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_AdvanceState(int playerId)
	{
		// WHY: validate that the requesting player is actually an admin.
		if (m_bAdminMode)
		{
			PlayerManager pm = GetGame().GetPlayerManager();
			if (!pm)
				return;

			// TODO: proper admin check — for now, check if player has admin role.
			// This should use the vanilla admin system or a custom check.
		}

		Print(string.Format("[PS_Lobby] Admin %1 advancing state", playerId), LogLevel.NORMAL);
		AdvanceLobbyState();
	}
}
