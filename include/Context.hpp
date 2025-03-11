#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include <vector>
#include <memory>

// Предварительные объявления классов
class GameStateManager;
class HintSystem;
class SoundManager;
class ScoreSystem;
class Pile;
class Game;

// Глобальный контекст игры
class Context {
public:
    Context(Game& game);
    ~Context();

    // Инициализация и настройка контекста
    void initialize();

    // Доступ к основным компонентам
    GameStateManager* getStateManager() const { return m_stateManager; }
    HintSystem& getHintSystem() { return *m_hintSystem; }
    SoundManager& getSoundManager();
    ScoreSystem& getScoreSystem() { return *m_scoreSystem; }
    Game& getGame() { return m_game; }

    // Доступ к стопкам карт
    std::vector<Pile*>& getTableauPiles() { return m_tableauPiles; }
    std::vector<Pile*>& getFoundationPiles() { return m_foundationPiles; }

    // Установка менеджера состояний
    void setStateManager(GameStateManager* stateManager) { m_stateManager = stateManager; }

    // Регистрация стопок карт
    void registerTableauPile(Pile* pile) { m_tableauPiles.push_back(pile); }
    void registerFoundationPile(Pile* pile) { m_foundationPiles.push_back(pile); }

    // Очистка списков стопок
    void clearPiles();

private:
    GameStateManager* m_stateManager;
    Game& m_game;

    // Компоненты игровой логики
    std::unique_ptr<HintSystem> m_hintSystem;
    std::unique_ptr<ScoreSystem> m_scoreSystem;

    // Ссылки на существующие стопки карт
    std::vector<Pile*> m_tableauPiles;
    std::vector<Pile*> m_foundationPiles;
};

// Сохраняем компатибильность с существующим кодом через статический класс
struct AppContext {
    static GameStateManager* stateManager;

    // Добавляем глобальный доступ к контексту игры (синглтон)
    static Context* gameContext;
};

#endif // CONTEXT_HPP
