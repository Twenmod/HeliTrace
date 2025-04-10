#pragma once
class ScoreManager
{
public:
	void AddScore(const float _score) { m_score += _score; };
	[[nodiscard]] int GetScore() const { return static_cast<int>(m_score); }
	void SetScore(const float _score) { m_score = _score; }

	static ScoreManager* GetScoreManager();
	void ResetScore();
	void MarkEnemyKilled() { ++m_enemiesKilled; }
	void AddTotalEnemies() { ++m_totalEnemies; }
	int GetEnemiesKilled() const { return m_enemiesKilled; }
	int GetEnemiesTotal() const { return m_totalEnemies; }
private:
	ScoreManager() = default;
	static ScoreManager* m_scoreManager;
	float m_score{0.f};
	int m_enemiesKilled{0};
	int m_totalEnemies{0};
};
