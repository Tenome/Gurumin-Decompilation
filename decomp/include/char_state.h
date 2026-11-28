// Per-character behaviour state: 0x40 bytes for each of the 0x80 character slots (DAT_1020b430). Every behaviour
// (NPC idling and talking, enemy chases, pendulums, route walkers, scripts, scripted shoves, ...) keeps its own
// variables here, so most words mean different things to different behaviours. The header gives each offset a
// union of the names the behaviours use; the comment on each name says which behaviour (source file) uses it.
// game_reset.cpp clears the whole array when a game starts; char_move.cpp clears one slot when a behaviour starts.
// This is the one declaration of this type: include this header, never redeclare it.
#ifndef GURUMIN_CHAR_STATE_H
#define GURUMIN_CHAR_STATE_H

// Number of character slots (DAT_1020b430, DAT_10db6ee8 and the other per-character arrays).
#define CHAR_SLOT_COUNT 0x80

// One scripted shove (char_shove.cpp): a character slides by a per-frame step for some frames, or turns to look at
// another character. A slot's state holds four of them while shoves run.
struct Shove
{
    int target;                   // character slot + 10000 * (non-zero = keep its heading), negative = look target
    int stepX;                    // per-frame step (for a look target: the character to look at)
    int stepY;
    int frames;                   // frames left + 10000 * (non-zero = keep its motion)
};

// The football (map 0x18) CPU player's state (soccer_ai.cpp): which of the three roles the player has, whether it
// goes for the ball, and the counters of its jump and of its sidestep around other players and the goal nets.
struct SoccerState
{
    int role;                     // 0x00: 0 = not picked yet, 1 = ball chaser, 2 = back player, 3 = forward player
    int pursuing;                 // 0x04: 1 = going for the ball, 0 = keeping its post
    int idleDelay;                // 0x08: frames to stand still before the idle motion starts
    int roleOwner[3];             // 0x0c: per role (1..3, team 0): the slot of the player holding it
    int jumpSpeed;                // 0x18: vertical speed of the jump in progress (0 = none)
    int jumpCooldown;             // 0x1c: frames before the next jump
    int netSide;                  // 0x20: way round the goal net: 0 = none, 1/2 = along y, 3/4 = along x
    int sidestep;                 // 0x24: way round another player: 0 = none, 1 = one side, 2 = the other
    int sidestepFrames;           // 0x28: frames the sidestep side is held
    int sidestepSwitch;           // 0x2c: frames the other player has been in the way (flips the side at 30)
    int netFrames;                // 0x30: frames spent sliding along the net (flips the way at 90)
    int delayA;                   // 0x34: counts down every frame
    int delayB;                   // 0x38: counts down every frame while positive
    int pad_003c;
};

// The BS_001 boss (bs001_update.cpp) keeps its state in its own slot (Bs001State) and uses the two slots after it:
// Bs001Aux (the next slot's state: action bookkeeping and the knock-back of its partner) and Bs001Fade (the slot
// after that: the fade out and fade in counters).
struct Bs001State
{
    int state;                    // 0x00: action (0..0x12 = the moves, 100 = pick the next one)
    int frame;                    // 0x04: frames spent in the action
    int homeX;                    // 0x08: position the cut-scene put the boss at (whole units; state 9 saves and restores it)
    int homeY;                    // 0x0c
    int homeZ;                    // 0x10
    int motion;                   // 0x14: motion to play (-1 = keep)
    int shownMotion;              // 0x18: motion shown last frame
    int motionFrames;             // 0x1c: frames the shown motion has run
    int idle;                     // 0x20: always cleared at the start of the frame
    int initialised;              // 0x24: 1 once the models and nodes were set up
    int hitRecover;               // 0x28: frames left before the boss reacts to a hit again
    int defeatTimer;              // 0x2c: frames of the defeat sequence
    int finished;                 // 0x30: 1 for a frame after an action ended
    int phase;                    // 0x34: 0 = healthy, 1 = hurt (below half health), 2 = enraged
    int hurtLevel;                // 0x38: grows by 10 per hit and decays
    int cutsceneHeld;             // 0x3c: 1 while a cut-scene or message holds the game
};

struct Bs001Aux
{
    int lastState;                // 0x00: the previous action
    int m_0004;                   // 0x04
    int attackCount;              // 0x08: moves chosen since the last rest
    int streak;                   // 0x0c: moves in a row that raise the chance of a stagger
    int partnerCooldown;          // 0x10: frames until the next hit counts (200 after a fall)
    int knockZ;                   // 0x14: knock-back of the partner (hundredths)
    int knockX;                   // 0x18
    int knockY;                   // 0x1c
    int knockActive;              // 0x20: 1 while the partner is thrown
    int waitFrames;               // 0x24: rests left before the next big attack
    int voiceCooldown;            // 0x28: kind 100's voice cooldown (enemy_voice.cpp)
    int m_002c[3];
    int lastHitLevel;             // 0x38: the hit level handled last
    int advancePartner;           // 0x3c: 1 = the partner's motion is stepped once more
};

struct Bs001Fade
{
    int fadeOut;                  // 0x00: frames of the fade out (1..30)
    int fadeIn;                   // 0x04: frames of the fade in (1..30)
};

// The stone golem boss of the dodge game (bs005_update.cpp, kind 0x3f): its state words by name. Words 8..15 are also the
// voice cooldowns of the neighbouring boss kinds (the golem's own is `voiceCooldown`).
struct Bs005State
{
    int state;                    // 0x00: 0 = set up, 1 = pieces woken, 2 = rests, 3..15 = attacks and turns, 100 = picks the next one
    int frames;                   // 0x04: frames in the current state (1000 and 2000 mark the second part of some states)
    int variant;                  // 0x08: random 0/1 (or 0..13) picked when the state starts
    int calmFrames;               // 0x0c: frames since the golem last picked an attack (counts while it is not held)
    int attacking;                // 0x10: 1 while an attack state runs
    int cameraHidden;             // 0x14: 1 while the camera nodes "CAboss" are hidden (the ending uses it as the motion)
    int savedStat4;               // 0x18: CharSlot::stats[4] before state 9 raised it
    int lastHealth;               // 0x1c: health at the end of the last frame
    int endingSetUp;              // 0x20: ending: 1 once the ending scene was set up
    int voiceCooldown;            // 0x24: frames before the golem speaks again (see Bs005_SayLine)
    int endingMotionStarted;      // 0x28: ending: 1 once its motion was started
    int endingTimer;              // 0x2c: ending: frames since it began (starts at -30)
    int healthMark;               // 0x30: health when the last attack was picked or the calm counter was reset
    int hitLevel;                 // 0x34: DAT_102550e0 of the last frame
    int motionFrame;              // 0x38: frame of the playing motion (-1 while blending)
    int motionLayer;              // 0x3c: layer of the playing motion
};

// The BS_004 boss (bs004_update.cpp): the moss root creature that follows a path of nodes, then hovers, dives and
// breathes fire. Words that the ending scene reuses: `hits` is its "motion started" flag, `hitLevel` its timer.
struct Bs004State
{
    int state;                    // 0x00: 0 = rises and hovers, 1 = follows the path, 2..19 = moves and attacks, 100 = picks the next
    int frames;                   // 0x04: frames in the current state (1000 and 2000 mark later parts of states 15 and 17)
    int motion;                   // 0x08: motion the state wants to play (-1 = none)
    int setUp;                    // 0x0c: 1 once the boss loaded its models and nodes
    int playingMotion;            // 0x10: motion that was started last (-1 = none)
    int pathPoint;                // 0x14: index of the path point it flies to
    int previousPoint;            // 0x18: the point before
    int lastState;                // 0x1c: the state picked last
    int minionsDown;              // 0x20: minions that were defeated so far
    int minionsAlive;             // 0x24: minions still standing
    int hits;                     // 0x28: hits taken since the last reaction (the ending: 1 once its motion started)
    int hitLevel;                 // 0x2c: DAT_102550e0 of this frame (the ending: frames since it began, from -30)
    int motionLayer;              // 0x30: layer of the playing motion
    int motionFrame;              // 0x34: frame of the playing motion (-1 while blending)
    int requested;                // 0x38: state requested by the end of state 12 (1 = state 1, 2 = state 10)
    int paused;                   // 0x3c: 1 while the game is paused
};

// The BS_002 boss (bs002_update.cpp, kind 105): a big beast that circles the player on a ring and strikes from it. Words of
// its own slot; the words before the record (0x1020b3f0, Bs002Extra) and the ending flag in the record before it are
// described in bs002.h.
struct Bs002State
{
    int state;                    // 0x00: 0 = set up, 1/2 = circle to the left/right, 3 = lunge ring, 4 = ring walk, 5..0xf = attacks, 100 = pick the next one
    int frames;                   // 0x04: frames in the state (1000 and 2000 mark later parts of state 10)
    int motion;                   // 0x08: motion started last (-1 = none)
    int lastState;                // 0x0c: the state before the pick
    int direction;                // 0x10: way round the ring (+1 or -1)
    int angle;                    // 0x14: angle on the ring in 1/1000 radians
    int height;                   // 0x18: height above the pit floor it stands at (follows targetHeight)
    int targetHeight;             // 0x1c: height it climbs or drops to
    int radiusBoost;              // 0x20: growth of the ring radius while it circles
    int riseSpeed;                // 0x24: how fast `height` follows `targetHeight` (at least 100)
    int hits;                     // 0x28: the ending: 1 once its motion started
    int mode;                     // 0x2c: height mode of the frame (0 free, 1..3); the ending: frames since it began, from -30
    int markedTarget;             // 0x30: character slot the target mark follows (-1 = none)
    int lunging;                  // 0x34: 1 while a lunge swing is armed
    int motionFrame;              // 0x38: frame of the playing motion (-1 while blending)
    int motionLayer;              // 0x3c: layer of the playing motion
};

// The prince (prince_update.cpp, kind 0x43): the fighter with the swappable weapon (O_100..O_103) and the gatling gun.
struct Kind43State
{
    int state;                    // 0x00: 0 = set up, 1..16 = moves, 100 = picks the next one
    int frames;                   // 0x04: frames in the current state (1000, 2000, 3000 mark later parts)
    int healing;                  // 0x08: 1 while the prince regains health (30 a frame up to the maximum)
    int weapon;                   // 0x0c: weapon held: 0 = bare, 1 = O_101, 2 = O_102, 3 = O_103
    int lastState;                // 0x10: the state picked last
    int wobble;                   // 0x14: 1 or 3 while the height wobbles, 2 = steady
    int counter;                  // 0x18: general counter of the state (shot count, chances)
    int heightPhase;              // 0x1c: direction * 1000 + height step (0x19 rises, 0x401 falls)
    int pad_0020;                 // 0x20
    int lastHeight;               // 0x24: height of the last frame (whole units), 0xffffd8f0 while it flies
    int attackMotion;             // 0x28: motion of the attack the state plays
    int engaged;                  // 0x2c: 1 once a state took hold of the prince
    int hurt;                     // 0x30: 1 for a frame after it was hit
    int age;                      // 0x34: frames since it started (the ending counts it from -30)
    int syncedFlag;               // 0x38: DAT_10315350 of the slot (the ending: 1 once started)
    int voiceCooldown;            // 0x3c: frames before the prince speaks again (shared with the other bosses)
};

// The BS_006 boss (bs006_update.cpp, kind 107): the dragon with fifteen body pieces in the slots after it. Words of the
// boss's own CharState record; its defeat timer lives in word 11 of the record after it (DAT_1020b49c, bs006.h).
struct Bs006State
{
    int state;                    // 0x00: 0 = set up, 1..0xe, 0xc9..0xcd, 0x12d..0x130 = actions, 100 = pick the next one, 0x65 = play the queued motion
    int frames;                   // 0x04: frames in the state (1000 and up mark the second half of some states)
    int impact;                   // 0x08: 1 or 2 while a stomp or landing shakes the ground (starts effect 0x45d)
    int maxHealth;                // 0x0c: health at the start of the fight (state 0 copies it from the slot)
    int lastState;                // 0x10: the state before the pick (kept while the state is not 100)
    int queuedMotion;             // 0x14: motion state 0x65 plays (-1 = none)
    int focusValue;               // 0x18: the value DAT_1101dddc follows (0xe6, -200 after a hit that staggers it)
    int lastHealth;               // 0x1c: health at the start of this frame (the damage taken is the difference)
    int recoverFrames;            // 0x20: frames until focusValue returns to 0xe6
    int attackPhase;              // 0x24: 0..3: how far the fight has gone (set by the hits it takes)
    int pad_0028[6];              // 0x28..0x3f: not used
};

// The words of the BS_006 boss's piece slots (slots index + 2 + k, k = 0..14).
struct Bs006Piece
{
    int phase;                    // 0x00: the hit flash counter: 1 = armed, 2 = the piece follows its node
    int active;                   // 0x04: 1 for the first four pieces, 0 for the others
    int pad_0008;                 // 0x08
    int frame;                    // 0x0c: the frame the piece's node shows (Node::frameSource points here)
    int pad_0010;                 // 0x10
    int healthOverride;           // 0x14: non-zero = the health the boss counts for this piece (companion pieces)
    int pad_0018[10];             // 0x18..0x3f: not used by the boss
};

struct CharState
{
    union
    {
        struct
        {
            union                                 // 0x00
            {
                int flierPoint;           // route_flier.cpp: the route point the flier last reached
                int targetSlot;           // target_race.cpp (slots 0x3c..0x43, one per target): character slot of the target, 0 = free
                int state;                        // behaviour step: 0 = not started yet, then per behaviour
                int turn;                         // damage_zone.cpp: turn counter of the zone
                int counterValue;                 // digit_counter.cpp: the value shown (0..1000)
                int approachStarted;              // npc_approach.cpp: 1 once the approach has begun
                int walkingBack;                  // route_follow.cpp: 1 while walking the route backwards
                int gatePoint;                    // npc_gate_carrier.cpp: route point it comes from (the point it is at)
            };
            union                                 // 0x04..0x37: behaviour variables
            {
                struct
                {
                    union                         // 0x04
                    {
                        int bossFrame;              // bs003_update.cpp: frame of the boss's animation (the cut-scenes run it up to 0x1644, fights count it as the playing frame)
                        int podFrames;              // enemy_pod_lobber.cpp: frames since the state began (state 2 counts the lob step 1..30, state 3 up to 99)
                        int lookTarget;           // scene_frame.cpp (slot 0): the character the one named by `state` turns to look at
                        int flierMode;            // route_flier.cpp: 0 = waiting for a route, 1 = flying, 2 = vanishing
                        int pounceStep;           // enemy_pounce_breath.cpp: step of state 10 (0 = roars, 1 = chases, 2 = pounces, 3 = breathes); in state 1 the frame the whirl begins
                        int dropHeight;           // target_race.cpp (target): height the target falls from (900, 30 less each frame to -150)
                        int wait;                 // frames in the current step (or until the next voice line)
                        int gatePrevious;         // npc_gate_carrier.cpp: the route point before gatePoint
                        int scriptRunning;        // char_script.cpp: 1 while the character's script runs
                        int cooldown;             // kind4a_turret.cpp: frames before the next random attack
                        int talkMotionStarted;    // npc_greet.cpp: 1 once the talk motion was started
                        int routeStopped;         // route_walk.cpp: non-zero = the character stays where it is
                        int layerCount;           // flame_layers.cpp: flame layers found (their timers are values[1..])
                        int counterPhase;         // digit_counter.cpp: 0 = idle, 1..10 = turning to count + 1
                        int shockFrames;          // npc_link_shock.cpp: 0 = standing, 1..60 = frames shaking (counted up)
                        int ballFrame;            // rolling_ball.cpp: 0 = not set up, 1 = waiting, then the frame count of the roll
                        int zoneRampStep;          // area_zone.cpp: how much the push grows each frame the player is inside
                        int driftSignX;           // drifter.cpp (state 0): non-zero = the x step is added, else subtracted
                        int orbitForward;         // drifter.cpp (state 1): non-zero = the spin angles grow, else they shrink
                        // (route_follow.cpp uses `wait` for the frames left at a pause point)
                        int swoopStep;                    // enemy_hop_spit.cpp: 0 = turning left, 1 = turning right, 2 = hopping at the player, 3 = resting, 4 = spitting
                        int gaitHold;                          // enemy_spit_charge.cpp: frames left of the recovery in gait state 2 and of the pause in gait state 1 (90 or 10 when it starts; the lift speed is released when it runs out)
                        int flockFrames;                       // enemy_flock_diver.cpp: frames in the current step (moves are timed from it; 1000s and 2000s mark the later stages of a step)
                        int leapFrames;                                      // enemy_leap_climb.cpp: frames in the current state (a step of the leap counts them up)
                        int wispFrames;            // enemy_wisp_caster.cpp: frames in the current state (state 3 counts the lunge, state 6 the ritual, state 7 the summon)
                        int zoneStepFrames;            // zone_roamer.cpp: frames in the current step of the zone walk (stand-still counter of the fade steps)
                    };
                    union                         // 0x08
                    {
                        int bossPose;               // bs003_update.cpp: 0 = standing, 1/2 = turning left/right, 3 = claw swipe, 4 = tail swipe (the pose picks the motion)
                        int podGrowth;              // enemy_pod_lobber.cpp: 0 = hidden, 1..29 fading in or out, 150 = fully there
                        int lookActive;           // scene_frame.cpp (slot 0): non-zero = the character `state` keeps turning towards `lookTarget`
                        int flierRoute;           // route_flier.cpp: the map record whose route it flies (record 5 flies the other way)
                        int stepFrames;           // enemy_pounce_breath.cpp: frames counted in the current step
                        int rivalHit;             // target_race.cpp (target): 1 once a rival destroyed it (scores for the rivals)
                        int talk;                 // talk id (script number * 100 + line) or the behaviour's
                                                  // parameter from the map (distance, speed limit, ...)
                        int pausing;              // enemy_chase.cpp: 1 = pausing instead of chasing
                        int facing;               // npc_facing.cpp: idle facing in steps of pi/6 (100+ = never
                                                  // turns to the player while talking)
                        int scriptStep;           // char_script.cpp: current script step
                        int attacking;            // kind4a_turret.cpp: 1 while an attack motion plays
                        int approachCode;         // npc_approach.cpp: stop distance / 10 (% 100) and release flag
                                                  // (/ 100)
                        int targetPoint;          // route_walk.cpp: route point the character heads for
                        int hideCount;            // companion_update.cpp (companion slot): frames the companion
                                                  // stays hidden (2 in the areas 0x13 and 0x30), counted down
                        int keepCompanionUp;      // companion_update.cpp (owner slot): 0 = a hidden companion
                                                  // sinks 50 units
                        int weaponMarkA;          // weapon_attach.cpp: -2 after an unknown weapon (no model)
                        int stuckFrames;          // enemy_slash.cpp: frames counted without moving
                        int wanderState;          // enemy_wander.cpp: 0 = walking, 1 or 2 = looking for a heading
                        int spiritStep;           // char_spirit.cpp: motion step of the character's motion table row
                        int lookoutMode;          // npc_lookout.cpp: what the lookout watches for (1..5)
                        int hitFrames;            // map_cursor_walk.cpp: frames since it was hit (state 1), 0..0xaa
                        int ballSlot;             // rolling_ball.cpp: slot of the character dropped in (states 1..30)
                        int ballTimer;            // rolling_ball.cpp: frames counted in state 3000
                  int swingReady;           // swing_platform.cpp: 1 once the platform was set up
                        int holdFrames;           // mole_game.cpp: frames to wait before the next step (counted down; 5 after a hit)
                        int reactFrames;          // enemy_wisp_caster.cpp: 0 = not set up yet, 1 = ready, 0x1f = just woke or was hit (counted down to 1)
                        int squashStep;           // pulse_charger.cpp: 0 = not squashed, 1..6 = step of the squash after a hit
                        int hoverReady;            // enemy_hover_shooter.cpp: 1 once the hoverer was set down above the ground
                        int zoneRampMax;           // area_zone.cpp: the push stops growing at this strength
                        int driftSignY;           // drifter.cpp (state 0): non-zero = the y step is added, else subtracted
                        int orbitCentreX;         // drifter.cpp (state 1): x of the centre the character circles (whole units)
                        int startHeight;          // perch_swooper.cpp: height (whole units) of the player when it took off
                        int sightBlocked;         // enemy_shy_ghost.cpp: 1 = a wall blocks the line to the player (looked up every 15th frame)
                        int rayLength0;           // rolling_tire.cpp: length of ray 0 (outermost left, whole units, -1 = not cast yet)
                        int debrisMarker;          // dodge_debris.cpp: marker slot (0x5c..0x7b) whose position the piece takes over and whose death destroys it
                        int resetSpare;                        // enemy_spit_charge.cpp: cleared by the reaction reset, otherwise unused
                        int flockPattern;                      // enemy_flock_diver.cpp (slot 0x3a): attack pattern 1..7 the flock follows when it is idle
                        int flockMotion;                       // enemy_flock_diver.cpp: motion last played (-1 at the start)
                        int lyingDown;                                       // enemy_leap_climb.cpp: 1 while it lies on the ground after a fall (state 11), 0 once it gets up
                        int nextZone;                  // zone_roamer.cpp: zone it heads for (script flag base + 0)
                    };
                    union                         // 0x0c
                    {
                        int bossBreathHeld;         // bs003_update.cpp: non-zero = the breath attack is not started on its key frame
                        int podHideFrames;          // enemy_pod_lobber.cpp: frames left of the flicker that hides the model (level 9 only)
                        int flierFrames;          // route_flier.cpp: frames left of the vanishing (counted down from 5)
                        int noticed;              // enemy_pounce_breath.cpp: 1 once it noticed the player (or was hit)
                        int edgeAhead;             // enemy_chase_lunge.cpp: 1 = no ground ahead of it (a ledge), tested every 3rd to 10th frame by slot
                        int range;                // char_move.cpp: distance at which a waiting character reacts
                        int count;                // enemy_react.cpp: frames counted towards the next action
                        int pauseFrames;          // enemy_chase.cpp
                        int angle;                // enemy_pendulum.cpp: swing angle in tenths of a degree
                        int idleStarted;          // npc_facing.cpp: 1 once the idle motion was started
                        int saved;                // npc_frozen.cpp: the slot's m_0140 while held
                        int marker;               // npc_ride.cpp: the marker character ridden behind
                        int push;                 // npc_wheel.cpp: push strength (speed = push / 4 - 20)
                        int route;                // route_carrier.cpp: the route walked
                        int revealed;             // enemy_wisp_caster.cpp: 1 once it was hit and showed itself (the flash model started)
                        int registered;           // char_script.cpp: 1 once the position is in the camera list
                        int attackTimer;          // kind4a_turret.cpp: frames left of the attack
                        int stopDistance;         // npc_approach.cpp
                        int placed;               // route_walk.cpp: 1 once the character was put on its route
                        int cameraFrames;         // camera_follow.cpp (slot 0): frames since the closing camera
                                                  // started (effect every 65, sinks after 90)
                        int pullVariant;          // camera_item_stand_pull.cpp (slot 0): random bits picked when the pull shot starts
                        int downTimer;            // companion_update.cpp (companion slot): frames counted in
                                                  // the down / revive cycle
                        int companionHealthCap;   // companion_update.cpp (owner slot): a revived companion gets
                                                  // at most 2000 + this
                        int weaponMarkB;          // weapon_attach.cpp: -2 after an unknown weapon (no model)
                        int fleeSide;             // enemy_flee.cpp: 1 = veers left, 2 = veers right (picked at random)
                        int hikariSpeed;          // char_hikari.cpp: speed counter, 0..90 (10 while it waits)
                        int spiritReady;          // char_spirit.cpp: 1 once the spirit was set up
                        int promptFrames;         // npc_lookout.cpp: frames the key prompt stays (counted down)
                        int watchPhase;           // npc_group_watch.cpp: 0 = idle, 1 = turning to a groupmate, 2 = showing an emote
                        int neighbour;            // npc_gesture.cpp: slot of the character it reacts to (own slot - talk / 100)
                        int gazeTimer;            // npc_greet.cpp: frames left in the current gaze step (counted down)
                        int shakeX;               // npc_link_shock.cpp: shake steps of the three axes, -/+ (x)
                        int ballSavedKind;        // rolling_ball.cpp: the dropped-in character's kind, restored after it landed
                  int swingStatus;          // swing_platform.cpp: 7 = the swing is running, -1 = the character was told to leave
                  int hoverLimits;          // npc_hover_carrier.cpp: talk % 100 = bits that tie it to its home position (1/2 x/y pinned to home, 4/8 x/y kept from passing home upwards, 16/32 x/y kept from passing home downwards)
                        int holeIndex;            // mole_game.cpp: the hole ("@p1".. "@p9" = 0..8) the mole heads for or dives into
                        int zoneRamp;              // area_zone.cpp: the push strength now (1/100 of the force)
                        int calmed;               // pulse_charger.cpp: 1 after a charge ended (it does not turn on the player until hit)
                        int hopTimer;             // enemy_chase_hop.cpp: frames left of the hop push (counted down, each frame speeds the motion up)
                        int hoverMotion;           // enemy_hover_shooter.cpp: motion to play (2 / 4 = banking, 7 = flying)
                        int hurlReactDigit;       // npc_hurl.cpp: talk % 10, a ready thrower throws with a chance of 2 * this + 2 percent per frame
                        int driftSignZ;           // drifter.cpp (state 0): non-zero = the z step is added, else subtracted
                        int orbitCentreY;         // drifter.cpp (state 1): y of the centre the character circles (whole units)
                        int anchorPoint;          // perch_swooper.cpp: which of the four LastPosition vectors it swoops to (1..3)
                        int castSetUp;            // enemy_cast_teleport.cpp: 1 once its spine node was found and it stood on the ground
                        int strafeFrames;         // enemy_shy_ghost.cpp: sidestep timer, positive = drifting one way (counts down), negative = the other (counts up)
                        int huntFrames;           // enemy_roam_spit.cpp: 0 = not hunting, else frames since the hunt began (counted to 150, then back to 0)
                        int rayLength1;           // rolling_tire.cpp: length of ray 1
                        int debrisShown;           // dodge_debris.cpp: 1 while the piece's node is shown
                        int updating;                     // enemy_hop_spit.cpp: 1 while its update runs (the others of its kind do not wake it then)
                        int gaitFrames;                        // enemy_spit_charge.cpp: frames counted in gait state 0 (a stride is 0x9b frames) and in gait state 1
                        int flockSide;                         // enemy_flock_diver.cpp: 0 = circles one way, non-zero = the other (a follower copies the previous slot)
                        int hoverPattern;                                    // enemy_leap_climb.cpp: bob pattern of the vertical move in states 0 to 4 (0 = rising, 1 = hovering, 2 = sinking, 3 = pit, 4 = held)
                        int currentZone;               // zone_roamer.cpp: zone it is in (script flag base + 1)
                    };
                    union                         // 0x10
                    {
                        int bossFlameOn;            // bs003_update.cpp: 1 while the flame breath burns (toggled on its key frames)
                        int podMotion;              // enemy_pod_lobber.cpp: motion the model plays (0 idle, 1 lobbing, 2 growing, 4 lobbing)
                        int action;               // char_move.cpp: marker to run to (Npc_RunToMarker), alarm
                                                  // raised (Npc_RideAlarm), attacks left to chain
                                                  // (Enemy_ShoveAndBlast)
                        int running;              // enemy_chase.cpp: 1 = running (faster) after the player was far
                        int weaponShowDelay;      // enemy_wisp_caster.cpp: frames before its weapon node is shown after it revealed itself
                        int overPit;               // enemy_chase_lunge.cpp: 1 = it was over the pit area (mode 2) and keeps falling
                        int strafeMode;           // breath_enemy.cpp: 0 = circles left, 1 = right, 2 = holds, 3 and 4 = also hold
                        int swingingBack;         // enemy_pendulum.cpp: 1 while swinging back
                        int nextPoint;            // route_carrier.cpp: the route point headed for
                        int partnerFade;          // partner_follow.cpp: frames of the fade out/in (0..15)
                        int downMode;             // companion_update.cpp (companion slot): 0 = active,
                        int passenger;            // npc_gate_carrier.cpp: slot of the enemy it carries (0 = none)
                                                  // 1 = reviving, 2 = recovering, 10 and above = going down
                        int spiritSize;           // char_spirit.cpp: 100 (with an "@O_047b" node) or 200; sets the swarm
                        int hikariPoint;          // char_hikari.cpp: the route point it last reached (route step output)
                        int eyeOpen;              // npc_lookout.cpp: 1 = the lamp's eye is lit
                        int groupSize;            // npc_group_watch.cpp: groupmates counted (same group digit, behaviour 3)
                        int pitPhase;             // map_cursor_walk.cpp: 0 = outside the pit area, 1 = entering, 2 = inside,
                                                  // 3 = leaving
                        int greeted;              // npc_greet.cpp: 0 = has not greeted yet
                        int shakeY;               // npc_link_shock.cpp: shake step of the y axis
                  int swingFlip;            // swing_platform.cpp: non-zero = the acceleration flips when the angle passes 0 or +-180
                  int hoverGround;          // npc_hover_carrier.cpp: ground height under the carrier (-5000 = none)
                        int lift;                 // mole_game.cpp: upward kick after a hit (0x30), 3 less each step it moves
                        int zoneType;              // area_zone.cpp: 0..4 push, 5 / 7 / 8 camera setting, 6 = other zone
                        int perchMode;            // npc_perch.cpp: talk % 100 (0 = copies the neighbour's heading, 1 and 2 = turn to the player and spit, 3 = carried)
                        int trailFrames;          // pulse_charger.cpp: frames left of the trail effect it leaves
                        int phaseFrames;          // enemy_chase_hop.cpp: frames counted in the following step (3)
                        int hurlRetryDigit;       // npc_hurl.cpp: (talk / 10) % 10, after a throw the next one starts when a roll of 100 reaches (this + 1) * 3
                        int driftStarted;         // drifter.cpp: 1 once the drift or orbit was set up
                        int flapTicks;            // perch_swooper.cpp: wing beat counter (flap speed * 50 per frame; a wing flap at 300)
                        int sightCountdown;       // enemy_cast_teleport.cpp: frames it keeps hunting after losing sight of the player (reset to 300)
                        int circleSide;           // enemy_hover_strike.cpp: which way it circles the player (1 = right, 0 = left; re-rolled every 120 frames)
                        int bodyRadius;           // enemy_roam_spit.cpp: how far (x 2.5) the other roamers steer clear of this character
                        int rayLength2;           // rolling_tire.cpp: length of ray 2 (straight ahead)
                        int debrisPartnerA;        // dodge_debris.cpp: slot of the first piece this one is linked to (0 = none)
                        int groundHeight;                 // enemy_hop_spit.cpp: height of the ground (or of the pit floor) below it, whole units
                        int actionStep;                        // enemy_spit_charge.cpp: 0 = wanders, 1 = reacts, 2 = approaches, 3 = spits, 4 = charges, 5 = idles, 6 = turns away from a wall
                        int flockRound;                        // enemy_flock_diver.cpp (slot 0x3a): round of the flock, 2 and above = the later attack steps
                        int veerFrames;                                      // enemy_leap_climb.cpp: frames left of a veer (positive = turns one way, negative = the other; counts towards 0)
                        int setupMap;                  // zone_roamer.cpp: map (DAT_10c62ed0) the roamer was set up on
                    };
                    union                         // 0x14
                    {
                        int walkerIdle;           // npc_route_walk.cpp: cleared when the route is read (otherwise unused)
                        int bossRockFrames;         // bs003_update.cpp: frames left of the thrown-rock fall (counts down from 180)
                        int podRipeness;            // enemy_pod_lobber.cpp: frames counted while it idles; raises the chance a pod starts growing
                        int flierGround;          // route_flier.cpp: height of the ground under it in whole units (-5000 = none)
                        int seenHealth;           // enemy_pounce_breath.cpp: the health it last saw (a change wakes it)
                        int circleFrames;          // enemy_chase_lunge.cpp: frames left of circling round an obstacle (30 after a lunge that ended near the player)
                        int rivalState;           // target_race.cpp (rival slots 0x3a, 0x3b): 0 = appear, 1 = run at a target, 2 = look for one, 3 = pounce, 4 = stunned
                        int alarmFlashes;         // char_move.cpp: alarm flashes so far (Npc_RideAlarm)
                        int swing;                // enemy_pendulum.cpp: angle change per frame
                        int reversed;             // route_carrier.cpp, route_follow.cpp: 1 while walking the route
                                                  // backwards
                        int messageDelay;         // tutorial_page_next.cpp (slot 0): frames before the "last page"
                                                  // message may show again
                        int reviveHealth;         // companion_update.cpp (companion slot): health it revives
                                                  // with (0 = its maximum + 2000)
                        int scriptValue;          // char_script.cpp: value carried between steps (opacity in
                                                  // tenths)
                        int chaseFrames;          // char_spirit.cpp: frames spent chasing (swarm every 4th)
                        int hikariReady;          // char_hikari.cpp: 2 once the light was set up on its route
                        int reactDelay;           // npc_lookout.cpp: frames until the event is raised (0 = idle)
                        int watchTarget;          // npc_group_watch.cpp: slot of the groupmate it turns to
                        int cheerPending;         // npc_gesture.cpp: 1 = a cheer starts this frame
                        int pickupBlocked;        // npc_gate_carrier.cpp: 1 = the player is outside the pick-up height band
                        int pitFrames;            // map_cursor_walk.cpp: frames in the current pit phase
                        int greetPending;         // npc_greet.cpp: 1 = greeted, the event is raised once greetings are off
                        int shakeZ;               // npc_link_shock.cpp: shake step of the z axis
                        int ballRollFrame;        // rolling_ball.cpp: frames the ball has rolled (the ball node's frame source)
                  int swingLength;          // swing_platform.cpp: length of the arm in units (distance from the pivot + 50)
                  int hoverLowered;         // npc_hover_carrier.cpp: non-zero = the carrier floats 10 units lower
                        int panicked;             // mole_game.cpp: 1 after a hit (the mole runs faster), 0 once it came up again
                        int zoneValue;             // area_zone.cpp: camera value (types 5 and 8) or 1 = the push also tests the height
                        int shotCount;            // npc_perch.cpp: spits so far in the current round (a mode 3 rider sets its neighbour's to 1)
                        int ageFrames;            // pulse_charger.cpp: frames lived (counted every frame it acts)
                        int stalledFrames;        // enemy_chase_hop.cpp: frames spent not getting closer to the player
                        int hurlReach;            // npc_hurl.cpp: (talk / 100) % 10, throwing range in hundreds of units
                        int shotAligned;           // enemy_hover_shooter.cpp: 1 = the bearing to the player is close enough to fire
                        int stalled;              // breath_enemy.cpp: 1 = the last step was blocked (no ground or a wall ahead)
                        int orbitCentreZ;         // drifter.cpp (state 1): z of the centre the character circles (whole units)
                        int fadeStep;             // enemy_cast_teleport.cpp: step of the teleport fade (15 counts down to 1, then up to 15)
                        int alerted;              // enemy_shy_ghost.cpp: 1 once it has shown itself to the player
                        int rayLength3;           // rolling_tire.cpp: length of ray 3
                        int debrisPartnerB;        // dodge_debris.cpp: slot of the second piece this one is linked to (0 = none)
                        int actionFrames;                      // enemy_spit_charge.cpp: frames counted in the current action step
                        int flockClock;                        // enemy_flock_diver.cpp (slot 0x3b): clock of the formation in thousandths of a radian
                        int walkMode;                  // zone_roamer.cpp: step inside the state: 0 = to the zone it is in, 1 = to the next zone, 2 = at the door, -1 = going straight
                    };
                    union                         // 0x18
                    {
                        int bossBeamSlot;           // bs003_update.cpp: the attachment slot (DAT_10f08880) the laser beam follows, -1 = none
                        int flierReady;           // route_flier.cpp: 1 once its wing node was found
                        int previousState;        // enemy_stalk_claw.cpp: the behaviour step of the last frame (the claw steps 3 and 4 blend from step 3 or 4)
                        int nodesFound;           // enemy_pounce_breath.cpp: 1 once its spine and mouth nodes were looked up
                        int circleDirection;       // enemy_chase_lunge.cpp: frames left of circling one way (positive = left, negative = right)
                        int rivalFrames;          // target_race.cpp (rival): frames in the current state
                        int fall;                 // enemy_pendulum.cpp: drop speed (more negative each frame)
                        int speed;                // route_carrier.cpp: units per frame (from the route's first
                                                  // point)
                        int keepAfterScript;      // char_script.cpp: non-zero = stay in the world after the script
                        int shotSound;            // enemy_strafe.cpp: SHOT_SOUND_PENDING = play the shot sound with
                                                  // the next shot
                        int spiritTimer;          // char_spirit.cpp: frames left of the spirit's visit (counts down)
                        int hikariHold;           // char_hikari.cpp: 15 after the route ended
                        int watchedCode;          // npc_lookout.cpp: item stock digits seen (-1 = none yet)
                        int groupDigit;           // npc_group_watch.cpp: talk % 10, the group it belongs to
                        int cheerReady;           // npc_gesture.cpp: 0 = the cheer is queued instead of played at once
                        int pickupHeight;         // npc_gate_carrier.cpp: lowest player height (below the carrier) for a pick-up
                        int shakeLevel;           // npc_link_shock.cpp: three digits (x, y, z) = how strongly each axis shakes
                  int swingMirror;          // swing_platform.cpp: non-zero = the swing turns the other way round
                        int motionSaved;          // mole_game.cpp: 1 while the motion below is put back after a hit (1 while it dives)
                        int zoneActive;            // area_zone.cpp: 1 while the player is inside and the zone acts
                        int hurlAiming;           // npc_hurl.cpp: 1 while the thrower holds the missile and aims (its charge effect shows)
                        int shotMirrored;          // enemy_hover_shooter.cpp: non-zero = the bearings are mirrored
                        int airborne;             // enemy_chase_hop.cpp: 1 = no ground within 25 units below (the enemy hops or falls), 0 = on the ground
                        int swingStarted;         // enemy_dash_slash.cpp: 1 once the hand swing effect of the current attack was started
                        int castMotion;           // enemy_cast_teleport.cpp: motion to play (1 = back off, 2 = idle, 5 = advance)
                        int hoverBase;            // enemy_hover_strike.cpp: height of the ground below it (or of the player, plus 75, where it has none)
                        int rayLength4;           // rolling_tire.cpp: length of ray 4 (outermost right)
                        int probeFrames;                  // enemy_hop_spit.cpp: frames since the last sight probe (every 10th; 30 puts it on the ground)
                        int healthSeen;                        // enemy_spit_charge.cpp: the health it last saw (a change makes it react; -1 forces a reaction)
                        int blockedLast;               // zone_roamer.cpp: 1 = the last step was blocked (it keeps its heading)
                    };
                    union                         // 0x1c
                    {
                        int standing;             // npc_route_walk.cpp: 1 once it stands at a point, 0 while it walks
                        int flierLastX;           // route_flier.cpp: x of its position last frame (whole units)
                        int heldDown;             // game_frame.cpp (kind 'M', a floor button): non-zero while something stands on it
                        int frameTick;             // enemy_chase_lunge.cpp: frames counted since it was set up (starts at its slot); staggers the probes between enemies
                        int rivalTarget;          // target_race.cpp (rival): target slot (0x3c..0x43) it runs at or pounces on
                        int sightState;           // enemy_cast_teleport.cpp: 0 = player not seen, 1 = seen, 2 = seen in the line of sight
                        int hoverGrounded;        // enemy_hover_strike.cpp: 1 = hoverBase is ground the probe found (it floats 75 to 150 above it), 0 = none
                        int rider;                // route_carrier.cpp, npc_hover_carrier.cpp: character carried (0 = none)
                        int scriptResult;         // char_script.cpp: result of the last step (5 = run the next
                                                  // step at once, 2 = waiting)
                        int forceStep;            // route_step.cpp: non-zero = take the point at once, whether
                                                  // or not the character is near it (cleared by the step)
                        int spiritHaunting;       // char_spirit.cpp: 1 while the visit counts down (150+ frames left)
                        int hikariStopped;        // char_hikari.cpp: 1 = stopped by an obstacle, waits for the way to clear
                        int bestValue;            // npc_lookout.cpp: the largest value seen
                        int sightScale;           // npc_group_watch.cpp: talk / 10, its sight range in units of 20
                        int cheerMotion;          // npc_gesture.cpp: motion of the cheer (0x5a + random 0..3)
                        int linkIndex;            // npc_link_shock.cpp: the link (DAT_1049f0c8) the character stands at, -1 = none
                        int savedLayer;           // mole_game.cpp: motion layer playing at the end of the last frame (-1 while blending)
                        int zoneFrame;             // area_zone.cpp: frame of the zone's cycle
                        int hurlHit;              // npc_hurl.cpp: 1 once the thrower was hit (it stops waiting and acts at once)
                        int motionFrameSaved;      // enemy_hover_shooter.cpp: motion frame kept while it flies (motion 7)
                        int airFrames;            // enemy_chase_hop.cpp: frames airborne (the hop eases with it), 0 once it lands
                        int turnDirection;        // rolling_tire.cpp: 0 = straight on, 1 / 2 = veering one way / the other
                        int obstacleDistance;             // enemy_hop_spit.cpp: distance to the obstacle the last probe found ahead (-1 = none)
                        int turnsRight;                        // enemy_spit_charge.cpp: 1 = it turns right at a wall or ledge, 0 = left (picked at random)
                        int sweepSkipped;                                    // enemy_leap_climb.cpp: 1 = the spit sweep of state 7 is skipped once (cleared when it was skipped)
                        int crowdFrames;               // zone_roamer.cpp: frames the character stands blocked (9 once set up)
                    };
                    union                         // 0x20
                    {
                        int promptTimer;          // npc_route_walk.cpp: frames since the last key prompt (0 = none pending, counts up from 1)
                        int flierLastY;           // route_flier.cpp: y of its position last frame (whole units)
                        int halted;                // enemy_chase_lunge.cpp: 1 = it stopped chasing (blocked or at a ledge) and re-tests the way every 60th frame
                        int rivalStuckFrames;     // target_race.cpp (rival): frames its way was blocked while running
                        int pointPause;           // route_carrier.cpp: frames to wait at a route point
                        int hopRequests;          // enemy_strafe.cpp (Enemy_HopAndStrafe): frames counted
                                                  // towards the next hop
                        int bestAmount;           // npc_lookout.cpp: the amount that came with it
                        int routeMode;            // spirit_route.cpp: 1..3 = put the spirit on a fixed route point
                        int gesturing;            // npc_gesture.cpp: 1 while the gesture shows (neighbours react to it)
                        int hikariSound;          // char_hikari.cpp: 1 while its "anime" sound plays
                        int releaseDistance;      // npc_link_shock.cpp: player distance (talk / 10 * 10) below which it is released
                  int swingGround;          // swing_platform.cpp: highest ground under the swing so far (-3500 = none)
                  int playerGround;         // npc_hover_carrier.cpp: height of the ground under the player
                        int savedFrame;           // mole_game.cpp: motion frame at the end of the last frame
                        int zonePeriod;            // area_zone.cpp: frames in the cycle
                        int floorHeight;           // enemy_hover_shooter.cpp: height of the ground under the spawn point
                        int hurlWound;            // npc_hurl.cpp: 1 once the wind-up motion reached its held pose (0x1b)
                        int specialTarget;         // partner_skill.cpp: slot of the enemy aimed at (slot 16) or grabbed (slot 7)
                        int specialFrames;         // partner_skill.cpp (slot 11): frames counted since its skill started
                        int hurtFrames;           // enemy_shy_ghost.cpp: frames since it was hit (counted up to 46, then it sinks away; counted down while not hit)
                        int driveFrames;          // rolling_tire.cpp: frames driven in the current heading (negative after a bounce)
                        int obstacleAhead;                // enemy_hop_spit.cpp: 1 when the last probe found an obstacle ahead
                        int alertFrames;                       // enemy_spit_charge.cpp: frames it stays alert after seeing the player (600, counted down)
                        int dustDone;                                        // enemy_leap_climb.cpp: 1 once the dust effect of the landing was started
                        int talkWait;                  // zone_roamer.cpp: frames left of a voice motion, -1 = start one
                    };
                    union                         // 0x24
                    {
                        int walkStyle;            // npc_route_walk.cpp: route record's last digit: 0 = walks a route, else stands
                        int flierLastZ;           // route_flier.cpp: z of its position last frame (whole units)
                        int hiddenFrames;          // enemy_chase_lunge.cpp: frames it is held out of play (colours blanked, no shadow), counted down
                        int rivalSkipTarget;      // target_race.cpp (rival): the target it gave up on (not picked again at once)
                        int m_0024;
                        int stepCount;            // enemy_wisp_caster.cpp: dashes in a row (state 2), frames of the charge (state 5), puffs (state 3)
                        int zoneActiveFrom;        // area_zone.cpp: first frame of the cycle in which the zone acts
                        int sinkLimit;            // map_cursor_walk.cpp: set to -40 while the character is not sinking
                        int restFrames;           // enemy_strafe.cpp (Enemy_HopAndStrafe): frames standing still
                                                  // in the idle motion
                        int hurlCarryHeight;      // npc_hurl.cpp: height the carried character is held at (set by the carrier)
                        int shotDelayMax;          // enemy_hover_shooter.cpp: frames after which a shot is certain (150)
                        int sparkCooldown;        // npc_tricks.cpp: frames before the next spark effect (counted down from 10)
                        int specialPhase;          // partner_skill.cpp: step of the partner skill (0 = ready, 1 = starts, then counts up; 1000 and above = the long one of slot 7)
                        int bumpCooldown;         // rolling_tire.cpp: frames left (10 after a bounce) in which it may not pick a new veer
                        int shadowHeight;                 // enemy_hop_spit.cpp: height of its shadow, kept between the ground lookups
                        int attackActive;                      // enemy_spit_charge.cpp: 1 while the spit or charge motion overrides the gait motion
                        int floorKnown;                                      // enemy_leap_climb.cpp: 1 once the height of the floor below it was found
                        int nearFrames;                // zone_roamer.cpp: frames spent hardly moving
                    };
                    union                         // 0x28
                    {
                        int sprayFrames;          // enemy_wisp_caster.cpp: frames of the dash spray (state 2) and of the blast (state 5)
                        int flierStillFrames;     // route_flier.cpp: frames it moved less than 10 units (it vanishes after 30)
                        int sightFrames;          // enemy_pounce_breath.cpp: 300 while the way to the player is free, counted down
                        int walkerSetUp;           // enemy_chase_lunge.cpp: -1 once its head and spine nodes were looked up
                        int fadeActions;          // route_carrier.cpp: bit n set = fade out at a point with
                                                  // action n
                        int skipGroundProbe;      // non-zero = skip the ground probe of a shot (0x1001a590)
                        int hopBack;              // enemy_strafe.cpp (Enemy_HopAndStrafe): 1 = the next hop
                                                  // ends the strafe
                        int fadeOut;              // enemy_busy.cpp: 0 = stays, 1.. = frames of fading out as it
                                                  // leaves the queue (QUEUE_FADE_FRAMES = gone)
                        int groundTimer;          // hover_watcher.cpp: frames counted; the shadow is put on the
                                                  // ground when it matches the slot (mod 30)
                        int headingFrom;          // route_step.cpp: heading (degrees) at the route's first point
                        int markerSlot;           // marker_rider.cpp: slot of the marker character ridden
                        int lastHealth;           // map_cursor_walk.cpp: the health of the slot, copied every frame
                        int zoneActiveTo;          // area_zone.cpp: last frame of the cycle in which the zone acts
                  int swingMaxAngle;        // swing_platform.cpp: largest swing angle seen over ground
                        int hurlCarried;          // npc_hurl.cpp: non-zero = the thrower is carried (moved to the node [8] position)
                        int shotDelayMin;          // enemy_hover_shooter.cpp: frames before a shot may come (30)
                        int retreatFrames;        // breath_enemy.cpp: frames left of a failed retreat (state 5 ends when it runs out)
                        int wallAhead;            // rolling_tire.cpp: 1 = the 500-unit look-ahead hit the world
                        int attackCooldown;               // enemy_hop_spit.cpp: frames before it may start to spit again
                        int gaitMotion;                        // enemy_spit_charge.cpp: motion picked by the gait state (1 = starting, 2 = striding, 3 = ending, 0 = by action)
                        int floorHeight;                                     // enemy_leap_climb.cpp: height of the floor below it (whole units)
                        int setupStep;                 // zone_roamer.cpp: 0, 1 while it sets itself up, 2 once set up
                    };
                    union                         // 0x2c
                    {
                        int fixedFacing;          // npc_route_walk.cpp: facing in quarter turns (0 = faces the player)
                        int m_002c;
                        int probeDelay;          // enemy_pounce_breath.cpp: frames to the next sight probe (10 after one)
                        int farewellFrames;       // partner_follow.cpp: frames left of the farewell message (60 at the start; it leaves at 1)
                        int dazeFrames;            // enemy_chase_lunge.cpp: frames it stands dazed facing the player (sparkle effect every second frame), counted down
                        int savedEffectParam;     // enemy_wisp_caster.cpp: CharSlot::stats[STAT_EFFECT_PARAM] when it was set up
                        int cursorNode;           // map_cursor_walk.cpp: the route node the character heads for
                        int headingTo;            // route_step.cpp: heading (degrees) at the route's last point
                        int vanishFrames;         // marker_rider.cpp: frames left of the vanishing once the marker fell
                        int hopFrame;             // enemy_strafe.cpp (Enemy_HopAndStrafe): frame of the hop
                                                  // under way (0 = not hopping)
                  int swingMinAngle;        // swing_platform.cpp: smallest swing angle seen over ground
                        int shotTimer;             // enemy_hover_shooter.cpp: frames since the last shot
                        int zoneEffect;            // area_zone.cpp: effect started at one frame of the cycle (0 = none)
                        int specialMark;           // partner_skill.cpp: set to 75 (0x4b) when the skill of slot 6 or 7 starts
                        int rayPlayerGap0;        // rolling_tire.cpp: distance of the player to ray 0 (whole units)
                        int approachFrames;                    // enemy_spit_charge.cpp: frames spent approaching the player (action step 2)
                        int turningAround;                                   // enemy_leap_climb.cpp: 1 while it turns round after a blocked leap (the sight check waits)
                        int previousZone;              // zone_roamer.cpp: zone it came from (script flag base + 2)
                    };
                    union                         // 0x30
                    {
                        int firstPoint;           // npc_route_walk.cpp: the route point it started at (Map_RoutePosition measures from it)
                        int podNodesFound;          // enemy_pod_lobber.cpp: 1 once the pod nodes of the model were looked up
                        int m_0030;
                        int probeResult;         // enemy_pounce_breath.cpp: result of the last sight probe (0 free, 2 = a step with a height)
                        int colourStep;            // enemy_chase_lunge.cpp: 0 = normal, 1 = colours blanked while hidden, 2 = identification colours restored
                        int wispsActive;          // enemy_wisp_caster.cpp: 1 while one of its two wisps burns
                        int zoneEffectFrame;       // area_zone.cpp: the frame of the cycle that starts the effect
                        int markerPhase;          // marker_rider.cpp: 0 = new, 1 = riding (flags set), 2 = vanishing
                        int arrivalHold;          // map_cursor.cpp: frames left in which the cursor counts as
                        int frozenFrames;          // enemy_hover_shooter.cpp: frames its course stays frozen after a shot
                        int hikariScanned;        // char_hikari.cpp: 1 once the character to carry along was looked up
                                                  // having been carried by a rider (15 after one lands on it)
                        int specialPause;          // partner_skill.cpp (slot 7): frames to wait before it picks an enemy
                        int rayPlayerGap1;        // rolling_tire.cpp: the same for ray 1
                        int retaliates;                        // enemy_spit_charge.cpp: 1 when its kind spits back (and so reacts to a hit with the spit step)
                        int hitReacted;                                      // enemy_leap_climb.cpp: 1 after the variant that reacts to hits was hit
                        int gatesOpen;                 // zone_roamer.cpp: 1 while the doors of the map are open for a talk
                    };
                    union                         // 0x34
                    {
                        int promptKind;           // npc_route_walk.cpp: key prompt shown now and then (0..2; the record's last digit - 1)
                        int podPiecesLive;          // enemy_pod_lobber.cpp: lobbed pods still flying this frame
                        int m_0034;
                        int probeHeight;         // enemy_pounce_breath.cpp: height the last sight probe found (whole units)
                        int shotRate;              // enemy_chase_lunge.cpp: 0 = never spits, 1..7 = it spits every (8 - this) * 10 frames
                        int zoneSound;             // area_zone.cpp: sound played with the effect (0 = none)
                        int cursorKind;           // map_cursor_walk.cpp: kind of the node last reached (MapCursor_Step)
                        int arrivalAction;        // npc_patrol.cpp: action of the last route point reached
                        int hopHistory;           // enemy_strafe.cpp (Enemy_HopAndStrafe): HopHistory state of
                        int hikariFollower;       // char_hikari.cpp: slot of the character carried along with the light (0 = none)
                        int hoverHeight;           // enemy_hover_shooter.cpp: height it hovers at (-200000 = not set yet)
                        int gateStarted;          // npc_gate_carrier.cpp: 2 once the carrier was set up on its route
                                                  // the position history in LastPosition
                        int specialResume;         // partner_skill.cpp (slot 7): the step to continue from after the long skill
                        int rayPlayerGap2;        // rolling_tire.cpp: the same for ray 2
                        int savedMotion;          // enemy_wisp_caster.cpp: motion layer playing at the end of the last frame (restored after a hit)
                        int modeLocked;                        // enemy_spit_charge.cpp: 1 = the model stays in mode 1 (set once it hops over a ledge)
                    };
                };
                int values[13];                   // the same variables by index (enemy_ai.cpp, enemy_moves.cpp,
                                                  // ai_pick.cpp and friends use values[0..5])
            };
            union                                 // 0x38
            {
                int bossBlastTimer;               // bs003_update.cpp: frame counter of the burst effects in the later fight phases (restarts at 300)
                int sawPlayer;                    // 1 once the character saw the player (or was told to react)
                int leapAngle;           // enemy_pounce_breath.cpp: start heading of the whirl in 1/100000 radians
                int volleyTimer;           // enemy_chase_lunge.cpp: frames since its last spit
                int zoneSoundDelay;               // area_zone.cpp: frames before that sound plays (0 = at once)
                int cursorPrevious;               // map_cursor_walk.cpp: the node reached before (MapCursor_Step)
                int releasable;                   // npc_approach.cpp: non-zero = the release flag ends the approach
                int buttonLiftOff;                // hikari_button.cpp: non-zero = the button light is switched off (blue, held still)
                int rayPlayerGap3;                // rolling_tire.cpp: the same for ray 3 (overlays sawPlayer)
                int groundRetries;                // enemy_hop_spit.cpp: times it was lifted because no ground was found below it (stops at 21)
                int ledgeResult;                       // enemy_spit_charge.cpp: result of the last ledge probe (0 free, 2 = a ledge with a height)
                int savedMotionFrame;     // enemy_wisp_caster.cpp: frame of that motion (-1 while it was blending)
                int headingHold;               // zone_roamer.cpp: frames the turn target is held at the saved heading
            };
            union                                 // 0x3c
            {
                int behaviour;                    // behaviour number (0 = not unpacked from the script parameter yet);
                                                  // setting it switches behaviour
                int zoneTracking;                 // area_zone.cpp (type 8): 1 once the follow height was taken
                int rayPlayerGap4;                // rolling_tire.cpp: the same for ray 4 (overlays behaviour)
                int turnRate;                    // enemy_pounce_breath.cpp: heading change of the whirl in 1/100000 radians (a sign flag while it wanders; overlays behaviour)
                int groundSetUp;                  // enemy_hop_spit.cpp: 1 once its first ground lookup is done (overlays behaviour; kind 4 is picked by the slot kind)
                int shielded;             // enemy_wisp_caster.cpp: 1 while the state is not interrupted by a hit (cleared every frame)
                int ledgeHeight;                       // enemy_spit_charge.cpp: height the last ledge probe found (whole units)
            };
        };
        struct
        {
            int laserSetup;                   // laser_zone.cpp: 0 = beam nodes not looked up yet, 1 afterwards
            int laserFrames;                  // laser_zone.cpp: frames the zone has run
            int laserTimer;                   // laser_zone.cpp: frames since the last beam pair was started
            int laserHits;                    // laser_zone.cpp: beam pairs that ended while the player stood in the box
            int laserPattern[12];             // laser_zone.cpp: per beam pair: 0 = free, else flag * 10000 + speed * 100 + type
        };
        SoccerState soccer;                       // soccer_ai.cpp: a football CPU player
        Bs001State bs001;                         // bs001_update.cpp: the BS_001 boss
        Bs001Aux bs001Aux;                        // bs001_update.cpp: the slot after the boss
        Bs001Fade bs001Fade;                      // bs001_update.cpp: two slots after the boss
        Bs005State bs005;                         // bs005_update.cpp: the stone golem boss
        Bs004State bs004;                         // bs004_update.cpp: the moss root boss
        Kind43State kind43;                       // prince_update.cpp: the prince
        Bs002State bs002;                         // bs002_update.cpp: the BS_002 boss
        Bs006State bs006;                         // bs006_update.cpp: the BS_006 boss
        Bs006Piece bs006Piece;                    // bs006_update.cpp: a piece slot of the BS_006 boss
        int words[0x10];                          // the whole block by index (resets and copies)
        Shove shoves[4];                          // char_shove.cpp: the slot's scripted shoves
    };
};

// The behaviour state of every character slot.
extern "C" CharState DAT_1020b430[CHAR_SLOT_COUNT];

#endif
