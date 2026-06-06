class PS_LobbyMetrics
{
	protected static ref PS_LobbyMetrics s_Instance;

	protected int m_iRpcCount;
	protected int m_iRpcCountPrevSecond;
	protected int m_iRpcPeakPerSecond;
	protected float m_fLastSecondTime;

	protected int m_iRegistrationBurstCount;
	protected float m_fRegistrationBurstStart;

	protected ref array<string> m_aRecentEvents = {};
	protected static const int MAX_RECENT_EVENTS = 50;

	static PS_LobbyMetrics GetInstance()
	{
		if (!s_Instance)
			s_Instance = new PS_LobbyMetrics();
		return s_Instance;
	}

	protected static float Now()
	{
		return GetGame().GetWorld().GetWorldTime();
	}

	protected static string Timestamp()
	{
		int ms = Now();
		int sec = ms / 1000;
		int min = sec / 60;
		return string.Format("%1:%2.%3", min, (sec % 60).ToString(2), (ms % 1000).ToString(3));
	}

	protected static void PushEvent(string msg)
	{
		PS_LobbyMetrics inst = GetInstance();
		string entry = Timestamp() + " " + msg;
		inst.m_aRecentEvents.Insert(entry);
		if (inst.m_aRecentEvents.Count() > MAX_RECENT_EVENTS)
			inst.m_aRecentEvents.RemoveOrdered(0);
	}

	protected static void TrackRpc(string rpcName)
	{
		PS_LobbyMetrics inst = GetInstance();
		inst.m_iRpcCount++;

		float now = Now();
		if (now - inst.m_fLastSecondTime >= 1000)
		{
			inst.m_iRpcCountPrevSecond = inst.m_iRpcCount;
			if (inst.m_iRpcCount > inst.m_iRpcPeakPerSecond)
				inst.m_iRpcPeakPerSecond = inst.m_iRpcCount;
			inst.m_iRpcCount = 0;
			inst.m_fLastSecondTime = now;
		}

		PushEvent(string.Format("RPC %1 [%2/s]", rpcName, inst.m_iRpcCount));
	}

	// ---- Instrumentation points ----

	static void OnTakeSlot(int playerId, RplId playableId)
	{
		TrackRpc(string.Format("TakeSlot p=%1 slot=%2", playerId, playableId));
	}

	static void OnLeaveSlot(int playerId)
	{
		TrackRpc(string.Format("LeaveSlot p=%1", playerId));
	}

	static void OnSetPlayerPlayable(int playerId, RplId playableId)
	{
		TrackRpc(string.Format("SetPlayerPlayable p=%1 slot=%2", playerId, playableId));
	}

	static void OnSetPlayerState(int playerId, int state)
	{
		TrackRpc(string.Format("SetPlayerState p=%1 s=%2", playerId, state));
	}

	static void OnSetFaction(int playerId, string factionKey)
	{
		TrackRpc(string.Format("SetFaction p=%1 f=%2", playerId, factionKey));
	}

	static void OnRegisterPlayable(RplId playableId)
	{
		PS_LobbyMetrics inst = GetInstance();
		if (inst.m_iRegistrationBurstCount == 0)
			inst.m_fRegistrationBurstStart = Now();
		inst.m_iRegistrationBurstCount++;

		TrackRpc(string.Format("RegisterPlayable id=%1 burst=%2", playableId, inst.m_iRegistrationBurstCount));
	}

	static void OnUnregisterPlayable(RplId playableId)
	{
		TrackRpc(string.Format("UnregisterPlayable id=%1", playableId));
	}

	static void OnRegisterVehicle(RplId rplId)
	{
		TrackRpc(string.Format("RegisterVehicle id=%1", rplId));
	}

	static void OnSetPlayerName(int playerId)
	{
		TrackRpc(string.Format("SetPlayerName p=%1", playerId));
	}

	static void OnSetGroupId(RplId playableId, int groupId)
	{
		TrackRpc(string.Format("SetGroupId slot=%1 g=%2", playableId, groupId));
	}

	static void OnSetPin(int playerId, bool pin)
	{
		TrackRpc(string.Format("SetPin p=%1 pin=%2", playerId, pin));
	}

	static void OnSetFactionReady(string factionKey, int ready)
	{
		TrackRpc(string.Format("SetFactionReady f=%1 r=%2", factionKey, ready));
	}

	static void OnDamageState(RplId playableId, int state)
	{
		TrackRpc(string.Format("DamageState slot=%1 s=%2", playableId, state));
	}

	static void OnMoveToRoom(int playerId, string roomName)
	{
		TrackRpc(string.Format("MoveToRoom p=%1 room=%2", playerId, roomName));
	}

	static void OnRemapPlayerIds(int oldId, int newId)
	{
		TrackRpc(string.Format("RemapIds old=%1 new=%2", oldId, newId));
	}

	// ---- Connection events ----

	static void OnPlayerConnected(int playerId)
	{
		PushEvent(string.Format("CONNECT p=%1", playerId));
	}

	static void OnPlayerDisconnected(int playerId, KickCauseCode cause)
	{
		PushEvent(string.Format("DISCONNECT p=%1 cause=%2", playerId, cause));
	}

	static void OnJipReceived(int playableCount)
	{
		PushEvent(string.Format("JIP_RECEIVED playables=%1", playableCount));
	}

	static void OnGameStateChanged(string state)
	{
		PS_LobbyMetrics inst = GetInstance();
		PushEvent(string.Format("STATE_CHANGE -> %1", state));

		if (inst.m_iRegistrationBurstCount > 0)
		{
			float duration = Now() - inst.m_fRegistrationBurstStart;
			PrintFormat("[PS_LobbyMetrics] Registration complete: %1 playables in %2ms", inst.m_iRegistrationBurstCount, duration);
		}

		PrintFormat("[PS_LobbyMetrics] State -> %1 | rpcPeak=%2/s", state, inst.m_iRpcPeakPerSecond);
	}

	// ---- Dump on kick ----

	static void DumpOnKick(int playerId, string reason)
	{
		PS_LobbyMetrics inst = GetInstance();

		PrintFormat("[PS_LobbyMetrics] ===== KICK DETECTED: player=%1 reason=%2 =====", playerId, reason);
		PrintFormat("[PS_LobbyMetrics] rpcPeak=%1/s rpcLastSecond=%2", inst.m_iRpcPeakPerSecond, inst.m_iRpcCountPrevSecond);

		int playerCount = 0;
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		playerCount = players.Count();
		PrintFormat("[PS_LobbyMetrics] players=%1 registeredPlayables=%2", playerCount, inst.m_iRegistrationBurstCount);

		PrintFormat("[PS_LobbyMetrics] Recent events before kick:");
		for (int i = 0; i < inst.m_aRecentEvents.Count(); i++)
		{
			PrintFormat("[PS_LobbyMetrics]   %1", inst.m_aRecentEvents[i]);
		}
		PrintFormat("[PS_LobbyMetrics] ===== END KICK DUMP =====");
	}
}
