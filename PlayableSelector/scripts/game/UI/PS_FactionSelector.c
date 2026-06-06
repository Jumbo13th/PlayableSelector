// PS_FactionSelector — Faction tab button in the lobby sidebar.
// Layout: {DA22ED7112FA8028}UI/Lobby/FactionSelector.layout
// Handler class referenced by the layout — name must match exactly.
//
// Displays faction name, flag, color, and player count.
// Clicking switches the lobby to show that faction's slots.

class PS_FactionSelector : SCR_ButtonBaseComponent
{
	// --- Widgets (looked up by name from layout) ---
	protected ImageWidget m_wFactionFlag;
	protected TextWidget m_wFactionName;
	protected ImageWidget m_wFactionColor;
	protected TextWidget m_wFactionCounter;
	protected ImageWidget m_wLockImage;

	// --- State ---
	protected string m_sFactionKey;
	protected SCR_Faction m_Faction;
	protected PS_CoopLobby m_CoopLobby;
	protected bool m_bSelected;

	// =====================================================================
	// INIT — called by PS_CoopLobby after widget creation
	// =====================================================================

	void Init(SCR_Faction faction, string factionKey, PS_CoopLobby lobby)
	{
		m_Faction = faction;
		m_sFactionKey = factionKey;
		m_CoopLobby = lobby;

		Widget root = GetRootWidget();
		if (!root)
			return;

		m_wFactionFlag = ImageWidget.Cast(root.FindAnyWidget("FactionFlag"));
		m_wFactionName = TextWidget.Cast(root.FindAnyWidget("FactionName"));
		m_wFactionColor = ImageWidget.Cast(root.FindAnyWidget("FactionColor"));
		m_wFactionCounter = TextWidget.Cast(root.FindAnyWidget("FactionCounter"));
		m_wLockImage = ImageWidget.Cast(root.FindAnyWidget("LockImage"));

		// Display faction info.
		if (m_Faction)
		{
			UIInfo uiInfo = m_Faction.GetUIInfo();
			if (uiInfo)
			{
				if (m_wFactionName)
					m_wFactionName.SetText(uiInfo.GetName());

				// WHY: UIInfo.SetIconTo doesn't exist. Load icon via ImageWidget API.
				if (m_wFactionFlag && uiInfo.GetIconPath() != "")
					m_wFactionFlag.LoadImageTexture(0, uiInfo.GetIconPath());
			}

			if (m_wFactionColor)
				m_wFactionColor.SetColor(m_Faction.GetFactionColor());
		}
		else
		{
			// Fallback for unknown faction.
			if (m_wFactionName)
				m_wFactionName.SetText(factionKey);
		}

		if (m_wLockImage)
			m_wLockImage.SetVisible(false);

		// Subscribe to click.
		m_OnClicked.Insert(OnClicked);
	}

	// =====================================================================
	// INTERACTION
	// =====================================================================

	protected void OnClicked()
	{
		if (m_CoopLobby)
			m_CoopLobby.SwitchCurrentFaction(m_sFactionKey);
	}

	// =====================================================================
	// UPDATE
	// =====================================================================

	void SetCounts(int occupied, int total, int locked)
	{
		if (m_wFactionCounter)
		{
			int available = total - locked;
			m_wFactionCounter.SetTextFormat("%1 / %2", occupied, available);
		}

		if (m_wLockImage)
			m_wLockImage.SetVisible(locked > 0);
	}

	void SetSelected(bool selected)
	{
		m_bSelected = selected;
		// WHY: visual feedback for which faction is active.
		// The layout uses button focus/toggle state for highlighting.
		Widget root = GetRootWidget();
		if (root)
		{
			SCR_ButtonBaseComponent btnComp = SCR_ButtonBaseComponent.Cast(root.FindHandler(SCR_ButtonBaseComponent));
			if (btnComp)
				btnComp.SetToggled(selected);
		}
	}

	string GetFactionKey()
	{
		return m_sFactionKey;
	}
}
