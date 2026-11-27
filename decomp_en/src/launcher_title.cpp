// EN-only launcher: the title page (CTitDlg). Falcom MFC dialog code (ClassWizard style) from the Steam build;
// see NOTES.md "Launcher (MFC)".
#include "launcher_dialog.h"

IMPLEMENT_DYNAMIC(CTitDlg, CDialog)

CTitDlg *g_titlePageThis;   // the title page object (EN 0x0161274c)

// Build date and revision shown in the version line (2015/04/27, revision 6).
const int TITLE_BUILD_YEAR = 2015;
const int TITLE_BUILD_MONTH = 4;
const int TITLE_BUILD_DAY = 27;
const int TITLE_BUILD_REVISION = 6;

// Builds the page; the three controls are subclassed later by DoDataExchange.
CTitDlg::CTitDlg(CWnd *pParent)
    : CDialog(IDD_TITLE_DIALOG, pParent)
{
}

// Destroys the two progress bars, the button and the dialog base.
CTitDlg::~CTitDlg()
{
}

// Subclasses the update button and the two progress bars.
void CTitDlg::DoDataExchange(CDataExchange *pDX)
{
    DDX_Control(pDX, IDC_TITLE_ONLINE_UPDATE, m_onlineUpdateCtrl);
    DDX_Control(pDX, IDC_TITLE_PROGRESS_A, m_progressACtrl);
    DDX_Control(pDX, IDC_TITLE_PROGRESS_B, m_progressBCtrl);
}

// Fills in the static texts (version line, copyright, update server label and status), disables the update button
// and hides the update-related controls. Called by MFC when the page is created.
BOOL CTitDlg::OnInitDialog()
{
    char versionText[256];

    CDialog::OnInitDialog();
    g_titlePageThis = this;
    // "Version<full-width colon>%d/%02d/%02d Rev.%d"
    sprintf(versionText, "Version\x81" "F%d/%02d/%02d Rev.%d", TITLE_BUILD_YEAR, TITLE_BUILD_MONTH, TITLE_BUILD_DAY,
            TITLE_BUILD_REVISION);
    g_titlePageThis->GetDlgItem(IDC_TITLE_VERSION_TEXT)->SetWindowText(versionText);
    g_titlePageThis->GetDlgItem(IDC_TITLE_COPYRIGHT)->SetWindowText(
        "(C) 2004-2015 Nihon Falcom Corporation. Localized and published under license by Mastiff. All rights reserved.");
    g_titlePageThis->m_onlineUpdateCtrl.EnableWindow(FALSE);
    g_titlePageThis->GetDlgItem(IDC_TITLE_UPDATE_SERVER)->SetWindowText("UpdateServer:");
    // Shift-JIS: "checking for the update server." (Japanese leftover of the original launcher)
    g_titlePageThis->GetDlgItem(IDC_TITLE_UPDATE_STATUS)->SetWindowText(
        "\x83" "A\x83" "b\x83v\x83" "f\x81[\x83g\x83" "T\x81[\x83o\x82\xcc\x8c\x9f\x8d\xf5\x92\x86\x82\xc5\x82\xb7\x81" "B");
    g_titlePageThis->GetDlgItem(IDC_TITLE_UPDATE_SERVER)->ShowWindow(SW_HIDE);
    g_titlePageThis->GetDlgItem(IDC_TITLE_UPDATE_STATUS)->ShowWindow(SW_HIDE);
    g_titlePageThis->GetDlgItem(IDC_TITLE_UPDATE_STATUS_B)->ShowWindow(SW_HIDE);
    g_titlePageThis->GetDlgItem(IDC_TITLE_ONLINE_UPDATE)->ShowWindow(SW_HIDE);
    g_titlePageThis->GetDlgItem(IDC_TITLE_PROGRESS_A)->ShowWindow(SW_HIDE);
    g_titlePageThis->GetDlgItem(IDC_TITLE_PROGRESS_B)->ShowWindow(SW_HIDE);
    g_titlePageThis->GetDlgItem(IDC_TITLE_HIDDEN_424)->ShowWindow(SW_HIDE);
    return TRUE;
}

BEGIN_MESSAGE_MAP(CTitDlg, CDialog)
    ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

// Draws the page's static texts with a transparent background.
HBRUSH CTitDlg::OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
    pDC->SetBkMode(TRANSPARENT);
    return hbr;
}
