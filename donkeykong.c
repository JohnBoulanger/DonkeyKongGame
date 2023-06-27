#define SDRAM_BASE            0xC0000000
#define FPGA_ONCHIP_BASE      0xC8000000
#define FPGA_CHAR_BASE        0xC9000000
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>

/* Cyclone V FPGA devices */
#define LEDR_BASE             0xFF200000
#define HEX3_HEX0_BASE        0xFF200020
#define HEX5_HEX4_BASE        0xFF200030
#define SW_BASE               0xFF200040
#define KEY_BASE              0xFF200050
#define TIMER_BASE            0xFF202000
#define PIXEL_BUF_CTRL_BASE   0xFF203020
#define CHAR_BUF_CTRL_BASE    0xFF203030
#define PS2_BASE              0xFF200100
#define PS2_DUAL_BASE         0xFF200108

/* VGA colors */
#define WHITE 0xFFFF
#define YELLOW 0xFFE0
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define GREY 0xC618
#define PINK 0xFC18
#define ORANGE 0xF960
#define BLACK 0x0000
#define BROWN 0x8244
#define TAN 0xFF19
#define KONG 0x82e9
#define DBRWN 0x4140
#define LPLT 0xF800
#define DPLT 0xBA12
#define LAD 0x3FFF
#define DGREY 0x2945
#define GOLD 0xA3E0
#define LPINK 0xF475
#define DPINK 0xF9CF
#define HAIR 0xF68A
#define EBLUE 0x069F
#define CBLUE 0x047F
#define HPINK 0xFEFF
#define LORANGE 0xFC83

#define ABS(x) (((x) > 0) ? (x) : -(x))

/* Screen size. */
#define RESOLUTION_X 320
#define RESOLUTION_Y 240

// Platform constants
#define PLAT_WIDTH 16
#define PLAT_LENGTH 17
#define RUNG_HEIGHT 4
#define RUNG_WIDTH 7
	
// Collision Constants
#define LEVEL7 23
#define LEVEL6 55
#define LEVEL5 92
#define LEVEL4 129
#define LEVEL3 164
#define LEVEL2 201
#define LEVEL1 232
	
// Collision detectors
bool plat_reached[20];


#define FALSE 0
#define TRUE 1

#include <stdlib.h>
#include <stdio.h>
	
void draw_box(int x, int y, short int box_color);
void clear_edges();
void plot_pixel(int x, int y, short int line_color);
void clear_screen();
void draw_line(int x1, int y1, int x2, int y2, short int line_color);
void wait_for_vsync();
void draw_mario(int x, int y);
void draw_mario_backwards(int x, int y);
void draw_mario_run_backwards(int x, int y);
void draw_mario_run(int x, int y);
void clear_mario();
void draw_donkey(int x, int y);
void draw_peach(int x, int y);
void draw_fireball(int x, int y);
void draw_fireball_up(int x, int y);
void draw_fireball_back(int x, int y);
void clear_fireball(int x, int y);
void draw_platform(int x, int y);
void draw_level();
void draw_ladders();
void draw_story_up(int x, int y);
void draw_story_down(int x, int y);
void draw_story_flat(int x, int y);
void draw_ladder(int x, int y, int h);
void check_ladder_collision(int x, int y);
void check_collisions();
void conditional_redraw();
void victory_animation(int x, int y);
void end_game();
void lose_game();

// Global variables
// Mario
int mario_x = 30; // start x coord for mario
int mario_y = 216; // start y coord for mario
int mario_feet = 0; // y coordinate of marios feet
int mario_front = 0;
int mario_dx = 0;
int mario_dy = 0;
int mario_level = 1;
int mario_centre = 0;
bool on_ladder = false;
bool stepping = false;
bool forward = true;
int step_frames = 0;

// Donkey Kong
int donkey_x = 35; // start x coord for mario
int donkey_y = 29; // start y coord for mario

// Peach
int peach_x = 130;
int peach_y = 6;

// Fireballs
int fireball_x[5] = {312, 0, 312, 0, 312};
int fireball_y[5] = {64, 102, 136, 174, 210};
int fireball_dx[5] = {-1, 1, -1, 1, -1};
int fireball_dy[5] = {1, 1, 1, 1, 1};
int fireball_frames = 0;

// Level
int initial_level_x = 0;
int initial_level_y = 232;
int victory_x = 150;
int victory_y = 8;


volatile int pixel_buffer_start; // global variable

int main(void)
{
	srand(time(0));
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
	volatile int * PS2_ptr = (int *)PS2_BASE;
	int PS2_data, RVALID;
	char move_dir;
	// PS/2 mouse needs to be reset (must already be plugged in)
	*(PS2_ptr) = 0xFF; // reset

    // declare other variables(not shown)
    // initialize location and direction of rectangles(not shown)

    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the 
                                        // back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
	// pixel_buffer_start points to the pixel buffer
    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen(); 
	draw_level(initial_level_x, initial_level_y);
    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
	// pixel_buffer_start points to the pixel buffer
	// we draw on the back buffer and the front buffer displays while we draw
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); 
	// clear the screen initially
    clear_screen();
	draw_level(initial_level_x, initial_level_y);
	// setup collisions
	for(int i = 0; i < 20; i++){
		plat_reached[i] = false;
	}
	// set the initial position of the boxes
    while (1)
    {	
        /* Draw new frame */
		draw_ladders();
		conditional_redraw(initial_level_x, initial_level_y);
		// determine movement direction of mario by the switches
		PS2_data = *(PS2_ptr); // read the data register in the PS/2 port
		RVALID = PS2_data & 0x8000; // extract RVALID Field
		if(RVALID) {
			move_dir = PS2_data & 0xFF;
			if(move_dir == 0x74 && mario_x < 304){
				forward = true;
				mario_dx = 1;
				if(mario_feet <= 225 && mario_feet >= 197 && on_ladder == true || mario_feet <= 155 && mario_feet >= 125 && on_ladder == true || mario_feet <= 83 && mario_feet >= 51 && on_ladder == true || mario_feet <= 190 && mario_feet >= 162 && on_ladder == true || mario_feet <= 118 && mario_feet >= 90 && on_ladder == true || mario_feet <= 194 && mario_feet >= 158 && on_ladder == true || mario_feet <= 158 && mario_feet >= 122 && on_ladder == true || mario_feet <= 121 && mario_feet >= 87 && on_ladder == true || mario_feet <= 51 && mario_feet >= 22 && on_ladder == true)
					mario_dx = 0;
			}
			else if(move_dir == 0x6B && mario_x > 16){
				forward = false;
				mario_dx = -1;
				if(mario_feet <= 225 && mario_feet >= 197 && on_ladder == true || mario_feet <= 155 && mario_feet >= 125 && on_ladder == true || mario_feet <= 83 && mario_feet >= 51 && on_ladder == true || mario_feet <= 190 && mario_feet >= 162 && on_ladder == true || mario_feet <= 118 && mario_feet >= 90 && on_ladder == true || mario_feet <= 194 && mario_feet >= 158 && on_ladder == true || mario_feet <= 158 && mario_feet >= 122 && on_ladder == true || mario_feet <= 121 && mario_feet >= 87 && on_ladder == true || mario_feet <= 51 && mario_feet >= 22 && on_ladder == true)
					mario_dx = 0;
			}
			else if(move_dir == 0x75 && on_ladder)
				mario_dy = -1;
			else if(move_dir == 0x72 && on_ladder && (mario_feet != 49 && mario_feet != 81 && (mario_feet != 116 || mario_x > 85) && mario_feet != 119 && (mario_feet != 153 || mario_x < 185) && mario_feet != 156 && (mario_feet != 188 || mario_x > 85) && mario_feet != 192 && mario_feet != 225))
				mario_dy = 1;
			else {
				mario_dy = 0;
				mario_dx = 0;
			}
		}
		if(move_dir == 0x74 && mario_front == 304 || (move_dir == 0x74 && mario_level % 2 == 0 && mario_front == 271) || (move_dir == 0x74 && mario_level == 7 && mario_front == 192))
			mario_dx = 0;
		
		
		if((move_dir == 0x6B && mario_x == 16) || (move_dir == 0x6B && mario_level == 3 && mario_x == 48) || (move_dir == 0x6B && mario_level == 5 && mario_x == 48) || (move_dir == 0x6B && mario_level == 6 && mario_x == 128))
			mario_dx = 0;
		
		mario_x += mario_dx;
		mario_y += mario_dy;
		mario_feet = mario_y + 15;
		mario_front = mario_x + 12;
		mario_centre = mario_x + 7;
		
		if(stepping == false){
			if (forward == true)
				draw_mario(mario_x, mario_y);
			else
				draw_mario_backwards(mario_x, mario_y);
			if(mario_dx != 0){
				if(step_frames == 3){
					stepping = true;
					step_frames = 0;
				} else
					step_frames++;
			}
			} else {
				if (forward == true)
					draw_mario_run(mario_x, mario_y);
				else
					draw_mario_run_backwards(mario_x, mario_y);
				if(step_frames == 3){
					stepping = false;
					step_frames = 0;
				} else
					step_frames++;
				if(mario_dx == 0)
					stepping = false;
		}
		draw_peach(peach_x, peach_y);
		// fireball logic
		for(int i = 0; i < 4; i++) {
			// move the fireball in the x direction
			fireball_x[i] += fireball_dx[i];
			if(fireball_x[i] % 16 == 0)
				fireball_y[i] += fireball_dy[i];
		}
		for(int i = 0; i < 4; i++) {
			// draw the fireballs
			if(fireball_frames == 0 || fireball_frames == 1 || fireball_frames == 2)
				draw_fireball(fireball_x[i], fireball_y[i]);
			else if (fireball_frames == 3 || fireball_frames == 4 || fireball_frames == 5)
				draw_fireball_back(fireball_x[i], fireball_y[i]);
			else if (fireball_frames == 6 || fireball_frames == 7 || fireball_frames == 8)
				draw_fireball_up(fireball_x[i], fireball_y[i]);
			if(fireball_frames == 8)
				fireball_frames = 0;
		}
		fireball_frames++;
		// reset fireballs to continuously fire
		if(fireball_x[1] == 319) {
			fireball_y[0] = 64;
			fireball_y[1] = 102;
			fireball_y[2] = 136;
			fireball_y[3] = 174;
			fireball_x[0] = 312;
			fireball_x[1] = 0;
			fireball_x[2] = 312;
			fireball_x[3] = 0;
		}

		// draw borders to hide fireballs when they go offscreen
		if(fireball_x[1] >= 296 || fireball_x[1] <= 16) {
			for (int i = 0; i < 16; i++){
				for (int j = 0; j < 240; j++){
					plot_pixel(i, j, DGREY);
				}
			}
			for (int i = 304; i < 320; i++){
				for (int j = 0; j < 240; j++){
					plot_pixel(i, j, DGREY);
				}
			}
		}
		
		mario_dy = 0;
		
		// collision check for platforms
		check_collisions();
		
		// checks to see if mario can climb a ladder
		check_ladder_collision(mario_centre, mario_feet);
		
		// checks to see if mario has hit a fireball
		for(int i = 0; i < 4; i++){
			if((fireball_y[i] <= mario_feet && fireball_y[i] >= mario_y) && (fireball_x[i] <= mario_front && fireball_x[i] >= mario_x)) {
				lose_game();
				clear_screen(); 
				draw_level(initial_level_x, initial_level_y);
				// pixel_buffer_start points to the pixel buffer
				// we draw on the back buffer and the front buffer displays while we draw
				pixel_buffer_start = *(pixel_ctrl_ptr + 1); 
				// clear the screen initially
				clear_screen();
				draw_level(initial_level_x, initial_level_y);
				wait_for_vsync();
				fireball_y[0] = 64;
				fireball_y[1] = 102;
				fireball_y[2] = 136;
				fireball_y[3] = 174;
				fireball_x[0] = 312;
				fireball_x[1] = 0;
				fireball_x[2] = 312;
				fireball_x[3] = 0;
			}
		}
		
		// check which level mario is currently on now
		if (mario_feet <= 240 && mario_feet >= LEVEL1)
			mario_level = 1; 
		else if (mario_feet <= LEVEL1 && mario_feet >= LEVEL2)
			mario_level = 1; 
		else if (mario_feet <= LEVEL2 && mario_feet >= LEVEL3)
			mario_level = 2; 		
		else if (mario_feet <= LEVEL3 && mario_feet >= LEVEL4) 
			mario_level = 3; 
		else if (mario_feet <= LEVEL4 && mario_feet >= LEVEL5) 
			mario_level = 4; 
		else if (mario_feet <= LEVEL5 && mario_feet >= LEVEL6) 
			mario_level = 5; 		
		else if (mario_feet <= LEVEL6 && mario_feet >= LEVEL7) 
			mario_level = 6; 
		else if (mario_feet <= LEVEL7 && mario_feet >= 0){
			mario_level = 7;
		}
		else
			mario_level = 0;
		
		// victory animation
		if(mario_x == 171 && mario_level == 7){	
			victory_animation(victory_x, victory_y);
			end_game();
				clear_screen(); 
				draw_level(initial_level_x, initial_level_y);
				// pixel_buffer_start points to the pixel buffer
				// we draw on the back buffer and the front buffer displays while we draw
				pixel_buffer_start = *(pixel_ctrl_ptr + 1); 
				// clear the screen initially
				clear_screen();
				draw_level(initial_level_x, initial_level_y);
				wait_for_vsync();
				fireball_y[0] = 64;
				fireball_y[1] = 102;
				fireball_y[2] = 136;
				fireball_y[3] = 174;
				fireball_x[0] = 312;
				fireball_x[1] = 0;
				fireball_x[2] = 312;
				fireball_x[3] = 0;
		}
		
		/* Erase previous frame */	
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
		// clear marios previous frame
		clear_mario(mario_x - mario_dx, mario_y);
		clear_mario(mario_x - mario_dx, mario_y+1);
		clear_mario(mario_x - mario_dx, mario_y-1);
		// clear the fireballs previous frames
		for(int i = 0; i < 5; i++) {
			clear_fireball(fireball_x[i] - fireball_dx[i], fireball_y[i] - fireball_dy[i]);
		}
		
		 // ladder clearing and drawing mechanics	
		
		wait_for_vsync(); // swap front and back buffers on VGA vertical sync
    }
}

void plot_pixel(int x, int y, short int line_color)
{
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

void clear_screen()
{
	// blank the screen by iterating over every pixel on the VGA display
	for (int i = 0; i < 320; i++){
		for (int j = 0; j < 240; j++){
			plot_pixel(i, j, BLACK);
		}
	}
	return;
}
// draws a 3x3 box on the screen where the x and y coordinates are the center pixel
void draw_box(int x, int y, short int box_color)
{
	for(int i= 0; i < 3; i++){
		for(int j = 0; j < 3; j++){
			plot_pixel(i+x, j+y, box_color);
		}
	}
	return;
}
	
// function is fully commented in part 1
void draw_line(int x1, int y1, int x2, int y2, short int line_color)
{
	int is_steep = 0;
	if(ABS(y2 - y1) > ABS(x2 - x1)){
		is_steep = 1;
	}
	if (is_steep == 1){
		int temp = x1;
		x1 = y1;
		y1 = temp;
		temp = x2;
		x2 = y2;
		y2 = temp;
	}
	if(x1 > x2){
		int temp = x1;
		x1 = x2;
		x2 = temp;
		temp = y1;
		y1 = y2;
		y2 = temp;
	}
	
	// x2 will always be larger here
	int delta_x = x2 - x1;
	int delta_y = ABS(y2 - y1);
	int error = (delta_x / 2) * (-1);
	
	int y = y1;
	int y_step;
	
	if(y1 < y2){
		y_step = 1;
	} else {
		y_step = -1;
	}
	
	for (int x = x1; x <= x2; x++){
		// draw the line
		if(is_steep == 1){
			plot_pixel(y, x, line_color);
		} else {
			plot_pixel(x, y, line_color);
		}
		// increment the error by delta_x
		error = error + delta_y;
		
		// if the error is now above 0, move down the vga screen and decrement the error
		if (error > 0){
			y = y + y_step;
			error = error - delta_x;
		}
	}
	return;
}

// when a box bounces off the edge, this is called to clear all of the pixels around the
// perimeter of the vga to black
void clear_edges()
{
	for (int i = 0; i < 4; i++){
		draw_line(0, i, 319, i, BLACK);
	}
	for (int i = 0; i < 4; i++){
		draw_line(0, 239-i, 319, 239-i, BLACK);
	}
	for (int i = 0; i < 4; i++){
		draw_line(i, 0, i, 239, BLACK);
	}
	for (int i = 0; i < 4; i++){
		draw_line(319-i, 0, 319-i, 239, BLACK);
	}
}

void conditional_redraw(int x, int y)
{
	if (mario_level == 1){
		// initial platform
		draw_story_flat(x, y);
		// initial ramp
		for (int i = 0; i < 13; i++)
			draw_platform(x+(PLAT_WIDTH*i) + 112, y-i);
		draw_story_down(x, y-47);
	}
	if (mario_level == 2) {
		draw_story_down(x, y-47);
		draw_story_up(x+48, y-68);
	}
	if (mario_level == 3){
		draw_story_up(x+48, y-68);
		draw_story_down(x, y-119);
	}
	if (mario_level == 4){
		draw_story_down(x, y-119);
		draw_story_up(x+48, y-140);
	}
	if (mario_level == 5){
		draw_story_up(x+48, y-140);
		// final ramp
		for (int i = 0; i < 10; i++) {
			draw_platform(x+(PLAT_WIDTH*i) + 112, (y+i) - 186);
		}
	}
	if (mario_level == 6){
		// final ramp
		for (int i = 0; i < 10; i++) {
			draw_platform(x+(PLAT_WIDTH*i) + 112, (y+i) - 186);
		}
		// final platform
		draw_story_flat(x, y-186);
		// peach platform
		for (int i = 0; i < 4; i++) {
			draw_platform((x+128)+((PLAT_WIDTH)*i), y-209);
		}	
	}
	if (mario_level == 7) {
		// final ramp
		for (int i = 0; i < 10; i++) {
			draw_platform(x+(PLAT_WIDTH*i) + 112, (y+i) - 186);
		}
		// final platform
		draw_story_flat(x, y-186);
		// peach platform
		for (int i = 0; i < 4; i++) {
			draw_platform((x+128)+((PLAT_WIDTH)*i), y-209);
		}
	}
	
	// draw borders
	for (int i = 0; i < 16; i++){
		for (int j = 0; j < 240; j++){
			plot_pixel(i, j, DGREY);
		}
	}
	for (int i = 304; i < 320; i++){
		for (int j = 0; j < 240; j++){
			plot_pixel(i, j, DGREY);
		}
	}
	
	return;	
}

void draw_level(int x, int y)
{
	// initial platform
	draw_story_flat(x, y);
	// initial ramp
	for (int i = 0; i < 13; i++) {
		draw_platform(x+(PLAT_WIDTH*i) + 112, y-i);
	}
	// intermediate ramps
	draw_story_down(x, y-47);
	draw_story_up(x+48, y-68);
	draw_story_down(x, y-119);
	draw_story_up(x+48, y-140);
	
	// final ramp
	for (int i = 0; i < 10; i++) {
		draw_platform(x+(PLAT_WIDTH*i) + 112, (y+i) - 186);
	}
	// final platform
	draw_story_flat(x, y-186);
	
	// peach platform
	for (int i = 0; i < 4; i++) {
		draw_platform((x+128)+((PLAT_WIDTH)*i), y-209);
	}
	draw_ladders();
	draw_donkey(donkey_x, donkey_y);
	
	return;	
}

void draw_ladders()
{
// CLIMABLE ladders
	
	// right side ladders
	draw_ladder(212, 206, 19);
	draw_ladder(212, 134, 19);
	draw_ladder(212, 60, 21);
	
	// left side ladders
	draw_ladder(68, 171, 17);
	draw_ladder(68, 99, 17);
	
	// middle ladders
	draw_ladder(132, 167, 25);
	draw_ladder(164, 131, 25);
	draw_ladder(116, 96, 23);
	
	// peach ladder
	draw_ladder(180, 31, 18);
	
	// UNCLIMBABLE ladders
	draw_ladder(84, 224, 7);
	draw_ladder(84, 197, 6);
	draw_ladder(100, 127, 4);
	draw_ladder(100, 153, 7);
	draw_ladder(196, 91, 9);
	draw_ladder(196, 120, 4);

	return;
}

void check_ladder_collision(int x, int y)
{
	// ladder 1
	if (x >= 210 && x <= 212 + RUNG_WIDTH+2 && y <= 226 && y >= 198)
		on_ladder = true;
	// ladder 2
	else if (x >= 210 && x <= 212 + RUNG_WIDTH+2 && y <= 154 && y >= 126)
		on_ladder = true;
	// ladder 3
	else if (x >= 210 && x <= 212 + RUNG_WIDTH+2 && y <= 82 && y >= 52)
		on_ladder = true;
	// ladder 4
	else if (x >= 66 && x <= 68 + RUNG_WIDTH+2 && y <= 189 && y >= 163)
		on_ladder = true;
	// ladder 5
	else if (x >= 66 && x <= 68 + RUNG_WIDTH+2 && y <= 117 && y >= 91)
		on_ladder = true;
	// ladder 6
	else if (x >= 130 && x <= 132 + RUNG_WIDTH+2 && y <= 193 && y >= 159)
		on_ladder = true;
	// ladder 7
	else if (x >= 162 && x <= 164 + RUNG_WIDTH+2 && y <= 157 && y >= 123)
		on_ladder = true;
	// ladder 8
	else if (x >= 114 && x <= 116 + RUNG_WIDTH+2 && y <= 120 && y >= 88)
		on_ladder = true;
	// ladder 9
	else if (x >= 178 && x <= 180 + RUNG_WIDTH+2 && y <= 50 && y >= 23)
		on_ladder = true;
	else
		on_ladder = false;
	
	return;
}

void draw_story_up(int x, int y)
{
	for (int i = 0; i < PLAT_LENGTH; i++) {
		draw_platform(x+(PLAT_WIDTH*i), y-i);
	}

	return;	
}

void draw_story_down(int x, int y)
{
	for (int i = 0; i < PLAT_LENGTH; i++) {
		draw_platform(x+(PLAT_WIDTH*i), y+i);
	}

	return;	
}

void draw_story_flat(int x, int y)
{
	for (int i = 0; i < 7; i++) {
		draw_platform(x+((PLAT_WIDTH)*i), y);
	}

	return;	
}

void draw_ladder(int x, int y, int h)
{
	draw_line(x, y, x, y+h, LAD);
	draw_line(x+RUNG_WIDTH, y, x+RUNG_WIDTH, y+h, LAD);
	
	for (int i = 0; (i < (h/3)) && (i*RUNG_HEIGHT < h); i++) {
		draw_line(x, (y+1)+(RUNG_HEIGHT*i), x+RUNG_WIDTH, (y+1)+(RUNG_HEIGHT*i), LAD);
	}
	return;
}


void draw_mario(int x, int y)
{
	// Marios drawing starts from the top left coordinate of his character
	// ROW 1
	plot_pixel(x+3, y, RED); plot_pixel(x+4, y, RED); plot_pixel(x+5, y, RED); plot_pixel(x+6, y, RED); plot_pixel(x+7, y, RED); plot_pixel(x+8, y, RED);
	// ROW 2
	plot_pixel(x+2, y+1, RED); plot_pixel(x+3, y+1, RED); plot_pixel(x+4, y+1, RED); plot_pixel(x+5, y+1, RED); plot_pixel(x+6, y+1, RED); plot_pixel(x+7, y+1, RED); plot_pixel(x+8, y+1, RED); plot_pixel(x+9, y+1, RED); plot_pixel(x+10, y+1, RED); plot_pixel(x+11, y+1, RED);
	// ROW 3
	plot_pixel(x+2, y+2, BROWN); plot_pixel(x+3, y+2, BROWN); plot_pixel(x+4, y+2, BROWN); plot_pixel(x+5, y+2, TAN); plot_pixel(x+6, y+2, TAN); plot_pixel(x+7, y+2, TAN); plot_pixel(x+8, y+2, BLACK); plot_pixel(x+9, y+2, TAN);
	// ROW 4
	plot_pixel(x+1, y+3, BROWN); plot_pixel(x+2, y+3, TAN); plot_pixel(x+3, y+3, BROWN); plot_pixel(x+4, y+3, TAN); plot_pixel(x+5, y+3, TAN); plot_pixel(x+6, y+3, TAN); plot_pixel(x+7, y+3, TAN); plot_pixel(x+8, y+3, BLACK); plot_pixel(x+9, y+3, TAN); plot_pixel(x+10, y+3, TAN); plot_pixel(x+11, y+3, TAN);
	// ROW 5
	plot_pixel(x+1, y+4, BROWN); plot_pixel(x+2, y+4, TAN); plot_pixel(x+3, y+4, BROWN); plot_pixel(x+4, y+4, BROWN); plot_pixel(x+5, y+4, TAN); plot_pixel(x+6, y+4, TAN); plot_pixel(x+7, y+4, TAN); plot_pixel(x+8, y+4, TAN); plot_pixel(x+9, y+4, BLACK); plot_pixel(x+10, y+4, TAN); plot_pixel(x+11, y+4, TAN); plot_pixel(x+12, y+4, TAN);
	// ROW 6
	plot_pixel(x+1, y+5, BROWN); plot_pixel(x+2, y+5, BROWN); plot_pixel(x+3, y+5, TAN); plot_pixel(x+4, y+5, TAN); plot_pixel(x+5, y+5, TAN); plot_pixel(x+6, y+5, TAN); plot_pixel(x+7, y+5, TAN); plot_pixel(x+8, y+5, BLACK); plot_pixel(x+9, y+5, BLACK); plot_pixel(x+10, y+5, BLACK); plot_pixel(x+11, y+5, BLACK);
	// ROW 7
	plot_pixel(x+3, y+6, TAN); plot_pixel(x+4, y+6, TAN); plot_pixel(x+5, y+6, TAN); plot_pixel(x+6, y+6, TAN); plot_pixel(x+7, y+6, TAN); plot_pixel(x+8, y+6, TAN); plot_pixel(x+9, y+6, TAN); plot_pixel(x+10, y+6, TAN);
	// ROW 8
	plot_pixel(x+2, y+7, RED); plot_pixel(x+3, y+7, RED); plot_pixel(x+4, y+7, BLUE); plot_pixel(x+5, y+7, RED); plot_pixel(x+6, y+7, RED); plot_pixel(x+7, y+7, RED); plot_pixel(x+8, y+7, RED);
	// ROW 9
	plot_pixel(x+1, y+8, RED); plot_pixel(x+2, y+8, RED); plot_pixel(x+3, y+8, RED); plot_pixel(x+4, y+8, BLUE); plot_pixel(x+5, y+8, RED); plot_pixel(x+6, y+8, RED); plot_pixel(x+7, y+8, BLUE); plot_pixel(x+8, y+8, RED); plot_pixel(x+9, y+8, RED); plot_pixel(x+10, y+8, RED);
	// ROW 10
	plot_pixel(x, y+9, RED); plot_pixel(x+1, y+9, RED); plot_pixel(x+2, y+9, RED); plot_pixel(x+3, y+9, RED); plot_pixel(x+4, y+9, BLUE); plot_pixel(x+5, y+9, BLUE); plot_pixel(x+6, y+9, BLUE); plot_pixel(x+7, y+9, BLUE); plot_pixel(x+8, y+9, RED); plot_pixel(x+9, y+9, RED); plot_pixel(x+10, y+9, RED); plot_pixel(x+11, y+9, RED);
	// ROW 11
	plot_pixel(x, y+10, TAN); plot_pixel(x+1, y+10, TAN); plot_pixel(x+2, y+10, RED); plot_pixel(x+3, y+10, BLUE); plot_pixel(x+4, y+10, YELLOW); plot_pixel(x+5, y+10, BLUE); plot_pixel(x+6, y+10, BLUE); plot_pixel(x+7, y+10, YELLOW); plot_pixel(x+8, y+10, BLUE); plot_pixel(x+9, y+10, RED); plot_pixel(x+10, y+10, TAN); plot_pixel(x+11, y+10, TAN);
	// ROW 12
	plot_pixel(x, y+11, TAN); plot_pixel(x+1, y+11, TAN); plot_pixel(x+2, y+11, TAN); plot_pixel(x+3, y+11, BLUE); plot_pixel(x+4, y+11, BLUE); plot_pixel(x+5, y+11, BLUE); plot_pixel(x+6, y+11, BLUE); plot_pixel(x+7, y+11, BLUE); plot_pixel(x+8, y+11, BLUE); plot_pixel(x+9, y+11, TAN); plot_pixel(x+10, y+11, TAN); plot_pixel(x+11, y+11, TAN);
	// ROW 13
	plot_pixel(x, y+12, TAN); plot_pixel(x+1, y+12, TAN); plot_pixel(x+2, y+12, BLUE); plot_pixel(x+3, y+12, BLUE); plot_pixel(x+4, y+12, BLUE); plot_pixel(x+5, y+12, BLUE); plot_pixel(x+6, y+12, BLUE); plot_pixel(x+7, y+12, BLUE); plot_pixel(x+8, y+12, BLUE); plot_pixel(x+9, y+12, BLUE); plot_pixel(x+10, y+12, TAN); plot_pixel(x+11, y+12, TAN);
	// ROW 14
	plot_pixel(x+2, y+13, BLUE); plot_pixel(x+3, y+13, BLUE); plot_pixel(x+4, y+13, BLUE); plot_pixel(x+7, y+13, BLUE); plot_pixel(x+8, y+13, BLUE); plot_pixel(x+9, y+13, BLUE);
	// ROW 15
	plot_pixel(x+1, y+14, BROWN); plot_pixel(x+2, y+14, BROWN); plot_pixel(x+3, y+14, BROWN); plot_pixel(x+8, y+14, BROWN); plot_pixel(x+9, y+14, BROWN); plot_pixel(x+10, y+14, BROWN);
	// ROW 16
	plot_pixel(x, y+15, BROWN); plot_pixel(x+1, y+15, BROWN); plot_pixel(x+2, y+15, BROWN); plot_pixel(x+3, y+15, BROWN); plot_pixel(x+8, y+15, BROWN); plot_pixel(x+9, y+15, BROWN); plot_pixel(x+10, y+15, BROWN); plot_pixel(x+11, y+15, BROWN);
	
	return;
}

void draw_mario_backwards(int x, int y)
{
	x = x + 12;
	// Marios drawing starts from the top left coordinate of his character
	// ROW 1
	plot_pixel(x-3, y, RED); plot_pixel(x-4, y, RED); plot_pixel(x-5, y, RED); plot_pixel(x-6, y, RED); plot_pixel(x-7, y, RED); plot_pixel(x-8, y, RED);
	// ROW 2
	plot_pixel(x-2, y+1, RED); plot_pixel(x-3, y+1, RED); plot_pixel(x-4, y+1, RED); plot_pixel(x-5, y+1, RED); plot_pixel(x-6, y+1, RED); plot_pixel(x-7, y+1, RED); plot_pixel(x-8, y+1, RED); plot_pixel(x-9, y+1, RED); plot_pixel(x-10, y+1, RED); plot_pixel(x-11, y+1, RED);
	// ROW 3
	plot_pixel(x-2, y+2, BROWN); plot_pixel(x-3, y+2, BROWN); plot_pixel(x-4, y+2, BROWN); plot_pixel(x-5, y+2, TAN); plot_pixel(x-6, y+2, TAN); plot_pixel(x-7, y+2, TAN); plot_pixel(x-8, y+2, BLACK); plot_pixel(x-9, y+2, TAN);
	// ROW 4
	plot_pixel(x-1, y+3, BROWN); plot_pixel(x-2, y+3, TAN); plot_pixel(x-3, y+3, BROWN); plot_pixel(x-4, y+3, TAN); plot_pixel(x-5, y+3, TAN); plot_pixel(x-6, y+3, TAN); plot_pixel(x-7, y+3, TAN); plot_pixel(x-8, y+3, BLACK); plot_pixel(x-9, y+3, TAN); plot_pixel(x-10, y+3, TAN); plot_pixel(x-11, y+3, TAN);
	// ROW 5
	plot_pixel(x-1, y+4, BROWN); plot_pixel(x-2, y+4, TAN); plot_pixel(x-3, y+4, BROWN); plot_pixel(x-4, y+4, BROWN); plot_pixel(x-5, y+4, TAN); plot_pixel(x-6, y+4, TAN); plot_pixel(x-7, y+4, TAN); plot_pixel(x-8, y+4, TAN); plot_pixel(x-9, y+4, BLACK); plot_pixel(x-10, y+4, TAN); plot_pixel(x-11, y+4, TAN); plot_pixel(x-12, y+4, TAN);
	// ROW 6
	plot_pixel(x-1, y+5, BROWN); plot_pixel(x-2, y+5, BROWN); plot_pixel(x-3, y+5, TAN); plot_pixel(x-4, y+5, TAN); plot_pixel(x-5, y+5, TAN); plot_pixel(x-6, y+5, TAN); plot_pixel(x-7, y+5, TAN); plot_pixel(x-8, y+5, BLACK); plot_pixel(x-9, y+5, BLACK); plot_pixel(x-10, y+5, BLACK); plot_pixel(x-11, y+5, BLACK);
	// ROW 7
	plot_pixel(x-3, y+6, TAN); plot_pixel(x-4, y+6, TAN); plot_pixel(x-5, y+6, TAN); plot_pixel(x-6, y+6, TAN); plot_pixel(x-7, y+6, TAN); plot_pixel(x-8, y+6, TAN); plot_pixel(x-9, y+6, TAN); plot_pixel(x-10, y+6, TAN);
	// ROW 8
	plot_pixel(x-2, y+7, RED); plot_pixel(x-3, y+7, RED); plot_pixel(x-4, y+7, BLUE); plot_pixel(x-5, y+7, RED); plot_pixel(x-6, y+7, RED); plot_pixel(x-7, y+7, RED); plot_pixel(x-8, y+7, RED);
	// ROW 9
	plot_pixel(x-1, y+8, RED); plot_pixel(x-2, y+8, RED); plot_pixel(x-3, y+8, RED); plot_pixel(x-4, y+8, BLUE); plot_pixel(x-5, y+8, RED); plot_pixel(x-6, y+8, RED); plot_pixel(x-7, y+8, BLUE); plot_pixel(x-8, y+8, RED); plot_pixel(x-9, y+8, RED); plot_pixel(x-10, y+8, RED);
	// ROW 10
	plot_pixel(x, y+9, RED); plot_pixel(x-1, y+9, RED); plot_pixel(x-2, y+9, RED); plot_pixel(x-3, y+9, RED); plot_pixel(x-4, y+9, BLUE); plot_pixel(x-5, y+9, BLUE); plot_pixel(x-6, y+9, BLUE); plot_pixel(x-7, y+9, BLUE); plot_pixel(x-8, y+9, RED); plot_pixel(x-9, y+9, RED); plot_pixel(x-10, y+9, RED); plot_pixel(x-11, y+9, RED);
	// ROW 11
	plot_pixel(x, y+10, TAN); plot_pixel(x-1, y+10, TAN); plot_pixel(x-2, y+10, RED); plot_pixel(x-3, y+10, BLUE); plot_pixel(x-4, y+10, YELLOW); plot_pixel(x-5, y+10, BLUE); plot_pixel(x-6, y+10, BLUE); plot_pixel(x-7, y+10, YELLOW); plot_pixel(x-8, y+10, BLUE); plot_pixel(x-9, y+10, RED); plot_pixel(x-10, y+10, TAN); plot_pixel(x-11, y+10, TAN);
	// ROW 12
	plot_pixel(x, y+11, TAN); plot_pixel(x-1, y+11, TAN); plot_pixel(x-2, y+11, TAN); plot_pixel(x-3, y+11, BLUE); plot_pixel(x-4, y+11, BLUE); plot_pixel(x-5, y+11, BLUE); plot_pixel(x-6, y+11, BLUE); plot_pixel(x-7, y+11, BLUE); plot_pixel(x-8, y+11, BLUE); plot_pixel(x-9, y+11, TAN); plot_pixel(x-10, y+11, TAN); plot_pixel(x-11, y+11, TAN);
	// ROW 13
	plot_pixel(x, y+12, TAN); plot_pixel(x-1, y+12, TAN); plot_pixel(x-2, y+12, BLUE); plot_pixel(x-3, y+12, BLUE); plot_pixel(x-4, y+12, BLUE); plot_pixel(x-5, y+12, BLUE); plot_pixel(x-6, y+12, BLUE); plot_pixel(x-7, y+12, BLUE); plot_pixel(x-8, y+12, BLUE); plot_pixel(x-9, y+12, BLUE); plot_pixel(x-10, y+12, TAN); plot_pixel(x-11, y+12, TAN);
	// ROW 14
	plot_pixel(x-2, y+13, BLUE); plot_pixel(x-3, y+13, BLUE); plot_pixel(x-4, y+13, BLUE); plot_pixel(x-7, y+13, BLUE); plot_pixel(x-8, y+13, BLUE); plot_pixel(x-9, y+13, BLUE);
	// ROW 15
	plot_pixel(x-1, y+14, BROWN); plot_pixel(x-2, y+14, BROWN); plot_pixel(x-3, y+14, BROWN); plot_pixel(x-8, y+14, BROWN); plot_pixel(x-9, y+14, BROWN); plot_pixel(x-10, y+14, BROWN);
	// ROW 16
	plot_pixel(x, y+15, BROWN); plot_pixel(x-1, y+15, BROWN); plot_pixel(x-2, y+15, BROWN); plot_pixel(x-3, y+15, BROWN); plot_pixel(x-8, y+15, BROWN); plot_pixel(x-9, y+15, BROWN); plot_pixel(x-10, y+15, BROWN); plot_pixel(x-11, y+15, BROWN);
	
	return;
	
}

void draw_mario_run(int x, int y)
{
	x = x + 3;
	// ROW 1
	plot_pixel(x+3, y, RED); plot_pixel(x+4, y, RED); plot_pixel(x+5, y, RED); plot_pixel(x+6, y, RED); plot_pixel(x+7, y, RED); plot_pixel(x+8, y, RED);
	// ROW 2
	plot_pixel(x+2, y+1, RED); plot_pixel(x+3, y+1, RED); plot_pixel(x+4, y+1, RED); plot_pixel(x+5, y+1, RED); plot_pixel(x+6, y+1, RED); plot_pixel(x+7, y+1, RED); plot_pixel(x+8, y+1, RED); plot_pixel(x+9, y+1, RED); plot_pixel(x+10, y+1, RED); plot_pixel(x+11, y+1, RED);
	// ROW 3
	plot_pixel(x+2, y+2, BROWN); plot_pixel(x+3, y+2, BROWN); plot_pixel(x+4, y+2, BROWN); plot_pixel(x+5, y+2, TAN); plot_pixel(x+6, y+2, TAN); plot_pixel(x+7, y+2, TAN); plot_pixel(x+8, y+2, BLACK); plot_pixel(x+9, y+2, TAN);
	// ROW 4
	plot_pixel(x+1, y+3, BROWN); plot_pixel(x+2, y+3, TAN); plot_pixel(x+3, y+3, BROWN); plot_pixel(x+4, y+3, TAN); plot_pixel(x+5, y+3, TAN); plot_pixel(x+6, y+3, TAN); plot_pixel(x+7, y+3, TAN); plot_pixel(x+8, y+3, BLACK); plot_pixel(x+9, y+3, TAN); plot_pixel(x+10, y+3, TAN); plot_pixel(x+11, y+3, TAN);
	// ROW 5
	plot_pixel(x+1, y+4, BROWN); plot_pixel(x+2, y+4, TAN); plot_pixel(x+3, y+4, BROWN); plot_pixel(x+4, y+4, BROWN); plot_pixel(x+5, y+4, TAN); plot_pixel(x+6, y+4, TAN); plot_pixel(x+7, y+4, TAN); plot_pixel(x+8, y+4, TAN); plot_pixel(x+9, y+4, BLACK); plot_pixel(x+10, y+4, TAN); plot_pixel(x+11, y+4, TAN); plot_pixel(x+12, y+4, TAN);
	// ROW 6
	plot_pixel(x+1, y+5, BROWN); plot_pixel(x+2, y+5, BROWN); plot_pixel(x+3, y+5, TAN); plot_pixel(x+4, y+5, TAN); plot_pixel(x+5, y+5, TAN); plot_pixel(x+6, y+5, TAN); plot_pixel(x+7, y+5, TAN); plot_pixel(x+8, y+5, BLACK); plot_pixel(x+9, y+5, BLACK); plot_pixel(x+10, y+5, BLACK); plot_pixel(x+11, y+5, BLACK);
	// ROW 7
	plot_pixel(x+3, y+6, TAN); plot_pixel(x+4, y+6, TAN); plot_pixel(x+5, y+6, TAN); plot_pixel(x+6, y+6, TAN); plot_pixel(x+7, y+6, TAN); plot_pixel(x+8, y+6, TAN); plot_pixel(x+9, y+6, TAN); plot_pixel(x+10, y+6, TAN);
	// ROW 8
	plot_pixel(x-1, y+7, RED); plot_pixel(x, y+7, RED); plot_pixel(x+1, y+7, RED); plot_pixel(x+2, y+7, RED); plot_pixel(x+3, y+7, BLUE); plot_pixel(x+4, y+7, RED); plot_pixel(x+5, y+7, RED); plot_pixel(x+6, y+7, RED); plot_pixel(x+7, y+7, BLUE); 
	// ROW 9
	plot_pixel(x-3, y+8, TAN); plot_pixel(x-2, y+8, TAN); plot_pixel(x-1, y+8, RED); plot_pixel(x, y+8, RED); plot_pixel(x+1, y+8, RED); plot_pixel(x+2, y+8, RED); plot_pixel(x+3, y+8, BLUE); plot_pixel(x+4, y+8, BLUE); plot_pixel(x+5, y+8, RED); plot_pixel(x+6, y+8, RED); plot_pixel(x+7, y+8, RED); plot_pixel(x+8, y+8, BLUE); plot_pixel(x+9, y+8, RED); plot_pixel(x+10, y+8, TAN); plot_pixel(x+11, y+8, TAN); plot_pixel(x+12, y+8, TAN);
	// ROW 10
	plot_pixel(x-3, y+9, TAN); plot_pixel(x-2, y+9, TAN); plot_pixel(x-1, y+9, TAN); plot_pixel(x+1, y+9, RED); plot_pixel(x+2, y+9, RED); plot_pixel(x+3, y+9, BLUE); plot_pixel(x+4, y+9, BLUE); plot_pixel(x+5, y+9, BLUE); plot_pixel(x+6, y+9, BLUE); plot_pixel(x+7, y+9, BLUE); plot_pixel(x+8, y+9, BLUE); plot_pixel(x+9, y+9, RED); plot_pixel(x+10, y+9, RED); plot_pixel(x+11, y+9, TAN); plot_pixel(x+12, y+9, TAN);
	// ROW 11
	plot_pixel(x-3, y+10, TAN); plot_pixel(x-2, y+10, TAN); plot_pixel(x+1, y+10, BLUE); plot_pixel(x+2, y+10, BLUE); plot_pixel(x+3, y+10, BLUE); plot_pixel(x+4, y+10, YELLOW); plot_pixel(x+5, y+10, BLUE); plot_pixel(x+6, y+10, BLUE); plot_pixel(x+7, y+10, BLUE); plot_pixel(x+8, y+10, YELLOW); plot_pixel(x+11, y+10, BROWN);
	// ROW 12
	plot_pixel(x, y+11, BLUE); plot_pixel(x+1, y+11, BLUE); plot_pixel(x+2, y+11, BLUE); plot_pixel(x+3, y+11, BLUE); plot_pixel(x+4, y+11, BLUE); plot_pixel(x+5, y+11, BLUE); plot_pixel(x+6, y+11, BLUE); plot_pixel(x+7, y+11, BLUE); plot_pixel(x+8, y+11, BLUE); plot_pixel(x+9, y+11, BLUE); plot_pixel(x+10, y+11, BROWN); plot_pixel(x+11, y+11, BROWN);
	// ROW 13
	plot_pixel(x-1, y+12, BLUE); plot_pixel(x, y+12, BLUE); plot_pixel(x+1, y+12, BLUE); plot_pixel(x+2, y+12, BLUE); plot_pixel(x+3, y+12, BLUE); plot_pixel(x+4, y+12, BLUE); plot_pixel(x+5, y+12, BLUE); plot_pixel(x+6, y+12, BLUE); plot_pixel(x+7, y+12, BLUE); plot_pixel(x+8, y+12, BLUE); plot_pixel(x+9, y+12, BLUE); plot_pixel(x+10, y+12, BROWN); plot_pixel(x+11, y+12, BROWN);
	// ROW 14
	plot_pixel(x-2, y+13, BROWN); plot_pixel(x-1, y+13, BROWN); plot_pixel(x, y+13, BLUE); plot_pixel(x+1, y+13, BLUE); plot_pixel(x+2, y+13, BLUE); plot_pixel(x+7, y+13, BLUE); plot_pixel(x+8, y+13, BLUE); plot_pixel(x+9, y+13, BLUE); plot_pixel(x+10, y+13, BROWN); plot_pixel(x+11, y+13, BROWN);
	// ROW 15
	plot_pixel(x-2, y+14, BROWN); plot_pixel(x-1, y+14, BROWN); plot_pixel(x, y+14, BROWN);
	// ROW 16
	plot_pixel(x-1, y+15, BROWN); plot_pixel(x, y+15, BROWN); plot_pixel(x+1, y+15, BROWN);
		
	return;
}

void draw_mario_run_backwards(int x, int y)
{
	x = x + 12;
	// ROW 1
	plot_pixel(x-3, y, RED); plot_pixel(x-4, y, RED); plot_pixel(x-5, y, RED); plot_pixel(x-6, y, RED); plot_pixel(x-7, y, RED); plot_pixel(x-8, y, RED);
	// ROW 2
	plot_pixel(x-2, y+1, RED); plot_pixel(x-3, y+1, RED); plot_pixel(x-4, y+1, RED); plot_pixel(x-5, y+1, RED); plot_pixel(x-6, y+1, RED); plot_pixel(x-7, y+1, RED); plot_pixel(x-8, y+1, RED); plot_pixel(x-9, y+1, RED); plot_pixel(x-10, y+1, RED); plot_pixel(x-11, y+1, RED);
	// ROW 3
	plot_pixel(x-2, y+2, BROWN); plot_pixel(x-3, y+2, BROWN); plot_pixel(x-4, y+2, BROWN); plot_pixel(x-5, y+2, TAN); plot_pixel(x-6, y+2, TAN); plot_pixel(x-7, y+2, TAN); plot_pixel(x-8, y+2, BLACK); plot_pixel(x-9, y+2, TAN);
	// ROW 4
	plot_pixel(x-1, y+3, BROWN); plot_pixel(x-2, y+3, TAN); plot_pixel(x-3, y+3, BROWN); plot_pixel(x-4, y+3, TAN); plot_pixel(x-5, y+3, TAN); plot_pixel(x-6, y+3, TAN); plot_pixel(x-7, y+3, TAN); plot_pixel(x-8, y+3, BLACK); plot_pixel(x-9, y+3, TAN); plot_pixel(x-10, y+3, TAN); plot_pixel(x-11, y+3, TAN);
	// ROW 5
	plot_pixel(x-1, y+4, BROWN); plot_pixel(x-2, y+4, TAN); plot_pixel(x-3, y+4, BROWN); plot_pixel(x-4, y+4, BROWN); plot_pixel(x-5, y+4, TAN); plot_pixel(x-6, y+4, TAN); plot_pixel(x-7, y+4, TAN); plot_pixel(x-8, y+4, TAN); plot_pixel(x-9, y+4, BLACK); plot_pixel(x-10, y+4, TAN); plot_pixel(x-11, y+4, TAN); plot_pixel(x-12, y+4, TAN);
	// ROW 6
	plot_pixel(x-1, y+5, BROWN); plot_pixel(x-2, y+5, BROWN); plot_pixel(x-3, y+5, TAN); plot_pixel(x-4, y+5, TAN); plot_pixel(x-5, y+5, TAN); plot_pixel(x-6, y+5, TAN); plot_pixel(x-7, y+5, TAN); plot_pixel(x-8, y+5, BLACK); plot_pixel(x-9, y+5, BLACK); plot_pixel(x-10, y+5, BLACK); plot_pixel(x-11, y+5, BLACK);
	// ROW 7
	plot_pixel(x-3, y+6, TAN); plot_pixel(x-4, y+6, TAN); plot_pixel(x-5, y+6, TAN); plot_pixel(x-6, y+6, TAN); plot_pixel(x-7, y+6, TAN); plot_pixel(x-8, y+6, TAN); plot_pixel(x-9, y+6, TAN); plot_pixel(x-10, y+6, TAN);
	// ROW 8
	plot_pixel(x+1, y+7, RED); plot_pixel(x, y+7, RED); plot_pixel(x-1, y+7, RED); plot_pixel(x-2, y+7, RED); plot_pixel(x-3, y+7, BLUE); plot_pixel(x-4, y+7, RED); plot_pixel(x-5, y+7, RED); plot_pixel(x-6, y+7, RED); plot_pixel(x-7, y+7, BLUE); 
	// ROW 9
	plot_pixel(x+3, y+8, TAN); plot_pixel(x+2, y+8, TAN); plot_pixel(x+1, y+8, RED); plot_pixel(x, y+8, RED); plot_pixel(x-1, y+8, RED); plot_pixel(x-2, y+8, RED); plot_pixel(x-3, y+8, BLUE); plot_pixel(x-4, y+8, BLUE); plot_pixel(x-5, y+8, RED); plot_pixel(x-6, y+8, RED); plot_pixel(x-7, y+8, RED); plot_pixel(x-8, y+8, BLUE); plot_pixel(x-9, y+8, RED); plot_pixel(x-10, y+8, TAN); plot_pixel(x-11, y+8, TAN); plot_pixel(x-12, y+8, TAN);
	// ROW 10
	plot_pixel(x+3, y+9, TAN); plot_pixel(x+2, y+9, TAN); plot_pixel(x+1, y+9, TAN); plot_pixel(x-1, y+9, RED); plot_pixel(x-2, y+9, RED); plot_pixel(x-3, y+9, BLUE); plot_pixel(x-4, y+9, BLUE); plot_pixel(x-5, y+9, BLUE); plot_pixel(x-6, y+9, BLUE); plot_pixel(x-7, y+9, BLUE); plot_pixel(x-8, y+9, BLUE); plot_pixel(x-9, y+9, RED); plot_pixel(x-10, y+9, RED); plot_pixel(x-11, y+9, TAN); plot_pixel(x-12, y+9, TAN);
	// ROW 11
	plot_pixel(x+3, y+10, TAN); plot_pixel(x+2, y+10, TAN); plot_pixel(x-1, y+10, BLUE); plot_pixel(x-2, y+10, BLUE); plot_pixel(x-3, y+10, BLUE); plot_pixel(x-4, y+10, YELLOW); plot_pixel(x-5, y+10, BLUE); plot_pixel(x-6, y+10, BLUE); plot_pixel(x-7, y+10, BLUE); plot_pixel(x-8, y+10, YELLOW); plot_pixel(x-11, y+10, BROWN);
	// ROW 12
	plot_pixel(x, y+11, BLUE); plot_pixel(x-1, y+11, BLUE); plot_pixel(x-2, y+11, BLUE); plot_pixel(x-3, y+11, BLUE); plot_pixel(x-4, y+11, BLUE); plot_pixel(x-5, y+11, BLUE); plot_pixel(x-6, y+11, BLUE); plot_pixel(x-7, y+11, BLUE); plot_pixel(x-8, y+11, BLUE); plot_pixel(x-9, y+11, BLUE); plot_pixel(x-10, y+11, BROWN); plot_pixel(x-11, y+11, BROWN);
	// ROW 13
	plot_pixel(x+1, y+12, BLUE); plot_pixel(x, y+12, BLUE); plot_pixel(x-1, y+12, BLUE); plot_pixel(x-2, y+12, BLUE); plot_pixel(x-3, y+12, BLUE); plot_pixel(x-4, y+12, BLUE); plot_pixel(x-5, y+12, BLUE); plot_pixel(x-6, y+12, BLUE); plot_pixel(x-7, y+12, BLUE); plot_pixel(x-8, y+12, BLUE); plot_pixel(x-9, y+12, BLUE); plot_pixel(x-10, y+12, BROWN); plot_pixel(x-11, y+12, BROWN);
	// ROW 14
	plot_pixel(x+2, y+13, BROWN); plot_pixel(x+1, y+13, BROWN); plot_pixel(x, y+13, BLUE); plot_pixel(x-1, y+13, BLUE); plot_pixel(x-2, y+13, BLUE); plot_pixel(x-7, y+13, BLUE); plot_pixel(x-8, y+13, BLUE); plot_pixel(x-9, y+13, BLUE); plot_pixel(x-10, y+13, BROWN); plot_pixel(x-11, y+13, BROWN);
	// ROW 15
	plot_pixel(x+2, y+14, BROWN); plot_pixel(x+1, y+14, BROWN); plot_pixel(x, y+14, BROWN);
	// ROW 16
	plot_pixel(x+1, y+15, BROWN); plot_pixel(x, y+15, BROWN); plot_pixel(x-1, y+15, BROWN);

	return;
}

void draw_donkey(int x, int y)
{
	// set body
	for(int i= 0; i < 18; i++){
		for(int j = 0; j < 17; j++){
			plot_pixel(i+x, j+y, KONG);
		}
	}
	// ROW 1
	plot_pixel(x+0, y+0, BLACK); plot_pixel(x+1, y+0, BLACK); plot_pixel(x+2, y+0, BLACK); plot_pixel(x+3, y+0, BLACK); plot_pixel(x+4, y+0, BLACK); plot_pixel(x+5, y+0, BLACK); plot_pixel(x+6, y+0, BLACK); plot_pixel(x+7, y+0, BLACK); plot_pixel(x+8, y+0, BLACK); plot_pixel(x+9, y+0, BLACK); plot_pixel(x+14, y+0, BLACK); plot_pixel(x+15, y+0, BLACK); plot_pixel(x+16, y+0, BLACK); plot_pixel(x+17, y+0, BLACK);
	// ROW 2
    plot_pixel(x+0, y+1, BLACK); plot_pixel(x+1, y+1, BLACK); plot_pixel(x+2, y+1, BLACK); plot_pixel(x+3, y+1, BLACK); plot_pixel(x+4, y+1, BLACK); plot_pixel(x+5, y+1, BLACK); plot_pixel(x+6, y+1, BLACK); plot_pixel(x+7, y+1, BLACK); plot_pixel(x+8, y+1, BLACK); plot_pixel(x+13, y+1, BLACK); plot_pixel(x+14, y+1, BLACK); plot_pixel(x+15, y+1, BLACK); plot_pixel(x+16, y+1, BLACK); plot_pixel(x+17, y+1, BLACK);
	// ROW 3
    plot_pixel(x+0, y+2, BLACK); plot_pixel(x+1, y+2, BLACK); plot_pixel(x+2, y+2, BLACK); plot_pixel(x+3, y+2, BLACK); plot_pixel(x+4, y+2, BLACK); plot_pixel(x+5, y+2, BLACK); plot_pixel(x+6, y+2, BLACK); plot_pixel(x+7, y+2, BLACK); plot_pixel(x+15, y+2, BLACK); plot_pixel(x+16, y+2, BLACK); plot_pixel(x+17, y+2, BLACK);
	// ROW 4
    plot_pixel(x+0, y+3, BLACK); plot_pixel(x+1, y+3, BLACK); plot_pixel(x+2, y+3, BLACK); plot_pixel(x+3, y+3, BLACK); plot_pixel(x+4, y+3, BLACK); plot_pixel(x+7, y+3, RED); plot_pixel(x+13, y+3, TAN); plot_pixel(x+14, y+3, TAN); plot_pixel(x+15, y+3, BLACK); plot_pixel(x+16, y+3, BLACK); plot_pixel(x+17, y+3, BLACK);
	// ROW 5
	plot_pixel(x+0, y+4, BLACK); plot_pixel(x+1, y+4, BLACK); plot_pixel(x+2, y+4, BLACK); plot_pixel(x+3, y+4, BLACK); plot_pixel(x+7, y+4, RED); plot_pixel(x+10, y+4, TAN); plot_pixel(x+11, y+4, TAN); plot_pixel(x+13, y+4, GREY); plot_pixel(x+14, y+4, BLACK); plot_pixel(x+15, y+4, BLACK); plot_pixel(x+16, y+4, BLACK); plot_pixel(x+17, y+4, BLACK);
	// ROW 6
	plot_pixel(x+0, y+5, BLACK); plot_pixel(x+1, y+5, BLACK); plot_pixel(x+2, y+5, BLACK); plot_pixel(x+7, y+5, RED); plot_pixel(x+10, y+5, TAN); plot_pixel(x+11, y+5, TAN); plot_pixel(x+13, y+5, WHITE); plot_pixel(x+14, y+5, BLACK); plot_pixel(x+15, y+5, BLACK); plot_pixel(x+16, y+5, BLACK); plot_pixel(x+17, y+5, BLACK);
	// ROW 7
	plot_pixel(x+0, y+6, BLACK); plot_pixel(x+1, y+6, BLACK); plot_pixel(x+8, y+6, RED); plot_pixel(x+11, y+6, TAN); plot_pixel(x+12, y+6, TAN); plot_pixel(x+13, y+6, TAN); plot_pixel(x+14, y+6, TAN); plot_pixel(x+16, y+6, TAN); plot_pixel(x+17, y+6, BLACK);
	// ROW 8
	plot_pixel(x+0, y+7, BLACK); plot_pixel(x+1, y+7, BLACK); plot_pixel(x+9, y+7, RED); plot_pixel(x+10, y+7, TAN); plot_pixel(x+11, y+7, TAN); plot_pixel(x+12, y+7, TAN); plot_pixel(x+13, y+7, TAN); plot_pixel(x+14, y+7, TAN); plot_pixel(x+15, y+7, TAN); plot_pixel(x+16, y+7, TAN); plot_pixel(x+17, y+7, TAN);
	// ROW 9
	plot_pixel(x+0, y+8, BLACK); plot_pixel(x+9, y+8, RED); plot_pixel(x+10, y+8, TAN); plot_pixel(x+11, y+8, TAN); plot_pixel(x+12, y+8, BLACK); plot_pixel(x+13, y+8, TAN); plot_pixel(x+14, y+8, TAN); plot_pixel(x+15, y+8, TAN); plot_pixel(x+16, y+8, TAN); plot_pixel(x+17, y+8, TAN);
	// ROW 10
	plot_pixel(x+0, y+9, BLACK); plot_pixel(x+10, y+9, RED); plot_pixel(x+11, y+9, TAN); plot_pixel(x+12, y+9, TAN); plot_pixel(x+13, y+9, BLACK); plot_pixel(x+14, y+9, BLACK); plot_pixel(x+15, y+9, BLACK); plot_pixel(x+16, y+9, BLACK); plot_pixel(x+17, y+9, BLACK); 
	// ROW 11
	plot_pixel(x+0, y+10, BLACK); plot_pixel(x+5, y+10, DBRWN); plot_pixel(x+11, y+10, TAN); plot_pixel(x+12, y+10, TAN); plot_pixel(x+13, y+10, TAN); plot_pixel(x+14, y+10, TAN); plot_pixel(x+15, y+10, TAN); plot_pixel(x+16, y+10, TAN); plot_pixel(x+17, y+10, BLACK);
	// ROW 12
	plot_pixel(x+5, y+11, DBRWN); plot_pixel(x+10, y+11, TAN); plot_pixel(x+11, y+11, RED); plot_pixel(x+12, y+11, TAN); plot_pixel(x+13, y+11, TAN); plot_pixel(x+14, y+11, TAN); plot_pixel(x+15, y+11, TAN); plot_pixel(x+16, y+11, BLACK); plot_pixel(x+17, y+11, BLACK);
	// ROW 13
	plot_pixel(x+4, y+12, DBRWN); plot_pixel(x+5, y+12, DBRWN); plot_pixel(x+10, y+12, TAN); plot_pixel(x+11, y+12, RED); plot_pixel(x+12, y+12, RED); plot_pixel(x+15, y+12, BLACK); plot_pixel(x+16, y+12, BLACK); plot_pixel(x+17, y+12, BLACK); 
	// ROW 14
	plot_pixel(x+5, y+13, TAN); plot_pixel(x+9, y+13, TAN); plot_pixel(x+10, y+13, TAN); plot_pixel(x+11, y+13, RED); plot_pixel(x+12, y+13, RED); plot_pixel(x+15, y+13, BLACK); plot_pixel(x+16, y+13, BLACK); plot_pixel(x+17, y+13, BLACK);
	// ROW 15
	plot_pixel(x+0, y+14, BLACK); plot_pixel(x+5, y+14, BLACK); plot_pixel(x+10, y+14, BLACK); plot_pixel(x+11, y+14, RED); plot_pixel(x+12, y+14, RED); plot_pixel(x+16, y+14, BLACK); plot_pixel(x+17, y+14, BLACK);
	// ROW 16
	plot_pixel(x+4, y+15, BLACK); plot_pixel(x+5, y+15, BLACK); plot_pixel(x+6, y+15, BLACK); plot_pixel(x+7, y+15, TAN); plot_pixel(x+10, y+15, BLACK); plot_pixel(x+11, y+15, BLACK); plot_pixel(x+12, y+15, RED); plot_pixel(x+13, y+15, RED); plot_pixel(x+16, y+15, BLACK); plot_pixel(x+17, y+15, BLACK); 
	// ROW 17
	plot_pixel(x+0, y+16, TAN); plot_pixel(x+1, y+16, TAN); plot_pixel(x+2, y+16, TAN); plot_pixel(x+3, y+16, TAN); plot_pixel(x+4, y+16, TAN); plot_pixel(x+5, y+16, BLACK); plot_pixel(x+6, y+16, TAN); plot_pixel(x+7, y+16, TAN); plot_pixel(x+8, y+16, TAN); plot_pixel(x+9, y+16, TAN); plot_pixel(x+10, y+16, BLACK); plot_pixel(x+11, y+16, BLACK); plot_pixel(x+12, y+16, RED); plot_pixel(x+13, y+16, RED); plot_pixel(x+14, y+16, TAN); plot_pixel(x+15, y+16, TAN); plot_pixel(x+16, y+16, BLACK); plot_pixel(x+17, y+16, BLACK);
	
	return;
}

void draw_peach(int x, int y)
{
	// ROW 1
	plot_pixel(x+4, y, GOLD); plot_pixel(x+6, y, GOLD); plot_pixel(x+8, y, GOLD);
	// ROW 2
	plot_pixel(x+4, y+1, CBLUE); plot_pixel(x+5, y+1, GOLD); plot_pixel(x+6, y+1, DPINK); plot_pixel(x+7, y+1, GOLD); plot_pixel(x+8, y+1, CBLUE);
	// ROW 3
	plot_pixel(x+3, y+2, HAIR); plot_pixel(x+4, y+2, HAIR); plot_pixel(x+5, y+2, HAIR); plot_pixel(x+6, y+2, HAIR); plot_pixel(x+7, y+2, HAIR); plot_pixel(x+8, y+2, HAIR);
	// ROW 4
    plot_pixel(x+2, y+3, HAIR); plot_pixel(x+3, y+3, HAIR); plot_pixel(x+4, y+3, HAIR); plot_pixel(x+5, y+3, HAIR); plot_pixel(x+6, y+3, HAIR); plot_pixel(x+7, y+3, HAIR); plot_pixel(x+8, y+3, HAIR); plot_pixel(x+9, y+3, HAIR);
	// ROW 5
	plot_pixel(x+2, y+4, HAIR); plot_pixel(x+3, y+4, HAIR); plot_pixel(x+4, y+4, HAIR); plot_pixel(x+5, y+4, HAIR); plot_pixel(x+6, y+4, HAIR); plot_pixel(x+7, y+4, HAIR); plot_pixel(x+8, y+4, TAN); plot_pixel(x+9, y+4, HAIR); plot_pixel(x+10, y+4, HAIR);
	// ROW 6
	plot_pixel(x, y+5, HAIR); plot_pixel(x+1, y+5, HAIR); plot_pixel(x+2, y+5, HAIR); plot_pixel(x+3, y+5, HAIR); plot_pixel(x+4, y+5, HAIR); plot_pixel(x+5, y+5, HAIR); plot_pixel(x+6, y+5, TAN); plot_pixel(x+7, y+5, CBLUE); plot_pixel(x+8, y+5, TAN); plot_pixel(x+9, y+5, HAIR); plot_pixel(x+10, y+5, HAIR);
	// ROW 7
	plot_pixel(x+1, y+6, HAIR); plot_pixel(x+2, y+6, HAIR); plot_pixel(x+3, y+6, HAIR); plot_pixel(x+4, y+6, TAN); plot_pixel(x+5, y+6, HAIR); plot_pixel(x+6, y+6, TAN); plot_pixel(x+7, y+6, EBLUE); plot_pixel(x+8, y+6, TAN); plot_pixel(x+9, y+6, TAN);
	// ROW 8
	plot_pixel(x+2, y+7, HAIR); plot_pixel(x+3, y+7, HAIR); plot_pixel(x+4, y+7, CBLUE); plot_pixel(x+5, y+7, HAIR); plot_pixel(x+6, y+7, TAN); plot_pixel(x+7, y+7, TAN); plot_pixel(x+8, y+7, TAN); plot_pixel(x+9, y+7, TAN);
	// ROW 9
	plot_pixel(x, y+8, HAIR); plot_pixel(x+1, y+8, HAIR); plot_pixel(x+2, y+8, HAIR); plot_pixel(x+3, y+8, HAIR); plot_pixel(x+4, y+8, DPINK); plot_pixel(x+5, y+8, DPINK); plot_pixel(x+6, y+8, DPINK); plot_pixel(x+7, y+8, HAIR); plot_pixel(x+8, y+8, CBLUE); plot_pixel(x+9, y+8, DPINK);
	// ROW 10
	plot_pixel(x+1, y+9, HAIR); plot_pixel(x+2, y+9, HAIR); plot_pixel(x+3, y+9, LPINK); plot_pixel(x+4, y+9, LPINK); plot_pixel(x+5, y+9, LPINK); plot_pixel(x+6, y+9, LPINK); plot_pixel(x+7, y+9, LPINK); plot_pixel(x+8, y+9, LPINK); plot_pixel(x+9, y+9, LPINK); plot_pixel(x+10, y+9, LPINK);
	// ROW 11
	plot_pixel(x+2, y+10, HAIR); plot_pixel(x+3, y+10, TAN); plot_pixel(x+4, y+10, TAN); plot_pixel(x+5, y+10, LPINK); plot_pixel(x+6, y+10, LPINK); plot_pixel(x+7, y+10, LPINK); plot_pixel(x+8, y+10, LPINK); plot_pixel(x+9, y+10, LPINK); plot_pixel(x+10, y+10, TAN);
	// ROW 12
	plot_pixel(x+2, y+11, WHITE); plot_pixel(x+3, y+11, WHITE); plot_pixel(x+4, y+11, DPINK); plot_pixel(x+5, y+11, DPINK); plot_pixel(x+6, y+11, DPINK); plot_pixel(x+7, y+11, DPINK); plot_pixel(x+8, y+11, DPINK); plot_pixel(x+9, y+11, DPINK); plot_pixel(x+10, y+11, WHITE); plot_pixel(x+11, y+11, WHITE);
	// ROW 13
	plot_pixel(x+1, y+12, WHITE); plot_pixel(x+2, y+12, WHITE); plot_pixel(x+3, y+12, DPINK); plot_pixel(x+4, y+12, DPINK); plot_pixel(x+5, y+12, DPINK); plot_pixel(x+6, y+12, DPINK); plot_pixel(x+7, y+12, LPINK); plot_pixel(x+8, y+12, LPINK);  plot_pixel(x+9, y+12, DPINK); plot_pixel(x+10, y+12, DPINK); plot_pixel(x+11, y+12, WHITE); plot_pixel(x+12, y+12, WHITE);
	// ROW 14
	plot_pixel(x+3, y+13, DPINK); plot_pixel(x+4, y+13, DPINK); plot_pixel(x+5, y+13, LPINK); plot_pixel(x+6, y+13, LPINK); plot_pixel(x+7, y+13, LPINK); plot_pixel(x+8, y+13, LPINK); plot_pixel(x+9, y+13, LPINK); plot_pixel(x+10, y+13, DPINK);
	// ROW 15
	plot_pixel(x+3, y+14, LPINK); plot_pixel(x+4, y+14, LPINK); plot_pixel(x+5, y+14, LPINK); plot_pixel(x+6, y+14, LPINK); plot_pixel(x+7, y+14, LPINK); plot_pixel(x+8, y+14, LPINK); plot_pixel(x+9, y+14, LPINK); plot_pixel(x+10, y+14, LPINK);
	// ROW 16
	plot_pixel(x+2, y+15, LPINK); plot_pixel(x+3, y+15, LPINK); plot_pixel(x+4, y+15, LPINK); plot_pixel(x+5, y+15, LPINK); plot_pixel(x+6, y+15, LPINK); plot_pixel(x+7, y+15, LPINK); plot_pixel(x+8, y+15, LPINK); plot_pixel(x+9, y+15, LPINK); plot_pixel(x+10, y+15, LPINK); plot_pixel(x+11, y+15, LPINK);
	// ROW 17
	plot_pixel(x+1, y+16, DPINK); plot_pixel(x+2, y+16, DPINK); plot_pixel(x+3, y+16, DPINK); plot_pixel(x+4, y+16, DPINK); plot_pixel(x+5, y+16, DPINK); plot_pixel(x+6, y+16, DPINK); plot_pixel(x+7, y+16, DPINK); plot_pixel(x+8, y+16, DPINK); plot_pixel(x+9, y+16, DPINK); plot_pixel(x+10, y+16, DPINK); plot_pixel(x+11, y+16, DPINK);
	
	return;	
}

void draw_fireball(int x, int y)
{
	// ROW 1
	plot_pixel(x+1, y, ORANGE); plot_pixel(x+3, y, ORANGE); plot_pixel(x+4, y, ORANGE);
	// ROW 2
	plot_pixel(x+2, y+1, ORANGE); plot_pixel(x+4, y+1, ORANGE); plot_pixel(x+5, y+1, ORANGE); plot_pixel(x+6, y+1, ORANGE);
	// ROW 3
	plot_pixel(x, y+2, ORANGE); plot_pixel(x+4, y+2, ORANGE); plot_pixel(x+5, y+2, LORANGE); plot_pixel(x+6, y+2, ORANGE); plot_pixel(x+7, y+2, ORANGE);
	// ROW 4
	plot_pixel(x+2, y+3, ORANGE); plot_pixel(x+3, y+3, ORANGE); plot_pixel(x+4, y+3, ORANGE); plot_pixel(x+5, y+3, LORANGE); plot_pixel(x+6, y+3, LORANGE); plot_pixel(x+7, y+3, ORANGE);
	// ROW 5
	plot_pixel(x+1, y+4, ORANGE); plot_pixel(x+2, y+4, ORANGE); plot_pixel(x+3, y+4, LORANGE); plot_pixel(x+4, y+4, LORANGE); plot_pixel(x+5, y+4, WHITE); plot_pixel(x+6, y+4, LORANGE); plot_pixel(x+7, y+4, ORANGE);
	// ROW 6
	plot_pixel(x+1, y+5, ORANGE); plot_pixel(x+2, y+5, LORANGE); plot_pixel(x+3, y+5, LORANGE); plot_pixel(x+4, y+5, WHITE); plot_pixel(x+5, y+5, LORANGE); plot_pixel(x+6, y+5, ORANGE); plot_pixel(x+7, y+5, ORANGE);
	// ROW 7
	plot_pixel(x+1, y+6, ORANGE); plot_pixel(x+2, y+6, ORANGE); plot_pixel(x+3, y+6, LORANGE); plot_pixel(x+4, y+6, LORANGE); plot_pixel(x+5, y+6, ORANGE); plot_pixel(x+6, y+6, ORANGE);
	// ROW 8
	plot_pixel(x+2, y+7, ORANGE); plot_pixel(x+3, y+7, ORANGE); plot_pixel(x+4, y+7, ORANGE); plot_pixel(x+5, y+7, ORANGE);
	
	return;
}

void draw_fireball_up(int x, int y)
{
	y = y + 7;
	// ROW 1
	plot_pixel(x+1, y, ORANGE); plot_pixel(x+3, y, ORANGE); plot_pixel(x+4, y, ORANGE);
	// ROW 2
	plot_pixel(x+2, y-1, ORANGE); plot_pixel(x+4, y-1, ORANGE); plot_pixel(x+5, y-1, ORANGE); plot_pixel(x+6, y-1, ORANGE);
	// ROW 3
	plot_pixel(x, y-2, ORANGE); plot_pixel(x+4, y-2, ORANGE); plot_pixel(x+5, y-2, LORANGE); plot_pixel(x+6, y-2, ORANGE); plot_pixel(x+7, y-2, ORANGE);
	// ROW 4
	plot_pixel(x+2, y-3, ORANGE); plot_pixel(x+3, y-3, ORANGE); plot_pixel(x+4, y-3, ORANGE); plot_pixel(x+5, y-3, LORANGE); plot_pixel(x+6, y-3, LORANGE); plot_pixel(x+7, y-3, ORANGE);
	// ROW 5
	plot_pixel(x+1, y-4, ORANGE); plot_pixel(x+2, y-4, ORANGE); plot_pixel(x+3, y-4, LORANGE); plot_pixel(x+4, y-4, LORANGE); plot_pixel(x+5, y-4, WHITE); plot_pixel(x+6, y-4, LORANGE); plot_pixel(x+7, y-4, ORANGE);
	// ROW 6
	plot_pixel(x+1, y-5, ORANGE); plot_pixel(x+2, y-5, LORANGE); plot_pixel(x+3, y-5, LORANGE); plot_pixel(x+4, y-5, WHITE); plot_pixel(x+5, y-5, LORANGE); plot_pixel(x+6, y-5, ORANGE); plot_pixel(x+7, y-5, ORANGE);
	// ROW 7
	plot_pixel(x+1, y-6, ORANGE); plot_pixel(x+2, y-6, ORANGE); plot_pixel(x+3, y-6, LORANGE); plot_pixel(x+4, y-6, LORANGE); plot_pixel(x+5, y-6, ORANGE); plot_pixel(x+6, y-6, ORANGE);
	// ROW 8
	plot_pixel(x+2, y-7, ORANGE); plot_pixel(x+3, y-7, ORANGE); plot_pixel(x+4, y-7, ORANGE); plot_pixel(x+5, y-7, ORANGE);
	
	return;
}

void draw_fireball_back(int x, int y)
{
	x = x + 7;
	// ROW 1
	plot_pixel(x-1, y, ORANGE); plot_pixel(x-3, y, ORANGE); plot_pixel(x-4, y, ORANGE);
	// ROW 2
	plot_pixel(x-2, y+1, ORANGE); plot_pixel(x-4, y+1, ORANGE); plot_pixel(x-5, y+1, ORANGE); plot_pixel(x-6, y+1, ORANGE);
	// ROW 3
	plot_pixel(x, y+2, ORANGE); plot_pixel(x-4, y+2, ORANGE); plot_pixel(x-5, y+2, LORANGE); plot_pixel(x-6, y+2, ORANGE); plot_pixel(x-7, y+2, ORANGE);
	// ROW 4
	plot_pixel(x-2, y+3, ORANGE); plot_pixel(x-3, y+3, ORANGE); plot_pixel(x-4, y+3, ORANGE); plot_pixel(x-5, y+3, LORANGE); plot_pixel(x-6, y+3, LORANGE); plot_pixel(x-7, y+3, ORANGE);
	// ROW 5
	plot_pixel(x-1, y+4, ORANGE); plot_pixel(x-2, y+4, ORANGE); plot_pixel(x-3, y+4, LORANGE); plot_pixel(x-4, y+4, LORANGE); plot_pixel(x-5, y+4, WHITE); plot_pixel(x-6, y+4, LORANGE); plot_pixel(x-7, y+4, ORANGE);
	// ROW 6
	plot_pixel(x-1, y+5, ORANGE); plot_pixel(x-2, y+5, LORANGE); plot_pixel(x-3, y+5, LORANGE); plot_pixel(x-4, y+5, WHITE); plot_pixel(x-5, y+5, LORANGE); plot_pixel(x-6, y+5, ORANGE); plot_pixel(x-7, y+5, ORANGE);
	// ROW 7
	plot_pixel(x-1, y+6, ORANGE); plot_pixel(x-2, y+6, ORANGE); plot_pixel(x-3, y+6, LORANGE); plot_pixel(x-4, y+6, LORANGE); plot_pixel(x-5, y+6, ORANGE); plot_pixel(x-6, y+6, ORANGE);
	// ROW 8
	plot_pixel(x-2, y+7, ORANGE); plot_pixel(x-3, y+7, ORANGE); plot_pixel(x-4, y+7, ORANGE); plot_pixel(x-5, y+7, ORANGE);
	
	return;
}



void draw_platform(int x, int y)
{
	// top beam
	draw_line(x, y, x+15, y, LPLT);
	draw_line(x, y+1, x+15, y+1, DPLT);
	
	//triangle 1
	plot_pixel(x, y+5, LPLT); plot_pixel(x, y+4, LPLT); plot_pixel(x+1, y+4, LPLT); plot_pixel(x+1, y+3, LPLT); plot_pixel(x+2, y+3, LPLT); plot_pixel(x+2, y+2, LPLT); plot_pixel(x+3, y+2, LPLT);
	plot_pixel(x+4, y+2, LPLT); plot_pixel(x+4, y+3, LPLT); plot_pixel(x+5, y+3, LPLT); plot_pixel(x+5, y+4, LPLT); plot_pixel(x+6, y+4, LPLT); plot_pixel(x+6, y+5, LPLT); plot_pixel(x+7, y+5, LPLT);
	
	// triangle 2
	plot_pixel(x+9, y+5, LPLT); plot_pixel(x+9, y+4, LPLT); plot_pixel(x+1+9, y+4, LPLT); plot_pixel(x+1+9, y+3, LPLT); plot_pixel(x+2+9, y+3, LPLT); plot_pixel(x+2+9, y+2, LPLT); plot_pixel(x+3+9, y+2, LPLT);
	plot_pixel(x+4+9, y+2, LPLT); plot_pixel(x+4+9, y+3, LPLT); plot_pixel(x+5+9, y+3, LPLT); plot_pixel(x+5+9, y+4, LPLT); plot_pixel(x+6+9, y+4, LPLT); plot_pixel(x+6+9, y+5, LPLT); plot_pixel(x+8, y+5, LPLT);
	// bottom beam
	draw_line(x, y+6, x+15, y+6, LPLT);
	draw_line(x, y+7, x+15, y+7, DPLT);
	
	return;	
}

void clear_mario(int x, int y)
{
	for(int i= 0; i < 16; i++){
		for(int j = 0; j < 16; j++){
			plot_pixel(i+x, j+y, BLACK);
		}
	}
	return;
}

void clear_fireball(int x, int y)
{
	for(int i= 0; i < 9; i++){
		for(int j = 0; j < 9; j++){
			plot_pixel(i+x, j+y, BLACK);
		}
	}
	return;
}


void wait_for_vsync(){

	volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
	// start the synchronization process (this requests to synchronize with the DMA controller)
	// and does not affect the register itself
	*pixel_ctrl_ptr = 1;
	register int status;
	
	// use polled io to wait until the syncronization process is done (S bit = 0)
	// this happens when the final pixel has been sent to the VGA to be drawn
	// read the status register at its address below
	status = *(pixel_ctrl_ptr + 3);
	//wait until the first bit in the status register is 0 then exit
	// when s becomes 0m the dma swaps the contents of these two registers
	while ((status & 0x01) != 0){
		status = *(pixel_ctrl_ptr + 3);
	}
	return;
}
void check_collisions() {
	// LEVEL 1
	if(mario_front % 16 == 0 && mario_level == 1 && mario_x > 111){
		if(mario_dx == 1)
			mario_dy = -1;
		else if(mario_dx == -1)
			mario_dy = 1;
		else
			mario_dy = 0;
	}
	else if(mario_x % 16 == 0 && mario_level == 2){
		if(mario_dx == 1)
			mario_dy = 1;
		else if(mario_dx == -1)
			mario_dy = -1;
		else
			mario_dy = 0;
	}
	else if(mario_front % 16 == 0 && mario_level == 3){
		if(mario_dx == 1)
			mario_dy = -1;
		else if(mario_dx == -1)
			mario_dy = 1;
		else
			mario_dy = 0;
	}
	else if(mario_x % 16 == 0 && mario_level == 4){
		if(mario_dx == 1)
			mario_dy = 1;
		else if(mario_dx == -1)
			mario_dy = -1;
		else
			mario_dy = 0;
	}
	else if(mario_front % 16 == 0 && mario_level == 5){
		if(mario_dx == 1)
			mario_dy = -1;
		else if(mario_dx == -1)
			mario_dy = 1;
		else
			mario_dy = 0;
	}
	else if(mario_x % 16 == 0 && mario_level == 6){
		if(mario_dx == 1)
			mario_dy = 1;
		else if(mario_dx == -1)
			mario_dy = -1;
		else
			mario_dy = 0;
	}
	else if(mario_front % 16 == 0 && mario_level == 7){
		if(mario_dx == 1)
			mario_dy = 0;
		else if(mario_dx == -1)
			mario_dy = 0;
		else
			mario_dy = 0;
	}
}

void victory_animation(int x, int y)
{
	// Draw heart
	for(int i = 0; i < 11; i++){
		for(int j = 0; j < 10; j++){
			plot_pixel(i+x, j+y, LPINK);
		}
	}
	// ROW 1 
	plot_pixel(x, y, BLACK); plot_pixel(x+1, y, BLACK); plot_pixel(x+2, y, DPINK); plot_pixel(x+3, y, DPINK); plot_pixel(x+4, y, BLACK); plot_pixel(x+5, y, BLACK); plot_pixel(x+6, y, BLACK); plot_pixel(x+7, y, DPINK); plot_pixel(x+8, y, DPINK); plot_pixel(x+9, y, BLACK); plot_pixel(x+10, y, BLACK);
	// ROW 2
	plot_pixel(x, y+1, BLACK); plot_pixel(x+1, y+1, DPINK); plot_pixel(x+4, y+1, DPINK); plot_pixel(x+5, y+1, BLACK); plot_pixel(x+6, y+1, DPINK); plot_pixel(x+9, y+1, DPINK); plot_pixel(x+10, y+1, BLACK);
	// ROW 3
	plot_pixel(x, y+2, DPINK); plot_pixel(x+2, y+2, HPINK); plot_pixel(x+3, y+2, HPINK); plot_pixel(x+5, y+2, DPINK); plot_pixel(x+10, y+2, DPINK);
	// ROW 4
	plot_pixel(x, y+3, DPINK); plot_pixel(x+2, y+3, HPINK); plot_pixel(x+3, y+3, HPINK); plot_pixel(x+10, y+3, DPINK);
	// ROW 5
	plot_pixel(x, y+4, DPINK); plot_pixel(x+10, y+4, DPINK);
	// ROW 6
	plot_pixel(x, y+5, BLACK); plot_pixel(x+1, y+5, DPINK); plot_pixel(x+9, y+5, DPINK); plot_pixel(x+10, y+5, BLACK);
	// ROW 7
	plot_pixel(x, y+6, BLACK); plot_pixel(x+1, y+6, BLACK); plot_pixel(x+2, y+6, DPINK); plot_pixel(x+8, y+6, DPINK); plot_pixel(x+9, y+6, BLACK); plot_pixel(x+10, y+6, BLACK);
	// ROW 8
	plot_pixel(x, y+7, BLACK); plot_pixel(x+1, y+7, BLACK); plot_pixel(x+2, y+7, BLACK); plot_pixel(x+3, y+7, DPINK); plot_pixel(x+7, y+7, DPINK); plot_pixel(x+8, y+7, BLACK); plot_pixel(x+9, y+7, BLACK); plot_pixel(x+10, y+7, BLACK);
	// ROW 9
	plot_pixel(x, y+8, BLACK); plot_pixel(x+1, y+8, BLACK); plot_pixel(x+2, y+8, BLACK); plot_pixel(x+3, y+8, BLACK); plot_pixel(x+4, y+8, DPINK); plot_pixel(x+6, y+8, DPINK); plot_pixel(x+7, y+8, BLACK); plot_pixel(x+8, y+8, BLACK); plot_pixel(x+9, y+8, BLACK); plot_pixel(x+10, y+8, BLACK);
	// ROW 10
	plot_pixel(x, y+9, BLACK); plot_pixel(x+1, y+9, BLACK); plot_pixel(x+2, y+9, BLACK); plot_pixel(x+3, y+9, BLACK); plot_pixel(x+4, y+9, BLACK); plot_pixel(x+5, y+9, DPINK); plot_pixel(x+6, y+9, BLACK); plot_pixel(x+7, y+9, BLACK); plot_pixel(x+8, y+9, BLACK); plot_pixel(x+9, y+9, BLACK); plot_pixel(x+10, y+9, BLACK);
	
	return;
	
}

void end_game()
{
	while(1){
		volatile int * PS2_ptr = (int *)PS2_BASE;
		int PS2_data = *(PS2_ptr); // read the data register in the PS/2 port
		int RVALID = PS2_data & 0x8000; // extract RVALID Field
		int restart;
		if(RVALID) {
			restart = PS2_data & 0xFF;
			if(restart ==  0x5A) {
				mario_x = 30;
				mario_y = 216;
				break;
			}
		}
	}
	return;
}

void lose_game()
{
	// clear the screen and repeat drawing the "YOU LOSE" message
	int x = 110;
	int y = 110;
	clear_screen();
	while(1) {
		// Letter Y
		draw_line(x, y, x+4, y+4, WHITE); draw_line(x, y+1, x+4, y+5, WHITE); draw_line(x+9, y, x+5, y+4, WHITE); draw_line(x+9, y+1, x+5, y+5, WHITE); draw_line(x+4, y+5, x+4, y+10, WHITE); draw_line(x+5, y+5, x+5, y+10, WHITE);
		// Letter O
		draw_line(x+12, y, x+19, y, WHITE); draw_line(x+12, y+1, x+19, y+1, WHITE); draw_line(x+12, y+9, x+19, y+9, WHITE); draw_line(x+12, y+10, x+19, y+10, WHITE); draw_line(x+12, y, x+12, y+10, WHITE); draw_line(x+13, y, x+13, y+10, WHITE); draw_line(x+20, y, x+20, y+10, WHITE); draw_line(x+21, y, x+21, y+10, WHITE);
		// Letter U
		draw_line(x+12+12, y+9, x+19+12, y+9, WHITE); draw_line(x+12+12, y+10, x+19+12, y+10, WHITE); draw_line(x+12+12, y, x+12+12, y+10, WHITE); draw_line(x+13+12, y, x+13+12, y+10, WHITE); draw_line(x+20+12, y, x+20+12, y+10, WHITE); draw_line(x+21+12, y, x+21+12, y+10, WHITE);
		
		// Letter L
		draw_line(x+12+34, y+9, x+19+34, y+9, WHITE); draw_line(x+12+34, y+10, x+19+34, y+10, WHITE); draw_line(x+12+34, y, x+12+34, y+10, WHITE); draw_line(x+13+34, y, x+13+34, y+10, WHITE);
		// Letter O
		draw_line(x+12+44, y, x+19+44, y, WHITE); draw_line(x+12+44, y+1, x+19+44, y+1, WHITE); draw_line(x+12+44, y+9, x+19+44, y+9, WHITE); draw_line(x+12+44, y+10, x+19+44, y+10, WHITE); draw_line(x+12+44, y, x+12+44, y+10, WHITE); draw_line(x+13+44, y, x+13+44, y+10, WHITE); draw_line(x+20+44, y, x+20+44, y+10, WHITE); draw_line(x+21+44, y, x+21+44, y+10, WHITE);
		// Letter S
		draw_line(x+12+56, y, x+19+56, y, WHITE); draw_line(x+12+56, y+1, x+19+56, y+1, WHITE); draw_line(x+12+56, y+9, x+19+56, y+9, WHITE); draw_line(x+12+56, y+10, x+19+56, y+10, WHITE); draw_line(x+12+56, y+5, x+19+56, y+5, WHITE); draw_line(x+12+56, y, x+12+56, y+5, WHITE); draw_line(x+19+56, y+5, x+19+56, y+10, WHITE); draw_line(x+13+56, y, x+13+56, y+5, WHITE); draw_line(x+18+56, y+5, x+18+56, y+10, WHITE);
		// Letter E
		draw_line(x+12+66, y, x+19+66, y, WHITE); draw_line(x+12+66, y+1, x+19+66, y+1, WHITE); draw_line(x+12+66, y+9, x+19+66, y+9, WHITE); draw_line(x+12+66, y+10, x+19+66, y+10, WHITE); draw_line(x+12+66, y, x+12+66, y+10, WHITE); draw_line(x+13+66, y, x+13+66, y+10, WHITE); draw_line(x+12+66, y+5, x+19+66, y+5, WHITE);
		// ! Character
		draw_line(x+12+76, y, x+12+76, y+7, WHITE); draw_line(x+12+77, y, x+12+77, y+7, WHITE); draw_line(x+12+76, y+9, x+12+76, y+10, WHITE); draw_line(x+12+77, y+9, x+12+77, y+10, WHITE);
		
		volatile int * PS2_ptr = (int *)PS2_BASE;
		int PS2_data = *(PS2_ptr); // read the data register in the PS/2 port
		int RVALID = PS2_data & 0x8000; // extract RVALID Field
		int restart;
		if(RVALID) {
			restart = PS2_data & 0xFF;
			if(restart ==  0x5A) {
				mario_x = 30;
				mario_y = 216;
				break;
			}
		}
	}
	return;	
}
