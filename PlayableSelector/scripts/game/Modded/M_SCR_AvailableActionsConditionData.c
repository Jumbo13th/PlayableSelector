// Suppresses the per-frame vanilla warning
//   "[SCR_AvailableActionsConditionData] - can't fetch health data!"
// In this mod the player controls the lobby/limbo "InitialPlayer" entity, which is a
// ChimeraCharacter with a CharacterController but legitimately has NO
// SCR_CharacterDamageManagerComponent. Vanilla FetchHealthData prints a WARNING and returns
// every frame in that case, spamming the log. We early-return silently when there is no
// character damage manager; when a real one exists, behavior is unchanged (delegates to vanilla).
modded class SCR_AvailableActionsConditionData
{
	override protected void FetchHealthData(float timeSlice)
	{
		// No character damage manager => nothing to fetch (limbo entity). Skip without the warning.
		if (!m_Character || !SCR_CharacterDamageManagerComponent.Cast(m_Character.GetDamageManager()))
			return;

		super.FetchHealthData(timeSlice);
	}
}
