class PS_PolyZoneObjectiveTriggerCaptureClass : PS_PolyZoneObjectiveTriggerClass
{

}

class PS_PolyZoneObjectiveTriggerCapture : PS_PolyZoneObjectiveTrigger
{
	[Attribute("30")]
	float m_fCaptureTime;
	[Attribute("2")]
	int m_iPowerBalance;
	[Attribute(""), RplProp()]
	FactionKey m_sCurrentFaction;
	[Attribute("0")]
	bool m_bCanRetake;
	[Attribute("0")]
	bool m_bEasyTake;
	[Attribute("0")]
	bool m_bTimeLoss;
	
	ref map<FactionKey, int> m_mFactionCounters = new map<FactionKey, int>();
	ref map<FactionKey, float> m_mFactionTimers = new map<FactionKey, float>();

	// Throttle timer sync to avoid flooding the network
	protected float m_fTimerSyncAccumulator = 0;
	protected static const float TIMER_SYNC_INTERVAL = 0.5; // Sync every 0.5 seconds

	override void OnInit(IEntity owner)
	{
		super.OnInit(owner);
		if (Replication.IsServer())
		{
			SetEventMask(EntityEvent.FRAME);
			SetEventMask(EntityEvent.POSTFRAME);
		}
	}
	
	override void LinkObjectives()
	{
		super.LinkObjectives();
		UpdateObjectives();
	}
	
	override void OnOnlyOneFactionAlive(FactionKey aliveFaction)
	{
		m_sCurrentFaction = aliveFaction;
		
		foreach (PS_Objective objective : m_aObjectives)
		{
			FactionKey factionKey = objective.GetFactionKey();
			objective.SetCompleted(factionKey == m_sCurrentFaction);
		}
	}
	
	void UpdateObjectives()
	{
		if (m_bAfterGame)
			return;
		
		foreach (PS_Objective objective : m_aObjectives)
		{
			FactionKey factionKey = objective.GetFactionKey();
			objective.SetCompleted(factionKey == m_sCurrentFaction);
		}
	}
	
	override void OnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer())
			return;

		// Sync timers to clients at a throttled rate instead of every frame
		m_fTimerSyncAccumulator += timeSlice;
		if (m_fTimerSyncAccumulator >= TIMER_SYNC_INTERVAL)
		{
			m_fTimerSyncAccumulator = 0;
			SyncFactionTimers();
		}

		int maxDiff = 0;
		int maxCount = 0;
		FactionKey maxFaction = "";
		foreach (FactionKey factionKey, int count : m_mFactionCounters)
		{
			if (m_mFactionCounters[factionKey] > maxCount)
			{
				maxDiff = m_mFactionCounters[factionKey] - maxCount;
				maxCount = m_mFactionCounters[factionKey];
				maxFaction = factionKey;
			}
		}
		
		foreach (FactionKey factionKey, float timer : m_mFactionTimers)
		{
			if (factionKey != maxFaction)
				m_mFactionTimers[factionKey] = timer - timeSlice;
			if (m_mFactionTimers[factionKey] < 0)
				m_mFactionTimers[factionKey] = 0;
		}
		
		if (maxFaction == "")
			return;
		if (maxFaction == m_sCurrentFaction)
			return;
		
		if (maxDiff > m_iPowerBalance || (m_bEasyTake && maxDiff == maxCount && maxDiff > 0))
		{
			m_mFactionTimers[maxFaction] = m_mFactionTimers[maxFaction] + timeSlice;
			if (m_mFactionTimers[maxFaction] > m_fCaptureTime)
			{
				m_sCurrentFaction = maxFaction;
				Replication.BumpMe();
				foreach (FactionKey factionKey, float timer : m_mFactionTimers)
				{
					m_mFactionTimers[factionKey] = 0;
				}
				UpdateObjectives();
				if (!m_bCanRetake)
				{
					ClearEventMask(EntityEvent.FRAME);
				}
			}
		}
	}
	
	// Batch-sync all faction timers in a single RPC at a throttled rate
	void SyncFactionTimers()
	{
		array<FactionKey> keys = {};
		array<float> values = {};
		foreach (FactionKey factionKey, float timer : m_mFactionTimers)
		{
			keys.Insert(factionKey);
			values.Insert(timer);
		}
		Rpc(RPC_SyncFactionTimers, keys, values);
		if (RplSession.Mode() != RplMode.Dedicated)
			RPC_SyncFactionTimers(keys, values);
	}
	[RplRpc(RplChannel.Unreliable, RplRcver.Broadcast)]
	void RPC_SyncFactionTimers(array<FactionKey> keys, array<float> values)
	{
		for (int i = 0; i < keys.Count(); i++)
		{
			m_mFactionTimers[keys[i]] = values[i];
		}
	}
	
	override bool ScriptedEntityFilterForQuery(IEntity ent)
	{
		if (!super.ScriptedEntityFilterForQuery(ent))
			return false;
		
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(ent);
		SCR_DamageManagerComponent damageManagerComponent = character.GetDamageManager();
		
		return damageManagerComponent.GetState() != EDamageState.DESTROYED;
	}
	
	bool IsCurrentFaction(FactionKey factionKey)
	{
		return m_sCurrentFaction == factionKey;
	}
	
	float GetFactionTime(FactionKey factionKey)
	{
		if (!m_mFactionTimers.Contains(factionKey))
			return m_fCaptureTime;
		return m_fCaptureTime - m_mFactionTimers[factionKey];
	}
	
	override protected void OnActivate(IEntity ent)
	{
		super.OnActivate(ent);
		Count(ent, 1);
	}
	
	override protected void OnDeactivate(IEntity ent)
	{
		super.OnDeactivate(ent);
		Count(ent, -1);
	}
	
	void Count(IEntity ent, int side)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(ent);
		PS_PlayableComponent playableComponent = character.PS_GetPlayable();
		FactionAffiliationComponent factionAffiliationComponent = playableComponent.GetFactionAffiliationComponent();
		Faction faction = factionAffiliationComponent.GetDefaultAffiliatedFaction();
		FactionKey factionKey = faction.GetFactionKey();
		if (!m_mFactionCounters.Contains(factionKey))
		{
			m_mFactionCounters[factionKey] = 0;
			m_mFactionTimers[factionKey] = 0;
		}
		int counter = m_mFactionCounters[factionKey];
		counter += side;
		m_mFactionCounters[factionKey] = counter;
	}
}
