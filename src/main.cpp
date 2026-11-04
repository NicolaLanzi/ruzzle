#include "raylib.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <random>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

constexpr int SCREEN_WIDTH = 1000;
constexpr int SCREEN_HEIGHT = 720;

constexpr int GRID_SIZE = 4;
constexpr float CELL_SIZE = 96.0f;
constexpr float CELL_GAP = 10.0f;
constexpr float CELL_STEP = CELL_SIZE + CELL_GAP;
constexpr float GRID_WIDTH = GRID_SIZE * CELL_SIZE + (GRID_SIZE - 1) * CELL_GAP;
constexpr float GRID_X = 60.0f;
constexpr float GRID_Y = 132.0f;

constexpr float ROUND_TIME = 90.0f;
constexpr int MIN_WORD_LENGTH = 3;
constexpr int POINTS_PER_LETTER = 2;

constexpr int SIDEBAR_X = 520;
constexpr int SIDEBAR_Y = 132;
constexpr int SIDEBAR_WIDTH = 420;
constexpr int SIDEBAR_HEIGHT = 440;

const Color PAGE_BACKGROUND = {245, 246, 250, 255};
const Color PANEL_BACKGROUND = {255, 255, 255, 255};
const Color CELL_BACKGROUND = {232, 234, 240, 255};
const Color CELL_HOVER = {222, 233, 245, 255};
const Color BORDER_COLOR = {190, 198, 210, 255};
const Color TEXT_DARK = {40, 45, 60, 255};
const Color TEXT_GRAY = {100, 110, 125, 255};
const Color SUCCESS_COLOR = {45, 160, 85, 255};
const Color WARNING_COLOR = {225, 140, 35, 255};
const Color ERROR_COLOR = {215, 65, 65, 255};

struct Cell {
    int row = 0;
    int col = 0;
};

struct FoundWord {
    std::string word;
    int points = 0;
};

struct GameState {
    std::array<std::array<char, GRID_SIZE>, GRID_SIZE> letters{};
    std::vector<Cell> selected;
    std::string currentWord;
    std::vector<FoundWord> foundWords;
    std::unordered_set<std::string> foundSet;
    int score = 0;
    float timeLeft = ROUND_TIME;
    bool gameOver = false;
    std::string feedback;
    Color feedbackColor = SUCCESS_COLOR;
    float feedbackTimer = 0.0f;
};

std::string ToUpper(const std::string& text) {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char character) {
                       return static_cast<char>(std::toupper(character));
                   });
    return result;
}

std::string TrimAndClean(const std::string& line) {
    std::size_t start = 0;
    std::size_t end = line.size();

    while (start < end && static_cast<unsigned char>(line[start]) <= ' ') {
        ++start;
    }
    while (end > start && static_cast<unsigned char>(line[end - 1]) <= ' ') {
        --end;
    }

    std::string word = line.substr(start, end - start);

    // Rimuove l'eventuale BOM UTF-8 all'inizio del file.
    if (word.size() >= 3 &&
        static_cast<unsigned char>(word[0]) == 0xEF &&
        static_cast<unsigned char>(word[1]) == 0xBB &&
        static_cast<unsigned char>(word[2]) == 0xBF) {
        word.erase(0, 3);
    }

    return word;
}

bool IsAsciiWord(const std::string& word) {
    for (unsigned char character : word) {
        if (character < 'A' || character > 'Z') {
            return false;
        }
    }
    return true;
}

std::unordered_set<std::string> LoadItalianDictionary() {
    const char* candidates[] = {
            "assets/italiano.txt",
            "italiano.txt",
            "../assets/italiano.txt",
            "../italiano.txt"
    };

    std::ifstream file;
    for (const char* candidate : candidates) {
        // clear() e' necessario per riprovare dopo un'apertura fallita:
        // altrimenti il failbit resta impostato e i percorsi successivi
        // verrebbero ignorati.
        file.clear();
        file.open(candidate);
        if (file.is_open()) {
            TraceLog(LOG_INFO, "Dizionario trovato: %s", candidate);
            break;
        }
    }

    std::unordered_set<std::string> dictionary;
    if (!file.is_open()) {
        TraceLog(LOG_ERROR,
                 "Impossibile aprire assets/italiano.txt. "
                 "Copia il dizionario nella cartella assets oppure avvia il gioco "
                 "dalla cartella del progetto.");
        return dictionary;
    }

    dictionary.reserve(700000);

    std::string line;
    while (std::getline(file, line)) {
        std::string word = ToUpper(TrimAndClean(line));
        if (word.size() < MIN_WORD_LENGTH || word.size() > 16 || !IsAsciiWord(word)) {
            continue;
        }
        dictionary.insert(std::move(word));
    }

    TraceLog(LOG_INFO, "Dizionario caricato: %d parole valide",
             static_cast<int>(dictionary.size()));
    return dictionary;
}

char RandomVowel(std::mt19937& rng) {
    static constexpr char vowels[] = "AEIOU";
    std::uniform_int_distribution<int> distribution(0, 4);
    return vowels[distribution(rng)];
}

char RandomConsonant(std::mt19937& rng) {
    static constexpr char consonants[] = "BCDFGHILMNOPQRSTVZ";
    std::uniform_int_distribution<int> distribution(
            0, static_cast<int>(sizeof(consonants) - 2));
    return consonants[distribution(rng)];
}

void GenerateGrid(GameState& state, std::mt19937& rng) {
    std::uniform_int_distribution<int> percentage(0, 99);

    int vowelCount = 0;
    for (int row = 0; row < GRID_SIZE; ++row) {
        for (int col = 0; col < GRID_SIZE; ++col) {
            bool vowel = percentage(rng) < 40;
            state.letters[row][col] = vowel ? RandomVowel(rng) : RandomConsonant(rng);
            if (vowel) {
                ++vowelCount;
            }
        }
    }

    std::uniform_int_distribution<int> cellDistribution(0, GRID_SIZE - 1);

    int guard = 0;
    while (vowelCount < 5 && guard++ < 100) {
        int row = cellDistribution(rng);
        int col = cellDistribution(rng);
        if (std::string("AEIOU").find(state.letters[row][col]) == std::string::npos) {
            state.letters[row][col] = RandomVowel(rng);
            ++vowelCount;
        }
    }

    guard = 0;
    while (vowelCount > 10 && guard++ < 100) {
        int row = cellDistribution(rng);
        int col = cellDistribution(rng);
        if (std::string("AEIOU").find(state.letters[row][col]) != std::string::npos) {
            state.letters[row][col] = RandomConsonant(rng);
            --vowelCount;
        }
    }
}

void ClearSelection(GameState& state) {
    state.selected.clear();
    state.currentWord.clear();
}

void ResetGame(GameState& state, std::mt19937& rng) {
    GenerateGrid(state, rng);
    ClearSelection(state);
    state.foundWords.clear();
    state.foundSet.clear();
    state.score = 0;
    state.timeLeft = ROUND_TIME;
    state.gameOver = false;
    state.feedback.clear();
    state.feedbackColor = SUCCESS_COLOR;
    state.feedbackTimer = 0.0f;
}

void SetFeedback(GameState& state, const std::string& message, Color color) {
    state.feedback = message;
    state.feedbackColor = color;
    state.feedbackTimer = 2.0f;
}

Rectangle CellRectangle(int row, int col) {
    return {
            GRID_X + static_cast<float>(col) * CELL_STEP,
            GRID_Y + static_cast<float>(row) * CELL_STEP,
            CELL_SIZE,
            CELL_SIZE
    };
}

Vector2 CellCenter(const Cell& cell) {
    Rectangle rectangle = CellRectangle(cell.row, cell.col);
    return {
            rectangle.x + rectangle.width / 2.0f,
            rectangle.y + rectangle.height / 2.0f
    };
}

bool GetCellAt(Vector2 mousePosition, int& row, int& col) {
    if (mousePosition.x < GRID_X || mousePosition.y < GRID_Y) {
        return false;
    }

    float localX = mousePosition.x - GRID_X;
    float localY = mousePosition.y - GRID_Y;

    int column = static_cast<int>(std::floor(localX / CELL_STEP));
    int line = static_cast<int>(std::floor(localY / CELL_STEP));

    if (column < 0 || column >= GRID_SIZE || line < 0 || line >= GRID_SIZE) {
        return false;
    }

    float insideX = localX - static_cast<float>(column) * CELL_STEP;
    float insideY = localY - static_cast<float>(line) * CELL_STEP;
    if (insideX > CELL_SIZE || insideY > CELL_SIZE) {
        return false;
    }

    row = line;
    col = column;
    return true;
}

bool IsSelected(const GameState& state, int row, int col) {
    for (const Cell& cell : state.selected) {
        if (cell.row == row && cell.col == col) {
            return true;
        }
    }
    return false;
}

bool AreAdjacent(const Cell& first, const Cell& second) {
    int rowDistance = std::abs(first.row - second.row);
    int colDistance = std::abs(first.col - second.col);
    return rowDistance <= 1 && colDistance <= 1 && (rowDistance + colDistance) > 0;
}

void AddCell(GameState& state, int row, int col) {
    state.selected.push_back({row, col});
    state.currentWord.push_back(state.letters[row][col]);
}

void RemoveLastCell(GameState& state) {
    if (state.selected.empty()) {
        return;
    }
    state.selected.pop_back();
    if (!state.currentWord.empty()) {
        state.currentWord.pop_back();
    }
}

void SubmitWord(GameState& state, const std::unordered_set<std::string>& dictionary) {
    if (state.currentWord.empty()) {
        return;
    }

    const std::string word = ToUpper(state.currentWord);

    if (word.size() < MIN_WORD_LENGTH) {
        SetFeedback(state, "La parola deve avere almeno 3 lettere", WARNING_COLOR);
    } else if (state.foundSet.find(word) != state.foundSet.end()) {
        SetFeedback(state, "Parola gia trovata", WARNING_COLOR);
    } else if (dictionary.find(word) == dictionary.end()) {
        SetFeedback(state, "Parola non valida", ERROR_COLOR);
    } else {
        int points = static_cast<int>(word.size()) * POINTS_PER_LETTER;
        state.score += points;
        state.foundWords.push_back({word, points});
        state.foundSet.insert(word);
        SetFeedback(state, "Parola valida! +" + std::to_string(points) + " punti",
                    SUCCESS_COLOR);
    }

    ClearSelection(state);
}

void HandleInput(GameState& state, const std::unordered_set<std::string>& dictionary) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        ClearSelection(state);
    }
    if (IsKeyPressed(KEY_BACKSPACE) || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        RemoveLastCell(state);
    }
    if ((IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) &&
        !state.currentWord.empty()) {
        SubmitWord(state, dictionary);
    }

    Vector2 mousePosition = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        int row = 0;
        int col = 0;
        if (GetCellAt(mousePosition, row, col)) {
            ClearSelection(state);
            AddCell(state, row, col);
        }
    } else if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && !state.selected.empty()) {
        int row = 0;
        int col = 0;
        if (GetCellAt(mousePosition, row, col)) {
            const Cell& last = state.selected.back();
            if (!IsSelected(state, row, col) && AreAdjacent(last, {row, col})) {
                AddCell(state, row, col);
            }
        }
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && !state.selected.empty()) {
        if (state.currentWord.size() >= MIN_WORD_LENGTH) {
            SubmitWord(state, dictionary);
        } else {
            ClearSelection(state);
        }
    }
}

void DrawCenteredText(const char* text, int centerX, int y, int fontSize, Color color) {
    int width = MeasureText(text, fontSize);
    DrawText(text, centerX - width / 2, y, fontSize, color);
}

void DrawConnectionLines(const GameState& state) {
    for (std::size_t index = 1; index < state.selected.size(); ++index) {
        DrawLineEx(CellCenter(state.selected[index - 1]),
                   CellCenter(state.selected[index]), 7.0f, Fade(BLUE, 0.55f));
    }

    for (const Cell& cell : state.selected) {
        DrawCircleV(CellCenter(cell), 5.0f, Fade(BLUE, 0.8f));
    }
}

void DrawGrid(const GameState& state) {
    int hoveredRow = -1;
    int hoveredCol = -1;
    if (!state.gameOver) {
        GetCellAt(GetMousePosition(), hoveredRow, hoveredCol);
    }

    for (int row = 0; row < GRID_SIZE; ++row) {
        for (int col = 0; col < GRID_SIZE; ++col) {
            Rectangle rectangle = CellRectangle(row, col);
            bool selected = IsSelected(state, row, col);
            bool isLast = selected && !state.selected.empty() &&
                          state.selected.back().row == row &&
                          state.selected.back().col == col;
            bool hovered = row == hoveredRow && col == hoveredCol;

            Color fill = CELL_BACKGROUND;
            Color border = BORDER_COLOR;

            if (state.gameOver) {
                fill = {205, 207, 212, 255};
            } else if (selected) {
                fill = isLast ? Color{65, 140, 220, 255} : SKYBLUE;
                border = DARKBLUE;
            } else if (hovered) {
                fill = CELL_HOVER;
            }

            DrawRectangleRounded(rectangle, 0.18f, 8, fill);
            DrawRectangleRoundedLinesEx(rectangle, 0.18f, 8, 2.0f, border);
        }
    }

    if (!state.gameOver) {
        DrawConnectionLines(state);
    }

    for (int row = 0; row < GRID_SIZE; ++row) {
        for (int col = 0; col < GRID_SIZE; ++col) {
            Rectangle rectangle = CellRectangle(row, col);
            bool selected = IsSelected(state, row, col);
            bool isLast = selected && !state.selected.empty() &&
                          state.selected.back().row == row &&
                          state.selected.back().col == col;

            char letterText[2] = {state.letters[row][col], '\0'};
            const int fontSize = 48;
            int textWidth = MeasureText(letterText, fontSize);
            Color textColor = isLast ? WHITE : BLACK;

            DrawText(letterText,
                     static_cast<int>(rectangle.x + (CELL_SIZE - textWidth) / 2.0f),
                     static_cast<int>(rectangle.y + (CELL_SIZE - fontSize) / 2.0f - 3.0f),
                     fontSize,
                     textColor);
        }
    }
}

void DrawSidebar(const GameState& state) {
    Rectangle panel = {
            static_cast<float>(SIDEBAR_X),
            static_cast<float>(SIDEBAR_Y),
            static_cast<float>(SIDEBAR_WIDTH),
            static_cast<float>(SIDEBAR_HEIGHT)
    };

    DrawRectangleRounded(panel, 0.03f, 8, PANEL_BACKGROUND);
    DrawRectangleRoundedLinesEx(panel, 0.03f, 8, 2.0f, BORDER_COLOR);

    DrawText("PAROLE TROVATE", SIDEBAR_X + 20, SIDEBAR_Y + 18, 24, TEXT_DARK);
    DrawText(TextFormat("%d parole - %d punti", static_cast<int>(state.foundWords.size()),
                        state.score),
             SIDEBAR_X + 20, SIDEBAR_Y + 48, 16, TEXT_GRAY);

    DrawLine(SIDEBAR_X + 20, SIDEBAR_Y + 72,
             SIDEBAR_X + SIDEBAR_WIDTH - 20, SIDEBAR_Y + 72, BORDER_COLOR);

    if (state.foundWords.empty()) {
        DrawText("Nessuna parola ancora.", SIDEBAR_X + 20, SIDEBAR_Y + 92, 18, TEXT_GRAY);
        DrawText("Trascina il mouse sulle lettere", SIDEBAR_X + 20, SIDEBAR_Y + 116,
                 16, TEXT_GRAY);
        DrawText("e rilascia per confermare.", SIDEBAR_X + 20, SIDEBAR_Y + 138,
                 16, TEXT_GRAY);
        return;
    }

    const int maxRows = 12;
    int firstRow = 0;
    if (static_cast<int>(state.foundWords.size()) > maxRows) {
        firstRow = static_cast<int>(state.foundWords.size()) - maxRows;
    }

    int y = SIDEBAR_Y + 88;
    for (int index = firstRow; index < static_cast<int>(state.foundWords.size()); ++index) {
        const FoundWord& found = state.foundWords[index];
        DrawText(found.word.c_str(), SIDEBAR_X + 20, y, 20, DARKBLUE);

        const char* points = TextFormat("+%d", found.points);
        int pointsWidth = MeasureText(points, 20);
        DrawText(points, SIDEBAR_X + SIDEBAR_WIDTH - 20 - pointsWidth, y, 20,
                 SUCCESS_COLOR);

        y += 28;
    }
}

void DrawHeader(const GameState& state) {
    DrawText("RUZZLE", 60, 18, 42, DARKBLUE);
    DrawText("ITALIANO", 60 + MeasureText("RUZZLE", 42) + 12, 32, 22, TEXT_GRAY);

    const char* scoreText = TextFormat("Punteggio: %d", state.score);
    int scoreWidth = MeasureText(scoreText, 28);
    DrawText(scoreText, SCREEN_WIDTH - 60 - scoreWidth, 24, 28, TEXT_DARK);

    const char* wordsText = TextFormat("%d parole", static_cast<int>(state.foundWords.size()));
    int wordsWidth = MeasureText(wordsText, 16);
    DrawText(wordsText, SCREEN_WIDTH - 60 - wordsWidth, 56, 16, TEXT_GRAY);

    float fraction = state.timeLeft / ROUND_TIME;
    if (fraction < 0.0f) {
        fraction = 0.0f;
    } else if (fraction > 1.0f) {
        fraction = 1.0f;
    }

    DrawRectangle(60, 78, 880, 16, Color{218, 222, 230, 255});

    Color barColor = GREEN;
    if (fraction <= 0.2f) {
        barColor = RED;
    } else if (fraction <= 0.5f) {
        barColor = ORANGE;
    }

    int barWidth = static_cast<int>(880.0f * fraction);
    if (barWidth > 0) {
        DrawRectangle(60, 78, barWidth, 16, barColor);
    }

    DrawText(TextFormat("Tempo: %.1f s", state.timeLeft), 60, 100, 18, TEXT_GRAY);
}

void DrawCurrentWord(const GameState& state) {
    if (state.currentWord.empty()) {
        const char* hint = "Trascina il mouse sulle lettere per formare una parola";
        int width = MeasureText(hint, 18);
        DrawText(hint, static_cast<int>(GRID_X + (GRID_WIDTH - width) / 2.0f), 570,
                 18, TEXT_GRAY);
        return;
    }

    int fontSize = 36;
    int width = MeasureText(state.currentWord.c_str(), fontSize);
    if (width > GRID_WIDTH) {
        fontSize = 26;
        width = MeasureText(state.currentWord.c_str(), fontSize);
    }

    DrawText(state.currentWord.c_str(),
             static_cast<int>(GRID_X + (GRID_WIDTH - width) / 2.0f), 562,
             fontSize, BLUE);
}

void DrawFeedback(const GameState& state) {
    if (state.feedback.empty() || state.feedbackTimer <= 0.0f) {
        return;
    }

    int width = MeasureText(state.feedback.c_str(), 20);
    DrawText(state.feedback.c_str(),
             static_cast<int>(GRID_X + (GRID_WIDTH - width) / 2.0f), 616,
             20, state.feedbackColor);
}

void DrawHints() {
    DrawText("R: nuova partita", 60, 692, 16, TEXT_GRAY);
    DrawCenteredText("Trascina per selezionare | Invio: conferma | ESC: deseleziona",
                     SCREEN_WIDTH / 2, 692, 16, TEXT_GRAY);

    const char* quit = "Q: esci";
    int width = MeasureText(quit, 16);
    DrawText(quit, SCREEN_WIDTH - 60 - width, 692, 16, TEXT_GRAY);
}

void DrawGameOver(const GameState& state) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.65f));

    Rectangle panel = {250, 190, 500, 330};
    DrawRectangleRounded(panel, 0.06f, 8, RAYWHITE);
    DrawRectangleRoundedLinesEx(panel, 0.06f, 8, 3.0f, DARKBLUE);

    DrawCenteredText("TEMPO SCADUTO!", SCREEN_WIDTH / 2, 218, 40, RED);
    DrawCenteredText(TextFormat("Punteggio finale: %d", state.score),
                     SCREEN_WIDTH / 2, 282, 30, TEXT_DARK);
    DrawCenteredText(TextFormat("%d parole trovate",
                                static_cast<int>(state.foundWords.size())),
                     SCREEN_WIDTH / 2, 324, 24, TEXT_GRAY);
    DrawCenteredText("Premi R per rigiocare", SCREEN_WIDTH / 2, 386, 26, DARKBLUE);
    DrawCenteredText("Premi ESC o Q per uscire", SCREEN_WIDTH / 2, 430, 20, TEXT_GRAY);
}

void DrawGame(const GameState& state) {
    ClearBackground(PAGE_BACKGROUND);

    DrawHeader(state);
    DrawGrid(state);
    DrawSidebar(state);
    DrawCurrentWord(state);
    DrawFeedback(state);
    DrawHints();

    if (state.gameOver) {
        DrawGameOver(state);
    }
}

} // namespace

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Ruzzle Italiano");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    std::unordered_set<std::string> dictionary = LoadItalianDictionary();
    if (dictionary.empty()) {
        CloseWindow();
        return 1;
    }

    std::random_device randomDevice;
    std::mt19937 randomGenerator(randomDevice());

    GameState state;
    ResetGame(state, randomGenerator);

    bool shouldClose = false;

    while (!WindowShouldClose() && !shouldClose) {
        float deltaTime = GetFrameTime();

        if (IsKeyPressed(KEY_Q)) {
            shouldClose = true;
        }
        if (IsKeyPressed(KEY_R)) {
            ResetGame(state, randomGenerator);
        }

        if (!state.gameOver) {
            state.timeLeft -= deltaTime;
            if (state.timeLeft <= 0.0f) {
                state.timeLeft = 0.0f;
                state.gameOver = true;
                ClearSelection(state);
            }
        }

        if (state.feedbackTimer > 0.0f) {
            state.feedbackTimer -= deltaTime;
        }

        if (!state.gameOver) {
            HandleInput(state, dictionary);
        } else if (IsKeyPressed(KEY_ESCAPE)) {
            shouldClose = true;
        }

        BeginDrawing();
        DrawGame(state);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
