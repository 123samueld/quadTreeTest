#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include <iostream>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Sleep.hpp>

class Quadtree {
public:
    sf::FloatRect bounds;
    std::vector<std::shared_ptr<Quadtree>> children;
    int depth;

    Quadtree(sf::FloatRect rect, int initialDepth) :
        bounds(rect), depth(initialDepth) {
    }

    sf::FloatRect calculateChildRect(int childIndex) const {
        float halfWidth = bounds.width / 2.0f;
        float halfHeight = bounds.height / 2.0f;

        switch (childIndex) {
        case 0: // Top Left
            return sf::FloatRect(bounds.left, bounds.top, halfWidth, halfHeight);
        case 1: // Top Right
            return sf::FloatRect(bounds.left + halfWidth, bounds.top, halfWidth, halfHeight);
        case 2: // Bottom Left
            return sf::FloatRect(bounds.left, bounds.top + halfHeight, halfWidth, halfHeight);
        case 3: // Bottom Right
            return sf::FloatRect(bounds.left + halfWidth, bounds.top + halfHeight, halfWidth, halfHeight);
        default:
            throw std::invalid_argument("Invalid childIndex"); // Handle invalid index
        }
    }


    void subdivide() {
        if (!children.empty() || depth <= 0) return;

        for (int i = 0; i < 4; ++i) {
            sf::FloatRect childRect = calculateChildRect(i);
            children.push_back(std::make_shared<Quadtree>(childRect, depth - 1));
            children.back()->subdivide(); 
        }
    }

    void draw(sf::RenderWindow& window) const {
        sf::RectangleShape rect(sf::Vector2f(bounds.width, bounds.height));
        rect.setPosition(bounds.left, bounds.top);
        rect.setFillColor(sf::Color::Transparent);
        rect.setOutlineThickness(1.0f);

        sf::Color color;
        if (depth == 0) {
            color = sf::Color::Red;
        }
        else if (depth % 2 == 0) {
            color = sf::Color::Green;
        }
        else {
            color = sf::Color::Blue;
        }
        rect.setOutlineColor(color);
  
        for (const auto& child : children) {
            child->draw(window);
        }     
        window.draw(rect);
    }
};

class Camera {
public:
    sf::Vector2f position;
    float zoomFactor;
    float scrollSpeed;

    Camera(sf::Vector2f initialPosition, float zoomFactor = 1.f, float scrollSpeed = 5.f)
        : position(initialPosition), zoomFactor(zoomFactor), scrollSpeed(scrollSpeed) {}


    void updateEdgeScrolling(const sf::RenderWindow& window) {
        const int EDGE_THRESHOLD = 70;
        sf::Vector2i mousePosition = sf::Mouse::getPosition(window);

        if (mousePosition.x < EDGE_THRESHOLD) {
            position.x -= scrollSpeed;
        }
        else if (mousePosition.x > window.getSize().x - EDGE_THRESHOLD) {
            position.x += scrollSpeed;
        }

        if (mousePosition.y < EDGE_THRESHOLD) {
            position.y -= scrollSpeed;
        }
        else if (mousePosition.y > window.getSize().y - EDGE_THRESHOLD) {
            position.y += scrollSpeed;
        }
    }

    void updateZoom(int scrollDelta) {
        zoomFactor = std::max(0.1f, zoomFactor + scrollDelta * 0.05f);
    }

    sf::Vector2f convertWorldToView(const sf::Vector2f& worldPos) const {
        return (worldPos - position) * zoomFactor;
    }

    sf::Vector2f convertViewToWorld(const sf::Vector2f& viewPos) const {
        return viewPos / zoomFactor + position;
    }

    void applyTransform(sf::RenderWindow& window) const {
        sf::View view = window.getView();
        view.setCenter(position);
        view.setSize(window.getSize().x / zoomFactor * 1.5f, window.getSize().y / zoomFactor * 1.5f);
        window.setView(view);
    }
};

class Unit {
public:
    sf::Vector2f position;
    float radius;
    bool isSelected;

    Unit(sf::Vector2f initialPosition, float radius)
        : position(initialPosition), radius(radius), isSelected(false) {}

    void draw(sf::RenderWindow& window) {
        sf::CircleShape shape(radius);
        shape.setPosition(position);
        shape.setFillColor(isSelected ? sf::Color::Green : sf::Color::Blue);
        window.draw(shape);
    }

    void moveTo(const sf::Vector2f& destination) {
        position = destination;
    }

    bool contains(const sf::Vector2f& point) const {
        float dx = point.x - (position.x + radius);
        float dy = point.y - (position.y + radius);
        return (dx * dx + dy * dy) <= (radius * radius);
    }
};


int main() {
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Quadtree Visualization", sf::Style::Fullscreen);
    Quadtree root(sf::FloatRect(0.0f, 0.0f, static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y)), 3);

    sf::Vector2f windowCenter(window.getSize().x / 2.0f, window.getSize().y / 2.0f);
    Camera camera(windowCenter);

    Unit myUnit(windowCenter, 30.0f);


    root.subdivide();

    sf::Vector2i previousMousePosition = sf::Mouse::getPosition();
    sf::Clock clock;

    while (window.isOpen()) {
        sf::Time elapsed = clock.restart();
        sf::Event event;

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            else if (event.type == sf::Event::MouseWheelScrolled) {
                camera.updateZoom(event.mouseWheelScroll.delta);
            }
            else if (event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2f mousePos = static_cast<sf::Vector2f>(sf::Mouse::getPosition(window));


                if (event.mouseButton.button == sf::Mouse::Left) {
                    // Toggle selection only if the click is within the unit
                    if (myUnit.contains(mousePos)) {
                        myUnit.isSelected = !myUnit.isSelected;
                    }
                    else {
                        // Deselect the unit if the click is outside
                        myUnit.isSelected = false;
                    }
                }
                else if (event.mouseButton.button == sf::Mouse::Right && myUnit.isSelected) {
                    // Move the unit only if it is selected
                    myUnit.moveTo(mousePos);
                }
            }
        }


        sf::Vector2i currentMousePosition = sf::Mouse::getPosition();
        sf::Vector2i mouseDelta = currentMousePosition - previousMousePosition;
        previousMousePosition = currentMousePosition;

        camera.updateEdgeScrolling(window);

        window.clear();
        camera.applyTransform(window);
        root.draw(window);
        myUnit.draw(window);

        window.display();

        sf::Time frameTime = sf::seconds(1.0f / 60.0f) - elapsed;
        if (frameTime > sf::Time::Zero) {
            sf::sleep(frameTime);
        }
    }
    return 0;
}
