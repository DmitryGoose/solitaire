#include "Card.hpp"
#include "Pile.hpp"
#include "Context.hpp"
#include "HintSystem.hpp"
#include "SoundManager.hpp"
#include "ScoreSystem.hpp"
#include <iostream>

// Инициализация статических переменных
sf::Texture Card::s_cardTexture;
sf::Texture Card::s_backTexture;
bool Card::s_debugMode = false;
float Card::s_cardScale = 0.351111f; // Уменьшаем карты до 35% от оригинального размера

bool Card::loadTextures(const std::string& cardsPath, const std::string& backPath) {
    if (!s_cardTexture.loadFromFile(cardsPath)) {
        std::cerr << "Failed to load cards texture from: " << cardsPath << std::endl;
        return false;
    }

    if (!s_backTexture.loadFromFile(backPath)) {
        std::cerr << "Failed to load card back texture from: " << backPath << std::endl;
        return false;
    }

    std::cout << "Текстуры карт успешно загружены: " << std::endl;
    std::cout << "- Карты: " << cardsPath << " (" << s_cardTexture.getSize().x << "x" << s_cardTexture.getSize().y << ")" << std::endl;
    std::cout << "- Рубашка: " << backPath << " (" << s_backTexture.getSize().x << "x" << s_backTexture.getSize().y << ")" << std::endl;
    std::cout << "- Размер одной карты: " << CARD_WIDTH << "x" << CARD_HEIGHT << std::endl;
    std::cout << "- Масштаб карт: " << s_cardScale << " (эффективный размер: "
              << (CARD_WIDTH * s_cardScale) << "x" << (CARD_HEIGHT * s_cardScale) << ")" << std::endl;

    return true;
}

void Card::unloadTextures() {
    // SFML автоматически освобождает ресурсы при уничтожении текстур
}

void Card::setDebugMode(bool debug) {
    s_debugMode = debug;
}

bool Card::isDebugMode() {
    return s_debugMode;
}

void Card::setCardScale(float scale) {
    s_cardScale = scale;
    // Примечание: уже созданные карты не будут автоматически изменять масштаб,
    // только вновь создаваемые карты будут использовать новый масштаб
}

float Card::getCardScale() {
    return s_cardScale;
}

Card::Card(Suit suit, Rank rank)
    : m_suit(suit)
    , m_rank(rank)
    , m_faceUp(false)
    , m_dragging(false)
    , m_pile(nullptr)
{
    // Настройка спрайта лицевой стороны
    m_frontSprite.setTexture(s_cardTexture);
    m_frontSprite.setTextureRect(getCardTextureRect());

    // Получаем фактические размеры текстурного прямоугольника для лицевой стороны
    sf::IntRect frontRect = getCardTextureRect();
    m_frontSprite.setOrigin(frontRect.width / 2.0f, frontRect.height / 2.0f);

    // Настройка спрайта рубашки
    m_backSprite.setTexture(s_backTexture);

    // Создаем текстурный прямоугольник для всей рубашки
    sf::IntRect backRect(0, 0, s_backTexture.getSize().x, s_backTexture.getSize().y);
    m_backSprite.setTextureRect(backRect);

    // Устанавливаем точку привязки в центр текстуры рубашки
    m_backSprite.setOrigin(backRect.width / 2.0f, backRect.height / 2.0f);

    // Масштабируем рубашку, чтобы она соответствовала размерам лицевой стороны
    float scaleX = static_cast<float>(CARD_WIDTH) / backRect.width;
    float scaleY = static_cast<float>(CARD_HEIGHT) / backRect.height;
    m_backSprite.setScale(scaleX, scaleY);

    // Применяем глобальный масштаб ко всей карте
    this->setScale(s_cardScale, s_cardScale);

    if (s_debugMode) {
        std::cout << "Создана карта: " << static_cast<int>(rank)
                  << " масти " << static_cast<int>(suit) << std::endl;
        std::cout << "Текстурный прямоугольник лицевой стороны: "
                  << frontRect.left << "," << frontRect.top << ","
                  << frontRect.width << "," << frontRect.height << std::endl;
        std::cout << "Текстурный прямоугольник рубашки: "
                  << backRect.left << "," << backRect.top << ","
                  << backRect.width << "," << backRect.height << std::endl;
        std::cout << "Масштаб рубашки: " << scaleX << "x" << scaleY << std::endl;
        std::cout << "Эффективный размер карты: "
                  << (CARD_WIDTH * s_cardScale) << "x" << (CARD_HEIGHT * s_cardScale) << std::endl;
    }
}

sf::IntRect Card::getCardTextureRect() const {
    int rankIndex = static_cast<int>(m_rank) - 1; // Ранг - 1 (от 0 до 12)
    int suitIndex = static_cast<int>(m_suit);     // От 0 до 3

    return sf::IntRect(
        rankIndex * CARD_WIDTH,  // X позиция в спрайтовом листе
        suitIndex * CARD_HEIGHT, // Y позиция в спрайтовом листе
        CARD_WIDTH,              // Ширина одной карты
        CARD_HEIGHT              // Высота одной карты
    );
}

Suit Card::getSuit() const {
    return m_suit;
}

Rank Card::getRank() const {
    return m_rank;
}

bool Card::isFaceUp() const {
    return m_faceUp;
}

bool Card::isRed() const {
    // ИСПРАВЛЕНО: Масти 1 (Буби) и 2 (Черви) - красные
    int suitValue = static_cast<int>(m_suit);
    return suitValue == 1 || suitValue == 2; // Hearts и Diamonds
}

void Card::flip() {
    // Сохраняем текущую позицию карты перед переворотом
    sf::Vector2f currentPosition = getPosition();

    // Меняем флаг отображения
    m_faceUp = !m_faceUp;

    // Восстанавливаем позицию
    setPosition(currentPosition);

    // Не меняем поворот и масштаб, так как они должны сохраняться
}

sf::FloatRect Card::getBounds() const {
    // Используем полный размер карты с учетом масштаба
    float scaledWidth = CARD_WIDTH * s_cardScale;
    float scaledHeight = CARD_HEIGHT * s_cardScale;

    // Центр карты и фактическая позиция
    sf::Vector2f position = getPosition();

    // Создаем прямоугольник реальных границ карты
    sf::FloatRect bounds;
    bounds.left = position.x - scaledWidth / 2.0f;
    bounds.top = position.y - scaledHeight / 2.0f;
    bounds.width = scaledWidth;
    bounds.height = scaledHeight;

    return bounds;
}

bool Card::contains(const sf::Vector2f& point) const {
    sf::FloatRect bounds = getBounds();
    bool result = bounds.contains(point);

    if (s_debugMode) {
        std::cout << "Клик: (" << point.x << "," << point.y << ")" << std::endl;
        std::cout << "Границы карты: (" << bounds.left << "," << bounds.top << ","
                  << bounds.width << "," << bounds.height << ")" << std::endl;
        std::cout << "Попадание: " << (result ? "да" : "нет") << std::endl;
    }

    return result;
}

void Card::setDragging(bool dragging) {
    m_dragging = dragging;
}

bool Card::isDragging() const {
    return m_dragging;
}

void Card::setPile(Pile* pile) {
    m_pile = pile;
}

Pile* Card::getPile() const {
    return m_pile;
}

bool Card::isVisible() const {
    return m_faceUp && m_pile != nullptr;
}

bool Card::canBeMoved() const {
    if (!isVisible()) {
        return false;
    }

    // Карта может быть перемещена, если она видима и
    // либо это верхняя карта в стопке, либо стопка позволяет перемещать группы карт
    return m_pile->isTopCard(this) || m_pile->canMoveMultipleCards();
}

bool Card::autoMove(Context& context) {
    if (!isVisible() || !canBeMoved()) {
        return false;
    }

    // Используем систему подсказок для поиска возможных ходов
    HintSystem& hintSystem = context.getHintSystem();
    std::vector<Pile*> possibleMoves = hintSystem.findPossibleMoves(this);

    // Проверяем возможные перемещения в приоритетном порядке
    // Сначала пробуем переместить в фундаментные стопки
    for (auto pile : possibleMoves) {
        if (pile->getPileType() == PileType::FOUNDATION && pile->canAcceptCard(this)) {
            return moveCardTo(pile, context);
        }
    }

    // Затем пробуем найти перемещения в игровые стопки
    for (auto pile : possibleMoves) {
        if (pile->getPileType() == PileType::TABLEAU && pile->canAcceptCard(this)) {
            return moveCardTo(pile, context);
        }
    }

    return false;
}

bool Card::moveCardTo(Pile* targetPile, Context& context) {
    if (!targetPile || !m_pile) {
        return false;
    }

    // Готовим список карт для перемещения (текущая и все карты выше неё)
    std::vector<Card*> cardsToMove;
    m_pile->getCardsAbove(this, cardsToMove);

    // Запоминаем текущую стопку карты
    Pile* sourcePile = m_pile;

    // Выполняем перемещение
    if (targetPile->addCards(cardsToMove)) {
        // Удаляем карты из исходной стопки (это уже вызывает updateAfterCardRemoval внутри)
        sourcePile->removeCards(cardsToMove);

        // Воспроизводим звук перемещения карты
        context.getSoundManager().playSound(SoundEffect::CARD_PLACE);

        // Обновляем очки
        context.getScoreSystem().addMovePoints(1);

        // Не нужно повторно вызывать updateAfterCardRemoval, так как он уже вызван в removeCards

        return true;
    }

    return false;
}

void Card::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();

    if (m_faceUp) {
        target.draw(m_frontSprite, states);
    } else {
        target.draw(m_backSprite, states);
    }

    // Отладочная рамка при включенном режиме отладки
    if (s_debugMode) {
        sf::RectangleShape border(sf::Vector2f(CARD_WIDTH, CARD_HEIGHT));
        border.setOrigin(CARD_WIDTH / 2.0f, CARD_HEIGHT / 2.0f);
        border.setFillColor(sf::Color::Transparent);
        border.setOutlineColor(sf::Color::Yellow);
        border.setOutlineThickness(1);
        target.draw(border, states);

        // Дополнительно, отрисовываем центральную точку (origin)
        sf::CircleShape originPoint(2);
        originPoint.setOrigin(2, 2);
        originPoint.setFillColor(sf::Color::Red);
        target.draw(originPoint, states);
    }
}
