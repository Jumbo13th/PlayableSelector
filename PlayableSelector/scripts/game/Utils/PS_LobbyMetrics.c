// PS_LobbyMetrics — Server-side observability for lobby health.
// Observes PS_LobbyManager events and tracks aggregated metrics.
// WHY separate class: manager doesn't know about metrics — metrics knows about manager.
// This keeps telemetry from polluting business logic.
//
// Usage: attach to GameMode entity. Logs metrics periodically.
// Operators can monitor lobby health without guessing.

class PS_LobbyMetricsClass : ScriptComponentClass
{
}

class PS_LobbyMetrics : ScriptComponent
{
	// --- Counters ---
	protected int m_iSlotRegistrations;
	protected int m_iSlotTakes;
	protected int m_iSlotLeaves;
	protected int m_iPlayerConnections;
	protected int m_iPlayerDisconnections;
	protected int m_iRpcCount;			// total RPCs observed this interval

	// --- Timing ---
	protected float m_fLastReportTime;
	protected static const float REPORT_INTERVAL_S = 30.0;	// log metrics every 30s

	// =====================================================================
	// LIFECYCLE
	// =====================================================================

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// WHY: metrics only matter on the server. Clients don't need this overhead.
		if (!Replication.IsServer())
			return;

		// WHY CallLater: manager may not exist yet at OnPostInit time.
		GetGame().GetCallqueue().CallLater(SubscribeToManager, 100, false);

		// Periodic reporting.
		GetGame().GetCallqueue().CallLater(ReportMetrics, REPORT_INTERVAL_S * 1000, true);

		m_fLastReportTime = System.GetTickCount();
	}

	override void OnDelete(IEntity owner)
	{
		super.OnDelete(owner);
		GetGame().GetCallqueue().Remove(ReportMetrics);
	}

	// =====================================================================
	// SUBSCRIPTION — listen to manager events
	// =====================================================================

	protected void SubscribeToManager()
	{
		PS_LobbyManager mgr = PS_LobbyManager.GetInstance();
		if (!mgr)
		{
			Print("[PS_Metrics] Manager not found, retrying...", LogLevel.WARNING);
			GetGame().GetCallqueue().CallLater(SubscribeToManager, 500, false);
			return;
		}

		mgr.GetOnSlotRegistered().Insert(OnSlotRegistered);
		mgr.GetOnPlayerAssigned().Insert(OnPlayerAssigned);
		mgr.GetOnPlayerUnassigned().Insert(OnPlayerUnassigned);

		Print("[PS_Metrics] Subscribed to LobbyManager events", LogLevel.NORMAL);
	}

	// =====================================================================
	// EVENT HANDLERS — increment counters
	// =====================================================================

	protected void OnSlotRegistered()
	{
		m_iSlotRegistrations++;
		m_iRpcCount++;
	}

	protected void OnPlayerAssigned()
	{
		m_iSlotTakes++;
		m_iRpcCount++;
	}

	protected void OnPlayerUnassigned()
	{
		m_iSlotLeaves++;
		m_iRpcCount++;
	}

	// =====================================================================
	// REPORTING
	// =====================================================================

	protected void ReportMetrics()
	{
		float now = System.GetTickCount();
		float elapsed = (now - m_fLastReportTime) / 1000.0; // seconds
		if (elapsed <= 0)
			elapsed = REPORT_INTERVAL_S;

		float rpcPerSecond = m_iRpcCount / elapsed;

		PS_LobbyManager mgr = PS_LobbyManager.GetInstance();
		int slotCount = 0;
		int occupiedSlots = 0;
		int vehicleCount = 0;

		if (mgr)
		{
			array<ref PS_SlotData> slots = mgr.GetSlots();
			slotCount = slots.Count();
			foreach (PS_SlotData slot : slots)
			{
				if (slot.m_iPlayerId >= 0)
					occupiedSlots++;
			}
			vehicleCount = mgr.GetVehicles().Count();
		}

		Print(string.Format("[PS_Metrics] slots=%1 occupied=%2 vehicles=%3 | registrations=%4 takes=%5 leaves=%6 | rpc/s=%.1f",
			slotCount, occupiedSlots, vehicleCount,
			m_iSlotRegistrations, m_iSlotTakes, m_iSlotLeaves,
			rpcPerSecond), LogLevel.NORMAL);

		// Reset interval counters.
		m_iRpcCount = 0;
		m_fLastReportTime = now;
	}
}
