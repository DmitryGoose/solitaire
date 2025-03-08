#ifndef CARD_HPP
#define CARD_HPP

#include <SFML/Graphics.hpp>
#include <string>

enum class Suit {
    HEARTS,
    DIAMONDS,
    CLUBS,
    SPADES
};

enum class Rank {
    ACE = 1,
    TWO,
    THREE,
    FOUR,
    FIVE,
    SIX,
    SEVEN,
    EIGHT,
    NINE,
    TEN,
    JACK,
    QUEEN,
    KING
};

class Card : public sf::Drawable, public sf::Transformable {
public:
    Card(Suit suit, Rank rank);

    Suit getSuit() const;
    Rank getRank() const;
    bool isFaceUp() const;
    bool isRed() const;

    void flip();
    sf::FloatRect getBounds() const;
    bool contains(const sf::Vector2f& point) const;

    void setDragging(bool dragging);
    bool isDragging() const;

    // Статические члены для работы с текстурой карт
    static bool loadTextures(const std::string& cardsPath, const std::string& backPath);
    static void unloadTextures();

    // Включение/выключение отладочного режима
    static void setDebugMode(bool debug);
    static bool isDebugMode();

    // Управление масштабом карт
    static void setCardScale(float scale);
    static float getCardScale();

private:
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    // Получение прямоугольника текстуры для конкретной карты
    sf::IntRect getCardTextureRect() const;

    Suit m_suit;
    Rank m_rank;
    bool m_faceUp;
    bool m_dragging;

    sf::Sprite m_frontSprite;
    sf::Sprite m_backSprite;

    // Статические текстуры, общие для всех карт
    static sf::Texture s_cardTexture;
    static sf::Texture s_backTexture;

    // Отладочный режим
    static bool s_debugMode;

    // Глобальный масштаб для всех карт
    static float s_cardScale;

    // Размеры одной карты в пикселях
    static const int CARD_WIDTH = 225;  // Размер карты в текстуре
    static const int CARD_HEIGHT = 310; // Размер карты в текстуре
};

#endif // CARD_HPP
