#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <vector>
#include <iostream>
#include <Windows.h>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

class Map : public sf::Drawable {
public:

  struct Cell {
    bool now = false;
    bool next = false;
  };

  Map(uintptr_t w, uintptr_t h) {
    if(w > (UINT32_MAX / 2 - 1) || h > (UINT32_MAX / 2 - 1)) {
      throw std::out_of_range("Map size too big");
    }

    size_.x = w;
    size_.y = h;

    vtBuff_.setPrimitiveType(sf::Quads);
    vtBuff_.setUsage(sf::VertexBuffer::Stream);
    vtBuff_.create(w * h * 4);
    vts_.resize(w * h * 4);
    map_.resize(w * h);

    uintptr_t index;
    for(uintptr_t i = 0; i < size_.x; ++i) {
      for(uintptr_t j = 0; j < size_.y; ++j) {
        index = (i + j * size_.x) * 4;

        sf::Vector2f pos(i, j);
        vts_[index + 0].position = pos + sf::Vector2f(0.F, 0.F);
        vts_[index + 1].position = pos + sf::Vector2f(0.F, 1.F);
        vts_[index + 2].position = pos + sf::Vector2f(1.F, 1.F);
        vts_[index + 3].position = pos + sf::Vector2f(1.F, 0.F);
      }
    }

    update();
  }

  Cell& get(intptr_t x, intptr_t y) {
    if(x < 0) {
      x = size_.x - 1;
    }
    if(y < 0) {
      y = size_.y - 1;
    }
    if(x >= size_.x) {
      x = 0;
    }
    if(y >= size_.y) {
      y = 0;
    }
    return map_[x + y * size_.x];
  }

  void set(intptr_t x, intptr_t y, const Cell& cell) {
    if(x < 0) {
      x = size_.x - 1;
    }
    if(y < 0) {
      y = size_.y - 1;
    }
    if(x >= size_.x) {
      x = 0;
    }
    if(y >= size_.y) {
      y = 0;
    }
    map_[x + y * size_.x] = cell;
  }

  void invert(const sf::Vector2f& vec) {
    intptr_t x = floorf(vec.x);
    intptr_t y = floorf(vec.y);
    if(x > size_.x - 1 || x < 0 || y > size_.y - 1 || y < 0) {
      return;
    }

    Cell& cell = get(x, y);
    cell.now = !cell.now;
    cell.next = cell.now;

    update();
  }

  void clear() {
    for(auto& i : map_) {
      i.now = false;
      i.next = false;
    }
  }

  void randomize() {
    for(auto& i : map_) {
      i.now = rand() % 2;
      i.next = i.now;
    }
  }

  void update() {
    unsigned v;
    for(intptr_t i = 0; i < size_.x; i++) {
      for(intptr_t j = 0; j < size_.y; j++) {
        Cell& cell = get(i, j);

        v = 0;
        v += get(i - 1, j - 1).now ? 1 : 0;
        v += get(i - 1, j + 1).now ? 1 : 0;
        v += get(i + 1, j - 1).now ? 1 : 0;
        v += get(i + 1, j + 1).now ? 1 : 0;
        v += get(i - 1, j).now ? 1 : 0;
        v += get(i + 1, j).now ? 1 : 0;
        v += get(i, j - 1).now ? 1 : 0;
        v += get(i, j + 1).now ? 1 : 0;

        if(v > 2) {
          cell.next = true;
        }
        if(v < 2 || v > 3) {
          cell.next = false;
        }
      }
    }

    for(auto& i : map_) {
      i.now = i.next;
    }

    uintptr_t indexS;
    uintptr_t index;
    for(uintptr_t i = 0; i < size_.x; ++i) {
      for(uintptr_t j = 0; j < size_.y; ++j) {
        indexS = i + j * size_.x;
        index = indexS * 4;

        sf::Color color = map_[indexS].now ? sf::Color::White : sf::Color::Black;
        vts_[index + 0].color = color;
        vts_[index + 1].color = color;
        vts_[index + 2].color = color;
        vts_[index + 3].color = color;
      }
    }

    vtBuff_.update(vts_.data());
  }

  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const {
    target.draw(vtBuff_);
  }
protected:

  std::vector<Cell> map_;
  std::vector<sf::Vertex> vts_;
  sf::VertexBuffer vtBuff_;
  sf::Vector2<uintptr_t> size_;
};

#if DEBUG
int main() {
  SetConsoleTitleA("Debug");
#else
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow) {
#endif
  const sf::VideoMode basicVideoMode(800, 600);
  const sf::VideoMode fullVidoeMode = sf::VideoMode::getDesktopMode();

  Map map(200, 200);

  sf::Cursor cursor;
  cursor.loadFromSystem(sf::Cursor::Cross);

  sf::View view;
  view.setSize(80.F, 60.F);
  view.setCenter(0, 0);

  sf::Event event;

  sf::RenderWindow window(basicVideoMode, "Life", sf::Style::Close);
  window.setKeyRepeatEnabled(false);
  window.setVerticalSyncEnabled(true);
  window.setMouseCursor(cursor);

  bool fullscr = false;
  bool pause = true;

  sf::Clock clock;
  while(window.isOpen()) {
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
      view.move(0, -1);
    }
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
      view.move(0, 1);
    }
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
      view.move(1, 0);
    }
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
      view.move(-1, 0);
    }

    if(clock.getElapsedTime().asMilliseconds() > 50) {
      clock.restart();
      if(!pause) {
        map.update();
      }
    }

    window.clear(sf::Color::Green);
    window.setView(view);
    window.draw(map);
    window.display();

    while(window.pollEvent(event)) {
      switch(event.type) {
        case sf::Event::Closed:
        window.close();
        break;

        case sf::Event::KeyPressed:
        switch(event.key.code) {
          case sf::Keyboard::F11:
          fullscr = !fullscr;
          if(fullscr) {
            window.create(fullVidoeMode, "Life", sf::Style::Fullscreen);
            window.setMouseCursor(cursor);
            window.setKeyRepeatEnabled(false);
            window.setVerticalSyncEnabled(true);
            view.setSize(fullVidoeMode.width / 10, fullVidoeMode.height / 10);
          }
          else {
            window.create(basicVideoMode, "Life", sf::Style::Close);
            window.setMouseCursor(cursor);
            window.setKeyRepeatEnabled(false);
            window.setVerticalSyncEnabled(true);
            view.setSize(800 / 10, 600 / 10);
          }
          break;

          case sf::Keyboard::R:
          pause = false;
          break;

          case sf::Keyboard::P:
          pause = true;
          break;

          case sf::Keyboard::C:
          map.clear();
          break;

          case sf::Keyboard::G:
          map.randomize();
          break;

          case sf::Keyboard::Enter:
          map.update();
          break;
        }
        break;
        case sf::Event::MouseButtonPressed:
        switch(event.mouseButton.button) {
          case sf::Mouse::Left:
          map.invert(window.mapPixelToCoords(sf::Mouse::getPosition(window), view));
          break;

          case sf::Mouse::Right:
          view.setCenter(window.mapPixelToCoords(sf::Mouse::getPosition(window), view));
          break;
        }
        break;
      }
    }
  }
}
