// EN-only launcher: the main dialog (CCongigDlg, sic), its tab control and the About box. Falcom MFC dialog code
// (ClassWizard style) from the Steam build; see NOTES.md "Launcher (MFC)".
#include <stdio.h>
#include <string.h>

#include "config_ini.h"
#include "launcher_dialog.h"

// About box constructor (inlined into CCongigDlg::OnSysCommand): loads dialog template 100.
CAboutDlg::CAboutDlg()
    : CDialog(IDD)
{
}

// The About box exchanges nothing.
void CAboutDlg::DoDataExchange(CDataExchange *pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(CCongigDlg, CDialog)

// Builds the main dialog and loads its icon (resource 128) from the module's resources.
CCongigDlg::CCongigDlg(CWnd *pParent)
    : CDialog(IDD_LAUNCHER_DIALOG, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(128);
}

BEGIN_MESSAGE_MAP(CCongigDlg, CDialog)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_CTLCOLOR()
    ON_WM_CREATE()
    ON_BN_CLICKED(IDC_LAUNCHER_HELP, &CCongigDlg::OnHelp)
    ON_NOTIFY(TCN_SELCHANGE, IDC_LAUNCHER_TAB, &CCongigDlg::OnTcnSelchange)
    ON_BN_CLICKED(IDC_LAUNCHER_START, &CCongigDlg::OnStartGame)
    ON_WM_CLOSE()
    ON_BN_CLICKED(IDOK, &CCongigDlg::OnCloseButton)
    ON_BN_CLICKED(IDCANCEL, &CCongigDlg::OnCancelButton)
    ON_BN_CLICKED(IDC_LAUNCHER_RESET, &CCongigDlg::OnResetDefaults)
END_MESSAGE_MAP()

bool g_launcherStartGame;   // true when the launcher closes to start the game (EN 0x016126b0)
int g_launcherClosing;      // set to 1 by every handler that closes the launcher (EN 0x016126a8)
int g_launcherCancelled;    // set to 1 when the launcher is cancelled (EN 0x016126ac)

// The tab control is the only control the main dialog exchanges.
void CCongigDlg::DoDataExchange(CDataExchange *pDX)
{
    DDX_Control(pDX, IDC_LAUNCHER_TAB, m_tab);
}

// Swallows Enter, Esc and F1 key presses so they cannot close the launcher or open the help by accident.
BOOL CCongigDlg::PreTranslateMessage(MSG *pMsg)
{
    if (pMsg->message == WM_KEYDOWN) {
        if (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE || pMsg->wParam == VK_F1) {
            return TRUE;
        }
    }
    return CDialog::PreTranslateMessage(pMsg);
}

// Tab changed: shows the page of the newly selected tab.
void CCongigDlg::OnTcnSelchange(NMHDR *pNMHDR, LRESULT *pResult)
{
    ShowPage(m_tab.GetCurSel());
    *pResult = 0;
}

// Shows the page that belongs to tab `page` inside the tab control's display area, scaled from dialog units to
// pixels, and hides every other page.
void CCongigDlg::ShowPage(int page)
{
    CWnd *shown;

    g_padPage.ShowWindow(SW_HIDE);
    g_kbPage.ShowWindow(SW_HIDE);
    g_videoPage.ShowWindow(SW_HIDE);
    g_netPage.ShowWindow(SW_HIDE);
    g_titlePage.ShowWindow(SW_HIDE);

    CRect unit(100, 100, 100, 100);   // 100 dialog units in each direction, converted to pixels below
    MapDialogRect(&unit);
    float scaleX = (float)unit.left / 175.0f;
    float scaleY = (float)unit.top / 150.0f;
    RECT area = { -3, 0, 872, 419 };  // display area of the 175 x 150 dialog-unit design size
    m_tab.SendMessage(TCM_ADJUSTRECT, FALSE, (LPARAM)&area);
    area.right = (int)((float)area.right * scaleX);
    area.bottom = (int)((float)area.bottom * scaleY);

    if (page == 1) {
        g_videoPage.ShowWindow(SW_SHOW);
        shown = &g_videoPage;
    } else if (page == 2) {
        g_padPage.ShowWindow(SW_SHOW);
        shown = &g_padPage;
    } else if (page == 3) {
        g_kbPage.ShowWindow(SW_SHOW);
        shown = &g_kbPage;
    } else if (page == 0) {
        g_titlePage.ShowWindow(SW_SHOW);
        shown = &g_titlePage;
    } else {
        return;
    }
    shown->MoveWindow(area.left, area.top, area.right - area.left, area.bottom - area.top, TRUE);
}

// The loader keeps the two gamma settings as slider positions: stored value / LAUNCHER_GAMMA_DIVISOR +
// LAUNCHER_GAMMA_BASE.
const int LAUNCHER_GAMMA_DIVISOR = 2;
const int LAUNCHER_GAMMA_BASE = 50;
// Values of Launcher_LoadConfig's `part` argument.
const int LAUNCHER_LOAD_ALL = 0;      // every setting
const int LAUNCHER_LOAD_PAD = 1;      // only the pad assignments
const int LAUNCHER_LOAD_KEYBOARD = 2; // only the keyboard assignments
const int LAUNCHER_TEXT_BYTES = 256;  // bytes of the path and lobby address locals

// Loads the launcher settings from bin\gurumin.def of the install folder into the members of the page objects, then
// refreshes the pages. `part` is LAUNCHER_LOAD_ALL (lobby address, graphics, audio, keyboard and pad settings),
// LAUNCHER_LOAD_PAD (pad assignments and their lists) or LAUNCHER_LOAD_KEYBOARD (keyboard assignments). Called by the
// Reset To Default button (all), the pad page (pad) and the keyboard page (keyboard).
void Launcher_LoadConfig(int part)
{
    char path[LAUNCHER_TEXT_BYTES];
    char lobbyAddress[LAUNCHER_TEXT_BYTES];
    sprintf(path, "%s\\bin\\gurumin.def", g_launcherInstallDir);
    lobbyAddress[0] = 0;
    Config_GetString(path, "CONFIG", "LOBBYADD1", lobbyAddress);
    if (lobbyAddress[0] == 0) {
        strcpy(lobbyAddress, "192.168.1.3");   // default lobby server address
    }
    g_netPage.m_lobbyAddress.SetString(lobbyAddress, (int)strlen(lobbyAddress));
    const int which = part;   // the original reuses the parameter's stack slot for the gamma reads below
    if (which == LAUNCHER_LOAD_ALL) {
        Config_GetInt(path, "CONFIG", "RESOLUTION", &g_videoPage.m_resolution);
        Config_GetInt(path, "CONFIG", "FULLSCREEN", &g_videoPage.m_windowed);
        Config_GetInt(path, "CONFIG", "HDVS", &g_videoPage.m_hwVertexShader);
        Config_GetInt(path, "CONFIG", "TEX4", &g_videoPage.m_highQualityTexture);
        g_videoPage.m_highQualityTexture = g_videoPage.m_highQualityTexture == 0;
        Config_GetInt(path, "CONFIG", "TEXD", &g_videoPage.m_uncompressedTextures);
        Config_GetInt(path, "CONFIG", "ANTI", &g_videoPage.m_antiAlias);
        Config_GetInt(path, "CONFIG", "ANTISUB", &g_videoPage.m_textureFiltering);
        Config_GetInt(path, "CONFIG", "FILTER", &g_videoPage.m_multiSampling);
        Config_GetInt(path, "CONFIG", "FADE", &g_videoPage.m_crossFade);
        Config_GetInt(path, "CONFIG", "MIRR", &g_videoPage.m_reflections);
        Config_GetInt(path, "CONFIG", "SOUND", &g_videoPage.m_audioEnable);
        Config_GetInt(path, "CONFIG", "DOF", &g_videoPage.m_depthOfField);
        Config_GetInt(path, "CONFIG", "VRAM", &g_videoPage.m_combo403);
        Config_GetInt(path, "CONFIG", "VRAM2", &g_videoPage.m_combo407);
        Config_GetInt(path, "CONFIG", "EFFECT", &g_videoPage.m_shadowComplexity);
        Config_GetInt(path, "CONFIG", "SHADOW", &g_videoPage.m_shadowQuality);
        Config_GetInt(path, "CONFIG", "BLUR", &g_videoPage.m_blurEffect);
        Config_GetInt(path, "CONFIG", "FOG", &g_videoPage.m_fogEffect);
        Config_GetInt(path, "CONFIG", "NOAVI", &g_videoPage.m_skipVideo);
        Config_GetInt(path, "CONFIG", "VPRE", &g_videoPage.m_preset);
        Config_GetInt(path, "CONFIG", "CMAKE", &g_videoPage.m_cacheAtStartup);
        Config_GetInt(path, "CONFIG", "NOAUTOUP", &g_netPage.m_noPatch);
        Config_GetInt(path, "CONFIG", "TIEN", &g_videoPage.m_delayMs);
        Config_GetInt(path, "CONFIG", "AUTOJUMP", &g_kbPage.m_swapAttackMove);
        Config_GetInt(path, "CONFIG", "WROT", &g_kbPage.m_wheelCamera);
        Config_GetInt(path, "CONFIG", "WJUMP2", &g_kbPage.m_dashDoubleClick);
        Config_GetInt(path, "CONFIG", "WJUMP3", &g_padPage.m_check41e);
        Config_GetInt(path, "CONFIG", "KANDO", &g_padPage.m_pos423);
        Config_GetInt(path, "CONFIG", "KANDO2", &g_padPage.m_pos429);
        Config_GetInt(path, "CONFIG", "USEVOICE", &g_videoPage.m_playVoices);
        Config_GetInt(path, "CONFIG", "USEGAMMA", &g_videoPage.m_gammaSettings);
        Config_GetInt(path, "CONFIG", "BGMDOUKI", &g_videoPage.m_sync);
        Config_GetInt(path, "CONFIG", "VSK", &g_videoPage.m_complexityShader);
        Config_GetInt(path, "CONFIG", "VSYNC", &g_videoPage.m_vsyncOff);
        Config_GetInt(path, "CONFIG", "USEBKB", &g_videoPage.m_useBackBuffer);
        if (Config_HasInt(path, "CONFIG", "USEBKB") == 0) {
            g_videoPage.m_useBackBuffer = 1;
        }
        Config_GetInt(path, "CONFIG", "USE32", &g_videoPage.m_color32bit);
        Config_GetInt(path, "CONFIG", "USEDX8", &g_videoPage.m_directX81);
        Config_GetInt(path, "CONFIG", "USEDX9", &g_videoPage.m_directX9);
        Config_GetInt(path, "CONFIG", "USETKUSE", &g_videoPage.m_reflectionEffect);
        if (Config_HasInt(path, "CONFIG", "USETKUSE") == 0) {
            g_videoPage.m_reflectionEffect = 1;
        }
        Config_GetInt(path, "CONFIG", "BGMVOL", &g_videoPage.m_bgmVolume);
        Config_GetInt(path, "CONFIG", "SEVOL", &g_videoPage.m_seVolume);
        Config_GetInt(path, "CONFIG", "SOUND3D", &g_videoPage.m_combo415);
        Config_GetInt(path, "CONFIG", "SOUND3D2", &g_videoPage.m_combo414);
        Config_GetInt(path, "CONFIG", "SOUNDTP", &g_videoPage.m_combo40e);
        Config_GetInt(path, "CONFIG", "SOUNDTP2", &g_videoPage.m_combo40f);
        Config_GetInt(path, "CONFIG", "GAMMA", &part);
        g_videoPage.m_contrast = part / LAUNCHER_GAMMA_DIVISOR + LAUNCHER_GAMMA_BASE;
        Config_GetInt(path, "CONFIG", "GAMMA2", &part);
        g_videoPage.m_brightness = part / LAUNCHER_GAMMA_DIVISOR + LAUNCHER_GAMMA_BASE;
        Config_GetInt(path, "CONFIG", "USEJP", &g_padPage.m_check3eb);
        Config_GetInt(path, "CONFIG", "USEVB", &g_padPage.m_check425);
        Config_GetInt(path, "CONFIG", "EKEY1", &g_padPage.m_check424);
        Config_GetInt(path, "CONFIG", "EKEY2", &g_padPage.m_check426);
        Config_GetInt(path, "CONFIG", "EKEY3", &g_padPage.m_check427);
        Config_GetInt(path, "CONFIG", "EKEY4", &g_padPage.m_check428);
        Config_GetInt(path, "CONFIG", "NOUDXI", &g_padPage.m_check42a);
    }
    if (which == LAUNCHER_LOAD_ALL || which == LAUNCHER_LOAD_KEYBOARD) {
        Config_GetInt(path, "KEYCONFIG", "KEY01", &g_kbPage.m_dash);
        Config_GetInt(path, "KEYCONFIG", "KEY02", &g_kbPage.m_moveLeft);
        Config_GetInt(path, "KEYCONFIG", "KEY03", &g_kbPage.m_moveForward);
        Config_GetInt(path, "KEYCONFIG", "KEY04", &g_kbPage.m_attack);
        Config_GetInt(path, "KEYCONFIG", "KEY05", &g_kbPage.m_cameraForward);
        Config_GetInt(path, "KEYCONFIG", "KEY07", &g_kbPage.m_moveRight);
        Config_GetInt(path, "KEYCONFIG", "KEY08", &g_kbPage.m_moveBackward);
        Config_GetInt(path, "KEYCONFIG", "KEY09", &g_kbPage.m_jump);
        Config_GetInt(path, "KEYCONFIG", "KEY10", &g_kbPage.m_cameraEnemy);
        Config_GetInt(path, "KEYCONFIG", "KEY11", &g_kbPage.m_cameraLeft);
        Config_GetInt(path, "KEYCONFIG", "KEY14", &g_kbPage.m_menu);
        Config_GetInt(path, "KEYCONFIG", "KEY15", &g_kbPage.m_cameraRight);
    }
    if (which == LAUNCHER_LOAD_ALL || which == LAUNCHER_LOAD_PAD) {
        Config_GetInt(path, "KEYCONFIG", "PAD01", &g_padPage.m_00d0);
        Config_GetInt(path, "KEYCONFIG", "PAD02", &g_padPage.m_00ec);
        Config_GetInt(path, "KEYCONFIG", "PAD03", &g_padPage.m_00f0);
        Config_GetInt(path, "KEYCONFIG", "PAD04", &g_padPage.m_00f4);
        Config_GetInt(path, "KEYCONFIG", "PAD05", &g_padPage.m_00f8);
        Config_GetInt(path, "KEYCONFIG", "PAD06", &g_padPage.m_00fc);
        Config_GetInt(path, "KEYCONFIG", "PAD07", &g_padPage.m_0100);
        Config_GetInt(path, "KEYCONFIG", "PAD08", &g_padPage.m_0104);
        Config_GetInt(path, "KEYCONFIG", "PAD09", &g_padPage.m_0108);
        Config_GetInt(path, "KEYCONFIG", "PAD10", &g_padPage.m_00e8);
        Config_GetInt(path, "KEYCONFIG", "PAD11", &g_padPage.m_00e4);
        Config_GetInt(path, "KEYCONFIG", "PAD12", &g_padPage.m_00e0);
        Config_GetInt(path, "KEYCONFIG", "PAD13", &g_padPage.m_00dc);
        Config_GetInt(path, "KEYCONFIG", "PAD14", &g_padPage.m_00d8);
        Config_GetInt(path, "KEYCONFIG", "PAD15", &g_padPage.m_00d4);
        Config_GetInt(path, "KEYCONFIG", "PADID", &g_padPage.m_0110);
    }
    if (which == LAUNCHER_LOAD_ALL || which == LAUNCHER_LOAD_PAD) {
        g_padPage.ShowAssignments(FALSE);
    }
    if (which == LAUNCHER_LOAD_ALL || which == LAUNCHER_LOAD_KEYBOARD) {
        g_kbPage.UpdateData(FALSE);
    }
    if (which == LAUNCHER_LOAD_ALL) {
        g_videoPage.UpdateData(FALSE);
        g_netPage.UpdateData(FALSE);
    }
    if (::SendMessageA(g_videoPage.m_combo415Ctrl.m_hWnd, CB_GETCURSEL, 0, 0) == 1) {
        g_videoPage.m_combo414Ctrl.EnableWindow(TRUE);
    } else {
        g_videoPage.m_combo414Ctrl.EnableWindow(FALSE);
    }
    if (g_videoPage.m_useBackBuffer == 0) {
        g_videoPage.m_crossFadeCtrl.EnableWindow(FALSE);
        g_videoPage.m_blurEffectCtrl.EnableWindow(FALSE);
        ::SendMessageA(g_videoPage.m_crossFadeCtrl.m_hWnd, BM_SETCHECK, 0, 0);
        ::SendMessageA(g_videoPage.m_blurEffectCtrl.m_hWnd, BM_SETCHECK, 0, 0);
        return;
    }
    g_videoPage.m_crossFadeCtrl.EnableWindow(TRUE);
    g_videoPage.m_blurEffectCtrl.EnableWindow(TRUE);
}

// "Reset To Default": after confirmation reloads the default settings.
void CCongigDlg::OnResetDefaults()
{
    if (MessageBox("Are you sure you want to restore default settings?", "Confirm", MB_YESNO) == IDYES) {
        Launcher_LoadConfig(0);
    }
}

// "Start Game": refuses to leave while the pad or keyboard assignment is incomplete (switching to that tab), else
// closes the launcher with the start flag set.
void CCongigDlg::OnStartGame()
{
    if (g_padPage.IsStartRefused(this)) {
        m_tab.SetCurSel(2);
        ShowPage(2);
        return;
    }
    if (g_kbPage.IsStartRefused(this)) {
        m_tab.SetCurSel(3);
        ShowPage(3);
        return;
    }
    g_launcherStartGame = true;
    g_launcherClosing = 1;
    Launcher_SaveSettings();
    OnOK();
}

// "Close" button: same checks as Start Game, but closes without starting the game.
void CCongigDlg::OnCloseButton()
{
    if (g_padPage.IsStartRefused(this)) {
        m_tab.SetCurSel(2);
        ShowPage(2);
        return;
    }
    if (g_kbPage.IsStartRefused(this)) {
        m_tab.SetCurSel(3);
        ShowPage(3);
        return;
    }
    g_launcherStartGame = false;
    g_launcherClosing = 1;
    Launcher_SaveSettings();
    OnOK();
}

// "Cancel" button: closes without starting the game and flags the launcher as cancelled.
void CCongigDlg::OnCancelButton()
{
    g_launcherStartGame = false;
    g_launcherClosing = 1;
    g_launcherCancelled = 1;
    Launcher_SaveSettings();
    OnCancel();
}

// Window close box: closes without starting the game.
void CCongigDlg::OnClose()
{
    g_launcherStartGame = false;
    g_launcherClosing = 1;
    Launcher_SaveSettings();
    CDialog::OnClose();
}

// Help button: opens the PDF manual in the associated viewer.
void CCongigDlg::OnHelp()
{
    ShellExecute(NULL, "open", "manual.pdf", NULL, NULL, SW_SHOWNORMAL);
}

// Window creation: sets the window class name "Falcom" and lets the base class finish; -1 means failure.
int CCongigDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    lpCreateStruct->lpszClass = "Falcom";
    if (CDialog::OnCreate(lpCreateStruct) == -1) {
        return -1;
    }
    return 0;
}

// Paints the dialog; while minimised, draws the dialog icon centred in the client area instead.
void CCongigDlg::OnPaint()
{
    if (IsIconic()) {
        CPaintDC dc(this);
        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;
        dc.DrawIcon(x, y, m_hIcon);
    } else {
        CDialog::OnPaint();
    }
}

// Cursor shown while the minimised window is dragged: the dialog icon.
HCURSOR CCongigDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

// System-menu commands: the About entry opens the About box, everything else goes to the base class.
void CCongigDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    } else {
        CDialog::OnSysCommand(nID, lParam);
    }
}

// Makes static texts and check boxes draw with a transparent background over the dialog bitmap.
HBRUSH CCongigDlg::OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
    pDC->SetBkMode(TRANSPARENT);
    return hbr;
}
