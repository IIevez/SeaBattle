#include <vector>
#include <cstdlib>
#include<iostream>
#include <random>
#include <ctime>
#include <limits>
#include <cctype>
#include <utility>
#include <clocale>
#include <Windows.h>
// для начала так сделал, потом отключу
using namespace std;
class Player {
private:
    int money{};

public:
    void addMoney(int value) { money += value; }
    bool canAfford(int price) const { return money >= price; }
    void spendMoney(int price) { money -= price; }
    int getMoney() const { return money; }
};
struct Board {// главное отличие структуры от класса - в классе все эелементы базово приватные, в структуре же - публичные.
    //
    class Ship {
    public:
        vector<pair<int, int>> coords;
        vector<bool> hits;
        void addCell(int r, int c) {
            coords.emplace_back(r, c);
            hits.push_back(false);
        }
        bool markHit(int r, int c) {
            for (size_t i = 0; i < coords.size(); ++i) {
                if (coords[i].first == r && coords[i].second == c) {
                    hits[i] = true;
                    return true;
                }
            }
            return false;
        }
        bool isSunk() const {
            for (bool h : hits) {
                if (!h) return false;
            }
            return true;
        }
    };
    //
    class DESK {
    private:
        // первая строка и столбец - числа(номера полей, чтоб их наглядно видел пользователь)
        //ввел временно, будет удалено, надо изменить код под size, тк он у нас везде одинаковый
        int rows = 13;
        int cols = 13;
        const int size = 13;
        vector <vector<char>> desk;
        vector <Board::Ship> ships;
        Player& owner;
    public:
        // можно использовать самоссылку, написав Board::desk():..., но внутри классов структуры и ее методов это использовать излишне
        //конструктор 
        DESK(Player& p ) : desk(size, vector<char>(size, '.')), owner(p) {}
        //интересный способ высунуть что-то из приватного типа)
        int getsize() const { return size; }
        const vector<vector<char>>& getDesk() const { return desk; }
        vector<Board::Ship>& getShips() { return ships; }
        int shipPrice(int len) const {
            switch (len) {
            case 2: return 60;
            case 3: return 90;
            case 4: return 150;
            default: return 0;
            }
        }
        //Добавить проверку, что x1y1 лежит рядом с xy
        // xy - начальная позиция, x1y1 - направление корабля, len - его длинна

        bool CanPlaceShip(int x, int y, int x1, int y1, int len) {
            bool checker = true;
            int startlen = len;
            int price = shipPrice(len);
            if (!owner.canAfford(price)) checker = false;
            else {
                if (desk[x][y] == '.' && ((x == x1) || (y == y1) == 1) && x + len <= size && y + len <= size && y<=size && x<=size && x1<=size && y1<=size) {
                    // delta_smth - для вычисления приращения
                    int sum = 1;
                    int delta_x = x1 - x;
                    int delta_y = y1 - y;
                    len--;
                    if (abs(delta_x) == 1) {
                        while (len != 0) {
                            sum += (desk[x + len * delta_x][y] == '.');
                            len--;
                        }
                    }
                    if (abs(delta_y) == 1) {
                        while (len != 0) {
                            sum += (desk[x][y + len * delta_y] == '.');
                            len--;
                        }
                    }
                    if (sum != startlen) {
                        checker = false;
                    }
                }
                else {
                    checker = false;
                }
            }
            return checker;
        }
        bool placeShip(int x, int x1, int y, int y1, int len) {
            if (!CanPlaceShip(x,x1,y,y1,len)) return false;
            Ship ship;
            //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            if (abs(x - x1) == 1) {
                for (int i = 0; i < len; ++i) {
                    desk[x+i][y] = 'S';
                    ship.addCell(x+i, y + i);
                }
            }
            else { // 'V' or 'v'
                for (int i = 0; i < len; ++i) {
                    desk[x][y+i] = 'S';
                    ship.addCell(x, y+i);
                }
            }
            owner.spendMoney(shipPrice(len));
            ships.push_back(ship);
            return true;
        }
        // shootAt возвращает: 0=промах, 1=попадание, 2=попадание и уничтожение, 3=уже стрелял тут, -1=ошибка
        int shootAt(int r, int c) {
            if (r < 0 || r >= rows || c < 0 || c >= cols) return -1;
            if (desk[r][c] == 'X' || desk[r][c] == 'o') return 3;
            if (desk[r][c] == 'S') {
                desk[r][c] = 'X';
                for (auto& ship : ships) {
                    if (ship.markHit(r, c)) {
                        if (ship.isSunk()) return 2;
                        else return 1;
                    }
                }
            }
            // промах
            desk[r][c] = 'o';
            return 0;
        }
        bool allShipsSunk() const {
            for (const auto& ship : ships) {
                if (!ship.isSunk()) return false;
            }
            return true;
        }
        void display(bool showShips) const {
            cout << "  ";
            char ch;
            for (int c = 0; c < cols; ++c) {
                cout << (c + 1) << ' ';
            }
            cout << "\n";
            for (int r = 0; r < rows; ++r) {
                cout << (r + 1) << ' ';
                for (int c = 0; c < cols; ++c) {
                    ch = desk[r][c];
                    if (!showShips && ch == 'S') cout << ". ";
                    else cout << ch << ' ';
                }
                cout << "\n";
            }
        }

    };

};
class GamePlayer {
public:
    Player wallet;
    Board::DESK board;

    GamePlayer(int startMoney): wallet(), board(wallet)
    {
        wallet.addMoney(startMoney);
    }
};
class Game {
private:
    // Шаг 1: Создаем двух игроков. 
    // Каждый GamePlayer внутри себя содержит Player (кошелек) и Board::DESK (поле).
    GamePlayer p1;
    GamePlayer p2;

public:
    // Шаг 2: Конструктор игры.
    // Принимает количество денег. Инициализирует p1 и p2 этими деньгами.
    // В первом коде размер поля зашит внутри DESK (size = 13), поэтому передаем только деньги.
    Game(int startMoney) : p1(startMoney), p2(startMoney) {}

    // Шаг 3: Вспомогательный метод для отрисовки интерфейса.
    // Мы используем метод display, который уже есть в вашем классе DESK.
    void showInterface(GamePlayer& activePlayer, GamePlayer& enemyPlayer, int playerNum) {
        cout << "n===============================" << endl;
        cout << "ХОД ИГРОКА " << playerNum << endl;
        cout << "Деньги в кошельке: " << activePlayer.wallet.getMoney() << endl;

        cout << "n--- ВАШЕ ПОЛЕ ---" << endl;
        // true означает показывать свои корабли ('S')
        activePlayer.board.display(true);

        cout << "n--- ПОЛЕ ПРОТИВНИКА ---" << endl;
        // false означает скрывать чужие корабли (заменять 'S' на '.')
        enemyPlayer.board.display(false);
    }

    // Шаг 4: Основной игровой цикл.
    void run() {
        // Здесь могла бы быть логика расстановки кораблей placeShip,
        // Размещение корабля
    void placeShip(int startRow, int startCol,
        int length, bool horizontal) {
        Ship ship(length);

        for (int i = 0; i < length; ++i) {
            int row = startRow + (horizontal ? 0 : i);
            int col = startCol + (horizontal ? i : 0);

            field[row][col] = SHIP;
            ship.addCell(row, col);
        }

        ships.push_back(ship);
    }
        // но для простоты предположим, что мы начинаем стрельбу.
        bool gameOver = false;
        int currentPlayer = 1;

        while (!gameOver) {
            // Определяем, кто сейчас ходит, а кто защищается
            GamePlayer& attacker = (currentPlayer == 1 ? p1 : p2);
            GamePlayer& defender = (currentPlayer == 1 ? p2 : p1);

            showInterface(attacker, defender, currentPlayer);

            // Шаг 5: Ввод координат для выстрела
            int r, c;
            cout << "nВведите координаты для выстрела (строка и столбец): ";
            if (!(cin >> r >> c)) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), 'n');
                cout << "Ошибка ввода! Введите числа." << endl;
                continue;
            }

            // В вашем коде координаты в shootAt должны учитывать смещение (если ввод от 1 до 13)
            // Вычитаем 1, так как индексы в векторе начинаются с 0
            int result = defender.board.shootAt(r - 1, c - 1);

            // Шаг 6: Обработка результата выстрела (на основе вашего метода shootAt)
            switch (result) {
            case -1:
                cout << "Промах! Вы выстрелили за пределы поля." << endl;
                currentPlayer = (currentPlayer == 1 ? 2 : 1); // Смена хода
                break;
            case 0:
                cout << "Мимо!" << endl;
                currentPlayer = (currentPlayer == 1 ? 2 : 1); // Смена хода
                break;
            case 1:
                cout << "ПОПАДАНИЕ!" << endl;
                // При попадании ход не меняется (игрок стреляет снова)
                break;
            case 2:
                cout << "УБИТ! Корабль противника пошел ко дну!" << endl;
                attacker.wallet.addMoney(50); // Бонус за уничтожение (логика из головы)
                break;
            case 3:
                cout << "Вы уже стреляли в эту клетку!" << endl;
                break;
            }

            // Шаг 7: Проверка условия победы
            if (defender.board.allShipsSunk()) {
                cout << "nПОЗДРАВЛЯЕМ! Игрок " << currentPlayer << " победил!" << endl;
                gameOver = true;
            }

            cout << "nНажмите Enter, чтобы продолжить...";
            cin.ignore();
            cin.get();
            system("cls"); // Очистка экрана (для Windows)
        }
    }
};
int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    Game game(500);
    game.run();
    return 0;
}
