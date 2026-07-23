#include "SDL.h"
#include "i_video.h"
#include "joyapi.h"
#include <string.h>

int joy_ack;

bool Up, Down, Left, Right;
bool Start, Back, LeftShoulder, RightShoulder;
bool AButton, BButton, XButton, YButton;

int16_t StickX, StickY, TriggerLeft, TriggerRight;

SDL_GameController* ControllerHandles[MAX_CONTROLLERS];
SDL_Haptic* RumbleHandles[MAX_CONTROLLERS];
JoyPadState joypads[MAX_CONTROLLERS];
int joy_num_pads;

int MaxJoysticks;
int ControllerIndex;
int JoystickIndex;

int AButtonconvert, BButtonconvert, XButtonconvert, YButtonconvert;
static unsigned int lastTime = 0;

/***************************************************************************
IPT_CalJoy() - Open Gamecontroller
 ***************************************************************************/
void 
IPT_CalJoy(
	void
)
{
	SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);

	MaxJoysticks = SDL_NumJoysticks();
	ControllerIndex = 0;
	AButtonconvert = 0;
	BButtonconvert = 0;
	XButtonconvert = 0;
	YButtonconvert = 0;

	for (JoystickIndex = 0; JoystickIndex < MaxJoysticks; ++JoystickIndex)
	{
		if (!SDL_IsGameController(JoystickIndex))
		{
			continue;
		}
		if (ControllerIndex >= MAX_CONTROLLERS)
		{
			break;
		}
		
		ControllerHandles[ControllerIndex] = SDL_GameControllerOpen(JoystickIndex);
		RumbleHandles[ControllerIndex] = SDL_HapticOpen(JoystickIndex);
		
		if (SDL_HapticRumbleInit(RumbleHandles[ControllerIndex]) != 0)
		{
			SDL_HapticClose(RumbleHandles[ControllerIndex]);
			RumbleHandles[ControllerIndex] = 0;
		}
	    
		ControllerIndex++;
		GetJoyButtonMapping();
	}
}

/***************************************************************************
IPT_CloJoy() - Close Gamecontroller
 ***************************************************************************/
void  
IPT_CloJoy(
	void
)
{
	for (ControllerIndex = 0; ControllerIndex < MAX_CONTROLLERS; ++ControllerIndex)
	{
		if (ControllerHandles[ControllerIndex])
		{
			if (RumbleHandles[ControllerIndex])
				SDL_HapticClose(RumbleHandles[ControllerIndex]);
			
			SDL_GameControllerClose(ControllerHandles[ControllerIndex]);
		}
	}
}

/***************************************************************************
I_HandleJoystickEvent() - Get current button or axis status
 ***************************************************************************/
void 
I_HandleJoystickEvent(
	SDL_Event *sdlevent
)
{
	joy_num_pads = 0;

	for (ControllerIndex = 0;
		ControllerIndex < MAX_CONTROLLERS;
		++ControllerIndex)
	{
		JoyPadState *jp = &joypads[ControllerIndex];

		memset(jp, 0, sizeof(*jp));

		if (ControllerHandles[ControllerIndex] != 0 && SDL_GameControllerGetAttached(ControllerHandles[ControllerIndex]))
		{
			jp->Up = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_DPAD_UP);
			jp->Down = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_DPAD_DOWN);
			jp->Left = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_DPAD_LEFT);
			jp->Right = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
			jp->Start = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_START);
			jp->Back = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_BACK);
			jp->LeftShoulder = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
			jp->RightShoulder = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
			jp->AButton = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_A);
			jp->BButton = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_B);
			jp->XButton = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_X);
			jp->YButton = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_Y);

			jp->StickX = SDL_GameControllerGetAxis(ControllerHandles[ControllerIndex], SDL_CONTROLLER_AXIS_LEFTX) / 8000;
			jp->StickY = SDL_GameControllerGetAxis(ControllerHandles[ControllerIndex], SDL_CONTROLLER_AXIS_LEFTY) / 8000;
			jp->TriggerLeft = SDL_GameControllerGetAxis(ControllerHandles[ControllerIndex], SDL_CONTROLLER_AXIS_TRIGGERLEFT) / 8000;
			jp->TriggerRight = SDL_GameControllerGetAxis(ControllerHandles[ControllerIndex], SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / 8000;

			joy_num_pads = ControllerIndex + 1;
		}
	}

	/* Mirror pad 0 into legacy globals for menus / 1P joystick mode. */
	Up = joypads[0].Up;
	Down = joypads[0].Down;
	Left = joypads[0].Left;
	Right = joypads[0].Right;
	Start = joypads[0].Start;
	Back = joypads[0].Back;
	LeftShoulder = joypads[0].LeftShoulder;
	RightShoulder = joypads[0].RightShoulder;
	AButton = joypads[0].AButton;
	BButton = joypads[0].BButton;
	XButton = joypads[0].XButton;
	YButton = joypads[0].YButton;
	StickX = joypads[0].StickX;
	StickY = joypads[0].StickY;
	TriggerLeft = joypads[0].TriggerLeft;
	TriggerRight = joypads[0].TriggerRight;

	if (sdlevent->type == SDL_CONTROLLERBUTTONUP)
		joy_ack = 0;

	if (sdlevent->type == SDL_CONTROLLERBUTTONDOWN)
		joy_ack = 1;
}

void
IPT_ReadJoyPad(
	int pad,
	JoyPadState *out
)
{
	if (!out)
		return;
	if (pad < 0 || pad >= MAX_CONTROLLERS)
	{
		memset(out, 0, sizeof(*out));
		return;
	}
	*out = joypads[pad];
}

/***************************************************************************
GetJoyButtonMapping() - Detect connected Gamecontroller and map buttons for it
 ***************************************************************************/
void 
GetJoyButtonMapping(
	void
)
{
	for (ControllerIndex = 0;
		ControllerIndex < MAX_CONTROLLERS;
		++ControllerIndex)
	{
		switch (SDL_GameControllerTypeForIndex(ControllerIndex))
		{
		case SDL_CONTROLLER_TYPE_PS3:
		case SDL_CONTROLLER_TYPE_PS4:
		case SDL_CONTROLLER_TYPE_PS5:
			AButtonconvert = 0;
			BButtonconvert = 1;
			XButtonconvert = 3;
			YButtonconvert = 2;
			break;
		
		case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO:
		case SDL_CONTROLLER_TYPE_XBOX360:
		case SDL_CONTROLLER_TYPE_XBOXONE:
			AButtonconvert = 0;
			BButtonconvert = 1;
			XButtonconvert = 2;
			YButtonconvert = 3;
			break;
		
		default:
			if ((AButtonconvert == 0) && (BButtonconvert == 0) && (XButtonconvert == 0) && (YButtonconvert == 0))
			{
				AButtonconvert = 0;
				BButtonconvert = 1;
				XButtonconvert = 2;
				YButtonconvert = 3;
			}
			break;
		}
	}
}

/***************************************************************************
IPT_CalJoyRumbleLow() - Gamecontroller rumbles low
 ***************************************************************************/
void 
IPT_CalJoyRumbleLow(
	void
)
{
	for (ControllerIndex = 0;
		ControllerIndex < MAX_CONTROLLERS;
		++ControllerIndex)
	{
		if (ControllerHandles[ControllerIndex])
			SDL_GameControllerRumble(ControllerHandles[ControllerIndex], 0x3fff, 0x3fff, 1000);
	}
}

/***************************************************************************
IPT_CalJoyRumbleMedium() - Gamecontroller rumbles medium
 ***************************************************************************/
void 
IPT_CalJoyRumbleMedium(
	void
)
{
	for (ControllerIndex = 0;
		ControllerIndex < MAX_CONTROLLERS;
		++ControllerIndex)
	{
		if (ControllerHandles[ControllerIndex])
		    SDL_GameControllerRumble(ControllerHandles[ControllerIndex], 0x7ffe, 0x7ffe, 1000);
	}
}

/***************************************************************************
IPT_CalJoyRumbleHigh() - Gamecontroller rumbles high
 ***************************************************************************/
void 
IPT_CalJoyRumbleHigh(
	void
)
{
	for (ControllerIndex = 0;
		ControllerIndex < MAX_CONTROLLERS;
		++ControllerIndex)
	{
		if (ControllerHandles[ControllerIndex])
			SDL_GameControllerRumble(ControllerHandles[ControllerIndex], 0xbffd, 0xbffd, 1000);
	}
}

/***************************************************************************
JOY_Wait() - Waits for button to be released
 ***************************************************************************/
void 
JOY_Wait(
	int button
)
{
	while (StickX || StickY || Up || Down || Left || Right || Start || Back || LeftShoulder || RightShoulder || AButton || BButton || XButton || YButton)
	{
		I_GetEvent();
	}
}

/***************************************************************************
JOY_IsKey() - Tests to see if button is down if so waits for release
 ***************************************************************************/
int 
JOY_IsKey(
	int button
)
{
	if (StickX || StickY || Up || Down || Left || Right || Start || Back || LeftShoulder || RightShoulder || AButton || BButton || XButton || YButton)
	{
		JOY_Wait(button);
		
		return 1;
	}
    
	return 0;
}

/***************************************************************************
JOY_IsKeyInGameStart() - Tests to see if button is down if so waits for release
 ***************************************************************************/
int 
JOY_IsKeyInGameStart(
	int button
)
{
	if (Start)
	{
		JOY_Wait(button);
		
		return 1;
	}
	
	return 0;
}

/***************************************************************************
JOY_IsKeyInGameBack() - Tests to see if button is down if so waits for release
 ***************************************************************************/
int 
JOY_IsKeyInGameBack(
	int button
)
{

	if (Back)
	{
		JOY_Wait(button);
		
		return 1;
	}
	
	return 0;
}

/***************************************************************************
JOY_IsKeyMenu() - Tests to see if button is down if so waits for release
 ***************************************************************************/
int 
JOY_IsKeyMenu(
	int button
)
{
	if (RightShoulder || Back || BButton)
	{
		JOY_Wait(button);
		
		return 1;
	}
	
	return 0;
}

/***************************************************************************
JOY_IsScroll() - Scroll cursor in menu
 ***************************************************************************/
int 
JOY_IsScroll(
	int scrollflag
)
{
	unsigned int currentTime;
	currentTime = SDL_GetTicks();
	
	if (currentTime > lastTime + 200)
	{
		lastTime = currentTime;
		
		return 1;
	}
	
	return 0;
}