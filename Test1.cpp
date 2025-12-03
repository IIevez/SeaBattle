#include <iostream>                     // Подключаем ввод/вывод
#include <vector>                       // Используем std::vector
#include <random>                       // Для случайных чисел
#include <ctime>                        // Для инициализации случайного генератора

using namespace std;

// ===============================================================
// КЛАСС Ship — хранит клетки корабля и попадания по ним
// ===============================================================
class Ship {
public:
    vector<pair<int,int>> cells;        // Вектор клеток корабля (каждая — координата)
    vector<bool> hits;                  // Те же размеры, но true/false — попадание

    // Конструктор. Принимает вектор клеток.
    Ship(const vector<pair<int,int>>& cells)
        : cells(cells), hits(cells.size(), false) {}

    // Проверяет, потоплен ли корабль (все клетки подбиты)
    bool isSunk() const {
        for (bool h : hits)             // Проходим по каждому элементу hits
            if (!h) return false;       // Если есть хоть одно неповреждённое — корабль жив
        return true;                    // Все подбиты — корабль утонул
    }

    // Отмечает попадание, если координата попала по кораблю
    bool registerHit(int x, int y) {
        for (int i = 0; i < cells.size(); i++) {   // Перебираем клетки корабля
            if (cells[i].first == x && cells[i].second == y) {
                hits[i] = true;        // Отмечаем попадание
                return true;
            }
        }
        return false;                  // Нет попадания
    }
};

// ===============================================================
// КЛАСС Board — игровое поле, корабли, выстрелы
// ===============================================================
class Board {
    int size;                                    // Размер поля NxN
    vector<vector<char>> grid;                    // Игровая сетка: '.', 'S', 'o', 'X'
    vector<Ship> ships;                           // Список кораблей
    mt19937 rng;                                  // Генератор случайных чисел

public:
    // Конструктор. Создаём поле NxN заполненное '.'
    Board(int n) : size(n), grid(n, vector<char>(n, '.')), rng(time(nullptr)) {}

    // Геттер размера
    int getSize() const { return size; }

    // Геттер игрового поля (для отображения)
    const vector<vector<char>>& getGrid() const { return grid; }

    // Доступ к кораблям
    vector<Ship>& getShips() { return ships; }

    // Проверяем, можно ли разместить корабль
    bool canPlaceShip(int x, int y, int len, bool horizontal) {
        if (horizontal) {                        // Горизонтальное размещение
            if (y + len > size) return false;    // Выход за границу
            for (int j = y; j < y + len; j++)
                if (grid[x][j] != '.') return false; // Клетка занята
        } else {                                 // Вертикальное размещение
            if (x + len > size) return false;
            for (int i = x; i < x + len; i++)
                if (grid[i][y] != '.') return false;
        }
        return true;                             // Размещение возможно
    }

    // Размещаем корабль (координаты уже проверены)
    void placeShip(int x, int y, int len, bool horizontal) {
        vector<pair<int,int>> cells;             // Будущие клетки корабля

        if (horizontal) {
            for (int j = y; j < y + len; j++) {
                grid[x][j] = 'S';                // Помечаем 'S'
                cells.push_back({x, j});
            }
        } else {
            for (int i = x; i < x + len; i++) {
                grid[i][y] = 'S';
                cells.push_back({i, y});
            }
        }
        ships.push_back(Ship(cells));            // Добавляем корабль в список
    }

    // Автоматическая расстановка кораблей
    void autoPlaceShips(int count) {
        uniform_int_distribution<int> dir(0, 1);        // 0 — вертикально, 1 — горизонтально
        uniform_int_distribution<int> dist(0, size - 1);// Случайные координаты

        for (int k = 0; k < count; k++) {
            bool placed = false;
            int len = 2 + (k % 3);              // Длины 2,3,4 повторяются

            while (!placed) {
                int x = dist(rng);
                int y = dist(rng);
                bool horizontal = dir(rng);

                if (canPlaceShip(x, y, len, horizontal)) {
                    placeShip(x, y, len, horizontal);
                    placed = true;
                }
            }
        }
    }

    // Выстрел по полю
    bool shoot(int x, int y) {
        if (grid[x][y] == 'S') {                 // Попадание
            grid[x][y] = 'X';                    // Отмечаем
            for (auto& s : ships)                // Отмечаем попадание у корабля
                s.registerHit(x, y);
            return true;
        }
        if (grid[x][y] == '.')                   // Промах
            grid[x][y] = 'o';
        return false;
    }

    // Проверка: все корабли потоплены?
    bool allSunk() const {
        for (const auto& s : ships)
            if (!s.isSunk()) return false;
        return true;
    }
};

// ===============================================================
// КЛАСС Player — содержит поле и делает выстрелы
// ===============================================================
class Player {
public:
    Board board;                 // Поле игрока

    // Конструктор
    Player(int size) : board(size) {}

    // Выставить корабли
    void setupShips(int count) {
        board.autoPlaceShips(count);
    }

    // Ход игрока — ввод координат + выстрел
    bool shoot(Player& enemy) {
        int x, y;
        cout << "Введите координаты выстрела (x y): ";
        cin >> x >> y;

        x--; y--;                // Перевод в 0-индексацию

        // Проверка на выход за границы
        if (x < 0 || y < 0 || x >= enemy.board.getSize() || y >= enemy.board.getSize()) {
            cout << "Выстрел вне поля! Ход потерян.\n";
            return false;
        }

        // Делаем выстрел
        bool hit = enemy.board.shoot(x, y);
        cout << (hit ? "Попадание!" : "Мимо.") << "\n";
        return hit;
    }
};

// ===============================================================
// КЛАСС Game — основной цикл игры
// ===============================================================
class Game {
    Player p1;                       // Игрок 1
    Player p2;                       // Игрок 2

public:
    // Конструктор
    Game(int size, int shipCount)
        : p1(size), p2(size)
    {
        p1.setupShips(shipCount);    // Расставляем корабли 1-го
        p2.setupShips(shipCount);    // Расставляем корабли 2-го
    }

    // Печать поля (можно скрывать корабли)
    void printBoard(const Board& b, bool hideShips) {
        int n = b.getSize();

        cout << "   ";
        for (int j = 0; j < n; j++) cout << j+1 << " ";
        cout << "\n";

        for (int i = 0; i < n; i++) {
            cout << i+1 << (i+1 < 10 ? "  " : " ");  // Красивое выравнивание

            for (int j = 0; j < n; j++) {
                char c = b.getGrid()[i][j];

                if (hideShips && c == 'S')           // Прячем корабли противника
                    cout << ". ";
                else
                    cout << c << " ";
            }
            cout << "\n";
        }
    }

    // Основной игровой цикл
    void run() {
        while (true) {

            cout << "\n==== Поле игрока 1 ====\n";
            printBoard(p1.board, false);

            cout << "\n==== Поле игрока 2 (скрыто) ====\n";
            printBoard(p2.board, true);

            cout << "\nХод игрока 1:\n";
            p1.shoot(p2);

            if (p2.board.allSunk()) {
                cout << "\nИгрок 1 победил!\n";
                break;
            }

            cout << "\n==== Поле игрока 1 (скрыто) ====\n";
            printBoard(p1.board, true);

            cout << "\n==== Поле игрока 2 ====\n";
            printBoard(p2.board, false);

            cout << "\nХод игрока 2:\n";
            p2.shoot(p1);

            if (p1.board.allSunk()) {
                cout << "\nИгрок 2 победил!\n";
                break;
            }
        }
    }
};

// ===============================================================
// MAIN — запуск игры
// ===============================================================
int main() {
    int size, ships;

    cout << "Введите размер поля (например 10): ";
    cin >> size;

    cout << "Введите количество кораблей: ";
    cin >> ships;

    Game g(size, ships);        // Создаём игру
    g.run();                    // Запускаем

    return 0;
}
