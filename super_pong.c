#include <SDL.h>
#include <SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define PADDLE_SPEED 6
#define PADDLE_WIDTH 10
#define PADDLE_HEIGHT 50
#define PADDLE_MARGIN 20
#define BALL_SIZE 8
#define BALL_SPEED 18
#define FONT_SIZE 24
typedef struct {
	int width;
	int height;
	SDL_Window *window;
	SDL_Renderer *renderer;
	TTF_Font *font;
} Video;

typedef struct {
	float x;
	float y;
	float velocity_y;
	int width;
	int height;
} Paddle;

typedef struct {
	float x;
	float y;
	float velocity_x;
	float velocity_y;
	int size;
} Ball;

typedef enum { PLAYER_HUMAN, PLAYER_AI_BASIC,
	// Add more AI types here
} PlayerType;

typedef struct {
	Paddle paddle;
	PlayerType type;
	bool is_left;
	int score;
} Player;

typedef struct {
	Video video;
	Player players[2];
	Ball ball;
	bool playing;
	bool taking_control;
} GameState;

int init_video(Video *video){
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_Log("SDL initialization failed: %s", SDL_GetError());
		return -1;
	}
	if (TTF_Init() < 0) {
		SDL_Log("SDL_ttf initialization failed: %s", TTF_GetError());
		return -1;
	}
	video->window =
	    SDL_CreateWindow("SuperPong 0.0.2", SDL_WINDOWPOS_UNDEFINED,
			     SDL_WINDOWPOS_UNDEFINED, video->width,
			     video->height, SDL_WINDOW_SHOWN);
	if (!video->window) {
		SDL_Log("Window creation failed: %s", SDL_GetError());
		return -1;
	}
	video->renderer =
	    SDL_CreateRenderer(video->window, -1, SDL_RENDERER_ACCELERATED);
	if (!video->renderer) {
		SDL_Log("Renderer creation failed: %s", SDL_GetError());
		return -1;
	}
	video->font = TTF_OpenFont("PressStart2P-vaV7.ttf", FONT_SIZE);
	if (!video->font) {
		SDL_Log("Font loading failed: %s", TTF_GetError());
		return -1;
	}
	return 0;
}

void cleanup_video(Video *video){
	TTF_CloseFont(video->font);
	SDL_DestroyRenderer(video->renderer);
	SDL_DestroyWindow(video->window);
	TTF_Quit();
	SDL_Quit();
}

void init_paddle(Paddle *paddle, float x){
	paddle->x = x;
	paddle->y = SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2;
	paddle->velocity_y = 0;
	paddle->width = PADDLE_WIDTH;
	paddle->height = PADDLE_HEIGHT;
}

void init_players(Player players[2]){
	init_paddle(&players[0].paddle, PADDLE_MARGIN);
	players[0].type = PLAYER_AI_BASIC;
	players[0].is_left = true;
	init_paddle(&players[1].paddle,
		    SCREEN_WIDTH - PADDLE_MARGIN - PADDLE_WIDTH);
	players[1].type = PLAYER_AI_BASIC;
	players[1].is_left = false;
}

void handle_input(GameState *state){
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			state->playing = false;
		} else if (event.type == SDL_KEYDOWN) {
			switch (event.key.keysym.scancode) {
				
			case SDL_SCANCODE_T:
				// T + [0|1] to take/leave control
				// this feature should have a timeout...
				state->taking_control = true;
				break;
			case SDL_SCANCODE_0:
				// 0: left player
				if (state->taking_control) {
					state->players[0].type =
						(state->players[0].type ==
						 PLAYER_HUMAN) ? PLAYER_AI_BASIC :
						PLAYER_HUMAN;
					state->taking_control = false;
				}
				break;
			case SDL_SCANCODE_1:
				// 1: right player
				if (state->taking_control) {
					state->players[1].type =
						(state->players[1].type ==
						 PLAYER_HUMAN) ? PLAYER_AI_BASIC :
						PLAYER_HUMAN;
					state->taking_control = false;
				}
				break;
			case SDL_SCANCODE_ESCAPE:
				state->playing = false;
				break;
			default:
				break;
			}
		}
	}
	const Uint8 *keyboard_state = SDL_GetKeyboardState(NULL);
	for (int i = 0; i < 2; i++) {
		if (state->players[i].type == PLAYER_HUMAN) {
			SDL_Scancode up_key =
			    state->players[i].is_left ? SDL_SCANCODE_W :
			    SDL_SCANCODE_UP;
			SDL_Scancode down_key =
			    state->players[i].is_left ? SDL_SCANCODE_S :
			    SDL_SCANCODE_DOWN;
			if (keyboard_state[up_key]) {
				state->players[i].paddle.velocity_y =
				    -PADDLE_SPEED;
			} else if (keyboard_state[down_key]) {
				state->players[i].paddle.velocity_y =
				    PADDLE_SPEED;
			} else {
				state->players[i].paddle.velocity_y = 0;
			}
		}
	}
}

void update_paddle(Paddle *paddle){
	paddle->y += paddle->velocity_y;
	if (paddle->y < 0)
		paddle->y = 0;
	if (paddle->y > SCREEN_HEIGHT - paddle->height)
		paddle->y = SCREEN_HEIGHT - paddle->height;
}

void init_ball(Ball *ball){
	ball->x = SCREEN_WIDTH / 2 - BALL_SIZE / 2;
	ball->y = SCREEN_HEIGHT / 2 - BALL_SIZE / 2;

	// Give the ball an initial speed
	float angle = (rand() % 60 - 30) * M_PI / 180.0;
	ball->velocity_x = BALL_SPEED * cos(angle) * (rand() % 2 == 0 ? 1 : -1);
	ball->velocity_y = BALL_SPEED * sin(angle);
	ball->size = BALL_SIZE;
}

#define LEFT_PADDLE_X (PADDLE_MARGIN)
#define RIGHT_PADDLE_X (SCREEN_WIDTH - PADDLE_MARGIN - PADDLE_WIDTH)
void update_ball(Ball *ball, Player players[2]){
	ball->x += ball->velocity_x;
	ball->y += ball->velocity_y;

	// Multi-collision avoidance
	if (ball->velocity_x > 0) {
		// Ball moving right
		if (ball->x < LEFT_PADDLE_X + PADDLE_WIDTH) {
			ball->x = LEFT_PADDLE_X + PADDLE_WIDTH;
		}
	} else {
		// Ball moving left
		if (ball->x > RIGHT_PADDLE_X - ball->size) {
			ball->x = RIGHT_PADDLE_X - ball->size;
		}
	}
	
	// Wall collisions
	if (ball->y <= 0 || ball->y >= SCREEN_HEIGHT - ball->size) {
		ball->velocity_y = -ball->velocity_y;
	}
	
	// Paddle collisions
	for (int i = 0; i < 2; i++) {
		Paddle *paddle = &players[i].paddle;
		if (ball->x < paddle->x + paddle->width
		    && ball->x + ball->size > paddle->x
		    && ball->y < paddle->y + paddle->height
		    && ball->y + ball->size > paddle->y) {

			// implement variations of paddle
			ball->velocity_x =
			    -ball->velocity_x + (((float)rand() / RAND_MAX) -
						 0.5);
			ball->velocity_y +=
			    (((float)rand() / RAND_MAX) - 0.5) * 2;
		}
	}
	
	// Reset ball if it goes out of bounds and update scores
	if (ball->x <= 0) {
		players[1].score++;
		init_ball(ball);
	} else if (ball->x >= SCREEN_WIDTH - ball->size) {
		players[0].score++;
		init_ball(ball);
	}
}

void update_ai(Player *player, Ball *ball){
	float paddle_center = player->paddle.y + player->paddle.height / 2;
	float ball_center = ball->y + ball->size / 2;
	float distance = ball_center - paddle_center;
	
	// Computer player logic
	if (fabs(distance) > player->paddle.height / 4) {
		if (distance > 0) {
			player->paddle.velocity_y = PADDLE_SPEED;
		} else {
			player->paddle.velocity_y = -PADDLE_SPEED;
		}
	} else {
		player->paddle.velocity_y = 0;
	}

	// Add some prediction
	if ((player->is_left && ball->velocity_x < 0)
	    || (!player->is_left && ball->velocity_x > 0)) {
		float time_to_reach =
		    fabs((player->paddle.x - ball->x) / ball->velocity_x);
		float predicted_y = ball->y + ball->velocity_y * time_to_reach;
		if (predicted_y < player->paddle.y) {
			player->paddle.velocity_y = -PADDLE_SPEED;
		} else if (predicted_y >
			   player->paddle.y + player->paddle.height) {
			player->paddle.velocity_y = PADDLE_SPEED;
		}
	}
}

void update_game(GameState *state){
	for (int i = 0; i < 2; i++) {
		if (state->players[i].type == PLAYER_AI_BASIC) {
			update_ai(&state->players[i], &state->ball);
		}
		update_paddle(&state->players[i].paddle);
	}
	update_ball(&state->ball, state->players);
}

void render_score(GameState *state){
	char score_text[20];
	SDL_Color white = { 255, 255, 255, 255 };
	snprintf(score_text, sizeof(score_text), "%d  %d",
		 state->players[0].score, state->players[1].score);
	SDL_Surface *surface =
		TTF_RenderText_Solid(state->video.font, score_text, white);
	if (!surface) {
		SDL_Log("Score rendering failed: %s", TTF_GetError());
		return;
	}
	SDL_Texture *texture =
		SDL_CreateTextureFromSurface(state->video.renderer, surface);
	if (!texture) {
		SDL_Log("Texture creation failed: %s", SDL_GetError());
		SDL_FreeSurface(surface);
		return;
	}
	SDL_Rect dest_rect = {.x = (SCREEN_WIDTH - surface->w) / 2,.y =
		20,.w = surface->w,.h = surface->h
	};
	SDL_RenderCopy(state->video.renderer, texture, NULL, &dest_rect);
	SDL_FreeSurface(surface);
	SDL_DestroyTexture(texture);
}

void render(GameState *state){
	SDL_SetRenderDrawColor(state->video.renderer, 0, 0, 0, 255);
	SDL_RenderClear(state->video.renderer);
	
	// Render paddles
	SDL_SetRenderDrawColor(state->video.renderer, 255, 255, 255, 255);
	for (int i = 0; i < 2; i++) {
		SDL_Rect paddle_rect = {
			(int)state->players[i].paddle.x,
			(int)state->players[i].paddle.y,
			state->players[i].paddle.width,
			state->players[i].paddle.height
		};
		SDL_RenderFillRect(state->video.renderer, &paddle_rect);
	}
	
	// Render ball
	SDL_SetRenderDrawColor(state->video.renderer, 255, 0, 0, 255);
	SDL_Rect ball_rect = {
		(int)state->ball.x,
		(int)state->ball.y, state->ball.size, state->ball.size
	};
	SDL_RenderFillRect(state->video.renderer, &ball_rect);
	
	// Render score
	render_score(state);
	SDL_RenderPresent(state->video.renderer);
}

int main(int argc, char **argv){
	GameState state = {
		.video = {SCREEN_WIDTH, SCREEN_HEIGHT, NULL, NULL, NULL},
		.playing = true,
		.taking_control = false
	};
	
	if (init_video(&state.video) < 0) {
		return 1;
	}
	
	init_players(state.players);
	init_ball(&state.ball);	
	
	srand(time(NULL));
	Uint32 frame_start, frame_time;
	const Uint32 frame_delay = 1000 / 60;	// 60 FPS
	printf("This is SuperPong 0.0.2\n");

	while (state.playing) {
		frame_start = SDL_GetTicks();
		handle_input(&state);
		update_game(&state);
		render(&state);
		frame_time = SDL_GetTicks() - frame_start;
		if (frame_delay > frame_time) {
			SDL_Delay(frame_delay - frame_time);
		}
	}
	cleanup_video(&state.video);
	return 0;
}
