/*
 * 终端数独游戏
 *
 * 编译：gcc -Wall -o sudoku sudoku.c
 * 运行：./sudoku
 *
 * 每局随机生成一道有唯一解的题目，可选简单 / 中等 / 困难三种难度。
 * 输入 "行 列 数字" 填数（如 "3 5 7"），数字为 0 表示清除该格。
 * 输入 h 查看全部命令。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define N 9

static int puzzle[N][N];   /* 题目给出的数字，不可修改 */
static int board[N][N];    /* 玩家当前盘面 */
static int solution[N][N]; /* 唯一解 */
static int use_color;

/* ---------- 规则与求解 ---------- */

/* 判断数字 num 能否放在 g 的 (row, col)，忽略该格自身 */
static int can_place(int g[N][N], int row, int col, int num)
{
    int br = row / 3 * 3, bc = col / 3 * 3;

    for (int i = 0; i < N; i++) {
        if (i != col && g[row][i] == num)
            return 0;
        if (i != row && g[i][col] == num)
            return 0;
        int r = br + i / 3, c = bc + i % 3;
        if ((r != row || c != col) && g[r][c] == num)
            return 0;
    }
    return 1;
}

static void shuffle(int *a, int n)
{
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int t = a[i];
        a[i] = a[j];
        a[j] = t;
    }
}

/* 用随机顺序的回溯填满整个盘面 */
static int fill_random(int g[N][N], int pos)
{
    if (pos == N * N)
        return 1;

    int r = pos / N, c = pos % N;
    int nums[N] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    shuffle(nums, N);
    for (int i = 0; i < N; i++) {
        if (can_place(g, r, c, nums[i])) {
            g[r][c] = nums[i];
            if (fill_random(g, pos + 1))
                return 1;
        }
    }
    g[r][c] = 0;
    return 0;
}

/* 统计解的个数，数到 limit 就停止 */
static int count_solutions(int g[N][N], int limit)
{
    for (int pos = 0; pos < N * N; pos++) {
        int r = pos / N, c = pos % N;
        if (g[r][c])
            continue;

        int count = 0;
        for (int num = 1; num <= N && count < limit; num++) {
            if (can_place(g, r, c, num)) {
                g[r][c] = num;
                count += count_solutions(g, limit - count);
            }
        }
        g[r][c] = 0;
        return count;
    }
    return 1;
}

/* 生成新题目：先随机填满，再在保持唯一解的前提下挖去最多 holes 个格子 */
static void generate(int holes)
{
    int order[N * N];

    memset(solution, 0, sizeof(solution));
    fill_random(solution, 0);
    memcpy(puzzle, solution, sizeof(puzzle));

    for (int i = 0; i < N * N; i++)
        order[i] = i;
    shuffle(order, N * N);

    for (int i = 0; i < N * N && holes > 0; i++) {
        int r = order[i] / N, c = order[i] % N;
        int saved = puzzle[r][c];

        puzzle[r][c] = 0;
        if (count_solutions(puzzle, 2) == 1)
            holes--;
        else
            puzzle[r][c] = saved;
    }
    memcpy(board, puzzle, sizeof(board));
}

/* ---------- 显示 ---------- */

static void print_board(void)
{
    printf("\n    1 2 3   4 5 6   7 8 9\n");
    for (int r = 0; r < N; r++) {
        if (r % 3 == 0)
            printf("  +-------+-------+-------+\n");
        printf("%d ", r + 1);
        for (int c = 0; c < N; c++) {
            if (c % 3 == 0)
                printf("| ");
            int v = board[r][c];
            if (!v)
                printf(". ");
            else if (puzzle[r][c] || !use_color)
                printf("%d ", v);
            else if (!can_place(board, r, c, v))
                printf("\033[31m%d\033[0m ", v); /* 冲突：红色 */
            else
                printf("\033[36m%d\033[0m ", v); /* 玩家填的：青色 */
        }
        printf("|\n");
    }
    printf("  +-------+-------+-------+\n");
}

static void print_help(void)
{
    printf("\n命令：\n"
           "  行 列 数字   在指定格填数，例如 3 5 7（数字 0 表示清除）\n"
           "  t            提示：随机揭开一个空格\n"
           "  c            检查：列出填错的格子\n"
           "  r            重置本局（清除所有已填的数字）\n"
           "  s            放弃并显示答案\n"
           "  n            开始新游戏\n"
           "  h            显示本帮助\n"
           "  q            退出\n");
}

/* ---------- 游戏逻辑 ---------- */

static int count_empty(void)
{
    int n = 0;
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++)
            n += board[r][c] == 0;
    return n;
}

static int is_solved(void)
{
    return memcmp(board, solution, sizeof(board)) == 0;
}

/* 读一行输入，EOF 返回 0 */
static int read_line(const char *prompt, char *buf, size_t size)
{
    printf("%s", prompt);
    fflush(stdout);
    return fgets(buf, (int)size, stdin) != NULL;
}

/* 选择难度并生成新局，用户退出返回 0 */
static int new_game(void)
{
    char buf[64];

    for (;;) {
        if (!read_line("\n选择难度 (1 简单 / 2 中等 / 3 困难，q 退出): ",
                       buf, sizeof(buf)))
            return 0;
        if (buf[0] == 'q' || buf[0] == 'Q')
            return 0;

        int level = atoi(buf);
        int holes[] = {0, 36, 46, 56};
        if (level >= 1 && level <= 3) {
            printf("正在生成题目...\n");
            generate(holes[level]);
            printf("题目已生成，空格数：%d。输入 h 查看帮助。\n", count_empty());
            return 1;
        }
        printf("请输入 1、2 或 3。\n");
    }
}

static void give_hint(void)
{
    int cells[N * N], n = 0;

    for (int i = 0; i < N * N; i++)
        if (board[i / N][i % N] != solution[i / N][i % N])
            cells[n++] = i;
    if (!n)
        return;

    int i = cells[rand() % n], r = i / N, c = i % N;
    board[r][c] = solution[r][c];
    printf("提示：第 %d 行第 %d 列是 %d。\n", r + 1, c + 1, solution[r][c]);
}

static void check_board(void)
{
    int wrong = 0;

    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++)
            if (board[r][c] && board[r][c] != solution[r][c]) {
                if (!wrong)
                    printf("填错的格子：");
                printf("(%d,%d) ", r + 1, c + 1);
                wrong++;
            }
    if (wrong)
        printf("\n共 %d 处错误。\n", wrong);
    else
        printf("目前填的都正确，还剩 %d 个空格。\n", count_empty());
}

/* 处理 "行 列 数字" 输入 */
static void place(int r, int c, int v)
{
    if (r < 1 || r > N || c < 1 || c > N || v < 0 || v > N) {
        printf("行、列须为 1-9，数字须为 0-9。\n");
        return;
    }
    r--;
    c--;
    if (puzzle[r][c]) {
        printf("第 %d 行第 %d 列是题目给出的数字，不能修改。\n", r + 1, c + 1);
        return;
    }
    board[r][c] = v;
    if (v && !can_place(board, r, c, v))
        printf("注意：%d 与同行、同列或同宫的数字冲突。\n", v);
}

int main(void)
{
    char buf[128];
    int playing;

    srand((unsigned)time(NULL));
    use_color = isatty(fileno(stdout));

    printf("===== 数独 =====\n"
           "把 1-9 填入空格，使每行、每列、每个 3x3 宫内数字都不重复。\n");

    playing = new_game();
    while (playing) {
        print_board();

        if (is_solved()) {
            printf("\n恭喜你，完成了这道数独！\n");
            playing = new_game();
            continue;
        }

        if (!read_line("> ", buf, sizeof(buf)))
            break;

        int r, c, v;
        char cmd = buf[strspn(buf, " \t")];

        if (sscanf(buf, "%d %d %d", &r, &c, &v) == 3) {
            place(r, c, v);
            continue;
        }

        switch (cmd) {
        case 't': case 'T':
            give_hint();
            break;
        case 'c': case 'C':
            check_board();
            break;
        case 'r': case 'R':
            memcpy(board, puzzle, sizeof(board));
            printf("已重置。\n");
            break;
        case 's': case 'S':
            memcpy(board, solution, sizeof(board));
            print_board();
            printf("\n这是答案。再来一局吧！\n");
            playing = new_game();
            break;
        case 'n': case 'N':
            playing = new_game();
            break;
        case 'h': case 'H': case '?':
            print_help();
            break;
        case 'q': case 'Q':
            playing = 0;
            break;
        case '\n': case '\0':
            break;
        default:
            printf("无法识别的输入，输入 h 查看帮助。\n");
        }
    }

    printf("再见！\n");
    return 0;
}
