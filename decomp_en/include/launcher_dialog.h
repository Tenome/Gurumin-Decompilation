// EN-only launcher/config dialogs ("Gurumin Ver.1.4 (STEAM)"): the MFC dialog classes Falcom wrote on top of static MFC.
//
// This header is the ONLY place that pulls in the MFC headers (afxwin.h, afxcmn.h). It is included by the launcher
// source files (src/launcher_dialog.cpp and friends) and by nothing else, so no shared game header ever sees MFC.
//
// Build setup (see NOTES.md "Launcher (MFC)"): the game links MFC statically (/MT, no _AFXDLL), so the MFC headers
// are used exactly as the build flags deliver them. VS2013 Community Update 4 ships them in VC\atlmfc\include
// (vcvars32.bat puts that directory on INCLUDE). The exe has no _UNICODE and no _MBCS define: the launcher uses the
// ANSI entry points (SendMessageA, SetWindowTextA) and char strings.
#ifndef GURUMIN_EN_LAUNCHER_DIALOG_H
#define GURUMIN_EN_LAUNCHER_DIALOG_H

// The Update 4 headers declare one more CDialog member than the MFC the game was built with (the 12.0.21005 MBCS
// libraries): `BOOL m_bClosedByEndDialog;` sits between m_hWndTop and m_pOccDialogInfo, which makes sizeof(CDialog)
// 0x98 instead of the 0x94 that every Falcom dialog class in the exe is laid out on (first derived member at 0x94).
// Turn that one declaration into an unnamed zero-width bit-field so the layout matches the exe; the macro is removed
// again right after the includes. MFC code that uses the member is in the MFC library, not in these headers.
#define m_bClosedByEndDialog :0
#include "launcher.h"   // the launcher helpers that need no MFC (Launcher_SaveSettings, ...)
#include <afxwin.h>
#include <afxcmn.h>
#include <uxtheme.h>   // SetWindowTheme (the launcher links uxtheme.lib)
#undef m_bClosedByEndDialog

// The layout rule the macro above establishes (MFC objects in the exe: CDialog is 0x94 bytes, a control wrapper 0x74).
static_assert(sizeof(CDialog) == 0x94, "CDialog must be 0x94 bytes like the MFC the exe was built with");
static_assert(sizeof(CButton) == 0x74, "control wrappers are 0x74 bytes");

// ---- Dialog template ids (RT_DIALOG resources of the exe) ------------------------------------------------------
enum LauncherDialogId {
    IDD_VIDEO_DIALOG = 0x82,     // "Graphics" / "Audio" settings page (CVideoDlg)
    IDD_PAD_DIALOG = 0x81,       // gamepad button assignment page (CPadDlg)
    IDD_NET_DIALOG = 0x83,       // network / lobby server page (CNetDlg)
    IDD_OSE_DIALOG = 0x84,       // "Osero" (Othello) mini game page (COseDlg)
    IDD_TITLE_DIALOG = 0x8b,     // title / progress page (CTitDlg)
    IDD_INSTALL_DIALOG = 0x8d,   // "installing, please wait" notice (CInstDlg)
    IDD_ADVANCED_DIALOG = 0x8e,  // "Advanced Option" dialog (CAdvancedOptionDlg)
    IDD_KEYBOARD_DIALOG = 0x8f   // keyboard key assignment page (CKbDlg)
};

// ---- Control ids of the Graphics / Audio page (names from the dialog template, RT_DIALOG 130) ---------------------
// The Advanced Option dialog (RT_DIALOG 142) reuses the same ids for its own copies of the graphics switches.
enum LauncherControlId {
    IDC_PLAY_VOICES = 0x3eb,            // "Play Voices" check box
    IDC_HIGH_QUALITY_TEXTURE = 0x3ec,   // "High Quality Texture" check box
    IDC_WINDOWED = 0x3ed,               // "Windowed" check box
    IDC_RESOLUTION = 0x3ee,             // "Resolution" combo box
    IDC_USE_BACKBUFFER = 0x3ef,         // "Use BackBuffer" check box
    IDC_DIRECTX81 = 0x3f0,              // "DirectX 8.1" radio button (first of the renderer group)
    IDC_DELAY_MS = 0x3f2,               // edit box: delay in milliseconds
    IDC_AUDIO_ENABLE = 0x3f3,           // "Enable" radio button (first of the audio on/off group)
    IDC_UNCOMPRESSED_TEXTURES = 0x3f7,  // "Uncompressed Textures" check box
    IDC_REFLECTIONS = 0x3f8,            // "Reflections" check box
    IDC_DIRECT_SOUND_3D = 0x3fb,        // "DirectSound3D" check box
    IDC_DEPTH_OF_FIELD = 0x3fc,         // "Depth Of Field" check box
    IDC_CROSS_FADE = 0x3fe,             // "Closs Fade" (sic) check box
    IDC_BLUR_EFFECT = 0x3ff,            // "Blur Effect" check box
    IDC_CACHE_AT_STARTUP = 0x400,       // generate the cache at start-up check box
    IDC_FOG_EFFECT = 0x401,             // "Fog Effect" check box
    IDC_HW_VERTEX_SHADER = 0x402,       // hardware vertex shader priority check box
    IDC_COMBO_403 = 0x403,              // combo box (memory/quality list, purpose not yet identified)
    IDC_PRESET = 0x404,                 // "Preset" combo box
    IDC_SHADOW_QUALITY = 0x405,         // "Shadow Quality" combo box (Low, Medium, High, Ultra)
    IDC_SHADOW_COMPLEXITY = 0x406,      // "Shadow Complexity" combo box (Low, Medium, High, Ultra)
    IDC_COMBO_407 = 0x407,              // combo box (memory/quality list, purpose not yet identified)
    IDC_SYNC = 0x408,                   // synchronise check box
    IDC_GAMMA_SETTINGS = 0x409,         // "Gamma Settings" check box
    IDC_COMPLEXITY_SHADER = 0x40a,      // "Complexity Shader" check box
    IDC_COLOR_32BIT = 0x40b,            // "32bitColor" check box
    IDC_DIRECTX9 = 0x40c,               // "DirectX 9" radio button (second of the renderer group)
    IDC_REFLECTION_EFFECT = 0x40d,      // "Reflaction Effect" (sic) check box
    IDC_COMBO_40E = 0x40e,              // combo box (purpose not yet identified)
    IDC_COMBO_40F = 0x40f,              // combo box (purpose not yet identified)
    IDC_CONTRAST = 0x410,               // "Contrast" slider
    IDC_BRIGHTNESS = 0x411,             // "Brightness" slider
    IDC_BGM_VOLUME = 0x412,             // "BGM Volume" slider
    IDC_SE_VOLUME = 0x413,              // sound-effect volume slider
    IDC_COMBO_414 = 0x414,              // combo box (purpose not yet identified)
    IDC_COMBO_415 = 0x415,              // combo box (purpose not yet identified)
    IDC_SKIP_VIDEO = 0x416,             // "Skip Video" check box
    IDC_VSYNC_OFF = 0x417,              // "Vsync Off" check box
    IDC_ADVANCED = 0x41f,               // "Advanced..." button
    IDC_TEXTURE_FILTERING = 0x425,      // "Texture Filtering" combo box
    IDC_MULTI_SAMPLING = 0x426          // "Multi Sampling" combo box
};

// ---- Control ids of the keyboard page (RT_DIALOG 143): one combo box per action, plus three mouse options --------
enum LauncherKeyboardControlId {
    IDC_KB_MOVE_LEFT = 0x3ff,          // "Move Left" key list
    IDC_KB_DASH = 0x3fe,               // "Dash" key list
    IDC_KB_MOVE_FORWARD = 0x400,       // "Move Forward" key list
    IDC_KB_ATTACK = 0x401,             // "Attack / OK / Action" key list
    IDC_KB_CAMERA_FORWARD = 0x402,     // "Camera: Show Forward" key list
    IDC_KB_MOVE_RIGHT = 0x404,         // "Move Right" key list
    IDC_KB_MOVE_BACKWARD = 0x405,      // "Move Backward" key list
    IDC_KB_JUMP = 0x406,               // "Jump / Cancel" key list
    IDC_KB_CAMERA_ENEMY = 0x407,       // "Camera: Show Enemy" key list
    IDC_KB_CAMERA_LEFT = 0x40f,        // "Camera: Show Left" key list
    IDC_KB_MENU = 0x415,               // "Menu On / Off" key list
    IDC_KB_CAMERA_RIGHT = 0x418,       // "Camera: Show Right" key list
    IDC_KB_SWAP_ATTACK_MOVE = 0x41b,   // "Swap Attack / Move Button" check box
    IDC_KB_WHEEL_CAMERA = 0x41c,       // "Rotate Camera With Wheel" check box
    IDC_KB_DASH_DOUBLE_CLICK = 0x41e   // "Dash: Double Right Click" check box
};

// Graphics / Audio settings page of the launcher (dialog template IDD_VIDEO_DIALOG). It is a plain wizard-style
// dialog: every setting is an int field exchanged with its control by DoDataExchange, and a second set of CWnd
// objects subclasses the same controls. Created by the launcher's tab dialog; its "Advanced..." button opens the
// Advanced Option dialog (CAdvancedOptionDlg).
class CVideoDlg : public CDialog {
public:
    CVideoDlg(CWnd *pParent = NULL);   // EN 0x006f06f0

    enum { IDD = IDD_VIDEO_DIALOG };

    int m_windowed;                  // 0x094 IDC_WINDOWED check state
    int m_highQualityTexture;        // 0x098 IDC_HIGH_QUALITY_TEXTURE check state
    int m_hwVertexShader;            // 0x09c IDC_HW_VERTEX_SHADER check state
    int m_resolution;                // 0x0a0 IDC_RESOLUTION selected row (-1 none)
    int m_uncompressedTextures;      // 0x0a4 IDC_UNCOMPRESSED_TEXTURES check state
    int m_audioEnable;               // 0x0a8 IDC_AUDIO_ENABLE radio group index (-1 none)
    int m_antiAlias;                 // 0x0ac ini key ANTI, loaded by Launcher_LoadConfig; not touched by the constructor or DoDataExchange
    int m_textureFiltering;          // 0x0b0 IDC_TEXTURE_FILTERING selected row
    int m_multiSampling;             // 0x0b4 IDC_MULTI_SAMPLING selected row
    int m_depthOfField;              // 0x0b8 IDC_DEPTH_OF_FIELD check state
    int m_reflections;               // 0x0bc IDC_REFLECTIONS check state
    int m_crossFade;                 // 0x0c0 IDC_CROSS_FADE check state
    int m_shadowQuality;             // 0x0c4 IDC_SHADOW_QUALITY selected row
    int m_shadowComplexity;          // 0x0c8 IDC_SHADOW_COMPLEXITY selected row
    int m_combo403;                  // 0x0cc IDC_COMBO_403 selected row
    int m_preset;                    // 0x0d0 IDC_PRESET selected row
    int m_blurEffect;                // 0x0d4 IDC_BLUR_EFFECT check state
    int m_cacheAtStartup;            // 0x0d8 IDC_CACHE_AT_STARTUP check state
    int m_combo407;                  // 0x0dc IDC_COMBO_407 selected row
    CButton m_cacheAtStartupCtrl;    // 0x0e0
    CComboBox m_combo403Ctrl;        // 0x154
    CComboBox m_combo407Ctrl;        // 0x1c8
    int m_fogEffect;                 // 0x23c IDC_FOG_EFFECT check state
    int m_delayMs;                   // 0x240 IDC_DELAY_MS value
    int m_contrast;                  // 0x244 IDC_CONTRAST slider position
    int m_brightness;                // 0x248 IDC_BRIGHTNESS slider position
    int m_gammaSettings;             // 0x24c IDC_GAMMA_SETTINGS check state
    int m_sync;                      // 0x250 IDC_SYNC check state
    int m_complexityShader;          // 0x254 IDC_COMPLEXITY_SHADER check state
    int m_useBackBuffer;             // 0x258 IDC_USE_BACKBUFFER check state
    int m_color32bit;                // 0x25c IDC_COLOR_32BIT check state
    int m_directX81;                 // 0x260 IDC_DIRECTX81 radio group index
    int m_directX9;                  // 0x264 IDC_DIRECTX9 radio group index
    int m_bgmVolume;                 // 0x268 IDC_BGM_VOLUME slider position
    int m_seVolume;                  // 0x26c IDC_SE_VOLUME slider position
    int m_directSound3D;             // 0x270 IDC_DIRECT_SOUND_3D check state
    int m_combo40e;                  // 0x274 IDC_COMBO_40E selected row
    int m_combo40f;                  // 0x278 IDC_COMBO_40F selected row
    int m_combo414;                  // 0x27c IDC_COMBO_414 selected row
    int m_reflectionEffect;          // 0x280 IDC_REFLECTION_EFFECT check state
    CSliderCtrl m_bgmVolumeCtrl;     // 0x284
    CButton m_directSound3DCtrl;     // 0x2f8
    CComboBox m_combo414Ctrl;        // 0x36c
    CButton m_syncCtrl;              // 0x3e0
    CEdit m_delayMsCtrl;             // 0x454
    CComboBox m_combo40fCtrl;        // 0x4c8
    CComboBox m_combo40eCtrl;        // 0x53c
    int m_05b0;                      // 0x5b0 zeroed by the constructor, otherwise unused
    CButton m_audioEnableCtrl;       // 0x5b4
    CButton m_gammaSettingsCtrl;     // 0x628
    CSliderCtrl m_contrastCtrl;      // 0x69c
    CSliderCtrl m_brightnessCtrl;    // 0x710
    CButton m_0784;                  // 0x784 constructed but never bound by DoDataExchange
    CButton m_hwVertexShaderCtrl;    // 0x7f8
    CButton m_color32bitCtrl;        // 0x86c
    CButton m_directX81Ctrl;         // 0x8e0
    int m_0954;                      // 0x954 zeroed by the constructor, otherwise unused
    CComboBox m_shadowQualityCtrl;   // 0x958
    CComboBox m_shadowComplexityCtrl;   // 0x9cc
    CButton m_highQualityTextureCtrl;   // 0xa40
    CButton m_depthOfFieldCtrl;         // 0xab4
    CButton m_uncompressedTexturesCtrl; // 0xb28
    CButton m_reflectionsCtrl;          // 0xb9c
    CButton m_crossFadeCtrl;            // 0xc10
    CButton m_blurEffectCtrl;           // 0xc84
    CButton m_fogEffectCtrl;            // 0xcf8
    CButton m_complexityShaderCtrl;     // 0xd6c
    CButton m_useBackBufferCtrl;        // 0xde0
    CButton m_reflectionEffectCtrl;     // 0xe54
    CComboBox m_presetCtrl;             // 0xec8
    CButton m_windowedCtrl;             // 0xf3c
    CComboBox m_resolutionCtrl;         // 0xfb0
    int m_playVoices;                   // 0x1024 IDC_PLAY_VOICES check state
    CButton m_playVoicesCtrl;           // 0x1028
    CComboBox m_combo415Ctrl;           // 0x109c
    int m_combo415;                     // 0x1110 IDC_COMBO_415 selected row
    CButton m_skipVideoCtrl;            // 0x1114
    int m_skipVideo;                    // 0x1188 IDC_SKIP_VIDEO check state
    int m_vsyncOff;                     // 0x118c IDC_VSYNC_OFF check state
    CButton m_vsyncOffCtrl;             // 0x1190

protected:
    virtual void DoDataExchange(CDataExchange *pDX);   // EN 0x006f0cf0
    afx_msg void OnAdvancedOption();                     // EN 0x006f1550 (IDC_ADVANCED button)
    afx_msg HBRUSH OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor);   // EN 0x006e9350
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);   // EN 0x006e93c0
    afx_msg BOOL OnEraseBkgnd(CDC *pDC);                                  // EN 0x006e93e0
    afx_msg void OnPaint();                                               // EN 0x006ea4f0 (paints nothing)
    afx_msg void OnNcPaint();                                             // EN 0x006ea440 (draws nothing)
    afx_msg void OnGammaSettings();                                       // EN 0x006f1280 (IDC_GAMMA_SETTINGS, does nothing)
    afx_msg void OnUseBackBuffer();                                       // EN 0x006f1290
    afx_msg void OnSelchangeCombo415();                                   // EN 0x006f1300
    afx_msg void OnCombo401();                                            // EN 0x006f1540 (does nothing)
    virtual BOOL OnInitDialog();                                          // EN 0x006ea3d0
    virtual BOOL PreTranslateMessage(MSG *pMsg);                          // EN 0x006ea660
    DECLARE_MESSAGE_MAP()
};

// Advanced Option dialog opened by the "Advanced..." button of the Graphics page: eleven graphics switches that the
// page copies in on construction. Constructed by CVideoDlg::OnAdvancedOption (EN 0x006f1550).
class CAdvancedOptionDlg : public CDialog {
    DECLARE_DYNAMIC(CAdvancedOptionDlg)
public:
    // Copies the graphics switches out of the Graphics page. EN 0x006e5d60.
    CAdvancedOptionDlg(CVideoDlg *config, CWnd *pParent = NULL);
    virtual ~CAdvancedOptionDlg();   // EN 0x006e5ed0 (out of line; the scalar deleting form is 0x006e5f70)

    // Template id used by the wizard-style DDX; EN 0x006e5d60 passes IDD_ADVANCED_DIALOG to CDialog.
    enum { IDD = IDD_ADVANCED_DIALOG };

    int m_highQualityTexture;     // 0x094 IDC_HIGH_QUALITY_TEXTURE check state
    int m_uncompressedTextures;   // 0x098 IDC_UNCOMPRESSED_TEXTURES check state
    int m_crossFade;              // 0x09c IDC_CROSS_FADE check state
    int m_fogEffect;              // 0x0a0 IDC_FOG_EFFECT check state
    int m_useBackBuffer;          // 0x0a4 IDC_USE_BACKBUFFER check state
    int m_shadowQuality;          // 0x0a8 IDC_SHADOW_QUALITY selected row (0 Low .. 3 Ultra)
    int m_depthOfField;           // 0x0ac IDC_DEPTH_OF_FIELD check state
    int m_reflections;            // 0x0b0 IDC_REFLECTIONS check state
    int m_blurEffect;             // 0x0b4 IDC_BLUR_EFFECT check state
    int m_complexityShader;       // 0x0b8 IDC_COMPLEXITY_SHADER check state
    int m_shadowComplexity;       // 0x0bc IDC_SHADOW_COMPLEXITY selected row (0 Low .. 3 Ultra)
    CButton m_reflectionEffectCtrl;       // 0x0c0 IDC_REFLECTION_EFFECT control
    int m_reflectionEffect;               // 0x134 IDC_REFLECTION_EFFECT check state
    CVideoDlg *m_config;                  // 0x138 the Graphics page the values were copied from
    CComboBox m_shadowQualityCtrl;        // 0x13c IDC_SHADOW_QUALITY control
    CComboBox m_shadowComplexityCtrl;     // 0x1b0 IDC_SHADOW_COMPLEXITY control

protected:
    virtual void DoDataExchange(CDataExchange *pDX);   // EN 0x006e60b0
    virtual BOOL OnInitDialog();                        // EN 0x006e6340
    afx_msg void OnBnClickedOk();                       // EN 0x006e6240 (IDOK handler)
    DECLARE_MESSAGE_MAP()
};

// "Installing, please wait" notice shown while the launcher updates the game (dialog template IDD_INSTALL_DIALOG).
// A bare dialog: no members and nothing exchanged.
class CInstDlg : public CDialog {
    DECLARE_DYNAMIC(CInstDlg)
public:
    CInstDlg(CWnd *pParent = NULL);   // EN 0x006eb600
    virtual ~CInstDlg();              // folded into the scalar deleting destructor EN 0x006eb630

    enum { IDD = IDD_INSTALL_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange *pDX);   // EN 0x006eb660 (empty)
    DECLARE_MESSAGE_MAP()
};

// Keyboard key assignment page (dialog template IDD_KEYBOARD_DIALOG): one drop-down list per action, each holding
// the same list of keys, and the page keeps the assignments unique (picking a key that another action already uses
// clears that other action back to row 0 "not assigned").
class CKbDlg : public CDialog {
    DECLARE_DYNAMIC(CKbDlg)
public:
    CKbDlg(CWnd *pParent = NULL);   // EN 0x006eb6a0
    virtual ~CKbDlg();              // folded into the scalar deleting destructor EN 0x006eb770

    enum { IDD = IDD_KEYBOARD_DIALOG };

    int m_dash;                    // 0x094 IDC_KB_DASH selected row
    int m_cameraEnemy;             // 0x098 IDC_KB_CAMERA_ENEMY selected row
    int m_cameraLeft;              // 0x09c IDC_KB_CAMERA_LEFT selected row
    int m_cameraRight;             // 0x0a0 IDC_KB_CAMERA_RIGHT selected row
    int m_cameraForward;           // 0x0a4 IDC_KB_CAMERA_FORWARD selected row
    int m_menu;                    // 0x0a8 IDC_KB_MENU selected row
    int m_jump;                    // 0x0ac IDC_KB_JUMP selected row
    int m_attack;                  // 0x0b0 IDC_KB_ATTACK selected row
    int m_moveForward;             // 0x0b4 IDC_KB_MOVE_FORWARD selected row
    int m_moveRight;               // 0x0b8 IDC_KB_MOVE_RIGHT selected row
    int m_moveLeft;                // 0x0bc IDC_KB_MOVE_LEFT selected row
    int m_moveBackward;            // 0x0c0 IDC_KB_MOVE_BACKWARD selected row
    int m_swapAttackMove;          // 0x0c4 IDC_KB_SWAP_ATTACK_MOVE check state
    int m_wheelCamera;             // 0x0c8 IDC_KB_WHEEL_CAMERA check state
    int m_dashDoubleClick;         // 0x0cc IDC_KB_DASH_DOUBLE_CLICK check state

    // Returns true when the player should not start: a key list is still on row 0 and the player refused the
    // default input setup offered by the warning box. EN 0x006eb860.
    bool IsStartRefused(CWnd *owner);

protected:
    virtual void DoDataExchange(CDataExchange *pDX);   // EN 0x006eb950
    virtual BOOL OnInitDialog();                        // EN 0x006ebe60
    // Called after the key list `sourceId` changed to row `row`: the list that already shows `row` is reset to row 0.
    // EN 0x006eb7a0.
    void ClearDuplicateKey(int row, int sourceId);
    afx_msg void OnSelchangeMoveForward();    // EN 0x006ebc80
    afx_msg void OnSelchangeMoveLeft();       // EN 0x006ebc30
    afx_msg void OnSelchangeMoveRight();      // EN 0x006ebd70
    afx_msg void OnSelchangeMoveBackward();   // EN 0x006ebdc0
    afx_msg void OnSelchangeMenu();           // EN 0x006ebb40
    afx_msg void OnSelchangeDash();           // EN 0x006ebbe0
    afx_msg void OnSelchangeJump();           // EN 0x006ebe10
    afx_msg void OnSelchangeAttack();         // EN 0x006ebcd0
    afx_msg void OnSelchangeCameraForward();  // EN 0x006ebd20
    afx_msg void OnSelchangeCameraLeft();     // EN 0x006ebaf0
    afx_msg void OnSelchangeCameraRight();    // EN 0x006ebb90
    afx_msg void OnSelchangeCameraEnemy();    // EN 0x006ebaa0
    DECLARE_MESSAGE_MAP()
};

// ---- Control ids of the network page (RT_DIALOG 131) and the title page (RT_DIALOG 139) -------------------------------
enum LauncherNetControlId {
    IDC_NET_NO_PATCH = 0x3eb,           // check box: do not apply patches at start-up
    IDC_NET_LOBBY_ADDRESS = 0x3f1       // edit box: address of the lobby server
};
enum LauncherTitleControlId {
    IDC_TITLE_ONLINE_UPDATE = 0x3ff,    // "Online Update(&L)" push button
    IDC_TITLE_PROGRESS_A = 0x415,       // first update progress bar
    IDC_TITLE_VERSION_TEXT = 0x416,     // static text: "Version: yyyy/mm/dd Rev.n"
    IDC_TITLE_UPDATE_STATUS_B = 0x417,  // static text of the update progress (hidden in this build)
    IDC_TITLE_UPDATE_STATUS = 0x418,    // static text: update check status message
    IDC_TITLE_PROGRESS_B = 0x419,       // second update progress bar
    IDC_TITLE_UPDATE_SERVER = 0x41a,    // static text: "UpdateServer:" label
    IDC_TITLE_COPYRIGHT = 0x41b,        // static text: copyright line
    IDC_TITLE_HIDDEN_424 = 0x424        // control hidden at start-up (purpose unknown)
};

// Network page (dialog template IDD_NET_DIALOG): the lobby server address and the "no patch at start-up" switch.
class CNetDlg : public CDialog {
public:
    CNetDlg(CWnd *pParent = NULL);   // EN 0x006ec0f0
                                     // (implicit destructor: releases m_lobbyAddress; scalar deleting form EN 0x006ec190)

    enum { IDD = IDD_NET_DIALOG };

    CString m_lobbyAddress;   // 0x094 IDC_NET_LOBBY_ADDRESS text
    int m_noPatch;            // 0x098 IDC_NET_NO_PATCH check state

protected:
    virtual void DoDataExchange(CDataExchange *pDX);   // EN 0x006ec210
    afx_msg HBRUSH OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor);   // EN 0x006e9290
    DECLARE_MESSAGE_MAP()
};

// Title page (dialog template IDD_TITLE_DIALOG): the "Online Update" button and the two update progress bars.
class CTitDlg : public CDialog {
    DECLARE_DYNAMIC(CTitDlg)
public:
    CTitDlg(CWnd *pParent = NULL);   // EN 0x006f0470
    virtual ~CTitDlg();              // EN 0x006f0510, scalar deleting form 0x006f05c0
    virtual BOOL OnInitDialog();     // EN 0x006ea250

    enum { IDD = IDD_TITLE_DIALOG };

    CButton m_onlineUpdateCtrl;     // 0x094 IDC_TITLE_ONLINE_UPDATE
    CProgressCtrl m_progressACtrl;  // 0x108 IDC_TITLE_PROGRESS_A
    CProgressCtrl m_progressBCtrl;  // 0x17c IDC_TITLE_PROGRESS_B

protected:
    virtual void DoDataExchange(CDataExchange *pDX);   // EN 0x006f0650
    afx_msg HBRUSH OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor);   // EN 0x006e9320
    DECLARE_MESSAGE_MAP()
};
extern CTitDlg *g_titlePageThis;        // the title page object, set by CTitDlg::OnInitDialog (EN 0x0161274c)

// ---- "Osero" (Othello) mini game page (RT_DIALOG 132): a hidden game on the 128x128 client area --------------------
enum LauncherOseControlId {
    IDC_OSE_STARTED = 0x3eb,       // push-like check box: pressed once the setup phase is over
    IDC_OSE_PLAYER_BLACK = 0x3ef,  // radio button: the player takes the black pieces (first of the colour group)
    IDC_OSE_RESET = 0x3ff,         // "Reset" button: empties the board
    IDC_OSE_PASS = 0x400           // pass button: hands the move to the computer
};
const int OSE_BOARD_SIZE = 8;               // cells per board side
const int OSE_CELL_COUNT = 64;              // OSE_BOARD_SIZE * OSE_BOARD_SIZE
const int OSE_BOARD_WORDS = 4;              // the packed board: 16 two-bit cells per 32-bit word
const int OSE_GRID_LINES = 9;                // lines of the board grid in each direction (OSE_BOARD_SIZE + 1)
const int OSE_DIRECTIONS = 8;               // neighbours of a cell (the 8 compass directions)
const int OSE_CELL_PIXELS = 16;             // width and height of a board cell on screen
const COLORREF OSE_COLOR_BLACK = 0x000000;   // colour of the grid lines and black pieces
const COLORREF OSE_COLOR_WHITE = 0xfafafa;   // colour of white pieces
const unsigned char OSE_CELL_EMPTY = 0;     // cell state: no piece
const unsigned char OSE_CELL_BLACK = 1;     // cell state: black piece
const unsigned char OSE_CELL_WHITE = 2;     // cell state: white piece

extern int g_oseBoard[OSE_BOARD_WORDS];   // the board, 2 bits per cell, row major (EN 0x01614d60)
extern int g_oseComputerTurnPending;               // non-zero while the computer owes a move (EN 0x01614d70)
extern int g_oseChosenColumn;                      // column the move search chose, -1 = no move (EN 0x01614d74)
extern int g_oseChosenRow;                         // row of the chosen move (EN 0x01615298)
extern int g_oseDirectionColumn[OSE_DIRECTIONS];   // column step of each of the 8 directions (EN 0x00908990)
extern int g_oseDirectionRow[OSE_DIRECTIONS];      // row step of each of the 8 directions (EN 0x009089b0)

const int OSE_SEARCH_DEPTH = 7;             // plies the exhaustive move search looks ahead
const int OSE_SEARCH_TABLE_SIZE = 164;      // entries the original reserves in the per-ply move tables
extern int g_oseMoveColumn[OSE_SEARCH_TABLE_SIZE];   // column of the move tried at each ply of the search (EN 0x01614d78)
extern int g_oseMoveRow[OSE_SEARCH_TABLE_SIZE];      // row of the move tried at each ply of the search (EN 0x01615008)
extern int g_oseBestCount;                           // most pieces of the counted colour reached so far (EN 0x0161529c)

// Move search of the computer player (EN 0x006ecb50): tries every empty cell for `color` (1 black, 2 white) on the
// unpacked board `cells`, plays it and recurses one ply deeper for the other colour (a side without a move passes).
// At the end of a line (board full or one colour gone, or OSE_SEARCH_DEPTH plies) it counts the pieces of
// `countColor`; the first move of the line with the highest count is left in g_oseChosenColumn/g_oseChosenRow.
// Always returns 0 (the result is unused). Called by COseDlg::OnTimer with depth 0.
int Ose_SearchMove(unsigned char *cells, int depth, int color, int countColor);
// The same search on a packed board (EN 0x006ecf40): unpacks it itself and packs every child board before recursing.
// Never called in the exe (dead code next to Ose_SearchMove); only recursion.
int Ose_SearchMovePacked(int *board, int depth, int color, int countColor);
// Plays a move of `color` at (column, row) on the packed board: flips the opponent pieces it captures in the 8
// directions and repacks the board. Returns 1 when the move was legal, 0 when it flips nothing or the cell is taken
// (EN 0x006ec360). Called by COseDlg::OnTimer (computer) and COseDlg::OnLButtonDown (player).
int Ose_PlayMove(int *board, unsigned int column, unsigned int row, int color);

// Othello page (dialog template IDD_OSE_DIALOG): the player plays one colour against the computer. The timer
// (every 10 ms) lets the computer move once the player has moved or passed.
class COseDlg : public CDialog {
public:
    COseDlg(CWnd *pParent = NULL);   // EN 0x006ec270

    enum { IDD = IDD_OSE_DIALOG };

    int m_playerColor;   // 0x094 IDC_OSE_PLAYER_BLACK radio group index (0 black, 1 white; -1 none)
    int m_started;       // 0x098 IDC_OSE_STARTED check state

protected:
    virtual void DoDataExchange(CDataExchange *pDX);   // EN 0x006ec2e0
    virtual BOOL OnInitDialog();                        // EN 0x006ec590
    afx_msg void OnReset();                             // EN 0x006ec4e0
    afx_msg void OnPass();                              // EN 0x006ec580
    afx_msg void OnTimer(UINT_PTR nIDEvent);            // EN 0x006eca20 (10 ms game timer: the computer's move)
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);   // EN 0x006ec5d0 (setup: black piece, else the player's move)
    afx_msg void OnPaint();                                  // EN 0x006ec790 (draws the board)
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);   // EN 0x006ec8f0 (setup phase: puts a white piece)
    DECLARE_MESSAGE_MAP()
};

// Application object of the launcher: a CWinApp whose only own code is InitInstance, which builds the page dialogs and
// the main dialog (CCongigDlg), runs it and starts the game. The standard Help command is routed to CWinApp::OnHelp.
class CLauncherApp : public CWinApp {
public:
    CLauncherApp();                      // EN 0x006e6460
    virtual BOOL InitInstance();         // EN 0x006e7470 (not yet reconstructed)
                                         // (implicit destructor EN 0x006e6480, scalar deleting form 0x006e6490)
    DECLARE_MESSAGE_MAP()                // GetMessageMap EN 0x006e64f0
};
extern CLauncherApp theApp;              // the one application object (EN 0x016125d0)

// ---- Main launcher dialog (RT_DIALOG 102) and its tab control -------------------------------------------------------
const int IDD_LAUNCHER_DIALOG = 0x66;   // "Gurumin Ver.1.4 (STEAM)" main dialog template (CCongigDlg)
const int IDC_LAUNCHER_TAB = 0x3e8;     // tab control holding the Graphics, Network, Keyboard and Pad pages
const int IDC_GROUP_GRAPHICS = 0x42c;   // "Graphics" group box of the graphics page
const int IDC_GROUP_AUDIO = 0x42d;      // "Audio" group box of the graphics page
const int IDC_LAUNCHER_START = 3;       // "Start Game" button
const int IDC_LAUNCHER_RESET = 0x3ff;   // "Reset To Default" button
const int IDC_LAUNCHER_HELP = 0x400;    // "Help" button (opens manual.pdf)
extern HBRUSH g_launcherPageBrush;      // brush the network page returns for its control colours (EN 0x0090897c, NULL)
const int IDM_ABOUTBOX = 0x10;          // system-menu command that opens the About box (CAboutDlg)

// Tab control of the main dialog: a plain CTabCtrl with a runtime class of its own.
class CTabCtrl2 : public CTabCtrl {
    DECLARE_DYNAMIC(CTabCtrl2)
public:
    CTabCtrl2();            // EN 0x006f0370 (MFC-style inline constructor)
    virtual ~CTabCtrl2();   // EN 0x006f03b0, scalar deleting form 0x006f03c0

protected:
    afx_msg HBRUSH OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor);   // EN 0x006e92f0
    afx_msg BOOL OnEraseBkgnd(CDC *pDC);                                  // EN 0x006e93d0
    afx_msg void OnNcPaint();                                             // EN 0x006ea430 (draws nothing)
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);   // EN 0x006e93b0
    DECLARE_MESSAGE_MAP()
};

// About box (dialog template 100): the copyright line and the OK button. Only ever lives on the stack of
// CCongigDlg::OnSysCommand, so its constructor and destructor are inlined there.
class CAboutDlg : public CDialog {
public:
    CAboutDlg();

    enum { IDD = 100 };

protected:
    virtual void DoDataExchange(CDataExchange *pDX);
    DECLARE_MESSAGE_MAP()
};

// Main launcher dialog ("Gurumin Ver.1.4 (STEAM)"): hosts a tab control with the settings pages and the Start Game,
// Close and Cancel buttons. Created by the launcher entry point (EN 0x006e7470).
class CCongigDlg : public CDialog {
    DECLARE_DYNAMIC(CCongigDlg)
public:
    CCongigDlg(CWnd *pParent = NULL);   // EN 0x006e6610
                                        // (implicit destructor EN 0x006e6780, scalar deleting form 0x006e6e70)

    enum { IDD = IDD_LAUNCHER_DIALOG };

    CTabCtrl2 m_tab;    // 0x094 IDC_LAUNCHER_TAB
    int m_0108;         // 0x108 not touched by the functions reconstructed so far
    HICON m_hIcon;      // 0x10c dialog icon (resource 128)

protected:
    afx_msg HBRUSH OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor);   // EN 0x006e9260
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);                  // EN 0x006e9230
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);                   // EN 0x006ea560
    afx_msg void OnPaint();                                               // EN 0x006ea450
    afx_msg HCURSOR OnQueryDragIcon();                                    // EN 0x006ea510
    afx_msg void OnHelp();                                                // EN 0x006e9080 (Help button)
    afx_msg void OnStartGame();                                           // EN 0x006e90d0 ("Start Game" button)
    afx_msg void OnCloseButton();                                         // EN 0x006e9160 ("Close" button, IDOK)
    afx_msg void OnCancelButton();                                        // EN 0x006e90a0 ("Cancel" button, IDCANCEL)
    afx_msg void OnClose();                                               // EN 0x006e91f0 (window close box)
    afx_msg void OnResetDefaults();                                       // EN 0x006e9060 ("Reset To Default" button)
    afx_msg void OnTcnSelchange(NMHDR *pNMHDR, LRESULT *pResult);         // EN 0x006ea520 (tab changed)
public:
    // Shows the page for tab `page` (0 title, 1 graphics, 2 pad, 3 keyboard) and hides the others. EN 0x006ea830.
    void ShowPage(int page);
protected:
    virtual void DoDataExchange(CDataExchange *pDX);              // EN 0x006e70b0
    virtual BOOL PreTranslateMessage(MSG *pMsg);                  // EN 0x006ea620
    DECLARE_MESSAGE_MAP()
};

// ---- Control ids of the gamepad page (RT_DIALOG 129) ----------------------------------------------------------------
// The page has twelve visible lists (one per action, same layout as the keyboard page), a "Set" button next to each,
// and hidden lists that hold the assigned button index; only the ids the reconstructed code uses are named, by value.
enum LauncherPadControlId {
    IDC_PAD_3EB = 0x3eb,
    IDC_PAD_3EC = 0x3ec,
    IDC_PAD_3ED = 0x3ed,
    IDC_PAD_3EE = 0x3ee,
    IDC_PAD_3F9 = 0x3f9,
    IDC_PAD_3FB = 0x3fb,
    IDC_PAD_3FD = 0x3fd,
    IDC_PAD_3FE = 0x3fe,
    IDC_PAD_3FF = 0x3ff,
    IDC_PAD_400 = 0x400,
    IDC_PAD_401 = 0x401,
    IDC_PAD_402 = 0x402,
    IDC_PAD_403 = 0x403,
    IDC_PAD_404 = 0x404,
    IDC_PAD_405 = 0x405,
    IDC_PAD_406 = 0x406,
    IDC_PAD_407 = 0x407,
    IDC_PAD_408 = 0x408,
    IDC_PAD_409 = 0x409,
    IDC_PAD_40A = 0x40a,
    IDC_PAD_40B = 0x40b,
    IDC_PAD_40C = 0x40c,
    IDC_PAD_40D = 0x40d,
    IDC_PAD_40F = 0x40f,
    IDC_PAD_410 = 0x410,
    IDC_PAD_411 = 0x411,
    IDC_PAD_412 = 0x412,
    IDC_PAD_414 = 0x414,
    IDC_PAD_415 = 0x415,
    IDC_PAD_417 = 0x417,
    IDC_PAD_418 = 0x418,
    IDC_PAD_41A = 0x41a,
    IDC_PAD_41B = 0x41b,
    IDC_PAD_41C = 0x41c,
    IDC_PAD_41D = 0x41d,
    IDC_PAD_41E = 0x41e,
    IDC_PAD_421 = 0x421,
    IDC_PAD_423 = 0x423,
    IDC_PAD_424 = 0x424,
    IDC_PAD_425 = 0x425,
    IDC_PAD_426 = 0x426,
    IDC_PAD_427 = 0x427,
    IDC_PAD_428 = 0x428,
    IDC_PAD_429 = 0x429,
    IDC_PAD_42A = 0x42a
};

// The twelve visible button lists of the gamepad page, one per action (positions in the dialog template).
enum LauncherPadListId {
    IDC_PAD_LIST_DASH = 0x3ef,             // "Dash"
    IDC_PAD_LIST_MOVE_LEFT = 0x3f0,        // "Move Left"
    IDC_PAD_LIST_MOVE_FORWARD = 0x3f1,     // "Move Forward"
    IDC_PAD_LIST_ATTACK = 0x3f2,           // "Attack / OK / Action"
    IDC_PAD_LIST_CAMERA_FORWARD = 0x3f3,   // "Camera: Show Forward"
    IDC_PAD_LIST_MOVE_RIGHT = 0x3f5,       // "Move Right"
    IDC_PAD_LIST_MOVE_BACKWARD = 0x3f6,    // "Move Backward"
    IDC_PAD_LIST_JUMP = 0x3f7,             // "Jump / Cancel"
    IDC_PAD_LIST_CAMERA_ENEMY = 0x3f8,     // "Camera: Show Enemy"
    IDC_PAD_LIST_CAMERA_LEFT = 0x3fa,      // "Camera: Show Left"
    IDC_PAD_LIST_MENU = 0x416,             // "Menu On / Off"
    IDC_PAD_LIST_CAMERA_RIGHT = 0x419      // "Camera: Show Right"
};

// Gamepad button assignment page (dialog template IDD_PAD_DIALOG). Every exchanged value is an int field of the
// ClassWizard kind (list row, check state, slider position); the "Set" buttons, one list, two sliders and further
// buttons are subclassed as CWnd members.
class CPadDlg : public CDialog {
public:
    CPadDlg(CWnd *pParent = NULL);   // EN 0x006ed470

    enum { IDD = IDD_PAD_DIALOG };

    int m_list3fe;                      // 0x094 list selection of id 0x3fe
    int m_list407;                      // 0x098 list selection of id 0x407
    int m_list412;                      // 0x09c list selection of id 0x412
    int m_list410;                      // 0x0a0 list selection of id 0x410
    int m_list40f;                      // 0x0a4 list selection of id 0x40f
    int m_list415;                      // 0x0a8 list selection of id 0x415
    int m_list418;                      // 0x0ac list selection of id 0x418
    int m_list3ff;                      // 0x0b0 list selection of id 0x3ff
    int m_list400;                      // 0x0b4 list selection of id 0x400
    int m_list401;                      // 0x0b8 list selection of id 0x401
    int m_list402;                      // 0x0bc list selection of id 0x402
    int m_list403;                      // 0x0c0 list selection of id 0x403
    int m_list404;                      // 0x0c4 list selection of id 0x404
    int m_list405;                      // 0x0c8 list selection of id 0x405
    int m_list406;                      // 0x0cc list selection of id 0x406
    int m_00d0;                         // 0x0d0 set to -1 by the constructor, not exchanged
    int m_00d4;                         // 0x0d4 not touched by the reconstructed functions
    int m_00d8;                         // 0x0d8 not touched by the reconstructed functions
    int m_00dc;                         // 0x0dc not touched by the reconstructed functions
    int m_00e0;                         // 0x0e0 not touched by the reconstructed functions
    int m_00e4;                         // 0x0e4 not touched by the reconstructed functions
    int m_00e8;                         // 0x0e8 set to -1 by the constructor, not exchanged
    int m_00ec;                         // 0x0ec set to -1 by the constructor, not exchanged
    int m_00f0;                         // 0x0f0 set to -1 by the constructor, not exchanged
    int m_00f4;                         // 0x0f4 set to -1 by the constructor, not exchanged
    int m_00f8;                         // 0x0f8 set to -1 by the constructor, not exchanged
    int m_00fc;                         // 0x0fc set to -1 by the constructor, not exchanged
    int m_0100;                         // 0x100 set to -1 by the constructor, not exchanged
    int m_0104;                         // 0x104 set to -1 by the constructor, not exchanged
    int m_0108;                         // 0x108 set to -1 by the constructor, not exchanged
    int m_list3ee;                      // 0x10c list selection of id 0x3ee
    int m_0110;                         // 0x110 not touched by the reconstructed functions
    CButton m_btn41a;                   // 0x114 control for id 0x41a
    CButton m_btn417;                   // 0x188 control for id 0x417
    CButton m_btn414;                   // 0x1fc control for id 0x414
    CButton m_btn411;                   // 0x270 control for id 0x411
    CButton m_btn3fb;                   // 0x2e4 control for id 0x3fb
    CButton m_btn3f9;                   // 0x358 control for id 0x3f9
    CButton m_btn3ed;                   // 0x3cc control for id 0x3ed
    CButton m_btn3fd;                   // 0x440 control for id 0x3fd
    CButton m_btn40b;                   // 0x4b4 control for id 0x40b
    CButton m_btn40c;                   // 0x528 control for id 0x40c
    CButton m_btn3ec;                   // 0x59c control for id 0x3ec
    CButton m_btn409;                   // 0x610 control for id 0x409
    CButton m_btn408;                   // 0x684 control for id 0x408
    CButton m_btn40a;                   // 0x6f8 control for id 0x40a
    CButton m_btn40d;                   // 0x76c control for id 0x40d
    int m_check41a;                     // 0x7e0 check state of id 0x41a
    int m_check417;                     // 0x7e4 check state of id 0x417
    int m_check414;                     // 0x7e8 check state of id 0x414
    int m_check411;                     // 0x7ec check state of id 0x411
    int m_check3fb;                     // 0x7f0 check state of id 0x3fb
    int m_check3f9;                     // 0x7f4 check state of id 0x3f9
    int m_check3ed;                     // 0x7f8 check state of id 0x3ed
    int m_check3fd;                     // 0x7fc check state of id 0x3fd
    int m_check40b;                     // 0x800 check state of id 0x40b
    int m_check40c;                     // 0x804 check state of id 0x40c
    int m_check3ec;                     // 0x808 check state of id 0x3ec
    int m_check409;                     // 0x80c check state of id 0x409
    int m_check408;                     // 0x810 check state of id 0x408
    int m_check40a;                     // 0x814 check state of id 0x40a
    int m_check40d;                     // 0x818 check state of id 0x40d
    int m_check41b;                     // 0x81c check state of id 0x41b
    int m_check41c;                     // 0x820 check state of id 0x41c
    int m_check41d;                     // 0x824 check state of id 0x41d
    CString m_text421;                  // 0x828 text of id 0x421
    CComboBox m_combo3ee;               // 0x82c control for id 0x3ee
    int m_check41e;                     // 0x8a0 check state of id 0x41e
    int m_pos423;                       // 0x8a4 slider position of id 0x423
    CSliderCtrl m_slider423;            // 0x8a8 control for id 0x423
    int m_check424;                     // 0x91c check state of id 0x424
    int m_check425;                     // 0x920 check state of id 0x425
    CButton m_btn424;                   // 0x924 control for id 0x424
    int m_check426;                     // 0x998 check state of id 0x426
    CButton m_btn426;                   // 0x99c control for id 0x426
    int m_check427;                     // 0xa10 check state of id 0x427
    CButton m_btn427;                   // 0xa14 control for id 0x427
    int m_check428;                     // 0xa88 check state of id 0x428
    CButton m_btn428;                   // 0xa8c control for id 0x428
    int m_check3eb;                     // 0xb00 check state of id 0x3eb
    CSliderCtrl m_slider429;            // 0xb04 control for id 0x429
    int m_pos429;                       // 0xb78 slider position of id 0x429
    CButton m_btn42a;                   // 0xb7c control for id 0x42a
    int m_check42a;                     // 0xbf0 check state of id 0x42a

    // Returns true when the player should not start: a pad list is still on row 0 and the player refused the
    // default input setup offered by the warning box. EN 0x006ed900.
    bool IsStartRefused(CWnd *owner);

    // Stores pad button `button` for action slot `action` (1-based) and clears the same button from any other slot;
    // refreshes the lists when it cleared one or when `refresh` is set. EN 0x006f0140.
    void AssignPadButton(int action, int button, bool refresh);
    // Shows the stored button assignments in the fifteen lists (converting each stored button number to its list
    // row), then runs UpdateData(saveAndValidate). EN 0x006f0230.
    void ShowAssignments(BOOL saveAndValidate);
    // Resets the state of the "Set" buttons after a button was captured (not yet reconstructed). EN 0x006effd0.
    void ResetSetButtons();

protected:
    afx_msg void OnTimer(UINT_PTR nIDEvent);   // EN 0x006efb10 (50 ms pad polling timer: captures the pressed button)
    virtual void DoDataExchange(CDataExchange *pDX);   // EN 0x006eda40
    afx_msg HBRUSH OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor);   // EN 0x006e92c0
    afx_msg void OnPaint();                                               // EN 0x006efae0 (paints nothing)
    afx_msg void OnDestroy();                                             // EN 0x006ef790
    virtual BOOL PreTranslateMessage(MSG *pMsg);                          // EN 0x006efbf0
    DECLARE_MESSAGE_MAP()
};

// Loads the launcher settings from bin\gurumin.def into the page objects' members (the lobby address, the graphics
// and audio switches, the keyboard and pad assignments) and refreshes the pages. `part` 0 loads everything, 1 only the
// pad assignments, 2 only the keyboard assignments. Called by the Reset To Default button (0), the pad page (1) and
// the keyboard page (2). EN 0x006e81d0.
void Launcher_LoadConfig(int part);

// Pad button capture state of the gamepad page (the 50 ms timer in CPadDlg::OnTimer reads it).
extern int g_padWaitingAction;      // 1-based action slot waiting for a button press, 0 = none (EN 0x016152a0)
extern HWND g_padOwnerWindow;       // top-level window handed to the pad scan (EN 0x016152a4)
extern int g_padCaptureStep;        // countdown that steps by 2 on every timer tick, restarts at 16 (EN 0x016152b4)
extern unsigned char g_padFlash;    // repaint ticks still owed after a capture (EN 0x016152b8)
// Scans the game controllers for a pressed button or stick direction and returns its code, 0 when none (large routine
// in the game's input code, not yet reconstructed, EN 0x007ad690).
extern "C" int Pad_FindPressedButton(HWND window, bool option);

// The page dialogs the main dialog shows in its tab control; they are global objects of the launcher.
extern CNetDlg g_netPage;       // network page (EN 0x01612750)
extern CPadDlg g_padPage;       // gamepad page, tab 2 (EN 0x016127f0)
extern CKbDlg g_kbPage;         // keyboard page, tab 3 (EN 0x016133e8)
extern CVideoDlg g_videoPage;   // graphics page, tab 1 (EN 0x016134c8)
extern CTitDlg g_titlePage;     // title page, tab 0 (EN 0x01614770)

// Launcher exit state shared by the main dialog's button handlers.
extern bool g_launcherStartGame;    // true when the launcher closes to start the game (EN 0x016126b0)
extern int g_launcherClosing;       // set to 1 by every handler that closes the launcher (EN 0x016126a8)
extern int g_launcherCancelled;     // set to 1 when the launcher is cancelled (EN 0x016126ac)

// Sizes of the dialog classes, from the m_nObjectSize of their CRuntimeClass in the exe (CAdvancedOptionDlg, CInstDlg,
// CKbDlg, CTitDlg, CTabCtrl2) and from the stride of their last member otherwise.
static_assert(sizeof(CAdvancedOptionDlg) == 0x224, "CAdvancedOptionDlg size");
static_assert(sizeof(CVideoDlg) == 0x1204, "CVideoDlg size");
static_assert(sizeof(CInstDlg) == 0x94, "CInstDlg size");
static_assert(sizeof(CKbDlg) == 0xd0, "CKbDlg size");
static_assert(sizeof(CNetDlg) == 0x9c, "CNetDlg size");
static_assert(sizeof(COseDlg) == 0x9c, "COseDlg size");
static_assert(sizeof(CTitDlg) == 0x1f0, "CTitDlg size");
static_assert(sizeof(CTabCtrl2) == 0x74, "CTabCtrl2 size");
static_assert(sizeof(CCongigDlg) == 0x110, "CCongigDlg size");
static_assert(sizeof(CPadDlg) == 0xbf4, "CPadDlg size");

#endif
