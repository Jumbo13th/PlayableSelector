// PS_PlayerSelector — Player list entry in the lobby sidebar.
// Layout: {B55DD7054C5892AE}UI/Lobby/PlayerSelector.layout
// Handler class referenced by the layout — name must match exactly.
//
// Displays a connected player's name, faction color, and assigned role.

class PS_PlayerSelector : SCR_ButtonBaseComponent
{
	// --- Widgets ---
	protected ImageWidget m_wPlayerFactionColor;
	protected RichTextWidget m_wPlayerName;
	protected TextWidget m_wPlayerGroupName;
	protected ImageWidget m_wReadyImage;
	protected ImageWidget m_wImageCurrent;

	// --- State ---
	protected int m_iPlayerId;
	protected PS_CoopLobby m_CoopLobby;

	// =====================================================================
	// INIT
	// =====================================================================

	void Init(int playerId, string playerName, PS_CoopLobby lobby)
	{
		m_iPlayerId = playerId;
		m_CoopLobby = lobby;

		Widget root = GetRootWidget();
		if (!root)
			return;

		m_wPlayerFactionColor = ImageWidget.Cast(root.FindAnyWidget("PlayerFactionColor"));
		m_wPlayerName = RichTextWidget.Cast(root.FindAnyWidget("PlayerName"));
		m_wPlayerGroupName = TextWidget.Cast(root.FindAnyWidget("PlayerGroupName"));
		m_wReadyImage = ImageWidget.Cast(root.FindAnyWidget("ReadyImage"));
		m_wImageCurrent = ImageWidget.Cast(root.FindAnyWidget("ImageCurrent"));

		// WHY: hide widgets we don't manage — they show broken defaults otherwise.
		// VoiceHideableButton has PS_VoiceButton handler that shows speaker icon by default.
		Widget voiceBtn = root.FindAnyWidget("VoiceHideableButton");
		if (voiceBtn)
			voiceBtn.SetVisible(false);

		// WHY: PinImage/PinButton are for admin pin feature — not implemented.
		Widget pinImage = root.FindAnyWidget("PinImage");
		if (pinImage)
			pinImage.SetVisible(false);

		Widget pinButton = root.FindAnyWidget("PinButton");
		if (pinButton)
			pinButton.SetVisible(false);

		if (m_wPlayerName)
			m_wPlayerName.SetText(playerName);

		// Set initial state.
		Refresh();
	}

	// =====================================================================
	// UPDATE
	// =====================================================================

	void UpdateName(string name)
	{
		if (m_wPlayerName)
			m_wPlayerName.SetText(name);
	}

	void Refresh()
	{
		if (!m_CoopLobby)
			return;

		PS_LobbyManager mgr = m_CoopLobby.GetLobbyManager();
		if (!mgr)
			return;

		bool isLocalPlayer = (m_iPlayerId == m_CoopLobby.GetLocalPlayerId());

		// Update faction color and assigned role.
		PS_SlotData slot = mgr.FindSlotByPlayerId(m_iPlayerId);
		if (slot && m_wPlayerFactionColor)
		{
			SCR_FactionManager fm = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			if (fm)
			{
				SCR_Faction faction = SCR_Faction.Cast(fm.GetFactionByKey(slot.m_sFactionKey));
				if (faction)
					m_wPlayerFactionColor.SetColor(faction.GetFactionColor());
			}

			// Show assigned role as group name.
			if (m_wPlayerGroupName)
				m_wPlayerGroupName.SetText(slot.m_sName);
		}
		else
		{
			// Player has no slot — show as unassigned.
			if (m_wPlayerFactionColor)
				m_wPlayerFactionColor.SetColor(Color.Gray);
			if (m_wPlayerGroupName)
				m_wPlayerGroupName.SetText("");
		}

		// WHY: White is default. Green = Ready state (not implemented yet).
		// Old code priority: disconnected (dark gray) → ready (green) → admin (orange) → white.
		if (m_wPlayerName)
			m_wPlayerName.SetColor(Color.White);

		// WHY: ImageCurrent is a selection highlight — show only for the local player.
		if (m_wImageCurrent)
			m_wImageCurrent.SetVisible(isLocalPlayer);
	}

	int GetPlayerId()
	{
		return m_iPlayerId;
	}
}
