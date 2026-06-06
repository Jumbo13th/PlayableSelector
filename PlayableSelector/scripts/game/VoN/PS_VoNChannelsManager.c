// PS_VoNChannelsManager — Voice channel management for the lobby.
// Lives on the GameMode entity as a SCR_BaseGameModeComponent.
//
// Uses STRING-BASED channel keys (not spatial positions) to route voice.
// Players are assigned to channels based on faction, group, and game state.
// The engine's radio encryption keys are used to isolate channels.
//
// Channel key format:
//   "Lobby"                           — global lobby channel (SLOTSELECTION)
//   "Faction_{factionKey}"            — faction-wide channel
//   "Group_{groupId}_{factionKey}"    — squad channel
//   "Briefing_{factionKey}"           — briefing channel (leaders only)
//   "Spectator_{playerId}"           — muted spectator channel
//
// Pattern reference: EchoLobby's EH_VoNChannelsManager (string-based, cleaner than spatial).
// Replication: RPCs for live updates, RplSave/RplLoad for JIP.

class PS_VoNChannelsManagerClass : SCR_BaseGameModeComponentClass
{
}

class PS_VoNChannelsManager : SCR_BaseGameModeComponent
{
	// =====================================================================
	// SINGLETON
	// =====================================================================

	protected static PS_VoNChannelsManager s_Instance;

	static PS_VoNChannelsManager GetInstance()
	{
		return s_Instance;
	}

	// =====================================================================
	// DATA — player→channel assignments. Synced via RPCs + RplSave/RplLoad.
	// =====================================================================

	// Current channel assignment per player. Server is authority.
	protected ref map<int, string> m_mPlayerChannels = new map<int, string>();

	// All created channel keys. Used for UI display.
	protected ref array<string> m_aChannels = {};

	// =====================================================================
	// EVENTS
	// =====================================================================

	protected ref ScriptInvoker m_OnPlayerChannelChanged = new ScriptInvoker(); // (int playerId, string channelKey)

	ScriptInvoker GetOnPlayerChannelChanged()
	{
		return m_OnPlayerChannelChanged;
	}

	// =====================================================================
	// LIFECYCLE
	// =====================================================================

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		s_Instance = this;

		Print("[PS_VoN] VoNChannelsManager initialized", LogLevel.NORMAL);
	}

	override void OnDelete(IEntity owner)
	{
		super.OnDelete(owner);
		if (s_Instance == this)
			s_Instance = null;
	}

	// =====================================================================
	// CHANNEL KEY BUILDERS
	// =====================================================================

	static string GetLobbyChannelKey()
	{
		return "Lobby";
	}

	static string GetFactionChannelKey(string factionKey)
	{
		return "Faction_" + factionKey;
	}

	static string GetGroupChannelKey(int groupId, string factionKey)
	{
		return string.Format("Group_%1_%2", groupId, factionKey);
	}

	static string GetBriefingChannelKey(string factionKey)
	{
		return "Briefing_" + factionKey;
	}

	static string GetSpectatorChannelKey(int playerId)
	{
		return string.Format("Spectator_%1", playerId);
	}

	// =====================================================================
	// SERVER-SIDE API — assign players to channels
	// =====================================================================

	// Assign a player to a channel. Server-only.
	void SetPlayerChannel_S(int playerId, string channelKey)
	{
		if (!Replication.IsServer())
			return;

		string currentChannel;
		if (m_mPlayerChannels.Find(playerId, currentChannel))
		{
			if (currentChannel == channelKey)
				return; // Already in this channel.
		}

		Print(string.Format("[PS_VoN] Player %1 → channel '%2'", playerId, channelKey), LogLevel.NORMAL);

		// Ensure channel exists.
		InitChannelIfNeeded(channelKey);

		// Apply locally.
		ApplySetPlayerChannel(playerId, channelKey);

		// Broadcast to all clients.
		Rpc(RpcDo_SetPlayerChannel, playerId, channelKey);

		// Set radio encryption key on the player's controller.
		SetPlayerRadioKey_S(playerId, channelKey);
	}

	// Move all players in a faction to a specific channel.
	void SetFactionChannel_S(string factionKey, string channelKey)
	{
		if (!Replication.IsServer())
			return;

		PS_LobbyManager mgr = PS_LobbyManager.GetInstance();
		if (!mgr)
			return;

		array<ref PS_SlotData> slots = mgr.GetSlotsForFaction(factionKey);
		foreach (PS_SlotData slot : slots)
		{
			if (slot.m_iPlayerId >= 0)
				SetPlayerChannel_S(slot.m_iPlayerId, channelKey);
		}
	}

	// Automatically assign channels based on current game state.
	// WHY: called on state transitions to move everyone to the right channel.
	void AssignChannelsForState_S(SCR_EGameModeState state)
	{
		if (!Replication.IsServer())
			return;

		PS_LobbyManager mgr = PS_LobbyManager.GetInstance();
		if (!mgr)
			return;

		Print(string.Format("[PS_VoN] Assigning channels for state: %1",
			typename.EnumToString(SCR_EGameModeState, state)), LogLevel.NORMAL);

		array<ref PS_SlotData> slots = mgr.GetSlots();

		foreach (PS_SlotData slot : slots)
		{
			if (slot.m_iPlayerId < 0)
				continue;

			string channelKey;

			switch (state)
			{
				case SCR_EGameModeState.SLOTSELECTION:
					// WHY: during slot selection, players hear their faction.
					channelKey = GetFactionChannelKey(slot.m_sFactionKey);
					break;

				case SCR_EGameModeState.BRIEFING:
					// WHY: during briefing, players are in squad channels.
					// Leaders also hear the faction briefing channel.
					channelKey = GetGroupChannelKey(slot.m_iGroupId, slot.m_sFactionKey);
					break;

				case SCR_EGameModeState.GAME:
					// WHY: during game, players use squad radio channels.
					channelKey = GetGroupChannelKey(slot.m_iGroupId, slot.m_sFactionKey);
					break;

				default:
					channelKey = GetFactionChannelKey(slot.m_sFactionKey);
					break;
			}

			SetPlayerChannel_S(slot.m_iPlayerId, channelKey);
		}
	}

	// Move a player to the spectator (muted) channel.
	void SetPlayerSpectator_S(int playerId)
	{
		SetPlayerChannel_S(playerId, GetSpectatorChannelKey(playerId));
	}

	// =====================================================================
	// RADIO KEY — set encryption key on player's radio for channel isolation
	// =====================================================================

	protected void SetPlayerRadioKey_S(int playerId, string channelKey)
	{
		if (!Replication.IsServer())
			return;

		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;

		IEntity controlledEntity = pm.GetPlayerControlledEntity(playerId);
		if (!controlledEntity)
			return;

		// WHY: find the radio on the character and set its encryption key.
		// The engine uses encryption keys to isolate voice channels.
		// Players with the same key on the same frequency can hear each other.
		BaseRadioComponent radio = BaseRadioComponent.Cast(
			controlledEntity.FindComponent(BaseRadioComponent));

		if (radio && radio.TransceiversCount() > 0)
		{
			BaseTransceiver transceiver = radio.GetTransceiver(0);
			if (transceiver)
				radio.SetEncryptionKey(channelKey);
		}
	}

	// =====================================================================
	// APPLY — shared mutation for server + client
	// =====================================================================

	protected void ApplySetPlayerChannel(int playerId, string channelKey)
	{
		m_mPlayerChannels.Set(playerId, channelKey);
		m_OnPlayerChannelChanged.Invoke(playerId, channelKey);
	}

	protected void InitChannelIfNeeded(string channelKey)
	{
		if (m_aChannels.Contains(channelKey))
			return;

		m_aChannels.Insert(channelKey);
	}

	// =====================================================================
	// RPCs — Broadcast
	// =====================================================================

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SetPlayerChannel(int playerId, string channelKey)
	{
		InitChannelIfNeeded(channelKey);
		ApplySetPlayerChannel(playerId, channelKey);
	}

	// =====================================================================
	// JIP STREAMING
	// =====================================================================

	override event protected bool RplSave(ScriptBitWriter writer)
	{
		// Write channels list.
		int channelCount = m_aChannels.Count();
		writer.WriteInt(channelCount);
		foreach (string ch : m_aChannels)
		{
			writer.WriteString(ch);
		}

		// Write player assignments.
		int playerCount = m_mPlayerChannels.Count();
		writer.WriteInt(playerCount);
		for (int i = 0; i < playerCount; i++)
		{
			writer.WriteInt(m_mPlayerChannels.GetKey(i));
			writer.WriteString(m_mPlayerChannels.GetElement(i));
		}

		Print(string.Format("[PS_VoN] RplSave: %1 channels, %2 assignments", channelCount, playerCount), LogLevel.NORMAL);
		return true;
	}

	override event protected bool RplLoad(ScriptBitReader reader)
	{
		// Read channels.
		int channelCount;
		reader.ReadInt(channelCount);
		for (int i = 0; i < channelCount; i++)
		{
			string ch;
			reader.ReadString(ch);
			m_aChannels.Insert(ch);
		}

		// Read player assignments.
		int playerCount;
		reader.ReadInt(playerCount);
		for (int i = 0; i < playerCount; i++)
		{
			int playerId;
			string channelKey;
			reader.ReadInt(playerId);
			reader.ReadString(channelKey);
			m_mPlayerChannels.Set(playerId, channelKey);
		}

		Print(string.Format("[PS_VoN] RplLoad: %1 channels, %2 assignments", channelCount, playerCount), LogLevel.NORMAL);

		// Fire events deferred.
		GetGame().GetCallqueue().CallLater(FireJIPEvents, 0, false);
		return true;
	}

	protected void FireJIPEvents()
	{
		for (int i = 0; i < m_mPlayerChannels.Count(); i++)
		{
			m_OnPlayerChannelChanged.Invoke(m_mPlayerChannels.GetKey(i), m_mPlayerChannels.GetElement(i));
		}
	}

	// =====================================================================
	// QUERY API — for UI
	// =====================================================================

	string GetPlayerChannel(int playerId)
	{
		string ch;
		if (m_mPlayerChannels.Find(playerId, ch))
			return ch;
		return "";
	}

	array<string> GetChannels()
	{
		return m_aChannels;
	}

	// Get all players in a specific channel.
	array<int> GetPlayersInChannel(string channelKey)
	{
		array<int> result = {};
		for (int i = 0; i < m_mPlayerChannels.Count(); i++)
		{
			if (m_mPlayerChannels.GetElement(i) == channelKey)
				result.Insert(m_mPlayerChannels.GetKey(i));
		}
		return result;
	}

	// =====================================================================
	// GAME STATE INTEGRATION — hook into state changes
	// =====================================================================

	override void OnGameStateChanged(SCR_EGameModeState state)
	{
		super.OnGameStateChanged(state);

		// WHY: automatically reassign channels when game state changes.
		if (Replication.IsServer())
			AssignChannelsForState_S(state);
	}

	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);

		if (!Replication.IsServer())
			return;

		// Remove from channel tracking.
		m_mPlayerChannels.Remove(playerId);
	}
}
