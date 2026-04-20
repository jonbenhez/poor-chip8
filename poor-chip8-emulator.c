#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdlib.h>
#include <stdint.h>
// I mostly used stdint types over SDL types
// because my IDE doesn't color SDL types I'm lazy
#include <stdio.h>
#include <stdbool.h>

/*
 popular documentations are used
 The Classic Chip-8 for COSMAC VIP

 This is not an example code
 neither a tutorial
 please don't blame me for
 poor explaination
*/

char program_name[] = "aprogramcomeshere.ch8";
//program's file name assuming in the same folder
//
bool logs_on = true;
// if you want to see logs in console
// there are just few of them xd
// be careful, it turns off error logs too


#define Vx as->V[opcode_2] 
#define Vy as->V[opcode_1] 
#define Vf as->V[0xF] 
// we define, otherwise it looks complex unnecessary
// using assignment like Vx = blah is not good
// it doesn't change the original data but the variable



// backbone SDL codes are copy paste from
// the original SDL examples, maybe little different
// SDL Copy paste
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_AudioStream *stream = NULL;
static int current_sine_sample = 0;

bool clear_screen = false;
// if true initializes all frame values = 0

int cycle = 0;
// counts how many insturactions we done

uint8_t opcode_0 ;
uint8_t opcode_1 ;
uint8_t opcode_2 ;
uint8_t opcode_3 ;
/* blocks of the opcode
|     opcode    |
| 3 | 2 | 1 | 0 |
 each of them is a nibble in the corresponding position
 opcode's itself is 2 bytes 
 made up of two adjacent memory locations  
*/

int pixel_size = 25;
// 64*32 is so small, making it bigger

int offset = 100;
// the top left of the chio-8 screen inside SDL window, x = y = offset 

// we will use appstate(SDL) to transfer the main struct
// maybe not neccesary or the best
// but I used it anyways
// somethings are inside the struct
// somethings are outside 
// no special reason

// here declaring the main struct
typedef struct{
    bool *frame;
    bool *keys;
    uint8_t *mem; 
    uint8_t *V;
    uint16_t I;
    uint8_t Delay;
    uint8_t Sound;
    uint16_t PC;
    uint8_t SP;
    uint16_t *Stack;
    int waiting_for_key;

}AppState_struct;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    
    //and here creating an instance of the struct
    AppState_struct *as = (AppState_struct *)SDL_calloc(1, sizeof(AppState_struct));
    
    //we want to transfer this instance as the appstate
    *appstate =  as;

    //assign the locations and values
    as->frame = SDL_calloc(64*32,sizeof(bool));
    as->keys = SDL_calloc(16,sizeof(bool));
    as->mem = SDL_calloc(4096,sizeof(uint8_t));
    as->V = SDL_calloc(16,sizeof(uint8_t));
    as->I = 0;
    as->Delay = 0;
    as->Sound = 0;
    as->PC = 0x200;
    as->SP = 0;
    as->Stack = SDL_calloc(16,sizeof(uint16_t));
    as->waiting_for_key = false;




    //mandatory fontset
    uint8_t fontset[80] =
    { 
      0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
      0x20, 0x60, 0x20, 0x20, 0x70, // 1
      0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
      0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
      0x90, 0x90, 0xF0, 0x10, 0x10, // 4
      0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
      0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
      0xF0, 0x10, 0x20, 0x40, 0x40, // 7
      0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
      0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
      0xF0, 0x90, 0xF0, 0x90, 0x90, // A
      0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
      0xF0, 0x80, 0x80, 0x80, 0xF0, // C
      0xE0, 0x90, 0x90, 0x90, 0xE0, // D
      0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
      0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };
    
    //load the fonts starting from memory 0
    for(int i = 0 ; i < 80 ; i++){
	as->mem[i] = fontset[i];
    }

    // open file in read binary mode
    // copy .ch8 program into memory
    FILE *fptr = fopen(program_name, "rb"); 
    for (int i = 0; !feof(fptr); i++ ){
	fread(&(as->mem[512+i]),sizeof(uint8_t),1,fptr);
    }
    fclose(fptr);


    // SDL Copy paste
    SDL_SetAppMetadata("Example Renderer Clear", "1.0", "com.example.renderer-clear");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        if(logs_on)SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!SDL_CreateWindowAndRenderer("examples/renderer/clear", 640, 480, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        if(logs_on)SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_DISABLED);
    

    // SOUND INIT SDL Copy paste
    SDL_AudioSpec spec;
    spec.channels = 1;
    spec.format = SDL_AUDIO_F32;
    spec.freq = 8000;
    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!stream) {
        if(logs_on)SDL_Log("Couldn't create audio stream: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_ResumeAudioStreamDevice(stream);

    return SDL_APP_CONTINUE;  
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    //get the struct from appsate
    AppState_struct *as = appstate; 

    //decrement sound and delay reg values
    if (as->Sound > 0) as->Sound--;
    if (as->Delay > 0) as->Delay--;
    //if no sound stop the audio stream
    //I have no idea if doing it this way is good
    if (!(as->Sound)){
	SDL_PauseAudioStreamDevice(stream);
    }
    else SDL_ResumeAudioStreamDevice(stream);

    // Fx0A's stuff, poor coding
    if(as->waiting_for_key){
	if(as->waiting_for_key > 1){ 
	    if(logs_on)SDL_Log("waiting for key release\n");
	    for (int i = 0; i < 16; i++){
		if (as->keys[as->waiting_for_key % 100] == 0) {
		    as->waiting_for_key = false;
		    break;
		}
	    }
	}
	if(as->waiting_for_key == 1){ 
	    if(logs_on)SDL_Log("waiting for key press\n");
	    for (int i = 0; i < 16; i++){
		if (as->keys[i]){
		    as->V[opcode_2] = i;
		    as->waiting_for_key = 100 + i;
		    break;
		}
	    }
	}
	SDL_Delay((1000/60));
	return SDL_APP_CONTINUE;
    }

    // we actually delay the speed of the emulation
    // it tries to do one iteration in 1/60 of a second
    // so here take current time, later in the code compare it
    // if that was fast, do SDL_Delay so slow down
    Uint64 IterateStart_time = SDL_GetTicks();

    uint16_t opcode = (as->mem[as->PC] << 8) | (as->mem[as->PC+1]);
    uint8_t opcode_0 = ( opcode & 0x000F );
    uint8_t opcode_1 = ( opcode & 0x00F0 ) >> 4;
    opcode_2 = ( opcode & 0x0F00 ) >> 8;
    uint8_t opcode_3 = ( opcode & 0xF000 ) >> 12;
    uint16_t NNN = opcode & 0x0FFF;
    uint16_t NN = opcode & 0x00FF;

    if(logs_on)SDL_Log ("cycle: %d opcode: 0x%X PC: %d \n", cycle, opcode,as->PC);
    as->PC += 2;

    // I will use as->mem[0xFF] as a value holder
    // no special reason to choose that address
    switch (opcode_3){
	case 0x0:
	    switch (opcode_0){
		case 0x0:
		    if(opcode == 0x0) break;
		    clear_screen = true;
		    break;
		case 0xE:
		    as->PC = as->Stack[as->SP-1];
		    as->SP--;
		    break;
		}
	    break;
	case 0x1:
	    as->PC = NNN;
	    break;
	case 0x2:
	    as->Stack[as->SP] = as->PC;
	    as->SP++;
	    as->PC = NNN;
	    break;
	case 0x3:
	    if( Vx == NN) as->PC += 2;
	    break;
	case 0x4:
	    if( Vx != NN) as->PC += 2;
	    break;
	case 0x5:
	    if( Vx == Vy) as->PC += 2;
	    break;
	case 0x6:
	    Vx = NN;
	    break;
	case 0x7:
	    Vx += NN;
	    break;
	case 0x8:
	    switch (opcode_0){
		case 0x0:
		    Vx = Vy;
		    break;
		case 0x1:
		    Vx |= Vy;
		    Vf = 0;
		    break;
		case 0x2:
		    Vx &= Vy;
		    Vf = 0;
		    break;
		case 0x3:
		    Vx ^= Vy;
		    Vf = 0;
		    break;
		case 0x4:
		    Vx += Vy;
		    // Vy can't be bigger than unsigned Vx + Vy
		    // if so overflow happened
		    Vf = (Vy > Vx ? 1 : 0);
		    break;
		case 0x5:
		    as->mem[0xFF] = (Vy > Vx ? 0 : 1);
		    Vx -= Vy ;
		    Vf = as->mem[0xFF];
		    break;
		case 0x6:
		    Vx = Vy ;
		    as->mem[0xFF] = Vx & 0x01;
		    Vx >>= 1;
		    Vf = as->mem[0xFF];
		    break;
		case 0x7:
		    Vx = Vy - Vx;
		    Vf = (Vx > Vy) ? 0 : 1;
		    break;
		case 0xE:
		    as->mem[0xFF] = (Vy & 0x80) >> 7;
		    Vy <<= 1;
		    Vx = Vy;
		    Vf = as->mem[0xFF];
		    break;
	    }
	    break;
	case 0x9:
	    if( Vx != Vy) as->PC += 2;
	    break;
	case 0xA:
	    as->I = NNN;
	    break;
	case 0xB:
	    as->PC = as->V[0x0] + NNN;
	    break;
	case 0xC:
	    Vx = SDL_rand(256) & NN;
	    break;
	case 0xD:
	    int x = Vx % 64;
	    // %64 because we want it to wrap around
	    // from the other side of the screen
	    
	    int y = Vy ;
	    // y has not that wrapping thing
	    for (int j = 0; j < opcode_0; j++){
		for (int i = 0; i < 8; i++){
		    if ( (y + j) >= 32) break;
		    // if y + j is outside don't contain that pixel
		    uint8_t bit = 0x80 >> i;
		    int pixel = (x + i) + (y + j)*64;
		    if((as->mem[as->I + j] & bit)) {
			if (as->frame[pixel]) Vf = 1;
			as->frame[pixel] ^= 1;
		    }
		}
	    }
	    break;
	case 0xE:
	    switch (opcode_0){
		case 0xE:
		    if ( as->keys[(Vx & 0x0F)]) as->PC += 2;
		    break;
		case 0x1:
		    if ( !as->keys[(Vx & 0x0F)]) as->PC += 2;
		    break;

	    }
	    break;
	case 0xF:
	    switch ((opcode_1 << 4) + opcode_0){
		case 0x07:
		    Vx = as->Delay;
		    break;
		case 0x0A:
		    as->waiting_for_key = true;
		    break;
		case 0x15:
		    as->Delay = Vx;
		    break;
		case 0x18:
		    as->Sound = Vx;
		    break;
		case 0x1E:
		    as->I += Vx;
		    break;
		case 0x29:
		    as->I = (Vx & 0xF) * 5;
		    break;
		case 0x33:
		    for (int i = 2; i >= 0 ; i--){
			as->mem[as->I+(2-i)] =  (Vx / (uint8_t) SDL_pow(10,i)) % 10; 
		    }
		    break;
		case 0x55:
		    for (int i = 0; i <= opcode_2 ; i++){
			as->mem[as->I + i] = as->V[i];
		    }
			as->I += opcode_2 + 1;
		    break;
		case 0x65:
		    for (int i = 0; i <= opcode_2 ; i++){
			as->V[i] = as->mem[as->I + i];
		    }
			as->I += opcode_2 + 1;
		    break;
	    }
	    break;
	default:
	    if(logs_on)SDL_Log ("Unknown opcode: 0x%X head: %X\n", opcode,opcode_3);
	    break;
    }

    cycle++;

    //draw background
    SDL_SetRenderDrawColor(renderer, 33, 33, 33, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, SDL_ALPHA_OPAQUE);
    SDL_FRect rect;
    rect.x = rect.y = offset;
    rect.w = 64*pixel_size;
    rect.h = 32*pixel_size;
    SDL_RenderFillRect(renderer, &rect);

    // if gonna clear the screen
    // set all the frame values to 0
    if ( clear_screen ){
	for (int i = 0; i < 64*32; i++){
	    as->frame[i] = false;
	}
	clear_screen = false;
    }

    // draw the turned on pixels
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, SDL_ALPHA_OPAQUE);
    rect.w = rect.h = 1 * pixel_size;
    for(int i = 0;i < 64*32; i++){
	if( as->frame[i] ){
	    rect.x = offset + ((i%64)*pixel_size);
	    rect.y = offset + ((i/64)*pixel_size);
	    SDL_RenderFillRect(renderer, &rect);
	}
    }
    

    //Copy paste SDL code
    const int minimum_audio = (8000 * sizeof (float)) / 4;  
    if (SDL_GetAudioStreamQueued(stream) < minimum_audio) {
        static float samples[512];  
        int i;
        for (i = 0; i < SDL_arraysize(samples); i++) {
            const int freq = 440;
            const float phase = current_sine_sample * freq / 8000.0f;
            samples[i] = SDL_sinf(phase * 2 * SDL_PI_F);
            current_sine_sample++;
        }
        current_sine_sample %= 8000;
        SDL_PutAudioStreamData(stream, samples, sizeof (samples));
    }

    //and here is the delay thing we talked about
    SDL_RenderPresent(renderer);
    Uint32 CalculatedDelay = (1000/60) - (Uint32) (SDL_GetTicks() - IterateStart_time);
    if (CalculatedDelay < (1000/60)) SDL_Delay(CalculatedDelay);

    return SDL_APP_CONTINUE;  
}

void SDL_AppQuit(void *appstate, SDL_AppResult result){}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    AppState_struct *as = appstate;

    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  
    }
    else if (event->type == SDL_EVENT_KEY_DOWN) {
	switch (event->key.key){
	    case SDLK_0:
		as->keys[0] = true;
		break;
	    case SDLK_1:
		as->keys[1] = true;
		break;
	    case SDLK_2:
		as->keys[2] = true;
		break;
	    case SDLK_3:
		as->keys[3] = true;
		break;
	    case SDLK_4:
		as->keys[4] = true;
		break;
	    case SDLK_5:
		as->keys[5] = true;
		break;
	    case SDLK_6:
		as->keys[6] = true;
		break;
	    case SDLK_7:
		as->keys[7] = true;
		break;
	    case SDLK_8:
		as->keys[8] = true;
		break;
	    case SDLK_9:
		as->keys[9] = true;
		break;
	    case SDLK_A:
		as->keys[0xA] = true;
		break;
	    case SDLK_B:
		as->keys[0xB] = true;
		break;
	    case SDLK_C:
		as->keys[0xC] = true;
		break;
	    case SDLK_D:
		as->keys[0xD] = true;
		break;
	    case SDLK_E:
		as->keys[0xE] = true;
		break;
	    case SDLK_F:
		as->keys[0xF] = true;
		break;

	    default:
		if(logs_on)SDL_Log("Wow, just pressed the %s key!", SDL_GetKeyName(event->key.key));
	}	

    }
    else if (event->type == SDL_EVENT_KEY_UP) {
	switch (event->key.key){
	    case SDLK_0:
		as->keys[0] = false;
		break;
	    case SDLK_1:
		as->keys[1] = false;
		break;
	    case SDLK_2:
		as->keys[2] = false;
		break;
	    case SDLK_3:
		as->keys[3] = false;
		break;
	    case SDLK_4:
		as->keys[4] = false;
		break;
	    case SDLK_5:
		as->keys[5] = false;
		break;
	    case SDLK_6:
		as->keys[6] = false;
		break;
	    case SDLK_7:
		as->keys[7] = false;
		break;
	    case SDLK_8:
		as->keys[8] = false;
		break;
	    case SDLK_9:
		as->keys[9] = false;
		break;
	    case SDLK_A:
		as->keys[0xA] = false;
		break;
	    case SDLK_B:
		as->keys[0xB] = false;
		break;
	    case SDLK_C:
		as->keys[0xC] = false;
		break;
	    case SDLK_D:
		as->keys[0xD] = false;
		break;
	    case SDLK_E:
		as->keys[0xE] = false;
		break;
	    case SDLK_F:
		as->keys[0xF] = false;
		break;

	    default:
		if(logs_on)SDL_Log("Yup, released the %s key!", SDL_GetKeyName(event->key.key));
	}	

    }
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}
