#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <conio.h>
#include <vector>
#include <string>
#include <windows.h>
#include <sstream>

// 可以通过更改这里的字符，以及ANSI转义码的颜色设置来改变游戏中不同字符显示

namespace Words  // 可换字符网站：https://m.weixinbiaoqing.com/
{
constexpr const char *Floor     = " ";
constexpr const char *Wall      = "\033[1;33m▨\033[0m";
constexpr const char *SnakeHead = "\033[1;34m❑\033[0m";
constexpr const char *SnakeBody = "\033[1;32m❑\033[0m";
constexpr const char *SnakeTail = "\033[38;5;208m❑\033[0m";
constexpr const char *Food      = "\033[1;35m♡\033[0m";
}  // namespace Words

namespace ColorlessWords  // 无颜色字符，在换字符的时候上面下面都换比较好
{
constexpr const char *Floor     = " ";
constexpr const char *Wall      = "▨";
constexpr const char *SnakeHead = "❑";
constexpr const char *SnakeBody = "❑";
constexpr const char *SnakeTail = "❑";
constexpr const char *Food      = "♡";
}  // namespace ColorlessWords

//=======================================================================
// 随机数生成器

int GetRandomNumber(int min, int max)
{
    static std::random_device rd;   // 获取真随机数种子
    static std::mt19937 gen(rd());  // 使用Mersenne Twister引擎
    std::uniform_int_distribution<> distrib(min, max);
    return distrib(gen);
}

//=======================================================================
// 获取终端大小

void GetTerminalSize(int &width, int &height)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

    width  = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

//======================================================================= 坐标

struct Position
{
    int m_X, m_Y;
    Position() = delete;
    Position(int X, int Y) : m_X(X), m_Y(Y) {}
    Position(const Position &)            = default;
    Position &operator=(const Position &) = default;

    bool operator==(const Position &other) const
    {
        return m_X == other.m_X && m_Y == other.m_Y;
    }
};

//=======================================================================
// Screen类

class Screen
{
   private:
    // 地图大小
    int m_Width, m_Height;
    // 字符缓冲区
    std::string m_Buffer;
    // Map是储存最新地图状态，BackMap是储存上一刻地图状态喵
    std::vector<int> m_Map;
    std::vector<int> m_BackMap;

    // 添加一个私有的辅助函数喵
    int GetIndex(Position pos) const
    {
        return pos.m_Y * m_Width + pos.m_X;  // 只在这里写一次喵
    }

   public:
    Screen(int width, int height)
        : m_Width(width),
          m_Height(height),
          m_Buffer(""),
          m_Map(m_Width * m_Height, 0),
          m_BackMap(m_Width * m_Height, 0)
    {
        // 设置控制台UTF-8编码喵
        SetConsoleOutputCP(65001);

        // 隐藏光标喵（ANSI方式）
        std::cout << "\033[?25l";

        // 初始清屏喵
        std::cout << "\033[2J\033[H";
        std::cout.flush();
    }
    ~Screen()
    {
        std::cout << "\033[2J\033[H\033[?25h\033[0m";
        std::cout.flush();
    }

    void SetBuffer(std::string &NewBuffer) { m_Buffer = NewBuffer; }

    // 清空输出字符缓冲区喵
    void Clear()
    {
        m_Buffer.clear();
        m_Buffer += "\033[H";  // 光标回到开头喵
    }

    // 清空当前地图喵
    void ClearMap() { std::fill(m_Map.begin(), m_Map.end(), 0); }

    // 获取当前地图某位置的类型喵
    int GetCurPixel(Position pos) const
    {
        int index = GetIndex(pos);
        if (index >= 0 && index < m_Map.size())
        {
            return m_Map[index];
        }
        return -1;  // 错误值喵
    }

    // 获取上一刻地图某位置的类型喵
    int GetBackPixel(Position pos) const
    {
        int index = GetIndex(pos);
        if (index >= 0 && index < m_Map.size())
        {
            return m_BackMap[index];
        }
        return -1;  // 错误值喵
    }

    // 设置地图某位置的类型为
    void SetPixel(Position pos, int type)
    {
        // 检查边界喵
        if (pos.m_X < 0 || pos.m_X >= m_Width || pos.m_Y < 0 ||
            pos.m_Y >= m_Height)
        {
            return;  // 超出范围就不设置喵
        }

        int index    = GetIndex(pos);
        m_Map[index] = type;
    }

    // 把需要渲染的元素写入字符缓冲区，包括位置和类型
    void DrawWord(Position pos, int type)
    {
        // 构建移动光标的ANSI码
        // 注意：ANSI码的坐标是从1开始的喵，不是0喵！
        int ansi_x = pos.m_X * 2 + 1;
        int ansi_y = pos.m_Y + 1;

        m_Buffer += "\033[" + std::to_string(ansi_y) + ';' +
                    std::to_string(ansi_x) + 'H';
        switch (type)
        {
            case Type::Floor:  // 空地
                m_Buffer += Words::Floor;
                break;
            case Type::Wall:  // 墙壁
                m_Buffer += Words::Wall;
                break;
            case Type::Food:  // 食物
                m_Buffer += Words::Food;
                break;
            case Type::SnakeHead:  // 蛇头
                m_Buffer += Words::SnakeHead;
                break;
            case Type::SnakeBody:  // 蛇身
                m_Buffer += Words::SnakeBody;
                break;
            case Type::SnakeTail:  // 蛇尾
                m_Buffer += Words::SnakeTail;
                break;
            default:  // 错误
                m_Buffer += "??";
        }
    }

    // 输入要打印的文本内容到字符缓冲区，包括位置和文本内容
    void DrawText(Position pos, const std::string &text)
    {
        m_Buffer += "\033[" + std::to_string(pos.m_Y) + ';' +
                    std::to_string(pos.m_X) + 'H';
        m_Buffer += text;
    }

    // 强制输出字符缓冲区中所有内容
    void Flush() const
    {
        std::cout << m_Buffer;
        std::cout.flush();
    }

    // 渲染地图内容到终端
    void Render()
    {
        Clear();

        for (int y = 0; y < m_Height; y++)
        {
            for (int x = 0; x < m_Width; x++)
            {
                int Index = y * m_Width + x;
                if (m_Map[Index] != m_BackMap[Index])
                {
                    DrawWord({x, y}, m_Map[Index]);
                }
            }
        }

        m_BackMap = m_Map;
        Flush();
    }

    // 获取地图宽度
    int GetWidth() const { return m_Width; }

    // 获取地图高度
    int GetHeight() const { return m_Height; }

   public:
    // Screen类渲染时的类型
    enum Type : int
    {
        Floor     = 0,
        Wall      = 1,
        SnakeHead = 2,
        SnakeBody = 3,
        SnakeTail = 4,
        Food      = 5
    };
};

//======================================================================= 蛇类

class Snake
{
   private:
    std::vector<Position> m_Body;

    bool m_ShouldGrow;

   public:
    enum class Direction
    {
        none  = 0,
        up    = 1,
        down  = 2,
        left  = 3,
        right = 4
    };
    Direction m_CurrentDirection;
    Direction m_NextDirection;
    // 在Snake类的public部分添加喵
    Direction m_BufferedDirection;

   public:
    Snake(std::vector<Position> InitialSnake)
        : m_Body(InitialSnake),
          m_ShouldGrow(false),
          m_CurrentDirection(Direction::none),
          m_NextDirection(Direction::none),
          m_BufferedDirection(Direction::none)
    {
    }
    ~Snake() {}

    // 获取某节身体的位置 0索引就是头部喵
    Position GetBodPosition(int i) const { return m_Body[i]; }

    // 获取头部的位置喵
    Position GetHeadPosition() const { return m_Body[0]; }

    // 获取蛇数组喵
    const std::vector<Position> &GetBody() const { return m_Body; }

    // 获取蛇的长度喵
    int GetLength() const { return m_Body.size(); }

    // 设置蛇应该变长了喵
    void Grow() { m_ShouldGrow = true; }

    // 设置蛇头的位置喵
    void SetHeadPosition(Position NewHead) { m_Body[0] = NewHead; }

    // 蛇头朝运动方向运动喵
    Position MoveHead() const
    {
        Position NewHead = m_Body[0];

        switch (m_CurrentDirection)
        {
            case Direction::none:
                break;
            case Direction::up:
                NewHead.m_Y--;
                break;
            case Direction::down:
                NewHead.m_Y++;
                break;
            case Direction::left:
                NewHead.m_X--;
                break;
            case Direction::right:
                NewHead.m_X++;
                break;
        }

        return NewHead;
    }

    // 传入蛇头移动到的位置，移动整条蛇喵
    void Move(Position NewHead)
    {
        // 更新当前方向喵
        m_CurrentDirection = m_NextDirection;

        // ⭐ 处理缓存
        if (m_BufferedDirection != Direction::none)
        {
            m_NextDirection     = m_BufferedDirection;
            m_BufferedDirection = Direction::none;
        }

        m_Body.insert(m_Body.begin(), NewHead);

        if (m_ShouldGrow)
        {
            m_ShouldGrow = false;
        }
        else
        {
            m_Body.pop_back();
        }
    }

    // 方向检测函数喵
    bool IsOpposite(Direction dir1, Direction dir2) const
    {
        return (dir1 == Direction::up && dir2 == Direction::down) ||
               (dir1 == Direction::down && dir2 == Direction::up) ||
               (dir1 == Direction::left && dir2 == Direction::right) ||
               (dir1 == Direction::right && dir2 == Direction::left);
    }

    // 改变移动方向喵
    void ChangeDirection(Direction NewDir)
    {
        // 检测输入方向和运动方向是否相反
        if (IsOpposite(NewDir, m_CurrentDirection)) return;

        // ⭐ 如果NextDirection还是当前方向，直接设置喵
        if (m_NextDirection == m_CurrentDirection)
        {
            m_NextDirection = NewDir;
        }
        else
        {
            // ⭐ 检测是否和NextDirection相反喵
            if (IsOpposite(NewDir, m_NextDirection)) return;

            // ⭐ 缓存方向喵
            m_BufferedDirection = NewDir;
        }
    }
};

//======================================================================= 食物类

class Food
{
   private:
    Position m_FoodPosition;

   public:
    Food(Position StartPos) : m_FoodPosition(StartPos) {}
    ~Food() {}

    // 设置食物位置喵
    void SetFoodPosition(Position NewFoodPosition)
    {
        m_FoodPosition = NewFoodPosition;
    }

    // 获取食物位置
    Position GetFoodPosition() { return m_FoodPosition; }
};

//======================================================================= 游戏类

class Game
{
   public:
    // ⭐ 游戏结束状态枚举喵
    enum class GameOverReason
    {
        None = 0,        // 游戏还在进行喵
        HitSelf,         // 撞到自己了喵
        HitTail,         // 咬到尾巴了喵
        AllFoodEaten,    // 吃完所有食物（胜利）喵
        PlayerQuit,      // 玩家主动退出喵
        UnexpectedError  // 出BUG了喵
    };

   private:
    Screen m_Screen;
    Snake m_Snake;
    Food m_Food;

    int m_Score;
    int m_Speed;
    bool m_IsGameOver;
    GameOverReason m_GameOverReason;
    bool m_IsPaused;
    bool m_IfNeedDrawFood;

    // 添加时间控制
    std::chrono::steady_clock::time_point m_LastMoveTime;

   public:
    Game(int MapWidth, int MapHeight, int Speed)
        : m_Screen(MapWidth, MapHeight),
          m_Snake({{(MapWidth / 2), (MapHeight / 2)}}),
          m_Food({0, 0}),
          m_Score(0),
          m_Speed(Speed),
          m_IsGameOver(false),
          m_GameOverReason(GameOverReason::None),
          m_IsPaused(false),
          m_IfNeedDrawFood(true)
    {
        DrawSnakeHead();
        DrawSnakeBody();
        DrawWalls();
        DrawFood();
    }
    ~Game() {}

    // 游戏主循环
    void Run()
    {
        while (!m_IsGameOver)
        {
            Input();

            if (!m_IsPaused)
            {
                if (ShouldMove())
                {
                    Update();
                    UpdateScreen();
                }
            }

            // 极短延迟，保证输入响应
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        // 游戏结束
        GameOver();
    }

    // 判断是否应该移动
    bool ShouldMove()
    {
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           currentTime - m_LastMoveTime)
                           .count();

        if (elapsed >= m_Speed)
        {
            m_LastMoveTime = currentTime;
            return true;
        }
        return false;
    }

    // 输入检测喵
    void Input()
    {
        while (_kbhit())
        {
            char key = _getch();

            Snake::Direction InputDirection = Snake::Direction::none;

            switch (key)
            {
                case 'w':
                case 'W':
                case 72:  // 72是方向键上
                    if (m_IsPaused == false)
                        InputDirection = Snake::Direction::up;
                    break;
                case 's':
                case 'S':
                case 80:  // 方向键下
                    if (m_IsPaused == false)
                        InputDirection = Snake::Direction::down;
                    break;
                case 'a':
                case 'A':
                case 75:  // 方向键左
                    if (m_IsPaused == false)
                        InputDirection = Snake::Direction::left;
                    break;
                case 'd':
                case 'D':
                case 77:  // 方向键右
                    if (m_IsPaused == false)
                        InputDirection = Snake::Direction::right;
                    break;
                case 32:  // 空格键
                    if (m_IsPaused == false)
                        m_IsPaused = true;
                    else
                        m_IsPaused = false;
                    break;
                case 27:  // ESC键
                    m_IsGameOver     = true;
                    m_GameOverReason = GameOverReason::PlayerQuit;
                    break;
                default:
                    break;
            }
            if (InputDirection != Snake::Direction::none)
                m_Snake.ChangeDirection(InputDirection);
        }
    }

    // 更新地图和处理逻辑喵
    void Update()
    {
        // 根据游戏得分情况变换难度（即变换蛇运动速度）
        // 游戏速度刷新
        static int originalSpeed = m_Speed;  // 或使用成员变量保存初始速度喵
        if (m_Score >= 50)
            m_Speed = (int)(originalSpeed * 0.6);
        else if (m_Score >= 40)
            m_Speed = (int)(originalSpeed * 0.7);
        else if (m_Score >= 30)
            m_Speed = (int)(originalSpeed * 0.75);
        else if (m_Score >= 20)
            m_Speed = (int)(originalSpeed * 0.8);
        else if (m_Score >= 10)
            m_Speed = (int)(originalSpeed * 0.9);

        // 移动蛇喵
        m_Snake.Move(m_Snake.MoveHead());

        // 更新地图状态喵
        m_Screen.ClearMap();
        DrawSnakeHead();
        DrawSnakeBody();
        DrawWalls();
        DrawFood();

        // 碰撞检测喵
        CollisionDetection();
    }

    // 完成更新屏幕显示工作喵
    void UpdateScreen()
    {
        m_Screen.Render();
        if (!m_IsGameOver)
        {
            std::cout << "\033[" << (m_Screen.GetHeight() + 1)
                      << ";1HScore: \033[1;35m" << m_Score << " "
                      << ColorlessWords::Food << " \033[0m";
        }
    }

    // 绘制墙喵
    void DrawWalls()
    {
        for (int x = 0; x < m_Screen.GetWidth(); x++)
        {
            m_Screen.SetPixel({x, 0}, m_Screen.Type::Wall);
            m_Screen.SetPixel({x, m_Screen.GetHeight() - 1},
                              m_Screen.Type::Wall);
        }
        for (int y = 0; y < m_Screen.GetHeight(); y++)
        {
            m_Screen.SetPixel(Position(0, y), m_Screen.Type::Wall);
            m_Screen.SetPixel(Position(m_Screen.GetWidth() - 1, y),
                              m_Screen.Type::Wall);
        }
    }

    // 绘制蛇头喵
    void DrawSnakeHead()
    {
        m_Screen.SetPixel(m_Snake.GetHeadPosition(), m_Screen.Type::SnakeHead);
    }

    // 绘制蛇身体喵
    void DrawSnakeBody()
    {
        for (int i = 1; i < m_Snake.GetBody().size(); i++)
            m_Screen.SetPixel(m_Snake.GetBodPosition(i),
                              m_Screen.Type::SnakeBody);
        if (m_Snake.GetBody().size() > 1)
            m_Screen.SetPixel(m_Snake.GetBody().back(),
                              m_Screen.Type::SnakeTail);
    }

    // 绘制食物喵
    void DrawFood()
    {
        if (m_IfNeedDrawFood)
        {
            m_Food.SetFoodPosition(SetRandomPos());
            m_Screen.SetPixel(m_Food.GetFoodPosition(), m_Screen.Type::Food);
            m_IfNeedDrawFood = false;
        }
        else
        {
            m_Screen.SetPixel(m_Food.GetFoodPosition(), m_Screen.Type::Food);
        }
    }

    // 获取一个空的随机位置喵
    Position SetRandomPos()
    {
        Position Pos = {0, 0};
        do
        {
            Pos = {GetRandomNumber(0, m_Screen.GetWidth() - 1),
                   GetRandomNumber(0, m_Screen.GetHeight() - 1)};
        } while (m_Screen.GetCurPixel(Pos));
        return Pos;
    }

    // 碰撞检测
    void CollisionDetection()
    {
        bool NeedRecheck      = true;
        int RecheckCount      = 0;
        const int MAX_RECHECK = 5;

        while (NeedRecheck && RecheckCount < MAX_RECHECK)
        {
            NeedRecheck = false;
            RecheckCount++;

            // 碰撞检测
            int Situation    = m_Screen.GetCurPixel(m_Snake.GetHeadPosition());
            Position CurHead = m_Snake.GetHeadPosition();
            bool IsChange    = false;

            switch (Situation)
            {
                case m_Screen.Wall:  // 即碰到墙壁
                    if (m_Snake.GetHeadPosition().m_X == 0)
                        m_Snake.SetHeadPosition(
                            {m_Screen.GetWidth() - 2, CurHead.m_Y});
                    if (m_Snake.GetHeadPosition().m_X ==
                        m_Screen.GetWidth() - 1)
                        m_Snake.SetHeadPosition({1, CurHead.m_Y});
                    if (m_Snake.GetHeadPosition().m_Y == 0)
                        m_Snake.SetHeadPosition(
                            {CurHead.m_X, m_Screen.GetHeight() - 2});
                    if (m_Snake.GetHeadPosition().m_Y ==
                        m_Screen.GetHeight() - 1)
                        m_Snake.SetHeadPosition({CurHead.m_X, 1});
                    IsChange    = true;
                    NeedRecheck = true;  // ⭐ 传送后需要重新检测喵
                    break;
                case m_Screen.Food:
                    m_Score++;
                    if (m_Score == ((m_Screen.GetWidth() - 2) *
                                        (m_Screen.GetHeight() - 2) -
                                    1))
                    {
                        m_IsGameOver     = true;
                        m_GameOverReason = GameOverReason::AllFoodEaten;
                        break;
                    }
                    m_Snake.Grow();
                    IsChange         = true;
                    m_IfNeedDrawFood = true;
                    break;
                case m_Screen.SnakeBody:
                    m_IsGameOver     = true;
                    m_GameOverReason = GameOverReason::HitSelf;
                    break;
                case m_Screen.SnakeTail:
                    m_IsGameOver     = true;
                    m_GameOverReason = GameOverReason::HitTail;
                    break;
                case m_Screen.Floor:
                default:
                    break;
            }

            if (IsChange)  // 为了理论上的可扩展性所以这里是把地图状态全部更新了，但目前其实可以做的更优化
            {
                // 更新地图状态
                m_Screen.ClearMap();
                DrawSnakeHead();
                DrawSnakeBody();
                DrawWalls();
                DrawFood();
            }
        }

        // ⭐ 如果重新检测次数超过上限，说明出bug了喵
        if (RecheckCount >= MAX_RECHECK)
        {
            // 可以输出错误信息或者直接结束游戏喵
            m_IsGameOver     = true;
            m_GameOverReason = GameOverReason::UnexpectedError;
        }
    }

    void GameOver()
    {
        // 先清屏喵
        std::cout << "\033[2J\033[H";

        // 根据胜利或失败选择颜色喵
        std::string color = IsVictory() ? "\033[1;33m" : "\033[1;31m";
        std::string title = IsVictory() ? "You Win!!! 喵" : "Game Over! 喵";

        // 如果是玩家主动退出，用特殊颜色喵
        if (m_GameOverReason == GameOverReason::PlayerQuit)
        {
            color = "\033[1;36m";
            title = "再见喵~ (=^･ω･^=) ";  // ⭐ 加了一个空格喵
        }

        int centerY = (m_Screen.GetHeight() / 2) - 1;
        int centerX = m_Screen.GetWidth() - 10;

        // ⭐ 固定的内容宽度喵
        const int contentWidth = 18;

        // 顶部边框喵
        std::cout << "\033[" << centerY << ";" << centerX << "H";
        std::cout << color << "╔════════════════════╗\033[0m\n";

        // 标题行喵
        std::cout << "\033[" << (centerY + 1) << ";" << centerX << "H";
        std::cout << color << "║ " << PadString(title, contentWidth)
                  << " ║\033[0m\n";

        // 原因行喵
        std::cout << "\033[" << (centerY + 2) << ";" << centerX << "H";
        std::cout << color << "║ "
                  << PadString(GetGameOverReasonText(), contentWidth)
                  << " ║\033[0m\n";

        // 分数行喵（紫色）
        std::cout << "\033[" << (centerY + 3) << ";" << centerX << "H";

        std::stringstream ss;
        ss << " Score: " << m_Score << ' ' << ColorlessWords::Food << " .";
        std::string scoreLine = ss.str();

        // ⭐ 边框用color，内容用紫色喵
        std::cout << color << "║ \033[1;35m"
                  << PadString(scoreLine, contentWidth) << "\033[0m" << color
                  << "  ║\033[0m\n";

        // 底部边框喵
        std::cout << "\033[" << (centerY + 4) << ";" << centerX << "H";
        std::cout << color << "╚════════════════════╝\033[0m\n";

        std::cout << "\033[" << (m_Screen.GetHeight() + 2) << ";1H";
        _getch();
    }

    int GetScore() const { return m_Score; }

   private:
    // ⭐ 获取结束原因的描述喵
    std::string GetGameOverReasonText() const
    {
        switch (m_GameOverReason)
        {
            case GameOverReason::HitSelf:
                return "撞到自己了 喵...";
            case GameOverReason::HitTail:
                return "咬到尾巴了 喵...";
            case GameOverReason::AllFoodEaten:
                return "吃完所有食物了 喵！";
            case GameOverReason::PlayerQuit:
                return "下次再玩  喵...";
            case GameOverReason::UnexpectedError:
                return "出现异常了 喵...";
            default:
                return "游戏结束  喵";
        }
    }

    // ⭐ 判断是否是胜利喵
    bool IsVictory() const
    {
        return m_GameOverReason == GameOverReason::AllFoodEaten;
    }

   private:
    // ⭐ 计算字符串的显示宽度喵
    int GetDisplayWidth(const std::string &str) const
    {
        int width = 0;
        for (size_t i = 0; i < str.length();)
        {
            unsigned char c = str[i];

            // 判断是否是中文字符（UTF-8编码）喵
            if (c >= 0x80)  // 非ASCII字符
            {
                width += 2;  // 中文字符占2个显示宽度喵

                // 跳过UTF-8的后续字节喵
                if ((c & 0xE0) == 0xC0)  // 2字节字符
                    i += 2;
                else if ((c & 0xF0) == 0xE0)  // 3字节字符
                    i += 3;
                else if ((c & 0xF8) == 0xF0)  // 4字节字符
                    i += 4;
                else
                    i += 1;
            }
            else
            {
                width += 1;  // ASCII字符占1个显示宽度喵
                i += 1;
            }
        }
        return width;
    }

    // ⭐ 填充空格到指定宽度喵
    std::string PadString(const std::string &str, int targetWidth) const
    {
        int currentWidth  = GetDisplayWidth(str);
        int paddingNeeded = targetWidth - currentWidth;

        if (paddingNeeded <= 0) return str;

        return str + std::string(paddingNeeded, ' ');
    }

   public:
    // ⭐ 获取游戏结束原因（用于调试或日志）喵
    GameOverReason GetGameOverReason() const { return m_GameOverReason; }

    // ⭐ 输出调试信息喵
    void PrintDebugInfo() const
    {
        std::cout << "\n=== Debug Info ===\n";
        std::cout << "Game Over: " << (m_IsGameOver ? "Yes" : "No") << "\n";
        std::cout << "Reason: ";

        switch (m_GameOverReason)
        {
            case GameOverReason::None:
                std::cout << "None (Still Playing)\n";
                break;
            case GameOverReason::HitSelf:
                std::cout << "Hit Self\n";
                break;
            case GameOverReason::HitTail:
                std::cout << "Hit Tail\n";
                break;
            case GameOverReason::AllFoodEaten:
                std::cout << "All Food Eaten (Victory!)\n";
                break;
            case GameOverReason::PlayerQuit:
                std::cout << "Player Quit\n";
                break;
            case GameOverReason::UnexpectedError:
                std::cout << "Unexpected Error\n";
                break;
            default:
                std::cout << "Unknown\n";
                break;
        }

        std::cout << "Score: " << m_Score << "\n";
        std::cout << "Snake Length: " << m_Snake.GetBody().size() << "\n";
        std::cout << "==================\n";
    }
};

//======================================================================= 主函数

int main()
{
    int width, height;
    GetTerminalSize(width, height);

    Game game((width / 2), height - 1, 160);
    game.Run();
}
