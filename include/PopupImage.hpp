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
    // Добавляем параметр scale для управления размером
    void showInvalidMove(float scale = 1.5f);

    // Обновление (для контроля времени отображения)
    void update(float deltaTime);

    // Проверка видимости
    bool isVisible() const;

private:
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    sf::Texture m_victoryTexture;
    sf::Texture m_invalidMoveTexture;
    sf::Sprite m_currentSprite;

    bool m_isVisible;
    float m_displayTimer;
    float m_displayDuration;
    float m_scale; // Масштаб текущего изображения
};
