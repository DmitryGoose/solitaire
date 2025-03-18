#pragma once

#include <SFML/Graphics.hpp>
#include <string>

class PopupImage : public sf::Drawable {
public:
    PopupImage();

    // Загрузка изображений
    bool loadTextures(const std::string& victoryImagePath, const std::string& invalidMoveImagePath);

    // Показать всплывающее изображение победы
    void showVictory();

    // Показать всплывающее изображение неправильного хода
    void showInvalidMove(float scale = 1.5f);

    // Обновление (для контроля времени отображения)
    void update(float deltaTime);

    // Проверка видимости
    bool isVisible() const;

    // Скрытие изображения
    void hide();

    // Установка позиции изображения
    void setPosition(float x, float y);

private:
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    sf::Texture m_victoryTexture;
    sf::Texture m_invalidMoveTexture;
    sf::Sprite m_currentSprite;

    bool m_isVisible;
    float m_displayTimer;
    float m_displayDuration;
    float m_scale; // Масштаб текущего изображения
    sf::Vector2f m_position; // Пользовательская позиция (если задана)
    bool m_hasCustomPosition;
};
