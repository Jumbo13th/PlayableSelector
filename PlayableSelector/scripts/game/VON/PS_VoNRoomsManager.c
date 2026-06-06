//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_VoNRoomsManagerClass: ScriptComponentClass
{

};

// just string but funnier
// struct: [FactionKey + "|"] + string
typedef string VoNRoomKey;

class PS_VoNRoomsManager : ScriptComponent
{
	// server-only reverse lookup
	ref map<VoNRoomKey, int> m_mVoiceRoomsFromName = new map<VoNRoomKey, int>();

	[RplProp()]
	ref PS_ReplicatedBasicMap<int, string> m_mVoiceRooms = new PS_ReplicatedBasicMap<int, string>();

	[RplProp()]
	ref PS_ReplicatedBasicMap<int, int> m_mPlayersRooms = new PS_ReplicatedBasicMap<int, int>();

	int m_iLastRoomId = 1;

	ref ScriptInvoker m_eOnRoomChanged = new ScriptInvoker();

	bool m_bRplLoaded = false;
	bool IsReplicated()
	{
		return m_bRplLoaded;
	}

	override protected void OnPostInit(IEntity owner)
	{
		SCR_BaseGameMode baseGameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		baseGameMode.GetOnPlayerConnected().Insert(OnPlayerConnected);

		m_mVoiceRooms.Set(0, "");
		m_mVoiceRoomsFromName[""] = 0;
		if (Replication.IsServer()) m_bRplLoaded = true;
	}

	void OnPlayerConnected(int playerId)
	{
		GetOrCreateRoomWithFaction("", "#PS-VoNRoom_Local" + playerId.ToString());
		GetOrCreateRoomWithFaction("", "#PS-VoNRoom_Public" + playerId.ToString());
	}

	static PS_VoNRoomsManager GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return PS_VoNRoomsManager.Cast(gameMode.FindComponent(PS_VoNRoomsManager));
		else
			return null;
	}

	// ------------------------- Room changing -------------------------
	void MoveToRoom(int playerId, FactionKey factionKey, string roomName)
	{
		if (!Replication.IsServer()) return;

		int roomId = GetOrCreateRoomWithFaction(factionKey, roomName);

		if (roomId == GetPlayerRoom(playerId))
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		PlayerController playerController = playerManager.GetPlayerController(playerId);

		if (playerController)
		{
			PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
			SCR_EGameModeState state = gameMode.GetState();

			if (roomName.StartsWith("#PS-VoNRoom_Local"))
			{
				playableController.SetVoNKey(roomName, roomId.ToString());
			} else if (state == SCR_EGameModeState.GAME) {
				playableController.SetVoNKey("Menu" + factionKey + roomName, roomId.ToString());
			} else if (state == SCR_EGameModeState.BRIEFING) {
				RplId playableId = playableManager.GetPlayableByPlayer(playerId);
				int GroupCallSign = playableManager.GetGroupCallsignByPlayable(playableId);
				playableController.SetVoNKey("Menu" + factionKey + GroupCallSign.ToString(), roomId.ToString());
			}
			else playableController.SetVoNKey("Menu" + factionKey, roomId.ToString());
		}

		RPC_MoveToRoom(playerId, roomId);
		Rpc(RPC_MoveToRoom, playerId, roomId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_MoveToRoom(int playerId, int roomId)
	{
		int oldRoomId = GetPlayerRoom(playerId);
		m_mPlayersRooms.Set(playerId, roomId);
		m_eOnRoomChanged.Invoke(playerId, roomId, oldRoomId);
	}

	void RestoreRoom(int playerId)
	{
		int roomId = GetPlayerRoom(playerId);
		if (roomId == -1) return;

		string roomKey = GetRoomName(roomId);
		string factionKey = "";
		string roomName = "#PS-VoNRoom_Global";
		if (roomKey.Contains("|")) {
			array<string> outTokens = new array<string>();
			roomKey.Split("|", outTokens, false);
			factionKey = outTokens[0];
			roomName = outTokens[1];
		}

		MoveToRoom(playerId, factionKey, roomName);
	}

	// ------------------------- Room creation -------------------------
	int GetOrCreateRoomWithFaction(FactionKey factionKey, string roomName)
	{
		VoNRoomKey roomKey = factionKey + "|" + roomName;
		if (roomKey == "|") roomKey = "";
		return GetOrCreateRoom(roomKey);
	}

	int GetOrCreateRoom(VoNRoomKey roomKey)
	{
		if (!Replication.IsServer()) return -1;
		if (!m_mVoiceRoomsFromName.Contains(roomKey)) {
			RPC_CreateRoom(m_iLastRoomId, roomKey);
			Rpc(RPC_CreateRoom, m_iLastRoomId, roomKey);
			m_iLastRoomId++;
		}
		return m_mVoiceRoomsFromName[roomKey];
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_CreateRoom(int roomId, VoNRoomKey roomKey)
	{
		m_mVoiceRoomsFromName[roomKey] = roomId;
		m_mVoiceRooms.Set(roomId, roomKey);
	}

	// ------------------------- Get -------------------------
	int GetPlayerRoom(int playerId)
	{
		if (!m_mPlayersRooms.Contains(playerId)) return -1;
		return m_mPlayersRooms.Get(playerId);
	}

	int GetRoomWithFaction(FactionKey factionKey, string roomName)
	{
		string roomKey = factionKey + "|" + roomName;
		if (roomKey == "|") roomKey = "";
		if (!m_mVoiceRoomsFromName.Contains(roomKey)) return -1;
		return m_mVoiceRoomsFromName[roomKey];
	}

	string GetRoomName(int roomId)
	{
		if (!m_mVoiceRooms.Contains(roomId)) return "";
		return m_mVoiceRooms.Get(roomId);
	}

	void GetPlayersPublicRooms(out notnull array<int> rooms)
	{
		array<int> playerIds = new array<int>();
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			int playerRoomId = GetPlayerRoom(playerId);
			if (rooms.Contains(playerRoomId)) continue;
			if (IsPublicRoom(playerRoomId))
			{
				rooms.Insert(playerRoomId);
			}
		}
	}

	void GetPlayersInRoom(out notnull array<int> players, int roomId)
	{
		array<int> playerIds = new array<int>();
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			if (GetPlayerRoom(playerId) == roomId)
				players.Insert(playerId);
		}
	}

	bool IsPublicRoom(int roomId)
	{
		string playerRoomName = GetRoomName(roomId);
		if (playerRoomName.Length() <= 13) return false;
		if (playerRoomName.ContainsAt("Public", 13))
			return true;
		return false;
	}

	bool IsFactionRoom(int roomId, FactionKey factionKey)
	{
		string playerRoomName = GetRoomName(roomId);
		if (factionKey == "")
		{
			if (playerRoomName.StartsWith("|")) return true;
			return false;
		}
		if (playerRoomName.StartsWith(factionKey + "|"))
			return true;
		return false;
	}

	bool IsGlobalRoom(int roomId)
	{
		string playerRoomName = GetRoomName(roomId);
		return playerRoomName == "|#PS-VoNRoom_Global";
	}

	bool IsLocalRoom(int roomId)
	{
		string playerRoomName = GetRoomName(roomId);
		return playerRoomName.StartsWith("|#PS-VoNRoom_Local");
	}

	void RemapPlayerId(int oldPlayerId, int newPlayerId)
	{
		if (m_mPlayersRooms.Contains(oldPlayerId))
		{
			m_mPlayersRooms.Set(newPlayerId, m_mPlayersRooms.Get(oldPlayerId));
			m_mPlayersRooms.Remove(oldPlayerId);
		}
	}
};
