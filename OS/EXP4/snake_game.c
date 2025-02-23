#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <ncurses.h>
#include <stdbool.h>
#include <time.h>

#define HEIGHT 30
#define WIDTH 40
#define MAX_SNAKE 1000

typedef struct {
    int x, y;
} Point;

int score = 0;
bool game_over = false;
int direction = 1;  // 0: up, 1: right, 2: down, 3: left

Point snake[MAX_SNAKE];
int snake_length = 3;

Point ball;
bool ball_active = true;

Point bomb;
bool bomb_active = false;

pthread_mutex_t lock;

void spawn_ball();
void spawn_bomb();
void draw_game();
void init_game();

void *input_thread(void *arg) {
    int ch;
    while (!game_over) {
        ch = getch();
        if (ch != ERR) {
            pthread_mutex_lock(&lock);
            // Change direction if valid and not reversing directly
            if(ch == 'w' && direction != 2) { direction = 0; }
            else if(ch == 'd' && direction != 3) { direction = 1; }
            else if(ch == 's' && direction != 0) { direction = 2; }
            else if(ch == 'a' && direction != 1) { direction = 3; }
            pthread_mutex_unlock(&lock);
        }
        usleep(50000);
    }
    return NULL;
}

void *bomb_thread(void *arg) {
    // Bomb is active for 10 seconds; during this time ball is not present.
    int t = 10;
    while(t-- && !game_over) {
        sleep(1);
    }
    pthread_mutex_lock(&lock);
    bomb_active = false;
    ball_active = true;
    spawn_ball();
    pthread_mutex_unlock(&lock);
    return NULL;
}

void *game_thread(void *arg) {
    while (!game_over) {
        usleep(200000);  // control snake speed

        pthread_mutex_lock(&lock);
        // Determine new head position based on current direction.
        int new_x = snake[0].x;
        int new_y = snake[0].y;
        switch(direction) {
            case 0: new_x--; break;
            case 1: new_y++; break;
            case 2: new_x++; break;
            case 3: new_y--; break;
        }
        // Wrap-around behavior.
        if(new_x < 0) new_x = HEIGHT - 1;
        if(new_x >= HEIGHT) new_x = 0;
        if(new_y < 0) new_y = WIDTH - 1;
        if(new_y >= WIDTH) new_y = 0;
        
        // Check collision with self.
        for(int i = 0; i < snake_length; i++) {
            if(snake[i].x == new_x && snake[i].y == new_y) {
                game_over = true;
            }
        }
        // Check collision with bomb.
        if(bomb_active && new_x == bomb.x && new_y == bomb.y) {
            game_over = true;
        }
        if(game_over) {
            pthread_mutex_unlock(&lock);
            break;
        }
        
        // Check if snake eats the ball.
        if(ball_active && new_x == ball.x && new_y == ball.y) {
            score++;
            // Grow the snake by shifting body and inserting new head.
            for(int i = snake_length; i > 0; i--) {
                snake[i] = snake[i-1];
            }
            snake[0].x = new_x;
            snake[0].y = new_y;
            snake_length++;

            // If score is a multiple of 10 (and not 0), spawn a bomb.
            if(score % 10 == 0) {
                bomb_active = true;
                ball_active = false; // ball not shown during bomb time
                spawn_bomb();
                pthread_t t_bomb;
                pthread_create(&t_bomb, NULL, bomb_thread, NULL);
                pthread_detach(t_bomb);
            } else {
                spawn_ball();
            }
        } else {
            // Normal movement: shift snake body forward.
            for(int i = snake_length - 1; i > 0; i--) {
                snake[i] = snake[i-1];
            }
            snake[0].x = new_x;
            snake[0].y = new_y;
        }
        
        draw_game();
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

void draw_game() {
    clear();
    // Draw border
    for (int i = 0; i < WIDTH + 2; i++)  
        mvprintw(0, i, "# ");  // Top border 
    for (int i = 1; i < HEIGHT + 1; i++) {
        mvprintw(i, 0, "# ");  // Left border 
        mvprintw(i, WIDTH + 1, "#");  // Right border 
    }
    for (int i = 0; i < WIDTH + 2; i++)  
        mvprintw(HEIGHT + 1, i, "#");  // Bottom border 
    
    // Draw snake: head as 'O' and body as 'o'
    for(int i = 0; i < snake_length; i++){
        mvprintw(snake[i].x + 1, snake[i].y + 1, (i==0) ? "O" : "o");
    }
    
    // Draw ball if active.
    if(ball_active)
        mvprintw(ball.x + 1, ball.y + 1, "*");
    // Draw bomb if active.
    if(bomb_active)
        mvprintw(bomb.x + 1, bomb.y + 1, "X");
    
    // Display score.
    mvprintw(HEIGHT+3, 0, "Score: %d", score);
    refresh();
}

void spawn_ball() {
    // Choose a random location not colliding with the snake.
    while(1) {
        int rx = rand() % HEIGHT;
        int ry = rand() % WIDTH;
        bool collision = false;
        for(int i = 0; i < snake_length; i++) {
            if(snake[i].x == rx && snake[i].y == ry) {
                collision = true;
                break;
            }
        }
        if(collision) continue;
        ball.x = rx;
        ball.y = ry;
        ball_active = true;
        break;
    }
}

void spawn_bomb() {
    // Choose a random location not colliding with the snake.
    while(1) {
        int rx = rand() % HEIGHT;
        int ry = rand() % WIDTH;
        bool collision = false;
        for(int i = 0; i < snake_length; i++) {
            if(snake[i].x == rx && snake[i].y == ry) {
                collision = true;
                break;
            }
        }
        if(collision) continue;
        bomb.x = rx;
        bomb.y = ry;
        break;
    }
}

void init_game() {
    // Initialize snake in the center (with initial length 3).
    snake[0].x = HEIGHT / 2;
    snake[0].y = WIDTH / 2;
    snake[1].x = snake[0].x;
    snake[1].y = snake[0].y - 1;
    snake[2].x = snake[0].x;
    snake[2].y = snake[0].y - 2;
    spawn_ball();
}

int main() {
    srand(time(NULL));
    initscr();
    noecho();
    cbreak();
    curs_set(FALSE);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    
    pthread_mutex_init(&lock, NULL);
    init_game();
    
    pthread_t t_input, t_game;
    pthread_create(&t_input, NULL, input_thread, NULL);
    pthread_create(&t_game, NULL, game_thread, NULL);
    
    pthread_join(t_input, NULL);
    pthread_join(t_game, NULL);
    
    // Final game over message.
    clear();
    mvprintw(HEIGHT/2, (WIDTH-10)/2, "Game Over!");
    mvprintw(HEIGHT/2+1, (WIDTH-12)/2, "Final Score: %d", score);
    refresh();
    sleep(3);
    
    endwin();
    pthread_mutex_destroy(&lock);
    return 0;
}
