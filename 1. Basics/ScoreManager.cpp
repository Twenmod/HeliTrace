#include "precomp.h"
#include "ScoreManager.h"

ScoreManager* ScoreManager::m_scoreManager = nullptr;

ScoreManager* ScoreManager::GetScoreManager()
{
	if (m_scoreManager == nullptr) m_scoreManager = new ScoreManager();
	return m_scoreManager;
}

void ScoreManager::ResetScore()
{
	m_enemiesKilled = 0;
	m_score = 0;
	m_totalEnemies = 0;
}
