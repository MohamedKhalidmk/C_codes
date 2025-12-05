// ttt_limit_27.c
// Implements parallel Tic-Tac-Toe game tree exploration using OpenMP tasks.
// Dynamically prunes the search space once 27 unique (canonical) terminal games are found.
//
// Compile: gcc -O2 -fopenmp ttt_limit_27.c -o ttt_limit_27
// Run:     ./ttt_limit_27

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <omp.h>
#include <stdatomic.h>

#define EMPTY 0
#define X 1
#define O 2
#define TARGET_COUNT 27

// --- Global Atomic Counters for Results and Control ---
atomic_int found_count = 0;       // Total unique canonical games found
atomic_int stop_search = 0;       // Global flag for dynamic pruning/early stop

atomic_int x_win_count = 0;       // Count of canonical X wins
atomic_int o_win_count = 0;       // Count of canonical O wins
atomic_int draw_count = 0;        // Count of canonical Draws

// --- Shared Data for Canonical History ---
// Stores canonical integer representations of boards already found.
int canonical_history[1000];
omp_lock_t history_lock;          // Lock to protect shared history array

// --- Game Logic Constants ---
static const int WIN[8][3] = {
    {0,1,2},{3,4,5},{6,7,8},
    {0,3,6},{1,4,7},{2,5,8},
    {0,4,8},{2,4,6}
};

// --- Board Helper Functions ---

// Checks if a player has won
int check_win(const int b[9]) {
    for (int i = 0; i < 8; ++i) {
        if (b[WIN[i][0]] != EMPTY &&
            b[WIN[i][0]] == b[WIN[i][1]] &&
            b[WIN[i][0]] == b[WIN[i][2]]) 
            return b[WIN[i][0]];
    }
    return 0;
}

// Checks if the board is completely full (implies a draw if no winner)
int is_full(const int b[9]) {
    for(int i = 0; i < 9; ++i)
        if(b[i] == EMPTY) 
            return 0;
    return 1;
}

// Converts a board state (base 3) into a unique integer key
int board_to_int(int b[9]) {
    int k = 0, m = 1;
    for(int i=0; i<9; ++i) {
        k += b[i] * m;
        m *= 3;
    }
    return k;
}

// Calculates the 8 symmetrical keys and finds the minimum (canonical) key
void get_transformed_key(int b[9], int *min_key) {
    // Defines the mapping indices for 8 symmetries (Identity, 3 Rotations, 4 Flips)
    int map[8][9] = {
        {0,1,2,3,4,5,6,7,8}, // Identity
        {2,5,8,1,4,7,0,3,6}, // Rot90
        {8,7,6,5,4,3,2,1,0}, // Rot180
        {6,3,0,7,4,1,8,5,2}, // Rot270
        {2,1,0,5,4,3,8,7,6}, // Mirror LR
        {6,7,8,3,4,5,0,1,2}, // Mirror UD
        {0,3,6,1,4,7,2,5,8}, // Mirror Main Diag
        {8,5,2,7,4,1,6,3,0}  // Mirror Anti Diag
    };

    for(int s = 0; s < 8; ++s) {
        int temp_k = 0, m = 1;
        for(int i = 0; i < 9; ++i) {
            // Apply symmetry map[s] to the original board b[]
            temp_k += b[map[s][i]] * m;
            m *= 3;
        }
        if(temp_k < *min_key)
            *min_key = temp_k;
    }
}

// Wrapper function to get the canonical key
int get_canonical_key(int b[9]) {
    int min_key = INT_MAX;
    get_transformed_key(b, &min_key);
    return min_key;
}

// --- Main Parallel Task Function ---

void play_game_task(int board[9], int player) {

    // 1. DYNAMIC PRUNING CHECK 
    if(atomic_load(&stop_search))
        return;

    int winner = check_win(board);
    int full = is_full(board);

    // 2. TERMINAL STATE PROCESSING
    if(winner || full) {
        int key = get_canonical_key(board);
        int is_new = 0;

        // Use lock for safe access and modification of shared history/counters
        omp_set_lock(&history_lock);

        if(!atomic_load(&stop_search)) {
            int exists = 0;
            int current_cnt = atomic_load(&found_count);

            // Check if this canonical key has already been stored
            for(int i = 0; i < current_cnt; ++i) {
                if(canonical_history[i] == key) {
                    exists = 1;
                    break;
                }
            }

            if(!exists) {
                canonical_history[current_cnt] = key;
                atomic_fetch_add(&found_count, 1);
                is_new = 1;

                // Update final results count
                if(winner == X)
                    atomic_fetch_add(&x_win_count, 1);
                else if(winner == O)
                    atomic_fetch_add(&o_win_count, 1);
                else
                    atomic_fetch_add(&draw_count, 1);

                // Check for TARGET and set PRUNING flag
                if(atomic_load(&found_count) >= TARGET_COUNT)
                    atomic_store(&stop_search, 1);
            }
        }

        omp_unset_lock(&history_lock);

        if(is_new) {
            printf("[Thread %d] Unique #%d  Winner: %c\n",
                   omp_get_thread_num(),
                   atomic_load(&found_count),
                   winner==X?'X':winner==O?'O':'D');
        }

        return;
    }

    // 3. SPAWN TASKS FOR EACH MOVE 
    for(int i = 0; i < 9; ++i) {
        // Re-check stop flag before spawning the next move's task
        if(atomic_load(&stop_search))
            return;

        if(board[i] == EMPTY) {
            int next_board[9];
            memcpy(next_board, board, 9 * sizeof(int));
            next_board[i] = player;

            // FIX: 'player' added to firstprivate list to satisfy default(none)
            #pragma omp task firstprivate(next_board, player) \
            shared(stop_search, history_lock, found_count, canonical_history, x_win_count, o_win_count, draw_count) \
            default(none)
            {
                // Recursive call, switching player (X plays O, O plays X)
                play_game_task(next_board, player==X?O:X);
            }
        }
    }

    // Wait for all child tasks (moves) from this board state to complete
    #pragma omp taskwait
}

// --- Main Execution ---

int main() {
    int root_board[9] = {0};

    omp_init_lock(&history_lock);
    omp_set_nested(1); // Allow tasks to spawn more tasks

    printf("=== Starting Parallel Tic-Tac-Toe Exploration ===\n");
    printf("Targeting %d unique (canonical) terminal games.\n", TARGET_COUNT);

    #pragma omp parallel
    {
        #pragma omp single
        {
            // Begin the search from the empty board with player X starting
            play_game_task(root_board, X);
        }
    }

    // --- Final Results ---
    printf("\n=== Finished Exploration ===\n");
    printf("Total Unique Canonical Games Found: %d\n", atomic_load(&found_count));
    printf("----------------------------------------\n");
    printf("X Wins: %d\n", atomic_load(&x_win_count));
    printf("O Wins: %d\n", atomic_load(&o_win_count));
    printf("Draws: %d\n", atomic_load(&draw_count));
    printf("----------------------------------------\n");


    if(atomic_load(&stop_search))
        printf("Search successfully PRUNED early because the target count was reached.\n");
    else
        printf("Search completed fully.\n"); // Note: This should not happen if target=27

    omp_destroy_lock(&history_lock);
    return 0;
}