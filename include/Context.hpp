#ifndef CONTEXT_HPP
#define CONTEXT_HPP

// Глобальный контекст приложения
class GameStateManager;

struct AppContext {
    static GameStateManager* stateManager;
};

#endif // CONTEXT_HPP
