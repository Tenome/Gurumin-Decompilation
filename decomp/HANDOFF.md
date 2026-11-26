# Session handoff

Rewrite this file at the end of every session (keep it short: state, not history). The next session starts by
reading it, then `AGENTS.md`.

## Standing instructions (keep this section when rewriting the file)
- **Autonomous run (user, 2026-11-22):** work without waiting for approval; do not stop to ask questions a sensible
  default answers. When this session's task is done (or it can go no further), rewrite this file, commit and push,
  and end the turn; the orchestrating session starts the next one with the continuation prompt below.
- **Human-readable above all:** no magic numbers (named constants, enums or a comment saying what each value is),
  comments on every function (what it does, who calls it), on struct fields and on non-obvious logic, informative
  function and variable names (rename `FUN_`/`M_`/`DAT_` and `m_XXXX` when the evidence supports a name), early
  returns and plain loops. Comments and names never change compiled bytes; if a readable spelling loses a match,
  keep the readable one only when it is a comment/name change, otherwise record the function in `open.tsv`.
- Hard rules from `AGENTS.md` (typed members, one declaration per type, no fakematches, never commit a lost match).

## Progress (2026-11-23, `progress/jp/report.json`)
| Measure | Value |
|---|---|
| Fully linked | 0.00% of code (151 of 264 units complete) |
| Matched | 21.40% of code, 910 of 1212 functions |
| Matched under the rules | 21.37% of code, 909 of the 910 matched functions |
| Fuzzy match | 50.68% |

"Matched under the rules": a matched function counts only when its file declares no struct/class/union and no
`extern` global of its own (`tools/rule_check.py`; build_all prints a `RULES` line). Only npc_greet.cpp (1 matched
function) and node_world.cpp (0) still break rule 2.

## State
- **Workflow (parent session, 2026-11-23):** per-function matching goes to Sonnet subagents (2-3 in parallel on
  disjoint files) with a self-contained brief (tools, idioms, hard rules, "report: matched or bytes off, do not
  commit"); the Opus session picks functions, verifies with build_all + check_matches, commits, pushes, updates this.
- Batches 310-311 matched Watcher_Update 0x10026730 (hover_watcher.cpp), Map_RoutePosition 0x10049b00
  (map_route.cpp: points at 2i/2i+1, 1-based marks[], three locals sharing one slot) and Enemy_BackAwayFromPlayer
  0x100279b0 (enemy_flee.cpp + enemy_flee.h). MapCursor_Step 0x10051190 is a draft (map_cursor.cpp, open.tsv).
- Batch 312: Debris_Draw 0x1000e340 (debris.cpp + debris.h; Node::StartProximityPass takes Matrix4 *reference),
  Char_ProbeSideStep 0x100509e0 and Char_ProbeSideLedge 0x10050df0 (ledge_probe.cpp) matched; Playback::Advance
  0x1016a480 drafted (playback_advance.cpp, shared tails; open.tsv).
- Batch 313: Message_DrawPanel 0x1000eed0 (message_panel.cpp), Characters_Draw 0x1000c720 (character_draw.cpp) and
  FontRenderer::DrawTextClipped 0x1014fdd0 (font_metrics.cpp) matched; DrawStringToPixels 0x1014f900 (2 bytes),
  KeyBank::Sample(float) 0x101af960 and KeyTrack::Sample 0x101afe00 drafted. Lead: the three KeyBank/KeyTrack
  samplers may need Spline as a thiscall member (vec4.cpp). ime_draw.cpp should call DrawTextClipped as a method.
- Batch 314: Camera_LiftOverGround 0x1016be80 (camera_lift.cpp; DAT_1101dde4 is a float) matched; Spline is now
  KeyInterpolator::Spline (empty base of KeyBank/KeyTrack), the sampler call blocks match, other residues remain;
  Player_SelectActionMotion 0x10011520 drafted (action_select.cpp, count kept in a dead stack slot).
- Batch 315: ListBank::DeclareFromText 0x100ed510 (listbank.cpp) and Lava_Update 0x1005f750 (lava_hazard.cpp, new
  effect_holds.h) matched; Regions_Update 0x1004d4b0 drafted (region_view.cpp, zero register).
- Batch 316 (drafts only): EffectModel::DrawKeyed 0x10037b40, DrawItemPreview 0x10038070, Costume_LoadPlayerModel
  0x1003b6e0, Char_SpawnEnemy 0x10050510 (5-16 bytes each, open.tsv). StartProximityPass takes an int frame;
  MeshOwner::M_1000be10 is EffectModel::Free.
- Batch 317: Ground_BuildHeights 0x100169c0 (ground_height.cpp; DAT_1049f200/DAT_1042e3f0 are float[0x60][16][16])
  and Collide_RayThroughRegions 0x10020090 (collide_regions.cpp) matched; ListBank::CompileStatement 0x100ed9b0
  drafted (tail layout, open.tsv).
- Batch 318: TextureObject::DrawColoured 0x1014c960 (texture_bind.cpp) matched; Map_DrawMovie 0x1001ed80
  (map_movie_draw.cpp) and TutorialBook_Open 0x1004ed60 (tutorial_open.cpp) drafted (register allocation).
- Batch 319: TextureObject::DrawBlurred 0x101446f0 matched; Char_InitSlot 0x1001dc20 (char_init.cpp) and
  ClearMarkers_Create 0x1004cf20 (clear_markers.cpp; DAT_1021a1e0/DAT_10254ac0 are Node *[]) drafted. To clean up:
  DAT_10f1fd08 (render_vars.h) lies inside DAT_10f1fd04[8]; AnimModel 0x16c is an int inside Vec4 m_00b4[12]
  (splitting the array loses the AnimModel constructor).
- Batch 320: Npc_RideMarker 0x10043fc0 (marker_rider.cpp, behaviour 0x10) matched; Route_StepPatrol 0x10029730
  (route_step.cpp) and MeshData::M_10150bb0 (mesh_build.cpp, shrink-wrapped prologue) drafted.
- Batch 321: ListBank::DeclareFunction 0x100366c0 matched (ScriptFunction union in listbank.h); Enemy_KeepDistanceFromPlayer
  0x10027da0 (enemy_flee.cpp) and Move_SweepToWorld 0x10034210 (move_sweep.cpp, scalar stack slots) drafted.
- Batch 322: UpdateDrillLevel 0x10057ee0 (drill_level.cpp) matched; AnimModel::LiftProbes 0x101643c0 drafted
  (model_lift.cpp; Node::model at 0xb8, M_1019b230 takes an AnimModel *). Agents must not declare aliases of
  DAT_10db6ee8 fields (0x10db9210 = DAT_10db6ee8[0].matrix).
- Batch 323 (drafts): TextInput_Update 0x1015f4e0, Debris_Update 0x100564d0, Flames_Update 0x10051e20,
  Node::ShadeWaveCopy 0x10193d10 (open.tsv). The 1400-1700 byte functions now mostly end as register-allocation
  drafts; retries with a concrete lead (Route_StepPatrol: one int per decoded part; KeepDistance: PlayMotion call
  written in both branches) are worth a subagent each.
- Batch 324: Enemy_KeepDistanceFromPlayer 0x10027da0 matched (PlayMotion written once per branch, no speed
  pointer local); Route_StepPatrol 0x10029730 down to 16 bytes (register naming only); Enemy_WanderNearHome
  0x10026d80 drafted (enemy_wander.cpp, 6 bytes: dead rand() store kept by the original).
- Batch 325: Enemy_Defeat 0x100627d0 matched (enemy_defeat.cpp; debris.h declares the spawners). Drafts:
  TextureManager::CreateDevice 0x10148b00 (src_od/render_device.cpp, an /Od unit; render_device.h types the D3D
  structs; build_all now passes /Op- for src_od), TextureObject::DrawFill 0x1014ce40 (texture_fill.cpp) and
  Input::ReadControls 0x10143180 (input_controls.cpp; the joystick_buttons.cpp helpers became Input methods).
- Batch 326: TutorialBook_Update 0x10065450 matched (tutorial_update.cpp, C++ linkage for the EH frame;
  tutorial_book.h). The four tutorial_open/page_next drafts have no EH frame (C++ linkage changes nothing).
  Drafts: FontRenderer::DrawText 0x101501f0 (text_draw.cpp), Camera_ShiftOrbit 0x10019670 (camera_shift_orbit.cpp;
  DAT_10da93e0 is a Vec4 now), Npc_PatrolRoute 0x10044b10 (npc_patrol.cpp; CharState::arrivalAction).
- Batch 327: SaveSummary_Draw 0x10021e70 matched (save_summary.cpp; script_flag.h holds the shared ScriptFlag
  inline: clear_markers.cpp and helmet_face.cpp still have local copies). Drafts: Npc_Lookout 0x100b7b10,
  Ground_Dig 0x1003bc60, Camera_TrackPathFrame 0x1016b6c0, Char_SpiritUpdate 0x1002a740 (stack slots only),
  Spirit_StepRoute 0x10028fa0. Warning: adding five extern globals to char_vars.h cost UpdateDrillLevel its match
  (an epilogue schedule changed although it uses none of them); they now live in npc_lookout.h. When a header
  edit loses an unrelated match, bisect the headers with zz_ copies and move the new declarations elsewhere.
- Batch 328: Camera_PullToItemStand 0x10018870 matched (camera_item_stand_pull.cpp). UpdateDrillLevel is fragile:
  one extra declaration in matrix4.h (ZeroMatrix 0x101b0cd0) also cost its match, so ZeroMatrix lives in
  camera_modes.h; keep new declarations out of the headers drill_level.cpp includes (vec4, matrix4, node,
  effect_manager, anim_model, char_vars, game_vars). Bgm_Play takes two parameters (obj_symbols row renamed).
  Drafts: MapMinigame_DrawHud 0x100301a0, Bgm_Update 0x1005c680, TriangleBatch::AppendQuad 0x10172cb0.
- Batch 329: Camera_ChaseTarget 0x10035050, Enemy_FetchDroppedWeapon 0x100a8ac0, EffectManager::LoadDefinitions
  0x101886d0 (EffectDef typed with its 12 key tracks) and Render_Command 0x1013f070 (render_state.h: command names)
  matched. UpdateDrillLevel fixed for good: declaring level before gauge makes its two reloads come out in the
  original order whatever the headers declare (the old order matched only by a tie-break that header edits moved).
  Drafts: Game_FrameStep 0x10137b10 (was Screenshot_Capture), Npc_GestureWithNeighbour 0x100b9150 (C++ linkage).
- Batch 330: Sound_UpdateDopplerLoops 0x1004fbe0 matched after a stack-slot study: same-size locals are placed by
  reference count, not declaration order (NOTES "Stack-slot order"), and the swap was a wrong-variable statement.
  Fourteen drafts (open.tsv): RunStatements, GetMeshBuffers, Char_HikariRide, Kind43_UpdateShots, Char_StepRouteTarget,
  MapMinigame_DrawHudTwoScores, Node::QueueDrawFading, MapCursor_Walk (10 bytes), Npc_GreetWhenFaced,
  Npc_GatedRouteCarrier, Npc_GroupWatch (one block), TextureObject::DrawMesh, plus the earlier slot-only ones.
  Most remaining large-function drafts are register-allocation or stack-slot residues.
- Batch 331: MeshData::DrawDebugBox 0x10146db0 matched (was M_10146db0). Drafts (open.tsv): ItemDictionary_Update
  (prologue register only, 119 bytes from one nop), Node::CastSegment2 (pre-check registers), Node::SphereHitsMesh
  (radius copy slot; Sphere() cast to clean up), Node::ScanChunks, StatusPanel text (pragma to review), Gimmick use,
  Npc_ShockByLink. DemoRecording now derives from InputState; Game::UpdateWindows declared.
- Batch 332: Char_SwingPlatform 0x10025ce0, WorldRenderer::DrawCaption 0x101687e0 and DrawScreenOverlays 0x10184160
  matched (texture_mgr.h's Animator was the WorldRenderer; caption fields typed). Drafts: Char_RollingBall (+2 bytes),
  Regions_Build, Node::RayCast and Node::CastSegment (same bl pre-check wall as CastSegment2: register assignment,
  no helper found), DrawLightKind22 (effect_trail.h), DrawWorldPictures (same residue as DrawPictures), MeshData::Draw.
  Cleanup: TranslationMatrix/RotationY are declared per .cpp in about ten files (move to matrix4.h carefully).
- Batch 333: AnimModel::Draw 0x10163150 and Camera_Refresh 0x10015960 matched; Node::CollideModel 0x10193140 (draft,
  prologue only) and the box overlaps became Node thiscall members, which made AnimModel::IsClear/IsClearAny/
  IsClearFlagged match; Game::PointInRect is a method. Drafts: Npc_HoverCarrier (frame 0x84 vs 0x80),
  TextWindows_Draw (text_window.h), Map17_UpdateMoleGame (20 bytes), Player_WalkAlongPath (has gotos),
  Char_AreaZone, Npc_PerchOnNeighbour (4 bytes; seat.z += aligns every slot, so the tail seat needs ~2 more
  references). Pending: Npc_RoamRoute 0x100b2900 is drafted but parked outside the tree (its char_state.h edit was
  refused by the permission check; see the shared notes file npc_roam_route_pending.md).
- Batch 334: Enemy_PulseAndCharge 0x100a3900, Char_UpdateAttachSlots 0x1001c670, Enemy_StalkAndBreathe 0x100ac440,
  Enemy_StalkAndLunge 0x100ad0f0 and ModelBank_CompareDistance 0x10165210 matched. FontRenderer DrawText/DrawOutlined/
  DrawOutlinedIn take float depth; Game window setters and Node::RenderShadowTexture retyped. Drafts (open.tsv has
  the residues): Ground_BuildMesh, ModelBank::PrepareFrame, Game::DrawWindows, Scene_Reset, Npc_HurlAtPlayer (6 bytes),
  Node::Query, MapMinigame_DrawTimerHud, Enemy_ChaseAndHop, Npc_PerformTricks, Node::SplitIntoCells, Char_Drifter
  (wrap loop keeps st0), Player_StartAttack, Enemy_HoverAndShoot (frame 0x184 vs 0x174).
- Batch 335 (QUEUE section 5, very large functions): Menu_SaveLoad 0x10064630 matched once its two alignment pads
  were traced to MessageWindow_SetTitle(0) inlined (include/message_window.h; NOTES has the lesson). Drafts: Menu_DrawScene
  10022830 (14 bytes), Map_PlaceCharacters 1005def0, Texture_LoadIti 10174ce0, EffectManager::DrawLightKind7/23
  101713f0/10170580 (same ebp/ebx residue as DrawLight), Char_HikariButtonRide 10079470, AnimModel::UpdateFrame 1017a010,
  ListBank::ApplyOperator 10004b10, Game::UpdateWindows 1000a0b0, Scene_RunCommand 10083eb0. Watch new obj_symbols
  rows: a repeated pointer parameter mangles as a back-reference (PAH0, not PAHPAH).
- Batch 336: near-miss retries pay off. Npc_HurlAtPlayer 0x100b6740 matched (read the field, not a cached local; store
  order around a float spill); Menu_DrawScene down to 12 bytes (SIB order in two haze loops). New drafts: ShopMenu_Draw
  10056b30 (0 hoisted into ebx), Char_RouteFlier 1007c050 (stack layout only), MapMinigame_DrawHudMinuteTimer 100311b0
  (the minigame ftol wall: the inlined _ftol of HalfWidth(width) moves past TextWidth whatever the spelling),
  Enemy_StalkAndDashSlash 100af1b0 (15 stack-slot lines). Npc_PerchOnNeighbour/Map17_UpdateMoleGame retried, notes added.
  A new header declaration clashed with a local `void` one in app_0012.cpp (7 lost until removed): grep for local
  declarations of any function a new header declares.
- Batch 337: Shaga_Update 0x10042a30 matched; Npc_HoverCarrier 0x10048050 matched by a retry (Vec4 operators take
  const float&, so a lea of a float temporary is the literal, not a named local; block-scope the floats). Drafts:
  Enemy_PerchAndSwoop 100a5960 (two slots swapped), Char_DropEquipment 100615f0 (four slots permuted), Game_LoadData
  100cb0c0, Map16_UpdateDodgeGame 10053020 (1 byte, register permutation), Debug_DrawShape 10145ba0, Scene_HostCallback
  10092af0 (8 bytes, zero stores in the slot reset). Enemy_StalkAndDashSlash down to 10 lines, Char_RouteFlier to 1832.
  Loose ends: scene_start.cpp passes FUN_10092af0 (WorldCallback should be (int, int *)); frame_step.cpp declares
  Game_LoadData separately from FUN_100cb0c0.
- Batch 338: Enemy_CastAndTeleport 1006e150, TitleScreen_Update 10114950 and Partner_SpecialSkill 10078060 matched.
  Drafts: FieldItemMenu_Update 100ba2a0 (0/1 register roles swapped, gotos), Map_SetupFromEntry 100fda40,
  Enemy_HoverAndCallStrike 100ade30 (end clamp tail merge), Player_UpdateFrame 1004d960 (15 bytes), Enemy_ShyGhost
  10075350, Enemy_RoamAndSpit 10084e80, Enemy_RollingTire 100a4440 (sret temporaries share the hit slot);
  Map16_UpdateDodgeGame closer. include/shadow_ground.h holds the shared inlined Char_PlaceOnGround body.
  Tidy-up list (matched but worth a reader pass): TitleScreen_Update keeps seven unused Matrix4 locals spareA..G and
  Partner_SpecialSkill/Enemy_CastAndTeleport unused Vec4 locals (the original constructs them); Enemy_RollingTire uses
  do { ... break; } while (0).
- Batch 339: drafts only. Enemy_StalkAndStrike 100a9240 (7 bytes: a dead load of variant before rand), Map_SetupByNumber
  100fc4a0 (map_setup_field.cpp; Game::FindMapEntry trailing params are int*, scene_callback.cpp declares it C++),
  Char_LaserZone 1002d4e0 (laser_zone.cpp, one CSE'd pattern read; char_state.h gained the laser union members).
  Map_SetupFromEntry may wrongly search sakana01 instead of sakana%02d.
- Batch 340: nine drafts of section-5 functions, no new match: Texture_LoadBmp 10173550 (backward goto gives an ebp frame),
  Regions_StartLightMorph 10062e70 (uses a float bit-pun for the tint.w test: review), Map15_UpdateTargetRace 1007dab0
  (~8 lines), Enemy_ChaseLungeAndSpit 10076790, Partner_Follow 100e0320, Enemy_PounceAndBreathe 100675d0 (+12 bytes),
  Node::CollideProbesWithMesh 1019f340 (same length, layout), DodgeGame_UpdateGolemPiece 1005fc00, Enemy_WanderHopAndSpit
  10065bf0. char_state.h gained many union members (add-only). Very large functions rarely match first time: alternate
  with near-miss retries (1007dab0, 100675d0, 1002d4e0, 100a9240 are the closest).
- Batch 341: retries of 1002d4e0, 1007dab0, 100a9240 and 100675d0, no match. LaserZone's one site is a CSE/regalloc
  decision (17 spellings identical); TargetRace's stun chain threads in ours (45 spellings); StalkAndStrike's dead variant
  load comes only from an equal-arm if/else (fakematch); PounceAndBreathe got its real constants (45.5/34.5) but the esi
  zero register remains. These four are permuter candidates rather than spelling retries.
- Batch 342: Scene_DrawFrame 10038c10 matched (the in-game 3D frame; a redundant `!= 0 &&` test is genuine). Drafts:
  Render_ComposeFrame 101493f0 (early), Node::ApplyCommandText 101a3000 (15 bytes, two spill slots; needs throw()),
  Script_SystemCommand 100fedd0 (first pass), Char_PutOnEquipment 100404e0 (9 bytes, first init block load order).
  Lesson (NOTES): anonymous unions in Node/AnimModel change unrelated matched code; use one typed member instead.
- Batch 343: eight drafts, no new match: Enemy_PodLobber 10070200, Enemy_StalkAndClaw 100aa820, Char_HikariWarp 1002ae70
  (cl 13.00 never overlays sibling scopes with 2+ Matrix4 locals), FieldItemMenu_Draw 1003c570 (four tails not merged),
  DrillClaws_Update 1008fea0, SoccerGame_UpdateCpuPlayer 10032240, Game::LoadMapData 10002440 (game.h pads split into
  typed tables; one *(MapFront *)this cast until Game/DAT_10874848 is split), Char_UpdateByKind 100f9670 (uses
  words[WORD_*] macros: name the CharState members before matching).
- Batch 344: drafts Script_GameBuiltin 10138280 (frame 0x8f0 vs 0x81c), Menu_Main 10112140 (10 bytes over, frame right),
  Enemy_SpitAndCharge 1006be40 (18 bytes short). FUN_100b4f00's register tie depends on the declaration count of
  matrix4.h (NOTES); DirectionMatrix and Multiply now live there.
- Batch 345: drafts Game_Main 1013b700 (was Game_Startup; frame 0xb40 vs 0xbb0), Node::ParseChunks 101a58a0 (frame 8 bytes
  larger, code matches through KAN3), TextureManager::OnResetDevice 1013fbb0 (~7 real diffs: gamma prologue registers and
  the memory-total load/add/store inside the crossfade loops).
- Batch 346: drafts ListBank::CompileCall 10089a10, Enemy_FlockDiver 10086380 (49 bytes over), Enemy_LeapAndClimb
  10068f70 (regalloc: zero in esi, variant in memory), Npc_RoamZones 10049ec0 (party NPC zone wander; frame 0x18 short),
  EffectManager::Step 10184440 (asm-only transcription, 1 KB short); InverseMatrix declared once in matrix_transform.h.
- Batch 347: drafts Script_ValueCommand 101011b0 (value_command.h; 2 KB short, entry Vec4/Matrix4 locals omitted),
  Enemy_WispCaster 10071ee0 (kind 0x10), Camera_Update 1016c3e0 (5 KB short; wall-search loop approximate).
- Batch 348: drafts Field_Update 1011a060 (game mode 15; frame 0x838 vs 0x7c0), Hud_DrawFrame 1008c680 (original has
  a /GS cookie we lack), Scene_Frame 100f5a80 (one 16-byte local short; DAT_10c64f78 setter threading differs).
- Batch 349: MATCH NameEntry_Update 10116550 (game mode 10, name entry). Drafts Map_SetUpTrigger 10058790 (2 bytes
  short, schedule only; callback returns int but ForEachActiveTrigger takes a void callback, callers not switched),
  AnimModel::MoveWithCollisions 10175f20 (6 KB short: original writes four probe blocks inline).
- Batch 350: Map_SetUpTrigger KSKK/BOBJ fixed (now 15040 vs 15026; rest is regalloc); both GameTrigger callbacks
  return int, so ForEachActiveTrigger/RunQueuedEvents take int (*)(GameTrigger *). Drafts Node::UpdateWorldWithIk
  10194f40, Map18_UpdateFootballGame 1007f1e0 (minigame_area.h now owns the shared dodge/football arrays).
  10093cc0 (effect hit hook) left open with a plan: name its AnimModel fields/tables and retype DAT_1101de40 first.
- Batch 351: drafts Bs003_Update 100e25f0 (BS_003 boss, Tokatyu; 5 KB short, helpers where the original repeats
  blocks), Script_SceneCommand 1009e380 (ids 30000..30129 as sequential ifs; 3 KB short, approximate cases listed).
- Batch 352: drafts Bs001_Update 1010c650 (kind 100 boss, 11 KB short), Bs005_Update 100cc2a0 (dodge-game golem,
  9 KB short), Enemy_UpdateFighter 100bdad0 (kind 0x2d, 6 KB short); new enemy_voice.h/actor_scan.h/volume_fade.h.
- Batch 353: drafts Bs004_Update 100c39c0 (moss-root boss kind 0x46), NeoGalaxy_Update 100e7280 (kind 0x32 boss),
  WorldRenderer::RunScriptFrames 1017cc90 (scene script runner; CsvTable folded into WorldRenderer).
- Batch 354: drafts EffectManager::DrawRecord 10189530 and Prince_Update 100edea0 (both split into static helpers:
  fold them back into one body), Bs002_Update 10104640 (kind 105 boss, 11 KB short). Effect light array has 0x80+2.
- Batch 355: drafts MeshData::DrawLit 10151ba0 (macros stand in for the inlines; frame 0x278 vs 0x3c98) and
  Bs006_Update 100d24a0 (BS_006, kind 107; written in four stages, 55024 vs 54584 bytes). Huge functions: Ghidra
  times out in tools/fn.py; the scratchpad w24a0/scripts DecompileAt.java copy (1500 s, 400 MB payload) with
  GHIDRA_HEADLESS_MAXMEM=12G gives the full pseudocode; then draft in stages (skeleton with STAGE markers first).
- Batch 356: drafts Message_DrawLabels 1000cf80 (message_labels.cpp, 1648 vs 1689) and Image_HalveRgb 10161770
  (image_halve.cpp, 448 vs 374). Odd-address queue entries 1000cfa1, 10161781, 10017c55 and 1004f8b7 are bodies
  or branch targets of their owners: open.tsv status `fragment` keeps them out of QUEUE.md.
- Batch 357: structural passes. CompileCall 10089a10 frame 0xb49c -> 0xa494 (one shared left operand, sibling
  blocks); RunScriptFrames 1017cc90 22352 -> 28512 bytes (explicit field reads, split M/A commands). DrawLit 10151ba0:
  inline helpers do not enlarge the frame; cl 13.00 keeps block locals in unique slots only inside a switch body or a
  loop span, which the original lacks (open.tsv). 0x1011d810 (in-game frame) skeleton in game_frame.cpp with 12 STAGE
  markers (plan in the scratchpad w1d810/STAGES.md), being filled stage by stage.
- Batches 358-372: the in-game frame 0x1011d810 (game_frame.cpp) drafted stage by stage, one Sonnet agent per
  ~500-1000 pseudo lines, each splice gated by build_all + check_matches. First complete draft: 101280 bytes vs about
  107 KB, not yet compared as a whole (open.tsv lists the known gaps). Lessons: (1) a split pad must add up to the old
  pad exactly (ItemText lost FUN_1003c370 until fixed; size checks now on ItemText, AttackRecord, GimmickSlot);
  (2) agents mapped AnimModel methods onto addresses already matched as ModelObject methods (0x10162f20, 0x10164090):
  keep the declaration, drop the second obj_symbols row - ModelObject and AnimModel look like two views of one
  struct and should be merged; (3) never sed -i or rewrite a whole file (an agent destroyed game_frame.cpp once);
  (4) check new global arrays against their neighbours (the shove tables first overlapped).
  Cleanup left: DAT_1088e650 declared in gimmick_slot.h and item_dictionary.h with two types (hidden by an #ifndef),
  local declarations of FUN_1000f360/FUN_10016500/FUN_10011aa0/FUN_100f9670/FUN_101148b0 in several files, the
  `*(int *)&shift.z` cast in stage 4c (MoveTowards probably takes a float).
  Commit progress/jp/progress.svg together with report.json from the same build_all run (CI no longer commits it).
- Batches 373-376: compare passes. In-game frame: C++ linkage (?FUN_1011d810@@YAHXZ) gives the original EH
  prologue (NOTES "Linkage and EH frames"; Game_LoadData 0x100cb0c0 got the same fix), call sites 1920 -> 1943 of
  1945, frame 0x486c vs 0x490c (open.tsv has the next steps). Bs006_Update: direct table indexing instead of cached
  references cut its diff from 15295 to 4437 lines (the same change gave 0-14% on Bs001/002/004/005, Prince,
  NeoGalaxy; none on Bs003 and Enemy_UpdateFighter); three zero-initialised locals at the top caused the 0/1
  register hoist. Scratchpad helpers for the in-game frame (w1d810): tgt20.py/tgt20f.py call-sequence diff,
  slots20.py stack-slot counts, rep20.py exact-snippet CRLF replace.
- Paused after batch 376 at the orchestrator's request (external PRs being merged outside decomp/).
- Batch 377: Prince_Update is one function body again (state helpers folded; code the original repeats stays as
  static __forceinline helpers, main callee counts equal the original's). In-game frame: second burst group has its
  own direction local; shoveDirs[4] iterator still unexplained (open.tsv).
- Batch 378: Prince_Update PrinceFrame struct split into locals, arena test as the original's angle comparison,
  CHARGE_A/B separate, lock-on sparks/glow (31152 of 31560 bytes; frame now the gap: ours 0x6ac vs 0x27c). In-game
  frame: recovered the missing leap-start block, scene dispatch order swan/shove/rest, talk-block Vec4 hoist (call
  sites 1946 vs 1945, frame 0x48d4 vs 0x490c).
- Batch 379: wall-retry lanes matched 16 (858 -> 874). Levers that work under cl 13.00 where VS2003-era notes said
  'register allocation': one return at the end instead of early returns (push placement / zero register), stores in
  the original's asm order, identical copy-pasted branches/cases that cl cross-jumps, declaration order of locals,
  `x *= x` in place on a parameter. Prince_Update parked on its frame (open.tsv). Next: more wall-retry lanes from
  QUEUE section 1 (check that a function is not already drafted in another file before writing a new one).
- Batch 380: wall lanes matched 9 (874 -> 883). New levers: write small float helpers (Length4, LengthSquared2,
  DistanceSquared2D) out at the call site; `while` instead of `for` for loops inside a branch (TestScriptFlag
  expansion); split `a || b` early returns into two ifs; explicit per-element stores where the original unrolls a
  fill; loop bounds as expressions instead of locals. In-game frame: ladders highest state first (ratio 0.968).
  build.bat drops `/DV=n` (use `-DV#n`). Playback::Play had a parameter shadowing the member `layer` (fixed).
- Batch 381: walls of 700-1600 bytes, 4 matched (883 -> 887): a local of its own instead of reusing one (slot
  swap), result stored straight into the tested variable, return order. tools/sbs.py hides stack-offset swaps
  (scratchpad w381d/strict.py keeps offsets). VisNode::UpdateVisibleCells renamed to Node:: in obj_symbols.tsv.
  Recurring residue worth a focused lane: `cmp reg,ebx` (zero register) in the original vs our `test reg,reg`.
  ParamVector duplicates Vec4 (merge before Save_ReadSlotInfo can use CharSlot record).
- Batch 382: no new matches. Zero-register rule found (NOTES.md "Zero register"). ParamVector -> Vec4 merge loses
  CharSlot::operator= and FUN_1003e710: 0x101acc20 is really `Vec4 &Vec4::operator=(const Vec4 &)` (we call it
  Copy, ~2975 call sites). A repo-wide Copy -> operator= rename is its own task (then ParamVector can go and
  Save_ReadSlotInfo can use `CharSlot record;`). DrawQuad 1 byte, CompileStatement 6 bytes, three 2-line walls.
- Batch 383: the Copy -> operator= rename was tried and reverted (it loses Node::Clone: the original Vec4 is
  block-copyable; 0x101acc20 belongs to a second type, ParamVector stays). NOTES.md "Vec4 is block-copyable".
- Batch 384: near misses 2 matched (887 -> 889: InTriangle via an exhaustive Dot-order permutation script in the
  w384i scratchpad, Vis_IsCharacterVisible via a boxes pointer local). Large walls (1600-4000) gave none: they are
  mostly register choice and slot order (NOTES "Stack-slot order of locals"). The remaining queue is walls only;
  exhaustive expression-order permutation is the lever still paying off on near misses.
- Batch 385: no new matches (13 near misses with exhaustive permutation, 22 large walls). Input::ReadControls now
  the original's size. What is left is mostly register choice, slot order and scheduling that source order does not
  steer; a permuter-style search tool may be the next lever (ask before installing anything).
- Batch 386: cpp_permuter installed with the user's OK (E:\Claude Typing of the Dead\tools\cpp_permuter, built with
  clang; adapter tools\gurumin_permuter: compile.cmd + score.py, README there). First run matched 3 (889 -> 892):
  BoxSphere_Overlap, DrawDebugText, StatusPanel_UpdateText (each rewritten as plain source). Zero-register walls
  do not move with it. Next: run it on more near misses when the machine is quiet (Render_FillRect, Menu_DrawScene,
  AntiTamper_Check), then over the walls of section 1 sorted by size.
- Batch 387: permuter over 99 small walls (600 random iterations each, 1500 on 13 near misses) matched 14
  (892 -> 906). Every honest fix was statement or declaration order, a duplicated tail (soundarc code-5 call,
  TutorialBook_Close found = 1), or return-else nesting (BeginRenderTo). Reading the permuter diff and testing each
  hunk alone found the honest spelling in most cases. Steering-only states not applied: DrawWindowMarks (one yoda
  compare), Plane::Distance (redundant parentheses). Next: the 41 walls with base score > 40, by size.
- Batch 388: Model_ApplyCommands 0x10012220 matched (parameter `a` reused as the character index; 907).
  Improved, not matched: EffectManager::Draw (8 bytes), Render_DrawGradientQuad (6), 1016b050, 1005dbf0
  (DAT_104c033a is unsigned char), 1002f050, 10194a10. Left unexamined (queues did not finish): 10024d00, 100b5550,
  1001b7b0, 1000f360.
- Batch 389: Char_ProbeAhead 0x100242c0 and Char_ProbeDirection 0x100244b0 matched (909): testOnly branch
  `return a && b;`, tail `if (hit.x == -200000.0f) return 0; return 1;`. 10024d00 35 -> 23, 1001b7b0 19 -> 8
  (the original keeps an `fmul 1.0f` that cl 13 folds). Walls of base > 40 did not move by hand.
- Batch 390: permuter over the 109 walls of 900-3000 bytes (26 with base <= 40). Node::BuildCollisionSpheres
  matched (named `bool isCollisionNode`; 910). Enemy_AimedShots 13 -> 5 and Enemy_StarVolley (reuse dx/dy for the
  turn), Save_Write 30 -> 9 (declaration order). 11 matched functions named in symbols.tsv. Remaining walls are
  register choice, SIB operand order, stack-slot order and the zero register; the permuter's honest yield is low.
- **enemy_barrage.cpp + barrage_enemy.h** (new, rule-clean, C++ linkage because the originals have EH frames):
  Enemy_CallBarrage 0x100a77f0 (behaviour 15), Enemy_SpinFire 0x100a7db0 (14) and Enemy_CallBursts 0x100a8370
  (16/17, variant argument) match. Enemy_SpinFire's lesson (NOTES): a node pointer with a fallback (`fire`, else the
  head) is two cross-jumped calls `if (a) Call(a); else if (b) Call(b); else {...}`, not `a || (a = b)`.
- **Enemy_StarVolley 0x100a7150** (13) and **Enemy_AimedShots 0x100a6a50** (12) drafted there: control flow, calls
  and frame match; only pairs of 4-byte stack slots are swapped (open.tsv lists the spellings tried; AimedShots gets
  to 5 bytes by reusing dx for the turn). The ring points are `DAT_10868838[index][n - 1]` (the `lea [eax-0x40]`
  displacement is inside the masked relocation).
- **Enemy_FlyAndStrafe 0x1009c070** (clamp temporaries), Enemy_StrafeAndShoot 0x1009a690 / Enemy_StrafeAndBurst
  0x10099b30 (ebx/edi swap), Enemy_ChaseAndSlash 0x1009d490 (side-step tails), DrawPictures 0x10167440,
  Node::QueueDraw 0x10199e20, Node::BuildMesh 0x101a82d0, npc_greet.cpp, node_world.cpp and the region prologue
  wall: unchanged (see open.tsv).
- **101713f0 / 10170580 / 10187520** (trail renderers, 0x1600-byte frames, KeyBank locals built with the array
  constructor helper FUN_10004650, effect definitions at DAT_1101dfdc 0x3d8 each, point lists at DAT_118e99f8 0x4014
  each): not started; they need new types for both tables first.
- The large Enemy_UpdateBehaviour kinds 10076790 (the common enemy behaviour), 100a5960, 100a9240, 100aa820,
  100ade30, 100af1b0 are unstarted.

## Next steps
1. The trail renderer family 101713f0/10170580/10187520 is drafted (effect_trail_draw.cpp, effect_light.cpp): one shared
   residue (this in ebp, frame 12 bytes smaller in the original); try the per-branch drawCount fix in DrawLight too.
2. The large Enemy_UpdateBehaviour kinds (start with the smallest of 100a5960/100a9240/100aa820/100ade30/100af1b0;
   enemy_barrage.cpp shows the shared shapes: Vec4 player/self, atan2 facing, EffectOptions + Matrix4 temporaries).
3. Near-misses with a genuinely new idea: the stack-slot swaps of 100a7150/100a6a50 (maybe a permuter run),
   Enemy_FlyAndStrafe's clamps, 1009d490's side-step tails, the 1009a690/10099b30 swap.
4. Section 4 large functions of `decomp/QUEUE.md` in order (character behaviours run from 0x100f9670 and the
   map-route family match well: delegate them to subagents).

## How the merges were run (reuse)
- Types: `python tools/type_move_driver.py apply <files>` then `python tools/prune_header_includes.py <new headers>`;
  views by hand (write the merged header, cut each copy, rename fields per file, build each file and compare its
  MATCH/DIFF lines with the HEAD version built as a scratch `src/zz_*.cpp`).
- Globals: `globals_driver.py plan|apply`, `use_header_globals.py`, `resolve_globals.py`. A global whose type is
  file-local cannot go into a shared header until the type does.
- Process files as bytes and keep each line's ending; never `sed -i` a CRLF file. Bash heredocs with `\n` inside
  Python strings get mangled on this machine: write scripts with the Write tool. Check `git diff --stat`.
- `check_matches.py` reports a renamed symbol as LOST + new: if the address and count are unchanged, update the
  `obj_symbols.tsv` row (this session: SceneNode/NodeBody -> Node, Bind's ModelRecord -> AnimModel).

## Continuation prompt
"Continue the Gurumin decompilation. Read `decomp/HANDOFF.md` (including its Standing instructions) and `AGENTS.md`.
Work through HANDOFF 'Next steps': first the trail renderer family 101713f0/10170580/10187520 (type their tables
first), then the large Enemy_UpdateBehaviour kinds, then the remaining near-misses with a genuinely new idea. Put
every new type and global in a shared header so new matches count under the rules. After each batch run
`python tools/build_all.py --report ../progress/jp/report.json` and `python tools/check_matches.py` from `decomp/`;
commit and push only with no real loss (a rename shows as LOST + new at the same address: update `obj_symbols.tsv`).
Keep everything human-readable (named constants, liberal comments, informative names). Work autonomously. Rewrite
HANDOFF.md after each batch with the Fully linked / Matched / Matched under the rules / Fuzzy numbers, keeping its
Standing instructions section and a continuation prompt."
