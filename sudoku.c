/*
 * 数独求解程序（回溯法）
 *
 * 用法：
 *   ./sudoku              求解内置的示例题目
 *   ./sudoku < puzzle.txt 从标准输入读取题目
 *
 * 输入格式：81 个数字，按行排列，0 或 '.' 表示空格，其余字符被忽略。
 */
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

#define N 9

static int grid[N][N];

static void print_grid(void)
{
    for (int r = 0; r < N; r++) {
        if (r % 3 == 0)
            printf("+-------+-------+-------+\n");
        for (int c = 0; c < N; c++) {
            if (c % 3 == 0)
                printf("| ");
            if (grid[r][c])
                printf("%d ", grid[r][c]);
            else
                printf(". ");
        }
        printf("|\n");
    }
    printf("+-------+-------+-------+\n");
}

/* 判断数字 num 能否放在 (row, col) */
static int can_place(int row, int col, int num)
{
    int br = row / 3 * 3, bc = col / 3 * 3;

    for (int i = 0; i < N; i++) {
        if (grid[row][i] == num || grid[i][col] == num)
            return 0;
        if (grid[br + i / 3][bc + i % 3] == num)
            return 0;
    }
    return 1;
}

/* 检查题目中已给出的数字是否互相冲突 */
static int is_valid_puzzle(void)
{
    for (int r = 0; r < N; r++) {
        for (int c = 0; c < N; c++) {
            int num = grid[r][c];
            if (!num)
                continue;
            grid[r][c] = 0;
            int ok = can_place(r, c, num);
            grid[r][c] = num;
            if (!ok)
                return 0;
        }
    }
    return 1;
}

/* 回溯求解：返回 1 表示找到解 */
static int solve(void)
{
    for (int r = 0; r < N; r++) {
        for (int c = 0; c < N; c++) {
            if (grid[r][c])
                continue;
            for (int num = 1; num <= N; num++) {
                if (can_place(r, c, num)) {
                    grid[r][c] = num;
                    if (solve())
                        return 1;
                    grid[r][c] = 0;
                }
            }
            return 0;
        }
    }
    return 1;
}

/* 从标准输入读取 81 个格子，成功返回 1 */
static int read_grid(FILE *in)
{
    int count = 0, ch;

    while (count < N * N && (ch = fgetc(in)) != EOF) {
        if (isdigit(ch))
            grid[count / N][count % N] = ch - '0';
        else if (ch == '.')
            grid[count / N][count % N] = 0;
        else
            continue;
        count++;
    }
    return count == N * N;
}

static const char *sample =
    "530070000"
    "600195000"
    "098000060"
    "800060003"
    "400803001"
    "700020006"
    "060000280"
    "000419005"
    "000080079";

int main(int argc, char *argv[])
{
    if (argc > 1) {
        fprintf(stderr, "用法: %s [< puzzle.txt]\n", argv[0]);
        return 1;
    }

    if (isatty(fileno(stdin))) {
        for (int i = 0; i < N * N; i++)
            grid[i / N][i % N] = sample[i] - '0';
    } else if (!read_grid(stdin)) {
        fprintf(stderr, "错误：输入不足 81 个格子\n");
        return 1;
    }

    printf("题目：\n");
    print_grid();

    if (!is_valid_puzzle()) {
        fprintf(stderr, "错误：题目中的数字有冲突\n");
        return 1;
    }

    if (solve()) {
        printf("\n答案：\n");
        print_grid();
        return 0;
    }

    printf("\n此题无解\n");
    return 1;
}
