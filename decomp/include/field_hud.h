// The HUD frame of the field (FieldHud_Draw, src/field_hud.cpp), installed as the scene callback by Field_Update.
#ifndef GURUMIN_FIELD_HUD_H
#define GURUMIN_FIELD_HUD_H

// 0x100bb5e0 (C++ linkage: the original has an EH frame). Draws the field HUD frame. Returns 0.
int FieldHud_Draw(void);

// 0x1001ed80 (Map_DrawMovie, map_movie_draw.cpp): the caller pushes the map number although the draft there reads none.
extern "C" int FUN_1001ed80(int mapNumber);

#endif
