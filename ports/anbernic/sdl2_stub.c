/**
 *    Name:        sdl2_stub.c
 *    Description: a stand-in for the handheld's own SDL2 (libSDL2-2.0.so.0) to link the game against: the
 *                 functions the window port calls, by name only
 *    Version:     0.1.0
 *    Created:     23-10-2026
 *    Modified:    07-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    The game runs with the SDL2 the handheld's firmware has (Anbernic's 2.0.12 on the stock one, which
 *    draws through its Mali GPU), so the build needs none of its own: linked against this library of the
 *    same name, the executable asks for libSDL2-2.0.so.0 and takes the console's when it starts. It is
 *    never run nor shipped. A function the port starts to call is added here (the link says which).
 */

void SDL_ClearQueuedAudio(void);
void SDL_ClearQueuedAudio(void)
{
}

void SDL_CloseAudioDevice(void);
void SDL_CloseAudioDevice(void)
{
}

void SDL_ConvertSurfaceFormat(void);
void SDL_ConvertSurfaceFormat(void)
{
}

void SDL_CreateRGBSurfaceWithFormatFrom(void);
void SDL_CreateRGBSurfaceWithFormatFrom(void)
{
}

void SDL_CreateRenderer(void);
void SDL_CreateRenderer(void)
{
}

void SDL_CreateTexture(void);
void SDL_CreateTexture(void)
{
}

void SDL_CreateTextureFromSurface(void);
void SDL_CreateTextureFromSurface(void)
{
}

void SDL_CreateWindow(void);
void SDL_CreateWindow(void)
{
}

void SDL_Delay(void);
void SDL_Delay(void)
{
}

void SDL_DestroyRenderer(void);
void SDL_DestroyRenderer(void)
{
}

void SDL_DestroyTexture(void);
void SDL_DestroyTexture(void)
{
}

void SDL_DestroyWindow(void);
void SDL_DestroyWindow(void)
{
}

void SDL_FlushEvents(void);
void SDL_FlushEvents(void)
{
}

void SDL_FreeSurface(void);
void SDL_FreeSurface(void)
{
}

void SDL_GameControllerAddMapping(void);
void SDL_GameControllerAddMapping(void)
{
}

void SDL_GameControllerClose(void);
void SDL_GameControllerClose(void)
{
}

void SDL_GameControllerGetAxis(void);
void SDL_GameControllerGetAxis(void)
{
}

void SDL_GameControllerGetButton(void);
void SDL_GameControllerGetButton(void)
{
}

void SDL_GameControllerGetButtonFromString(void);
void SDL_GameControllerGetButtonFromString(void)
{
}

void SDL_GameControllerGetJoystick(void);
void SDL_GameControllerGetJoystick(void)
{
}

void SDL_GameControllerGetStringForButton(void);
void SDL_GameControllerGetStringForButton(void)
{
}

void SDL_GameControllerMappingForGUID(void);
void SDL_GameControllerMappingForGUID(void)
{
}

void SDL_GameControllerName(void);
void SDL_GameControllerName(void)
{
}

void SDL_GameControllerNumMappings(void);
void SDL_GameControllerNumMappings(void)
{
}

void SDL_GameControllerOpen(void);
void SDL_GameControllerOpen(void)
{
}

void SDL_GetBasePath(void);
void SDL_GetBasePath(void)
{
}

void SDL_GetCPUCount(void);
void SDL_GetCPUCount(void)
{
}

void SDL_GetCurrentAudioDriver(void);
void SDL_GetCurrentAudioDriver(void)
{
}

void SDL_GetCurrentVideoDriver(void);
void SDL_GetCurrentVideoDriver(void)
{
}

void SDL_GetDisplayUsableBounds(void);
void SDL_GetDisplayUsableBounds(void)
{
}

void SDL_GetError(void);
void SDL_GetError(void)
{
}

void SDL_GetKeyName(void);
void SDL_GetKeyName(void)
{
}

void SDL_GetKeyboardState(void);
void SDL_GetKeyboardState(void)
{
}

void SDL_GetModState(void);
void SDL_GetModState(void)
{
}

void SDL_GetMouseState(void);
void SDL_GetMouseState(void)
{
}

void SDL_GetPerformanceCounter(void);
void SDL_GetPerformanceCounter(void)
{
}

void SDL_GetPerformanceFrequency(void);
void SDL_GetPerformanceFrequency(void)
{
}

void SDL_GetPrefPath(void);
void SDL_GetPrefPath(void)
{
}

void SDL_GetQueuedAudioSize(void);
void SDL_GetQueuedAudioSize(void)
{
}

void SDL_GetRendererInfo(void);
void SDL_GetRendererInfo(void)
{
}

void SDL_GetRendererOutputSize(void);
void SDL_GetRendererOutputSize(void)
{
}

void SDL_GetScancodeFromName(void);
void SDL_GetScancodeFromName(void)
{
}

void SDL_GetScancodeName(void);
void SDL_GetScancodeName(void)
{
}

void SDL_GetTicks(void);
void SDL_GetTicks(void)
{
}

void SDL_GetVersion(void);
void SDL_GetVersion(void)
{
}

void SDL_GetWindowDisplayIndex(void);
void SDL_GetWindowDisplayIndex(void)
{
}

void SDL_GetWindowFlags(void);
void SDL_GetWindowFlags(void)
{
}

void SDL_Init(void);
void SDL_Init(void)
{
}

void SDL_InitSubSystem(void);
void SDL_InitSubSystem(void)
{
}

void SDL_IsGameController(void);
void SDL_IsGameController(void)
{
}

void SDL_JoystickClose(void);
void SDL_JoystickClose(void)
{
}

void SDL_JoystickGetAxis(void);
void SDL_JoystickGetAxis(void)
{
}

void SDL_JoystickGetButton(void);
void SDL_JoystickGetButton(void)
{
}

void SDL_JoystickGetDeviceGUID(void);
void SDL_JoystickGetDeviceGUID(void)
{
}

void SDL_JoystickGetGUID(void);
void SDL_JoystickGetGUID(void)
{
}

void SDL_JoystickGetGUIDString(void);
void SDL_JoystickGetGUIDString(void)
{
}

void SDL_JoystickGetHat(void);
void SDL_JoystickGetHat(void)
{
}

void SDL_JoystickInstanceID(void);
void SDL_JoystickInstanceID(void)
{
}

void SDL_JoystickName(void);
void SDL_JoystickName(void)
{
}

void SDL_JoystickNameForIndex(void);
void SDL_JoystickNameForIndex(void)
{
}

void SDL_JoystickNumAxes(void);
void SDL_JoystickNumAxes(void)
{
}

void SDL_JoystickNumButtons(void);
void SDL_JoystickNumButtons(void)
{
}

void SDL_JoystickNumHats(void);
void SDL_JoystickNumHats(void)
{
}

void SDL_JoystickOpen(void);
void SDL_JoystickOpen(void)
{
}

void SDL_JoystickUpdate(void);
void SDL_JoystickUpdate(void)
{
}

void SDL_LoadBMP_RW(void);
void SDL_LoadBMP_RW(void)
{
}

void SDL_LockTexture(void);
void SDL_LockTexture(void)
{
}

void SDL_Log(void);
void SDL_Log(void)
{
}

void SDL_MapRGB(void);
void SDL_MapRGB(void)
{
}

void SDL_NumJoysticks(void);
void SDL_NumJoysticks(void)
{
}

void SDL_OpenAudioDevice(void);
void SDL_OpenAudioDevice(void)
{
}

void SDL_PauseAudioDevice(void);
void SDL_PauseAudioDevice(void)
{
}

void SDL_PollEvent(void);
void SDL_PollEvent(void)
{
}

void SDL_PumpEvents(void);
void SDL_PumpEvents(void)
{
}

void SDL_QueueAudio(void);
void SDL_QueueAudio(void)
{
}

void SDL_Quit(void);
void SDL_Quit(void)
{
}

void SDL_QuitSubSystem(void);
void SDL_QuitSubSystem(void)
{
}

void SDL_RWFromFile(void);
void SDL_RWFromFile(void)
{
}

void SDL_RenderClear(void);
void SDL_RenderClear(void)
{
}

void SDL_RenderCopy(void);
void SDL_RenderCopy(void)
{
}

void SDL_RenderPresent(void);
void SDL_RenderPresent(void)
{
}

void SDL_RenderSetIntegerScale(void);
void SDL_RenderSetIntegerScale(void)
{
}

void SDL_RenderSetLogicalSize(void);
void SDL_RenderSetLogicalSize(void)
{
}

void SDL_SetColorKey(void);
void SDL_SetColorKey(void)
{
}

void SDL_SetHint(void);
void SDL_SetHint(void)
{
}

void SDL_SetRelativeMouseMode(void);
void SDL_SetRelativeMouseMode(void)
{
}

void SDL_SetRenderDrawColor(void);
void SDL_SetRenderDrawColor(void)
{
}

void SDL_SetRenderTarget(void);
void SDL_SetRenderTarget(void)
{
}

void SDL_SetTextureBlendMode(void);
void SDL_SetTextureBlendMode(void)
{
}

void SDL_SetTextureColorMod(void);
void SDL_SetTextureColorMod(void)
{
}

void SDL_SetTextureScaleMode(void);
void SDL_SetTextureScaleMode(void)
{
}

void SDL_SetWindowFullscreen(void);
void SDL_SetWindowFullscreen(void)
{
}

void SDL_SetWindowIcon(void);
void SDL_SetWindowIcon(void)
{
}

void SDL_SetWindowSize(void);
void SDL_SetWindowSize(void)
{
}

void SDL_SetWindowTitle(void);
void SDL_SetWindowTitle(void)
{
}

void SDL_ShowSimpleMessageBox(void);
void SDL_ShowSimpleMessageBox(void)
{
}

void SDL_UnlockTexture(void);
void SDL_UnlockTexture(void)
{
}

void SDL_UpdateTexture(void);
void SDL_UpdateTexture(void)
{
}

void SDL_free(void);
void SDL_free(void)
{
}

void SDL_memset(void);
void SDL_memset(void)
{
}

void SDL_strcasecmp(void);
void SDL_strcasecmp(void)
{
}

void SDL_strdup(void);
void SDL_strdup(void)
{
}

void SDL_strlcat(void);
void SDL_strlcat(void)
{
}

void SDL_strlcpy(void);
void SDL_strlcpy(void)
{
}
