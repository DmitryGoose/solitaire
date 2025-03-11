#include "Context.hpp"
#include "HintSystem.hpp"
#include "SoundManager.hpp"
#include "ScoreSystem.hpp"
#include "GameState.hpp"
#include "Game.hpp"

// Инициализация статических переменных
GameStateManager* AppContext::stateManager = nullptr;
Context* AppContext::gameContext = nullptr;

Context::Context(Game& game)
    : m_stateManager(nullptr),
      m_game(game)
{
    // Создаем глобальную ссылку на контекст
    AppContext::gameContext = this;
}

Context::~Context() {
    // Обнуляем глобальную ссылку, если этот контекст был глобальным
    if (AppContext::gameContext == this) {
        AppContext::gameContext = nullptr;
    }
}

void Context::initialize() {
    // Создаем HintSystem, используя Game из контекста
    m_hintSystem = std::make_unique<HintSystem>(m_game);

    // HintSystem должна знать о контексте (для функции автоматического перемещения)
    m_hintSystem->setContext(this);

    // Создаем ScoreSystem
    m_scoreSystem = std::make_unique<ScoreSystem>();

    // Устанавливаем ссылку на менеджер состояний из существующего AppContext
    m_stateManager = AppContext::stateManager;
}

SoundManager& Context::getSoundManager() {
    // Получаем экземпляр SoundManager через статический метод getInstance
    return SoundManager::getInstance();
}

void Context::clearPiles() {
    m_tableauPiles.clear();
    m_foundationPiles.clear();
}
