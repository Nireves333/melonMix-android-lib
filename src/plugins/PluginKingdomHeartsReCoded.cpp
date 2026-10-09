#include "PluginKingdomHeartsReCoded.h"
#include "KingdomHeartsHDCollection.h"
#include <cmath>

namespace Plugins
{

u32 PluginKingdomHeartsReCoded::usGamecode = 1161382722;
u32 PluginKingdomHeartsReCoded::euGamecode = 1345932098;
u32 PluginKingdomHeartsReCoded::jpGamecode = 1245268802;

#define ASPECT_RATIO_ADDRESS_US 0x0202A810
#define ASPECT_RATIO_ADDRESS_EU 0x0202A824
#define ASPECT_RATIO_ADDRESS_JP 0x0202A728

// The bottom screen's TREE/MAP tab (right edge of the map panel). Touch target and the
// tab-label probe pixels were measured on-device (2026-10-01): tapping (225,150) flips
// the view both ways; at y=148 the label glyphs light x=214 when the tab reads "MAP"
// (tree view showing) and x=230 when it reads "TREE" (map view showing).
#define TREE_TAB_TOUCH_X 225
#define TREE_TAB_TOUCH_Y 150
#define TREE_TAB_LABEL_PROBE_Y 148
#define TREE_TAB_MAP_LABEL_PROBE_X 214
#define TREE_TAB_TREE_LABEL_PROBE_X 230
#define TREE_TAB_LABEL_COLOR 0x103e3e3e

// Enlarged tree view footprint (raw shape units, like the fullscreen map's 232x116 at
// 1.5): 256x192 at 1.15 is about the enlarged map's area, a touch taller. Plain
// centered (no upward lift like the map: the tree's header tabs would slide under the
// top soft-button strip). Tuning knobs — the frontend touch box follows automatically
// via getTreeViewRect().
#define TREE_ENLARGED_SCALE 1.15f
#define TREE_ENLARGED_MARGIN_BOTTOM 0.0f
// Grow/shrink animation length, matching the fullscreen map's transition walk
#define TREE_TRANSITION_DURATION 20
// Swap handoff morph length (bottom-screen box <-> map view reframe)
#define SWAP_MORPH_DURATION 10

// 0x0d9eaa00 => main menu
// 0x0da75400 => config
// 0x00149600 => debug reports
// 0x0014c800 => debug reports - trophies
// 0x00150800 => debug reports - collection
// 0x0e098800 => debug reports - story
// 0x0df58400 => debug reports - enemy profiles
// 0x00158200 => debug reports - character files
// 0x0da15000 => quest list
// 0x0da82800 => tutorials
// 0x0d8ba800 => save menu
#define MAIN_MENU_SCREEN_2_US 0x02055c20
#define MAIN_MENU_SCREEN_2_EU 0x02055c20
#define MAIN_MENU_SCREEN_2_JP 0x02055a40

// 0x6780 => main menu
// 0x0040 => stat matrix
// 0x0380 => command matrix
// 0x5f40 => gear matrix
#define MAIN_MENU_SCREEN_1_US 0x02055cac // 0x0205a8c8
#define MAIN_MENU_SCREEN_1_EU 0x02055cac
#define MAIN_MENU_SCREEN_1_JP 0x02055acc

// 0x0540 => save menu
#define MAIN_MENU_SCREEN_3_US 0x02055e20
#define MAIN_MENU_SCREEN_3_EU 0x02055e20
#define MAIN_MENU_SCREEN_3_JP 0x02055c40

#define STATIC_CUTSCENE_ADDRESS_US 0x0205d97c
#define STATIC_CUTSCENE_ADDRESS_EU 0x0205d97c
#define STATIC_CUTSCENE_ADDRESS_JP 0x0205d99c

// 0x00 => intro and main menu
#define IS_MAIN_MENU_US 0x02060c94
#define IS_MAIN_MENU_EU 0x020608d0
#define IS_MAIN_MENU_JP 0x02060aa0

#define IS_MAIN_MENU_VALUE_US 0x00
#define IS_MAIN_MENU_VALUE_EU 0xff
#define IS_MAIN_MENU_VALUE_JP 0x00

// Alternatives to IS_MAIN_MENU_EU:
// 0x020608cc is 0x00000000 / 0x00000008
// 0x020608d0 is 0xffffffff / 0x00000000
// 0x020608d4 is 0x00000000 / 0x00030000
// 0x020608d8 is 0x00000000 / 0x00000009
// 0x02060b64 is 0x00000000 / 0x00001000
// 0x02060b68 is 0x00000000 / 0x00001000
// 0x02060cac is 0x00000000 / 0x00001000
// 0x02060df0 is 0x00000000 / 0x00001000

#define PAUSE_SCREEN_ADDRESS_US 0x020569d0
#define PAUSE_SCREEN_ADDRESS_EU 0x020569d0
#define PAUSE_SCREEN_ADDRESS_JP 0x020567f0
#define PAUSE_SCREEN_VALUE_TRUE_PAUSE 0x01

// 0x03 => cutscene; 0x01 => not cutscene
#define IS_CUTSCENE_US 0x02056e90
#define IS_CUTSCENE_EU 0x02056e90
#define IS_CUTSCENE_JP 0x02056cb0

#define IS_LOAD_SCREEN_US 0x02056f4c // may also be 0x02056f48, 0x02056f50, 0x02056f58 or 0x02056f5c
#define IS_LOAD_SCREEN_EU 0x0205ffd0
#define IS_LOAD_SCREEN_JP 0x02056d6c

#define IS_LOAD_SCREEN_VALUE_US 0x10
#define IS_LOAD_SCREEN_VALUE_EU 0x00010004
#define IS_LOAD_SCREEN_VALUE_JP 0x10

// 0x01 => cutscene with skip button
// 0x03 => regular cutscene
// 0x08 => cutscene with static images (can cause false positives outside of cutscenes)
// 0x10 => other stuff
#define CUTSCENE_STATE_ADDRESS_US 0x02056f4a
#define CUTSCENE_STATE_ADDRESS_EU 0x02056f4a
#define CUTSCENE_STATE_ADDRESS_JP 0x02056d6a

// 0x00 => nothing on screen, debug reports trophies
// 0x01 => debug reports enemy profiles
// 0x03 => intro, title screen, debug reports collection/story
// 0x07 => cutscene with static images
// 0x09 => debug reports character profiles
// 0x0b => world selection
// 0x0c => load screen, main menu, alt main menu
// 0x0f => in-game
#define GAME_STATE_ADDRESS_US 0x02056f50
#define GAME_STATE_ADDRESS_EU 0x02056f50
#define GAME_STATE_ADDRESS_JP 0x02056d70

// 0x00 => death screen
#define DEATH_SCREEN_ADDRESS_US 0x02056f5c
#define DEATH_SCREEN_ADDRESS_EU 0x02056f5c // TODO: KH probably correct, but didn't check
#define DEATH_SCREEN_ADDRESS_JP 0x02056d7c // TODO: KH probably correct, but didn't check

// 0x04 => playable (example: ingame); 0x03 => world selection; 0x02 => not playable (menus)
#define IS_PLAYABLE_AREA_US 0x0205a8c0
#define IS_PLAYABLE_AREA_EU 0x0205a8c0
#define IS_PLAYABLE_AREA_JP 0x0205a6e0

#define RESULT_SCREEN_ID_US 0x02060434
#define RESULT_SCREEN_ID_EU 0x02060434
#define RESULT_SCREEN_ID_JP 0x02060254 // TODO: KH probably correct, but didn't check

#define BUG_SECTOR_IDENTIFIER_ADDRESS_US 0x0206083d
#define BUG_SECTOR_IDENTIFIER_ADDRESS_EU 0x0206083d
#define BUG_SECTOR_IDENTIFIER_ADDRESS_JP 0x0206065d // TODO: KH probably correct, but didn't check

#define BUG_SECTOR_IDENTIFIER_VALUE_US 0x48
#define BUG_SECTOR_IDENTIFIER_VALUE_EU 0x48
#define BUG_SECTOR_IDENTIFIER_VALUE_JP 0x48 // TODO: KH probably correct, but didn't check

#define FLOOR_LEVEL_ADDRESS_US 0x02060867
#define FLOOR_LEVEL_ADDRESS_EU 0x02060867
#define FLOOR_LEVEL_ADDRESS_JP 0x02060687 // TODO: KH

#define TYPE_OF_BATTLE_ADDRESS_US 0x020b5608 // or 0x020b5620
#define TYPE_OF_BATTLE_ADDRESS_EU 0x020b5628 // TODO: KH unconfirmed, but probable
#define TYPE_OF_BATTLE_ADDRESS_JP 0x020b5608 // TODO: KH wrong

#define CUTSCENE_ADDRESS_US 0x020b7db8
#define CUTSCENE_ADDRESS_EU 0x020b7dd8
#define CUTSCENE_ADDRESS_JP 0x020b7858

#define INGAME_MENU_COMMAND_LIST_SETTING_ADDRESS_US 0x02198310
#define INGAME_MENU_COMMAND_LIST_SETTING_ADDRESS_EU 0x021991b0
#define INGAME_MENU_COMMAND_LIST_SETTING_ADDRESS_JP 0x02198310 // TODO: KH wrong

#define INGAME_MENU_COMMAND_LIST_SETTING_VALUE_US 0x200
#define INGAME_MENU_COMMAND_LIST_SETTING_VALUE_EU 0x200
#define INGAME_MENU_COMMAND_LIST_SETTING_VALUE_JP 0x002 // TODO: KH probably wrong

#define DIALOG_SCREEN_ADDRESS_US 0x0219e9a8 // may also be 0x02060390
#define DIALOG_SCREEN_ADDRESS_EU 0x0219e9c8
#define DIALOG_SCREEN_ADDRESS_JP 0x0219e9a8 // TODO: KH wrong

#define DIALOG_SCREEN_VALUE_US 0x00000000
#define DIALOG_SCREEN_VALUE_EU 0x00000000
#define DIALOG_SCREEN_VALUE_JP 0x00000000 // TODO: KH probably correct, but didn't check

#define SONG_ID_ADDRESS_US      0x02192012
#define SONG_ID_ADDRESS_EU      0x02192EB2
#define SONG_ID_ADDRESS_JP      0x02190CB2

#define SSEQ_TABLE_ADDRESS_US      0x020E1E70
#define SSEQ_TABLE_ADDRESS_EU      0x020E2D10
#define SSEQ_TABLE_ADDRESS_JP      0x020E0B10

#define STRM_ADDRESS_US      0x0205E790
#define STRM_ADDRESS_EU      0x0205E790
#define STRM_ADDRESS_JP      0x0205E5B0

#define SWITCH_TARGET_PRESS_FRAME_LIMIT   100
#define SWITCH_TARGET_TIME_BETWEEN_SWITCH 20
#define LOCK_ON_PRESS_FRAME_LIMIT         100

// If you want to undertand that, check GPU2D_Soft.cpp, at the bottom of the SoftRenderer::DrawScanline function
#define PARSE_BRIGHTNESS_FOR_WHITE_BACKGROUND(b) (b & (1 << 15) ? (0xF - ((b - 1) & 0xF)) : 0xF)
#define PARSE_BRIGHTNESS_FOR_BLACK_BACKGROUND(b) (b & (1 << 14) ? ((b - 1) & 0xF) : 0)
#define PARSE_BRIGHTNESS_FOR_UNKNOWN_BACKGROUND(b) (b & (1 << 14) ? ((b - 1) & 0xF) : (b & (1 << 15) ? (0xF - ((b - 1) & 0xF)) : 0))

enum
{
    gameScene_Intro,                    // 0
    gameScene_TitleScreen,              // 1
    gameScene_IntroLoadMenu,            // 2
    gameScene_Cutscene,                 // 3
    gameScene_CutsceneWithStaticImages, // 4
    gameScene_InGameWithMap,            // 5
    gameScene_InGameDialog,             // 6
    gameScene_InGameOlympusBattle,      // 7
    gameScene_InGameMenu,               // 8
    gameScene_InGameSaveMenu,           // 9
    gameScene_ResultScreen,             // 10
    gameScene_WorldSelection,           // 11
    gameScene_PauseMenu,                // 12
    gameScene_Tutorial,                 // 13
    gameScene_Shop,                     // 14
    gameScene_LoadingScreen,            // 15
    gameScene_DeathScreen,              // 16
    gameScene_TheEnd,                   // 17
    gameScene_Other2D,                  // 18
    gameScene_Other                     // 19
};

enum
{
    gameSceneState_showHud,
    gameSceneState_dialogVisible,
    gameSceneState_textOverScreen,
    gameSceneState_showRegularPlayerHealth,
    gameSceneState_showOlympusBattlePlayerHealth,
    gameSceneState_showMinimap,
    gameSceneState_showFullscreenMap,
    gameSceneState_showCommandMenu,
    gameSceneState_showNextAreaName,
    gameSceneState_showFloorCounter,
    gameSceneState_showEnemiesCounter,
    gameSceneState_bugLevelVisible,
    gameSceneState_topScreenMissionInformationVisible,
    gameSceneState_showBottomScreenMissionInformation,
    gameSceneState_showChallengeMeter,
    gameSceneState_bottomScreenCutscene,
    gameSceneState_topScreenCutscene,
    gameSceneState_deweyDialogVisible,
    gameSceneState_comboLimitVisible,
    gameSceneState_speedComboFinisherVisible,
    gameSceneState_starRaveFinisherVisible,
    gameSceneState_spinnerSawFinisherVisible,
    gameSceneState_showFullscreenTree,
    gameSceneState_showSmallTree
};

enum
{
    HK_LSwitchTarget,
    HK_RSwitchTarget,
    HK_RLockOn,
    HK_CommandMenuLeft,
    HK_CommandMenuRight,
    HK_CommandMenuUp,
    HK_CommandMenuDown,
    HK_HUDToggle,
    HK_FullscreenMapToggle,
    HK_ReplacementTexturesToggle,
    HK_OpenSettings,
    HK_ViewEnlargeToggle
};

PluginKingdomHeartsReCoded::PluginKingdomHeartsReCoded(u32 gameCode)
{
    GameCode = gameCode;

    hudToggle();

    priorMap = -1;
    Map = 0;

    // game scene detection utils
    _muchOlderHad3DOnTopScreen = false;
    _muchOlderHad3DOnBottomScreen = false;
    _olderHad3DOnTopScreen = false;
    _olderHad3DOnBottomScreen = false;
    _had3DOnTopScreen = false;
    _had3DOnBottomScreen = false;

    // apply addon to input mask utils
    for (int i = 0; i < PRIOR_ADDON_MASK_SIZE; i++) {
        PriorAddonMask[i] = 0;
    }
    LastSwitchTargetPress = SWITCH_TARGET_PRESS_FRAME_LIMIT;
    LastLockOnPress = LOCK_ON_PRESS_FRAME_LIMIT;

    customKeyMappingNames = {
        "HK_LSwitchTarget",
        "HK_RSwitchTarget",
        "HK_RLockOn",
        "HK_CommandMenuLeft",
        "HK_CommandMenuRight",
        "HK_CommandMenuUp",
        "HK_CommandMenuDown",
        "HK_HUDToggle",
        "HK_FullscreenMapToggle",
        "HK_ReplacementTexturesToggle",
        "HK_OpenSettings",
        "HK_ViewEnlargeToggle"
    };
    customKeyMappingLabels = {
        "Switch Target - Left",
        "Switch Target - Right",
        "Lock On",
        "Command Menu - Back",
        "Command Menu - Select",
        "Command Menu - Up",
        "Command Menu - Down",
        "HUD Toggle",
        "Fullscreen Map Toggle",
        "Toggle Replacement Textures",
        "Open Settings",
        "Enlarge Map/Tree View"
    };

    Cutscenes = std::array<Plugins::CutsceneEntry, 15> {{
        {"OP",     "hd501",         "501_opening_part_1",         0x04bb3a00, 0x04c1be00, 0x04b04200, 2+1},
        {"Secret", "hd593",         "593_",                       0x05e9b400, 0x05f03800, 0x05db0200, 2},
        {"w1_ED",  "hd510_PLUS_mm", "510_unaccountable_accounts", 0x06784800, 0x067ecc00, 0x06699e00, 2},
        {"w1_OP",  "hd502",         "502_opening_part_2",         0x06e43800, 0x06eabc00, 0x06d58e00, 4+2},
        {"w2_ED",  "hd516_PLUS_mm", "516_",                       0x07c4ee00, 0x07cb7200, 0x07b64400, 2},
        {"w2_OP",  "hd512_PLUS_mm", "512_",                       0x08548600, 0x085b0a00, 0x0845f800, 2},
        {"w3_ED",  "hd524",         "524_",                       0x08706200, 0x0876e600, 0x0861D400, 2},
        {"w4_ED",  "hd531",         "531_",                       0x09503000, 0x0956b400, 0x0941a200, 2},
        {"w5_ED",  "hd539_PLUS_mm", "539_",                       0x09990800, 0x099f8c00, 0x098a7a00, 2},
        {"w6_ED",  "hd549",         "549_",                       0x0a3c9400, 0x0a431800, 0x0a2e0600, 2},
        {"w7_ED",  "hd572",         "572_",                       0x0ac3aa00, 0x0aca2e00, 0x0ab51c00, 2},
        {"w8_ED1", "hd590_PLUS_mm", "590_",                       0x0b150400, 0x0b1b8800, 0x0b067600, 2},
        {"w8_ED2", "hd592",         "592_",                       0x0bcd3400, 0x0bd3b800, 0x0bbea600, 2},
        {"w8_ED3", "hd576_PLUS_mm", "576_",                       0x0c216800, 0x0c27ec00, 0x0c12cc00, 2},
        {"w8_OP",  "hd573_PLUS_mm", "573_",                       0x0c426000, 0x0c48e400, 0x0c33c400, 2},
    }};

    Dialogues = std::array<Plugins::CutsceneEntry, 74> {{
        {"503" ,   "hd503_PLUS",    "503_dive_to_the_heart_part_1", 0x07000000, 0x00000000, 0x00000000, 8},
        {"505" ,   "hd505",         "505_dive_to_the_heart_part_2", 0x07000000, 0x00000000, 0x00000000, 8},
        {"506" ,   "hd506",         "506_destiny_islands_part_1",   0x07000000, 0x00000000, 0x00000000, 8},
        {"507" ,   "hd507",         "507_destiny_islands_part_2",   0x07000000, 0x00000000, 0x00000000, 8},
        {"508" ,   "hd508",         "508_destiny_islands_part_3",   0x07000000, 0x00000000, 0x00000000, 8},
        {"509" ,   "hd509",         "509_destiny_islands_part_4",   0x07000000, 0x00000000, 0x00000000, 8},
        {"510" ,   "hd510_PLUS",    "510_destiny_islands_part_5",   0x07000000, 0x00000000, 0x00000000, 8},
        {"512" ,   "hd512_PLUS",    "512_traverse_town_part_1",     0x07000000, 0x00000000, 0x00000000, 8},
        {"514" ,   "hd514",         "514_traverse_town_part_2",     0x07000000, 0x00000000, 0x00000000, 8},
        {"515" ,   "hd515",         "515_traverse_town_part_3",     0x07000000, 0x00000000, 0x00000000, 8},
        {"516" ,   "hd516_PLUS",    "516_traverse_town_part_4",     0x07000000, 0x00000000, 0x00000000, 8},
        {"518" ,   "hd518",         "518_wonderland_part_1",        0x07000000, 0x00000000, 0x00000000, 8},
        {"519" ,   "hd519",         "519_wonderland_part_2",        0x07000000, 0x00000000, 0x00000000, 8},
        {"520" ,   "hd520",         "520_wonderland_part_3",        0x07000000, 0x00000000, 0x00000000, 8},
        {"521" ,   "hd521",         "521_wonderland_part_4",        0x07000000, 0x00000000, 0x00000000, 8},
        {"522" ,   "hd522_PLUS",    "522_wonderland_part_5",        0x07000000, 0x00000000, 0x00000000, 8},
        {"524" ,   "hd524",         "524_wonderland_part_6",        0x07000000, 0x00000000, 0x00000000, 8},
        {"525" ,   "hd525",         "525_olympus_part_1",           0x07000000, 0x00000000, 0x00000000, 8},
        {"528" ,   "hd528",         "528_olympus_part_2",           0x07000000, 0x00000000, 0x00000000, 8},
        {"530" ,   "hd530",         "530_olympus_part_3",           0x07000000, 0x00000000, 0x00000000, 8},
        {"531" ,   "hd531",         "531_olympus_part_4",           0x07000000, 0x00000000, 0x00000000, 8},
        {"532" ,   "hd532",         "532_agrabah_part_1",           0x07000000, 0x00000000, 0x00000000, 8},
        {"535" ,   "hd535",         "535_agrabah_part_2",           0x07000000, 0x00000000, 0x00000000, 8},
        {"536" ,   "hd536",         "536_agrabah_part_3",           0x07000000, 0x00000000, 0x00000000, 8},
        {"538" ,   "hd538",         "538_agrabah_part_4",           0x07000000, 0x00000000, 0x00000000, 8},
        {"539" ,   "hd539_PLUS",    "539_agrabah_part_5",           0x07000000, 0x00000000, 0x00000000, 8},
        {"542" ,   "hd542",         "542_hollow_bastion_part_1",    0x07000000, 0x00000000, 0x00000000, 8},
        {"543" ,   "hd543",         "543_hollow_bastion_part_2",    0x07000000, 0x00000000, 0x00000000, 8},
        {"544" ,   "hd544",         "544_hollow_bastion_part_3",    0x07000000, 0x00000000, 0x00000000, 8},
        {"545" ,   "hd545",         "545_hollow_bastion_part_4",    0x07000000, 0x00000000, 0x00000000, 8},
        {"546" ,   "hd546",         "546_hollow_bastion_part_5",    0x07000000, 0x00000000, 0x00000000, 8},
        {"547" ,   "hd547",         "547_hollow_bastion_part_6",    0x07000000, 0x00000000, 0x00000000, 8},
        {"548" ,   "hd548",         "548_hollow_bastion_part_7",    0x07000000, 0x00000000, 0x00000000, 8},
        {"549" ,   "hd549",         "549_hollow_bastion_part_8",    0x07000000, 0x00000000, 0x00000000, 8},
        {"550" ,   "hd550",         "550_rikus_datascape_part_1",   0x07000000, 0x00000000, 0x00000000, 8},
        {"551" ,   "hd551",         "551_rikus_datascape_part_2",   0x07000000, 0x00000000, 0x00000000, 8},
        {"552" ,   "hd552",         "552_rikus_datascape_part_3",   0x07000000, 0x00000000, 0x00000000, 8},
        {"553" ,   "hd553",         "553_rikus_datascape_part_4",   0x07000000, 0x00000000, 0x00000000, 8},
        {"554" ,   "hd554",         "554_rikus_datascape_part_5",   0x07000000, 0x00000000, 0x00000000, 8},
        {"555" ,   "hd555",         "555_rikus_datascape_part_6",   0x07000000, 0x00000000, 0x00000000, 8},
        {"556" ,   "hd556",         "556_rikus_datascape_part_7",   0x07000000, 0x00000000, 0x00000000, 8},
        {"557" ,   "hd557",         "557_rikus_datascape_part_8",   0x07000000, 0x00000000, 0x00000000, 8},
        {"558" ,   "hd558",         "558_rikus_datascape_part_9",   0x07000000, 0x00000000, 0x00000000, 8},
        {"559" ,   "hd559",         "559_rikus_datascape_part_10",  0x07000000, 0x00000000, 0x00000000, 8},
        {"560" ,   "hd560",         "560_rikus_datascape_part_11",  0x07000000, 0x00000000, 0x00000000, 8},
        {"561" ,   "hd561",         "561_rikus_datascape_part_12",  0x07000000, 0x00000000, 0x00000000, 8},
        {"562" ,   "hd562_PLUS",    "562_rikus_datascape_part_13",  0x07000000, 0x00000000, 0x00000000, 8},
        {"570" ,   "hd570",         "570_rikus_datascape_part_14",  0x07000000, 0x00000000, 0x00000000, 8},
        {"571" ,   "hd571",         "571_rikus_datascape_part_15",  0x07000000, 0x00000000, 0x00000000, 8},
        {"572" ,   "hd572",         "572_rikus_datascape_part_16",  0x07000000, 0x00000000, 0x00000000, 8},
        {"573" ,   "hd573_PLUS",    "573_castle_oblivion_part_1",   0x07000000, 0x00000000, 0x00000000, 8},
        {"575" ,   "hd575",         "575_castle_oblivion_part_2",   0x07000000, 0x00000000, 0x00000000, 8},
        {"576" ,   "hd576_PLUS",    "576_castle_oblivion_part_3",   0x07000000, 0x00000000, 0x00000000, 8},
        {"579" ,   "hd579",         "579_castle_oblivion_part_4",   0x07000000, 0x00000000, 0x00000000, 8},
        {"580" ,   "hd580",         "580_castle_oblivion_part_5",   0x07000000, 0x00000000, 0x00000000, 8},
        {"581" ,   "hd581",         "581_castle_oblivion_part_6",   0x07000000, 0x00000000, 0x00000000, 8},
        {"582" ,   "hd582",         "582_castle_oblivion_part_7",   0x07000000, 0x00000000, 0x00000000, 8},
        {"583" ,   "hd583",         "583_castle_oblivion_part_8",   0x07000000, 0x00000000, 0x00000000, 8},
        {"584" ,   "hd584",         "584_castle_oblivion_part_9",   0x07000000, 0x00000000, 0x00000000, 8},
        {"585" ,   "hd585",         "585_castle_oblivion_part_10",  0x07000000, 0x00000000, 0x00000000, 8},
        {"586" ,   "hd586",         "586_castle_oblivion_part_11",  0x07000000, 0x00000000, 0x00000000, 8},
        {"587" ,   "hd587",         "587_castle_oblivion_part_12",  0x07000000, 0x00000000, 0x00000000, 8},
        {"588" ,   "hd588",         "588_castle_oblivion_part_13",  0x07000000, 0x00000000, 0x00000000, 8},
        {"589" ,   "hd589",         "589_castle_oblivion_part_14",  0x07000000, 0x00000000, 0x00000000, 8},
        {"590" ,   "hd590_PLUS",    "590_castle_oblivion_part_15",  0x07000000, 0x00000000, 0x00000000, 8},
        {"592" ,   "hd592",         "592_ending_part_1",            0x07000000, 0x00000000, 0x00000000, 8},
        {"593" ,   "hd593",         "593_ending_part_2",            0x07000000, 0x00000000, 0x00000000, 8},
        {"594" ,   "hd594",         "594_ending_part_3",            0x07000000, 0x00000000, 0x00000000, 8},
        {"595" ,   "hd595",         "595_ending_part_4",            0x07000000, 0x00000000, 0x00000000, 8},
        {"596" ,   "hd596",         "596_credits",                  0x07000000, 0x00000000, 0x00000000, 8},
    }};

    BgmEntries = std::array<BgmEntry, 41> {{
        { 0,  0,    "WorldSelect", "" },
        { 1,  1,    "Avatar", "" },
        { 2,  2,    "ShroudingDark", "" },
        { 3,  3,    "Dest_F", "" },
        { 4,  4,    "Night_B", "" },
        { 5,  5,    "Destati", "" },
        { 6,  6,    "StrangeWhispers", "" },
        { 7,  7,    "Andante", "" },
        { 8,  8,    "Ttown_F", "" },
        { 9,  9,    "Alice_F", "" },
        { 10, 10,   "Alice_B", "" },
        { 11, 11,   "Herc_F", "" },
        { 12, 12,   "Herc_F2", "" },
        { 13, 13,   "Herc_B", "" },
        { 14, 14,   "Aladdin_F", "" },
        { 15, 15,   "Aladdin_B", "" },
        { 16, 16,   "Hollow_F", "" },
        { 17, 17,   "Hollow_B", "" },
        { 18, 18,   "Hollow_B2", "" },
        { 19, 19,   "Castle_F", "" },
        { 20, 20,   "Castle_B", "" },
        { 21, 21,   "Namine", "" },
        { 22, 22,   "Sysnormal_F", "" },
        { 23, 23,   "Sysnormal_B", "" },
        { 24, 24,   "Sysbug_F", "" },
        { 25, 25,   "Sysbug_B", "" },
        { 26, 26,   "Ability", "" },
        { 27, 27,   "Destiny_B", "" },
        { 28, 28,   "Roxas_B", "" },
        { 29, 29,   "Missionboss1_B", "" },
        { 30, 30,   "DisneyBoss1_B", "" },
        { 31, 31,   "Boss3_B", "" },
        { 32, 32,   "Boss4_B", "" },
        { 33, 33,   "End_B", "" },
        { 34, 34,   "Villains", "" },
        { 35, 35,   "Riku", "" },
        { 36, 36,   "Laughter", "" },
        { 37, 37,   "Courage", "" },
        { 38, 38,   "LastBoss", "" },
        { 39, 39,   "Debug", "" },
        { 40, 40,   "Rik_BG", "" }
    }};

    StreamedBgmEntries = std::array<StreamedBgmEntry, 1> {{
        { 0X64, 41, "Dearly Beloved", 3212253 }
    }};
}

void PluginKingdomHeartsReCoded::overrideConfigs(
    std::function<void(std::string, bool)> setBoolConfig,
    std::function<void(std::string, int)> setIntConfig,
    std::function<void(std::string, std::string)> setStringConfig
)
{
    if (AutomaticallyMapJoysticks)
    {
        overrideJoystickMappings(setIntConfig);
        setBoolConfig(tomlUniqueIdentifier() + ".JoystickDefaultsApplied", true);
    }

    KingdomHeartsHDCollection::KHMareConfig* config = KingdomHeartsHDCollection::config();
    if (config == nullptr)
    {
        return;
    }

    std::string khLanguage = KingdomHeartsHDCollection::language();
    int localIndex = 1;
    if (khLanguage == "japanese")
        localIndex = 0;
    else if (khLanguage == "english")
        localIndex = 1;
    else if (khLanguage == "french")
        localIndex = 2;
    else if (khLanguage == "deutch")
        localIndex = 3;
    else if (khLanguage == "italian")
        localIndex = 4;
    else if (khLanguage == "spanish")
        localIndex = 5;
    setIntConfig("Instance0.Firmware.Language", localIndex);

    // TODO: KH Load Steam persona name
    // setStringConfig("Instance0.Firmware.Username", "MelonMix"); // max length of 10

    int scaleFactor = (int)std::ceil(std::max(((float)config->resolutionWidth)/256, ((float)config->resolutionHeight)/192));
    setIntConfig("3D.GL.ScaleFactor", scaleFactor - 1); // TODO: KH Once we implement support to frame skip, this "- 1" shouldn't be necessary

    float aspectRatio = ((float)config->resolutionWidth)/((float)config->resolutionHeight);
    bool isAspectRatioId0 = std::abs(aspectRatio -  4.0/3.0) < 0.0001;
    bool isAspectRatioId1 = std::abs(aspectRatio - 16.0/9.0) < 0.0001;
    bool isAspectRatioId2 = std::abs(aspectRatio - 21.0/9.0) < 0.0001;
    bool isAspectRatioId4 = std::abs(aspectRatio -  5.0/3.0) < 0.0001;
    int aspectRatioId = isAspectRatioId0 ? 0 : (isAspectRatioId1 ? 1 : (isAspectRatioId2 ? 2 : (isAspectRatioId4 ? 4 : 3)));
    setIntConfig("Instance0.Window0.ScreenAspectTop", aspectRatioId);

    setIntConfig("Audio.Volume", (config->sound.masterVolume == 1) ? 0 : (config->sound.masterVolume*256)/100);
    setIntConfig("Audio.BGMVolume", (config->sound.bgmVolume == 1) ? 0 : (config->sound.bgmVolume*10));
}

void PluginKingdomHeartsReCoded::overrideJoystickMappings(std::function<void(std::string, int)> setIntConfig)
{
    KingdomHeartsHDCollection::KHMareConfig* config = KingdomHeartsHDCollection::config();
    if (config == nullptr)
    {
        if (!JoystickDefaultsApplied)
            KingdomHeartsHDCollection::applyJoystickMappings(setIntConfig, false);
        return;
    }

    KingdomHeartsHDCollection::applyKeyboardAndJoystickMappings(config, setIntConfig);
}

void PluginKingdomHeartsReCoded::applyRecommendedJoystickMappings(std::function<void(std::string, int)> setIntConfig)
{
    KingdomHeartsHDCollection::applyJoystickMappings(setIntConfig, false);
}

std::string PluginKingdomHeartsReCoded::saveFilePath()
{
    KingdomHeartsHDCollection::createSignalFile();

    const char* saveFilePathStrPtr = std::getenv("MELON_MIX_SAVE");
    if (saveFilePathStrPtr != nullptr)
    {
        return std::string(saveFilePathStrPtr);
    }

    std::string saveFilePathStr = "";
    std::filesystem::path saveFilePath = KingdomHeartsHDCollection::configFolderPath();
    if (!saveFilePath.empty())
    {
        saveFilePathStr = saveFilePath.string();
    }
    if (saveFilePathStr.empty())
    {
        return "";
    }

    std::string saveFileName = "KHRECODED_WW.sav";

    int i = saveFilePathStr.length() - 1;
    if (saveFilePathStr[i] != '/' && saveFilePathStr[i] != '\\')
    {
        saveFilePathStr += "/";
    }

    return saveFilePathStr + saveFileName;
}

StartupWindowConfig PluginKingdomHeartsReCoded::startupWindowConfig()
{
    KingdomHeartsHDCollection::KHMareConfig* config = KingdomHeartsHDCollection::config();
    if (config == nullptr) {
        return { FullscreenOnStartup ? 1 : -1, 0, 0 };
    }
    // Launcher display mode: 0 = fullscreen, 1 = borderless, 2 = windowed.
    StartupWindowConfig result;
    result.mode = config->windowMode;
    result.width = config->resolutionWidth;
    result.height = config->resolutionHeight;
    delete config;
    return result;
}

bool PluginKingdomHeartsReCoded::shouldOpenKHExtendedSettings() {
    if (GameScene == gameScene_InGameMenu) {
        u32 mainMenuView = getCurrentMainMenuView();
        if (mainMenuView == 6) { // config
            return true;
        }
    }
    return GameScene == gameScene_Intro || GameScene == gameScene_TitleScreen;
}

void PluginKingdomHeartsReCoded::loadLocalization() {
    u8* rom = (u8*)nds->GetNDSCart()->GetROM();

    std::string language = GameLanguage.code;

    std::string LocalizationFilePath = localizationFilePath(language);
    Platform::FileHandle* f = Platform::OpenLocalFile(LocalizationFilePath.c_str(), Platform::FileMode::ReadText);
    if (f) {
        char linebuf[1024];
        char entryname[32];
        char entryval[1024];
        bool firstLine = true;
        while (!Platform::IsEndOfFile(f))
        {
            if (!Platform::FileReadLine(linebuf, 1024, f))
                break;

            const char* line = linebuf;
            if (firstLine)
            {
                firstLine = false;
                line = skipUtf8Bom(line);
            }

            int ret = sscanf(line, "%31[A-Za-z_0-9]=%[^\t\r\n]", entryname, entryval);
            entryname[31] = '\0';
            if (ret < 2) continue;

            std::string entrynameStr = std::string(entryname);
            if (entrynameStr.compare(0, 2, "0x") == 0) {
                int addrGap = 0;
                unsigned int addr = std::stoul(entrynameStr.substr(2), nullptr, 16);

                bool ended = false;
                for (int i = 0; i < 1023; i++) {
                    if (*((u8*)&rom[addr + i]) == 0x00) {
                        break;
                    }

                    if (entryval[i + addrGap] == '\\' && entryval[i + addrGap + 1] == 'n') {
                        *((u8*)&rom[addr + i]) = 0x0A;
                        addrGap++;
                        continue;
                    }

                    if (entryval[i + addrGap] == 0 || entryval[i + addrGap] == '\0') {
                        ended = true;
                    }

                    if (ended) {
                        *((u8*)&rom[addr + i]) = 0x20;
                    }
                    else {
                        *((u8*)&rom[addr + i]) = entryval[i + addrGap];
                    }
                }
            }
        }

        CloseFile(f);
    }
    else if (false) {
        int firstAddr = 0;
        int lastAddr = 0;
        bool validCharFound = false;
        bool forbCharFound = false;
        for (int addr = 0x06A66638; addr < 0x06C49D0C; addr++) { // TODO: KH Those are the Days addresses
            bool usual = rom[addr] >= 0x41 && rom[addr] <= 0x7E;
            bool accents = rom[addr] == 0xC2 || rom[addr] == 0xC3 || (rom[addr] >= 0x80 && rom[addr] <= 0xBF);
            bool quotes = rom[addr] == 0xE2 || rom[addr] == 0x80 || rom[addr] == 0x9C || rom[addr] == 0x9D;
            bool unusual = (rom[addr] >= 0x20 && rom[addr] <= 0x40) || accents || quotes || rom[addr] == 0x0A;
            bool forb = rom[addr] == 0x93 || rom[addr] == 0x5F || rom[addr] == 0x2F;
            if (usual || unusual) {
                if (firstAddr == 0) {
                    firstAddr = addr;
                    lastAddr = addr;
                }
                else {
                    lastAddr = addr;
                }
            }
            if (usual) {
                validCharFound = true;
            }
            if (forb) {
                forbCharFound = true;
            }
            if (!usual && !unusual) {
                if (firstAddr != 0) {
                    if (!forbCharFound && validCharFound && lastAddr - firstAddr > 2) {
                        printf("0x%08X=", firstAddr);
                        for (int pAddr = firstAddr; pAddr <= lastAddr; pAddr++) {
                            if ((char)rom[pAddr] == 0x0A) {
                                printf("\\n");
                            }
                            else {
                                printf("%c", (char)rom[pAddr]);
                            }
                        }
                        printf("\n");
                    }

                    firstAddr = 0;
                    lastAddr = 0;
                    validCharFound = false;
                    forbCharFound = false;
                }
            }
        }
    }
}

void PluginKingdomHeartsReCoded::onLoadROM() {
    Plugin::onLoadROM();

    loadLocalization();

    u8* rom = (u8*)nds->GetNDSCart()->GetROM();
}

std::string PluginKingdomHeartsReCoded::gameFolderName() {
    return "recoded";
}

std::string PluginKingdomHeartsReCoded::tomlUniqueIdentifier() {
    return getStringByCart("KHReCoded_US", "KHReCoded_EU", "KHReCoded_JP");
}

void PluginKingdomHeartsReCoded::renderer_composition_component_missionInformationFromBottomScreen(std::vector<ShapeData2D>* shapes, float aspectRatio, float hudScale) {
    bool showChallengeMeter = (GameSceneState & (1 << gameSceneState_showChallengeMeter)) > 0;
    int challengeMeterHeight = showChallengeMeter ? 7 : 0;
    if (showChallengeMeter)
    {
        // challenge meter icon
        shapes->push_back(ShapeBuilder2D::square()
                .fromPosition(10, 9)
                .withSize(13, challengeMeterHeight)
                .placeAtCorner(corner_TopLeft)
                .withMargin(21.0, 32.0, 0.0, 0.0)
                .hudScale(hudScale)
                .build(aspectRatio));

        // challenge meter bar
        shapes->push_back(ShapeBuilder2D::square()
                .fromPosition(24, 10)
                .withSize(87, challengeMeterHeight - 2)
                .placeAtCorner(corner_TopLeft)
                .withMargin(35.0, 34.0, 0.0, 0.0)
                .sourceScale(2.25, 0.6)
                .hudScale(hudScale)
                .build(aspectRatio));
    }

    // bottom mission information (top right corner)
    shapes->push_back(ShapeBuilder2D::square()
            .fromBottomScreen()
            .fromPosition(5, 166)
            .withSize(119, 3)
            .placeAtCorner(corner_TopLeft)
            .withMargin(131.0, 6.0, 0.0, 0.0)
            .cropSquareCorners(0.0, 4.0, 0.0, 0.0)
            .mirror(mirror_X)
            .hudScale(hudScale)
            .build(aspectRatio));

    // bottom mission information (top right corner transparent BG)
    shapes->push_back(ShapeBuilder2D::square()
            .fromPosition(118, 162)
            .withSize(3, 3)
            .placeAtCorner(corner_TopLeft)
            .withMargin(247.0, 6.0, 0.0, 0.0)
            .force()
            .hudScale(hudScale)
            .build(aspectRatio));

    // bottom mission information (bottom center black-blue separation)
    shapes->push_back(ShapeBuilder2D::square()
            .fromBottomScreen()
            .fromPosition(10, 175)
            .withSize(237, 4)
            .placeAtCorner(corner_TopLeft)
            .withMargin(8.0, 32.0 + challengeMeterHeight, 0.0, 0.0)
            .mirror(mirror_Y)
            .sourceScale(1.0, 0.5)
            .hudScale(hudScale)
            .build(aspectRatio));

    // bottom mission information (bottom left black-blue separation)
    shapes->push_back(ShapeBuilder2D::square()
            .fromBottomScreen()
            .fromPosition(5, 175)
            .withSize(10, 6)
            .placeAtCorner(corner_TopLeft)
            .withMargin(3.0, 31.0 + challengeMeterHeight, 0.0, 0.0)
            .cropSquareCorners(0.0, 0.0, 2.25, 0.0)
            .mirror(mirror_Y)
            .sourceScale(1.0, 0.5)
            .hudScale(hudScale)
            .build(aspectRatio));

    // bottom mission information (bottom right black-blue separation)
    shapes->push_back(ShapeBuilder2D::square()
            .fromBottomScreen()
            .fromPosition(242, 175)
            .withSize(10, 6)
            .placeAtCorner(corner_TopLeft)
            .withMargin(240.0, 31.0 + challengeMeterHeight, 0.0, 0.0)
            .mirror(mirror_Y)
            .sourceScale(1.0, 0.5)
            .hudScale(hudScale)
            .build(aspectRatio));

    // bottom mission information (bigger area)
    shapes->push_back(ShapeBuilder2D::square()
            .fromBottomScreen()
            .fromPosition(5, 166)
            .withSize(247, 26)
            .placeAtCorner(corner_TopLeft)
            .withMargin(3.0, 6.0, 0.0, 0.0)
            .cropSquareCorners(4.0, 0.0, 0.0, 0.0)
            .hudScale(hudScale)
            .build(aspectRatio));

    if (showChallengeMeter)
    {
        // bottom mission information (side areas)
        shapes->push_back(ShapeBuilder2D::square()
                .fromBottomScreen()
                .fromPosition(5, 190)
                .withSize(247, 2)
                .placeAtCorner(corner_TopLeft)
                .withMargin(3.0, 32.0, 0.0, 0.0)
                .sourceScale(1.0, 4.0)
                .hudScale(hudScale)
                .build(aspectRatio));
    }

    // bottom mission information (bottom right corner)
    shapes->push_back(ShapeBuilder2D::square()
            .fromBottomScreen()
            .fromPosition(5, 166)
            .withSize(119, 5)
            .placeAtCorner(corner_TopLeft)
            .withMargin(131.0, 32.0 + challengeMeterHeight, 0.0, 0.0)
            .cropSquareCorners(0.0, 0.0, 0.0, 4.0)
            .mirror(mirror_XY)
            .sourceScale(1.0, 0.5)
            .hudScale(hudScale)
            .build(aspectRatio));

    // bottom mission information (bottom left corner)
    shapes->push_back(ShapeBuilder2D::square()
            .fromBottomScreen()
            .fromPosition(5, 166)
            .withSize(128, 5)
            .placeAtCorner(corner_TopLeft)
            .withMargin(3.0, 32.0 + challengeMeterHeight, 0.0, 0.0)
            .cropSquareCorners(0.0, 0.0, 4.0, 0.0)
            .mirror(mirror_Y)
            .sourceScale(1.0, 0.5)
            .hudScale(hudScale)
            .build(aspectRatio));
}

std::vector<ShapeData2D> PluginKingdomHeartsReCoded::renderer_composition()
{
    auto shapes = std::vector<ShapeData2D>();

    if (!SingleScreenMode) {
        return shapes;
    }

    float aspectRatio = AspectRatio / (4.f / 3.f);
    float hudScale = (((float)UIScale) - 4) / 2 + 4;
    int fullscreenMapTransitionDuration = 20;

    switch (GameScene) {
        case gameScene_IntroLoadMenu:
            {
                float scale = 192.0/(192 - 31 + 48);

                // load label
                shapes.push_back(ShapeBuilder2D::square()
                        .fromPosition(0, 0)
                        .withSize(100, 16)
                        .placeAtCorner(corner_TopLeft)
                        .sourceScale(scale, scale)
                        .hudScale(hudScale)
                        .preserveDsScale()
                        .build(aspectRatio));

                // rest of load label header
                shapes.push_back(ShapeBuilder2D::square()
                        .fromPosition(100, 0)
                        .withSize(20, 16)
                        .placeAtCorner(corner_TopRight)
                        .sourceScale(1000.0, scale)
                        .hudScale(hudScale)
                        .preserveDsScale()
                        .build(aspectRatio));

                // footer
                shapes.push_back(ShapeBuilder2D::square()
                        .fromBottomScreen()
                        .fromPosition(0, 144)
                        .withSize(256, 48)
                        .placeAtCorner(corner_BottomLeft)
                        .sourceScale(scale, scale)
                        .hudScale(hudScale)
                        .preserveDsScale()
                        .build(aspectRatio));

                // rest of footer
                shapes.push_back(ShapeBuilder2D::square()
                        .fromBottomScreen()
                        .fromPosition(251, 144)
                        .withSize(5, 48)
                        .placeAtCorner(corner_BottomRight)
                        .sourceScale(1000.0, scale)
                        .hudScale(hudScale)
                        .preserveDsScale()
                        .build(aspectRatio));

                // main content
                shapes.push_back(ShapeBuilder2D::square()
                        .placeAtCorner(corner_Top)
                        .sourceScale(scale, scale)
                        .hudScale(hudScale)
                        .preserveDsScale()
                        .build(aspectRatio));

                // main content background
                shapes.push_back(ShapeBuilder2D::square()
                        .withSize(1, 192)
                        .placeAtCorner(corner_Top)
                        .sourceScale(2000.0, scale)
                        .hudScale(hudScale)
                        .preserveDsScale()
                        .build(aspectRatio));
            }

            break;

        case gameScene_Cutscene:
            if ((GameSceneState & (1 << gameSceneState_bottomScreenCutscene)) > 0) {
                shapes.push_back(ShapeBuilder2D::square()
                        .fromBottomScreen()
                        .placeAtCorner(corner_Center)
                        .hudScale(hudScale)
                        .preserveDsScale()
                        .build(aspectRatio));
            }
            break;

        case gameScene_InGameSaveMenu:
            {
                float scale = 192.0/(192 - 31 + 48);

                // save label
                shapes.push_back(ShapeBuilder2D::square()
                        .fromPosition(0, 0)
                        .withSize(100, 16)
                        .placeAtCorner(corner_TopLeft)
                        .sourceScale(scale, scale)
                        .hudScale(hudScale)
                        .preserveDsScale()
                        .build(aspectRatio));

                // rest of save label header
                shapes.push_back(ShapeBuilder2D::square()
                        .fromPosition(100, 0)
                        .withSize(20, 16)
                        .placeAtCorner(corner_TopRight)
                        .sourceScale(1000.0, scale)
                        .hudScale(hudScale)
                        .preserveDsScale()
                        .build(aspectRatio));

                // footer info
                shapes.push_back(ShapeBuilder2D::square()
                        .fromPosition(0, 164)
                        .withSize(256, 28)
                        .withMargin(128.0 * (4.0 / hudScale), 0.0, 0.0, 10.0 * (4.0 / hudScale))
                        .placeAtCorner(corner_BottomLeft)
                        .sourceScale(scale, scale)
                        .hudScale(hudScale)
                        .preserveDsScale()
                        .build(aspectRatio));

                // footer
                shapes.push_back(ShapeBuilder2D::square()
                        .fromBottomScreen()
                        .fromPosition(0, 144)
                        .withSize(256, 48)
                        .placeAtCorner(corner_BottomLeft)
                        .sourceScale(scale, scale)
                        .hudScale(hudScale)
                        .preserveDsScale()
                        .build(aspectRatio));

                // rest of footer
                shapes.push_back(ShapeBuilder2D::square()
                        .fromBottomScreen()
                        .fromPosition(251, 144)
                        .withSize(5, 48)
                        .placeAtCorner(corner_BottomRight)
                        .sourceScale(1000.0, scale)
                        .hudScale(hudScale)
                        .preserveDsScale()
                        .build(aspectRatio));

                // main content
                shapes.push_back(ShapeBuilder2D::square()
                        .placeAtCorner(corner_Top)
                        .sourceScale(scale, scale)
                        .hudScale(hudScale)
                        .preserveDsScale()
                        .build(aspectRatio));

                // main content background
                shapes.push_back(ShapeBuilder2D::square()
                        .withSize(1, 192)
                        .placeAtCorner(corner_Top)
                        .sourceScale(2000.0, scale)
                        .hudScale(hudScale)
                        .preserveDsScale()
                        .build(aspectRatio));

                break;
            }

        case gameScene_InGameWithMap:
            if ((GameSceneState & (1 << gameSceneState_showFullscreenMap)) > 0)
            {
                fullscreenMapTransitionStep += 1;
                if (fullscreenMapTransitionStep > fullscreenMapTransitionDuration) {
                    fullscreenMapTransitionStep = fullscreenMapTransitionDuration;
                }
            }
            else
            {
                fullscreenMapTransitionStep -= 1;
                if (fullscreenMapTransitionStep < 0) {
                    fullscreenMapTransitionStep = 0;
                }
            }

            // same walk for the tree's corner <-> enlarged morph
            if ((GameSceneState & (1 << gameSceneState_showFullscreenTree)) > 0)
            {
                treeTransitionStep += 1;
                if (treeTransitionStep > TREE_TRANSITION_DURATION) {
                    treeTransitionStep = TREE_TRANSITION_DURATION;
                }
            }
            else
            {
                treeTransitionStep -= 1;
                if (treeTransitionStep < 0) {
                    treeTransitionStep = 0;
                }
            }

            // swap handoff morphs tick down here (one-shot walks)
            if (swapMorphToMapStep > 0) {
                swapMorphToMapStep -= 1;
            }
            if (swapMorphFromMapStep > 0) {
                swapMorphFromMapStep -= 1;
            }

            if ((GameSceneState & (1 << gameSceneState_showHud)) > 0)
            {
                // Follow the real bottom screen's master-brightness fade: scale the
                // map/tree box opacity with it and drop the small HUD crops while any
                // fade is on (their colorToAlpha path ignores shape opacity). Keeps
                // the mid-rebuild garbage of a tab flip off screen, and the box
                // fades out/in on the game's own transition rhythm.
                float bottomFade = bottomScreenBrightnessFactor();

                // Deliberate swap feel: the box squeezes toward its center as it
                // fades, then springs back open once the new view is lit. The
                // pull-away tracks the fade directly (it must never outlive the
                // opacity), but the spring-back walks ~12 frames past the fade so
                // the motion is actually visible — the fade itself only lasts a few
                // frames. One self-correcting follower, no swap-machine coupling.
                // sourceScale must shrink with the quad or the smaller quad would
                // CROP the content (the FS maps source pixels through sourceScale,
                // not the quad size).
                float easedFade = bottomFade * bottomFade * (3.0 - 2.0 * bottomFade);
                if (easedFade <= viewTuckFollower) {
                    viewTuckFollower = easedFade;
                } else {
                    viewTuckFollower = std::min(easedFade, viewTuckFollower + 1.0f/12.0f);
                }
                float viewTuckScale = 0.4 + 0.6 * viewTuckFollower;
                auto applyViewTuck = [viewTuckScale](ShapeData2D& s) {
                    float cx = (s.squareFinalCoords.x + s.squareFinalCoords.z) * 0.5;
                    float cy = (s.squareFinalCoords.y + s.squareFinalCoords.w) * 0.5;
                    s.squareFinalCoords.x = cx + (s.squareFinalCoords.x - cx) * viewTuckScale;
                    s.squareFinalCoords.z = cx + (s.squareFinalCoords.z - cx) * viewTuckScale;
                    s.squareFinalCoords.y = cy + (s.squareFinalCoords.y - cy) * viewTuckScale;
                    s.squareFinalCoords.w = cy + (s.squareFinalCoords.w - cy) * viewTuckScale;
                    s.sourceScale.x *= viewTuckScale;
                    s.sourceScale.y *= viewTuckScale;
                };

                // A tree -> map swap reframes the box into the map view's frame from
                // the TAP onward and HOLDS it there (keyed like the map view) until the
                // game's tab lands, so the raw full-screen map framing never settles on
                // screen — the flip animation plays inside an already-map-shaped frame.
                bool khSwapToMapInFlight = SingleScreenMode && !TreeViewSelected &&
                        (wantedBottomTab == 0 || enlargeMapPending || swapMorphToMapStep > 0);

                if ((GameSceneState & ((1 << gameSceneState_showFullscreenTree) |
                                       (1 << gameSceneState_showSmallTree))) > 0
                        || khSwapToMapInFlight) {
                    // tree view: the whole bottom screen, morphing between the minimap's
                    // corner slot (glance view) and a center box roughly the enlarged
                    // map's footprint, with the map's transition walk (treeTransitionStep
                    // advances at case entry next to the map's). The enlarged geometry is
                    // mirrored in getTreeViewRect() for the frontend's touch passthrough
                    // — keep the two in sync. During a swap handoff the box additionally
                    // morphs from/into the map view's frame instead of cutting.
                    float treeDegree = ((float)treeTransitionStep) / TREE_TRANSITION_DURATION;
                    if (khSwapToMapInFlight || swapMorphFromMapStep > 0) {
                        treeDegree = ViewEnlarged ? 1.0f : 0.0f; // swap keeps its size
                    }

                    ShapeBuilder2D treeBoxBuilder = ShapeBuilder2D::square();
                    treeBoxBuilder
                            .fromBottomScreen()
                            .fromPosition(0, 0)
                            .withSize(256, 192)
                            .placeAtCorner(corner_TopRight)
                            .withMargin(0.0, 30.0, 12.0, 0.0)
                            .sourceScale(0.35)
                            .fadeBorderSize(5.0, 5.0, 5.0, 5.0)
                            .opacity(0.95 * bottomFade)
                            .hudScale(hudScale);
                    if (khSwapToMapInFlight) {
                        // map content is sliding into the box: key out the map screen's
                        // paper colors (like the map view does) so every swap frame
                        // already looks integrated, never like the raw DS screen
                        treeBoxBuilder
                                .singleColorToAlpha(0xaa, 0xaa, 0xaa)
                                .singleColorToAlpha(0xeb, 0xe3, 0xeb)
                                .singleColorToAlpha(0x30, 0x30, 0x30);
                    }
                    ShapeData2D treeShape = treeBoxBuilder.build(aspectRatio);

                    if (treeDegree > 0)
                    {
                        ShapeData2D enlargedTreeShape = ShapeBuilder2D::square()
                                .fromBottomScreen()
                                .fromPosition(0, 0)
                                .withSize(256, 192)
                                .placeAtCorner(corner_Center)
                                .withMargin(0.0, 0.0, 0.0, TREE_ENLARGED_MARGIN_BOTTOM)
                                .sourceScale(TREE_ENLARGED_SCALE)
                                .fadeBorderSize(5.0, 5.0, 5.0, 5.0)
                                .opacity(0.80 * bottomFade)
                                .hudScale(hudScale)
                                .build(aspectRatio);
                        treeShape.transitionTo(enlargedTreeShape, treeDegree);
                    }

                    if (khSwapToMapInFlight || swapMorphFromMapStep > 0)
                    {
                        // the map view's frame at the size the swap keeps: the corner
                        // minimap, or fully transitioned to the fullscreen map (at
                        // degree 1 the minimap base is fully lerped away, so a stale
                        // center cannot leak into the enlarged geometry). The center is
                        // the last one cached off live map content — the live probe may
                        // be blind mid-flip.
                        ivec2 _morphMinimapCenter = lastKnownMinimapCenter;
                        ShapeData2D mapSideShape = ShapeBuilder2D::square()
                                .fromBottomScreen()
                                .fromPosition(_morphMinimapCenter.x - 54, _morphMinimapCenter.y - 54)
                                .withSize(108, 108)
                                .placeAtCorner(corner_TopRight)
                                .withMargin(0.0, 30.0, 12.0, 0.0)
                                .sourceScale(0.65)
                                .fadeBorderSize(5.0, 5.0, 5.0, 5.0)
                                .opacity(0.95 * bottomFade)
                                .hudScale(hudScale)
                                .build(aspectRatio);
                        if (ViewEnlarged)
                        {
                            ShapeData2D bigMapSideShape = ShapeBuilder2D::square()
                                    .fromBottomScreen()
                                    .fromPosition(12, 32)
                                    .withSize(232, 116)
                                    .placeAtCorner(corner_Center)
                                    .withMargin(0.0, 0.0, 0.0, 20.0)
                                    .sourceScale(1.5)
                                    .fadeBorderSize(5.0, 5.0, 5.0, 5.0)
                                    .opacity(0.80 * bottomFade)
                                    .hudScale(hudScale)
                                    .build(aspectRatio);
                            mapSideShape.transitionTo(bigMapSideShape, 1.0);
                        }

                        if (khSwapToMapInFlight)
                        {
                            // ramp into the map frame over the morph walk, then HOLD it
                            // there for however long the game's tab flip still takes
                            float degree = swapMorphToMapStep > 0
                                    ? 1.0f - ((float)swapMorphToMapStep) / SWAP_MORPH_DURATION
                                    : 1.0f;
                            treeShape.transitionTo(mapSideShape, degree);
                        }
                        else
                        {
                            float degree = ((float)swapMorphFromMapStep) / SWAP_MORPH_DURATION;
                            treeShape.transitionTo(mapSideShape, degree);
                        }
                    }

                    applyViewTuck(treeShape);
                    shapes.push_back(treeShape);
                }

                // (during a swap into the map, the box draws the map view's frame itself)
                if ((GameSceneState & (1 << gameSceneState_showMinimap)) > 0 && !khSwapToMapInFlight) {
                    // minimap
                    ivec2 _minimapCenter = minimapCenter();
                    lastKnownMinimapCenter = _minimapCenter; // for swap morph geometry
                    float fullscreenDegree = ((float)fullscreenMapTransitionStep) / fullscreenMapTransitionDuration;
                    if ((GameSceneState & (1 << gameSceneState_comboLimitVisible)) > 0)
                    {
                        fullscreenDegree = 0;
                    }

                    ShapeData2D minimapShape = ShapeBuilder2D::square()
                            .fromBottomScreen()
                            .fromPosition(_minimapCenter.x - 54, _minimapCenter.y - 54)
                            .withSize(108, 108)
                            .placeAtCorner(corner_TopRight)
                            .withMargin(0.0, 30.0, 12.0, 0.0)
                            .sourceScale(0.65)
                            .fadeBorderSize(5.0, 5.0, 5.0, 5.0)
                            .opacity(0.95 * bottomFade)
                            .singleColorToAlpha(0xaa, 0xaa, 0xaa)
                            .singleColorToAlpha(0xeb, 0xe3, 0xeb)
                            .singleColorToAlpha(0x30, 0x30, 0x30)
                            .hudScale(hudScale)
                            .build(aspectRatio);

                    if (fullscreenDegree > 0)
                    {
                        ShapeData2D bigMapShape = ShapeBuilder2D::square()
                                .fromBottomScreen()
                                .fromPosition(12, 32)
                                .withSize(232, 116)
                                .placeAtCorner(corner_Center)
                                .withMargin(0.0, 0.0, 0.0, 20.0)
                                .sourceScale(1.5)
                                .fadeBorderSize(5.0, 5.0, 5.0, 5.0)
                                .opacity(0.80 * bottomFade)
                                .singleColorToAlpha(0xaa, 0xaa, 0xaa)
                                .singleColorToAlpha(0xeb, 0xe3, 0xeb)
                                .singleColorToAlpha(0x30, 0x30, 0x30)
                                .hudScale(hudScale)
                                .build(aspectRatio);
                        minimapShape.transitionTo(bigMapShape, fullscreenDegree);
                    }

                    ShapeData2D minimapBGShape = ShapeBuilder2D::square()
                            .fromBottomScreen()
                            .fromPosition(204, 141)
                            .withSize(1, 1)
                            .placeAtCorner(corner_TopRight)
                            .withMargin(0.0, 30.0, 12.0, 0.0)
                            .sourceScale(0.65*108)
                            .fadeBorderSize(5.0, 5.0, 5.0, 5.0)
                            .opacity(0.95 * bottomFade)
                            .hudScale(hudScale)
                            .build(aspectRatio);

                    if (fullscreenDegree > 0)
                    {
                        ShapeData2D bigMapBGShape = ShapeBuilder2D::square()
                                .fromBottomScreen()
                                .fromPosition(204, 141)
                                .withSize(2, 1)
                                .placeAtCorner(corner_Center)
                                .withMargin(0.0, 0.0, 0.0, 20.0)
                                .sourceScale(1.5*116)
                                .fadeBorderSize(5.0, 5.0, 5.0, 5.0)
                                .opacity(0.80 * bottomFade)
                                .hudScale(hudScale)
                                .build(aspectRatio);
                        minimapBGShape.transitionTo(bigMapBGShape, fullscreenDegree);
                    }

                    applyViewTuck(minimapShape);
                    applyViewTuck(minimapBGShape);
                    shapes.push_back(minimapShape);
                    shapes.push_back(minimapBGShape);
                }

                if ((GameSceneState & (1 << gameSceneState_showFloorCounter)) > 0 && bottomFade > 0.999)
                {
                    // floor label
                    shapes.push_back(ShapeBuilder2D::square()
                            .fromBottomScreen()
                            .fromPosition(0, 0)
                            .withSize(50, 15)
                            .placeAtCorner(corner_TopRight)
                            .withMargin(0.0, 106.0, 11.0, 0.0)
                            .colorToAlpha(0x8, 0x30, 0xaa)
                            .sourceScale(1.0/1.4)
                            .hudScale(hudScale)
                            .build(aspectRatio));

                    // floor value
                    shapes.push_back(ShapeBuilder2D::square()
                            .fromBottomScreen()
                            .fromPosition(50, 0)
                            .withSize(82, 15)
                            .placeAtCorner(corner_TopRight)
                            .withMargin(0.0, 116.0, 12.0, 0.0)
                            .colorToAlpha(0x8, 0x30, 0xaa)
                            .sourceScale(1.0/1.4)
                            .hudScale(hudScale)
                            .build(aspectRatio));
                }

                if ((GameSceneState & (1 << gameSceneState_showEnemiesCounter)) > 0 && bottomFade > 0.999)
                {
                    float enemiesCounterDiagonalMergeFactor = 5.0;

                    // enemies counter (biggest part)
                    shapes.push_back(ShapeBuilder2D::square()
                            .fromBottomScreen()
                            .fromPosition(133, 0)
                            .withSize(123, 15)
                            .placeAtCorner(corner_Bottom)
                            .withMargin(0.0, 0.0, 11.5 - enemiesCounterDiagonalMergeFactor/2, 12.0)
                            .cropSquareCorners(0.0, 0.0, 0.0, enemiesCounterDiagonalMergeFactor)
                            .colorToAlpha(0x8, 0x30, 0xaa)
                            .hudScale(hudScale)
                            .build(aspectRatio));

                    // enemies counter (right side)
                    shapes.push_back(ShapeBuilder2D::square()
                            .fromBottomScreen()
                            .fromPosition(133, 0)
                            .withSize(23, 15)
                            .placeAtCorner(corner_Bottom)
                            .withMargin(61.5 - enemiesCounterDiagonalMergeFactor/2, 0.0, 0.0, 12.0)
                            .cropSquareCorners(enemiesCounterDiagonalMergeFactor, 0.0, 0.0, 0.0)
                            .colorToAlpha(0x8, 0x30, 0xaa)
                            .hudScale(hudScale)
                            .mirror(mirror_XY)
                            .build(aspectRatio));
                }

                if ((GameSceneState & (1 << gameSceneState_showBottomScreenMissionInformation)) > 0 && bottomFade > 0.999)
                {
                    renderer_composition_component_missionInformationFromBottomScreen(&shapes, aspectRatio, hudScale);
                }
            }

            break;

        case gameScene_Tutorial:
            // tutorial
            shapes.push_back(ShapeBuilder2D::square()
                    .fromBottomScreen()
                    .fromPosition(5, 0)
                    .withSize(246, 192)
                    .placeAtCorner(corner_Center)
                    .sourceScale(5.0)
                    .squareBorderRadius(10.0, 10.0, 5.0, 5.0)
                    .build(aspectRatio));

            // background
            shapes.push_back(ShapeBuilder2D::square()
                    .fromBottomScreen()
                    .fromPosition(0, 96)
                    .withSize(5, 5)
                    .placeAtCorner(corner_Center)
                    .sourceScale(1000.0)
                    .opacity(0.75)
                    .build(aspectRatio));

            break;

        case gameScene_LoadingScreen:
            shapes.push_back(ShapeBuilder2D::square()
                    .fromBottomScreen()
                    .placeAtCorner(corner_BottomRight)
                    .hudScale(hudScale)
                    .build(aspectRatio));
            break;

    }

    return shapes;
}

std::vector<ShapeData2D> PluginKingdomHeartsReCoded::renderer_topScreen_2DShapes() {
    float aspectRatio = AspectRatio / (4.f / 3.f);
    auto shapes = std::vector<ShapeData2D>();
    float hudScale = (((float)UIScale) - 4) / 2 + 4;
    int fullscreenMapTransitionDuration = 20;

    switch (GameScene) {
        case gameScene_Cutscene:
            if ((GameSceneState & (1 << gameSceneState_topScreenCutscene)) > 0) {
                shapes.push_back(ShapeBuilder2D::square()
                        .placeAtCorner(corner_Center)
                        .hudScale(hudScale)
                        .preserveDsScale()
                        .build(aspectRatio));
            }
            break;

        case gameScene_CutsceneWithStaticImages:
            shapes.push_back(ShapeBuilder2D::square()
                        .placeAtCorner(corner_Center)
                        .hudScale(hudScale)
                        .preserveDsScale()
                        .build(aspectRatio));
            break;

        case gameScene_InGameMenu:
            // config and quest list; the others are in horizontal style
            shapes.push_back(ShapeBuilder2D::square()
                    .placeAtCorner(corner_Center)
                    .hudScale(hudScale)
                    .preserveDsScale()
                    .force()
                    .build(aspectRatio));
            break;

        case gameScene_ResultScreen:
            // review/result screens of different kinds
            shapes.push_back(ShapeBuilder2D::square()
                    .placeAtCorner(corner_Center)
                    .hudScale(hudScale)
                    .preserveDsScale()
                    .build(aspectRatio));
            break;

        case gameScene_InGameOlympusBattle:
            // moves list
            shapes.push_back(ShapeBuilder2D::square()
                    .fromPosition(0, 0)
                    .withSize(256, 40)
                    .placeAtCorner(corner_TopLeft)
                    .hudScale(hudScale)
                    .build(aspectRatio));

            // cleaning the rest of the upper area of the screen
            shapes.push_back(ShapeBuilder2D::square()
                    .fromPosition(118, 152)
                    .withSize(14, 10)
                    .placeAtCorner(corner_Top)
                    .sourceScale(aspectRatio*20, 1.0*4)
                    .hudScale(hudScale)
                    .preserveDsScale()
                    .build(aspectRatio));

        case gameScene_InGameDialog:
            if ((GameSceneState & (1 << gameSceneState_dialogVisible)) > 0) {
                shapes.push_back(ShapeBuilder2D::square()
                        .placeAtCorner(corner_Center)
                        .hudScale(hudScale)
                        .preserveDsScale()
                        .build(aspectRatio));
                break;
            }

        case gameScene_InGameWithMap:
            if ((GameSceneState & (1 << gameSceneState_bugLevelVisible)) > 0)
            {
                // bug level meter
                shapes.push_back(ShapeBuilder2D::square()
                        .fromPosition(0, 0)
                        .withSize(140, 20)
                        .placeAtCorner(corner_TopLeft)
                        .hudScale(hudScale)
                        .build(aspectRatio));
            }

            if ((GameSceneState & (1 << gameSceneState_topScreenMissionInformationVisible)) > 0)
            {
                // top mission information
                shapes.push_back(ShapeBuilder2D::square()
                        .fromPosition(0, 0)
                        .withSize(256, 40)
                        .placeAtCorner(corner_TopLeft)
                        .hudScale(hudScale)
                        .build(aspectRatio));
            }

            if ((GameSceneState & (1 << gameSceneState_textOverScreen)) > 0)
            {
                // texts over screen, like in the tutorial
                shapes.push_back(ShapeBuilder2D::square()
                        .placeAtCorner(corner_Center)
                        .hudScale(hudScale)
                        .preserveDsScale()
                        .build(aspectRatio));
                break;
            }

            if ((GameSceneState & (1 << gameSceneState_dialogVisible)) > 0) {
                shapes.push_back(ShapeBuilder2D::square()
                        .placeAtCorner(corner_Center)
                        .hudScale(hudScale)
                        .preserveDsScale()
                        .build(aspectRatio));
                break;
            }

            if ((GameSceneState & (1 << gameSceneState_showHud)) > 0)
            {
                if ((GameSceneState & (1 << gameSceneState_topScreenMissionInformationVisible)) == 0)
                {
                    if ((GameSceneState & (1 << gameSceneState_deweyDialogVisible)) > 0) {
                        // dewey dialog when visiting the alleyway for the first time, while walking backwards
                        // Note: the workaround below avoids duplicating the enemy health

                        shapes.push_back(ShapeBuilder2D::square()
                                .fromPosition(36, 0)
                                .withSize(184, 16)
                                .placeAtCorner(corner_Top)
                                .withMargin(0.0, 7.5, 0.0, 0.0)
                                .hudScale(hudScale)
                                .build(aspectRatio));

                        shapes.push_back(ShapeBuilder2D::square()
                                .fromPosition(0, 16)
                                .withSize(256, 79)
                                .placeAtCorner(corner_Top)
                                .withMargin(0.0, 23.5, 0.0, 0.0)
                                .hudScale(hudScale)
                                .build(aspectRatio));

                        // enemy health, if any
                        shapes.push_back(ShapeBuilder2D::square()
                                .fromPosition(220, 0)
                                .withSize(36, 16)
                                .placeAtCorner(corner_TopRight)
                                .withMargin(0.0, 7.5, 9.0, 0.0)
                                .hudScale(hudScale)
                                .build(aspectRatio));
                    }
                    else {
                        // enemy health
                        shapes.push_back(ShapeBuilder2D::square()
                                .fromPosition(163, 0)
                                .withSize(93, 22)
                                .placeAtCorner(corner_TopRight)
                                .withMargin(0.0, 7.5, 9.0, 0.0)
                                .hudScale(hudScale)
                                .build(aspectRatio));
                    }
                }

                if ((GameSceneState & (1 << gameSceneState_showOlympusBattlePlayerHealth)) > 0)
                {
                    // player health
                    shapes.push_back(ShapeBuilder2D::square()
                            .fromPosition(134, 114)
                            .withSize(122, 78)
                            .placeAtCorner(corner_BottomRight)
                            .hudScale(hudScale)
                            .build(aspectRatio));
                }

                if ((GameSceneState & (1 << gameSceneState_showRegularPlayerHealth)) > 0)
                {
                    float playerHealthBottomMargin = 12.5;
                    float playerHealthRightMargin = 12.0;

                    // player health (green bar)
                    shapes.push_back(ShapeBuilder2D::square()
                            .fromPosition(110, 182)
                            .withSize(146, 10)
                            .placeAtCorner(corner_BottomRight)
                            .withMargin(0.0, 0.0, playerHealthRightMargin, playerHealthBottomMargin)
                            .hudScale(hudScale)
                            .build(aspectRatio));

                    // player health
                    shapes.push_back(ShapeBuilder2D::square()
                            .fromPosition(168, 114)
                            .withSize(88, 78)
                            .placeAtCorner(corner_BottomRight)
                            .withMargin(0.0, 0.0, playerHealthRightMargin, playerHealthBottomMargin)
                            .cropSquareCorners(60.0, 0.0, 0.0, 0.0) // avoids clashing with speed combo finisher
                            .hudScale(hudScale)
                            .build(aspectRatio));

                    // TODO: KH UI implement cropped corner
                    //    This may no longer be needed
                    // if (finalPos.x*1.7 + finalPos.y > 64.0) {

                    if ((GameSceneState & (1 << gameSceneState_deweyDialogVisible)) == 0) {
                        // player allies health
                        shapes.push_back(ShapeBuilder2D::square()
                                .fromPosition(220, 72)
                                .withSize(36, 120)
                                .placeAtCorner(corner_BottomRight)
                                .withMargin(0.0, 0.0, playerHealthRightMargin, playerHealthBottomMargin)
                                .hudScale(hudScale)
                                .build(aspectRatio));
                    }
                }

                if ((GameSceneState & (1 << gameSceneState_showCommandMenu)) > 0)
                {
                    // command menu
                    shapes.push_back(ShapeBuilder2D::square()
                            .fromPosition(0, 108)
                            .withSize(88, 84)
                            .placeAtCorner(corner_BottomLeft)
                            .withMargin(10.0, 0.0, 0.0, 0.0)
                            .hudScale(hudScale)
                            .build(aspectRatio));
                }

                if ((GameSceneState & (1 << gameSceneState_comboLimitVisible)) > 0)
                {
                    // combo limit counter
                    shapes.push_back(ShapeBuilder2D::square()
                            .fromPosition(0, 130)
                            .withSize(78, 61)
                            .placeAtCorner(corner_BottomLeft)
                            .cropSquareCorners(0.0, 11.0, 0.0, 0.0)
                            .hudScale(hudScale)
                            .build(aspectRatio));

                    // top center counter
                    shapes.push_back(ShapeBuilder2D::square()
                            .fromPosition(100, 0)
                            .withSize(54, 16)
                            .placeAtCorner(corner_Top)
                            .withMargin(0.0, 5.0, 0.0, 0.0)
                            .hudScale(hudScale)
                            .build(aspectRatio));

                    if ((GameSceneState & (1 << gameSceneState_speedComboFinisherVisible)) > 0)
                    {
                        // vertical area, including bottom arrow that points down
                        shapes.push_back(ShapeBuilder2D::square()
                                .fromPosition(112, 15)
                                .withSize(36, 165)
                                .placeAtCorner(corner_Center)
                                .hudScale(hudScale)
                                .build(aspectRatio));

                        // most of the finisher
                        shapes.push_back(ShapeBuilder2D::square()
                                .fromPosition(54, 15)
                                .withSize(150, 147)
                                .placeAtCorner(corner_Center)
                                .withMargin(0.0, 0.0, 0.0, 9.0)
                                .cropSquareCorners(0.0, 0.0, 46.0, 46.0)
                                .hudScale(hudScale)
                                .build(aspectRatio));
                    }
                }

                if (GameScene != gameScene_InGameOlympusBattle)
                {
                    if ((GameSceneState & (1 << gameSceneState_showNextAreaName)) > 0)
                    {
                        // next area name
                        shapes.push_back(ShapeBuilder2D::square()
                                .fromPosition(88, 160)
                                .withSize(80, 23)
                                .placeAtCorner(corner_Bottom)
                                .withMargin(0.0, 0.0, 0.0, 9.0)
                                .cropSquareCorners(0.0, 0.0, 0.0, 9.0)
                                .hudScale(hudScale)
                                .build(aspectRatio));
                    }

                    if ((GameSceneState & (1 << gameSceneState_topScreenMissionInformationVisible)) > 0)
                    {
                        // cleaning the rest of the upper area of the screen
                        shapes.push_back(ShapeBuilder2D::square()
                                .fromPosition(118, 152)
                                .withSize(20, 10)
                                .placeAtCorner(corner_Top)
                                .sourceScale(aspectRatio*13, 1.0*4)
                                .hudScale(hudScale)
                                .force()
                                .preserveDsScale()
                                .build(aspectRatio));
                    }

                    if ((GameSceneState & (1 << gameSceneState_deweyDialogVisible)) == 0 &&
                        (GameSceneState & (1 << gameSceneState_speedComboFinisherVisible)) == 0) {
                        // overclock notification
                        shapes.push_back(ShapeBuilder2D::square()
                                .fromPosition(0, 81)
                                .withSize(95, 27)
                                .placeAtCorner(corner_BottomLeft)
                                .withMargin(0.0, 0.0, 0.0, 84.0)
                                .hudScale(hudScale)
                            .build(aspectRatio));

                        // pickup / item notification
                        shapes.push_back(ShapeBuilder2D::square()
                                .fromPosition(0, 39)
                                .withSize(102, 32)
                                .placeAtCorner(corner_BottomLeft)
                                .withMargin(0.0, 0.0, 0.0, 124.0)
                                .hudScale(hudScale)
                                .build(aspectRatio));

                        // level up notification + Scout-L License
                        shapes.push_back(ShapeBuilder2D::square()
                                .fromPosition(161, 24)
                                .withSize(95, 47)
                                .placeAtCorner(corner_TopRight)
                                .withMargin(0.0, 133.0, 0.0, 0.0)
                                .hudScale(hudScale)
                                .build(aspectRatio));
                    }
                }

                // background
                shapes.push_back(ShapeBuilder2D::square()
                        .fromPosition(110, 160)
                        .withSize(4, 2)
                        .placeAtCorner(corner_Center)
                        .sourceScale(5000.0)
                        .build(aspectRatio));
            }

            break;

        case gameScene_PauseMenu:
            // pause menu (hidden because we got the new one on overlay)
            /*shapes.push_back(ShapeBuilder2D::square()
                    .placeAtCorner(corner_Center)
                    .hudScale(hudScale)
                    .build(aspectRatio));*/

            // background
            shapes.push_back(ShapeBuilder2D::square()
                    .fromPosition(118, 152)
                    .withSize(20, 10)
                    .placeAtCorner(corner_Center)
                    .sourceScale(1000.0)
                    .build(aspectRatio));

            break;

        case gameScene_DeathScreen:
            shapes.push_back(ShapeBuilder2D::square()
                    .placeAtCorner(corner_Center)
                    .hudScale(hudScale)
                    .preserveDsScale()
                    .build(aspectRatio));
            break;
    }

    return shapes;
}

std::vector<ShapeData3D> PluginKingdomHeartsReCoded::renderer_topScreen_3DShapes() {
    float aspectRatio = AspectRatio / (4.f / 3.f);
    auto shapes = std::vector<ShapeData3D>();
    float hudScale = (((float)UIScale) - 4) / 2 + 4;

    if (GameScene == gameScene_InGameWithMap       || GameScene == gameScene_InGameDialog ||
        GameScene == gameScene_InGameOlympusBattle || GameScene == gameScene_PauseMenu)
    {
        bool isFinisherWithComboLimitHappening = ((GameSceneState & (1 << gameSceneState_comboLimitVisible)) > 0);

        if (HideAllHUD)
        {
            // no HUD
            shapes.push_back(ShapeBuilder3D::square()
                    .placeAtCorner(corner_Center)
                    .zRange(-1.0, -0.000001)
                    .hide()
                    .build(aspectRatio));
            return shapes;
        }

        if (GameScene != gameScene_InGameOlympusBattle) {

            if (isFinisherWithComboLimitHappening)
            {
                if ((GameSceneState & (1 << gameSceneState_starRaveFinisherVisible))   > 0) {
                    // buttons
                    shapes.push_back(ShapeBuilder3D::square()
                                .polygonMode()
                                .zRange(-1.0, -1.0)
                                .hudScale(hudScale)
                                .build(aspectRatio));

                    // lines
                    shapes.push_back(ShapeBuilder3D::square()
                                .polygonMode()
                                .placeAtCorner(corner_Center)
                                .zRange(-0.999999, -0.99)
                                .sourceScale(aspectRatio, 1.0)
                                .hudScale(SCREEN_SCALE)
                                .build(aspectRatio));
                }

                if ((GameSceneState & (1 << gameSceneState_spinnerSawFinisherVisible)) > 0) {
                    shapes.push_back(ShapeBuilder3D::square()
                                .polygonMode()
                                .placeAtCorner(corner_Center)
                                .zRange(-1.0, -0.1)
                                .hudScale(hudScale)
                                .build(aspectRatio));
                    }
            }

            if (!isFinisherWithComboLimitHappening) {
                // SP score, and Inside Riku Data Percent
                shapes.push_back(ShapeBuilder3D::square()
                        .polygonMode()
                        .negatePolygonAttributes(2031808) // pickup license notification
                        .fromPosition(0, 0)
                        .withSize(130, 60)
                        .placeAtCorner(corner_TopLeft)
                        .withMargin(0.0, 30.0, 0.0, 0.0)
                        .sourceScale(1.5)
                        .zRange(-1.0, -1.0)
                        .hudScale(hudScale)
                        .negatedTextureParam(942331720) // aim
                        .negatedTextureParam(949999400) // aim (lock on)
                        .build(aspectRatio));
            }
        }

        if (!isFinisherWithComboLimitHappening) {
            // aim
            shapes.push_back(ShapeBuilder3D::square()
                    .polygonMode()
                    .polygonVertexesCount(4)
                    .polygonAttributes(1058996416)
                    .includeOutOfBoundsPolygons()
                    .zRange(-1.0, -0.5)
                    .adjustAspectRatioOnly()
                    .build(aspectRatio));

            // aim
            shapes.push_back(ShapeBuilder3D::square()
                    .polygonMode()
                    .polygonVertexesCount(4)
                    .polygonAttributes(1042219200)
                    .includeOutOfBoundsPolygons()
                    .zRange(-1.0, -0.5)
                    .adjustAspectRatioOnly()
                    .build(aspectRatio));

            // green aim small square
            shapes.push_back(ShapeBuilder3D::square()
                    .polygonMode()
                    .polygonVertexesCount(4)
                    .polygonAttributes(1025441984)
                    .fromPosition(0, 60)
                    .withSize(256, 136)
                    .zRange(-1.0, -0.5)
                    .adjustAspectRatioOnly()
                    .build(aspectRatio));

            // green aim big square
            shapes.push_back(ShapeBuilder3D::square()
                    .polygonMode()
                    .polygonVertexesCount(4)
                    .polygonAttributes(2033856)
                    .fromPosition(0, 60)
                    .withSize(256, 136)
                    .zRange(-1.0, -0.5)
                    .adjustAspectRatioOnly()
                    .build(aspectRatio));

            // pickup license notification
            shapes.push_back(ShapeBuilder3D::square()
                    .polygonMode()
                    .fromPosition(0, 27)
                    .withSize(80, 44)
                    .placeAtCorner(corner_BottomLeft)
                    .withMargin(0.0, 0.0, 0.0, 125.0)
                    .zRange(-1.0, -1.0)
                    .negateColor(0xFFFFFF)
                    .hudScale(hudScale)
                    .build(aspectRatio));

            if (GameScene != gameScene_PauseMenu) {
                // command menu
                shapes.push_back(ShapeBuilder3D::square()
                        .polygonMode()
                        .polygonAttributes(2031808)
                        .fromPosition(0, 69)
                        .withSize(80, 124)
                        .placeAtCorner(corner_BottomLeft)
                        .withMargin(10.0, 0.0, 0.0, 0.5)
                        .zRange(-1.0, -1.0)
                        .negateColor(0xFFFFFF)
                        .hudScale(hudScale)
                        .build(aspectRatio));
            }
        }
    }

    if (GameScene == gameScene_InGameOlympusBattle) {
        // olympus hand pointers
        shapes.push_back(ShapeBuilder3D::square()
                .placeAtCorner(corner_Center)
                .zRange(-1.0, -1.0)
                .build(aspectRatio));

        // player health
        shapes.push_back(ShapeBuilder3D::square()
                .fromPosition(128, 140)
                .withSize(128, 52)
                .placeAtCorner(corner_BottomRight)
                .zRange(-1.0, 0.0)
                .hudScale(UIScale)
                .build(aspectRatio));

        // player health (red part)
        shapes.push_back(ShapeBuilder3D::square()
                .fromPosition(128, 140)
                .withSize(128, 52)
                .placeAtCorner(corner_BottomRight)
                .zRange(0.25, 0.50)
                .hudScale(UIScale)
                .build(aspectRatio));
    }

    if (GameScene == gameScene_InGameMenu)
    {
        u32 mainMenuView = getCurrentMainMenuView();
        switch (mainMenuView) {
            case 17: // challenge view
                shapes.push_back(ShapeBuilder3D::square()
                        .placeAtCorner(corner_Center)
                        .build(aspectRatio));
        }
    }

    if (GameScene == gameScene_WorldSelection)
    {
        shapes.push_back(ShapeBuilder3D::square()
                .withSize(256, 192)
                .placeAtCorner(corner_Center)
                .sourceScale(aspectRatio*aspectRatio, 1.0)
                .hudScale(SCREEN_SCALE)
                .build(aspectRatio));
    }

    return shapes;
}

int PluginKingdomHeartsReCoded::renderer_gameSceneState() {
    int state = 0;
    // declared at function scope (the case bodies are not brace-scoped)
    bool khBottomScreenBoxVisible = false;

    switch (GameScene) {
        case gameScene_IntroLoadMenu:
            break;

        case gameScene_Cutscene:
            if (detectTopScreenCutscene() == nullptr) {
                state |= (1 << gameSceneState_bottomScreenCutscene);
            }
            else if (detectBottomScreenCutscene() == nullptr) {
                state |= (1 << gameSceneState_topScreenCutscene);
            }
            break;

        case gameScene_CutsceneWithStaticImages:
            break;

        case gameScene_InGameDialog:
            if (!isHealthVisible() || !isCommandMenuVisible()) {
                state |= (1 << gameSceneState_dialogVisible);
                break;
            }

        case gameScene_InGameOlympusBattle:
        case gameScene_InGameWithMap:
            if (isDeweyDialogVisible())
            {
                state |= (1 << gameSceneState_deweyDialogVisible);
            }

            if (isComboLimitVisible())
            {
                state |= (1 << gameSceneState_comboLimitVisible);
            }
            if (isSpeedComboFinisherVisible())
            {
                state |= (1 << gameSceneState_speedComboFinisherVisible);
            }
            if (isStarRaveFinisherVisible())
            {
                state |= (1 << gameSceneState_starRaveFinisherVisible);
            }
            if (isSpinnerSawFinisherVisible())
            {
                state |= (1 << gameSceneState_spinnerSawFinisherVisible);
            }

            if (isBugLevelVisibleOnTopScreen())
            {
                state |= (1 << gameSceneState_bugLevelVisible);
            }

            if (isMissionInformationVisibleOnTopScreen())
            {
                state |= (1 << gameSceneState_topScreenMissionInformationVisible);
            }

            if (!isHealthVisible() && !isCommandMenuVisible())
            {
                state |= (1 << gameSceneState_textOverScreen);
                break;
            }

            if (isDialogVisible() && !isComboLimitVisible())
            {
                state |= (1 << gameSceneState_dialogVisible);
                break;
            }

            if (GameScene == gameScene_InGameWithMap && SingleScreenMode && !HideAllHUD) {
                // The bottom-screen box shows the tree — and stays up THROUGH a map<->
                // tree swap (the game's own tab-flip animation plays inside it), so the
                // swap never leaves a hole where a view used to be. But it only ever
                // shows NON-map content (tab != 0): over settled map content the proper
                // map view renders instead, so the raw DS map screen is never exposed.
                int bottomTab = bottomScreenTabState();
                bool treeShowing = TreeViewSelected && bottomTab == 1;
                bool swapInFlight = (wantedBottomTab != -1 || enlargeMapPending) && bottomTab != 0;
                if ((treeShowing || swapInFlight) && !ShowFullscreenMap) {
                    khBottomScreenBoxVisible = true;
                    state |= (1 << (ViewEnlarged ? gameSceneState_showFullscreenTree
                                                 : gameSceneState_showSmallTree));
                }
            }

            if (GameScene == gameScene_InGameWithMap && isMinimapVisible()) {
                if (ShowFullscreenMap) {
                    state |= (1 << gameSceneState_showFullscreenMap);
                }

                // the box replaces the corner minimap while it is up (tab==0 frames of a
                // swap would otherwise draw the map twice, small and boxed)
                if (ShowMap && !khBottomScreenBoxVisible) {
                    state |= (1 << gameSceneState_showMinimap);

                    if (isBugSector())
                    {
                        state |= (1 << gameSceneState_showFloorCounter);
                        state |= (1 << gameSceneState_showEnemiesCounter);
                        state |= (1 << gameSceneState_showBottomScreenMissionInformation);

                        if (isChallengeMeterVisible() && !isMissionInformationVisibleOnTopScreen())
                        {
                            state |= (1 << gameSceneState_showChallengeMeter);
                        }
                    }
                }
            }

            if (!HideAllHUD)
            {
                if (isHealthVisible())
                {
                    if (GameScene == gameScene_InGameOlympusBattle) {
                        state |= (1 << gameSceneState_showOlympusBattlePlayerHealth);
                    }
                    else {
                        state |= (1 << gameSceneState_showRegularPlayerHealth);
                    }
                }

                if (isCommandMenuVisible())
                {
                    state |= (1 << gameSceneState_showCommandMenu);

                    if (GameScene != gameScene_InGameOlympusBattle) {
                        state |= (1 << gameSceneState_showNextAreaName);
                    }
                }

                state |= (1 << gameSceneState_showHud);
            }

            break;

        case gameScene_PauseMenu:
            break;

        case gameScene_Tutorial:
            break;

        case gameScene_LoadingScreen:
            break;
    }

    return state;
}

int PluginKingdomHeartsReCoded::renderer_screenLayout()
{
    if (!SingleScreenMode) {
        return screenLayout_Top;
    }

    switch (GameScene) {
        case gameScene_InGameWithMap:
        case gameScene_PauseMenu:
        case gameScene_CutsceneWithStaticImages:
        case gameScene_InGameSaveMenu:
        case gameScene_InGameDialog:
        case gameScene_InGameOlympusBattle:
        case gameScene_ResultScreen:
            return screenLayout_Top;

        case gameScene_IntroLoadMenu:
        case gameScene_Tutorial:
        case gameScene_LoadingScreen:
            return screenLayout_Bottom;

        case gameScene_Intro:
        case gameScene_TitleScreen:
        case gameScene_WorldSelection:
        case gameScene_Shop:
        case gameScene_TheEnd:
        case gameScene_Other2D:
        case gameScene_Other:
            return screenLayout_BothHorizontal;

        case gameScene_Cutscene:
            return detectTopScreenCutscene() == nullptr ? screenLayout_Bottom : (detectBottomScreenCutscene() == nullptr ? screenLayout_Top : screenLayout_BothHorizontal);
    }

    if (GameScene == gameScene_InGameMenu) {
        u32 mainMenuView = getCurrentMainMenuView();
        switch (mainMenuView) {
            case 6:  // config
            case 13: // quest list
            case 17: // challenge view
                return screenLayout_Top;

            default:
                return screenLayout_BothHorizontal;
        }
    }

    return screenLayout_Top;
};

int PluginKingdomHeartsReCoded::renderer_brightnessMode()
{
    if (!SingleScreenMode) {
        // (port fix, not in desktop KHMM) Default's bottom-screen fade override blacks
        // out both screens of a dual layout whenever the game fades only the bottom one.
        return brightnessMode_PerScreen;
    }

    if (GameScene == gameScene_InGameWithMap            ||
        GameScene == gameScene_PauseMenu                ||
        GameScene == gameScene_CutsceneWithStaticImages ||
        GameScene == gameScene_InGameSaveMenu           ||
        GameScene == gameScene_InGameDialog             ||
        GameScene == gameScene_InGameOlympusBattle      ||
        GameScene == gameScene_ResultScreen             ||
        GameScene == gameScene_Other2D) {
        return brightnessMode_TopScreen;
    }
    if (GameScene == gameScene_Tutorial ||
        GameScene == gameScene_WorldSelection) {
        return brightnessMode_BottomScreen;
    }
    if (GameScene == gameScene_Intro          ||
        GameScene == gameScene_WorldSelection ||
        GameScene == gameScene_Shop           ||
        GameScene == gameScene_TheEnd         ||
        GameScene == gameScene_Other2D        ||
        GameScene == gameScene_Other) {
        return brightnessMode_Horizontal;
    }
    if (GameScene == gameScene_Cutscene) {
        return brightnessMode_DisableBrightnessControl;
    }
    if (GameScene == gameScene_InGameMenu) {
        u32 mainMenuView = getCurrentMainMenuView();
        switch (mainMenuView) {
            case 0:  // nothing
            case 2:  // main menu root (save menu)
            case 6:  // config
            case 13: // quest list
            case 17: // challenge view
                return brightnessMode_TopScreen;

            default:
                return brightnessMode_Horizontal;
        }
    }
    return brightnessMode_Default;
}

float PluginKingdomHeartsReCoded::renderer_forcedAspectRatio()
{
    if (!SingleScreenMode && shouldPreserveDsTopScreenWhileOnDualScreenModeForGameScene()) {
        return (4.0/3);
    }
    return (GameScene == gameScene_CutsceneWithStaticImages) ? (4.0/3) : AspectRatio;
};

bool PluginKingdomHeartsReCoded::renderer_showOriginalUI() {
    // (port fix, not in desktop KHMM) Upstream hardcodes false, which leaves dual screen
    // mode rendering every scene through the single-screen shape walk — the shapes only
    // fit the packed single-screen presentation, so boot logos, the title screen and
    // most non-gameplay scenes came out blank or partial. Mirror Days: pass the original
    // DS top screen through except in the scenes with real dual-screen handling.
    if (!SingleScreenMode && shouldPreserveDsTopScreenWhileOnDualScreenModeForGameScene()) {
        return true;
    }
    return false;
}

bool PluginKingdomHeartsReCoded::shouldPreserveDsTopScreenWhileOnDualScreenModeForGameScene()
{
    return GameScene != gameScene_InGameWithMap &&
           GameScene != gameScene_InGameDialog &&
           GameScene != gameScene_InGameOlympusBattle &&
           GameScene != gameScene_DeathScreen &&
           GameScene != gameScene_PauseMenu &&
           GameScene != gameScene_Tutorial;
}

void PluginKingdomHeartsReCoded::onLoadState() {
    Plugin::onLoadState();

    texturesIndex.clear();

    loadLocalization();

    GameScene = gameScene_InGameWithMap;
}

void PluginKingdomHeartsReCoded::applyHotkeyToInputMaskOrTouchControls(u32* InputMask, u16* touchX, u16* touchY, bool* isTouching, u32* HotkeyMask, u32* HotkeyPress)
{
    bool shouldContinue = _superApplyHotkeyToInputMask(InputMask, HotkeyMask, HotkeyPress);
    if (!shouldContinue) {
        return;
    }

    if (GameScene == -1) {
        return;
    }

    if (GameScene == gameScene_LoadingScreen) {
        *HotkeyMask |= (1<<4); // Fast Forward (skip loading screen)
    }
}

void PluginKingdomHeartsReCoded::applyAddonKeysToInputMaskOrTouchControls(u32* InputMask, u16* touchX, u16* touchY, bool* isTouching, u32* AddonMask, u32* AddonPress)
{
    if (GameScene == -1) {
        return;
    }

    if (JoystickConfirmIndex != 0)
    {
        bool aPressed = ((~(*InputMask)) & (1<<0)) != 0;
        bool bPressed = ((~(*InputMask)) & (1<<1)) != 0;
        if (aPressed) {
            *InputMask &= ~(1<<1); // B
        }
        else {
            *InputMask |= (1<<1); // B
        }
        if (bPressed) {
            *InputMask &= ~(1<<0); // A
        }
        else {
            *InputMask |= (1<<0); // A
        }
    }

    // While the cutscene Skip/Continue menu is open, the d-pad navigates it.
    if (_superApplyAddonKeysToCutsceneMenu(AddonMask, AddonPress, HK_CommandMenuUp, HK_CommandMenuDown)) {
        return;
    }

    if ((*AddonPress) & (1 << HK_HUDToggle)) {
        hudToggle();
    }
    if ((*AddonPress) & (1 << HK_ReplacementTexturesToggle)) {
        replacementTexturesToggle();
    }
    // Tap/hold split on the map-toggle button: a tap swaps the corner view map <-> tree
    // (size untouched; HK_ViewEnlargeToggle owns that), a hold sends the game a real
    // Select press (its native combined camera + minimap zoom), which the recommended
    // controller layout otherwise makes unreachable (the physical Select button is
    // bound to this addon key).
    {
        bool mapTogglePressed = ((*AddonPress) & (1 << HK_FullscreenMapToggle)) != 0;
        bool mapToggleHeld = ((*AddonMask) & (1 << HK_FullscreenMapToggle)) != 0;
        if (mapTogglePressed) {
            mapToggleHeldFrames = 0;
            mapToggleHoldConsumed = false;
        }
        else if (mapToggleHeld && mapToggleHeldFrames >= 0) {
            mapToggleHeldFrames++;
            if (!mapToggleHoldConsumed && mapToggleHeldFrames >= 20) {
                mapToggleHoldConsumed = true;
                fullscreenMapSelectPressStep = 3;
            }
        }
        else if (!mapToggleHeld && mapToggleHeldFrames >= 0) {
            if (!mapToggleHoldConsumed) {
                toggleMapTreeView();
            }
            mapToggleHeldFrames = -1;
        }
    }
    if ((*AddonPress) & (1 << HK_ViewEnlargeToggle)) {
        toggleViewEnlarge();
    }
    refreshViewSizeForScene();
    driveBottomScreenTab();
    if (treeTabTapStep > 0) {
        treeTabTapStep--;
        if (treeTabTapStep > 0) {
            nds->TouchScreen(TREE_TAB_TOUCH_X, TREE_TAB_TOUCH_Y);
        } else {
            nds->ReleaseScreen();
        }
    }
    if ((fullscreenMapSelectPressStep--) > 0) {
        *InputMask &= ~(1<<2); // select
    }

    if (GameScene == gameScene_InGameWithMap || GameScene == gameScene_InGameOlympusBattle) {
        // Enabling L + D-Pad
        if ((*AddonMask) & ((1 << HK_CommandMenuLeft) | (1 << HK_CommandMenuRight) | (1 << HK_CommandMenuUp) | (1 << HK_CommandMenuDown)))
        {
            u32 dpadMenuAddress = getU32ByCart(INGAME_MENU_COMMAND_LIST_SETTING_ADDRESS_US,
                                               INGAME_MENU_COMMAND_LIST_SETTING_ADDRESS_EU,
                                               INGAME_MENU_COMMAND_LIST_SETTING_ADDRESS_JP);
            u32 controlTypeOffset = getU32ByCart(INGAME_MENU_COMMAND_LIST_SETTING_VALUE_US,
                                                 INGAME_MENU_COMMAND_LIST_SETTING_VALUE_EU,
                                                 INGAME_MENU_COMMAND_LIST_SETTING_VALUE_JP);

            if ((nds->ARM7Read32(dpadMenuAddress) & controlTypeOffset) == 0) {
                nds->ARM7Write32(dpadMenuAddress, nds->ARM7Read32(dpadMenuAddress) | controlTypeOffset);
            }
        }

        // So the arrow keys can be used to control the command menu
        if ((*AddonMask) & ((1 << HK_CommandMenuLeft) | (1 << HK_CommandMenuRight) | (1 << HK_CommandMenuUp) | (1 << HK_CommandMenuDown)))
        {
            *InputMask &= ~(1<<9); // L
            *InputMask |= (1<<5); // left
            *InputMask |= (1<<4); // right
            *InputMask |= (1<<6); // up
            *InputMask |= (1<<7); // down
            if (PriorAddonMask[1] & (1 << HK_CommandMenuLeft)) // Old D-pad left
                *InputMask &= ~(1<<5); // left
            if (PriorAddonMask[1] & (1 << HK_CommandMenuRight)) // Old D-pad right
                *InputMask &= ~(1<<4); // right
            if (PriorAddonMask[1] & (1 << HK_CommandMenuUp)) // Old D-pad up
                *InputMask &= ~(1<<6); // up
            if (PriorAddonMask[1] & (1 << HK_CommandMenuDown)) // Old D-pad down
                *InputMask &= ~(1<<7); // down
        }

        // R / Lock On
        {
            if ((*AddonMask) & (1 << HK_RLockOn)) {
                *InputMask &= ~(1<<8); // R
                *InputMask &= ~(1<<9); // L
            }
        }

        // Switch Target
        {
            if ((*AddonMask) & (1 << HK_LSwitchTarget)) {
                *InputMask &= ~(1<<5); // left
                *InputMask &= ~(1<<8); // R
                *InputMask &= ~(1<<9); // L
            }

            if ((*AddonMask) & (1 << HK_RSwitchTarget)) {
                *InputMask &= ~(1<<4); // right
                *InputMask &= ~(1<<8); // R
                *InputMask &= ~(1<<9); // L
            }
        }
    }
    else {
        // So the arrow keys can be used as directionals
        if ((*AddonMask) & (1 << HK_CommandMenuLeft)) {
            *InputMask &= ~(1<<5); // left
        }
        if ((*AddonMask) & (1 << HK_CommandMenuRight)) {
            *InputMask &= ~(1<<4); // right
        }
        if ((*AddonMask) & (1 << HK_CommandMenuUp)) {
            *InputMask &= ~(1<<6); // up
        }
        if ((*AddonMask) & (1 << HK_CommandMenuDown)) {
            *InputMask &= ~(1<<7); // down
        }

        if ((*AddonMask) & (1 << HK_RLockOn)) {
            *InputMask &= ~(1<<8); // R
        }
    }

    if (GameScene == gameScene_InGameMenu) {
        // Toggle screens
        {
            bool clear = false;
            for (int i = PRIOR_ADDON_MASK_SIZE - 1; i >= 0; i--) {
                if (clear) {
                    PriorAddonMask[i] = PriorAddonMask[i] & ~((1<<2) | (1<<3));
                }
                if (PriorAddonMask[i] & (1 << 2) || PriorAddonMask[i] & (1 << 3)) {
                    clear = true;
                }
            }

            {
                if (PriorAddonMask[10] & (1 << HK_LSwitchTarget) || PriorAddonMask[9] & (1 << HK_LSwitchTarget) || PriorAddonMask[8] & (1 << HK_LSwitchTarget)) {
                    *InputMask &= ~(1<<10); // X
                }
                else if (PriorAddonMask[6] & (1 << HK_LSwitchTarget) || PriorAddonMask[5] & (1 << HK_LSwitchTarget) || PriorAddonMask[4] & (1 << HK_LSwitchTarget)) {
                    *InputMask &= ~(1<<9); // L
                }
                else if (PriorAddonMask[2] & (1 << HK_LSwitchTarget) || PriorAddonMask[1] & (1 << HK_LSwitchTarget) || PriorAddonMask[0] & (1 << HK_LSwitchTarget)) {
                    *InputMask &= ~(1<<10); // X
                }
            }

            {
                if (PriorAddonMask[10] & (1 << HK_RSwitchTarget) || PriorAddonMask[9] & (1 << HK_RSwitchTarget) || PriorAddonMask[8] & (1 << HK_RSwitchTarget)) {
                    *InputMask &= ~(1<<10); // X
                }
                else if (PriorAddonMask[6] & (1 << HK_RSwitchTarget) || PriorAddonMask[5] & (1 << HK_RSwitchTarget) || PriorAddonMask[4] & (1 << HK_RSwitchTarget)) {
                    *InputMask &= ~(1<<8); // R
                }
                else if (PriorAddonMask[2] & (1 << HK_RSwitchTarget) || PriorAddonMask[1] & (1 << HK_RSwitchTarget) || PriorAddonMask[0] & (1 << HK_RSwitchTarget)) {
                    *InputMask &= ~(1<<10); // X
                }
            }
        }
    }

    for (int i = PRIOR_ADDON_MASK_SIZE - 1; i > 0; i--) {
        PriorAddonMask[i] = PriorAddonMask[i - 1];
    }
    PriorAddonMask[0] = (*AddonMask);

    if (LastSwitchTargetPress < SWITCH_TARGET_PRESS_FRAME_LIMIT) LastSwitchTargetPress++;
    if (LastLockOnPress < LOCK_ON_PRESS_FRAME_LIMIT) LastLockOnPress++;
}

bool PluginKingdomHeartsReCoded::overrideMouseTouchCoords_singleScreen(int width, int height, int& x, int& y, bool& touching) {
    int X0 = 0;
    int Y0 = 0;
    int X1 = width;
    int Y1 = height;
    int trueWidth = width;
    int trueHeight = height;
    if (AspectRatio * height < width) {
        trueWidth = (int)(AspectRatio * height);
        X0 = (width - trueWidth)/2;
        X1 = X0 + trueWidth;
    }
    else if (width / AspectRatio < height) {
        trueHeight = (int)(width / AspectRatio);
        Y0 = (height - trueHeight)/2;
        Y1 = Y0 + trueHeight;
    }

    if (x < X0 || x > X1 || y < Y0 || y > Y1) {
        return true;
    }

    x = (255*(x - X0))/trueWidth;
    y = (192*(y - Y0))/trueHeight;
    return true;
}
bool PluginKingdomHeartsReCoded::overrideMouseTouchCoords_horizontalDualScreen(int width, int height, bool invert, int& x, int& y, bool& touching) {
    if (AspectRatio == 0)
    {
        return false;
    }

    int X0 = 0;
    int Y0 = 0;
    int X1 = width;
    int Y1 = height;
    int trueWidth = width;
    int trueHeight = height;
    if (AspectRatio * height < width) {
        trueWidth = (int)(AspectRatio * height);
        X0 = (width - trueWidth)/2;
        X1 = X0 + trueWidth;
    }
    else if (width / AspectRatio < height) {
        trueHeight = (int)(width / AspectRatio);
        Y0 = (height - trueHeight)/2;
        Y1 = Y0 + trueHeight;
    }

    width = trueWidth;
    height = trueHeight;
    trueWidth = trueWidth/2;
    X0 = X0 + (invert ? 0 : trueWidth);
    float innerAspectRatio = 4.0/3;
    if (innerAspectRatio * trueHeight * 2 < width) {
        trueWidth = (int)(innerAspectRatio * trueHeight);
        X0 = X0 + (width - trueWidth*2)/2;
        X1 = X0 + trueWidth;
    }
    else if (trueWidth / innerAspectRatio < height) {
        trueHeight = (int)(trueWidth / innerAspectRatio);
        Y0 = Y0 + (height - trueHeight)/2;
        Y1 = Y0 + trueHeight;
    }

    x = (255*(x - X0))/trueWidth;
    y = (191*(y - Y0))/trueHeight;
    if (x < 0 || x > 255 || y < 0 || y > 191) {
        return true;
    }

    return true;
}
bool PluginKingdomHeartsReCoded::overrideMouseTouchCoords(int width, int height, int& x, int& y, bool& touching) {
    if (renderer_screenLayout() == screenLayout_BothHorizontal) {
        return overrideMouseTouchCoords_horizontalDualScreen(width, height, false, x, y, touching);
    }
    return false;
}

void PluginKingdomHeartsReCoded::applyTouchKeyMaskToTouchControls(u16* touchX, u16* touchY, bool* isTouching, u32 TouchKeyMask)
{
    if (GameScene == -1)
    {
        return;
    }

    s16 rStrength = (s16)CameraSensitivity;
    s16 right = (s16)(((~TouchKeyMask) & 0xF) << rStrength);
    s16 left  = (s16)((((~TouchKeyMask) >> 4)  & 0xF) << rStrength);
    s16 down  = (s16)((((~TouchKeyMask) >> 8)  & 0xF) << rStrength);
    s16 up    = (s16)((((~TouchKeyMask) >> 12) & 0xF) << rStrength);
    s16 axisX = left - right;
    s16 axisY = up - down;

    if (axisX > 127) axisX = 127;
    if (axisX < -127) axisX = -127;
    if (axisX < 0) axisX = axisX + 65536;

    if (axisY > 127) axisY = 127;
    if (axisY < -127) axisY = -127;
    if (axisY < 0) axisY = axisY + 65536;


    // Reference:
    // https://github.com/shocoman/Analog-Controls-for-NDS-Games-on-3DS/tree/master/patches_for_other_games/kingdom_hearts_recoded/action_replay_codes

    u32 ADDR_MAILBOX = 0x02FFFDF0;
    u32 ADDR_BOOT_STAGE_COUNTER = 0x02FFFC3C;
    u32 ADDR_HOOK_CAVE = 0x02000A78;

    nds->ARM7Write16(ADDR_MAILBOX + 6, axisX);
    nds->ARM7Write16(ADDR_MAILBOX + 8, axisY);

    if (nds->ARM7Read32(ADDR_BOOT_STAGE_COUNTER) > 0x400) {
        static const u32 hookCode[] = {
            0xE92D403F, 0xE59F40B4,
            0xE1D440B0, 0xE3540000,
            0x1A000003, 0xE8BD403F,
            0xE3540001, 0xE1C500B2,
            0xE12FFF1E, 0xE1A05804,
            0xE1A04C04, 0xE1A04C44,
            0xE1A05C45, 0xE2655000,
            0xE1A00004, 0xE1A01005,
            0xE59F507C, 0xE12FFF35,
            0xE2800902, 0xE1A06000,
            0xE8BD403F, 0xE1C560B0,
            0xE28EE004, 0xE12FFF1E,
            0xE92D4006, 0xE59F0054,
            0xE1D010F6, 0xE5940098,
            0xE0800281, 0xE1A00800,
            0xE1A00820, 0xE5840098,
            0xE2160004, 0xE8BD8006,
            0xE0100006, 0xE58D0010,
            0xE92D0007, 0xE59F0024,
            0xE1D020F8, 0xE1A02202,
            0xE09220C2, 0xC28EE020,
            0xB28EE084, 0xB2622000,
            0x158420C8, 0x13A07000,
            0xE8BD0007, 0xE12FFF1E,
            0x02FFFDF0, 0x020062BC
        };

        u32 addr = ADDR_HOOK_CAVE;
        for (u32 word : hookCode) {
            nds->ARM7Write32(addr, word);
            addr += 4;
        }
    }

    if (nds->ARM7Read32(ADDR_BOOT_STAGE_COUNTER) > 0x400) {
        if (isUsaCart())
        {
            if (nds->ARM7Read16(0x020A6F8E) == 0x2C01) {
                nds->ARM7Write16(0x020A6F8E, 0xF759);
                nds->ARM7Write16(0x020A6F90, 0xED74);
            }

            if (nds->ARM7Read16(0x020C4368) == 0x4030) {
                nds->ARM7Write16(0x020C4236, 0xF73C);
                nds->ARM7Write16(0x020C4238, 0xEC50);
                nds->ARM7Write16(0x020C4368, 0xF73C);
                nds->ARM7Write16(0x020C436A, 0xEBCA);
            }
        }

        if (isEuropeCart())
        {
            if (nds->ARM7Read16(0x020A6FAE) == 0x2C01) {
                nds->ARM7Write16(0x020A6FAE, 0xF759);
                nds->ARM7Write16(0x020A6FB0, 0xED74);
            }

            if (nds->ARM7Read16(0x020C4388) == 0x4030) {
                nds->ARM7Write16(0x020C4256, 0xF73C);
                nds->ARM7Write16(0x020C4258, 0xEC40);
                nds->ARM7Write16(0x020C4388, 0xF73C);
                nds->ARM7Write16(0x020C438A, 0xEBBA);
            }
        }

        if (isJapanCart())
        {
            if (nds->ARM7Read16(0x020A6A4E) == 0x2C01) {
                nds->ARM7Write16(0x020A6A4E, 0xF759);
                nds->ARM7Write16(0x020A6A50, 0xE814);
            }

            if (nds->ARM7Read16(0x020C3DA8) == 0x4030) {
                nds->ARM7Write16(0x020C3C76, 0xF73C);
                nds->ARM7Write16(0x020C3C78, 0xEF30);
                nds->ARM7Write16(0x020C3DA8, 0xF73C);
                nds->ARM7Write16(0x020C3DAA, 0xEEAA);
            }
        }
    }
}

void PluginKingdomHeartsReCoded::applyGameSettingsKeepAliveToInputMask(u32* InputMask)
{
    if (GameScene != gameScene_TitleScreen)
    {
        settingsOverlayKeepAliveTimer = 0;
        return;
    }

    // Tap Up/Down once a minute so the title screen never sits idle long enough to drop into
    // the intro cutscene while the game settings has real inputs suspended.
    settingsOverlayKeepAliveTimer++;
    *InputMask |= (1 << 6); // up
    *InputMask |= (1 << 7); // down
    if ((settingsOverlayKeepAliveTimer % 3600) >= 3590)
        *InputMask &= ~(1<<6); // up
    else if ((settingsOverlayKeepAliveTimer % 3600) >= 3580)
        *InputMask &= ~(1<<7); // down
}

void PluginKingdomHeartsReCoded::hudToggle()
{
    HUDState = HUDState + 1;
    if (HUDState >= 2) {
        HUDState = 0;
    }
    if (HUDState == 0) { // map mode
        ShowMap = true;
        ShowFullscreenMap = false;
        HideAllHUD = false;
    }
    else { // zero hud
        ShowMap = false;
        ShowFullscreenMap = false;
        HideAllHUD = true;
    }
    if (TreeViewSelected) {
        // Leaving the tree view through the HUD toggle: bring the game's tab back to
        // the map so the minimap is there when the HUD comes back.
        TreeViewSelected = false;
        wantedBottomTab = 0;
        treeTabRetryFrames = 150;
    }
    ViewEnlarged = false;
    swapAwaitingContentFlip = false;
    restoreLatchedZoom(); // HUD toggle also leaves the enlarged world
}

void PluginKingdomHeartsReCoded::toggleFullscreenMap()
{
    ShowFullscreenMap = !ShowFullscreenMap;

    ivec2 zoomedOutCenter = minimapCenter(false, true, -1, -1);
    bool isZoomedIn = zoomedOutCenter.x == -1 && zoomedOutCenter.x == -1;
    bool isZoomedOut = !isZoomedIn;

    if (!fullscreenMapShouldPreserveZoom && ShowFullscreenMap != isZoomedOut)
    {
        fullscreenMapSelectPressStep = 3;
    }

    fullscreenMapShouldPreserveZoom = (ShowFullscreenMap && isZoomedOut);
}

// Which view the game's bottom screen is actually showing, read from the TREE/MAP tab
// label: -1 unknown (tab not readable right now), 0 map view, 1 tree view. The game owns
// the tab, so every consumer trusts this probe over our own intent flags.
int PluginKingdomHeartsReCoded::bottomScreenTabState()
{
    u32* bs = bottomScreen2DTexture();
    bool mapLabel  = getPixel(bs, TREE_TAB_MAP_LABEL_PROBE_X,  TREE_TAB_LABEL_PROBE_Y, 0) == TREE_TAB_LABEL_COLOR;
    bool treeLabel = getPixel(bs, TREE_TAB_TREE_LABEL_PROBE_X, TREE_TAB_LABEL_PROBE_Y, 0) == TREE_TAB_LABEL_COLOR;
    if (mapLabel == treeLabel) {
        return -1;
    }
    return mapLabel ? 1 : 0;
}

// [KHMM-Android] How faded the REAL bottom screen currently is: 1.0 fully lit, 0.0
// fully faded to black/white (master brightness, decoded like GPU2D_Soft applies it).
// The game hides bottom-screen rebuilds (e.g. the TREE/MAP tab flip) behind this
// fade; the single-screen composite follows the TOP screen's brightness, so shapes
// sampling the bottom screen must follow the bottom fade by hand or they show the
// mid-rebuild framebuffer the fade is meant to cover.
float PluginKingdomHeartsReCoded::bottomScreenBrightnessFactor()
{
    bool bottomIsUnitB = ((nds->PowerControl9 >> 15) & 1) == 1; // display swap: A on top
    u16 mb = bottomIsUnitB ? nds->GPU.GPU2D_B.MasterBrightness : nds->GPU.GPU2D_A.MasterBrightness;
    u32 mode = (mb >> 14) & 0x3;
    if (mode != 1 && mode != 2) { // 1 = fade to white, 2 = fade to black
        return 1.0;
    }
    u32 factor = mb & 0x1F;
    if (factor > 16) factor = 16;
    float lit = 1.0 - ((float)factor) / 16.0;
    // The game swaps the bottom screen's content mid-fade once the panel is
    // "dark enough" (seen during the TREE/MAP tab flip), so a linear follow
    // still shows a faint one-frame ghost of the incoming screen. Treat the
    // dark end as fully dark and renormalize the rest so the fade stays smooth.
    const float darkFloor = 0.4;
    return lit <= darkFloor ? 0.0 : (lit - darkFloor) / (1.0 - darkFloor);
}

// A tap on the map-toggle button. Single screen: swap WHAT the view shows, map <->
// tree, at whatever size is currently selected (ViewEnlarged persists across swaps).
// Dual screen: both screens are real, so the button just flips the game's own tab
// (same as tapping it on the touchscreen), map <-> tree.
void PluginKingdomHeartsReCoded::toggleMapTreeView()
{
    if (!SingleScreenMode) {
        int tab = bottomScreenTabState();
        if (tab != -1) {
            wantedBottomTab = (tab == 1) ? 0 : 1;
            treeTabRetryFrames = 150;
        }
        return;
    }

    if (bottomScreenTabState() == -1 && !TreeViewSelected) {
        // No TREE/MAP tab on the bottom screen right now (bug sectors swap it for the
        // mission-information panel) — there is nothing to drive, so a blind swap would
        // just show that panel in the box for the retry budget and then snap back.
        // Ignore the tap; leaving the tree (tab unreadable mid-flip) stays allowed.
        return;
    }

    if (TreeViewSelected) {
        // tree -> map (small = minimap, enlarged = fullscreen map). The fullscreen-map
        // entry is DEFERRED until the tab is really back on the map: toggleFullscreenMap
        // probes the minimap zoom from bottom-screen pixels, which are tree content
        // right now (garbage probe + a possible spurious Select into the tree).
        TreeViewSelected = false;
        wantedBottomTab = 0;
        treeTabRetryFrames = 150;
        enlargeMapPending = ViewEnlarged;
        // treeTransitionStep untouched: the box reframes from its current size.
        // The into-the-map-frame morph starts NOW (not at tab landing): the box ramps
        // into the map view's frame while the game's flip plays inside it, and holds
        // there until the real map view takes over in the same geometry.
        swapMorphToMapStep = SWAP_MORPH_DURATION;
        swapMorphFromMapStep = 0;      // cancel an opposite morph from a rapid double-swap
        swapAwaitingContentFlip = false;
    }
    else {
        // map -> tree (the size flag carries over). The PROPER map view stays on
        // screen for now: the handoff to the box happens when the game's tab really
        // starts flipping (refreshViewSizeForScene), so the raw DS map screen never
        // shows through the box, and no zoom-restoring Select is ever injected here.
        TreeViewSelected = true;
        wantedBottomTab = 1;
        treeTabRetryFrames = 150;
        enlargeMapPending = false;
        swapAwaitingContentFlip = true;
        if (ViewEnlarged) {
            treeTransitionStep = TREE_TRANSITION_DURATION;
        }
        swapMorphToMapStep = 0;
    }
}

// The enlarge button: toggles the SIZE of whatever the corner view currently shows.
// Map: the upstream fullscreen-map machinery (transition animation + auto zoom-sync).
// Tree: corner glance box <-> the readable centered 4:3 box.
void PluginKingdomHeartsReCoded::toggleViewEnlarge()
{
    if (!SingleScreenMode || HideAllHUD || GameScene != gameScene_InGameWithMap) {
        return; // dual mode shows everything already; no view to enlarge otherwise
    }
    ViewEnlarged = !ViewEnlarged;
    if (!TreeViewSelected) {
        int tab = bottomScreenTabState();
        if (tab == 0 || isMinimapVisible()) {
            // map content is on the bottom screen (tab confirmed, or the minimap probe
            // sees it — bug sectors have no tab but do show the map)
            toggleFullscreenMap(); // its zoom sync makes the latch moot either way
            fullscreenMapZoomLatched = false;
        }
        else if (tab == 1) {
            // The game was somehow left on the tree tab: bring the map back first and
            // defer the fullscreen-map flip (growing only — see toggleMapTreeView).
            wantedBottomTab = 0;
            treeTabRetryFrames = 150;
            if (ViewEnlarged && !ShowFullscreenMap) {
                enlargeMapPending = true;
            }
            else if (ShowFullscreenMap) {
                ShowFullscreenMap = false; // shrink needs no zoom probe
            }
        }
        else {
            ViewEnlarged = !ViewEnlarged; // unknown bottom content: nothing to enlarge
        }
    }
    else if (!ViewEnlarged) {
        // Shrinking while on the tree: the map is not on screen, but the native zoom
        // may still be latched out from the enlarged map — restore it now (a Select
        // press works during the tree tab; the camera zoom is a field-level state).
        restoreLatchedZoom();
    }
}

// Restore the native Select zoom that an enlarged-map session left zoomed out, when
// leaving the enlarged world by a path that bypasses toggleFullscreenMap's own sync.
void PluginKingdomHeartsReCoded::restoreLatchedZoom()
{
    if (!fullscreenMapZoomLatched) {
        return;
    }
    fullscreenMapZoomLatched = false;
    if (!fullscreenMapShouldPreserveZoom) {
        fullscreenMapSelectPressStep = 3;
    }
}

// Scene changes (dialog, cutscene, area load, pause...) shrink the view back to the
// corner slot so nothing big covers the next scene; the map/tree CHOICE persists, and
// the shrink (plus its possible zoom-restoring Select press) waits until we are back
// in gameplay, where injecting Select is safe.
void PluginKingdomHeartsReCoded::refreshViewSizeForScene()
{
    if (GameScene != lastViewSizeScene) {
        if (lastViewSizeScene != -1 && SingleScreenMode && (ViewEnlarged || ShowFullscreenMap)) {
            viewSizeResetPending = true;
        }
        lastViewSizeScene = GameScene;
        // Keep the tree CHOICE alive across the transition: the game may reset its own
        // tab to the map, so re-request the tree once we are back (one-shot — a manual
        // MAP-tab tap mid-gameplay must still win via the honesty sync).
        if (GameScene == gameScene_InGameWithMap && SingleScreenMode &&
                TreeViewSelected && wantedBottomTab == -1) {
            wantedBottomTab = 1;
            treeTabRetryFrames = 150;
        }
    }
    // A map->tree swap hands off from the proper map view to the box the moment the
    // game's tab really starts flipping (probe leaves the map) — never earlier, so the
    // raw DS map screen is never exposed inside the box over settled map content.
    if (swapAwaitingContentFlip) {
        if (!TreeViewSelected) {
            swapAwaitingContentFlip = false; // swap reversed before the flip started
        }
        else if (GameScene == gameScene_InGameWithMap && bottomScreenTabState() != 0) {
            swapAwaitingContentFlip = false;
            if (ShowFullscreenMap) {
                ShowFullscreenMap = false; // raw: the native zoom stays latched
                fullscreenMapZoomLatched = true;
                fullscreenMapTransitionStep = 0;
            }
            // reframe the outgoing map view into the box instead of cutting
            swapMorphFromMapStep = SWAP_MORPH_DURATION;
            swapMorphToMapStep = 0;
        }
    }

    if (viewSizeResetPending && GameScene == gameScene_InGameWithMap) {
        viewSizeResetPending = false;
        enlargeMapPending = false;
        swapAwaitingContentFlip = false;
        ViewEnlarged = false;
        if (ShowFullscreenMap) {
            toggleFullscreenMap();
            fullscreenMapZoomLatched = false;
        }
        else {
            restoreLatchedZoom(); // the scene cut may land on the tree side
        }
    }
    // Deferred fullscreen-map entry (set when enlarging/swapping to the map while the
    // game's tab was still on the tree): flip once the map content is really there.
    if (enlargeMapPending && GameScene == gameScene_InGameWithMap &&
            !TreeViewSelected && ViewEnlarged && bottomScreenTabState() == 0) {
        enlargeMapPending = false;
        if (!ShowFullscreenMap) {
            if (fullscreenMapZoomLatched) {
                // swap re-entry: the native zoom is still where the fullscreen map left
                // it, so raise the flag raw — no Select injection, no camera lurch
                ShowFullscreenMap = true;
            }
            else {
                toggleFullscreenMap();
            }
            // land at full size: the minimap-grow walk must not replay on a swap (the
            // box has already reframed into this exact geometry and hands off cleanly)
            fullscreenMapTransitionStep = TREE_TRANSITION_DURATION;
        }
    }
    if (enlargeMapPending && (TreeViewSelected || !ViewEnlarged)) {
        enlargeMapPending = false; // superseded by a newer toggle
    }
}

// Keeps injecting tab taps (with settle gaps for the game's tab animation) until the
// game's view matches the wanted one or the retry budget runs out — a tap can get eaten
// (dialog opening, combat interruptions), and the user can flip the tab themselves.
void PluginKingdomHeartsReCoded::driveBottomScreenTab()
{
    if (wantedBottomTab < 0) {
        // Keep our intent honest when the user flips the tab themselves (the tree-view
        // touch passthrough makes the on-screen MAP tab tappable).
        if (TreeViewSelected && GameScene == gameScene_InGameWithMap && bottomScreenTabState() == 0) {
            TreeViewSelected = false;
        }
        return;
    }
    if (GameScene != gameScene_InGameWithMap) {
        return; // the tab isn't on screen/tappable right now; resume when back ingame
    }
    if (treeTabTapStep > 0) {
        return; // a tap is in flight
    }

    int tab = bottomScreenTabState();
    if (tab == wantedBottomTab) {
        // Done — checked even mid-settle, so swap handoffs (and the transition box)
        // never linger after the game's tab has really flipped.
        wantedBottomTab = -1;
        treeTabSettleFrames = 0;
        return;
    }
    if (treeTabSettleFrames > 0) {
        treeTabSettleFrames--; // not there yet: keep waiting out the tab animation
        return;
    }

    treeTabRetryFrames--;
    if (treeTabRetryFrames <= 0) {
        // Give up and accept whatever the game is showing
        wantedBottomTab = -1;
        swapAwaitingContentFlip = false;
        if (tab != -1) {
            TreeViewSelected = SingleScreenMode && tab == 1;
        }
        return;
    }

    if (tab != -1) {
        treeTabTapStep = 4;
        treeTabSettleFrames = 30;
    }
}

bool PluginKingdomHeartsReCoded::isTreeViewActive()
{
    // Stricter than the shape bit: during a map<->tree swap the enlarged box stays on
    // screen showing MAP content (the tab flip plays inside it), and touches must never
    // reach the map screen (it drives Sora's camera). Only the real, settled tree takes
    // touch passthrough.
    return SingleScreenMode
        && GameScene == gameScene_InGameWithMap
        && GameSceneState != -1
        && (GameSceneState & (1 << gameSceneState_showFullscreenTree)) != 0
        && TreeViewSelected
        && wantedBottomTab == -1
        && bottomScreenTabState() == 1;
}

// Mirrors the enlarged tree shape's geometry in renderer_composition (corner_Center,
// sourceScale TREE_ENLARGED_SCALE, bottom margin), normalized to the composite output:
// shape units live on a 256x192 * (SCREEN_SCALE/hudScale) canvas whose horizontal axis
// is later stretched by the widescreen aspect (which heightScale pre-compensates).
bool PluginKingdomHeartsReCoded::getTreeViewRect(float& x, float& y, float& w, float& h)
{
    float aspectRatio = AspectRatio / (4.f / 3.f);
    float hudScale = (((float)UIScale) - 4) / 2 + 4;
    float iuTexScale = SCREEN_SCALE / hudScale;
    if (aspectRatio <= 0 || iuTexScale <= 0) {
        return false;
    }

    w = TREE_ENLARGED_SCALE / (iuTexScale * aspectRatio);
    h = TREE_ENLARGED_SCALE / iuTexScale;
    x = (1.0f - w) / 2.0f;
    y = (1.0f - h) / 2.0f - TREE_ENLARGED_MARGIN_BOTTOM / (192.0f * iuTexScale);
    return true;
}

const char* PluginKingdomHeartsReCoded::getGameSceneName()
{
    switch (GameScene) {
        case gameScene_Intro: return "Game scene: Intro";
        case gameScene_TitleScreen: return "Game scene: Title screen";
        case gameScene_IntroLoadMenu: return "Game scene: Intro load menu";
        case gameScene_Cutscene: return "Game scene: Cutscene";
        case gameScene_InGameWithMap: return "Game scene: Ingame (with minimap)";
        case gameScene_InGameMenu: return "Game scene: Ingame menu";
        case gameScene_InGameSaveMenu: return "Game scene: Ingame save menu";
        case gameScene_PauseMenu: return "Game scene: Pause menu";
        case gameScene_Tutorial: return "Game scene: Tutorial";
        case gameScene_Shop: return "Game scene: Shop";
        case gameScene_LoadingScreen: return "Game scene: Loading screen";
        case gameScene_CutsceneWithStaticImages: return "Game scene: Cutscene with static images";
        case gameScene_ResultScreen: return "Game scene: Result screen";
        case gameScene_WorldSelection: return "Game scene: World selection";
        case gameScene_InGameDialog: return "Game scene: Ingame dialog";
        case gameScene_InGameOlympusBattle: return "Game scene: Ingame (Olympus battle)";
        case gameScene_Other2D: return "Game scene: Unknown (2D)";
        case gameScene_Other: return "Game scene: Unknown (3D)";
        default: return "Game scene: Unknown";
    }
}

bool PluginKingdomHeartsReCoded::isBufferBlack(unsigned int* buffer)
{
    if (!buffer) {
        return true;
    }

    // when the result is 'null' (filled with zeros), it's a false positive, so we need to exclude that scenario
    bool newIsNullScreen = true;
    bool newIsBlackScreen = true;
    for (int i = 0; i < 192*256; i++) {
        unsigned int color = buffer[i] & 0xFFFFFF;
        newIsNullScreen = newIsNullScreen && color == 0;
        newIsBlackScreen = newIsBlackScreen &&
                (color == 0 || color == 0x000080 || color == 0x010000 || (buffer[i] & 0xFFFFE0) == 0x018000);
        if (!newIsBlackScreen) {
            break;
        }
    }
    return !newIsNullScreen && newIsBlackScreen;
}

u32* PluginKingdomHeartsReCoded::topScreen2DTexture()
{
    int FrontBuffer = nds->GPU.FrontBuffer;
    return nds->GPU.Framebuffer[FrontBuffer][0].get();
}

u32* PluginKingdomHeartsReCoded::bottomScreen2DTexture()
{
    int FrontBuffer = nds->GPU.FrontBuffer;
    return nds->GPU.Framebuffer[FrontBuffer][1].get();
}

bool PluginKingdomHeartsReCoded::isTopScreen2DTextureBlack()
{
    int FrontBuffer = nds->GPU.FrontBuffer;
    u32* topBuffer = nds->GPU.Framebuffer[FrontBuffer][0].get();
    return isBufferBlack(topBuffer);
}

bool PluginKingdomHeartsReCoded::isBottomScreen2DTextureBlack()
{
    int FrontBuffer = nds->GPU.FrontBuffer;
    u32* bottomBuffer = nds->GPU.Framebuffer[FrontBuffer][1].get();
    return isBufferBlack(bottomBuffer);
}

bool PluginKingdomHeartsReCoded::isResultScreenVisible()
{
    u32 address = getU32ByCart(RESULT_SCREEN_ID_US, RESULT_SCREEN_ID_EU, RESULT_SCREEN_ID_JP);
    u32 value = nds->ARM7Read32(address);
    return value != 0;
}

bool PluginKingdomHeartsReCoded::isDeweyDialogVisible()
{
    u32* buffer = topScreen2DTexture();
    return (has2DOnTopOf3DAt(buffer, 50, 40) && has2DOnTopOf3DAt(buffer, 140, 40)) ||
           (has2DOnTopOf3DAt(buffer, 50, 70) && has2DOnTopOf3DAt(buffer, 140, 70)) ||
           (has2DOnTopOf3DAt(buffer, 140, 40) && has2DOnTopOf3DAt(buffer, 190, 40)) ||
           (has2DOnTopOf3DAt(buffer, 140, 60) && has2DOnTopOf3DAt(buffer, 190, 60));
}

bool PluginKingdomHeartsReCoded::isBugLevelVisibleOnTopScreen()
{
    u32* buffer = topScreen2DTexture();
    return (has2DOnTopOf3DAt(buffer, 64,  0) || has2DOnTopOf3DAt(buffer, 64,  10)) &&
           (has2DOnTopOf3DAt(buffer, 128, 0) || has2DOnTopOf3DAt(buffer, 128, 10)) &&
          !(has2DOnTopOf3DAt(buffer, 170, 0) || has2DOnTopOf3DAt(buffer, 170, 10));
}

bool PluginKingdomHeartsReCoded::isMissionInformationVisibleOnTopScreen()
{
    u32* buffer = topScreen2DTexture();
    return (has2DOnTopOf3DAt(buffer, 64,  0) || has2DOnTopOf3DAt(buffer, 64,  10)) &&
           (has2DOnTopOf3DAt(buffer, 128, 0) || has2DOnTopOf3DAt(buffer, 128, 10)) &&
           (has2DOnTopOf3DAt(buffer, 170, 0) || has2DOnTopOf3DAt(buffer, 170, 10));
}

bool PluginKingdomHeartsReCoded::isDialogVisible()
{
    u32* buffer = topScreen2DTexture();
    return has2DOnTopOf3DAt(buffer, 128, 155);
}

bool PluginKingdomHeartsReCoded::isMinimapVisible()
{
    u32* buffer = bottomScreen2DTexture();
    u32 pixel = getPixel(buffer, 1, 190, 0);
    return ((pixel >> 0) & 0x3F) < 5 && ((pixel >> 8) & 0x3F) < 15 && ((pixel >> 16) & 0x3F) > 39;
}

bool PluginKingdomHeartsReCoded::isBugSector()
{
    return getFloorLevel() != 0;
}

bool PluginKingdomHeartsReCoded::isChallengeMeterVisible()
{
    u32* buffer = topScreen2DTexture();
    return has2DOnTopOf3DAt(buffer, 12, 12);
}

bool PluginKingdomHeartsReCoded::isCommandMenuVisible()
{
    u32* buffer = topScreen2DTexture();
    return has2DOnTopOf3DAt(buffer, 35, 185);
}

bool PluginKingdomHeartsReCoded::isComboLimitVisible()
{
    u32* buffer = topScreen2DTexture();
    return has2DOnTopOf3DAt(buffer, 12, 146) && !has2DOnTopOf3DAt(buffer, 35, 185);
}

bool PluginKingdomHeartsReCoded::isSpeedComboFinisherVisible()
{
    u32* buffer = topScreen2DTexture();
    return isComboLimitVisible() && has2DOnTopOf3DAt(buffer, 65, 60) && has2DOnTopOf3DAt(buffer, 65, 75) &&
                                    has2DOnTopOf3DAt(buffer, 65, 90) && has2DOnTopOf3DAt(buffer, 65, 120);
}

bool PluginKingdomHeartsReCoded::isStarRaveFinisherVisible()
{
    int matches = 0;
    for (int i = 0; i < nds->GPU.GPU3D.RenderNumPolygons; i++)
    {
        melonDS::Polygon* polygon = nds->GPU.GPU3D.RenderPolygonRAM[i];

        bool match = false;
        for (int i = 0; i < polygon->NumVertices; i++)
        {
            s32 z = polygon->Vertices[i]->Position[2];
            float _z = ((float)z)/(1 << 22);
            if (_z > -1.0 && _z <= -0.99)
            {
                match = true;
            }
        }

        if (match)
        {
            matches++;
        }
    }

    if (matches == 2)
    {
        return isComboLimitVisible();
    }

    return false;
}

bool PluginKingdomHeartsReCoded::isSpinnerSawFinisherVisible()
{
    return isComboLimitVisible() && !isSpeedComboFinisherVisible() && !isStarRaveFinisherVisible();
}

bool PluginKingdomHeartsReCoded::isHealthVisible()
{
    u32* buffer = topScreen2DTexture();
    return has2DOnTopOf3DAt(buffer, 233, 175);
}

#define IS_COLOR(pixel,r,g,b) ((((pixel >> 8) & 0xFF) == b) && (((pixel >> 4) & 0xFF) == g) && (((pixel >> 0) & 0xFF) == r))

ivec2 PluginKingdomHeartsReCoded::minimapCenter(bool zoomedIn, bool zoomedOut, int fallbackX, int fallbackY)
{
    int distanceToCenter = 54;
    int minY = 31;
    int maxY = 150;
    int minX = 8;
    int maxX = 247;

    bool targetColorMap1[6][6] = { // zoomed in map
        {false, false, false, false, false, false},
        {false, false, true,  true,  false, false},
        {false, true,  true,  true,  true,  false},
        {false, true,  true,  true,  true,  false},
        {false, false, true,  true,  false, false},
        {false, false, false, false, false, false}
    };
    bool targetColorMap2[4][4] = { // zoomed out map
        {false, false, false, false},
        {false, true,  true,  false},
        {false, true,  true,  false},
        {false, false, false, false}
    };
    bool targetColorMap3[5][3] = { // zoomed in map (side scroll)
        {false, false, false},
        {false, true,  false},
        {false, true,  false},
        {false, true,  false},
        {false, false, false}
    };

    std::vector<ivec4> possibilities;
    u32* buffer = bottomScreen2DTexture();
    for (int y = minY; y < maxY; y++) {
        for (int x = minX; x < maxX; x++) {
            if ((getPixel(buffer, x, y, 0) == 0x1000343e) || (getPixel(buffer, x, y, 0) == 0x1000383e)) {
                bool valid = true;
                for (int subY = 0; subY < 6; subY ++) {
                    for (int subX = 0; subX < 6; subX ++) {
                        u32 pixel = getPixel(buffer, x + subX - 2, y + subY - 2, 0);
                        valid = valid && zoomedIn && (targetColorMap1[subY][subX] ? (pixel == 0x1000343e) : (pixel != 0x1000343e));
                        if (!valid) break;
                    }
                    if (!valid) break;
                }

                if (valid) {
                    possibilities.push_back(ivec4{x:x, y:y, z:6, w:6});
                }

                if (!valid) {
                    valid = true;
                    for (int subY = 0; subY < 4; subY ++) {
                        for (int subX = 0; subX < 4; subX ++) {
                            u32 pixel = getPixel(buffer, x + subX - 1, y + subY - 1, 0);
                            valid = valid && zoomedOut && (targetColorMap2[subY][subX] ? (pixel == 0x1000343e) : (pixel != 0x1000343e));
                            if (!valid) break;
                        }
                        if (!valid) break;
                    }

                    if (valid) {
                        possibilities.push_back(ivec4{x:x, y:y, z:4, w:4});
                    }

                    if (!valid) {
                        valid = true;
                        for (int subY = 0; subY < 5; subY ++) {
                            for (int subX = 0; subX < 3; subX ++) {
                                u32 pixel = getPixel(buffer, x + subX - 1, y + subY - 2, 0);
                                valid = valid && zoomedIn && (targetColorMap3[subY][subX] ? (pixel == 0x1000383e) : (pixel != 0x1000383e));
                                if (!valid) break;
                            }
                            if (!valid) break;
                        }

                        if (valid) {
                            possibilities.push_back(ivec4{x:x, y:y, z:3, w:5});
                        }
                    }
                }
            }
        }
    }

    int posSize = possibilities.size();
    if (posSize == 0) {
        return ivec2{x:fallbackX, y:fallbackY};
    }

    int x = 0;
    int y = 0;
    int bigSize = 0;
    for (int i = 0; i < posSize; i++) {
        if (bigSize < possibilities[i].z*possibilities[i].w) {
            bigSize = possibilities[i].z*possibilities[i].w;
            x = 0;
            y = 0;
        }
        if (bigSize == possibilities[i].z*possibilities[i].w) {
            x = std::max(x, possibilities[i].x);
            y = std::max(y, possibilities[i].y);
        }
    }
    ivec2 result = {
        x:std::max(std::min(x, maxX - distanceToCenter), minX + distanceToCenter),
        y:std::max(std::min(y, maxY - distanceToCenter), minY + distanceToCenter)
    };

    return result;
}

ivec2 PluginKingdomHeartsReCoded::minimapCenter()
{
    ivec2 result = minimapCenter(true, true, MinimapCenterX, MinimapCenterY);
    MinimapCenterX = result.x;
    MinimapCenterY = result.y;
    return result;
}

bool PluginKingdomHeartsReCoded::has2DOnTopOf3DAt(u32* buffer, int x, int y)
{
    /*
     * If it matches that condition, there is no 2D on top of 3D
        (alphaColor.a == 0x0) ||
        (alphaColor.a == 0x1 && _3dpix.a > 0 && alphaColor.g == 0) ||
        (alphaColor.a == 0x2 && _3dpix.a > 0 && alphaColor.g < 4) ||
        (alphaColor.a == 0x3 && _3dpix.a > 0 && alphaColor.g < 4) ||
        (alphaColor.a == 0x4 && (_3dpix.a & 0x1F) == 0x1F)
    */

    u32 pixel = getPixel(buffer, x, y, 2);
    u32 pixelG = (pixel >> 8) & 0xFF;
    u32 pixelAlpha = (pixel >> (8 * 3)) & 0xFF;
    if (pixelAlpha > 0x4)
    {
        return true;
    }
    if (pixelAlpha == 0x4)
    {
        return false;
    }
    if (pixelAlpha == 0x1 && pixelG == 0)
    {
        return false;
    }
    if ((pixelAlpha == 0x2 || pixelAlpha == 0x3) && pixelG < 4)
    {
        return false;
    }

    u32 colorPixel = getPixel(buffer, x, y, 0);
    u32 colorPixelAlpha = (colorPixel >> (8 * 3)) & 0xFF;
    if (colorPixelAlpha == 0x20) {
        return false;
    }
    return true;
}

bool PluginKingdomHeartsReCoded::shouldRenderFrame()
{
    if (!_superShouldRenderFrame())
    {
        return false;
    }

    return true;
}

int PluginKingdomHeartsReCoded::detectGameScene()
{
    if (nds == nullptr)
    {
        return GameScene;
    }

    bool wasSaveLoaded = isSaveLoaded();
    bool muchOlderHad3DOnTopScreen = _muchOlderHad3DOnTopScreen;
    bool muchOlderHad3DOnBottomScreen = _muchOlderHad3DOnBottomScreen;
    bool olderHad3DOnTopScreen = _olderHad3DOnTopScreen;
    bool olderHad3DOnBottomScreen = _olderHad3DOnBottomScreen;
    bool had3DOnTopScreen = _had3DOnTopScreen;
    bool had3DOnBottomScreen = _had3DOnBottomScreen;
    bool has3DOnTopScreen = (nds->PowerControl9 >> 15) == 1;
    bool has3DOnBottomScreen = (nds->PowerControl9 >> 9) == 1;
    _muchOlderHad3DOnTopScreen = _olderHad3DOnTopScreen;
    _muchOlderHad3DOnBottomScreen = _olderHad3DOnBottomScreen;
    _olderHad3DOnTopScreen = _had3DOnTopScreen;
    _olderHad3DOnBottomScreen = _had3DOnBottomScreen;
    _had3DOnTopScreen = has3DOnTopScreen;
    _had3DOnBottomScreen = has3DOnBottomScreen;
    bool has3DOnBothScreens = (muchOlderHad3DOnTopScreen || olderHad3DOnTopScreen || had3DOnTopScreen || has3DOnTopScreen) &&
                              (muchOlderHad3DOnBottomScreen || olderHad3DOnBottomScreen || had3DOnBottomScreen || has3DOnBottomScreen);

    int ingameState = nds->ARM7Read16(getU32ByCart(GAME_STATE_ADDRESS_US, GAME_STATE_ADDRESS_EU, GAME_STATE_ADDRESS_JP));
    bool isMainMenuOrIntroOrLoadMenu = nds->ARM7Read8(getU32ByCart(IS_MAIN_MENU_US, IS_MAIN_MENU_EU, IS_MAIN_MENU_JP)) ==
        getU8ByCart(IS_MAIN_MENU_VALUE_US, IS_MAIN_MENU_VALUE_EU, IS_MAIN_MENU_VALUE_JP);
    bool isPauseScreen = nds->ARM7Read8(getU32ByCart(PAUSE_SCREEN_ADDRESS_US, PAUSE_SCREEN_ADDRESS_EU, PAUSE_SCREEN_ADDRESS_JP)) == PAUSE_SCREEN_VALUE_TRUE_PAUSE;
    bool isCutscene = nds->ARM7Read8(getU32ByCart(IS_CUTSCENE_US, IS_CUTSCENE_EU, IS_CUTSCENE_JP)) == 0x03;
    bool isIntroLoadMenu = nds->ARM7Read32(getU32ByCart(IS_LOAD_SCREEN_US, IS_LOAD_SCREEN_EU, IS_LOAD_SCREEN_JP)) ==
        getU32ByCart(IS_LOAD_SCREEN_VALUE_US, IS_LOAD_SCREEN_VALUE_EU, IS_LOAD_SCREEN_VALUE_JP);
    bool isInGameDialog = nds->ARM7Read32(getU32ByCart(DIALOG_SCREEN_ADDRESS_US, DIALOG_SCREEN_ADDRESS_EU, DIALOG_SCREEN_ADDRESS_JP)) ==
        getU32ByCart(DIALOG_SCREEN_VALUE_US, DIALOG_SCREEN_VALUE_EU, DIALOG_SCREEN_VALUE_JP);
    bool isDeathScreen = nds->ARM7Read32(getU32ByCart(DEATH_SCREEN_ADDRESS_US, DEATH_SCREEN_ADDRESS_EU, DEATH_SCREEN_ADDRESS_JP)) == 0;
    u32 mainMenuView = getCurrentMainMenuView();

    u8 gameState2 = nds->ARM7Read8(getU32ByCart(IS_PLAYABLE_AREA_US, IS_PLAYABLE_AREA_EU, IS_PLAYABLE_AREA_JP));
    bool isUnplayableArea = gameState2 == 0x01 || gameState2 == 0x02;
    bool isWorldSelection = gameState2 == 0x03;

    // Scale of brightness, from 0 (black) to 15 (every element is visible)
    u8 topScreenBrightness = PARSE_BRIGHTNESS_FOR_WHITE_BACKGROUND(nds->GPU.GPU2D_A.MasterBrightness);
    u8 botScreenBrightness = PARSE_BRIGHTNESS_FOR_WHITE_BACKGROUND(nds->GPU.GPU2D_B.MasterBrightness);

    if (isCutscene)
    {
        return gameScene_Cutscene;
    }

    if (isMainMenuOrIntroOrLoadMenu)
    {
        // Intro
        if (GameScene == -1 || GameScene == gameScene_Intro)
        {
            if (nds->GPU.GPU3D.NumVertices == 0 && nds->GPU.GPU3D.NumPolygons == 0)
            {
                return gameScene_Intro;
            }
        }

        if (isIntroLoadMenu)
        {
            return gameScene_IntroLoadMenu;
        }

        bool mayBeTitleScreen = nds->GPU.GPU3D.NumVertices == 4 && nds->GPU.GPU3D.NumPolygons == 1 && nds->GPU.GPU3D.RenderNumPolygons == 1;

        if (GameScene == gameScene_IntroLoadMenu)
        {
            if (mayBeTitleScreen)
            {
                return gameScene_TitleScreen;
            }
        }

        if (GameScene == gameScene_TitleScreen)
        {
            mayBeTitleScreen = nds->GPU.GPU3D.NumVertices < 15 && nds->GPU.GPU3D.NumPolygons < 15;
            if (mayBeTitleScreen) {
                return gameScene_TitleScreen;
            }
        }

        // Main menu
        if (mayBeTitleScreen)
        {
            return gameScene_TitleScreen;
        }

        // Intro
        if (GameScene == -1 || GameScene == gameScene_Intro)
        {
            mayBeTitleScreen = nds->GPU.GPU3D.NumVertices > 0 && nds->GPU.GPU3D.NumPolygons > 0;
            return mayBeTitleScreen ? gameScene_TitleScreen : gameScene_Intro;
        }
    }

    if (has3DOnBothScreens)
    {
        return gameScene_Other;
    }
    else if (has3DOnBottomScreen)
    {
        if (isUnplayableArea)
        {
            return gameScene_InGameMenu;
        }

        if (nds->GPU.GPU3D.RenderNumPolygons < 20)
        {
            if (nds->GPU.GPU2D_B.BlendCnt == 143 && nds->GPU.GPU2D_B.BlendAlpha == 16)
            {
                return gameScene_LoadingScreen;
            }
        }

        // Unknown
        return gameScene_Other;
    }
    else if (!has3DOnTopScreen)
    {
        // Unknown 2D
        return gameScene_Other2D;
    }

    if (ingameState == 0x07)
    {
        return gameScene_CutsceneWithStaticImages;
    }

    if (isUnplayableArea)
    {
        if (mainMenuView == 15)
        {
            return gameScene_InGameSaveMenu;
        }
        if (mainMenuView == 18)
        {
            return gameScene_Shop;
        }

        return gameScene_InGameMenu;
    }
    if (isWorldSelection)
    {
        return gameScene_WorldSelection;
    }

    if (isPauseScreen)
    {
        return gameScene_PauseMenu;
    }
    else if (GameScene == gameScene_PauseMenu)
    {
        return PriorGameScene;
    }

    if (isDeathScreen)
    {
        return gameScene_DeathScreen;
    }

    if (isResultScreenVisible())
    {
        return gameScene_ResultScreen;
    }

    if (isInGameDialog && !isComboLimitVisible())
    {
        return gameScene_InGameDialog;
    }

    u32 typeOfBattleAddr = getU32ByCart(TYPE_OF_BATTLE_ADDRESS_US, TYPE_OF_BATTLE_ADDRESS_EU, TYPE_OF_BATTLE_ADDRESS_JP);
    u32 typeOfBattle = nds->ARM7Read32(typeOfBattleAddr);
    if (typeOfBattle == 0) {
        return gameScene_InGameOlympusBattle;
    }

    // Regular gameplay
    return gameScene_InGameWithMap;
}

u32 PluginKingdomHeartsReCoded::getAspectRatioAddress()
{
    return getU32ByCart(ASPECT_RATIO_ADDRESS_US, ASPECT_RATIO_ADDRESS_EU, ASPECT_RATIO_ADDRESS_JP);
}

u32 PluginKingdomHeartsReCoded::getCutsceneAddress(CutsceneEntry* entry)
{
    return getU32ByCart(entry->usAddress, entry->euAddress, entry->jpAddress);
}

CutsceneEntry* PluginKingdomHeartsReCoded::getMobiCutsceneByAddress(u32 cutsceneAddressValue)
{
    if (cutsceneAddressValue == 0) {
        return nullptr;
    }

    CutsceneEntry* cutscene1 = nullptr;
    for (CutsceneEntry* entry = &Cutscenes[0]; entry->usAddress; entry++) {
        if (getCutsceneAddress(entry) == cutsceneAddressValue) {
            cutscene1 = entry;
        }
    }

    return cutscene1;
}

CutsceneEntry* PluginKingdomHeartsReCoded::getInEngineCutsceneById(u32 cutsceneAddressValue)
{
    // TODO: KH In engine cutscene
    if (cutsceneAddressValue == 0) {
        return nullptr;
    }

    CutsceneEntry* cutscene1 = nullptr;
    if (isInEngineCutsceneGameScene())
    {
        for (CutsceneEntry* entry = &Dialogues[0]; entry->usAddress; entry++) {
            if (getCutsceneAddress(entry) == cutsceneAddressValue) {
                cutscene1 = entry;
            }
        }

        // TODO: KH This is just temporary until we match all the cutscenes
        if (cutscene1 == nullptr &&
                cutsceneAddressValue != _lastUnknownInEngineCutsceneAddress &&
                postMessageToOsd != nullptr) {
            char message[64];
            snprintf(message, sizeof(message), "Unknown in-engine cutscene address: 0x%08X", cutsceneAddressValue);
            printf("%s\n", message);
            postMessageToOsd(message);
        }
        _lastUnknownInEngineCutsceneAddress = cutsceneAddressValue;
    }

    return cutscene1;
}

int PluginKingdomHeartsReCoded::cutsceneMenuLanguage()
{
    return GameLanguageIndex;
}

u8 PluginKingdomHeartsReCoded::getU8ByCart(u8 usAddress, u8 euAddress, u8 jpAddress)
{
    u8 cutsceneAddress = 0;
    if (isUsaCart()) {
        cutsceneAddress = usAddress;
    }
    if (isEuropeCart()) {
        cutsceneAddress = euAddress;
    }
    if (isJapanCart()) {
        cutsceneAddress = jpAddress;
    }
    return cutsceneAddress;
}

u32 PluginKingdomHeartsReCoded::getU32ByCart(u32 usAddress, u32 euAddress, u32 jpAddress)
{
    u32 cutsceneAddress = 0;
    if (isUsaCart()) {
        cutsceneAddress = usAddress;
    }
    if (isEuropeCart()) {
        cutsceneAddress = euAddress;
    }
    if (isJapanCart()) {
        cutsceneAddress = jpAddress;
    }
    return cutsceneAddress;
}

std::string PluginKingdomHeartsReCoded::getStringByCart(std::string usAddress, std::string euAddress, std::string jpAddress)
{
    std::string cutsceneAddress = "";
    if (isUsaCart()) {
        cutsceneAddress = usAddress;
    }
    else if (isEuropeCart()) {
        cutsceneAddress = euAddress;
    }
    else if (isJapanCart()) {
        cutsceneAddress = jpAddress;
    }
    return cutsceneAddress;
}

bool PluginKingdomHeartsReCoded::getBoolByCart(bool usAddress, bool euAddress, bool jpAddress)
{
    bool cutsceneAddress = false;
    if (isUsaCart()) {
        cutsceneAddress = usAddress;
    }
    else if (isEuropeCart()) {
        cutsceneAddress = euAddress;
    }
    else if (isJapanCart()) {
        cutsceneAddress = jpAddress;
    }
    return cutsceneAddress;
}

u32 PluginKingdomHeartsReCoded::detectTopScreenMobiCutsceneAddress()
{
    return getU32ByCart(CUTSCENE_ADDRESS_US, CUTSCENE_ADDRESS_EU, CUTSCENE_ADDRESS_JP);
}

u32 PluginKingdomHeartsReCoded::detectTopScreenInEngineCutsceneId()
{
    return getU32ByCart(STATIC_CUTSCENE_ADDRESS_US, STATIC_CUTSCENE_ADDRESS_EU, STATIC_CUTSCENE_ADDRESS_JP);
}

bool PluginKingdomHeartsReCoded::isMobiCutsceneGameScene()
{
    return GameScene == gameScene_Cutscene;
}

bool PluginKingdomHeartsReCoded::isInEngineCutsceneGameScene()
{
    return GameScene == gameScene_CutsceneWithStaticImages;
}

bool PluginKingdomHeartsReCoded::isCutsceneGameScene()
{
    return isMobiCutsceneGameScene() || isInEngineCutsceneGameScene();
}

bool PluginKingdomHeartsReCoded::didMobiCutsceneEnded()
{
    bool isCutsceneScene = GameScene == gameScene_Cutscene;
    if (!isCutsceneScene) {
        return true;
    }

    if (isSaveLoaded()) {
        // the old cutscene ended, and a new cutscene started
        return _CutscenesQueue.size() > 1;
    }

    return false;
}

bool PluginKingdomHeartsReCoded::didInEngineCutsceneEnded()
{
    if (!isInEngineCutsceneGameScene()) {
        return true;
    }

    u32 cutsceneAddressValue = detectTopScreenInEngineCutsceneId();
    if (cutsceneAddressValue != 0) {
        CutsceneEntry* currentCutscene = getInEngineCutsceneById(cutsceneAddressValue);
        if (currentCutscene != nullptr && currentCutscene->usAddress != _CutscenesQueue[0]->usAddress) {
            return true;
        }
    }

    return false;
}

bool PluginKingdomHeartsReCoded::canReturnToGameAfterReplacementCutscene()
{
    if (isSaveLoaded()) {
        bool isCutsceneScene = GameScene == gameScene_Cutscene;
        // either:
        // 1. the cutscene is over
        // 2. the old cutscene ended, and a new cutscene started, so it needs to be skipped as well
        // 3. the cutscene is unskippable, so even if it didn't end, we need to return
        return !isCutsceneScene || _CutscenesQueue.size() > 1 || _IsUnskippableCutscene;
    }

    return true;
}

bool PluginKingdomHeartsReCoded::isPauseMenuGameScene()
{
    return GameScene == gameScene_PauseMenu;
}

std::filesystem::path PluginKingdomHeartsReCoded::patchReplacementCutsceneIfNeeded(CutsceneEntry* cutscene, std::filesystem::path folderPath) {
    std::string filename = std::string(cutscene->MmName) + ".mp4";
    std::filesystem::path fullPath = folderPath / filename;
    if (!std::filesystem::exists(fullPath))
    {
        // TODO: KH Cutscene should be patched, if needed
    }
    if (!std::filesystem::exists(fullPath))
    {
        return "";
    }
    return fullPath;
}

std::string PluginKingdomHeartsReCoded::replacementCutsceneFilePath(CutsceneEntry* cutscene) {
    if (!HDCutscenesEnabled) {
        return "";
    }
    std::string filename = std::string(cutscene->MmName) + ".mp4";
    std::filesystem::path _assetsFolderPath = gameAssetsFolderPath();
    std::filesystem::path fullPath = _assetsFolderPath / "cutscenes" / "cinematics" / filename;
    if (std::filesystem::exists(fullPath)) {
        return fullPath.string();
    }

    filename = "hd" + std::string(cutscene->DsName) + ".mp4";
    fullPath = _assetsFolderPath / "cutscenes" / "cinematics" / filename;
    if (std::filesystem::exists(fullPath)) {
        return fullPath.string();
    }

    filename = std::string(cutscene->MmName) + ".mp4";
    fullPath = _assetsFolderPath / "cutscenes" / "dialogs" / filename;
    if (std::filesystem::exists(fullPath)) {
        return fullPath.string();
    }

    filename = "hd" + std::string(cutscene->DsName) + ".mp4";
    fullPath = _assetsFolderPath / "cutscenes" / "dialogs" / filename;
    if (std::filesystem::exists(fullPath)) {
        return fullPath.string();
    }

    std::filesystem::path collectionPath = KingdomHeartsHDCollection::path();
    if (!collectionPath.empty()) {
        std::filesystem::path newEpicFolderPath = collectionPath / "EPIC" / "Mare" / "MOVIE" / "ReCoded" / "en";
        if (std::filesystem::exists(newEpicFolderPath)) {
            std::filesystem::path newEpicFullPath = patchReplacementCutsceneIfNeeded(cutscene, newEpicFolderPath);
            if (newEpicFullPath != "") {
                return newEpicFullPath.string();
            }
        }
        std::filesystem::path newSteamFolderPath = collectionPath / "STEAM" / "Mare" / "MOVIE" / "ReCoded" / "dt";
        if (std::filesystem::exists(newSteamFolderPath)) {
            std::filesystem::path newSteamFullPath = patchReplacementCutsceneIfNeeded(cutscene, newSteamFolderPath);
            if (newSteamFullPath != "") {
                return newSteamFullPath.string();
            }
        }
    }

    return "";
}

bool PluginKingdomHeartsReCoded::isUnskippableMobiCutscene(CutsceneEntry* cutscene) {
    return false;
    // return isSaveLoaded() && strcmp(cutscene->DsName, "843") == 0;
}

std::string PluginKingdomHeartsReCoded::localizationFilePath(std::string language) {
    std::string filename = language + ".ini";
    std::filesystem::path _assetsFolderPath = gameAssetsFolderPath();
    std::filesystem::path fullPath = _assetsFolderPath / "localization" / filename;
    if (std::filesystem::exists(fullPath)) {
        // u8string(): this is handed to Platform::OpenLocalFile, which decodes it as UTF-8.
        return fullPath.u8string();
    }

    return "";
}

u8 PluginKingdomHeartsReCoded::getFloorLevel()
{
    u32 placeIdentAddr = getU32ByCart(BUG_SECTOR_IDENTIFIER_ADDRESS_US, BUG_SECTOR_IDENTIFIER_ADDRESS_EU, BUG_SECTOR_IDENTIFIER_ADDRESS_JP);
    u8 placeIdentValue = getU8ByCart(BUG_SECTOR_IDENTIFIER_VALUE_US, BUG_SECTOR_IDENTIFIER_VALUE_EU, BUG_SECTOR_IDENTIFIER_VALUE_JP);
    if (nds->ARM7Read8(placeIdentAddr) == placeIdentValue) {
        u32 floorLevelAddr = getU32ByCart(FLOOR_LEVEL_ADDRESS_US, FLOOR_LEVEL_ADDRESS_EU, FLOOR_LEVEL_ADDRESS_JP);
        return nds->ARM7Read8(floorLevelAddr);
    }
    return 0;
}

u32 PluginKingdomHeartsReCoded::getCurrentMission()
{
    return 0;
    // return nds->ARM7Read8(getU32ByCart(CURRENT_MISSION_US, CURRENT_MISSION_EU, CURRENT_MISSION_JP, CURRENT_MISSION_JP_REV1));
}

// 0 -> none
// 1 -> main menu root
// 2 -> alt main menu root (also known as save menu)
// 3 -> stat matrix
// 4 -> command matrix
// 5 -> gear matrix
// 6 -> config
// 7 -> debug reports
// 8 -> debug reports - trophies
// 9 -> debug reports - collection
// 10 -> debug reports - story
// 11 -> debug reports - enemy profiles
// 12 -> debug reports - character files
// 13 -> quest list
// 14 -> tutorials
// 15 -> save menu
// 16 -> world selection
// 17 -> challenge view before system sector
// 18 -> shop
// 19 -> stat/command/gear matrix submenu
u32 PluginKingdomHeartsReCoded::getCurrentMainMenuView()
{
    if (GameScene == -1)
    {
        return 0;
    }

    u32* topScreen = topScreen2DTexture();
    u32* bottomScreen = bottomScreen2DTexture();
    u32 pixel_2_8 = getPixel(topScreen, 2, 8, 0);
    if (pixel_2_8 == 0x04020e1e) { // debug reports
        return 10; // 7, 8, 9, 10, 11, 12
    }

    if (pixel_2_8 == 0x083a3818) {
        return 17; // challenge view before system sector
    }

    if (pixel_2_8 == 0x0802022c) {
        return 18; // shop
    }

    if (pixel_2_8 == 0x08081a00) { // alt main menu, save menu, world selection
        if (nds->GPU.GPU3D.RenderNumPolygons > 0) {
            return 16; // world selection
        }
        if (getPixel(topScreen, 10, 25, 0) == 0x08062602) {
            return 2; // alt main menu
        }
        return 15; // save menu
    }

    if (getPixel(topScreen, 4, 8, 0) == 0x082a0c02) { // main menu root, config, quest list, tutorials
        if (nds->GPU.GPU3D.RenderNumPolygons > 0) {
            return 1; // main menu root
        }
        if (getPixel(topScreen, 60, 33, 0) == 0x083c180e) {
            return 13; // quest list
        }
        if (getPixel(topScreen, 252, 190, 0) == 0x08000000) {
            return 6; // config
        }
        return 14; // tutorials
    }

    u32 pixel_11_10 = getPixel(topScreen, 11, 10, 0);
    if (pixel_11_10 == 0x1016142c) {
        return 3; // stat matrix
    }

    if (pixel_2_8 == 0x081E0802 || pixel_2_8 == 0x08240A02) {
        u32 pixel_200_4 = getPixel(bottomScreen, 200, 4, 0);
        u32 pixel_215_4 = getPixel(bottomScreen, 215, 4, 0);
        u32 pixel_230_4 = getPixel(bottomScreen, 230, 4, 0);
        if (pixel_215_4 == 0x10000000 && pixel_230_4 == 0x10000000) {
            return 3; // stat matrix
        }
        if (pixel_200_4 == 0x10000000 && pixel_230_4 == 0x10000000) {
            return 4; // command matrix
        }
        if (pixel_200_4 == 0x10000000 && pixel_215_4 == 0x10000000) {
            return 5; // gear matrix
        }
        return 19; // stat/command/gear matrix submenu
    }

    return 0; // none
}

u32 PluginKingdomHeartsReCoded::getCurrentMap()
{
    return 0;

    // if (GameScene == -1)
    // {
    //     return 0;
    // }

    // u8 world = nds->ARM7Read8(getU32ByCart(CURRENT_WORLD_US, CURRENT_WORLD_EU, CURRENT_WORLD_JP, CURRENT_WORLD_JP_REV1));
    // u8 map = nds->ARM7Read8(getU32ByCart(CURRENT_MAP_FROM_WORLD_US, CURRENT_MAP_FROM_WORLD_EU, CURRENT_MAP_FROM_WORLD_JP, CURRENT_MAP_FROM_WORLD_JP_REV1));
    // u32 fullMap = world;
    // fullMap = (fullMap << 4*2) | map;

    // if (Map != fullMap) {
    //     priorMap = Map;
    //     Map = fullMap;
    // }

    // if (Map == 128) { // cutscene
    //     return priorMap;
    // }

    // return Map;
}

bool PluginKingdomHeartsReCoded::isSaveLoaded()
{
    return getCurrentMap() != 0;
}


u16 PluginKingdomHeartsReCoded::getMidiBgmId() {
    return nds->ARM7Read16(getU32ByCart(SONG_ID_ADDRESS_US, SONG_ID_ADDRESS_EU, SONG_ID_ADDRESS_JP));
}

u8 PluginKingdomHeartsReCoded::getMidiBgmState() {
    u32 SONG_STATE_ADDRESS = getU32ByCart(SONG_ID_ADDRESS_US, SONG_ID_ADDRESS_EU, SONG_ID_ADDRESS_JP) + 0x04;
    // See enum EMidiState for details of the state
    return nds->ARM7Read8(SONG_STATE_ADDRESS);
}

u16 PluginKingdomHeartsReCoded::getMidiBgmToResumeId() {
    // This byte is set by the audio engine when a "Field" track is getting stopped and we are playing a "Battle" track.
    // This tells us that the "Field" track will resume playing when the "Battle" track ends.
    u32 SONG_SECOND_SLOT_ADDRESS = getU32ByCart(SONG_ID_ADDRESS_US, SONG_ID_ADDRESS_EU, SONG_ID_ADDRESS_JP) + 0x0C;
    return nds->ARM7Read8(SONG_SECOND_SLOT_ADDRESS);
}

s16 PluginKingdomHeartsReCoded::getSongIdInSongTable(u16 bgmId) {
    auto found = std::find_if(BgmEntries.begin(), BgmEntries.end(), [&bgmId](const auto& e) {
        return e.dsId == bgmId; });
    if(found != BgmEntries.end()) {
        return found->loadingTableId;
    }

    return 0;
}

u32 PluginKingdomHeartsReCoded::getMidiSequenceAddress(u16 bgmId) {
    const u32 songTableAddr = getU32ByCart(SSEQ_TABLE_ADDRESS_US, SSEQ_TABLE_ADDRESS_EU, SSEQ_TABLE_ADDRESS_JP);

    s16 idInTable = getSongIdInSongTable(bgmId);
    if (idInTable >= 0) {
        const u32 entryAddr = songTableAddr + (idInTable * 16) + 4;
        return nds->ARM9Read32(entryAddr);
    }

    return 0;
}

u16 PluginKingdomHeartsReCoded::getMidiSequenceSize(u16 bgmId) {
    const u32 songTableAddr = getU32ByCart(SSEQ_TABLE_ADDRESS_US, SSEQ_TABLE_ADDRESS_EU, SSEQ_TABLE_ADDRESS_JP);

    s16 idInTable = getSongIdInSongTable(bgmId);
    if (idInTable >= 0) {
        const u32 songSizeAddr = songTableAddr + (idInTable * 16);
        return nds->ARM9Read32(songSizeAddr);
    }

    return 0;
}

u32 PluginKingdomHeartsReCoded::getStreamBgmAddress() {
    return getU32ByCart(STRM_ADDRESS_US, STRM_ADDRESS_EU, STRM_ADDRESS_JP);
}

u16 PluginKingdomHeartsReCoded::getStreamBgmCustomIdFromDsId(u8 dsId, u32 numSamples) {
    auto found = std::find_if(StreamedBgmEntries.begin(), StreamedBgmEntries.end(), [&](const auto& e) {
        return e.dsId == dsId && e.numSamples == numSamples; });
    if(found != StreamedBgmEntries.end()) {
        return found->customId;
    }

    return BGM_INVALID_ID;
}

u8 PluginKingdomHeartsReCoded::getMidiBgmVolume() {
    u32 SONG_MASTER_VOLUME_ADDRESS = getU32ByCart(SONG_ID_ADDRESS_US, SONG_ID_ADDRESS_EU, SONG_ID_ADDRESS_JP) + 0x05;
    // Usually 0x7F during gameplay and 0x40 when game is paused
    return nds->ARM7Read8(SONG_MASTER_VOLUME_ADDRESS);
}

u32 PluginKingdomHeartsReCoded::getBgmFadeOutDuration() {
    // Caution: this RAM value should be queried on the first frame when the "Stopping" state was set, otherwise fadeout is already in progress!
    u32 SONG_FADE_OUT_PROGRESS_ADDRESS = getU32ByCart(SONG_ID_ADDRESS_US, SONG_ID_ADDRESS_EU, SONG_ID_ADDRESS_JP) + 0x06;
    u8 progress = nds->ARM7Read8(SONG_FADE_OUT_PROGRESS_ADDRESS);
    // converts value in game frames to milliseconds + some smoothing
    float valueMs = (progress / 30.0f);
    if (progress >= 59) { valueMs *= 1.2; }
    else if (progress >= 29) { valueMs *= 1.5; }
    else { valueMs *= 2; }

    return std::round(valueMs * 1000);
}

std::string PluginKingdomHeartsReCoded::getBackgroundMusicName(u16 bgmId) {
    auto found = std::find_if(BgmEntries.begin(), BgmEntries.end(), [&bgmId](const auto& e) {
        return e.dsId == bgmId; });
    if(found != BgmEntries.end()) {
        return found->Name;
    }

    return "Unknown";
}

void PluginKingdomHeartsReCoded::onStreamBgmReplacementStarted() {
    Plugin::onStreamBgmReplacementStarted();

    // Reset the number of muted blocks
    _MutedStreamBlocksCount = 0;
}

void PluginKingdomHeartsReCoded::muteStreamedMusic() {
    Plugin::muteStreamedMusic();

    if (_MutedStreamBlocksCount == 0) {
        // We only need to mute the first streamed block in the audio buffer, to fix
        // a small audio glitch. Then, muting "normally" by setting volume to O is enough
        const u32 strmHeaderAddress = getStreamBgmAddress();
        const u32 streamBufferAddress = nds->ARM9Read32(strmHeaderAddress + 0x84);
        const u32 streamBufferSize = nds->ARM9Read32(strmHeaderAddress + 0x88);
    
        u32 firstValue = nds->ARM9Read32(streamBufferAddress);
        if (firstValue != 0) {
            u32 startErase = streamBufferAddress;
            u32 endErase = streamBufferAddress + streamBufferSize;
            for (u32 addr = startErase; addr < endErase; addr+=4) {
                nds->ARM7Write32(addr, 0x00);
            }
        }

        //printf("Erased stream block! Address: 0x%08x Size: %d\n", streamBufferAddress, streamBufferSize);
        _MutedStreamBlocksCount++;
    }
}


void PluginKingdomHeartsReCoded::debugLogs(int gameScene)
{
    // PRINT_AS_32_BIT_HEX(0x020dacd0);
    // PRINT_AS_32_BIT_HEX(0x020b7db8);
    // printf("\n");

    if (!DEBUG_MODE_ENABLED) {
        return;
    }

    printf("Game scene: %d\n", GameScene);
    printf("Game scene state: %d\n", GameSceneState);
    printf("Current map: %d\n", getCurrentMap());
    printf("Current main menu view: %d\n", getCurrentMainMenuView());
    printf("Is save loaded: %d\n", isSaveLoaded() ? 1 : 0);
    printf("\n");
}

}
