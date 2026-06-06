// PS_GameModeHeader — Top bar showing lobby state tabs and mission info.
// Layout: GameModeHeader.layout (embedded in CoopLobby.layout)
// Handler class referenced by the layout — name must match exactly.
//
// Displays: state tabs (Preview, Lobby, Briefing, InGame, Debriefing),
// mission title/author, weather/time info, and admin advance button.

class PS_GameModeHeader : ScriptedWidgetComponent
{
	// --- Widgets ---
	protected ButtonWidget m_wPreviewButton;
	protected ButtonWidget m_wLobbyButton;
	protected ButtonWidget m_wBriefingButton;
	protected ButtonWidget m_wInGameButton;
	protected ButtonWidget m_wDebriefingButton;
	protected ButtonWidget m_wAdvanceButton;
	protected TextWidget m_wTitleText;
	protected RichTextWidget m_wAuthorText;
	protected ImageWidget m_wWeatherImage;
	protected ImageWidget m_wTimeImage;
	protected ImageWidget m_wHeaderLine;

	// --- Handlers ---
	protected PS_GameModeHeaderButton m_hButtonLobby;
	protected PS_GameModeHeaderButton m_hButtonBriefing;
	protected PS_GameModeHeaderButton m_hButtonInGame;
	protected PS_GameModeHeaderButton m_hButtonDebriefing;

	// --- State ---
	protected PS_GameModeCoop m_GameModeCoop;
	protected bool m_bInitialized;
	protected SCR_EGameModeState m_eLastState;

	// =====================================================================
	// LIFECYCLE
	// =====================================================================

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		m_wPreviewButton = ButtonWidget.Cast(w.FindAnyWidget("PreviewButton"));
		m_wLobbyButton = ButtonWidget.Cast(w.FindAnyWidget("LobbyButton"));
		m_wBriefingButton = ButtonWidget.Cast(w.FindAnyWidget("BriefingButton"));
		m_wInGameButton = ButtonWidget.Cast(w.FindAnyWidget("InGameButton"));
		m_wDebriefingButton = ButtonWidget.Cast(w.FindAnyWidget("DebriefingButton"));
		m_wAdvanceButton = ButtonWidget.Cast(w.FindAnyWidget("AdvanceButton"));
		m_wTitleText = TextWidget.Cast(w.FindAnyWidget("TitleText"));
		m_wAuthorText = RichTextWidget.Cast(w.FindAnyWidget("AuthorRichText"));
		m_wWeatherImage = ImageWidget.Cast(w.FindAnyWidget("WeatherImage"));
		m_wTimeImage = ImageWidget.Cast(w.FindAnyWidget("TimeImage"));
		m_wHeaderLine = ImageWidget.Cast(w.FindAnyWidget("GameModeHeaderLine"));

		// Find button handlers.
		if (m_wLobbyButton)
			m_hButtonLobby = PS_GameModeHeaderButton.Cast(m_wLobbyButton.FindHandler(PS_GameModeHeaderButton));
		if (m_wBriefingButton)
			m_hButtonBriefing = PS_GameModeHeaderButton.Cast(m_wBriefingButton.FindHandler(PS_GameModeHeaderButton));
		if (m_wInGameButton)
			m_hButtonInGame = PS_GameModeHeaderButton.Cast(m_wInGameButton.FindHandler(PS_GameModeHeaderButton));
		if (m_wDebriefingButton)
			m_hButtonDebriefing = PS_GameModeHeaderButton.Cast(m_wDebriefingButton.FindHandler(PS_GameModeHeaderButton));

		// Wire advance button.
		if (m_wAdvanceButton)
		{
			SCR_ButtonBaseComponent advanceComp = SCR_ButtonBaseComponent.Cast(
				m_wAdvanceButton.FindHandler(SCR_ButtonBaseComponent));
			if (advanceComp)
				advanceComp.m_OnClicked.Insert(OnAdvanceClicked);
		}

		// Set mission info.
		SetMissionInfo();

		m_GameModeCoop = PS_GameModeCoop.GetInstance();
		m_bInitialized = true;
	}

	// =====================================================================
	// UPDATE — called every frame from PS_CoopLobby.OnMenuUpdate
	// =====================================================================

	void TryUpdate()
	{
		if (!m_bInitialized || !m_GameModeCoop)
			return;

		SCR_EGameModeState currentState = m_GameModeCoop.GetLobbyState();
		if (currentState == m_eLastState)
			return;

		m_eLastState = currentState;
		UpdateStateButtons(currentState);
		UpdateAdvanceButton(currentState);
	}

	// =====================================================================
	// STATE DISPLAY
	// =====================================================================

	protected void UpdateStateButtons(SCR_EGameModeState state)
	{
		// WHY: highlight the active state tab and enable/disable navigation.
		if (m_hButtonLobby)
			m_hButtonLobby.SetActive(state == SCR_EGameModeState.SLOTSELECTION);
		if (m_hButtonBriefing)
			m_hButtonBriefing.SetActive(state == SCR_EGameModeState.BRIEFING);
		if (m_hButtonInGame)
			m_hButtonInGame.SetActive(state == SCR_EGameModeState.GAME);
		if (m_hButtonDebriefing)
			m_hButtonDebriefing.SetActive(state == SCR_EGameModeState.DEBRIEFING);
	}

	protected void UpdateAdvanceButton(SCR_EGameModeState state)
	{
		if (!m_wAdvanceButton)
			return;

		// WHY: only show advance button for admins, and only when there's a next state.
		bool canAdvance = false;

		if (m_GameModeCoop.IsAdminMode())
		{
			// TODO: proper admin check for the local player.
			// For now, always show if admin mode is enabled.
			canAdvance = (state != SCR_EGameModeState.DEBRIEFING);
		}

		m_wAdvanceButton.SetVisible(canAdvance);
	}

	// =====================================================================
	// MISSION INFO
	// =====================================================================

	protected void SetMissionInfo()
	{
		// WHY: read mission title and author from the header.
		SCR_MissionHeader header = SCR_MissionHeader.Cast(GetGame().GetMissionHeader());
		if (!header)
			return;

		if (m_wTitleText)
			m_wTitleText.SetText(header.m_sName);

		if (m_wAuthorText)
			m_wAuthorText.SetText(header.m_sAuthor);
	}

	// =====================================================================
	// ACTIONS
	// =====================================================================

	protected void OnAdvanceClicked()
	{
		PS_LobbyPlayerComponent lobbyPlayer = PS_LobbyPlayerComponent.GetLocalInstance();
		if (lobbyPlayer)
			lobbyPlayer.AskAdvanceState();

		Print("[PS_Lobby] Admin: advancing state", LogLevel.NORMAL);
	}
}

// PS_GameModeHeaderButton — Individual state tab button in the header.
// Referenced by GameModeHeader.layout on each tab button.

class PS_GameModeHeaderButton : ScriptedWidgetComponent
{
	protected Widget m_wRoot;
	protected bool m_bActive;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wRoot = w;
	}

	void SetActive(bool active)
	{
		m_bActive = active;

		if (!m_wRoot)
			return;

		SCR_ButtonBaseComponent btnComp = SCR_ButtonBaseComponent.Cast(m_wRoot.FindHandler(SCR_ButtonBaseComponent));
		if (btnComp)
			btnComp.SetToggled(active);
	}

	bool IsActive()
	{
		return m_bActive;
	}
}
