/*******************************************************************  
Program for calculating and displaying JuliaSet
Functionalities:
- position on the screen can be changed by the red and green rotary dials
- real and imaginary constants for calculation can be changed by the blue rotary dial
- to zoom in push the red button, to zoom out the green button (functionality isnt supported in gallery mode)
- to activate the gallery mode, which changes periodically (every 2 seconds) the real and imaginary part of constant used for calculation, press the blue button

 final_sem_code.c		- main and only file

 (C) Copyright 2018 by Klara Pacalova
	  e-mail: pacalkla@fel.cvut.cz
	  license: FEL CTU
 *******************************************************************/

#define _POSIX_C_SOURCE 200112L
#define WIDTH 480
#define HEIGHT 320

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <stdbool.h>

#include "mzapo_parlcd.h"
#include "mzapo_phys.h"
#include "mzapo_regs.h"

//mutex
pthread_mutex_t mtx;

typedef struct
{
  double real;
  double img;
}complex_t;

//vraci druhou mocninu komplexniho cisla
complex_t power2(complex_t z)
{
  complex_t new_complex;
  new_complex.real = (z.real * z.real) - (z.img * z.img);
  new_complex.img = 2 * z.real * z.img;
  return new_complex;
}
 
//vraci abs hodnotu komplexniho cisla
float complex_abs(complex_t z)
{
  return sqrt(z.real*z.real + z.img*z.img);
}
//globalni promenne
bool new_computation = false;
bool new_calculation = false;
bool gallery_mode = false;

//konstanty pro vypocet
double const_real[] = {-0.4, -0.79, -0.162, 0.3, -1.476, -0.12, 0.28, 0.285, 0.285, 0.45, -0.70176, -0.835, -0.8, -0.7269, 0};
double const_imag[] = {0.6, 0.15, 1.04, -0.1, 0, -0.77, 0.008, 0, 0.01, 0.1428, -0.3842, -0.2321, 0.156, 0.1889, -0.8};
int const_idx = 0;
complex_t start_default;

//vraci k kdyz failed test, n+1 jestli patri do julset 
//zjistuje jestli cislo patri do jul. mnoziny
//k=failed
//n+1=ok
int belong_to_julset(complex_t z, complex_t c, int n)
{
  complex_t zi = z;
  for (int i = 1; i <= n; ++i)
  {
    zi = power2(zi);
    zi.real += c.real; 
    zi.img += c.img; 
    if (complex_abs(zi) > (float)2.0)
      return i;
  }
  return n+1;
}

typedef struct{
  uint8_t R;
  uint8_t G;
  uint8_t B;
}rgb_t;

//sdilene promenne - pole pro vykresleni na displej, pocatek souradnic
rgb_t display_array[HEIGHT][WIDTH];

//vzdalenost mezi pixely
float diff_x_default = 0.0066;
float diff_y_default = 0.0066;

//detekovani zpozdeni vypocetni fce - pomocna struktura
typedef struct{
 int height;
 int width;
}pixel;

pixel pix;

//funkce na bitovy posun barev pro format rgb565
uint16_t rgb565 (uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t R = r >> 3;
    uint8_t G = g >> 2;
    uint8_t B = b >> 3;
    uint16_t rgb = (R<<11)|(G<<5)|B;
    return rgb;
}

void *JuliaSetComputing(void *vargp)
{
	//init
	int num_of_iter;
	double diff_x;
	double diff_y;
	rgb_t current_pixel;
	complex_t constant;
	complex_t start;
	bool reset = false;
	complex_t start_new;
	pthread_mutex_lock(&mtx);
	
	//konstanty pro nastaveni pocatku souradnic
	start.real = start_default.real;
	start.img = start_default.img;
	
	//konstanty pro nastaveni vzdalenosti mezi pixely
	diff_x = diff_x_default;
	diff_y = diff_y_default;
	
	//re+im slozka pocatecni konstanty
	constant.real = const_real[const_idx];
	constant.img = const_imag[const_idx];		
	pthread_mutex_unlock(&mtx);
	reset = true;
	
	while(1){
		//zmena startu, aby pri pri/od-zoomovani zustal na stredu
		start_new.img = start.img - 160 * diff_y;
		start_new.real = start.real - 240 * diff_x;
		float k;
		
		//reset
		if (reset == true){
			reset = false;
			pthread_mutex_lock(&mtx);
			new_calculation = 1;
			pix.width = 0;
			pix.height = 0;
			pthread_mutex_unlock(&mtx);
			
			printf("new computation Start new real:%f,Start new img:%f\n",start.real,start.img);
			for (int i = 0; i < HEIGHT; i++){
				for (int j = 0; j < WIDTH; j++){					
					num_of_iter = belong_to_julset(start_new,constant,60);
					//nastaveni pixelu nad horni hranici iterace na cernou
					if (num_of_iter >= 60){
						current_pixel.R = 0;
						current_pixel.G = 0;
						current_pixel.B = 0;
					}
					
					//vypocet
					else{
						k = num_of_iter/(float)60;
						current_pixel.R = 9*(1-k)*k*k*k*255;
						current_pixel.G = 15*(1-k)*(1-k)*k*k*255;
						current_pixel.B = 8.5*(1-k)*(1-k)*(1-k)*(k)*255;
					}
					pthread_mutex_lock(&mtx);
					display_array[i][j] = current_pixel;
					pix.width = j;
					pix.height = i;
					reset = new_computation;
					pthread_mutex_unlock(&mtx);
					start_new.real = start_new.real + diff_x;
				}
				if(reset == true)
				{
					break;
				}
				
				//skok na novy radek
				start_new.real = start.real - 240 * diff_x;
				start_new.img = start_new.img + diff_y;
			}
			printf("computation done\n");
		}
		pthread_mutex_lock(&mtx);
		//konstanty pro nastaveni pocatku souradnic
		start.real = start_default.real;
		start.img = start_default.img;
		//konstanty pro nastaveni vzdalenosti mezi pixely
		diff_x = diff_x_default;
		diff_y = diff_y_default;
		//re+im slozka pocatecni konstanty
		constant.real = const_real[const_idx];
		constant.img = const_imag[const_idx];
		reset = new_computation;
		new_computation = false;
		pthread_mutex_unlock(&mtx);
	}
	
	return NULL;
}

void *InputFromBoard(void *vargp)
{
	//R rotate na posun po ose x
	//G rotate na posun po ose y
	//B rotate na meneni konstanty
	//R button na zoom in
	//G button na zoom out
	
	unsigned char *knobs_mem_base = map_phys_address(SPILED_REG_BASE_PHYS, SPILED_REG_SIZE, 0);
	//obnoveni hodnoty na tlacitkach
	uint32_t knobs = *(volatile uint32_t*)(knobs_mem_base + SPILED_REG_KNOBS_8BIT_o);
	
	//promenne na otaceni tlacitek
	uint8_t R_rotate = (knobs >> 16) & 0x000000ff; 
	uint8_t G_rotate = (knobs >> 8) & 0x000000ff;  
	uint8_t B_rotate = knobs & 0x000000ff; 
	  
	//promenne na stisknuti tlacitek          
	bool R_button = (knobs >> 26) & 0x00000001;    
	bool G_button = (knobs >> 25) & 0x00000001;   
	bool B_button = (knobs >> 24) & 0x00000001;    

	//nove pozice
	uint8_t R_rotate_new = (knobs >> 16) & 0x000000ff; 
	uint8_t G_rotate_new = (knobs >> 8) & 0x000000ff;  
	uint8_t B_rotate_new = knobs & 0x000000ff;         
	bool R_button_new = (knobs >> 26) & 0x00000001;    
	bool G_button_new = (knobs >> 25) & 0x00000001;   
	bool B_button_new = (knobs >> 24) & 0x00000001;   
	
	//promenne na pocitani rozdilu oproti predchozi pozici
	int R_rotate_diff = 0;
	int G_rotate_diff = 0;
	int B_rotate_diff = 0;
	int max = 255;
	bool gallery = false;
	
	while(1){
		knobs = *(volatile uint32_t*)(knobs_mem_base + SPILED_REG_KNOBS_8BIT_o);
		B_rotate_new = knobs & 0x000000ff;         
		G_rotate_new = (knobs >> 8) & 0x000000ff;  
		R_rotate_new = (knobs >> 16) & 0x000000ff; 
		
		//printf("R rotate new:%d\n", R_rotate_new);
		//printf("G rotate new:%d\n", G_rotate_new);
		//printf("B rotate new:%d\n", B_rotate_new);
		
		B_button_new = (knobs >> 24) & 0x00000001;    
		G_button_new = (knobs >> 25) & 0x00000001;    
		R_button_new = (knobs >> 26) & 0x00000001;
		
		R_rotate_diff = R_rotate_new - R_rotate;
		G_rotate_diff = G_rotate_new - G_rotate;
		B_rotate_diff = B_rotate_new - B_rotate;
		
		
		if (R_rotate != R_rotate_new && gallery == 0){
			//printf("R rotate diff:%d\n", R_rotate_diff);
			//detekce na kterou stranu otacime
			if ((max - abs(R_rotate_diff)) < abs(R_rotate_diff)){
				if (R_rotate_diff < 0)
					R_rotate_diff = (max + R_rotate_diff)*(-1);
				else
					R_rotate_diff = max - R_rotate_diff;
				}
				pthread_mutex_lock(&mtx);
				start_default.real = start_default.real + diff_x_default*R_rotate_diff;
				new_computation = true;
				pthread_mutex_unlock(&mtx);
				//printf("R rotate diff:%d\n", R_rotate_diff);
			}
			//detekce na kterou stranu otacime
			if (G_rotate != G_rotate_new && gallery == 0){
				if ((max - abs(G_rotate_diff)) < abs(G_rotate_diff)){
					if (G_rotate_diff < 0)
						G_rotate_diff = (max + G_rotate_diff)*(-1);
					else
						G_rotate_diff = max - G_rotate_diff;
				}
				pthread_mutex_lock(&mtx);
				start_default.img = start_default.img + diff_y_default*G_rotate_diff;
				new_computation = true;
				pthread_mutex_unlock(&mtx);
				//printf("G rotate diff:%d\n", G_rotate_diff);
			}
			//detekce na kterou stranu otacime
			if (B_rotate != B_rotate_new && gallery == 0){
				if ((max - abs(B_rotate_diff)) < abs(B_rotate_diff)){
					if (B_rotate_diff < 0)
						B_rotate_diff = (max + B_rotate_diff)*(-1);
					else
						B_rotate_diff = max - B_rotate_diff;
					}
				//15 - velikost pole konstant
				pthread_mutex_lock(&mtx);
				const_idx += B_rotate_diff;
				if (const_idx < 0)
					const_idx = 15-abs(const_idx)%15;
				else
					const_idx = const_idx%15;
				new_computation = true;	
				pthread_mutex_unlock(&mtx);
				//printf("B rotate diff:%d\n", B_rotate_diff);
			}
		
		if (R_button != R_button_new && gallery == 0){
			if (R_button_new == 1){
				pthread_mutex_lock(&mtx);
				//zoom in
				diff_x_default = 1.1*diff_x_default;
				diff_y_default = 1.1*diff_y_default;
				new_computation = true;
				pthread_mutex_unlock(&mtx);
				printf("R button diff:%f\n", diff_x_default);
			}
		}
			
		if (G_button != G_button_new && gallery == 0){
			if (G_button_new == 1){
				pthread_mutex_lock(&mtx);
				//zoom out
				diff_x_default = diff_x_default/1.1;
				diff_y_default = diff_y_default/1.1;
				new_computation = true;
				pthread_mutex_unlock(&mtx);
				printf("G button diff:%f\n", diff_y_default);
				}
			}
		if (B_button != B_button_new){
			if (B_button_new == 1){
				pthread_mutex_lock(&mtx);
				if (gallery_mode == 1){
					gallery_mode = 0;
					gallery = 0;
				}
				else{
				gallery_mode = 1;
				gallery = 1;
				}
				new_computation = true;	
				pthread_mutex_unlock(&mtx);
				printf("Gallery mode state: %d\n", gallery_mode);
			}
		}
		//prepsani starych hodnot za nove
		R_rotate = R_rotate_new;
		G_rotate = G_rotate_new;
		B_rotate = B_rotate_new;
		
		R_button = R_button_new;
		G_button = G_button_new;
		B_button = B_button_new;
		sleep(0.001);
	}
	
	return NULL;
}

void *PrintingOutput(void *vargp)
{
	//mapovani fyzicke adresy
	uint8_t *lcd_mem_base = map_phys_address(PARLCD_REG_BASE_PHYS, PARLCD_REG_SIZE, 0);
	//mapovani displeje
	parlcd_hx8357_init(lcd_mem_base);
	parlcd_write_cmd(lcd_mem_base, 0x2c);
	bool reset_test = 0;
	rgb_t current_pixel;
	bool output_done = 0;
	
	while(1){
		if (reset_test == 1){
			//printf("showing new screen\n");
			reset_test = 0;
			parlcd_write_cmd(lcd_mem_base, 0x2c);
			for (int i = 0; i < HEIGHT; i++){
				for (int j = 0; j < WIDTH; j++){
					pthread_mutex_lock(&mtx);
					while ((i > pix.height) || (j > pix.width)){
						pthread_mutex_unlock(&mtx);
						sleep(0.001);
						pthread_mutex_lock(&mtx);
						if (new_calculation == 1){
							break;
						}
					}
					reset_test = new_calculation;
					current_pixel.R = display_array[i][j].R;
					current_pixel.G = display_array[i][j].G;
					current_pixel.B = display_array[i][j].B;
					pthread_mutex_unlock(&mtx);
					if (reset_test == 1){
						break;
					}
					//printing to display
					parlcd_write_data(lcd_mem_base, rgb565(current_pixel.R, current_pixel.G, current_pixel.B));
					}
				if (reset_test == 1){
					break;
				}
			}
			//printf("showing new screen done\n");
			output_done = 1;
		}
		pthread_mutex_lock(&mtx);
		reset_test = new_calculation;
		new_calculation = 0;
		
		if (gallery_mode == 1 && output_done == 1){
			pthread_mutex_unlock(&mtx);
			sleep(2);
			pthread_mutex_lock(&mtx);
			new_computation = 1;
			const_idx += 1;
			const_idx = const_idx%15;
			start_default.real = 0.0022;
			start_default.img = -0.0038;
			diff_x_default = 0.0066;
			diff_y_default = 0.0066;
		}
		pthread_mutex_unlock(&mtx);
		output_done = 0;
	}
	
	return NULL;
}

int main(int argc, char *argv[])
{
	//init start pozice
	start_default.real = 0.0022;
	start_default.img = -0.0038;

	//vlakna - init, def, create
	pthread_mutex_init(&mtx, NULL);
	pthread_t threads[3];
	    
	pthread_create(&threads[0], NULL, JuliaSetComputing, NULL);
	pthread_create(&threads[1], NULL, PrintingOutput, NULL);
	pthread_create(&threads[2], NULL, InputFromBoard, NULL);
	
	//zanik vlaken
	pthread_join(threads[0], NULL);
	pthread_join(threads[1], NULL);
	pthread_join(threads[2], NULL);
	
	return 0;
}
