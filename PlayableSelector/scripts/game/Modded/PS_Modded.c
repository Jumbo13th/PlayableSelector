// PS_Modded — Modded vanilla classes needed by the lobby.
// WHY: some vanilla classes have private fields we need to modify.
// modded class has access to private members of the original class.

// WHY: SCR_BaseGameMode.m_eGameState is private with no public setter.
// Vanilla only exposes StartGameMode() (→GAME) and EndGameMode() (→POSTGAME).
// We need custom states (SLOTSELECTION, BRIEFING, DEBRIEFING), so we add a setter.
// This is the same approach the old PlayableSelector used.
modded class SCR_BaseGameMode
{
	void SetGameModeState(SCR_EGameModeState state)
	{
		if (!IsMaster())
			return;

		m_eGameState = state;
		Replication.BumpMe();

		OnGameStateChanged();
	}
}

// WHY: SCR_ManualCamera disables camera when menus are open.
// The spectator menu needs camera input to work, so we whitelist it.
modded class SCR_ManualCamera
{
	override protected bool IsDisabledByMenu()
	{
		if (!m_MenuManager)
			return false;

		if (m_MenuManager.IsAnyDialogOpen())
			return true;

		MenuBase topMenu = m_MenuManager.GetTopMenu();

		// WHY: allow camera input when lobby or spectator menu is on top.
		return topMenu && !topMenu.IsInherited(EditorMenuUI);
	}
}
