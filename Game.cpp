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
        DESK(Player& p) : desk(size, vector<char>(size, '.')), owner(p) {}
        //интересный способ высунуть что-то из приватного типа)
        int getsize() const { return size; }
        const vector<vector<char>>& getDesk() const { return desk; }
        vector<Board::Ship>& getShips() { return ships; }
        int shipPrice(int len) const {
            switch (len) {
            case 1: return 30;
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
                if (desk[x][y] == '.' && ((x == x1) || (y == y1) == 1) && x + len <= size && y + len <= size && y <= size && x <= size && x1 <= size && y1 <= size) {
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
            if (!CanPlaceShip(x, x1, y, y1, len)) return false;
            Ship ship;
            //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            if (abs(x - x1) == 1) {
                for (int i = 0; i < len; ++i) {
                    desk[x + i][y] = 'S';
                    ship.addCell(x + i, y + i);
                }
            }
            else { // 'V' or 'v'
                for (int i = 0; i < len; ++i) {
                    desk[x][y + i] = 'S';
                    ship.addCell(x, y + i);
                }
            }
            owner.spendMoney(shipPrice(len));
            ships.push_back(ship);
            return true;
        }

        void placeAllShips(int playerNum) {
            // по одному кораблю каждой длины 1..4
            bool placed[5] = { false, false, false, false, false };
            int placedCount = 0;

            while (placedCount < 4) {
                cout << "\n===============================\n";
                cout << "РАССТАНОВКА КОРАБЛЕЙ (Игрок " << playerNum << ")\n";
                cout << "Уже поставлено: ";
                for (int len = 1; len <= 4; ++len) {
                    cout << len << (placed[len] ? "[OK] " : "[ ] ");
                }
                cout << "\n\nВаше поле:\n";
                display(true);

                int len;
                cout << "\nВведите длину корабля (1..4): ";
                if (!(cin >> len)) {
                    cin.clear();
                    cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                    cout << "Ошибка ввода! Введите число.\n";
                    continue;
                }
                if (len < 1 || len > 4) {
                    cout << "Длина должна быть от 1 до 4.\n";
                    continue;
                }
                if (placed[len]) {
                    cout << "Корабль длины " << len << " уже поставлен.\n";
                    continue;
                }

                int r, c;
                char dir;
                cout << "Введите стартовую клетку (строка и столбец): ";
                if (!(cin >> r >> c)) {
                    cin.clear();
                    cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                    cout << "Ошибка ввода! Введите два числа.\n";
                    continue;
                }
                cout << "Введите направление (U/D/L/R): ";
                if (!(cin >> dir)) {
                    cin.clear();
                    cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                    cout << "Ошибка ввода!\n";
                    continue;
                }
                dir = (char)toupper((unsigned char)dir);

                int dr = 0, dc = 0;
                if (dir == 'U') dr = -1;
                else if (dir == 'D') dr = 1;
                else if (dir == 'L') dc = -1;
                else if (dir == 'R') dc = 1;
                else {
                    cout << "Направление должно быть U/D/L/R.\n";
                    continue;
                }

                // перевод в 0-based
                int sr = r - 1;
                int sc = c - 1;

                // проверка всех клеток
                bool ok = true;
                for (int i = 0; i < len; ++i) {
                    int rr = sr + dr * i;
                    int cc = sc + dc * i;

                    if (rr < 0 || rr >= size || cc < 0 || cc >= size) {
                        ok = false;
                        break;
                    }
                    if (desk[rr][cc] != '.') {
                        ok = false;
                        break;
                    }
                }

                if (!ok) {
                    cout << "Нельзя поставить корабль: выходит за поле или пересекается с другим.\n";
                    continue;
                }

                // размещаем
                Ship ship;
                for (int i = 0; i < len; ++i) {
                    int rr = sr + dr * i;
                    int cc = sc + dc * i;
                    desk[rr][cc] = 'S';
                    ship.addCell(rr, cc);
                }
                ships.push_back(ship);

                placed[len] = true;
                placedCount++;

                cout << "Корабль длины " << len << " размещён.\n";
            }

            cout << "\nВсе корабли игрока " << playerNum << " размещены!\n";
        }
        // Проверка: можно ли поставить набор клеток (newCells) для выбранного shipIndex
      // Не вылезает за поле и не накрывает другие корабли/следы выстрелов.
        bool canApplyMove(int shipIndex, const vector<pair<int, int>>& newCells) const {
            for (const auto& cell : newCells) {
                int r = cell.first;
                int c = cell.second;

                if (r < 0 || r >= size || c < 0 || c >= size)
                    return false;

                char ch = desk[r][c];

                bool isOwnCell = false;
                for (const auto& oldCell : ships[shipIndex].coords) {
                    if (oldCell.first == r && oldCell.second == c) {
                        isOwnCell = true;
                        break;
                    }
                }
                if (!isOwnCell && ch != '.' && ch != 'o' && ch != 'X')
                    return false;
            }
            return true;
        }


        // Применение перемещения/поворота: очищаем старые клетки и рисуем новые с учетом попаданий
        void applyMove(int shipIndex, const vector<pair<int, int>>& newCells) {
            // очистить старые клетки корабля
            for (const auto& cell : ships[shipIndex].coords) {
                int r = cell.first;
                int c = cell.second;
                if (desk[r][c] == 'S' || desk[r][c] == 'X' || desk[r][c] == 'N')
                    desk[r][c] = '.';
            }

            ships[shipIndex].coords = newCells;

            // поставить новые клетки
            for (size_t i = 0; i < newCells.size(); ++i) {
                int r = newCells[i].first;
                int c = newCells[i].second;

                char prev = desk[r][c];

                if (ships[shipIndex].hits[i]) {
                    desk[r][c] = 'X';
                }
                else if (prev == 'o' || prev == 'X') {
                    desk[r][c] = 'N';
                }
                else {
                    desk[r][c] = 'S';
                }
            }
        }


        // Меню перемещения одного корабля: сдвиги U/D/L/R и разворот T (turn)
        void moveShipMenu(int playerNum) {
            if (ships.empty()) {
                cout << "\nУ вас нет кораблей для перемещения.\n";
                return;
            }
            while (true) {
                cout << "\n===============================\n";
                cout << "ПЕРЕМЕЩЕНИЕ КОРАБЛЕЙ (Игрок " << playerNum << ")\n";
                cout << "Стоимость действия: 50 монет\n";
                cout << "Ваши деньги: " << owner.getMoney() << "\n\n";
                display(true);

                cout << "\nСписок кораблей:\n";
                for (size_t i = 0; i < ships.size(); ++i) {
                    cout << (i + 1) << ") длина " << ships[i].coords.size();
                    if (ships[i].isSunk()) cout << " (потоплен)";
                    cout << "\n";
                }

                cout << "\nВыберите корабль (1.." << ships.size() << ", 0 - выйти в меню действий): ";
                int idx;
                if (!(cin >> idx)) {
                    cin.clear();
                    cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                    cout << "Ошибка ввода!\n";
                    continue;
                }
                if (idx == 0) return;
                if (idx < 1 || idx >(int)ships.size()) {
                    cout << "Нет такого корабля.\n";
                    continue;
                }

                int shipIndex = idx - 1;

                cout << "\nДействие: U(up) / D(down) / L(left) / R(right) / T(развернуть) / Q(назад): ";
                char act;
                if (!(cin >> act)) {
                    cin.clear();
                    cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                    cout << "Ошибка ввода!\n";
                    continue;
                }
                act = (char)toupper((unsigned char)act);

                if (act == 'Q') continue;

                if (!owner.canAfford(50)) {
                    cout << "Недостаточно денег (нужно 50).\n";
                    continue;
                }

                // подготовим новые клетки
                vector<pair<int, int>> newCells = ships[shipIndex].coords;

                if (act == 'U' || act == 'D' || act == 'L' || act == 'R') {
                    int dr = 0, dc = 0;
                    if (act == 'U') dr = -1;
                    if (act == 'D') dr = 1;
                    if (act == 'L') dc = -1;
                    if (act == 'R') dc = 1;

                    for (auto& cell : newCells) {
                        cell.first += dr;
                        cell.second += dc;
                    }
                }
                else if (act == 'T') {
                    // разворот вокруг "старта" = coords[0]
                    int len = (int)newCells.size();
                    if (len == 1) {
                        cout << "Корабль длины 1 разворачивать некуда.\n";
                        continue;
                    }

                    int ar = ships[shipIndex].coords[0].first;
                    int ac = ships[shipIndex].coords[0].second;

                    // определяем текущую ориентацию по coords[0] и coords[1]
                    int r1 = ships[shipIndex].coords[1].first;
                    int c1 = ships[shipIndex].coords[1].second;

                    bool isHorizontal = (r1 == ar); // значит менялся столбец
                    // если горизонтальный — делаем вертикальный вниз от якоря
                    // если вертикальный — делаем горизонтальный вправо от якоря
                    newCells.clear();
                    newCells.reserve(len);

                    if (isHorizontal) {
                        for (int i = 0; i < len; ++i) {
                            newCells.emplace_back(ar + i, ac);
                        }
                    }
                    else {
                        for (int i = 0; i < len; ++i) {
                            newCells.emplace_back(ar, ac + i);
                        }
                    }
                }
                else {
                    cout << "Неизвестное действие.\n";
                    continue;
                }
                if (!canApplyMove(shipIndex, newCells)) {
                    cout << "Нельзя выполнить: выйдет за поле или накроет другой корабль/следы выстрелов.\n";
                    continue;
                }

                // списываем деньги и применяем
                owner.spendMoney(50);
                applyMove(shipIndex, newCells);

                cout << "Готово! (-50 монет)\n";
                // можно двигать сколько угодно, пока есть деньги - поэтому не выходим
            }
        }

        // Меню действий игрока перед выстрелом
        // shootAt возвращает: 0=промах, 1=попадание, 2=попадание и уничтожение, 3=уже стрелял тут, -1=ошибка
        int shootAt(int r, int c) {
            if (r < 0 || r >= rows || c < 0 || c >= cols) return -1;
            if (desk[r][c] == 'X' || desk[r][c] == 'o') return 3;

            if (desk[r][c] == 'N') {
                // Находим, какой корабль занимает эту клетку, и какой сегмент
                for (auto& ship : ships) {
                    for (size_t i = 0; i < ship.coords.size(); ++i) {
                        if (ship.coords[i].first == r && ship.coords[i].second == c) {
                            // Если сегмент живой -> попадание
                            if (!ship.hits[i]) {
                                ship.hits[i] = true;
                                desk[r][c] = 'X';
                                if (ship.isSunk()) return 2;
                                return 1;
                            }
                            // Если сегмент уже подбит -> промах, но N остается N
                            return 0;
                        }
                    }
                }
                // Если вдруг N без корабля (не должно случиться) — считаем промахом
                return 0;
            }

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

    GamePlayer(int startMoney) : wallet(), board(wallet)
    {
        wallet.addMoney(startMoney);
    }
};

class Shop {
public:
    void open(GamePlayer& player, int playerNum) {
        while (true) {
            cout << "\n===============================\n";
            cout << "МАГАЗИН (Игрок " << playerNum << ")\n";
            cout << "Деньги: " << player.wallet.getMoney() << "\n\n";

            cout << "1. Купить корабль\n";
            cout << "2. Починить корабль\n";
            cout << "3. Передвинуть корабль\n";
            cout << "0. Выйти и стрелять\n";

            int choice;
            if (!(cin >> choice)) {
                cin.clear();
                cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                continue;
            }

            if (choice == 0) return;
            if (choice == 1) buyShip(player);
            if (choice == 2) repairShip(player);
            if (choice == 3) moveShip(player, playerNum);
        }
    }
private:
    void buyShip(GamePlayer& player) {
        int len;
        cout << "Длина корабля (1..4): ";
        if (!(cin >> len)) {
            cin.clear();
            cin.ignore((numeric_limits<streamsize>::max)(), '\n');
            cout << "Ошибка ввода.\n";
            return;
        }

        if (len < 1 || len > 4) {
            cout << "Длина должна быть от 1 до 4.\n";
            return;
        }

        int price = player.board.shipPrice(len);
        if (!player.wallet.canAfford(price)) {
            cout << "Недостаточно денег.\n";
            return;
        }

        int r, c;
        char dir;

        cout << "Старт (строка столбец): ";
        if (!(cin >> r >> c)) {
            cin.clear();
            cin.ignore((numeric_limits<streamsize>::max)(), '\n');
            cout << "Ошибка ввода.\n";
            return;
        }
        if (len == 1) {
            dir = 'R';
        }
        else {
            cout << "Направление (U/D/L/R): ";
            if (!(cin >> dir)) {
                cin.clear();
                cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                cout << "Ошибка ввода.\n";
                return;
            }
        }

        dir = (char)toupper((unsigned char)dir);

        int dr = 0, dc = 0;
        if (dir == 'U') dr = -1;
        else if (dir == 'D') dr = 1;
        else if (dir == 'L') dc = -1;
        else if (dir == 'R') dc = 1;
        else {
            cout << "Неверное направление.\n";
            return;
        }


        int x0 = r - 1;
        int y0 = c - 1;
        int x1 = x0 + dr * (len - 1);
        int y1 = y0 + dc * (len - 1);


        bool placed = player.board.placeShip(
            x0, x1,
            y0, y1,
            len
        );

        if (!placed) {
            cout << "Не удалось разместить корабль.\n";
        }
        else {
            cout << "Корабль успешно куплен и размещён.\n";
        }
    }

    void repairShip(GamePlayer& player) {
        auto& ships = player.board.getShips();

        if (ships.empty()) {
            cout << "Кораблей нет.\n";
            return;
        }

        cout << "Выберите корабль:\n";
        for (size_t i = 0; i < ships.size(); ++i) {
            cout << i + 1 << ") длина " << ships[i].coords.size() << "\n";
        }

        int idx;
        cin >> idx;
        idx--;

        int cost = 30;
        if (!player.wallet.canAfford(cost)) {
            cout << "Недостаточно денег.\n";
            return;
        }

        for (size_t i = 0; i < ships[idx].hits.size(); ++i) {
            if (ships[idx].hits[i]) {
                ships[idx].hits[i] = false;
                player.wallet.spendMoney(cost);
                cout << "Клетка корабля починена.\n";
                return;
            }
        }

        cout << "Корабль не повреждён.\n";
    }

    void moveShip(GamePlayer& player, int playerNum) {
        player.board.moveShipMenu(playerNum);
    }
};



class Game {
private:
    // Шаг 1: Создаем двух игроков. 
    // Каждый GamePlayer внутри себя содержит Player (кошелек) и Board::DESK (поле).
    GamePlayer p1;
    GamePlayer p2;
    Shop shop;

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
        // но для простоты предположим, что мы начинаем стрельбу.
        p1.board.placeAllShips(1);
        system("cls");
        p2.board.placeAllShips(2);
        system("cls");

        bool gameOver = false;
        int currentPlayer = 1;

        int lastShotR[3] = { -1, -1, -1 };
        int lastShotC[3] = { -1, -1, -1 };
        bool hasLastShot[3] = { false, false, false };

        while (!gameOver) {
            // Определяем, кто сейчас ходит, а кто защищается
            GamePlayer& attacker = (currentPlayer == 1 ? p1 : p2);
            GamePlayer& defender = (currentPlayer == 1 ? p2 : p1);

            showInterface(attacker, defender, currentPlayer);
            cout << "\nЗайти в магазин? (1 - да, 0 - нет): ";
            int goShop;
            cin >> goShop;
            if (goShop == 1) {
                shop.open(attacker, currentPlayer);
            }

            // Шаг 5: Ввод координат для выстрела
            int r, c;
            cout << "nВведите координаты для выстрела (строка и столбец): ";
            if (!(cin >> r >> c)) {
                cin.clear();
                cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                cout << "Ошибка ввода! Введите числа." << endl;
                continue;
            }

            // Нельзя стрелять в ту же клетку два хода подряд (можно только через ход) ДЛЯ КАЖДОГО ИГРОКА ОТДЕЛЬНО
            if (hasLastShot[currentPlayer] && (r - 1) == lastShotR[currentPlayer] && (c - 1) == lastShotC[currentPlayer]) {
                cout << "В эту клетку нельзя стрелять два хода подряд. Выберите другую.\n";
                continue;
            }

            int result = defender.board.shootAt(r - 1, c - 1);

            if (result != -1) {
                lastShotR[currentPlayer] = r - 1;
                lastShotC[currentPlayer] = c - 1;
                hasLastShot[currentPlayer] = true;
            }

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
                attacker.wallet.addMoney(50);
                break;
            case 3:
                cout << "Вы уже стреляли в эту клетку!" << endl;
                break;
            }

            // Шаг 7: Проверка условия победы
            if (defender.board.allShipsSunk()) {
                cout << "\nПОЗДРАВЛЯЕМ! Игрок " << currentPlayer << " победил!" << endl;
                gameOver = true;
            }

            cout << "\nНажмите Enter, чтобы продолжить...";
            cin.ignore();
            cin.get();
            system("cls"); // Очистка экрана (для Windows)
        }
    }
};
int main() {
    setlocale(LC_ALL, "Ru");
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    Game game(500);
    game.run();
    return 0;
}
