#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <iostream>
#include <Windows.h>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

class Map : public sf::Drawable {
public:

  using Size = sf::Vector2<intptr_t>;
  struct Cell {
    bool now = false;
    bool next = false;
  };

  Map() {
    vtBuff_.setPrimitiveType(sf::Quads);
    vtBuff_.setUsage(sf::VertexBuffer::Stream);
  }

  Map(const Size& size) {
    vtBuff_.setPrimitiveType(sf::Quads);
    vtBuff_.setUsage(sf::VertexBuffer::Stream);

    setSize(size);
  }

  ~Map() = default;

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
    const intptr_t x = static_cast<intptr_t>(floorf(vec.x));
    const intptr_t y = static_cast<intptr_t>(floorf(vec.y));
    if(x > size_.x - 1 || x < 0 ||
       y > size_.y - 1 || y < 0) {
      return;
    }

    Cell& cell = get(x, y);
    cell.now = !cell.now;
    cell.next = cell.now;

    updateRenderer();
  }

  Size getSize() const {
    return size_;
  }

  void setSize(const Size& size) {
    size_ = size;

    vtBuff_.create(size_.x * size_.y * 4);
    vts_.resize(size_.x * size_.y * 4);
    map_.resize(size_.x * size_.y);

    intptr_t index;
    for(intptr_t i = 0; i < size_.x; ++i) {
      for(intptr_t j = 0; j < size_.y; ++j) {
        index = (i + j * size_.x) * 4;

        sf::Vector2f pos(static_cast<float>(i),
                         static_cast<float>(j));
        vts_[index + 0].position = pos + sf::Vector2f(0.F, 0.F);
        vts_[index + 1].position = pos + sf::Vector2f(0.F, 1.F);
        vts_[index + 2].position = pos + sf::Vector2f(1.F, 1.F);
        vts_[index + 3].position = pos + sf::Vector2f(1.F, 0.F);
      }
    }

    update();
  }

  void clear() {
    for(auto& i : map_) {
      i.now = false;
      i.next = false;
    }

    updateRenderer();
  }

  void randomize() {
    for(auto& i : map_) {
      i.now = rand() % 2;
      i.next = i.now;
    }

    updateRenderer();
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

    updateRenderer();
  }

  void updateRenderer() {
    intptr_t indexS;
    intptr_t index;
    for(intptr_t i = 0; i < size_.x; ++i) {
      for(intptr_t j = 0; j < size_.y; ++j) {
        indexS = i + j * size_.x;
        index = indexS * 4;

        const sf::Color& color = map_[indexS].now ? sf::Color::White : sf::Color::Black;
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
  Size size_;
};
Map* map = nullptr;

class MapUpdater {
public:

  std::atomic_bool pause = true;

  MapUpdater() :thread_(&MapUpdater::worker, this) {}

  ~MapUpdater() {
    work_ = false;
    thread_.join();
  }

  void lock() {
    mutex_.lock();
  }

  void unlock() {
    mutex_.unlock();
  }
protected:

  void worker() {
    sf::Clock clock;

    while(work_) {
      if(clock.getElapsedTime().asMilliseconds() > 100) {
        clock.restart();

        if(!pause) {
          if(mutex_.try_lock()) {
            map->update();
            mutex_.unlock();
          }
        }

        sf::sleep(sf::milliseconds(5));
      }
    }
  }


  std::atomic_bool work_ = true;
  std::mutex mutex_;
  std::thread thread_;
};
MapUpdater* mapUpdater = nullptr;

#if DEBUG
int main() {
  SetConsoleTitleA("Debug");
#else
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow) {
#endif
  const sf::VideoMode basicVideoMode(800, 600);
  const sf::VideoMode fullVidoeMode = sf::VideoMode::getDesktopMode();

  map = new Map(Map::Size(500, 500));
  mapUpdater = new MapUpdater;

  sf::Cursor cursor;
  cursor.loadFromSystem(sf::Cursor::Cross);

  sf::View view;
  view.setSize(sf::Vector2f(80.F, 60.F));
  view.setCenter(sf::Vector2f(0.F, 0.F));

  sf::Event event;

  sf::RenderWindow window(basicVideoMode, "Life", sf::Style::Close);
  window.setKeyRepeatEnabled(false);
  window.setVerticalSyncEnabled(true);
  window.setMouseCursor(cursor);

  bool fullscr = false;
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

    window.clear(sf::Color::Green);
    window.setView(view);
    mapUpdater->lock();
    window.draw(*map);
    mapUpdater->unlock();
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
            view.setSize(sf::Vector2f(fullVidoeMode.width / 10.F, fullVidoeMode.height / 10.F));
          }
          else {
            window.create(basicVideoMode, "Life", sf::Style::Close);
            window.setMouseCursor(cursor);
            window.setKeyRepeatEnabled(false);
            window.setVerticalSyncEnabled(true);
            view.setSize(sf::Vector2f(80.F, 60.F));
          }
          break;

          case sf::Keyboard::P:
          mapUpdater->pause = !mapUpdater->pause;
          break;

          case sf::Keyboard::C:
          mapUpdater->lock();
          map->clear();
          mapUpdater->unlock();
          break;

          case sf::Keyboard::R:
          mapUpdater->lock();
          map->randomize();
          mapUpdater->unlock();
          break;

          case sf::Keyboard::Enter:
          if(mapUpdater->pause) {
            mapUpdater->lock();
            map->update();
            mapUpdater->unlock();
          }
          break;
        }
        break;

        case sf::Event::MouseButtonPressed:
        switch(event.mouseButton.button) {
          case sf::Mouse::Left:
          mapUpdater->lock();
          map->invert(window.mapPixelToCoords(sf::Mouse::getPosition(window), view));
          mapUpdater->unlock();
          break;

          case sf::Mouse::Right:
          view.setCenter(window.mapPixelToCoords(sf::Mouse::getPosition(window), view));
          break;
        }
        break;
      }
    }
  }

  delete mapUpdater;
  delete map;
}
