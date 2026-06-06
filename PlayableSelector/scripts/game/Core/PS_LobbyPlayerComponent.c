// PS_LobbyPlayerComponent — Client-side lobby interface on PlayerController.
// Lives on each player's PlayerController entity.
// WHY on PlayerController: this is the only entity the client has authority (Owner) over.
// RPCs from client→server must originate from an entity the client owns.
//
// Pattern: client calls Ask_ method → RPC goes to server → server validates →
// server calls PS_LobbyManager._S() method → manager broadcasts result via RpcDo_.
//
// This component does NOT store lobby data — it only sends requests.
// Data lives in PS_LobbyManager, which clients read directly.

class PS_LobbyPlayerComponentClass : ScriptComponentClass
{
}

class PS_LobbyPlayerComponent : ScriptComponent
{
	// =====================================================================
	// CONVENIENCE — get this component from a player ID
	// =====================================================================

	static PS_LobbyPlayerComponent GetByPlayerId(int playerId)
	{
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return null;

		PlayerController pc = pm.GetPlayerController(playerId);
		if (!pc)
			return null;

		return PS_LobbyPlayerComponent.Cast(pc.FindComponent(PS_LobbyPlayerComponent));
	}

	// Get the local player's component.
	static PS_LobbyPlayerComponent GetLocalInstance()
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return null;

		return PS_LobbyPlayerComponent.Cast(pc.FindComponent(PS_LobbyPlayerComponent));
	}

	// Get our player ID from the owning PlayerController.
	int GetPlayerId()
	{
		PlayerController pc = PlayerController.Cast(GetOwner());
		if (!pc)
			return -1;

		return pc.GetPlayerId();
	}

	// =====================================================================
	// LIFECYCLE
	// =====================================================================

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// WHY: when this component initializes on the server, send the player's name
		// to the lobby manager so it's available for display.
		if (Replication.IsServer())
		{
			int playerId = GetPlayerId();
			if (playerId > 0)
			{
				PlayerManager pm = GetGame().GetPlayerManager();
				if (pm)
				{
					string name = pm.GetPlayerName(playerId);
					PS_LobbyManager mgr = PS_LobbyManager.GetInstance();
					if (mgr)
						mgr.SetPlayerName_S(playerId, name);
				}
			}
		}

		if (!GetGame().InPlayMode())
			return;

		// WHY: subscribe unconditionally here — GetGame().GetPlayerController() may be null
		// at OnPostInit time (Workbench/JIP timing). The local-player guard is in the callbacks.
		// This mirrors the vanilla SCR_PlayerDeployMenuHandlerComponent pattern.
		PS_GameModeCoop gameMode = PS_GameModeCoop.GetInstance();
		if (gameMode)
		{
			gameMode.GetOnGameStateChanged().Insert(OnGameStateChanged_Client);
			Print("[PS_Lobby] LobbyPlayerComponent: subscribed to state changes", LogLevel.NORMAL);
		}
		else
		{
			Print("[PS_Lobby] LobbyPlayerComponent: WARNING — PS_GameModeCoop not found at init time", LogLevel.WARNING);
		}

		// WHY CallLater: at OnPostInit time the game mode state may already be
		// SLOTSELECTION (JIP scenario) but OpenMenu needs one frame to be safe.
		GetGame().GetCallqueue().CallLater(CheckAndOpenLobby, 100, false);
	}

	override void OnDelete(IEntity owner)
	{
		super.OnDelete(owner);

		// Unsubscribe to avoid dangling callbacks.
		PS_GameModeCoop gameMode = PS_GameModeCoop.GetInstance();
		if (gameMode)
			gameMode.GetOnGameStateChanged().Remove(OnGameStateChanged_Client);
	}

	// =====================================================================
	// CLIENT-SIDE MENU MANAGEMENT
	// =====================================================================

	// Returns true if this component is on the LOCAL player's controller.
	// WHY helper: GetGame().GetPlayerController() may be null at OnPostInit time,
	// so we do this check lazily at callback time when it's guaranteed to be set.
	protected bool IsLocalPlayer()
	{
		PlayerController local = GetGame().GetPlayerController();
		if (!local)
		{
			Print("[PS_Lobby] LobbyPlayerComponent: IsLocalPlayer — GetPlayerController() is null", LogLevel.WARNING);
			return false;
		}
		return local == GetOwner();
	}

	// Called when the game mode state changes — opens or closes the lobby menu.
	protected void OnGameStateChanged_Client(int state)
	{
		// WHY guard: subscribed by ALL player controller instances. Only the local
		// player's instance should open UI.
		if (!IsLocalPlayer())
			return;

		if (state == SCR_EGameModeState.SLOTSELECTION)
		{
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CoopLobby);
			Print("[PS_Lobby] CoopLobby menu opened (state=SLOTSELECTION)", LogLevel.NORMAL);
		}
		else
		{
			GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CoopLobby);
			Print(string.Format("[PS_Lobby] CoopLobby menu closed (state=%1)",
				typename.EnumToString(SCR_EGameModeState, state)), LogLevel.NORMAL);
		}
	}

	// Called once on a short delay after init — handles JIP case where state is already SLOTSELECTION.
	protected void CheckAndOpenLobby()
	{
		if (!IsLocalPlayer())
			return;

		PS_GameModeCoop gameMode = PS_GameModeCoop.GetInstance();
		if (!gameMode)
			return;

		SCR_EGameModeState state = gameMode.GetState();
		if (state == SCR_EGameModeState.SLOTSELECTION)
		{
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CoopLobby);
			Print("[PS_Lobby] CoopLobby menu opened (JIP, state=SLOTSELECTION)", LogLevel.NORMAL);
		}
	}

	// =====================================================================
	// CLIENT → SERVER RPCs (Ask_ pattern)
	// WHY RplRcver.Server: these go from the owning client to the server.
	// Server validates the request before applying it.
	// =====================================================================

	// --- Take Slot ---
	// WHY: client selected a character in the lobby UI. Ask server to assign it.

	void AskTakeSlot(int slotRplId)
	{
		Rpc(RpcAsk_TakeSlot, slotRplId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_TakeSlot(int slotRplId)
	{
		int playerId = GetPlayerId();

		PS_LobbyManager mgr = PS_LobbyManager.GetInstance();
		if (!mgr)
			return;

		bool success = mgr.TakeSlot_S(playerId, slotRplId);
		if (!success)
		{
			Print(string.Format("[PS_Lobby] RpcAsk_TakeSlot denied: player=%1 slot=%2", playerId, slotRplId), LogLevel.WARNING);
		}
	}

	// --- Leave Slot ---
	// WHY: client clicked "deselect" or wants to pick a different slot.

	void AskLeaveSlot()
	{
		Rpc(RpcAsk_LeaveSlot);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_LeaveSlot()
	{
		int playerId = GetPlayerId();

		PS_LobbyManager mgr = PS_LobbyManager.GetInstance();
		if (!mgr)
			return;

		mgr.LeaveSlot_S(playerId);
	}

	// --- Advance Game State (Admin) ---
	// WHY: admin clicks "Start Briefing" or "Start Game" button.
	// Server validates admin permissions in the game mode.

	void AskAdvanceState()
	{
		int playerId = GetPlayerId();
		Rpc(RpcAsk_AdvanceState, playerId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AdvanceState(int playerId)
	{
		PS_GameModeCoop gameMode = PS_GameModeCoop.GetInstance();
		if (!gameMode)
			return;

		gameMode.RpcAsk_AdvanceState(playerId);
	}

	// --- Lock/Unlock Slot (Admin) ---

	void AskSetSlotLocked(int slotRplId, bool locked)
	{
		Rpc(RpcAsk_SetSlotLocked, slotRplId, locked);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SetSlotLocked(int slotRplId, bool locked)
	{
		// TODO: validate admin permissions

		PS_LobbyManager mgr = PS_LobbyManager.GetInstance();
		if (!mgr)
			return;

		mgr.SetSlotLocked_S(slotRplId, locked);
	}

	// --- Kick Player from Slot (Admin) ---

	void AskKickPlayer(int targetPlayerId)
	{
		Rpc(RpcAsk_KickPlayer, targetPlayerId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_KickPlayer(int targetPlayerId)
	{
		// TODO: validate admin permissions

		PS_LobbyManager mgr = PS_LobbyManager.GetInstance();
		if (!mgr)
			return;

		mgr.LeaveSlot_S(targetPlayerId);
	}

	// --- Enter Spectator (client died) ---
	// WHY: spectator VoN channel change must happen on the server.
	// Client calls this after detecting their character was destroyed.

	void AskEnterSpectator()
	{
		Rpc(RpcAsk_EnterSpectator);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_EnterSpectator()
	{
		int playerId = GetPlayerId();

		PS_VoNChannelsManager vonMgr = PS_VoNChannelsManager.GetInstance();
		if (vonMgr)
			vonMgr.SetPlayerSpectator_S(playerId);

		Print(string.Format("[PS_Lobby] Player %1 entered spectator mode", playerId), LogLevel.NORMAL);
	}
}
