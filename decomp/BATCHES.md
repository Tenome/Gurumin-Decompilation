# Work batches (2026-11-18)

Short batches taken from `QUEUE.md` section 3 (medium functions) and `open.tsv`, grouped by system so that each
batch shares structures and callees. Every batch ends with: comments, names in symbols.tsv/obj_symbols.tsv,
`build_all.py --report`, `sync_symbols.py`, `gen_queue.py`, commit and push. Functions that only match with
unreadable source go to `open.tsv`.

| # | Batch | Functions |
|---|---|---|
| 1 | Effect pool spawns | 1000dd90, 1000dba0, 1000df10, 1000e130 |
| 2 | Script tokenizers | 10035d20, 10035e80, 10035fa0, 10035b10 |
| 3 | Script parser pieces | 10036560, 100357a0, 10037560 |
| 4 | Game panels and HUD | 100084b0, 10008a30, 10008c40, 10008df0, 100092c0 |
| 5 | Message/HUD drawers | 1000c020, 1000c390, 1000cc30, 1000d6b0 |
| 6 | Data loaders | 1000beb0 TestDat_Load, 1000b060, 1000b2c0, 1000b4e0, 1000b750 |
| 7 | Character helpers | 10024d00, 1001c090, 1001c210, 1001b880 |
| 8 | Character movement | 100242c0, 100244b0, 10018100, 10011dd0 |
| 9 | Enemy helpers | 100543a0, 100541f0, 10034cc0, 10045c30, 10045ed0 |
| 10 | Item/list tables | 10054770, 10054d50, 10054f30, 10055110 |
| 11 | Table sweeps | 10055410, 10055690, 100558d0, 10055aa0, 10055ca0 |
| 12 | Scene node | 101ac010 Node::Load, 101ab9d0, 101ad350, 1019c3a0 |
| 13 | Scene node 2 | 10191500 Node::Init, 1019e9a0 Node::Release, 1019e610, 1019c690 |
| 14 | Anim model | 10162420, 10162610 PathMover::SetPosition, 101612b0, 10166f20 |
| 15 | Model bank / textures | 10169c60, 1016a2e0, 1016b050, 10175a90, 10175c90 |
| 16 | Renderer | 1015cae0, 10142b20, 10142dd0, 1013e260, 1013e790 |
| 17 | Sound and save | 10001cb0 Bgm_CheckHeader, 1003aef0 Bgm_Play, 10011240 Save_ReadThumbnail, 10012220 |
| 18 | Math and camera | 1002f320, 1002f050, 1002ee70, 1002ebf0, 10021c80 |
| 19 | Wall retries | the newest open.tsv entries with the NOTES patterns (MoveCursor, Char_ResetState, char_effects) |
| 20 | Naming sweep | `M_` methods and `DAT_` globals in the files touched by batches 1-19 |

### Status of batches 1-20 (2026-11-18)

Matched: Debris_Spawn, Debris_SpawnFromCharacter, Debris_SpawnFrom (1); ListBank::ReadStatement, ReadArgument,
ReadWord, ReadTerm (2); ListBank::GetIndexPart (3); Game::DrawLabelledNumber, DrawNumber, DrawNumberAlt (4);
Hud_DrawNameBanners, Drops_Update (5); Game::AddValueEntry, AddFlagEntry, AddKind5Entry, AddRangeEntry (6);
Char_PlaceOnGround, Npc_TalkFacingSimple, Npc_TalkFacing (7); Map_FindRegionAt (8); Enemy_Destroy (9);
PathMover::SetPosition, PathMover::Teleport (14); WorldRenderer::SetModelState (14).
Written, recorded in open.tsv: CharParams::operator= (needs a caller), Game::OpenWindow, Char_ProbeAhead,
Char_ProbeDirection, Npc_StandOnGround, Names_Expand, Screenshot_ReadBackBuffer, Region_WaveVertex.
Deferred (moved into batches 21-70): 1000dba0, 100092c0, 1000c020, 1000d6b0, 10037560, 100543a0, 10034cc0,
101ad350, 101612b0 (with the 10161781 fragment), 10142b20, 1016b050, 101670e0 (WorldRenderer constructor).
Batches 10 and 11 are map event raisers of the loop-pointer wall family (BACKLOG item 23).

## Batches 21-70 (found 2026-11-18)

Every remaining medium (M) and large (L) function of QUEUE.md, grouped by address range (one system each) so that a
batch shares structures and callees. Walls already recorded in open.tsv are left out; batch 10/11-style map event
raisers wait for the loop-pointer fix (BACKLOG item 23).

| # | System | Functions |
|---|---|---|
| 21 | game panels, HUD, menus | 10001170, 10001cb0 Bgm_CheckHeader, 1000baf0, 1000beb0 TestDat_Load, 1000c720 (L), 1000cfa1 (L) |
| 22 | game panels, HUD, menus | 1000e340 (L), 1000eed0 (L), 1000f5a0, 1000f7f0 (L), 1000fb80 |
| 23 | characters and collision | 100106e0 Save_Write (L), 10010d10 Save_ReadSlotInfo (L), 10011240 Save_ReadThumbnail, 10011520 (L), 10012220, 10012510 |
| 24 | characters and collision | 10012780, 10012990 (L), 10012d90 (L), 100131b0 (L), 100144c0 Ability_GetDescription (L), 10014d30 (L) |
| 25 | characters and collision | 100154c0, 10015960 (L), 10016700, 100169c0 (L), 10016ec0 (L), 10017d30 |
| 26 | characters and collision | 100184e0 (L), 10018870 (L), 10018fb0, 10019310 (L), 10019670 (L), 10019d80 (L) |
| 27 | characters and collision | 1001a0d0, 1001a990 (L), 1001b0a0 (L), 1001bb20 (L), 1001d2d0 (L), 1001d920 |
| 28 | characters and collision | 1001dc20 (L), 1001e4b0 (L), 1001ed80 (L), 1001f270, 1001f4d0 (L) |
| 29 | character movement and scripts | 10020090 (L), 10020560 (L), 10021380 (L), 10021c80, 10021e70 (L), 100236c0, 10023900 (L) |
| 30 | character movement and scripts | 10023d30 (L), 10024d00, 10025ce0 (L), 10026730 (L), 10026d80 (L), 10027630 (L), 100279b0 (L) |
| 31 | character movement and scripts | 10027da0 (L), 100286d0 (L), 10028fa0 (L), 10029730 (L), 10029cf0 (L), 1002a110, 1002a490 |
| 32 | character movement and scripts | 1002a740 (L), 1002cc70 (L), 1002ebf0, 1002ee70, 1002f050, 1002f320 |
| 33 | script parser, sound, saves | 100301a0 (L), 100308e0 (L), 10034210 (L), 10035050 (L), 100360f0 (L), 100366c0 (L) |
| 34 | script parser, sound, saves | 10036ca0 Script_RegisterBuiltin (L), 10037090 (L), 10037850 Script_InitLibrary, 10037b40 (L), 10038070 (L), 1003aef0 Bgm_Play |
| 35 | script parser, sound, saves | 1003b2c0 Save_Load (L), 1003b6e0 Costume_LoadPlayerModel (L), 1003bc60 (L), 1003c370, 1003fc40 (L) |
| 36 | items and effects | 10042140 (L), 100425f0 (L), 10043e20, 10043fc0 (L), 10044560 (L), 10044b10 (L), 100451c0 (L) |
| 37 | items and effects | 10045830 (L), 10045c30, 10045ed0, 100460d0 (L), 100469d0 (L), 10046f10 (L), 10047440 (L) |
| 38 | items and effects | 10047a10 (L), 10048050 (L), 10048b70, 10048e90 (L), 10049240 (L), 10049650 (L), 10049b00 (L) |
| 39 | items and effects | 1004cf20 (L), 1004d4b0 (L), 1004ed60 (L), 1004f250, 1004f4f0 (L), 1004f8b7 (L), 1004fbe0 (L) |
| 40 | map events and tables | 10050510 (L), 100509e0 (L), 10050df0 (L), 10051190 (L), 10051570 (L), 10051e20 (L) |
| 41 | map events and tables | 100524d0 (L), 10055ee0 (L), 10056260, 100564d0 (L), 10057c40, 10057ee0 (L) |
| 42 | map events and tables | 10058500, 1005c680 (L), 1005dbf0, 1005ecb0 (L), 1005f750 (L) |
| 43 | enemy and NPC behaviours | 100627d0 (L), 10065450 (L), 1007a370 (L), 1007ad20 (L), 1007b870 (L), 1007d0a0 (L), 10089210 (L) |
| 44 | enemy and NPC behaviours | 10098be0 (L), 10099040 (L), 10099b30 (L), 1009a690 (L), 1009d100 (L), 1009d490 (L), 1009ddd0 |
| 45 | enemy and NPC behaviours | 1009dfd0, 100a3900 (L), 100a6a50 (L), 100a7150 (L), 100a77f0 (L), 100a7db0 (L), 100a8370 (L) |
| 46 | enemy and NPC behaviours | 100a8ac0 (L), 100b0250 (L), 100b0880 (L), 100b0f00 (L), 100b1520, 100b1800 (L), 100b1ff0 (L) |
| 47 | enemy and NPC behaviours | 100b23b0 (L), 100b2900 (L), 100b3430 (L), 100b3770 (L), 100b4300 (L), 100b4930 (L), 100b4f00 (L) |
| 48 | enemy and NPC behaviours | 100b5550 (L), 100b58a0, 100b5c30 (L), 100b5f80, 100b6290 (L), 100b7360 (L), 100b7b10 (L) |
| 49 | enemy and NPC behaviours | 100b8220, 100b9150 (L), 100b9930, 100b9bb0 (L), 100ba040 |
| 50 | scene and game flow | 100d2110 (L), 100dfcf0 (L), 100e1c40 Enemy_IsBusy (L), 100e21a0, 100ed510 (L), 100ed9b0 Script_CompileStatement (L) |
| 51 | scene and game flow | 10115cd0 (L), 10137b10 Screenshot_Capture (L), 1013ad90 Script_RegisterGameBuiltins (L), 1013e260, 1013f070 (L) |
| 52 | renderer and textures | 10142790 (L), 10142dd0, 10143180 (L), 10143bc0, 10143e00, 10144320 (L), 101446f0 (L) |
| 53 | renderer and textures | 10144bf0, 101453e0 Font_DrawGlyph, 101455d0 Font_Init (L), 10146db0 (L), 10147a20 (L), 10148590, 10148b00 Render_CreateDevice (L) |
| 54 | renderer and textures | 10149100, 1014ade0 (L), 1014b480, 1014b750, 1014bc60 (L), 1014c080 (L), 1014c430 (L) |
| 55 | renderer and textures | 1014c960 (L), 1014ce40 (L), 1014d4e0 (L), 1014d850 (L), 1014ddf0 (L), 1014e5d0 (L), 1014e940 |
| 56 | renderer and textures | 1014ec30, 1014f1f0 (L), 1014f590 (L), 1014f900 (L), 1014fdd0 (L), 101501f0 (L), 10150bb0 (L) |
| 57 | renderer and textures | 10151150 (L), 1015c450, 1015cae0, 1015f4e0 (L), 1015fea0 SystemSav_WriteRecord (L) |
| 58 | models and effects | 101603c0 SystemSav_ReadRecord (L), 10160a90 Mmv_Open (L), 10161f80 (L), 101629f0, 10163150 (L), 101643c0 (L) |
| 59 | models and effects | 10164c10 (L), 10165240 (L), 10165e00, 10166100 (L), 10166520, 10166950 Character_Load (L) |
| 60 | models and effects | 10167440 (L), 10167d90 (L), 101687e0 (L), 101693f0, 10169c60, 1016a2e0 |
| 61 | models and effects | 1016a480 (L), 1016a980, 1016ac90, 1016b400, 1016b6c0 (L), 1016be80 (L) |
| 62 | models and effects | 1016fb80 (L), 10172350 (L), 10172ac0, 10172cb0 (L), 10175a90, 10175c90 |
| 63 | models and effects | 1017b210 (L), 1017b9b0 (L), 1017c160 (L), 1017c5f0, 1017c9c0 |
| 64 | effects, scene nodes, maths | 101886d0 (L), 101890f0, 10189340, 10190e30 (L), 10191500 Node::Init, 10191fd0, 10192200 |
| 65 | effects, scene nodes, maths | 10192520 (L), 10192a00, 10192c90 (L), 10193140 (L), 10193d10 (L), 10194a10, 10194c60 |
| 66 | effects, scene nodes, maths | 10199380, 101995a0, 101999a0, 10199e20 (L), 1019a6b0 (L), 1019b350, 1019b630 |
| 67 | effects, scene nodes, maths | 1019b890 (L), 1019bdd0 (L), 1019c3a0, 1019c690, 1019d8e0 (L), 1019dca0 (L), 1019e120 (L) |
| 68 | effects, scene nodes, maths | 1019e610, 1019e9a0 Node::Release, 1019eeb0 (L), 101a0cf0 (L), 101a24e0 (L), 101a29e0 (L), 101a4a60 It3_ScanChunks (L) |
| 69 | effects, scene nodes, maths | 101a53b0 (L), 101a82d0 (L), 101a8780 (L), 101a8e60 Node::Collide (L), 101a9670 (L), 101aa080 (L), 101aaa20 (L) |
| 70 | effects, scene nodes, maths | 101ab3e0 (L), 101ab9d0, 101abe30 Model_LoadIt3, 101ac010 Node::Load, 101af4c0 KeyBank::Sample (L), 101af960 (L), 101afe00 (L) |

### Progress on batches 21-70 (2026-11-18)

Matched: Loading_Tick (21), Char_SetHandTool, Char_SetHandToolPlain, Char_WearHelmet (23-25), Node::Draw,
DrawList_DrawPass (66), Node::Load (70), Node::SetupSkins (70), NodeExtra::AnimatedMatrix (60), Render_ResetSwapTarget
(54), Camera_Ease (26), Npc_TalkWhenNear (48). Written and recorded in open.tsv: Bgm_CheckHeader, TestDat_Load,
Model_ApplyCommands, Helmet_FaceSettings, Pickup_Collect, Node::LoadIt3, Node::FindBone, Circle_Intersect,
Enemy_CanReturnHome. Naming: 18 Game window/HUD methods and six TextureObject draw methods.

## Batches 71-120 (generated 2026-11-18)

Functions not in batches 1-70, in address order, then the open walls to retry with the patterns added to
NOTES.md this session (shared pushes from per-branch calls, low-byte shifts, negated NaN comparisons, scoped
copies, inline float helpers). (L) = over 1500 bytes.

| # | Area | Functions |
|---|---|---|
| 71 | new functions | 10002440 (L), 10004b10 (L), 100094e0 (L) |
| 72 | new functions | 1000a0b0 (L), 1000cf80, 100135b0 (L) |
| 73 | new functions | 10017c55, 1001c670 (L), 10022830 (L) |
| 74 | new functions | 10024f80 (L), 1002ae70 (L), 1002d4e0 (L) |
| 75 | new functions | 1002f520 (L), 100311b0 (L), 10032240 (L) |
| 76 | new functions | 10038c10 (L), 1003c570 (L), 1003e710 (L) |
| 77 | new functions | 100404e0 (L), 10042a30 (L), 10049ec0 (L) |
| 78 | new functions | 1004d960 (L), 1004f860, 10053020 (L) |
| 79 | new functions | 10056b30 (L), 10058790 (L), 1005ce80 (L) |
| 80 | new functions | 1005def0 (L), 1005fc00 (L), 100615f0 (L) |
| 81 | new functions | 10062e70 (L), 10064630 (L), 10065bf0 (L) |
| 82 | new functions | 100675d0 (L), 10068f70 (L), 1006be40 (L) |
| 83 | new functions | 1006e150 (L), 1006f490 (L), 10070200 (L) |
| 84 | new functions | 10071ee0 (L), 10075350 (L), 10076790 (L) |
| 85 | new functions | 10078060 (L), 10079470 (L), 1007c050 (L) |
| 86 | new functions | 1007dab0 (L), 1007f1e0 (L), 10083eb0 (L) |
| 87 | new functions | 10084e80 (L), 10086380 (L), 10089a10 (L) |
| 88 | new functions | 1008c680 (L), 1008fea0 (L), 10092af0 (L) |
| 89 | new functions | 10093cc0 (L), 1009b120 (L), 1009c070 (L) |
| 90 | new functions | 1009e380 (L), 100a4440 (L), 100a5960 (L) |
| 91 | new functions | 100a9240 (L), 100aa820 (L), 100ac440 (L) |
| 92 | new functions | 100ad0f0 (L), 100ade30 (L), 100af1b0 (L) |
| 93 | new functions | 100b6740 (L), 100b84a0 (L), 100ba2a0 (L) |
| 94 | new functions | 100bcce0 (L), 100bdad0 (L), 100c39c0 (L) |
| 95 | new functions | 100cb0c0 (L), 100cc2a0 (L), 100d24a0 (L) |
| 96 | new functions | 100e0320 (L), 100e25f0 (L), 100e7280 (L) |
| 97 | new functions | 100edea0 (L), 100f5a80 (L), 100f9670 (L) |
| 98 | new functions | 100fb7b0 (L), 100fc4a0 (L), 100fda40 (L) |
| 99 | new functions | 100fedd0 (L), 101011b0 (L), 10104640 (L) |
| 100 | new functions | 1010c650 (L), 10112140 (L), 10114950 (L) |
| 101 | new functions | 10116550 (L), 1011a060 (L), 1011d810 (L) |
| 102 | new functions | 10138280 (L), 1013b700 (L), 1013fbb0 (L) |
| 103 | new functions | 10145ba0 (L), 101493f0 (L), 10151ba0 (L) |
| 104 | new functions | 1015d520 (L), 1016c3e0 (L), 10170580 (L) |
| 105 | new functions | 101713f0 (L), 10173550 (L), 10174ce0 (L) |
| 106 | new functions | 10175f20 (L), 1017a010 (L), 1017cc90 (L) |
| 107 | new functions | 10184440 (L), 10187520 (L), 10189530 (L) |
| 108 | new functions | 10194f40 (L), 1019c8d0 (L), 1019f340 (L) |
| 109 | new functions | 101a1640 (L), 101a3000 (L), 101a58a0 (L) |
| 110 | walls to retry with the 2026-11-18 patterns | 100014b0, 100016c0, 10001980, 10001cb0, 10004760, 100084b0, 10008910, 10009050, 100091d0, 1000b9f0 |
| 111 | walls to retry with the 2026-11-18 patterns | 1000beb0, 1000e870, 1000ed20, 1000f360, 10011dd0, 10012130, 10012220, 100143f0, 10014d30, 10015710 |
| 112 | walls to retry with the 2026-11-18 patterns | 100157b0, 10015910, 1001b670, 1001b7b0, 100211a0, 100242c0, 100244b0, 10024d00, 1002a340, 1002ee70 |
| 113 | walls to retry with the 2026-11-18 patterns | 10034c80, 10034cc0, 100357a0, 10035a30, 10037470, 1003ab40, 10043cc0, 10045ed0, 10054550, 10054970 |
| 114 | walls to retry with the 2026-11-18 patterns | 10054a60, 10054b70, 10054c60, 10054d50, 10054f30, 10055110, 100bd960, 100e2080, 100e2110, 100e2440 |
| 115 | walls to retry with the 2026-11-18 patterns | 100e24d0, 100e2560, 100fb6d0, 1013e4f0, 1013e790, 1013e9d0, 10143870, 10144040, 101441b0, 10144ee0 |
| 116 | walls to retry with the 2026-11-18 patterns | 10145380, 1014b390, 1015cac0, 1015fb20, 1015fbf0, 10160960, 10160f00, 10161120, 10161200, 101614c0 |
| 117 | walls to retry with the 2026-11-18 patterns | 101615d0, 10163e90, 10163f90, 101640b0, 101649c0, 10164af0, 10165020, 10166870, 10167390, 10169720 |
| 118 | walls to retry with the 2026-11-18 patterns | 10169e10, 1016a020, 1016a090, 1016a100, 1016b010, 1016b1e0, 1016c300, 10175c90, 101883b0, 10188580 |
| 119 | walls to retry with the 2026-11-18 patterns | 10191fd0, 10194a10, 10194c60, 101992f0, 10199380, 10199880, 10199be0, 1019b280, 1019c560, 101abe30 |
| 120 | walls to retry with the 2026-11-18 patterns | 101acc60, 101ad240, 101ad2c0, 101ad5c0, 101ad920, 101ae950, 101aed50, 101aeee0 |

### Progress (2026-11-18, second half)

Matched: Node::SortPacks, Node::ResetVertexColours, InputState::SetButtons, Camera_TurnPush, ScreenBatch::Add,
EffectManager::DrawSpriteA/B, Route_Follow, Char_CanSee, Sound_UpdateListener, ListBank::InitLibrary,
Script_RegisterGameBuiltins, Item_DrawCaption. Written and recorded in open.tsv: Node::UpdateWorld, Box_Overlap,
Game::RaiseFireEvents/RaiseLeverEvents/RaiseDoorEvents, Texture_CreateFromPixels, Char_UpdateShoves,
Party_BlocksMove, Npc_ApproachPlayer, Playback::Play, MotionList::FrameAt, Char_WalkRoute, Joystick_IsButtonDown,
Save_ReadThumbnail.

## Batches 121+ (2026-11-18, rolling)

Each finished batch adds two new ones, taken from the medium-function section of QUEUE.md.

| # | Area | Functions | Result |
|---|---|---|---|
| 121 | textures, user folder, balloons | 10169c60 TextureObject::Create, 1013e260 UserFolder_FindNt, 100543a0 Balloon_Blast | Balloon_Blast matched; the other two open (block placement) |
| 122 | medium functions | 100558d0 ListBank::FindCase, 1000c020 Text_Substitute, 101612b0 Rle_Decode | Rle_Decode matched; FindCase open (jump threading); Text_Substitute deferred to batch 125 (eight loops) |
| 123 | medium functions | 1000dba0 Fade_Draw, 10017d30 RenderTarget_Probe, 10055aa0 | Fade_Draw matched; RenderTarget_Probe open (2 bytes); 10055aa0 (label search like FindCase) moved to batch 124 |
| 124 | medium functions | 10055690 Game::IsQueuedEventReady, 10055410, 10055ca0, 10055aa0 | IsQueuedEventReady open (trigger-loop pointer base; game.h gained queuedTriggers and queuedEvents); 10055410 has the same loop, left; 10055ca0/10055aa0 (keyword scanners) left |
| 125 | medium functions | 1000c020 Text_Substitute, 10054770, 1009ddd0 Kind4a_Update | Kind4a_Update matched; Text_Substitute open (15 bytes); 10054770 is Game::RaiseDeadEvents (same loop wall as the other Raise*Events), left |
| 126 | medium functions | 10175a90 Texture_CreateCopy, 1019e610, 100092c0 | Texture_CreateCopy matched; 1019e610 (vertex de-duplication, three nested loops) and 100092c0 (window scroll arrows over Game's per-window field arrays) left |
| 127 | medium functions | 100236c0 Overlay_DrawModel, 1000f5a0, 1001f270 | Overlay_DrawModel matched; 1000f5a0 (camera matrices from the map's camera node) and 1001f270 (nearest movie screen and its fade) left: many unnamed globals |
| 128 | medium functions | 1017c9c0, 1015c450 DShowStream::Open, 101629f0 | DShowStream::Open open (cross-jumped failure messages); its Close (was SurfaceSet::Release) moved into dshow_stream.cpp; 1017c9c0 (big object reset) and 101629f0 (model constructor) need their class layouts first |
| 129 | medium functions | 100ba040, 1019b630, 1003aef0 Bgm_Play | Bgm_Play matched; 100ba040 (talk prompt near NPCs, many globals) and 1019b630 (recursive bone blend) left |
| 130 | medium functions | 100b8220 Npc_Greet, 1002ebf0 DamageZone_Update, 100b9930 | Npc_Greet matched; DamageZone_Update open (index register); 100b9930 (enemy retreat behaviour) left |
| 131 | medium functions | 10058500 Switch_UpdateEffects, 10192a00, 10045c30 | Switch_UpdateEffects matched; 10192a00 (cylinder overlap) and 10045c30 (NPC watching the player) left |
| 132 | medium functions | 1004f250, 10057c40 Switch_Hold, 1000d6b0 | Switch_Hold open (slot register swap); 1004f250 (tutorial texture loader) and 1000d6b0 (key prompt drawing) left |
| 133 | medium functions | 1001d920, 100e21a0, 1002a490 Block_Update | Block_Update open (model address register); 1001d920 (camera target lock) and 100e21a0 (per-kind enemy hit reactions) left |
| 134 | medium functions | 1002f050, 10016700, 1016b400 Camera_TurnInput | Camera_TurnInput open (blend weight copy); 1002f050 (route point decoding) and 10016700 (ground height sampling) left |
| 135 | medium functions | 10001170 Obj::Update, 1014b750, 1016a980 Camera_Reset | Obj::Update and Camera_Reset matched; 1014b750 (scaled texture quad) left |
| 136 | medium functions | 10148590, 100b1520, 10142dd0 Keyboard_IsButtonDown | Keyboard_IsButtonDown open (shared return blocks); 10148590 (buffer pool eviction) and 100b1520 (enemy sight and alarm) left. Also matched from a scan: 10021380 Map_SetEntryFlags |
| 137 | medium functions | 10149100, 10037560, 10144bf0 | |
| 138 | medium functions | 1014e940 TextureObject::DrawFrameAlt, 1014ec30, 1017c5f0 | |
| 139 | medium functions | 10166520, 1005dbf0, 10165e00 | |
| 140 | medium functions | 10048b70, 10192200, 1000baf0 | |
| 141 | medium functions | 100b5f80, 1009dfd0, 101453e0 Font_DrawGlyph | |
| 142 | medium functions | 10161781, 1016b050, 101670e0 | |
| 143 | medium functions | 10055690 retry, 1013f070 neighbours: 10148590, 1014b750 | |
| 144 | medium functions | 1019b350, 10037560, 1014e940 | |
| 145 | medium functions | 1017c5f0, 10166520, 1005dbf0 | |
| 146 | medium functions | 10165e00, 1000baf0, 10192200 | |
| 147 | medium functions | 10048b70, 100b5f80, 1009dfd0 | |
| 148 | walls to retry | 10169c60, 1013e260, 10017d30, 10057c40 | |
| 149 | medium functions | 1014ec30, 10144bf0, 10149100 | |
| 150 | medium functions | 101670e0, 10169720 retry, 1015fea0 | |
| 151 | medium functions | 1003e710, 10161f80, 10163150 | |
| 152 | large, mechanical | 1003e710 Game_Reset, 10161f80 (model record reset, needs the record layout) | Game_Reset matched (5424 bytes), and with it CharParams::operator= (0x100357a0) |
| 153 | medium functions | 1003b2c0 Save_Load, 101455d0 Font_Init, 10160a90 MmvPlayer::Open | Save_Load and MmvPlayer::Open matched; Font_Init (font object layout) left |
| 154 | low-branch functions | 10146db0, 100106e0 Save_Write, 10010d10 Save_ReadSlotInfo | Save_Write open (unrolled loop registers); 10146db0 and Save_ReadSlotInfo (two save formats) left |
| 155 | low-branch functions | 101603c0 SystemSav_ReadRecord, 1015fea0 SystemSav_WriteRecord, 10166950 Character_Load | |
| 156 | save and system files | 10010d10 Save_ReadSlotInfo, 101603c0 SystemSav_ReadRecord, 101602d0 File_Checksum | |
| 157 | medium functions | 1004f4f0, 10023900, 10055ee0 | |
| 158 | large, mechanical | 10161f80 (model record reset), 1015d520 Text_ToFullWidth | Text_ToFullWidth matched (7664 bytes); 10161f80 AnimModel::ResetState matched (fields named by offset) |
| 159 | medium functions | 10146db0, 10023900, 1004f4f0 | |
| 160 | large, table-like | 101ab3e0, 1013b700 Game_Startup (10376 bytes), 10138280 Script_GameBuiltin (10107 bytes) | |
| 161 | large, table-like | 1011a060 (14244 bytes), 100fedd0 (6739 bytes), 10112140 Menu_Main | |
| 162 | model class | 101629f0 AnimModel::AnimModel (matched), 1017c9c0 (scene object reset, member arrays), 10166950 Character_Load | |
| 163 | large, table-like | 10116550 (staff roll, 15107 bytes), 10023900, 1004f4f0 | 10023900 Map_ResetOnEnter open (49 bytes: New Year block placement), 1004f4f0 TutorialBook_NextPage open (draft) |
| 164 | small helpers | cosf/floorf/sinf/sqrtf copies, 1013f880 Random, Vec4::Length, VertexPack/IndexList::Init, RenderWindow::WindowProc + GameWindow ctor, 10161970 | all 11 matched (File_Checksum was already matched); the game's window procedure is GameWindow::WindowProc |
| 166 | save and system files | 101603c0 SystemSav_ReadRecord, 1015fea0 SystemSav_WriteRecord, 10161770 (fragment check) | |
| 167 | medium functions | 101670e0, 1016b050, 101453e0 | 101670e0 WorldRenderer::WorldRenderer matched, and with its layout 1017c9c0 WorldRenderer::Bind; 1016b050 Camera_SelectPath open (shrink-wrapping); 101453e0 left |
| 168 | world renderer | 10166950 WorldRenderer::LoadCharacter, 10167390, 1017cc90 | LoadCharacter matched (1488 bytes); 10167390/10167400 are WorldRenderer::StartFade/TickFade (moved from model_params.cpp; StartFade open, 16 bytes); 1017cc90 (draw lists, very large) left |
| 170 | world renderer | 1017cc90 (draw-list builder), 10189000 WorldRenderer::Seek retry, 101692c0 | |
| 169 | medium functions | 101453e0 Font_DrawGlyph, 10055aa0, 10054770 | 10054770 Game::MarkKilled open (two-pointer loop, as RaiseGameOverEvents); 10143e00 Render_FillRect open (6 bytes); 10055aa0 (label search) and Font_DrawGlyph left |
| 171 | table-like | 100144c0 Ability_GetDescription (generated from the asm skeleton), 1019b630, 10055410 | Ability_GetDescription matched (2146 bytes; int parameter); the others left |
| 172 | string-heavy | 1001d2d0 (digit counter), 10034920 (BS_003 variants), 100b9bb0 | 10034920 Bs003_SetVariant matched; 1001d2d0 DigitCounter_Update open (register swap); 100b9bb0 left |
| 173 | string-heavy | 1001e4b0, 1017c160, 1004ed60 | 1017c160 ModelBank::CreateTextures matched (first build); others left |
| 174 | string-heavy | 100135b0 Map_ShowEquipment, 1001e4b0, 1001a990 | Map_ShowEquipment open (3600 bytes, cross-jump direction); 1001e4b0 (records text) and 1001a990 (texture names by kind) left |
| 175 | init-like | 1003b6e0 Costume_LoadPlayerModel, 100b0f00, 100b0250 | none (branchy character logic); left |
| 176 | save and system files | 1015fea0 SystemSav_WriteRecord, 101603c0 SystemSav_ReadRecord, 10015910 retry | WriteRecord matched (FILETIME times[4]); ReadRecord open (20 bytes, return block order); 10015910 still threads its tests |
| 177 | save files | 10010d10 Save_ReadSlotInfo, 1001b0a0, 1004f250 | Save_ReadSlotInfo open (stack layout, as Save_ReadThumbnail); the others left |
| 178 | node functions | 1019e610 Node::WeldVertices, 1004ed60, 10055410 | WeldVertices matched (first build); others left |
| 179 | node functions | 1019eeb0, 1019b630, 1019b350 | 1019eeb0 Node::SetupSkinning open (one null test); Node::M_1019ece0 renamed Find (returns Node*); others left |
| 180 | node functions | 1019b350 Node::CopyMesh, 1019b630, 1019b890 | CopyMesh matched (Node bounds as a Vec4[3] union); others left |
| 181 | node functions | 1019b630 Node::BlendFrames, 1019b890, 101a53b0 | BlendFrames matched (Node::poses[16] at 0x2d4, frames at 0x290); others left |
| 182 | node functions | 1019b890 Node::CopyAnimation, 101a53b0, 1019dca0 | CopyAnimation matched (reading `frame` per branch fixed the registers); others left |
| 183 | node functions | 101a53b0 Node::Clone, 1019dca0, 1019d8e0 | Clone matched (first build); others left |
| 184 | node and effect functions | 1019d8e0 Node::ComputeBounds, 10190e30 EffectManager::Draw, 1019dca0 | ComputeBounds and Draw open (register/scheduling residue); 1019dca0 (ray-triangle) left |
| 185 | engine functions | 1019e120 Node::BuildCollisionSpheres, 1016ac90 Camera_CollectPaths, 10192200 | Camera_CollectPaths matched (first build); BuildCollisionSpheres open (14 bytes, address-mode operand order); 10192200 (collision geometry) left |
| 186 | integer functions | 101455d0 Font_Init, 10023d30 Map_Unload, 10142790 | Font_Init (FontRenderer layout filled in) and Map_Unload open (register choice); 10142790 left |
| 187 | integer functions | 10142790, 1014f1f0 TextureObject::Upload, 10148590 | 10148590 TexturePool_Get open; the other two left (DirectInput, pixel loops) |
| 188 | wall retries | 1015cac0, 10169e10, 10034c80, 10166870, 1016b010 | four matched (float setter, method call, two store orders); 1016b010 left (eax/edx) |
| 189 | wall retries | 1000ed20, 1016a090, 10012130, 1016b1e0 | 1000ed20 (counter read into a local) and 1016b1e0 (two ternaries) matched; 1016a090 and 10012130 still open |
| 190 | wall retries | 1016b010, 100e2080 family, 10015710, 101992f0 | all still open: control-flow, store-order, result-variable and flag variants leave the zero/1.0f register choice unchanged |
| 191 | small/medium functions | 1000cf80, 1004f860, 10017c55, 10055aa0 | 10055aa0 ListBank::FindLabel matched and FindCase 0x100558d0 with it (`*text` tests, no jump threading); 1004f860 TutorialBook_PrevPage open; 1000cf80/10017c55 are mis-sized fragments |
| 192 | medium functions | 10055410, 10055ca0, 10161781 | 10055410 Game::RunQueuedEvents (trigger pointer taken after the id test) and 10055ca0 ListBank::SkipStatement (declaration order) matched; 10161781 is a fragment |
| 193 | medium functions | 101453e0 Font_DrawGlyph, 100092c0, 1000f5a0 | 101453e0 and 100092c0 Game::DrawWindowMarks open (near-matches); the Game::Draw* colour argument is a float (callers push floats), retyped without losing matches |
| 195 | medium functions | 1000f5a0, 1000d6b0, 1001d920 | 1001d920 Carry_StackOnObject and 1000d6b0 KeyPrompt_DrawAll matched (float comparison spellings) |
| 197 | medium functions | 1000f5a0, 10045c30, 1004f250 | |
| 194 | medium functions | 1001f270, 100ba040, 10055 neighbours | 1001f270 Map_SelectMovie open (registers; it is a C++ function, extern "C" drops its EH frame) |
| 196 | wall retries | 10055690, Raise*Events | IsQueuedEventReady, RaiseGameOver/Kskon/Talk/OpenEvents matched (element pointer after the first test); Fire/Lever/Door left (inline budget) |
| 198 | medium functions | 100ba040, 100b9930, 10192a00 | 10192a00 Cylinder_Overlap matched (block-scoped temporaries) and wall 10191fd0 Box_Overlap with it (separate early returns) |
| 199 | medium functions | 100ba040, 100b9930, 1004f250 | 100b9930 Enemy_DragToPit open (one float copy) |
| 201 | medium functions | 100ba040, 1004f250, 10045c30 | 100ba040 Npc_PlayGreeting open; pick.py now keeps line endings |
| 202 | medium functions | 1004f250, 10045c30, 10037560 | 10045c30 Npc_WaitFacingAway matched (if/else calls, block-scoped turn, no pointer local) |
| 203 | medium functions | 1004f250, 10037560, 10016700 | 1004f250 TutorialBook_Close open (registers) |
| 204 | medium functions | 10037560, 10016700, 1014b750 | 10037560 ListBank::AssignValue open (frame layout) |
| 205 | medium functions | 10016700, 1014b750, 100b1520 | 10016700 Ground_HeightAt open (float copy pattern shared with two other drafts) |
| 206 | ListBank | 10037090, 100360f0 (1014b750 skipped: quad-draw register wall family) | 10037090 FindVariable open (search-loop registers; ReportError found to be inline) |
| 208 | large functions | 100360f0, 100b9bb0, 10055ee0 | 100b9bb0 Enemy_PlayVoice open (jump threading); 100360f0 and 10055ee0 left (goto-heavy) |
| 209 | wall retries | walls with a ternary or a pointer local (if/else calls, no pointer local) | 1019c560 Node::ApplyMaterialColours matched (no pointer local); 1000f360 down to an esi/edi swap |
| 210 | wall retries + medium | 1003ab40, 101ad350, 1002f050 | walls unchanged (register allocation); 1002f050 Route_FollowGates open (registers) |
| 211 | near-miss walls | 10194c60, 1001b670, 101ad240, 1002ee70 (fewest differing bytes) | all residues confirmed: EH state numbering, slot order by first use, dot-product schedule, fadd schedule; 100e21a0 left (enemy AI zero-register family) |
| 212 | large functions | 10172350, 1001b0a0, 100366c0 | 10172350 EffectManager::Start studied (needs the 0x1dc-byte effect record laid out: matrix 0x104, options 0x17c..0x1c0, handle 0x1d8); not written yet |
| 213 | effect record layout | EffectRecordA fields for 10172350, then the function | effect_start.cpp lays out the record, definition and options; EffectManager::Start open (one store cluster) |
| 214 | ListBank | 10036ca0 (1016fa40/10172780 were already done) | 10036ca0 RegisterBuiltin open (shrink-wrapping); builtin tables laid out in listbank_vars.cpp |
| 215 | large functions | 1001b0a0, 100366c0, 10021e70 | 1001b0a0 Char_SelectKind open (register roles) |
| 216 | large functions | 100366c0, 10021e70, 1001a990 | 1001a990 Char_AttachWeapon matched (switch on the model count, goto past for the default) |
| 217 | large functions | 100366c0, 10021e70, 1004ed60 | studied: 100366c0 is ListBank::DeclareFunction (RegisterBuiltin's twin with parameter names at 0x1230c), 10021e70 the save menu's status digits; Enemy_PlayVoice switch forms retried (worse) |
| 218 | large functions + near-miss | 1004ed60, 100b0f00, 10143180, 100091d0 | studied only: 1004ed60 TutorialBook_Open (same register family as the other book functions), 100b0f00 (EH, enemy state), 10143180 input any-key poll; DrawDebugText statement orders tried |
| 219 | naming + study | the five unnamed matched functions; 100b0250/100b0880/100b0f00 | named all five; 100b0250, 100b0880 and 100b0f00 are three near-identical companion updates (EH, ~1600 bytes each), left for a dedicated batch |
| 220 | companion updates | 100b0250, 100b0880, 100b0f00 | Companion1_Update and Companion2_Update matched; Companion0_Update open (20 bytes, idle-branch temporaries) |
| 221 | large functions | 1015f4e0, 10147a20, 1005ecb0, 100b1520 | Npc_WatchForPlayer matched (one shared `DAT_10210b30[index] = 0` at the end instead of one per return kept ebp free of a cached zero); the other three studied only |
| 207 | wall study | the fld/fstp float-copy pattern (100ba040, 100b9930, 10016700) | found for 10016700 (sum of two casts spills the first); the other two are a different copy, still open |
| 200 | wall retries | float-compare walls, MarkKilled | MarkKilled and Vec4::InTriangle still open (register roles, return-block placement) |
| 165 | medium functions | 10055ee0, 10146db0, 10010d10 Save_ReadSlotInfo | |
| 222 | NPC behaviours | 100b5f80, 100b5c30, 100b5550, 100b3430 | Npc_AlertWhenSeen, Npc_TurnWhenSeen (C++, EH for the effect matrix) and Npc_RiseWhenSeen matched; Npc_GreetWhenNear open (index/record offset registers swapped) |
| 223 | NPC behaviours | 100b1ff0, 100b6290 | Npc_FaceAndTalk and Npc_RunToMarker matched (Vec4 0x101af080 normalises in place) |
| 224 | NPC behaviours | 100b23b0 | Npc_ChildFollow matched (per-return clears here, so the zero lives in ebp as in the original) |
| 225 | NPC behaviours | 100b4f00 | Npc_PopUp matched (C++, EH; a clamp written into a second float keeps the self-copy; `max(min(x, 20), 0)` macros) |
| 226 | NPC behaviours | 100b4300 | Npc_RaiseAlarm drafted (33 bytes, zero register around the alarm); the `||` form of a call test made cl 13.00 inline atan2f |
| 227 | NPC behaviours | 100b4930 | Npc_RideAlarm drafted (25 bytes: index/loop counter registers swapped); `if (a > c || b > d) X else Y` reproduces the original block order |
| 228 | enemy behaviours | 1009dfd0 | Enemy_FollowAndBlast matched (`rand() % 100 <= 2`, the record matrix is now the shared Matrix4 view) |
| 229 | enemy behaviours | 1009d100, 10098be0 | Enemy_ChaseAndBlast and Enemy_ShoveAndBlast matched (the latter with a direct `atan2` call where cl 13.00 would not expand `Atan2`) |
| 230 | character behaviours | 10048b70 | Char_SwingDoor matched (C++, EH; root node matrix = saved matrix * RotationY 0x101b1bb0) |
| 231 | character behaviours | 10045830 | Npc_SleepUntilWoken matched in its own unit (npc_sleep.cpp): a new CharRecord member in char_move.cpp flipped Npc_PopUp |
| 232 | player | 1005dbf0 | Player_TakeDamage drafted (player_damage.cpp); its hurt-voice helper will not inline under cl 13.00 |
| 233 | enemy behaviours | 10048e90 | Enemy_DropIn matched (switch with two cases carrying the same statement gives dec/je/dec/jne) |
| 234 | enemy behaviours | 10049240, 10049b00 | Enemy_DropInWhenNear matched; 10049b00 (route position interpolation, 0x40 bytes per point) studied only |
| 235 | enemy behaviours | 10046f10 | Enemy_AppearNearby matched in its own unit (enemy_blink.cpp) |
| 236 | enemy behaviours | 10044560 | Enemy_Pendulum matched (own unit; one function-scope `frames` shared by two states gives the original slots; fields read directly instead of an `angle` local) |
| 237 | behaviour dispatch | 100dfcf0 | Char_RunBehaviour matched (npc_dispatch.cpp): the behaviour table 1..0x31 now names every NPC behaviour by number |
| 238 | NPC behaviours | 100451c0, 10044b10 | Npc_WalkRoute drafted (route_carrier.cpp, frame 8 bytes larger); 10044b10 (another route walker) studied only |
| 239 | NPC behaviours | 10047a10, 100b4930 | Npc_RideBehindMarker drafted (4 bytes); sqrtf/atan2f order; Npc_RideAlarm down to its register swap |
| 240 | NPC behaviours | 100469d0, 10043fc0 | Npc_Held matched (npc_frozen.cpp: if/else around the two Start calls, ModelState follow flag as a bitfield); 10043fc0 studied only |
| 241 | NPC behaviours | 10047440 | Npc_PushCart drafted (npc_wheel.cpp; a spilled pointer local in the original) |
| 242 | camera | 10019d80, 100b1800 | Camera_Between matched (camera_follow.cpp); 100b1800 (group chat NPC, three unrolled scans) studied only |
| 243 | camera | 100184e0 | Camera_ItemStand matched (camera_follow.cpp) |
| 244 | map view | 1000f5a0 | Map_BuildTopView matched (map_view.cpp) |
| 245 | map view | 1000f7f0 | Map_DrawTopView matched (map_view.cpp, a void function) |
| 246 | attached meshes | 1000baf0 | MeshSlot::Draw matched (mesh_slots.cpp; double literals for the camera offsets) |
| 247 | enemies | 100e21a0 | Enemy_DefaultStep drafted (enemy_react.cpp; zero register) |
| 248 | walls retried | 1002ee70, 10034cc0, 10024d00 | Char_UpdateShoves and Party_BlocksMove matched with math.h atan2f; Enemy_CanReturnHome differs earlier |
| 249 | enemy moves | 10027630 | Enemy_ChasePlayer drafted (enemy_chase.cpp) |
| 250 | collision probes | 10050df0 | Char_ProbeSideLedge drafted (ledge_probe.cpp) |
| 251 | enemy waves | 10049650 | Wave_Update drafted (enemy_spawn.cpp; stack slot order) |
| 252 | shaga | 100425f0 | Shaga_Create matched (shaga_spawn.cpp; if/else RotateZ calls) |
| 253 | shaga | 10042140 | Shaga_BreakOff matched (shaga_spawn.cpp; base-kind switch, case 0x10 placed after 0x31) |
| 254 | texture | 1014e940 | TextureObject::DrawFrameAlt matched (texture_bind.cpp; last state call repeated in both arms, x then y) |
| 255 | texture | 1014ec30, 10144bf0 | DrawFrameAltTinted and DrawFrameAltPlain matched (texture_bind.cpp; DrawFrameAlt with an RGB colour) |
| 256 | texture | 1014e5d0 | DrawFrameAltStretched matched (texture_bind.cpp; loop invariants as named locals just before the loop) |
| 257 | texture | 1014d4e0 | TextureObject::DrawFrame matched (texture_bind.cpp; balloon fan, first build) |
| 258 | texture | 1014d850 | DrawFillAlt draft recorded (texture_bind.cpp; array setup scheduling) |
| 259 | texture | 1014bc60 | TextureObject::DrawCell matched (texture_bind.cpp; per-vertex corner/UV order, chained colour/specular/z/rhw) |
| 260 | texture | 1014c430 | TextureObject::DrawTinted matched (texture_bind.cpp; caller parameter names corrected in game_panels.cpp) |
| 261 | texture | 1014b750 | TextureObject::DrawScreenCopy matched (texture_bind.cpp; first build) |
| 262 | texture | 1014c080 | DrawQuad one byte off (texture_bind.cpp; SIB operand order), recorded |
| 263 | texture | 10144320 | TextureObject::DrawThroughShader matched (texture_bind.cpp; descending chained coordinates, MIN(size, screen)) |
| 264 | texture | 10149100 | SwapTarget_DrawPreview matched (texture_bind.cpp; divisions inline in the chained coordinates) |
| 265 | frame | 1014ade0 | Frame_Pace matched (new frame_pacer.cpp; engine tick helper __forceinline, four copies) |
| 266 | input | 10142790 (+10142670, 10142690, 101426d0) | Input::Update two bytes off (new input_poll.cpp); its three enumeration callbacks matched; input_font.cpp device names corrected |
| 267 | texture | 1014f1f0 | TextureObject::Upload draft recorded (texture_bind.cpp; frame and pointer induction) |
| 268 | lens | 10166520 | ModelBank::DrawLensFlares matched (model_bank.cpp; table declaration order sizes, steps, lenses, alphas; separate early returns) |
| 269 | shadow | 10165e00 | ModelBank::DrawShadows matched (model_bank.cpp; separate early returns, repeated litModels[casters[i]] reads) |
| 270 | shadow | 10166100 | ModelBank::DrawShadowsInto matched (model_bank.cpp; first build) |
| 271 | phase0 | - | Vec4 merged into include/vec4.h in 110 files (Box2 too; scene_node, visibility, world.h by hand; npc_greet.cpp left, open.tsv); 789 matched unchanged |
| 272 | phase0 | - | Matrix4 merged into include/matrix4.h in 77 files plus world.h (Matrix/CharMatrix/NodeMatrix views too; Character position union became matrix._41/_42/_43; npc_greet.cpp left, open.tsv); 789 matched unchanged, 4 renamed symbols |
| 273 | phase0 | - | Vec3 into include/vec3.h (world.h + 6 files; Vec3_* helpers declared once in vec4.h; Matrix4 row vectors drop the camera-list casts); 789 matched unchanged |
| 274 | phase0 | - | Node into include/node.h (45 files + RootNode/HeadNode/PathNode/AnimNode/CollisionNode/PlacedNode/ShagaNode/ModelNode/VisNode; world.h uses Node); 789 matched unchanged, 7 methods renamed at the same address |
| 275 | phase0 | - | com_min.h, ddraw_min.h, d3d9_min.h texture/surface slots (dshow_stream, surface_copy, screenshot, texture_bind); 789 matched unchanged |
| 276 | phase0 | - | key_track.h, node_data.h, mesh.h (22 files lose NodeSaved/NodeExtra/NodeSkin/Material/MeshData/pack copies); 789 matched unchanged |
| 277 | phase0 | - | KeyBank into key_track.h; anim_model.h (AnimModel 0x249c, MotionChannels, Playback): 37 files drop AnimModel/CharRecord/SceneOwner/PathMover/... copies; 789 matched, renames at the same addresses |
| 278 | phase0 | - | dinput_min.h (input_poll, input_font, player_damage); 789 matched unchanged |
| 279 | phase0 | - | Character (DAT_10db6ed8) views are AnimModels at DAT_10db6ee8 (world.h + 35 files); npc_greet still deferred; 789 matched unchanged |
| 280 | phase0 | - | char_state.h (CharState + 13 views of DAT_1020b430), model_bank.h, effect_manager.h (EffectOptions), texture_object.h, game.h globals, listbank.h, Playback for model_state/mesh_cache; 789 matched unchanged |
| 281 | phase0 | - | MeshInstance = TextureObject (TextureHandle base); effect tables in effect_manager.h (EffectDef/EffectRecord/EffectSprite/EffectLight); CharSlot at DAT_10daaae8 in char_slot.h (CharParams, CharSlot@DAT_10daab84, ActorSlot, PlayerRecord views merged); CharState words[k] named; 789 matched unchanged |
| 282 | phase0 | - | CharSlot aliases (DAT_10daadfc, DAT_10daaf39, DAT_10daab20, ...) are DAT_10daaae8 fields; tools/globals_driver.py moves 1029 extern "C" globals from 172 files into char_tables.h and eight *_vars.h subsystem headers (156 held: type/size conflicts, file-local types); 789 matched unchanged |
| 283 | phase0 | - | step 4: tools/rule_check.py + build_all RULES line / matched_rules_* measures (182 of 789); type_move_driver.py (116 file-local types into headers), engine_memory.h, win_vars.h, merged views GameFile/ItemInfo/TextureManager/MeshSlot/Tracker/StartVectors/ScreenVertex/TextureRef/Channel/Plane, overlay model = Node, resolve_globals.py; matched under the rules 488 of 789; 789 matched unchanged |
| 284 | 4 | 0x1014f590 | FontRenderer::DrawString matched (GlyphWidth args are unsigned short); Player_KnockBack 0x10055ee0 drafted (open: stored probe flag); position-history globals in render_vars.h; 790 matched, 489 under the rules |
| 285 | 4 | 0x101af4c0 | KeyBank::Sample drafted in vec4.cpp (Spline moved there; open: linear flag folded, (float)offset hoisted); Spline is probably a thiscall member but declaring it in KeyBank breaks FUN_100b4f00 (NOTES); 790 matched, fuzzy 14.78% |
| 286 | 4 | 0x10012990 | Number_Draw / Number_DrawRightAligned / Number_DrawBack matched (new hud_digits.cpp + hud_digits.h, rule-clean; `y--; advance = 0;` before the width table fixes the schedule); KeyBank::Sample: blend factor inside the Spline calls removes the hoist (still open: flag fold); 793 matched, 492 under the rules |
| 287 | 4 | 0x10192200 | BoxCylinder_Overlap matched (box_overlap.cpp, rule-clean; the outside test written as an || of >=/<= gives the original's non-NaN jumps); CylinderSphere_Overlap 0x10192c90 drafted (open: y-sum operand order); 794 matched, 493 under the rules, fuzzy 15.06% |
| 288 | 4 | 0x10192520 | BoxSphere_Overlap drafted in box_overlap.cpp (4 bytes off: y-sum operand order, same wall as 0x10192c90); 794 matched, 493 under the rules, fuzzy 15.13% |
| 289 | 4 | 0x1017b210 | AnimModel::CloneModel and AnimModel::LoadFile (0x1017b9b0) matched (new anim_clone.cpp, rule-clean; a `Playback *playback` local picks ecx for the playback state); Enemy_SeekShaga 0x100d2110 drafted (shaga_seek.cpp + shaga.h; open: push placement); DAT_10ff19e8 is Matrix4 *; 796 matched, 764 under the rules, fuzzy 15.44% |
| 290 | 4 | 0x101a8780 | Node::SegmentHitsMesh matched (new node_mesh_hits.cpp, rule-clean; the triangle sphere test reads `spheres[t].w * spheres[t].w` in the condition, a radius local CSEs the sphere address and moves the induction registers); Node::Collide 0x101a8e60 drafted (open: region prologue tail merging); Node::m_0070 = triangleSpheres, Node::IntersectTriangle 0x1019dca0 declared; 797 matched, 765 under the rules, fuzzy 15.57% |
| 291 | 8 | rules | ScreenBatch merged into TriangleBatch (0x10172ac0 = TriangleBatch::AppendVertices, vertices are ScreenVertex *); the save/player-start globals DAT_10202464 (int[4]), DAT_108029b8 (Vec4), DAT_1042de28 (char[0x80]) and DAT_1020d860 (ground_height.h) declared once; names_od globals in name_paths.h; save_slot, screen_vertices and names_od now rule-clean; 797 matched, 770 under the rules, fuzzy 15.57% |
| 292 | 1 | 0x1019dca0 | Node::IntersectTriangle matched (new node_triangle.cpp, rule-clean: nested not-outside tests, fresh uv locals); the CDL region table DAT_11affdd8 was DAT_11afffe8 - 0x210 (removed); Collide prologue variants tried (open.tsv) |
| 293 | 8 | rules | Switch hold slots in switch_slots.h (SwitchObject * table; game_reset clears SWITCH_HOLD_SLOTS_CLEARED explicitly); MapObject views were Game (lever/door/RegisterEvent) and MapRegion (buffer0004); GameObject merged into Game (M_100082e0); message texts extern in message_text.h with MESSAGE_TEXT_CLEARED_BYTES; 778 under the rules |
| 294 | 7 | rules | Map record table DAT_10210fc0 as union MapRecord (map_records.h: points[16] / MapEntry / DamageZone / CameraPoint, the DAT_10210fc4 views folded in); 781 under the rules |
| 295 | 16 | rules | Node family merged: SceneNode/NodeBody/GameNodeBody/WorldNode are Node (Node::Node 0x1019ebc0, Node::~Node 0x10191480; boundsMin/boundsMax became box[2], the ctor's two-element loop); WorldRenderer's four views in world_renderer.h (ModelRecord = AnimModel); RenderWindow/GameWindow in render_window.h (ImeWindowHooks view removed); 797 of 798 under the rules |
| 296 | 2 | match | Node::RenderShadowTexture 0x101a24e0 and Node::DrawShadow 0x101a29e0 (new shadow_texture.cpp; render_state.h names FUN_1013f070 commands); ShadowScene and ModelData were Node (worldMatrix, bounds.w); Node::Draw / MeshData::Draw parameters typed (projector matrix, shadow centre floats, light direction, model); 800 matched, 799 under the rules |
| 297 | 1 | match | Node::SegmentHitsBox 0x101ab3e0 (new node_box_hit.cpp: twelve box triangles through IntersectTriangle); Node::QueueDraw 0x10199e20 drafted in node_queue.cpp (frame matches, register allocation open); fog globals typed (DAT_10f668f8 Vec4 fog colour, Node fogColour/fogNear/fogFar/fogDensity, queueCallback); 801 matched, 800 under the rules |
| 298 | 0 | draft | Node::BuildMesh 0x101a82d0 drafted (node_mesh_build.cpp; only VertexPack address operand order differs); PackHeaders/IndexLists_Alloc are Node::AllocPackHeaders/AllocIndexLists (thiscall); Vertex::m_0038 = sourceMaterial; 801 matched, 800 under the rules |
| 299 | 0 | draft | WorldRenderer::DrawPictures 0x10167440 drafted (new world_pictures.cpp, 21 bytes of x87 scheduling left in one branch): the 0x20 script pictures typed in world_renderer.h (alpha, UVs, angle, grey, positions, sizes, in-world, blend, kind, screen copy texture and size; PictureKind/PictureBlend); render_state.h: RENDER_DEPTH_WRITE/RENDER_DEPTH_TEST, BLEND_ADD/BLEND_DARKEN; BuildMesh first vertex fixed (only the packs operand order left); 801 matched, 800 under the rules, fuzzy 16.11% |
| 300 | 1 | match | Enemy_StrafeAndLunge 0x10099040 matched (new enemy_strafe.cpp + strafe_enemy.h, kinds 0x4c-0x4e); Enemy_StrafeAndShoot 0x1009a690 and Enemy_StrafeAndBurst 0x10099b30 drafted from the same shape (ebx/edi swap and tail-merge order left); CharState::shotSound, DAT_10467ba0 typed; 802 matched, 801 under the rules, fuzzy 16.54% |
| 301 | 0 | draft | Enemy_ChaseAndSlash 0x1009d490 (kind 0x4f) drafted in new enemy_slash.cpp + slash_enemy.h (CharState::stuckFrames at 0x08); frame and registers match, side-step tail merging left (open.tsv) |
| 302 | 1 | match | Enemy_HopAndStrafe 0x1009b120 (kind 0x50) matched in enemy_strafe.cpp: hop part typed (CharState hopRequests/restFrames/hopBack/hopFrame/hopHistory, LastPosition history2/history3, HopHistory, DAT_10254310 bytes); Enemy_FlyAndStrafe 0x1009c070 (kind 0x51) drafted (only the two height clamps differ, open.tsv); AnimModel::M_101627d0 is SetMoveStep(x, y, float z); 803 matched, 802 under the rules, fuzzy 16.91% |
| 303 | 2 | match | Enemy_CallBarrage 0x100a77f0 (behaviour 15) and Enemy_SpinFire 0x100a7db0 (behaviour 14) matched in new enemy_barrage.cpp + barrage_enemy.h (C++ linkage for their EH frames); fire-node-else-head fallback = two cross-jumped calls (NOTES); 805 matched, 804 under the rules, fuzzy 17.08% |
| 304 | 3 | match | Enemy_CallBursts 0x100a8370 (behaviours 16/17) matched in enemy_barrage.cpp; Enemy_StarVolley 0x100a7150 (13) and Enemy_AimedShots 0x100a6a50 (12) drafted (stack-slot swaps only, open.tsv); DAT_10868838[index][n - 1] for the ring points; 806 matched, 805 under the rules, fuzzy 17.37% |
| 305 | 4 | draft | Char_DrawLinks 0x1001bb20 drafted in new char_links.cpp + char_links.h (CharLink table, link timers); 16 bytes of x87 store/push scheduling (open.tsv); 806 matched, 805 under the rules, fuzzy 17.48% |
| 306 | 5 | draft | Enemy_IsBusy 0x100e1c40 drafted in new enemy_busy.cpp + enemy_queue.h (queue globals, CharState::fadeOut; callers now call Enemy_IsBusy), two scheduling spots left; EffectManager::DrawLight 0x10187520 drafted by a subagent in effect_light.cpp (EffectLight typed, ScreenVertex fields named once in screen_vertex.h); Node::M_10194730 takes an unsigned char; 806 matched, 805 under the rules, fuzzy 17.52% |
| 307 | 6 | draft | ListBank::ReadToken 0x100360f0 (listbank.cpp) and ModelBank::LightDirection 0x10164c10 / LightForRoot 0x1017c5f0 (model_bank.cpp, LIGHT_* constants) drafted; ReadToken: jump threading over the operator chains; light pair: one-element Vec4 arrays built through the vector constructor iterator (open.tsv); 806 matched, 805 under the rules, fuzzy 17.55% |
| 308 | 7 | match | Node::Root 0x101992f0 matched as tail recursion (the compiler turns it into the parent loop with edx; NOTES); 807 matched, 806 under the rules, fuzzy 17.55% |
| 309 | 8 | match | Char_RunScriptStep 0x10029cf0 matched in char_script.cpp (new char_script.h: step kinds, STEP_FRAMES; DAT_104c0374 in map_vars.h): separate decode variables per part, and the fade-in clamp written as ++wait > 30 ? 30 : wait; 808 matched, 807 under the rules, fuzzy 17.61% |
| 310 | 9 | match | Watcher_Update 0x10026730 matched in the new hover_watcher.cpp (char_ground.h declares Char_PlaceOnGround; CharState::groundTimer): the inline float Atan2 gives the float-temporary argument rounding, and the eased turn divides into its own variable first; WakeNearby slot orders and the retire x * 1.0f retried without luck; 809 matched, 808 under the rules, fuzzy 17.67% |
| 311 | 10 | match | Sonnet subagents: Map_RoutePosition 0x10049b00 (map_route.cpp) and Enemy_BackAwayFromPlayer 0x100279b0 (new enemy_flee.cpp/enemy_flee.h, CharState::fleeSide) matched; MapCursor_Step 0x10051190 drafted (map_cursor.cpp, register allocation; open.tsv); 811 matched, 810 under the rules, fuzzy 17.79% |
| 312 | 11 | match | Sonnet subagents: Debris_Draw 0x1000e340 (debris.cpp, new debris.h; StartProximityPass third parameter is a Matrix4 *), Char_ProbeSideStep 0x100509e0 and its sibling Char_ProbeSideLedge 0x10050df0 (ledge_probe.cpp: offset in side.x, early-exit layout) matched; Playback::Advance 0x1016a480 drafted (playback_advance.cpp, MotionSource in anim_model.h); 814 matched, 813 under the rules, fuzzy 17.95% |
| 313 | 12 | match | Sonnet subagents: Message_DrawPanel 0x1000eed0, Characters_Draw 0x1000c720 (if/else call pairs share one call; branchless ternary shifts registers) and FontRenderer::DrawTextClipped 0x1014fdd0 matched; DrawStringToPixels 0x1014f900, KeyBank::Sample(float) 0x101af960, KeyTrack::Sample 0x101afe00 drafted; 817 matched, 816 under the rules, fuzzy 18.19% |
| 314 | 13 | match | Sonnet subagents: Camera_LiftOverGround 0x1016be80 matched (camera_lift.cpp); Spline made a thiscall member of the empty KeyInterpolator base (sampler call blocks now match; samplers still drafts); Player_SelectActionMotion 0x10011520 drafted (action_select.cpp); 818 matched, 817 under the rules, fuzzy 18.28% |
| 315 | 14 | match | Sonnet subagents: ListBank::DeclareFromText 0x100ed510 (types 0 and 1 in separate branches) and Lava_Update 0x1005f750 (lava_hazard.cpp, EffectHold in effect_holds.h, RotationX/Z in matrix4.h) matched; Regions_Update 0x1004d4b0 drafted (region_view.cpp); 820 matched, 819 under the rules, fuzzy 18.44% |
| 316 | 15 | draft | Sonnet subagents: EffectModel::DrawKeyed 0x10037b40 (mesh_slots.cpp), DrawItemPreview 0x10038070 (item_preview.cpp), Costume_LoadPlayerModel 0x1003b6e0 (costume_load.cpp) and Char_SpawnEnemy 0x10050510 (char_spawn.cpp) drafted; StartProximityPass frame is an int, EffectModel::Free 0x1000be10 (renames only); 820 matched, 819 under the rules, fuzzy 18.66% |
| 317 | 16 | match | Sonnet subagents: Ground_BuildHeights 0x100169c0 and Collide_RayThroughRegions 0x10020090 (min a > b ? b : a, max a < b ? b : a under /Op) matched; ListBank::CompileStatement 0x100ed9b0 drafted (statementParts union in listbank.h); 822 matched, 821 under the rules, fuzzy 18.87% |
| 318 | 17 | match | Sonnet subagents: TextureObject::DrawColoured 0x1014c960 matched (texture_bind.cpp); Map_DrawMovie 0x1001ed80 and TutorialBook_Open 0x1004ed60 drafted (register allocation); 823 matched, 822 under the rules, fuzzy 19.01% |
| 319 | 18 | match | Sonnet subagents: TextureObject::DrawBlurred 0x101446f0 matched; Char_InitSlot 0x1001dc20 and ClearMarkers_Create 0x1004cf20 drafted; an AnimModel array split that lost the constructor was reverted before committing; 824 matched, 823 under the rules, fuzzy 19.16% |
| 320 | 19 | match | Sonnet subagents: Npc_RideMarker 0x10043fc0 matched (if (d >= 0) switch for the 30/15/0 pick; overlap tests written self - 75 < m + 75 / self + 75 > m - 75); Route_StepPatrol 0x10029730 and MeshData::M_10150bb0 drafted; 825 matched, 824 under the rules, fuzzy 19.31% |
| 321 | 20 | match | Sonnet subagents: ListBank::DeclareFunction 0x100366c0 matched (while loop with the failed check inside; source/params/word scoped with the MessageBox check); Enemy_KeepDistanceFromPlayer 0x10027da0 and Move_SweepToWorld 0x10034210 drafted; 826 matched, 825 under the rules, fuzzy 19.54% |
| 322 | 21 | match | Sonnet subagents: UpdateDrillLevel 0x10057ee0 matched (drill_level.cpp; aliases of the player model replaced by DAT_10db6ee8[0].root/.matrix); AnimModel::LiftProbes 0x101643c0 drafted (model_lift.cpp, Node::model typed); 827 matched, 826 under the rules, fuzzy 19.71% |
| 323 | 22 | draft | Sonnet subagents: TextInput_Update 0x1015f4e0, Debris_Update 0x100564d0, Flames_Update 0x10051e20 and Node::ShadeWaveCopy 0x10193d10 drafted (register allocation / block layout); 827 matched, 826 under the rules, fuzzy 19.83% |
| 324 | 23 | match | Sonnet subagents: Enemy_KeepDistanceFromPlayer 0x10027da0 matched (call per branch); Route_StepPatrol 0x10029730 16 bytes off; Enemy_WanderNearHome 0x10026d80 drafted; 828 matched, 827 under the rules, fuzzy 19.91% |
| 325 | 24 | match | Sonnet subagents: Enemy_Defeat 0x100627d0 matched; TextureManager::CreateDevice 0x10148b00 (/Od, src_od), TextureObject::DrawFill 0x1014ce40 and Input::ReadControls 0x10143180 drafted; 829 matched, 828 under the rules, fuzzy 20.14% |
| 326 | 25 | match | Sonnet subagents: TutorialBook_Update 0x10065450 matched; FontRenderer::DrawText 0x101501f0, Camera_ShiftOrbit 0x10019670 and Npc_PatrolRoute 0x10044b10 drafted; 830 matched, 829 under the rules, fuzzy 20.39% |
| 327 | 26 | match | Sonnet subagents: SaveSummary_Draw 0x10021e70 matched; Npc_Lookout, Ground_Dig, Camera_TrackPathFrame, Char_SpiritUpdate, Spirit_StepRoute drafted; lookout globals moved out of char_vars.h (it cost UpdateDrillLevel its match); 831 matched, 830 under the rules, fuzzy 20.77% |
| 328 | 27 | match | Sonnet subagents: Camera_PullToItemStand 0x10018870 matched; MapMinigame_DrawHud, Bgm_Update, TriangleBatch::AppendQuad drafted; UpdateDrillLevel kept by moving ZeroMatrix out of matrix4.h; 832 matched, 831 under the rules, fuzzy 21.00% |
| 329 | 28 | match | Sonnet subagents: Camera_ChaseTarget, Enemy_FetchDroppedWeapon, EffectManager::LoadDefinitions, Render_Command matched; Game_FrameStep, Npc_GestureWithNeighbour drafted; UpdateDrillLevel made robust (level declared first); 836 matched, 835 under the rules, fuzzy 21.58% |
| 330 | 29 | match | Sonnet subagents: Sound_UpdateDopplerLoops 0x1004fbe0 matched (wrong-variable statement found by the stack-slot study); twelve large drafts; 837 matched, 836 under the rules, fuzzy 22.66% |
| 331 | 30 | match | Sonnet subagents: MeshData::DrawDebugBox 0x10146db0 matched; ItemDictionary_Update, Node::CastSegment2, Node::SphereHitsMesh, Node::ScanChunks, StatusPanel text, Gimmick use, Npc_ShockByLink drafted; 838 matched, 837 under the rules, fuzzy 23.31% |
| 332 | 31 | match | Sonnet subagents: Char_SwingPlatform, WorldRenderer::DrawCaption and DrawScreenOverlays matched; seven drafts; 840 matched, 839 under the rules, fuzzy 24.18% |
| 333 | 32 | match | Sonnet subagents: AnimModel::Draw, Camera_Refresh matched; CollideModel as a Node member fixed IsClear/IsClearAny/IsClearFlagged; seven drafts; 845 matched, 844 under the rules, fuzzy 25.20% |
| 334 | 33 | match | Sonnet subagents: Enemy_PulseAndCharge, Char_UpdateAttachSlots, Enemy_StalkAndBreathe, Enemy_StalkAndLunge, ModelBank_CompareDistance matched; 13 drafts; 849 matched, 848 under the rules, fuzzy 27.18% |
| 335 | 34 | match | Sonnet subagents on QUEUE section 5: Menu_SaveLoad matched (inlined MessageWindow_SetTitle explains its alignment pads); 11 drafts; 850 matched, 849 under the rules, fuzzy 28.34% |
| 336 | 35 | match | Sonnet subagents: Npc_HurlAtPlayer matched by a near-miss retry; Menu_DrawScene to 12 bytes; 4 drafts; 851 matched, 850 under the rules, fuzzy 29.05% |
| 337 | 36 | match | Sonnet subagents: Shaga_Update matched; Npc_HoverCarrier matched by a near-miss retry; 6 drafts, 2 retries closer; 853 matched, 852 under the rules, fuzzy 30.44% |
| 338 | 37 | match | Sonnet subagents: Enemy_CastAndTeleport, TitleScreen_Update, Partner_SpecialSkill matched; 7 drafts; 856 matched, 855 under the rules, fuzzy 32.34% |
| 339 | 38 | match | Sonnet subagents: 3 drafts (Enemy_StalkAndStrike, Map_SetupByNumber, Char_LaserZone); 856 matched, 855 under the rules, fuzzy 33.02% |
| 340 | 38 | match | Sonnet subagents: 9 drafts of very large functions, no new match; 856 matched, 855 under the rules, fuzzy 34.18% |
| 341 | 38 | match | Sonnet subagents: 4 near-miss retries, no match (permuter candidates); 856 matched, fuzzy 34.18% |
| 342 | 38 | match | Sonnet subagents: Scene_DrawFrame matched; 4 drafts; union lesson; 857 matched, 856 under the rules, fuzzy 35.61% |
| 343 | 38 | match | Sonnet subagents: 8 drafts of very large functions incl. the kind dispatcher and LoadMapData; 857 matched, fuzzy 36.89% |
| 344 | 38 | match | Sonnet subagents: 3 drafts (Script_GameBuiltin, Menu_Main, Enemy_SpitAndCharge); matrix4.h tie fixed; 857 matched, fuzzy 37.62% |
| 345 | 38 | match | Sonnet subagents: 3 drafts (Game_Main, Node::ParseChunks, OnResetDevice ~7 diffs); 857 matched, fuzzy 38.42% |
| 346 | 38 | match | Sonnet subagents: 5 drafts (CompileCall, Enemy_FlockDiver, Enemy_LeapAndClimb, Npc_RoamZones, EffectManager::Step); 857 matched, fuzzy 39.24% |
| 347 | 38 | match | Sonnet subagents: 3 drafts (Script_ValueCommand, Enemy_WispCaster, Camera_Update); 857 matched, fuzzy 39.97% |
| 348 | 38 | match | Sonnet subagents: 3 drafts (Field_Update, Hud_DrawFrame, Scene_Frame); 857 matched, fuzzy 40.85% |
| 349 | 38 | match | Sonnet subagents: NameEntry_Update matched; 2 drafts (Map_SetUpTrigger, AnimModel::MoveWithCollisions); 858 matched, fuzzy 42.19% |
| 350 | 38 | match | Sonnet subagents: callback type fix; 2 drafts (Node::UpdateWorldWithIk, Map18_UpdateFootballGame); 858 matched, fuzzy 42.78% |
| 351 | 38 | match | Sonnet subagents: 2 drafts (Bs003_Update, Script_SceneCommand); 858 matched, fuzzy 43.43% |
| 352 | 38 | match | Sonnet subagents: 3 drafts (Bs001_Update, Bs005_Update, Enemy_UpdateFighter); 858 matched, fuzzy 44.52% |
| 353 | 38 | match | Sonnet subagents: 3 drafts (Bs004_Update, NeoGalaxy_Update, WorldRenderer::RunScriptFrames); 858 matched, fuzzy 45.31% |
| 354 | 38 | match | Sonnet subagents: 3 drafts (EffectManager::DrawRecord, Prince_Update, Bs002_Update); 858 matched, fuzzy 46.81% |
| 355 | 38 | match | Sonnet subagents: MeshData::DrawLit draft; Bs006_Update drafted in 4 stages; 858 matched, fuzzy 48.03% |
| 356 | 38 | match | Sonnet subagent: Message_DrawLabels and Image_HalveRgb drafts; 4 fragments folded; 858 matched, fuzzy 48.03% |
| 357 | 38 | match | Sonnet subagents: CompileCall and RunScriptFrames structural passes, DrawLit frame study; 858 matched |
| 358 | 38 | match | in-game frame 0x1011d810 skeleton; 858 matched |
| 359 | 38 | match | in-game frame 0x1011d810 stage 1a; 858 matched |
| 360 | 38 | match | in-game frame stage 1b; 858 matched |
| 361 | 38 | match | in-game frame stage 2; 858 matched |
| 362 | 38 | match | in-game frame stage 3; 858 matched |
| 363 | 38 | match | in-game frame stage 4a; 858 matched |
| 364 | 38 | match | in-game frame stage 4b; 858 matched |
| 365 | 38 | match | in-game frame stage 4c (ItemText pad fix); 858 matched |
| 366 | 38 | match | in-game frame stage 5; 858 matched |
| 367 | 38 | match | in-game frame stage 6 first part; 858 matched |
| 368 | 38 | match | in-game frame stages 6c-6d; 858 matched |
| 369 | 38 | match | in-game frame stage 7; 858 matched |
| 370 | 38 | match | in-game frame stages 8-9; 858 matched |
| 371 | 38 | match | in-game frame stage 10; 858 matched |
| 372 | 38 | match | in-game frame stages 11-12: first complete draft; 858 matched |
| 373 | 38 | match | in-game frame first compare pass (C++ linkage, EH prologue); 858 matched |
| 374 | 38 | match | Game_LoadData C++ linkage; 858 matched |
| 375 | 38 | match | direct table indexing in boss/character updates; in-game frame layout; 858 matched |
| 376 | 38 | match | in-game frame call sites; Bs006 register hoist; 858 matched |
| 377 | 38 | match | Prince_Update one body; in-game frame burst local; 858 matched |
| 378 | 38 | match | Prince_Update locals split, arena test, charge states; in-game frame leap-start block and scene order; 858 matched |
| 379 | 38 | match | wall retries (16 matched); Prince_Update leap/effect poses, frame parked; 874 matched |
| 380 | 38 | match | wall retries (9 matched); in-game frame ladder order; 883 matched |
| 381 | 38 | match | wall retries 700-1600 bytes (4 matched); 887 matched |
| 382 | 38 | notes | zero-register rule, Vec4 operator= finding; no new matches; 887 matched |
| 383 | 38 | notes | Vec4 operator= rename tried and reverted (Node::Clone); ParamVector is a real second type; 887 matched |
| 384 | 38 | match | near misses (2 matched); large walls none; 889 matched |
| 385 | 38 | notes | permutation of near misses and large walls: none matched; 889 matched |
| 386 | 38 | match | cpp_permuter on near misses (3 matched); 892 matched |
| 387 | 38 | match | cpp_permuter over 99 small walls (14 matched, all order/tail rewrites); 906 matched |
| 388 | 38 | match | permuter frontier + large walls (1 matched, 6 improved); 907 matched |
| 389 | 38 | match | collide_query tails (2 matched), 2 improved; 909 matched |
