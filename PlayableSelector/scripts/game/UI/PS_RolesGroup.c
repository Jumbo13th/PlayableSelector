// PS_RolesGroup — Squad/group container in the lobby.
// Layout: {B45A0FA6883A7A0E}UI/Lobby/RolesGroup.layout
// Handler class referenced by the layout — name must match exactly.
//
// Groups character slots by their AI group (squad).
// Displays group name, member count, and contains PS_CharacterSelector children.

class PS_RolesGroup : SCR_ScriptedWidgetComponent
{
	// Layout prefab for individual slot widgets.
	protected ResourceName m_sCharacterSelectorPrefab = "{3F761F63F1DF29D1}UI/Lobby/CharacterSelector.layout";

	// --- Widgets ---
	protected TextWidget m_wRolesGroupName;
	protected RichTextWidget m_wRolesGroupNameCustom;
	protected ImageWidget m_wGroupFactionColor;
	protected VerticalLayoutWidget m_wCharactersList;
	protected VerticalLayoutWidget m_wList;
	protected ButtonWidget m_wLockButton;
	protected ButtonWidget m_wVoiceJoinButton;
	protected ButtonWidget m_wRolesGroupButton;

	// --- State ---
	protected string m_sFactionKey;
	protected int m_iGroupId;
	protected PS_CoopLobby m_CoopLobby;
	protected ref array<PS_CharacterSelector> m_aCharacterSelectors = {};
	protected bool m_bFolded;
	protected int m_iSlotsCount;
	protected int m_iPlayersCount;
	protected int m_iLockedCount;

	// =====================================================================
	// INIT
	// =====================================================================

	void Init(string factionKey, int groupId, string groupName, PS_CoopLobby lobby)
	{
		m_sFactionKey = factionKey;
		m_iGroupId = groupId;
		m_CoopLobby = lobby;

		Widget root = GetRootWidget();
		if (!root)
			return;

		m_wRolesGroupName = TextWidget.Cast(root.FindAnyWidget("RolesGroupName"));
		m_wRolesGroupNameCustom = RichTextWidget.Cast(root.FindAnyWidget("RolesGroupNameCustom"));
		m_wGroupFactionColor = ImageWidget.Cast(root.FindAnyWidget("GroupFactionColor"));
		m_wCharactersList = VerticalLayoutWidget.Cast(root.FindAnyWidget("CharactersList"));
		m_wList = VerticalLayoutWidget.Cast(root.FindAnyWidget("List"));
		m_wLockButton = ButtonWidget.Cast(root.FindAnyWidget("LockButton"));
		m_wVoiceJoinButton = ButtonWidget.Cast(root.FindAnyWidget("VoiceJoinButton"));
		m_wRolesGroupButton = ButtonWidget.Cast(root.FindAnyWidget("RolesGroupButton"));

		// Set faction color.
		if (m_wGroupFactionColor)
		{
			SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			if (factionManager)
			{
				SCR_Faction faction = SCR_Faction.Cast(factionManager.GetFactionByKey(factionKey));
				if (faction)
					m_wGroupFactionColor.SetColor(faction.GetFactionColor());
			}
		}

		// Set group name — use callsign resolved at registration time.
		// Fallback to "Group" if none was set (solo/non-callsign scenarios).
		if (m_wRolesGroupName)
		{
			if (groupName != "")
				m_wRolesGroupName.SetText(groupName);
			else
				m_wRolesGroupName.SetText("Group");
		}

		// Wire fold button.
		if (m_wRolesGroupButton)
		{
			SCR_ButtonBaseComponent btnComp = SCR_ButtonBaseComponent.Cast(
				m_wRolesGroupButton.FindHandler(SCR_ButtonBaseComponent));
			if (btnComp)
				btnComp.m_OnClicked.Insert(OnGroupClicked);
		}
	}

	// =====================================================================
	// SLOT MANAGEMENT
	// =====================================================================

	void AddSlot(PS_SlotData slot)
	{
		if (!m_wCharactersList)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		Widget charWidget = workspace.CreateWidgets(m_sCharacterSelectorPrefab, m_wCharactersList);
		if (!charWidget)
			return;

		PS_CharacterSelector charSelector = PS_CharacterSelector.Cast(
			charWidget.FindHandler(PS_CharacterSelector));
		if (!charSelector)
			return;

		charSelector.Init(slot, m_CoopLobby);
		m_aCharacterSelectors.Insert(charSelector);

		m_iSlotsCount++;
		if (slot.m_iPlayerId >= 0)
			m_iPlayersCount++;
		if (slot.m_bLocked)
			m_iLockedCount++;

		UpdateCustomName();
	}

	// =====================================================================
	// UPDATE
	// =====================================================================

	void UpdateCustomName()
	{
		if (!m_wRolesGroupNameCustom)
			return;

		int available = m_iSlotsCount - m_iLockedCount;
		if (m_iLockedCount >= m_iSlotsCount)
			m_wRolesGroupNameCustom.SetText("(Locked)");
		else
			m_wRolesGroupNameCustom.SetTextFormat("(%1/%2)", m_iPlayersCount, available);
	}

	void OnSlotPlayerChanged(int oldPlayerId, int newPlayerId)
	{
		// WHY: called by CharacterSelector when a player takes/leaves a slot.
		if (oldPlayerId >= 0)
			m_iPlayersCount--;
		if (newPlayerId >= 0)
			m_iPlayersCount++;

		UpdateCustomName();
	}

	// =====================================================================
	// INTERACTION
	// =====================================================================

	protected void OnGroupClicked()
	{
		m_bFolded = !m_bFolded;
		if (m_wList)
			m_wList.SetVisible(!m_bFolded);
	}

	// =====================================================================
	// GETTERS
	// =====================================================================

	string GetFactionKey()
	{
		return m_sFactionKey;
	}

	int GetGroupId()
	{
		return m_iGroupId;
	}

	// WHY: GetRootWidget() is inherited from SCR_ScriptedWidgetComponent — no override needed.
	// m_wRoot is set automatically by HandlerAttached() in the parent class.
}
