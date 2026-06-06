// PS_SpectatorManager — Client-side spectator mode management.
// Lives on the GameMode entity as a SCR_BaseGameModeComponent.
//
// Detects when a player should enter spectator mode (character destroyed, no slot).
// Spawns a spectator camera and manages the alive player list.
// ENTIRELY CLIENT-SIDE — no replication. Server only handles VoN channel move.
//
// This is a basic implementation — spectator UI (PS_SpectatorMenu) will be wired in Phase 2
// of the spectator system if needed.

class PS_SpectatorManagerClass : SCR_BaseGameModeComponentClass
{
}

class PS_SpectatorManager : SCR_BaseGameModeComponent
{
	// =====================================================================
	// SINGLETON
	// =====================================================================

	protected static PS_SpectatorManager s_Instance;

	static PS_SpectatorManager GetInstance()
	{
		return s_Instance;
	}

	// =====================================================================
	// STATE
	// =====================================================================

	// Whether the local player is currently in spectator mode.
	protected bool m_bIsSpectating;

	// The currently spectated slot RplId (-1 = free camera).
	protected int m_iSpectatedSlotRplId = -1;

	// =====================================================================
	// EVENTS
	// =====================================================================

	protected ref ScriptInvoker m_OnSpectatorEnter = new ScriptInvoker();	// ()
	protected ref ScriptInvoker m_OnSpectatorExit = new ScriptInvoker();	// ()
	protected ref ScriptInvoker m_OnSpectatedChanged = new ScriptInvoker();	// (int slotRplId)

	ScriptInvoker GetOnSpectatorEnter()		{ return m_OnSpectatorEnter; }
	ScriptInvoker GetOnSpectatorExit()		{ return m_OnSpectatorExit; }
	ScriptInvoker GetOnSpectatedChanged()	{ return m_OnSpectatedChanged; }

	// =====================================================================
	// LIFECYCLE
	// =====================================================================

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		s_Instance = this;
	}

	override void OnDelete(IEntity owner)
	{
		super.OnDelete(owner);
		if (s_Instance == this)
			s_Instance = null;
	}

	// =====================================================================
	// SPECTATOR ACTIVATION — called when local player's character is destroyed
	// =====================================================================

	// Enter spectator mode. Client-side only.
	void EnterSpectator()
	{
		if (m_bIsSpectating)
			return;

		m_bIsSpectating = true;
		m_iSpectatedSlotRplId = -1;

		Print("[PS_Spectator] Entering spectator mode", LogLevel.NORMAL);

		// WHY: ask server to move player to spectator VoN channel.
		// This is a client-side component — we can't call server methods directly.
		// The PS_LobbyPlayerComponent (on our PlayerController) can send RPCs to server.
		PS_LobbyPlayerComponent lobbyPlayer = PS_LobbyPlayerComponent.GetLocalInstance();
		if (lobbyPlayer)
			lobbyPlayer.AskEnterSpectator();

		m_OnSpectatorEnter.Invoke();

		// TODO: spawn spectator camera entity
		// TODO: open spectator menu
	}

	// Exit spectator mode (player respawned or took a new slot).
	void ExitSpectator()
	{
		if (!m_bIsSpectating)
			return;

		m_bIsSpectating = false;
		m_iSpectatedSlotRplId = -1;

		Print("[PS_Spectator] Exiting spectator mode", LogLevel.NORMAL);

		m_OnSpectatorExit.Invoke();

		// TODO: destroy spectator camera
		// TODO: close spectator menu
	}

	// =====================================================================
	// SPECTATED TARGET — cycle through alive players
	// =====================================================================

	// Spectate a specific slot.
	void SpectateSlot(int slotRplId)
	{
		if (!m_bIsSpectating)
			return;

		m_iSpectatedSlotRplId = slotRplId;

		Print(string.Format("[PS_Spectator] Now spectating slot: %1", slotRplId), LogLevel.NORMAL);

		m_OnSpectatedChanged.Invoke(slotRplId);

		// TODO: move camera to the character entity
	}

	// Cycle to the next alive player.
	void SpectateNext()
	{
		if (!m_bIsSpectating)
			return;

		PS_LobbyManager mgr = PS_LobbyManager.GetInstance();
		if (!mgr)
			return;

		array<ref PS_SlotData> slots = mgr.GetSlots();
		if (slots.IsEmpty())
			return;

		// WHY: filter to alive, occupied slots only.
		array<int> aliveSlotIds = {};
		foreach (PS_SlotData slot : slots)
		{
			if (slot.m_iPlayerId >= 0 && !slot.IsDestroyed())
				aliveSlotIds.Insert(slot.m_iRplId);
		}

		if (aliveSlotIds.IsEmpty())
			return;

		// Find current index and advance.
		int currentIdx = -1;
		if (m_iSpectatedSlotRplId >= 0)
			currentIdx = aliveSlotIds.Find(m_iSpectatedSlotRplId);

		int nextIdx = (currentIdx + 1) % aliveSlotIds.Count();
		SpectateSlot(aliveSlotIds[nextIdx]);
	}

	// Cycle to the previous alive player.
	void SpectatePrevious()
	{
		if (!m_bIsSpectating)
			return;

		PS_LobbyManager mgr = PS_LobbyManager.GetInstance();
		if (!mgr)
			return;

		array<ref PS_SlotData> slots = mgr.GetSlots();
		array<int> aliveSlotIds = {};
		foreach (PS_SlotData slot : slots)
		{
			if (slot.m_iPlayerId >= 0 && !slot.IsDestroyed())
				aliveSlotIds.Insert(slot.m_iRplId);
		}

		if (aliveSlotIds.IsEmpty())
			return;

		int currentIdx = -1;
		if (m_iSpectatedSlotRplId >= 0)
			currentIdx = aliveSlotIds.Find(m_iSpectatedSlotRplId);

		int prevIdx;
		if (currentIdx <= 0)
			prevIdx = aliveSlotIds.Count() - 1;
		else
			prevIdx = currentIdx - 1;

		SpectateSlot(aliveSlotIds[prevIdx]);
	}

	// =====================================================================
	// QUERY
	// =====================================================================

	bool IsSpectating()
	{
		return m_bIsSpectating;
	}

	int GetSpectatedSlotRplId()
	{
		return m_iSpectatedSlotRplId;
	}

	// Get alive players for the spectator UI alive list.
	// Optionally filter by faction for FriendliesSpectatorOnly mode.
	array<ref PS_SlotData> GetAliveSlots(string factionFilter = "")
	{
		array<ref PS_SlotData> result = {};

		PS_LobbyManager mgr = PS_LobbyManager.GetInstance();
		if (!mgr)
			return result;

		array<ref PS_SlotData> slots = mgr.GetSlots();
		foreach (PS_SlotData slot : slots)
		{
			if (slot.m_iPlayerId < 0)
				continue;
			if (slot.IsDestroyed())
				continue;
			if (factionFilter != "" && slot.m_sFactionKey != factionFilter)
				continue;

			result.Insert(slot);
		}

		return result;
	}

	// =====================================================================
	// DAMAGE STATE LISTENER — detect when local player's character dies
	// =====================================================================

	// WHY: this should be called by PS_PlayableComponent or the game mode
	// when the local player's character is destroyed.
	// The game mode's OnControllableDestroyed can trigger this.
	void OnLocalPlayerCharacterDestroyed()
	{
		PS_GameModeCoop gameMode = PS_GameModeCoop.GetInstance();
		if (!gameMode)
			return;

		// WHY: only enter spectator during GAME state.
		// During SLOTSELECTION/BRIEFING, player should respawn or wait.
		SCR_EGameModeState state = gameMode.GetLobbyState();
		if (state != SCR_EGameModeState.GAME)
			return;

		EnterSpectator();
	}
}
