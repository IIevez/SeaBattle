
#include <winsock2.h>
#include <ws2tcpip.h> // Для inet_pton, хоть ты и используешь inet_addr в Client, Host будет использовать INADDR_ANY
#include <vector>
#include <cstdlib>
#include <iostream>
#include <random>
#include <ctime>
#include <limits>
#include <cctype>
#include <utility>
#include <clocale>
#include <Windows.h>
#include <thread>
#include <string> // Для std::string
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable: 4996) // Отключаем предупреждение для inet_addr
// для начала так сделал, потом отключу
using namespace std;

// --- ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ДЛЯ СЕТЕВОГО ПОДКЛЮЧЕНИЯ ---
// Connection будет использоваться для прямой связи между Хостом (Игроком 1) и Клиентом (Игроком 2)
SOCKET Connection;

// Эти глобальные переменные из твоего сервера НЕ будут использоваться для 1-на-1 игровой логики,
// так как они предназначены для центрального сервера с несколькими клиентами.
// Их наличие здесь не влияет на игру 1-на-1, но они не будут задействованы.
SOCKET Connections[2];
int Counter = 0;

// --- СУЩЕСТВУЮЩИЙ КОД ClientHandler (НЕ ИЗМЕНЯЕТСЯ СОГЛАСНО ИНСТРУКЦИИ) ---
// ВНИМАНИЕ: Эта функция ClientHandler не будет использоваться для пошаговой игровой логики
// в модели "один Хост, один Клиент", так как Game::runNetwork самостоятельно управляет recv/send.
// Запуск ее в потоке приведет к конфликтам сообщений.
// Она сохранена здесь ТОЛЬКО в соответствии с ограничением "НЕ ИЗМЕНЯТЬ".
void Clienthandler(int index) { // Принимает индекс, но в 1-на-1 режиме это не имеет смысла
    char msg_buffer[256];
    while (true) {
        // Эта часть кода может быть вызвана только если Connections[index] действительно существует и активен.
        // В 1-на-1 Host/Client режиме Connection - это единственный сокет, Connections - не используется.
        // Это потенциально проблемный код для текущей архитектуры.
        // Если бы он использовался, то recv здесь бы конкурировал с recv в Game::runNetwork.
        // Поскольку он не используется, это просто неактивный код.
        memset(msg_buffer, 0, sizeof(msg_buffer));
        int bytes_received = recv(Connections[index], msg_buffer, sizeof(msg_buffer), 0);
        if (bytes_received <= 0) {
            // cout << "ClientHandler: Соединение разорвано или ошибка." << endl;
            break;
        }
        // cout << "ClientHandler: Получено сообщение: " << msg_buffer << endl;
        // Если это центральный сервер, он бы тут ретранслировал. В 1-на-1 он этого не делает.
        for (int i = 0; i < Counter; i++) {
            if (i == index) continue;
            // send(Connections[i], msg_buffer, bytes_received, 0); // Ретрансляция
        }
    }
}
// ----------------------------------------------------------------------------------

class Player {
private:
    int money{};

public:
    void addMoney(int value) { money += value; }
    bool canAfford(int price) const { return money >= price; }
    void spendMoney(int price) { money -= price; }
    int getMoney() const { return money; }
};
struct Board {
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
    class DESK {
    private:
        int rows = 13;
        int cols = 13;
        const int size = 13;
        vector <vector<char>> desk;
        vector <Board::Ship> ships;
        Player& owner;
    public:
        DESK(Player& p) : desk(size, vector<char>(size, '.')), owner(p) {}
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
        bool CanPlaceShip(int x, int y, int x1, int y1, int len) {
            bool checker = true;
            int startlen = len;
            int price = shipPrice(len);
            if (!owner.canAfford(price)) checker = false;
            else {
                if (desk[x][y] == '.' && ((x == x1) || (y == y1) == 1) && x + len <= size && y + len <= size && y <= size && x <= size && x1 <= size && y1 <= size) {
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

                int sr = r - 1;
                int sc = c - 1;

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

        void applyMove(int shipIndex, const vector<pair<int, int>>& newCells) {
            for (const auto& cell : ships[shipIndex].coords) {
                int r = cell.first;
                int c = cell.second;
                if (desk[r][c] == 'S' || desk[r][c] == 'X' || desk[r][c] == 'N')
                    desk[r][c] = '.';
            }

            ships[shipIndex].coords = newCells;

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
                    int len = (int)newCells.size();
                    if (len == 1) {
                        cout << "Корабль длины 1 разворачивать некуда.\n";
                        continue;
                    }

                    int ar = ships[shipIndex].coords[0].first;
                    int ac = ships[shipIndex].coords[0].second;

                    int r1 = ships[shipIndex].coords[1].first;
                    int c1 = ships[shipIndex].coords[1].second;

                    bool isHorizontal = (r1 == ar);
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

                owner.spendMoney(50);
                applyMove(shipIndex, newCells);

                cout << "Готово! (-50 монет)\n";
            }
        }

        // shootAt возвращает: -1=ошибка, 0=промах, 1=попадание, 2=попадание и уничтожение, 3=уже стрелял тут
        int shootAt(int r, int c) {
            if (r < 0 || r >= rows || c < 0 || c >= cols) return -1;
            if (desk[r][c] == 'X' || desk[r][c] == 'o') return 3;

            if (desk[r][c] == 'N') {
                for (auto& ship : ships) {
                    for (size_t i = 0; i < ship.coords.size(); ++i) {
                        if (ship.coords[i].first == r && ship.coords[i].second == c) {
                            if (!ship.hits[i]) {
                                ship.hits[i] = true;
                                desk[r][c] = 'X';
                                if (ship.isSunk()) return 2;
                                return 1;
                            }
                            return 0;
                        }
                    }
                }
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
    GamePlayer p1;
    GamePlayer p2; // <-- Исправлено: p2 теперь будет инициализироваться

    Shop shop;

public:
    // Шаг 2: Конструктор игры.
    Game(int startMoney) : p1(startMoney), p2(startMoney) {} // <-- Исправлено: p2 инициализируется


    // Шаг 3: Вспомогательный метод для отрисовки интерфейса.
    void showInterface(GamePlayer& activePlayer, GamePlayer& enemyPlayer, int playerNum, bool isMyTurn) {
        system("cls"); // Очистка экрана для свежего интерфейса
        cout << "\n===============================\n";
        cout << "ХОД ИГРОКА " << playerNum << (isMyTurn ? " (Ваш ход)" : " (Ход противника)") << endl;
        cout << "Деньги в кошельке: " << activePlayer.wallet.getMoney() << endl;

        cout << "\n--- ВАШЕ ПОЛЕ ---" << endl;
        activePlayer.board.display(true);
        cout << "\n--- ПОЛЕ ПРОТИВНИКА ---" << endl;
        enemyPlayer.board.display(false); // Поле противника отображается без его кораблей
        cout << "===============================\n";
    }

    // Вспомогательная функция для парсинга координат "R C"
    pair<int, int> parseCoords(const string& data) {
        size_t space_pos = data.find(' ');
        if (space_pos == string::npos) return { -1, -1 }; // Ошибка формата
        try {
            int r = stoi(data.substr(0, space_pos));
            int c = stoi(data.substr(space_pos + 1));
            return { r, c };
        }
        catch (...) {
            return { -1, -1 }; // Ошибка парсинга
        }
    }

    // Вспомогательная функция для форматирования результата выстрела
    string formatShootResult(int result) {
        switch (result) {
        case 0: return "MISS";
        case 1: return "HIT";
        case 2: return "SUNK";
        case 3: return "ALREADY_SHOT";
        case -1: return "ERROR_SHOT";
        default: return "UNKNOWN_RESULT";
        }
    }

    // Шаг 4: Основной игровой цикл для СЕТЕВОЙ игры.
    void runNetwork(bool isHost) {
        // Установка стартовых денег
        // Расстановка кораблей для игрока на этом ПК
        // isHost = true (Вы Игрок 1), isHost = false (Вы Игрок 2)
        int myPlayerNum = (isHost ? 1 : 2);

        cout << (isHost ? "Вы - Игрок 1. Расставляйте корабли..." : "Вы - Игрок 2. Расставляйте корабли...") << endl;
        if (isHost) {
            p1.board.placeAllShips(1);
        }
        else {
            p2.board.placeAllShips(2);
        }

        // Обмен начальными сообщениями о назначении игрока
        string player_assignment_msg;
        if (isHost) {
            player_assignment_msg = "PLAYER_ASSIGNMENT_2"; // Хост сообщает клиенту, что он (клиент) - игрок 2
        }
        else {
            player_assignment_msg = "PLAYER_ASSIGNMENT_1"; // Клиент сообщает хосту, что он (хост) - игрок 1
        }

        send(Connection, player_assignment_msg.c_str(), (int)player_assignment_msg.length(), 0);

        char confirmation_buffer[256] = { 0 };
        int bytes_received = recv(Connection, confirmation_buffer, sizeof(confirmation_buffer), 0);
        if (bytes_received <= 0) {
            cout << "Соединение разорвано или ошибка получения подтверждения назначения игрока." << endl;
            return;
        }
        cout << "Получено подтверждение назначения игрока: " << confirmation_buffer << endl;

        system("cls");

        bool gameOver = false;
        bool myTurn = isHost; // Хост ходит первым (Игрок 1)

        int lastShotR = -1; // Для отслеживания последнего выстрела игрока на этом ПК
        int lastShotC = -1;
        bool hasLastShot = false;

        while (!gameOver) {
            GamePlayer& myPlayer = (isHost ? p1 : p2);
            GamePlayer& opponentPlayer = (isHost ? p2 : p1); // Поле противника для отображения

            showInterface(myPlayer, opponentPlayer, myPlayerNum, myTurn);

            if (myTurn) {
                // МОЙ ХОД
                cout << "\nВАШ ХОД!" << endl;
                cout << "Зайти в магазин? (1 - да, 0 - нет): ";
                int goShop;
                if (!(cin >> goShop)) {
                    cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    goShop = 0; // Считаем, что не зашли в магазин при ошибке ввода
                }
                if (goShop == 1) {
                    shop.open(myPlayer, myPlayerNum);
                    showInterface(myPlayer, opponentPlayer, myPlayerNum, myTurn); // Обновить после магазина
                }

                int r, c;
                bool validShot = false;
                while (!validShot) {
                    cout << "\nВведите координаты для выстрела (строка и столбец): ";
                    if (!(cin >> r >> c)) {
                        cin.clear();
                        cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                        cout << "Ошибка ввода! Введите числа." << endl;
                        continue;
                    }
                    if (r < 1 || r > myPlayer.board.getsize() || c < 1 || c > myPlayer.board.getsize()) {
                        cout << "Координаты за пределами поля. Попробуйте снова." << endl;
                        continue;
                    }

                    if (hasLastShot && (r - 1) == lastShotR && (c - 1) == lastShotC) {
                        cout << "В эту клетку нельзя стрелять два хода подряд. Выберите другую.\n";
                        continue;
                    }
                    validShot = true;
                }

                // Отправляем выстрел противнику
                string shot_msg = to_string(r) + " " + to_string(c);
                send(Connection, shot_msg.c_str(), (int)shot_msg.length(), 0);
                cout << "Выстрел отправлен в " << r << " " << c << ". Ожидаем результат..." << endl;

                // Ждем результат выстрела от противника
                memset(confirmation_buffer, 0, sizeof(confirmation_buffer));
                bytes_received = recv(Connection, confirmation_buffer, sizeof(confirmation_buffer), 0);
                if (bytes_received <= 0) {
                    cout << "Соединение разорвано или ошибка получения результата." << endl;
                    gameOver = true;
                    break;
                }
                string result_str = string(confirmation_buffer, bytes_received);

                int shoot_result = -1;
                if (result_str == "MISS") { shoot_result = 0; }
                else if (result_str == "HIT") { shoot_result = 1; }
                else if (result_str == "SUNK") { shoot_result = 2; }
                else if (result_str == "ALREADY_SHOT") { shoot_result = 3; }
                else if (result_str == "GAME_OVER_WIN") { // Противник сообщил о нашей победе
                    cout << "ПОЗДРАВЛЯЕМ! Вы победили!" << endl;
                    gameOver = true;
                    break;
                }
                else { cout << "Неизвестный результат выстрела: " << result_str << endl; }

                // Обновление отображения поля противника (это не его реальная доска, а только наше представление)
                if (shoot_result == 1 || shoot_result == 2) {
                    // Можно добавить логику обновления скрытой доски противника у себя
                }
                else if (shoot_result == 0) {
                    // Можно добавить логику обновления скрытой доски противника у себя
                }

                switch (shoot_result) {
                case 0:
                    cout << "Мимо!" << endl;
                    myTurn = false; // Смена хода
                    break;
                case 1:
                    cout << "ПОПАДАНИЕ!" << endl;
                    break; // Ход не меняется
                case 2:
                    cout << "УБИТ! Корабль противника пошел ко дну!" << endl;
                    myPlayer.wallet.addMoney(50);
                    break;
                case 3:
                    cout << "Вы уже стреляли в эту клетку!" << endl;
                    break;
                case -1:
                    cout << "Ошибка при выстреле." << endl;
                    myTurn = false;
                    break;
                }

                lastShotR = r - 1;
                lastShotC = c - 1;
                hasLastShot = true;

                // Если противник сообщил GAME_OVER_WIN, то gameOver уже true
                // Если нет, то мы не можем проверить allShipsSunk() на opponentPlayer.board,
                // потому что это наша локальная копия, а не реальное состояние его доски.
                // Только противник может сообщить о своей гибели.

            }
            else {
                // ХОД ПРОТИВНИКА
                cout << "\nХОД ПРОТИВНИКА. Ожидаем выстрел..." << endl;
                char shot_buffer[256] = { 0 };
                bytes_received = recv(Connection, shot_buffer, sizeof(shot_buffer), 0);
                if (bytes_received <= 0) {
                    cout << "Соединение разорвано или ошибка получения выстрела." << endl;
                    gameOver = true;
                    break;
                }
                string shot_str = string(shot_buffer, bytes_received);
                pair<int, int> coords = parseCoords(shot_str);

                if (coords.first == -1) {
                    cout << "Ошибка парсинга координат от противника: " << shot_str << endl;
                    string error_response = "ERROR_PARSING_SHOT";
                    send(Connection, error_response.c_str(), (int)error_response.length(), 0);
                    myTurn = true; // Смена хода на всякий случай
                    continue;
                }

                int r = coords.first;
                int c = coords.second;

                cout << "Противник выстрелил в: " << r << " " << c << endl;

                // Обрабатываем выстрел на своей доске
                int result = myPlayer.board.shootAt(r - 1, c - 1);
                string response_str = formatShootResult(result);

                // Проверка, все ли мои корабли потоплены ПОСЛЕ выстрела противника
                if (myPlayer.board.allShipsSunk()) {
                    cout << "Все ваши корабли потоплены! Вы проиграли." << endl;
                    response_str = "GAME_OVER_WIN"; // Отправляем противнику, что он победил
                    gameOver = true;
                }

                // Отправляем результат обратно противнику
                send(Connection, response_str.c_str(), (int)response_str.length(), 0);
                cout << "Отправили результат: " << response_str << endl;

                // Если противник промахнулся, ход переходит ко мне
                if (result == 0 || result == -1 || result == 3) { // Промах, ошибка или уже стрелял
                    myTurn = true; // Мой ход
                }
                else { // Попадание или убийство
                    // Ход не меняется, противник стреляет снова
                }
            }

            cout << "\nНажмите Enter, чтобы продолжить...";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cin.get();
        }
    }

    // Шаг 4: Исходный игровой цикл (для локальной игры, если захочешь)
    void runLocal() {
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
            GamePlayer& attacker = (currentPlayer == 1 ? p1 : p2);
            GamePlayer& defender = (currentPlayer == 1 ? p2 : p1);

            showInterface(attacker, defender, currentPlayer, true);

            cout << "\nЗайти в магазин? (1 - да, 0 - нет): ";
            int goShop;
            if (!(cin >> goShop)) {
                cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
                goShop = 0;
            }
            if (goShop == 1) {
                shop.open(attacker, currentPlayer);
            }

            int r, c;
            cout << "\nВведите координаты для выстрела (строка и столбец): ";
            if (!(cin >> r >> c)) {
                cin.clear();
                cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                cout << "Ошибка ввода! Введите числа." << endl;
                continue;
            }

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

            switch (result) {
            case -1:
                cout << "Промах! Вы выстрелили за пределы поля." << endl;
                currentPlayer = (currentPlayer == 1 ? 2 : 1);
                break;
            case 0:
                cout << "Мимо!" << endl;
                currentPlayer = (currentPlayer == 1 ? 2 : 1);
                break;
            case 1:
                cout << "ПОПАДАНИЕ!" << endl;
                break;
            case 2:
                cout << "УБИТ! Корабль противника пошел ко дну!" << endl;
                attacker.wallet.addMoney(50);
                break;
            case 3:
                cout << "Вы уже стреляли в эту клетку!" << endl;
                break;
            }

            if (defender.board.allShipsSunk()) {
                cout << "\nПОЗДРАВЛЯЕМ! Игрок " << currentPlayer << " победил!" << endl;
                gameOver = true;
            }

            cout << "\nНажмите Enter, чтобы продолжить...";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cin.get();
            system("cls");
        }
    }
};

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "Ru");
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    WSAData wsaData;
    WORD DLLVersion = MAKEWORD(2, 2); // Обычно 2.2 для современных WinSock
    if (WSAStartup(DLLVersion, &wsaData) != 0) {
        cout << "Error: WSAStartup failed!" << endl;
        exit(1);
    }

    int choice;
    cout << "Выберите режим игры:\n";
    cout << "1. Создать игру (Host)\n";
    cout << "2. Присоединиться к игре (Client)\n";
    cout << "Ваш выбор: ";
    cin >> choice;

    if (choice == 1) { // РЕЖИМ ХОСТА (Игрок 1)
        cout << "Режим Хоста выбран. Вы Игрок 1." << endl;

        // --- КОД СЕРВЕРА ИЗ ВАШЕГО ПРЕДОСТАВЛЕННОГО ФРАГМЕНТА (АДАПТИРОВАН) ---
        SOCKADDR_IN addr;
        int sizeofaddr_val = sizeof(addr); // Переименовал, чтобы не конфликтовало с глобальным sizeofaddr
        addr.sin_addr.s_addr = INADDR_ANY; // Слушаем на всех доступных IP-адресах
        addr.sin_port = htons(1111); // Порт
        addr.sin_family = AF_INET;

        SOCKET sListen = socket(AF_INET, SOCK_STREAM, 0); // Используем 0 вместо NULL
        if (sListen == INVALID_SOCKET) {
            cout << "Error: Failed to create listen socket! " << WSAGetLastError() << endl;
            WSACleanup();
            system("pause");
            return 1;
        }

        if (bind(sListen, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            cout << "Error: Failed to bind socket! " << WSAGetLastError() << endl;
            closesocket(sListen);
            WSACleanup();
            system("pause");
            return 1;
        }

        if (listen(sListen, SOMAXCONN) == SOCKET_ERROR) {
            cout << "Error: Failed to listen on socket! " << WSAGetLastError() << endl;
            closesocket(sListen);
            WSACleanup();
            system("pause");
            return 1;
        }

        cout << "Сервер запущен. Ожидание подключения Клиента (Игрока 2) на порту 1111..." << endl;

        // --- АДАПТАЦИЯ: ACCEPT ОДНОГО КЛИЕНТА В ГЛОБАЛЬНЫЙ Connection ---
        Connection = accept(sListen, (SOCKADDR*)&addr, &sizeofaddr_val); // Принимаем ТОЛЬКО ОДНО подключение
        if (Connection == INVALID_SOCKET) {
            cout << "Error: Failed to accept client connection! " << WSAGetLastError() << endl;
            closesocket(sListen);
            WSACleanup();
            system("pause");
            return 1;
        }
        cout << "Клиент (Игрок 2) подключился! Начинаем игру." << endl;

        // Закрываем слушающий сокет, так как нам нужно только одно подключение
        closesocket(sListen);

        // В этом режиме ClientHandler не запускается, т.к. Game::runNetwork управляет напрямую Connection
        // (Иначе ClientHandler будет конкурировать за сообщения или ретранслировать их неправильно для 1-на-1 игры)

        // Запуск сетевой игры
        Game game(500);
        game.runNetwork(true); // isHost = true для Хоста

    }
    else if (choice == 2) { // РЕЖИМ КЛИЕНТА (Игрок 2)
        cout << "Режим Клиента выбран. Вы Игрок 2." << endl;
        string server_ip;
        cout << "Введите IP-адрес сервера (например, 5.3.175.81 или 127.0.0.1): ";
        cin >> server_ip;

        SOCKADDR_IN addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(1111); // Порт сервера

        if (inet_pton(AF_INET, server_ip.c_str(), &addr.sin_addr) <= 0) {
            cout << "Ошибка: Неправильный формат IP-адреса." << endl;
            WSACleanup();
            system("pause");
            return 1;
        }

        Connection = socket(AF_INET, SOCK_STREAM, 0); // Используем 0 вместо NULL
        if (Connection == INVALID_SOCKET) {
            cout << "Error: Failed to create socket! " << WSAGetLastError() << endl;
            WSACleanup();
            system("pause");
            return 1;
        }

        cout << "Подключение к Хосту " << server_ip << ":" << 1111 << "..." << endl;
        if (connect(Connection, (SOCKADDR*)&addr, sizeof(addr)) != 0) {
            cout << "Error: Failed to connect to Host! " << WSAGetLastError() << endl;
            closesocket(Connection);
            WSACleanup();
            system("pause");
            return 1;
        }

        cout << "Успешно подключено к Хосту (Игроку 1)!" << endl;

        // В этом режиме ClientHandler также не запускается, т.к. Game::runNetwork управляет напрямую Connection

        // Запуск сетевой игры
        Game game(500);
        game.runNetwork(false); // isHost = false для Клиента

    }
    else {
        cout << "Неверный выбор. Завершение программы." << endl;
        WSACleanup();
        system("pause");
        return 1;
    }

    // Закрываем сокет и деинициализируем WinSock в конце
    if (Connection != INVALID_SOCKET) {
        closesocket(Connection);
    }
    WSACleanup();
    system("pause");
    return 0;
}
