// BackgroundSelector.hpp
#ifndef BACKGROUND_SELECTOR_HPP
#define BACKGROUND_SELECTOR_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <functional>

class BackgroundSelector : public sf::Drawable {
public:
    BackgroundSelector();

    void init(sf::Vector2f position, sf::Font& font);
    void handleEvent(const sf::Event& event, const sf::Vector2f& mousePos);
    void update();

    // Установка функции обратного вызова при изменении фона
    void setChangeCallback(std::function<void(const std::string&)> callback);

    // Переопределение метода отрисовки
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    // Показать/скрыть селектор
    void show(bool visible = true);
    bool isVisible() const;

private:
    void updatePreview();

    bool m_isVisible;
    bool m_isActive;

    sf::RectangleShape m_backgroundRect;    // Фон для селектора
    sf::Text m_titleText;                  // Заголовок
    sf::Text m_currentBgText;              // Текущий фон

    sf::RectangleShape m_prevButton;       // Кнопка "Предыдущий"
    sf::RectangleShape m_nextButton;       // Кнопка "Следующий"
    sf::Text m_prevButtonText;             // Текст кнопки "Предыдущий"
    sf::Text m_nextButtonText;             // Текст кнопки "Следующий"

    sf::RectangleShape m_previewRect;      // Рамка для предпросмотра
    sf::Sprite m_previewSprite;            // Спрайт предпросмотра

    std::vector<std::string> m_backgrounds; // Список доступных фонов
    size_t m_currentIndex;                 // Индекс текущего фона

    std::function<void(const std::string&)> m_changeCallback; // Функция обратного вызова
};

#endif // BACKGROUND_SELECTOR_HPP
