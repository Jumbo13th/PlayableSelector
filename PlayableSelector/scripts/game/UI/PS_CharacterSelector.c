// PS_CharacterSelector — Individual playable slot widget in the lobby.
// Layout: {3F761F63F1DF29D1}UI/Lobby/CharacterSelector.layout
// Handler class referenced by the layout — name must match exactly.
//
// Displays character name, role icon, assigned player name, and state.
// Clicking assigns/unassigns the local player to this slot.
// Reads data from PS_SlotData, sends actions via PS_LobbyPlayerComponent.

class PS_CharacterSelector : SCR_ButtonComponent
{
	// --- Widgets ---
	protected ImageWidget m_wFactionColor;
	protected ImageWidget m_wUnitIcon;
	protected TextWidget m_wCharacterClassName;
	protected ImageWidget m_wStateIcon;
	protected RichTextWidget m_wCharacterStatus;
	protected ButtonWidget m_wStateButton;

	// --- State ---
	protected int m_iSlotRplId = -1;
	protected PS_CoopLobby m_CoopLobby;
	protected int m_iCurrentPlayerId = -1;

	// =====================================================================
	// INIT
	// =====================================================================

	void Init(PS_SlotData slot, PS_CoopLobby lobby)
	{
		m_iSlotRplId = slot.m_iRplId;
		m_CoopLobby = lobby;

		Widget root = GetRootWidget();
		if (!root)
			return;

		m_wFactionColor = ImageWidget.Cast(root.FindAnyWidget("CharacterFactionColor"));
		m_wUnitIcon = ImageWidget.Cast(root.FindAnyWidget("UnitIcon"));
		m_wCharacterClassName = TextWidget.Cast(root.FindAnyWidget("CharacterClassName"));
		m_wStateIcon = ImageWidget.Cast(root.FindAnyWidget("StateIcon"));
		m_wCharacterStatus = RichTextWidget.Cast(root.FindAnyWidget("CharacterStatus"));
		m_wStateButton = ButtonWidget.Cast(root.FindAnyWidget("StateButton"));

		// WHY: hide widgets we don't manage — they show broken defaults otherwise.
		// VoiceHideableButton has PS_VoiceButton handler that shows mute icon by default.
		Widget voiceBtn = root.FindAnyWidget("VoiceHideableButton");
		if (voiceBtn)
			voiceBtn.SetVisible(false);

		// WHY: StateButton is for admin context menus — old code always hid it too.
		if (m_wStateButton)
			m_wStateButton.SetVisible(false);

		// Set character name.
		if (m_wCharacterClassName)
			m_wCharacterClassName.SetText(slot.m_sName);

		// Set faction color.
		if (m_wFactionColor)
		{
			SCR_FactionManager fm = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			if (fm)
			{
				SCR_Faction faction = SCR_Faction.Cast(fm.GetFactionByKey(slot.m_sFactionKey));
				if (faction)
					m_wFactionColor.SetColor(faction.GetFactionColor());
			}
		}

		// Subscribe to manager events for this specific slot.
		PS_LobbyManager mgr = PS_LobbyManager.GetInstance();
		if (mgr)
		{
			mgr.GetOnSlotUpdated().Insert(OnSlotUpdated);
			mgr.GetOnPlayerAssigned().Insert(OnPlayerChanged);
			mgr.GetOnPlayerUnassigned().Insert(OnPlayerChanged);
		}

		// Apply initial state.
		UpdateDisplay(slot);

		// Wire click handler.
		m_OnClicked.Insert(OnClicked);
	}

	// =====================================================================
	// DISPLAY UPDATE
	// =====================================================================

	protected void UpdateDisplay(PS_SlotData slot)
	{
		if (!slot)
			return;

		int oldPlayerId = m_iCurrentPlayerId;
		m_iCurrentPlayerId = slot.m_iPlayerId;

		// Player name / status text.
		if (m_wCharacterStatus)
		{
			if (slot.m_bLocked)
			{
				m_wCharacterStatus.SetText("Locked");
				m_wCharacterStatus.SetColor(Color.Gray);
			}
			else if (slot.IsDestroyed())
			{
				m_wCharacterStatus.SetText("KIA");
				m_wCharacterStatus.SetColor(Color.FromInt(0xFF2c2c2c));
			}
			else if (slot.m_iPlayerId >= 0)
			{
				// Show player name.
				string playerName = "";
				PS_LobbyManager mgr = PS_LobbyManager.GetInstance();
				if (mgr)
					playerName = mgr.GetPlayerName(slot.m_iPlayerId);

				if (playerName == "")
				{
					PlayerManager pm = GetGame().GetPlayerManager();
					if (pm)
						playerName = pm.GetPlayerName(slot.m_iPlayerId);
				}

				m_wCharacterStatus.SetText(playerName);

				// WHY: White is the default player color. Green = Ready state (not implemented yet).
				// Old code priority: disconnected → ready → admin → white.
				m_wCharacterStatus.SetColor(Color.White);
			}
			else
			{
				m_wCharacterStatus.SetText("");
				m_wCharacterStatus.SetColor(Color.White);
			}
		}

		// State icon — hidden for empty/occupied slots, visible only for locked/dead.
		// WHY: old code hid StateIcon for Empty state. We also hide it for normal occupied
		// slots since we show player name instead. Only show for locked/dead states.
		if (m_wStateIcon)
		{
			if (slot.m_bLocked)
			{
				m_wStateIcon.SetVisible(true);
				m_wStateIcon.LoadImageFromSet(0, "{3262679C50EF4F01}UI/imagesets/icons/icons_wrapperUI-glow.imageset", "Locked");
			}
			else if (slot.IsDestroyed())
			{
				m_wStateIcon.SetVisible(true);
				m_wStateIcon.LoadImageFromSet(0, "{3262679C50EF4F01}UI/imagesets/icons/icons_wrapperUI-glow.imageset", "death");
			}
			else
			{
				m_wStateIcon.SetVisible(false);
			}
		}

		// Notify parent RolesGroup about player change.
		if (oldPlayerId != m_iCurrentPlayerId)
		{
			PS_RolesGroup parentGroup = GetParentRolesGroup();
			if (parentGroup)
				parentGroup.OnSlotPlayerChanged(oldPlayerId, m_iCurrentPlayerId);
		}
	}

	// =====================================================================
	// EVENT HANDLERS
	// =====================================================================

	protected void OnSlotUpdated(PS_SlotData slot)
	{
		if (!slot || slot.m_iRplId != m_iSlotRplId)
			return;

		UpdateDisplay(slot);
	}

	protected void OnPlayerChanged(int playerId, int slotRplId)
	{
		if (slotRplId != m_iSlotRplId)
			return;

		PS_LobbyManager mgr = PS_LobbyManager.GetInstance();
		if (mgr)
		{
			PS_SlotData slot = mgr.FindSlotByRplId(m_iSlotRplId);
			if (slot)
				UpdateDisplay(slot);
		}
	}

	// =====================================================================
	// INTERACTION — click to take/leave slot
	// =====================================================================

	protected void OnClicked()
	{
		if (!m_CoopLobby)
			return;

		PS_LobbyPlayerComponent lobbyPlayer = m_CoopLobby.GetLobbyPlayerComponent();
		if (!lobbyPlayer)
			return;

		PS_LobbyManager mgr = m_CoopLobby.GetLobbyManager();
		if (!mgr)
			return;

		int localPlayerId = m_CoopLobby.GetLocalPlayerId();
		PS_SlotData slot = mgr.FindSlotByRplId(m_iSlotRplId);
		if (!slot)
			return;

		// WHY: if clicking our own slot, deselect. If clicking another, select it.
		if (slot.m_iPlayerId == localPlayerId)
		{
			// Leave current slot.
			lobbyPlayer.AskLeaveSlot();
			Print(string.Format("[PS_Lobby] Requesting leave slot %1", m_iSlotRplId), LogLevel.NORMAL);
		}
		else if (slot.IsAvailable())
		{
			// Take this slot.
			lobbyPlayer.AskTakeSlot(m_iSlotRplId);
			Print(string.Format("[PS_Lobby] Requesting take slot %1 (%2)", m_iSlotRplId, slot.m_sName), LogLevel.NORMAL);
		}
		else
		{
			// Slot is occupied by another player, locked, or destroyed — do nothing.
			Print(string.Format("[PS_Lobby] Slot %1 not available", m_iSlotRplId), LogLevel.NORMAL);
		}
	}

	// =====================================================================
	// HELPERS
	// =====================================================================

	protected PS_RolesGroup GetParentRolesGroup()
	{
		Widget root = GetRootWidget();
		if (!root)
			return null;

		// Walk up the widget tree to find the RolesGroup handler.
		Widget parent = root.GetParent();
		while (parent)
		{
			PS_RolesGroup group = PS_RolesGroup.Cast(parent.FindHandler(PS_RolesGroup));
			if (group)
				return group;

			parent = parent.GetParent();
		}

		return null;
	}

	int GetSlotRplId()
	{
		return m_iSlotRplId;
	}
}
