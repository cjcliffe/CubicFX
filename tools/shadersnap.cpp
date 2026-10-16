// Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//
// ShaderSnap: render one frame of a visualizer shader offscreen and save it
// as a PNG, without launching the full app. Used to eyeball shader changes
// (filtering, uniforms, layout) from the command line or scripts.
//
// Usage (run from the build directory so relative asset paths resolve):
//   ShaderSnap <frag> <out.png> [linear|point] [time]
// Defaults: filter linear, time 12. Audio uniforms get fixed dummy values.
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <raylib.h>

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("usage: ShaderSnap <frag> <out.png> [linear|point] [time]\n");
        return 2;
    }
    const char *frag = argv[1];
    const char *out = argv[2];
    bool linear = (argc < 4) || (strcmp(argv[3], "point") != 0);
    float t = (argc > 4) ? (float)atof(argv[4]) : 12.0f;

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(640, 360, "shadersnap");

    Shader sh = LoadShader("shaders/vertex_common.vs", frag);
    if (sh.id == 0) {
        printf("LOAD FAILED: %s\n", frag);
        CloseWindow();
        return 1;
    }

    float res[2] = { 640.0f, 360.0f };
    float mouse[2] = { 0.0f, 0.0f };
    float kick = 0.6f, vu = 0.6f, alpha = 1.0f;
    float rc[3] = { 1.0f, 0.5f, 0.25f };
    int loc;
    loc = GetShaderLocation(sh, "resolution");
    if (loc >= 0) SetShaderValue(sh, loc, res, SHADER_UNIFORM_VEC2);
    loc = GetShaderLocation(sh, "time");
    if (loc >= 0) SetShaderValue(sh, loc, &t, SHADER_UNIFORM_FLOAT);
    loc = GetShaderLocation(sh, "mouse");
    if (loc >= 0) SetShaderValue(sh, loc, mouse, SHADER_UNIFORM_VEC2);
    loc = GetShaderLocation(sh, "timerKick");
    if (loc >= 0) SetShaderValue(sh, loc, &kick, SHADER_UNIFORM_FLOAT);
    loc = GetShaderLocation(sh, "timerMid");
    if (loc >= 0) SetShaderValue(sh, loc, &vu, SHADER_UNIFORM_FLOAT);
    loc = GetShaderLocation(sh, "timerHigh");
    if (loc >= 0) SetShaderValue(sh, loc, &vu, SHADER_UNIFORM_FLOAT);
    loc = GetShaderLocation(sh, "vuLow");
    if (loc >= 0) SetShaderValue(sh, loc, &vu, SHADER_UNIFORM_FLOAT);
    loc = GetShaderLocation(sh, "vuMid");
    if (loc >= 0) SetShaderValue(sh, loc, &vu, SHADER_UNIFORM_FLOAT);
    loc = GetShaderLocation(sh, "vuHigh");
    if (loc >= 0) SetShaderValue(sh, loc, &vu, SHADER_UNIFORM_FLOAT);
    loc = GetShaderLocation(sh, "randColor");
    if (loc >= 0) SetShaderValue(sh, loc, rc, SHADER_UNIFORM_VEC3);
    loc = GetShaderLocation(sh, "randColor2");
    if (loc >= 0) SetShaderValue(sh, loc, rc, SHADER_UNIFORM_VEC3);
    loc = GetShaderLocation(sh, "blendAlpha");
    if (loc >= 0) SetShaderValue(sh, loc, &alpha, SHADER_UNIFORM_FLOAT);

    // Image textures the app would bind from front-matter config.
    Texture2D tex[3] = { 0 };
    const char *names[3] = { "tex0", "tex1", "tex2" };
    for (int i = 0; i < 3; i++) {
        if (GetShaderLocation(sh, names[i]) < 0) continue;
        Image img = LoadImage("png/tex_noise1.png");
        ImageFlipVertical(&img);
        tex[i] = LoadTextureFromImage(img);
        UnloadImage(img);
        SetTextureFilter(tex[i], linear ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);
    }

    RenderTexture2D rt = LoadRenderTexture(640, 360);
    BeginTextureMode(rt);
    ClearBackground(BLANK);
    BeginShaderMode(sh);
    for (int i = 0; i < 3; i++) {
        if (tex[i].id == 0) continue;
        SetShaderValueTexture(sh, GetShaderLocation(sh, names[i]), tex[i]);
    }
    DrawRectangle(0, 0, 640, 360, WHITE);
    EndShaderMode();
    EndTextureMode();

    Image snap = LoadImageFromTexture(rt.texture);
    ImageFlipVertical(&snap);
    ExportImage(snap, out);
    printf("wrote %s\n", out);

    UnloadImage(snap);
    UnloadRenderTexture(rt);
    UnloadShader(sh);
    CloseWindow();
    return 0;
}
