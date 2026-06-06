// PS_FreezeTimeCounter — Freeze time countdown display.
// Layout: UI/layouts/FreezeTime/FreezeTimeCounter.layout
// Handler class referenced by the layout — name must match exactly.

class PS_FreezeTimeCounter : ScriptedWidgetComponent
{
	// --- Widgets ---
	protected Widget m_wRoot;
	protected TextWidget m_wFreezeTimeText;
	protected TextWidget m_wFreezeTimeCounterText;

	// --- State ---
	protected PS_GameModeCoop m_GameModeCoop;
	protected float m_fTotalFreezeTime;
	protected bool m_bVisible;

	// =====================================================================
	// LIFECYCLE
	// =====================================================================

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		m_wRoot = w;
		m_wFreezeTimeText = TextWidget.Cast(w.FindAnyWidget("FreezeTimeText"));
		m_wFreezeTimeCounterText = TextWidget.Cast(w.FindAnyWidget("FreezeTimeCounterText"));

		m_GameModeCoop = PS_GameModeCoop.GetInstance();

		if (m_GameModeCoop)
		{
			m_fTotalFreezeTime = m_GameModeCoop.GetFreezeTimeDuration() / 1000.0;
			m_GameModeCoop.GetOnGameStateChanged().Insert(OnGameStateChanged);
		}

		// Start hidden.
		w.SetVisible(false);

		GetGame().GetCallqueue().CallLater(UpdateDisplay, 200, true);
	}

	override void HandlerDeattached(Widget w)
	{
		super.HandlerDeattached(w);
		GetGame().GetCallqueue().Remove(UpdateDisplay);

		if (m_GameModeCoop)
			m_GameModeCoop.GetOnGameStateChanged().Remove(OnGameStateChanged);
	}

	// =====================================================================
	// EVENT HANDLERS
	// =====================================================================

	protected void OnGameStateChanged(int state)
	{
		SCR_EGameModeState gameState = state;
		bool shouldShow = (gameState == SCR_EGameModeState.GAME);

		if (m_wRoot)
			m_wRoot.SetVisible(shouldShow);

		m_bVisible = shouldShow;
	}

	// =====================================================================
	// DISPLAY UPDATE
	// =====================================================================

	protected void UpdateDisplay()
	{
		if (!m_bVisible || !m_GameModeCoop)
			return;

		float remaining = m_GameModeCoop.GetFreezeTimeRemaining();

		if (remaining <= 0)
		{
			if (m_wRoot)
				m_wRoot.SetVisible(false);

			m_bVisible = false;
			return;
		}

		// Format as MM:SS.
		int totalSeconds = Math.Floor(remaining);
		int minutes = totalSeconds / 60;
		int seconds = totalSeconds - (minutes * 60);

		if (m_wFreezeTimeCounterText)
			m_wFreezeTimeCounterText.SetTextFormat("%1:%2", minutes.ToString(2), seconds.ToString(2));
	}
}
