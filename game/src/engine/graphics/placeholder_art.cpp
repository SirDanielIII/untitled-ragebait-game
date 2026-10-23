#include "engine/graphics/placeholder_art.h"
#include "ui/theme.h"

namespace rage::art
{
void Student(const Resources& resources, Rectangle b)
{
    if (resources.DrawArt(ArtId::Student, b)) return;
    const auto rect = [&](float x, float y, float w, float h, Color c) {
        DrawRectangleRec({b.x + x*b.width, b.y + y*b.height, w*b.width, h*b.height}, c);
    };
    rect(0.2f, 0, 0.64f, 0.38f, {203, 165, 143, 255});
    rect(0.15f, 0, 0.74f, 0.1f, theme::background);
    rect(0.26f, 0.19f, 0.17f, 0.04f, theme::background);
    rect(0.6f, 0.19f, 0.17f, 0.04f, theme::background);
    rect(0.26f, 0.25f, 0.17f, 0.04f, {126, 101, 111, 255});
    rect(0.6f, 0.25f, 0.17f, 0.04f, {126, 101, 111, 255});
    rect(0.16f, 0.39f, 0.68f, 0.4f, {113, 126, 164, 255});
    rect(0, 0.43f, 0.18f, 0.3f, {161, 110, 102, 255});
    rect(0.21f, 0.79f, 0.2f, 0.21f, theme::paper);
    rect(0.61f, 0.79f, 0.2f, 0.21f, theme::paper);
}
void Entity(const Resources& resources, Rectangle b)
{
    if (resources.DrawArt(ArtId::Entity, b)) return;
    const float cx=b.x+b.width/2;
    DrawEllipse(static_cast<int>(cx), static_cast<int>(b.y+b.height*0.45f), b.width*0.42f, b.height*0.42f, {11, 10, 20, 255});
    DrawTriangle({cx-b.width*0.3f,b.y+b.height*0.3f}, {cx-b.width*0.13f,b.y+b.height*0.3f}, {cx-b.width*0.38f,b.y}, theme::background);
    DrawTriangle({cx+b.width*0.13f,b.y+b.height*0.3f}, {cx+b.width*0.3f,b.y+b.height*0.3f}, {cx+b.width*0.38f,b.y}, theme::background);
    DrawRectangleRec({cx-b.width*0.26f,b.y+b.height*0.4f,b.width*0.15f,b.height*0.035f}, theme::danger);
    DrawRectangleRec({cx+b.width*0.11f,b.y+b.height*0.4f,b.width*0.15f,b.height*0.035f}, theme::danger);
    DrawLineEx({cx-b.width*0.17f,b.y+b.height*0.6f}, {cx+b.width*0.17f,b.y+b.height*0.63f}, 3, theme::mint);
}
void Platform(const Resources& resources, Rectangle bounds, Color accent)
{
    if (resources.DrawArt(ArtId::Platform, bounds, ArtFit::Stretch)) return;
    DrawRectangleRec(bounds, {60, 59, 76, 255});
    DrawRectangleRec({bounds.x,bounds.y,bounds.width,4}, accent);
}
void LevelBackdrop(const Resources& resources, ArtId backdrop, Rectangle world, Color accent)
{
    if (resources.DrawArt(backdrop, world, ArtFit::Stretch)) return;
    DrawRectangleRec(world, {28, 28, 42, 255});
    // These are scenery only. Authored platform rectangles remain the collision source.
    if (backdrop == ArtId::Level0)
    {
        for (int x=80; x<world.width; x+=280)
        {
            DrawRectangle(x, 120, 145, 180, {44, 43, 65, 255});
            DrawRectangleLines(x, 120, 145, 180, theme::border);
            DrawLine(x+72,120,x+72,300,theme::border);
        }
        DrawRectangle(1260,410,110,240,theme::background);
        DrawRectangleLines(1260,410,110,240,accent);
    }
    else if (backdrop == ArtId::Level1)
    {
        for (int x=0; x<world.width; x+=220)
        {
            const int height=160+(x%3)*70;
            DrawRectangle(x,650-height,170,height,{38,39,57,255});
            DrawRectangle(x+30,670-height,30,40,{92,78,91,255});
        }
        DrawCircle(1400,140,65,{66,56,86,255});
    }
    else if (backdrop == ArtId::Level2)
    {
        for (int y=70; y<world.height; y+=180)
        {
            DrawRectangle(80,y,840,5,theme::border);
            DrawRectangleLines(410,y+35,180,90,theme::border);
        }
    }
    else
    {
        DrawRectangle(1200,300,280,350,{52,49,65,255});
        DrawRectangleLines(1200,300,280,350,accent);
        DrawRectangle(1300,530,70,120,theme::background);
        Entity(resources,{1020,420,140,180});
    }
}
void MenuBackdrop(const Resources& resources, ArtId screen)
{
    ClearBackground(theme::background);
    if (resources.DrawArt(screen, {0,0,1600,900}, ArtFit::Stretch)) return;
    // A quiet, off-kilter room. Exported menu backgrounds replace this whole layer.
    for (int y=0; y<900; y+=64) DrawLine(0,y,1600,y,{23,22,35,255});
    DrawCircleGradient({1350,160},320,{55,43,77,255},theme::background);
    if (screen != ArtId::MainMenu) return;
    DrawRectangle(710,110,810,560,{27,26,43,255});
    DrawRectangleLinesEx({710,110,810,560},2,theme::border);
    DrawRectangle(745,460,190,170,{46,39,57,255});
    DrawTriangle({728,460},{950,460},{838,371},{60,44,69,255});
    DrawRectangle(810,535,54,95,theme::background);
    DrawRectangleLines(810,535,54,95,theme::gold);
    DrawRectangle(870,492,34,42,theme::gold);
    // Missing stairs drift toward a classroom door.
    for (int i=0;i<6;++i)
        DrawRectangle(942+i*70,580-i*42,85,14,{107,91,137,255});
    DrawRectangle(1360,245,98,160,theme::background);
    DrawRectangleLines(1360,245,98,160,theme::mint);
    DrawCircle(1438,330,4,theme::gold);
    DrawRectangle(735,630,760,8,theme::border);
    // Hallucinated clouds, loose masonry and a paper airplane.
    DrawEllipse(1040,275,62,16,{80,65,103,255});
    DrawEllipse(1100,245,48,14,{93,76,116,255});
    DrawRectanglePro({1290,180,58,22},{0,0},-18,theme::border);
    DrawTriangle({1440,480},{1470,497},{1510,457},theme::paper);
    DrawLine(1470,497,1485,478,theme::mint);
    Entity(resources,{1150,115,170,230});
    Student(resources,{920,503,66,115});
}
}

