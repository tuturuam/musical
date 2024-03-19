#include <math.h>
#include <raylib.h>
#include <raymath.h>
#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"
#define SUPPORT_MODULE_RAUDIO
#include <stdio.h>

#define GLSL_VERSION 330

static float *filterBuffer = NULL;
static unsigned int filterBufferSize = 0;
static unsigned int filterReadIndex = 2;
static unsigned int filterWriteIndex = 0;

static float averageVolume[2] = {0.0f};

static void AudioProcessEffectLPF(void *buffer, unsigned int frames);
static void AudioProcessEffectAverage(void *buffer, unsigned int frames);

int main() {
  const int screenWidth = 2560;
  const int screenHeight = 1440;
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  SetConfigFlags(FLAG_WINDOW_TRANSPARENT);
  InitWindow(screenWidth, screenHeight, "test");
  SetWindowMonitor(1);

  InitAudioDevice();
  AttachAudioMixedProcessor(AudioProcessEffectAverage);

  // Define the camera to look into our 3d world
  Camera camera = {0};
  camera.position = (Vector3){2.0f, 4.0f, 6.0f}; // Camera position
  camera.target = (Vector3){0.0f, 0.0f, 0.0f};   // Camera looking at point
  camera.up =
      (Vector3){0.0f, 1.0f, 0.0f}; // Camera up vector (rotation towards target)
  camera.fovy = 103.0f;            // Camera field-of-view Y
  camera.projection = CAMERA_PERSPECTIVE; // Camera projection type

  // Load models
  Model plane = LoadModelFromMesh(GenMeshPlane(40.0f, 40.0f, 3, 3));
  Model sphere = LoadModel("./resources/models/barracks.obj");
  Texture2D texture = LoadTexture("./resources/models/barracks_diffuse.png");
  Texture2D spec = LoadTexture("./resources/mask.png");

  // Load shader
  // Shader shader = LoadShader(
  //     TextFormat("resources/shaders/glsl%i/lighting.vs", GLSL_VERSION),
  //     TextFormat("./resources/shaders/glsl330/raymarching.fs",
  //     GLSL_VERSION));
  Shader shaders[2] = {0};

  shaders[0] = LoadShader(
      TextFormat("./resources/shaders/glsl330/lighting.vs", GLSL_VERSION),
      TextFormat("./resources/shaders/glsl330/lighting.fs", GLSL_VERSION));

  shaders[1] =
      LoadShader(0, TextFormat("./resources/shaders/glsl330/cross_hatching.fs",
                               GLSL_VERSION));

  Light light = CreateLight(LIGHT_POINT, (Vector3){0, 3, 0}, Vector3Zero(),
                            SKYBLUE, shaders[0]);
  int ambientLoc = GetShaderLocation(shaders[0], "ambient");
  plane.materials[0].shader = shaders[0];
  plane.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
  // plane.materials[0].maps[MATERIAL_MAP_SPECULAR].texture = spec;
  sphere.materials[0].shader = shaders[0];
  sphere.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
  // sphere.materials[0].maps[MATERIAL_MAP_SPECULAR].texture = spec;
  for (int i = 0; i < 2; i++) {
    shaders[i].locs[SHADER_LOC_VECTOR_VIEW] =
        GetShaderLocation(shaders[i], "viewPos");

    // ambient light
    SetShaderValue(shaders[i], ambientLoc, (float[4]){0.0f, 0.05, 0.1f, 1.0f},
                   SHADER_UNIFORM_VEC4);
  }

  printf("hello world\n");
  printf("%s\n", GetMonitorName(1));

  // Audio
  Music music = LoadMusicStream("./test.mp3"); // Load music stream
                                               // load from file
  // Allocate buffer
  filterBufferSize = 4 * (48000 * 2);
  PlayMusicStream(music);

  bool enableEffectLPF = false;
  bool phonk = false;
  bool mouseLock = true;

  SetTargetFPS(240);

  RenderTexture2D target = LoadRenderTexture(screenWidth, screenHeight);

  float lastVol = 0.1;
  float runTime = 0.0f;
  while (!WindowShouldClose()) {
    // update
    float k = 0.01f;
    runTime += GetFrameTime();
    if (IsKeyPressed(KEY_TAB)) {
      mouseLock = !mouseLock;
      if (mouseLock) {
        ShowCursor();
      } else {
        DisableCursor();
      }
    }
    if (mouseLock) {
      camera.target = (Vector3){0.0f, 0.0f, 0.0f};
      UpdateCamera(&camera, CAMERA_ORBITAL);
    } else {
      UpdateCamera(&camera, CAMERA_FIRST_PERSON);
    }
    // point towards 0, 0, 0
    float cameraPos[3] = {camera.position.x, camera.position.y,
                          camera.position.z};
    // Set shader required uniform values
    SetShaderValue(shaders[0], shaders[0].locs[SHADER_LOC_VECTOR_VIEW],
                   cameraPos, SHADER_UNIFORM_VEC3);
    UpdateLightValues(shaders[0], light);
    UpdateMusicStream(music);
    if (IsKeyPressed(KEY_F)) {
      enableEffectLPF = !enableEffectLPF;
      if (enableEffectLPF)
        AttachAudioStreamProcessor(music.stream, AudioProcessEffectLPF);
      else
        DetachAudioStreamProcessor(music.stream, AudioProcessEffectLPF);
    }
    if (IsKeyPressed(KEY_K)) {
      phonk = !phonk;
      if (phonk)
        SetMusicPitch(music, 0.8f);
      else
        SetMusicPitch(music, 1.0f);
    }

    // draw
    // BeginTextureMode(target);
    // ClearBackground(BLANK);
    // BeginMode3D(camera);
    // DrawModel(plane, Vector3Zero(), 1.0f, RAYWHITE);

    // float lerped = 100;
    // for (int i = 0; i < 1; i++) {
    //   k = 1.0f - powf(k, GetFrameTime());
    //   lerped = Lerp(lastVol, averageVolume[i], k);
    //   if (lerped < 0.1)
    //     lerped = 0.1;
    //   // printf("%.2f\n", k);
    //   // printf("%.2f\n", lerped);
    //   DrawModel(sphere, (Vector3){0.0f, 0.0f, 0.0f}, lerped, WHITE);
    // }

    // DrawSphereEx(light.position, 0.2f, 4, 4, WHITE);
    // DrawGrid(40, 1.0f);
    // lastVol = lerped;
    // EndMode3D(); // End 3d mode drawing, returns to orthographic 2d mode
    // EndTextureMode();

    BeginDrawing();
    ClearBackground(BLANK);

    // BeginShaderMode(shaders[(int)phonk]);
    // NOTE: Render texture must be y-flipped due to default OpenGL
    // coordinates
    // (left-bottom)
    // DrawTextureRec(target.texture,
    //                (Rectangle){0, 0, (float)target.texture.width,
    //                            (float)-target.texture.height},
    //                (Vector2){0, 0}, WHITE);
    // EndShaderMode();

    BeginMode3D(camera);
    DrawModel(plane, Vector3Zero(), 1.0f, RAYWHITE);

    float lerped = 100;
    for (int i = 0; i < 1; i++) {
      k = 1.0f - powf(k, GetFrameTime());
      lerped = Lerp(lastVol, averageVolume[i], k);
      if (lerped < 0.1)
        lerped = 0.1;
      // printf("%.2f\n", k);
      // printf("%.2f\n", lerped);
      DrawModel(sphere, (Vector3){0.0f, 0.0f, 0.0f}, lerped, WHITE);
    }

    DrawSphereEx(light.position, 0.2f, 4, 4, WHITE);
    DrawGrid(40, 1.0f);
    lastVol = lerped;
    EndMode3D(); // End 3d mode drawing, returns to orthographic 2d mode
    // Draw 2d shapes and text over drawn texture
    DrawRectangle(0, 9, 300, 65, Fade(LIGHTGRAY, 0.7f));
    DrawFPS(0, 9);
    DrawText(TextFormat("PRESS F FOR LPF: %i", enableEffectLPF), 0, 31, 20,
             LIME);
    DrawText(TextFormat("PRESS K FOR SLOMO: %i", phonk), 0, 53, 20, LIME);
    EndDrawing();
  }

  DetachAudioStreamProcessor(music.stream, AudioProcessEffectLPF);
  DetachAudioMixedProcessor(AudioProcessEffectAverage);
  UnloadMusicStream(music); // unload music stream

  UnloadTexture(texture);

  UnloadModel(plane);
  UnloadModel(sphere);
  for (int i = 0; i < 2; i++) {
    UnloadShader(shaders[i]);
  }

  CloseAudioDevice();

  CloseWindow();

  return 0;
}

static void AudioProcessEffectLPF(void *buffer, unsigned int frames) {
  static float low[2] = {0.0f, 0.0f};
  static const float cutoff = 50.0f / 44100.0f;      // 70 Hz lowpass filter
  const float k = cutoff / (cutoff + 0.1591549431f); // RC filter formula
  const float attack = 0.5f;

  // Converts the buffer data before using it
  float *bufferData = (float *)buffer;
  float slow = 1;
  for (unsigned int i = 0; i < frames * 2; i += 2) {
    const float l = bufferData[i];
    const float r = bufferData[i + 1];
    low[0] += k * (l - low[0]);
    low[1] += k * (r - low[1]);
    bufferData[i] = low[0];
    bufferData[i + 1] = low[1];
  }
}
static void AudioProcessEffectAverage(void *buffer, unsigned int frames) {
  // Converts the buffer data before using it
  float *bufferData = (float *)buffer;
  float average = 0.0f;
  for (unsigned int i = 0; i < frames * 2; i += 2) {
    average += fabsf(bufferData[i]) / frames;
    average += fabsf(bufferData[i + 1]) / frames;
  }
  for (int i = 0; i < 2; i++)
    averageVolume[i] = averageVolume[i + 1];
  averageVolume[1] = average;
}
