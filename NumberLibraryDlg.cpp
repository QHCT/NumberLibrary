// NumberLibraryDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "NumberLibrary.h"
#include "NumberLibraryDlg.h"
#include "afxdialogex.h"
#include <regex>
#include <shellapi.h>
#include <Urlmon.h>
#include <process.h>

#pragma comment(lib, "Urlmon.lib")

// CInputDialog 实现
BEGIN_MESSAGE_MAP(CInputDialog, CDialog)
END_MESSAGE_MAP()

CInputDialog::CInputDialog(CString strInitValue)
	: CDialog(IDD_DIALOG1), m_strInput(strInitValue)
{
}

BOOL CInputDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	SetDlgItemText(IDC_EDIT5, m_strInput);
	SetWindowText(_T("编辑备注"));
	SetDlgItemText(IDC_STATIC, _T("请输入备注内容:"));
	return TRUE;
}

void CInputDialog::OnOK()
{
	GetDlgItemText(IDC_EDIT5, m_strInput);
	CDialog::OnOK();
}

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 配置文件名
#define CONFIG_FILE _T("NumberLibrary.cfg")
// 数据文件名
#define DATA_FILE _T("NumberLibrary.dat")

// 获取线程参数结构
struct FETCH_THREAD_PARAM
{
	CNumberLibraryDlg* pDlg;
	int nItem;
	int nStatusIndex;  // 状态在数组中的索引
};

// CAboutDlg 实现
BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
	ON_STN_CLICKED(IDC_STATIC_WEBSITE, &CAboutDlg::OnStnClickedStaticWebsite)
	ON_STN_CLICKED(IDC_STATIC_GITHUB, &CAboutDlg::OnStnClickedStaticGithub)
	ON_WM_CTLCOLOR()
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()

CAboutDlg::CAboutDlg() : CDialogEx(IDD_DIALOG_ABOUT)
{
	m_hLinkCursor = NULL;
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_WEBSITE, m_websiteLink);
	DDX_Control(pDX, IDC_STATIC_GITHUB, m_githubLink);
}

BOOL CAboutDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	
	// 创建下划线字体
	LOGFONT lf;
	GetFont()->GetLogFont(&lf);
	lf.lfUnderline = TRUE;
	m_linkFont.CreateFontIndirect(&lf);
	
	// 设置字体
	m_websiteLink.SetFont(&m_linkFont);
	m_githubLink.SetFont(&m_linkFont);
	
	// 加载手型光标
	m_hLinkCursor = LoadCursor(NULL, IDC_HAND);
	
	return TRUE;
}

HBRUSH CAboutDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
	
	// 设置链接文本为蓝色
	if (pWnd->GetDlgCtrlID() == IDC_STATIC_WEBSITE || 
		pWnd->GetDlgCtrlID() == IDC_STATIC_GITHUB)
	{
		pDC->SetTextColor(RGB(0, 0, 255)); // 蓝色
	}
	
	return hbr;
}

BOOL CAboutDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// 获取鼠标位置
	CPoint point;
	GetCursorPos(&point);
	
	// 转换为客户端坐标
	CRect rect;
	m_websiteLink.GetWindowRect(&rect);
	ScreenToClient(&rect);

	if (rect.PtInRect(point))
	{
		::SetCursor(m_hLinkCursor);
		return TRUE;
	}

	m_githubLink.GetWindowRect(&rect);
	ScreenToClient(&rect);
	if (rect.PtInRect(point))
	{
		::SetCursor(m_hLinkCursor);
		return TRUE;
	}
	
	return CDialogEx::OnSetCursor(pWnd, nHitTest, message);
}

void CAboutDlg::OnStnClickedStaticWebsite()
{
	ShellExecute(NULL, _T("open"), _T("http://www.ctzyz.com"), NULL, NULL, SW_SHOWNORMAL);
}

void CAboutDlg::OnStnClickedStaticGithub()
{
	ShellExecute(NULL, _T("open"), _T("https://github.com/QHCT/NumberLibrary"), NULL, NULL, SW_SHOWNORMAL);
}

CNumberLibraryDlg::CNumberLibraryDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_NUMBERLIBRARY_DIALOG, pParent)
	, m_nRefreshTime(3)
	, m_nVerifyCount(3)
	, m_strNumberRegex(_T("^([^\\-]+)----([^\\-]+)$"))
	, m_strVerifyCodeRegex(_T("(\\d{6})(?=[^\\d]*短信登录验证码)"))
	, m_nTimerID(0)
	, m_hFetchThread(NULL)
	, m_bThreadRunning(false)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CNumberLibraryDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST2, m_listCtrl);
	DDX_Control(pDX, IDC_EDIT1, m_editRefreshTime);
	DDX_Control(pDX, IDC_EDIT2, m_editVerifyCount);
	DDX_Control(pDX, IDC_EDIT3, m_editNumberRegex);
	DDX_Control(pDX, IDC_EDIT4, m_editVerifyCodeRegex);
	
	// 将成员变量与控件关联
	DDX_Text(pDX, IDC_EDIT1, m_nRefreshTime);
	DDX_Text(pDX, IDC_EDIT2, m_nVerifyCount);
	DDX_Text(pDX, IDC_EDIT3, m_strNumberRegex);
	DDX_Text(pDX, IDC_EDIT4, m_strVerifyCodeRegex);
}

BEGIN_MESSAGE_MAP(CNumberLibraryDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	ON_WM_TIMER()
	ON_COMMAND(ID_MENU_IMPORT, &CNumberLibraryDlg::OnMenuImport)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST2, &CNumberLibraryDlg::OnNMDblclkList2)
	ON_COMMAND(ID_MENU_TEST_REGEX, &CNumberLibraryDlg::OnMenuTestRegex)
	ON_MESSAGE(WM_USER + 100, OnFetchError)
	ON_MESSAGE(WM_USER + 101, OnFetchSuccess)
	ON_COMMAND(ID_PROGRAM_ABOUT, &CNumberLibraryDlg::OnProgramAbout)
	ON_NOTIFY(NM_RCLICK, IDC_LIST2, &CNumberLibraryDlg::OnNMRClickList2)
	ON_COMMAND(ID_MENU_DELETE_ITEM, &CNumberLibraryDlg::OnMenuDeleteItem)
	ON_COMMAND(ID_MENU_DELETE_ALL, &CNumberLibraryDlg::OnMenuDeleteAll)
END_MESSAGE_MAP()


// CNumberLibraryDlg 消息处理程序

BOOL CNumberLibraryDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	// 加载菜单
	CMenu menu;
	menu.LoadMenu(IDR_MENU1);
	
	// 添加测试正则表达式菜单项
	CMenu* pSubMenu = menu.GetSubMenu(0);
	if (pSubMenu)
	{
		pSubMenu->AppendMenu(MF_SEPARATOR);
		pSubMenu->AppendMenu(MF_STRING, ID_MENU_TEST_REGEX, _T("测试正则表达式"));
	}
	
	SetMenu(&menu);
	menu.Detach();

	// 加载配置
	LoadConfig();

	UpdateData(FALSE);

	m_listCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	
	// 添加列标题
	m_listCtrl.InsertColumn(0, _T("号码"), LVCFMT_LEFT, 100);
	m_listCtrl.InsertColumn(1, _T("验证码"), LVCFMT_LEFT, 150);
	m_listCtrl.InsertColumn(2, _T("打开链接"), LVCFMT_LEFT, 150);
	m_listCtrl.InsertColumn(3, _T("开始获取"), LVCFMT_LEFT, 100);
	m_listCtrl.InsertColumn(4, _T("备注"), LVCFMT_LEFT, 120);

	LoadDataFile();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}



void CNumberLibraryDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CNumberLibraryDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// 添加数据到列表的方法
void CNumberLibraryDlg::AddDataToList(const CString& number, const CString& verifyCode, 
                                     const CString& link, const CString& startGet, const CString& remark)
{
	NumberData data;
	data.strNumber = number;
	data.strVerifyCode = verifyCode;
	data.strLink = link;
	data.strStartGet = startGet;
	data.strRemark = remark;
	m_vecData.push_back(data);

	int nIndex = m_listCtrl.GetItemCount();
	m_listCtrl.InsertItem(nIndex, number);
	m_listCtrl.SetItemText(nIndex, 1, verifyCode);
	m_listCtrl.SetItemText(nIndex, 2, _T("打开链接"));
	m_listCtrl.SetItemText(nIndex, 3, _T("开始获取"));
	m_listCtrl.SetItemText(nIndex, 4, remark);
}

// 加载配置文件
BOOL CNumberLibraryDlg::LoadConfig()
{
	TCHAR szPath[MAX_PATH];
	GetModuleFileName(NULL, szPath, MAX_PATH);

	CString strPath(szPath);
	int nPos = strPath.ReverseFind('\\');
	if (nPos > 0)
		strPath = strPath.Left(nPos + 1);

	CString strConfigFile = strPath + CONFIG_FILE;

	if (GetFileAttributes(strConfigFile) == INVALID_FILE_ATTRIBUTES)
	{
		m_nRefreshTime = 3;
		m_nVerifyCount = 3;
		m_strNumberRegex = _T("^([^\\-]+)----([^\\-]+)$");
		m_strVerifyCodeRegex = _T("(\\d{6})(?=[^\\d]*短信登录验证码)");
		return FALSE;
	}

	CStdioFile file;
	if (!file.Open(strConfigFile, CFile::modeRead | CFile::typeText | CFile::typeUnicode))
	{
		if (!file.Open(strConfigFile, CFile::modeRead | CFile::typeText))
		{
			TRACE(_T("无法打开配置文件: %s\n"), strConfigFile);
			return FALSE;
		}
	}

	CString strLine;

	if (file.ReadString(strLine))
		m_nRefreshTime = _ttoi(strLine);

	if (file.ReadString(strLine))
		m_nVerifyCount = _ttoi(strLine);

	if (file.ReadString(strLine))
	{
		m_strNumberRegex = strLine;
		TRACE(_T("读取号码正则: %s\n"), m_strNumberRegex);
	}

	if (file.ReadString(strLine))
	{
		m_strVerifyCodeRegex = strLine;
		TRACE(_T("读取验证码正则: %s\n"), m_strVerifyCodeRegex);
	}
	
	file.Close();
	return TRUE;
}

// 保存配置文件
BOOL CNumberLibraryDlg::SaveConfig()
{
	TCHAR szPath[MAX_PATH];
	GetModuleFileName(NULL, szPath, MAX_PATH);

	CString strPath(szPath);
	int nPos = strPath.ReverseFind('\\');
	if (nPos > 0)
		strPath = strPath.Left(nPos + 1);

	CString strConfigFile = strPath + CONFIG_FILE;
	UpdateData(TRUE);
	TRACE(_T("保存配置 - 号码正则: %s\n"), m_strNumberRegex);
	TRACE(_T("保存配置 - 验证码正则: %s\n"), m_strVerifyCodeRegex);

	CStdioFile file;
	if (!file.Open(strConfigFile, CFile::modeCreate | CFile::modeWrite | CFile::typeText | CFile::typeUnicode))
	{
		TRACE(_T("无法打开配置文件进行写入: %s\n"), strConfigFile);
		return FALSE;
	}

	CString strLine;
	strLine.Format(_T("%d"), m_nRefreshTime);
	file.WriteString(strLine + _T("\n"));
	strLine.Format(_T("%d"), m_nVerifyCount);
	file.WriteString(strLine + _T("\n"));
	file.WriteString(m_strNumberRegex + _T("\n"));
	file.WriteString(m_strVerifyCodeRegex + _T("\n"));
	file.Close();
	
	TRACE(_T("配置文件保存成功: %s\n"), strConfigFile);
	return TRUE;
}

// 加载数据文件
BOOL CNumberLibraryDlg::LoadDataFile()
{
	TCHAR szPath[MAX_PATH];
	GetModuleFileName(NULL, szPath, MAX_PATH);

	CString strPath(szPath);
	int nPos = strPath.ReverseFind('\\');
	if (nPos > 0)
		strPath = strPath.Left(nPos + 1);
	
	CString strDataFile = strPath + DATA_FILE;

	if (GetFileAttributes(strDataFile) == INVALID_FILE_ATTRIBUTES)
	{
		TRACE(_T("数据文件不存在: %s\n"), strDataFile);
		return FALSE;
	}

	CStdioFile file;
	if (!file.Open(strDataFile, CFile::modeRead | CFile::typeText | CFile::typeUnicode))
	{
		if (!file.Open(strDataFile, CFile::modeRead | CFile::typeText))
		{
			TRACE(_T("无法打开数据文件: %s\n"), strDataFile);
			return FALSE;
		}
	}

	//m_vecData.clear();
	//m_listCtrl.DeleteAllItems();

	CString strContent;
	CString strLine;
	while (file.ReadString(strLine))
	{
		strContent += strLine;
	}
	file.Close();
	
	TRACE(_T("读取到的文件内容: %s\n"), strContent);

	int recordCount = 0;
	if (strContent.IsEmpty())
	{
		TRACE(_T("文件内容为空\n"));
		return FALSE;
	}

	CStringArray records;
	
	// 检查是否包含 ||-|| 分隔符
	if (strContent.Find(_T("||-||")) != -1)
	{
		TRACE(_T("检测到新格式数据，使用||-||分隔\n"));
		int startPos = 0;
		int nextPos = 0;
		
		while ((nextPos = strContent.Find(_T("||-||"), startPos)) != -1)
		{
			CString currentRecord = strContent.Mid(startPos, nextPos - startPos);
			records.Add(currentRecord);
			TRACE(_T("解析记录: %s\n"), currentRecord);
			startPos = nextPos + 5;  // ||-|| 长度为5
		}
		
		if (startPos < strContent.GetLength())
		{
			CString lastRecord = strContent.Mid(startPos);
			records.Add(lastRecord);
			TRACE(_T("解析最后记录: %s\n"), lastRecord);
		}
	}
	else
	{
		TRACE(_T("使用标准格式解析数据: 号码||验证码||链接||备注\n"));
		records.Add(strContent);
		TRACE(_T("添加完整内容进行解析: %s\n"), strContent);
	}
	
	TRACE(_T("找到 %d 条记录\n"), records.GetSize());
	
	// 处理每条记录
	for (int i = 0; i < records.GetSize(); i++)
	{
		CString record = records[i];
		TRACE(_T("处理记录 #%d: %s\n"), i + 1, record);
		CString number, verifyCode, link, remark;
		int pos1 = record.Find(_T("||"));
		if (pos1 == -1)
		{
			TRACE(_T("记录格式错误，没有分隔符: %s\n"), record);
			continue;
		}
		
		number = record.Left(pos1);
		if (number.IsEmpty())
		{
			TRACE(_T("跳过空号码记录\n"));
			continue;
		}
		
		CString remaining = record.Mid(pos1 + 2);
		int pos2 = remaining.Find(_T("||"));
		if (pos2 != -1)
		{
			verifyCode = remaining.Left(pos2);
			remaining = remaining.Mid(pos2 + 2);
			int pos3 = remaining.Find(_T("||"));
			if (pos3 != -1)
			{
				link = remaining.Left(pos3);
				remark = remaining.Mid(pos3 + 2);
			}
			else
			{
				link = remaining;
			}
		}
		else
		{
			link = remaining;
		}
		
		TRACE(_T("解析结果 - 号码: [%s], 验证码: [%s], 链接: [%s], 备注: [%s]\n"), 
			number, verifyCode, link, remark);
			
		bool exists = false;
		for (size_t j = 0; j < m_vecData.size(); j++)
		{
			if (m_vecData[j].strNumber == number)
			{
				exists = true;
				TRACE(_T("记录已存在，跳过: %s\n"), number);
				break;
			}
		}
		
		if (!exists)
		{
			AddDataToList(number, verifyCode, link, _T("开始获取"), remark);
			recordCount++;
			TRACE(_T("成功添加记录 #%d\n"), recordCount);
		}
	}
	
	CString strDebug;
	strDebug.Format(_T("成功加载 %d 条记录"), recordCount);
	TRACE(strDebug + _T("\n"));
	
	return recordCount > 0;
}

// 保存数据文件
BOOL CNumberLibraryDlg::SaveDataFile()
{
	TCHAR szPath[MAX_PATH];
	GetModuleFileName(NULL, szPath, MAX_PATH);
	CString strPath(szPath);
	int nPos = strPath.ReverseFind('\\');
	if (nPos > 0)
		strPath = strPath.Left(nPos + 1);
	
	CString strDataFile = strPath + DATA_FILE;
	CStdioFile file;
	if (!file.Open(strDataFile, CFile::modeCreate | CFile::modeWrite | CFile::typeText | CFile::typeUnicode))
		return FALSE;
	
	TRACE(_T("准备保存 %d 条记录\n"), m_vecData.size());
	CString strContent;
	int validCount = 0;
	
	for (size_t i = 0; i < m_vecData.size(); i++)
	{
		if (m_vecData[i].strNumber.IsEmpty())
		{
			TRACE(_T("跳过空号码记录 #%d\n"), i);
			continue;
		}
		
		CString strItemInfo;
		strItemInfo.Format(_T("保存记录 #%d - 号码: %s, 验证码: %s, 链接: %s"), 
			i, m_vecData[i].strNumber, m_vecData[i].strVerifyCode, m_vecData[i].strLink);
		TRACE(strItemInfo + _T("\n"));
		if (validCount > 0)
			strContent += _T("||-||");
		
		CString link = m_vecData[i].strLink;
		if (link.IsEmpty())
			link = _T("");
			
		CString verifyCode = m_vecData[i].strVerifyCode;
		if (verifyCode.IsEmpty())
			verifyCode = _T("");
			
		CString remark = m_vecData[i].strRemark;
		if (remark.IsEmpty())
			remark = _T("");
		
		// 格式: 号码||验证码||链接||备注 - 严格确保这个格式
		CString formattedRecord;
		formattedRecord.Format(_T("%s||%s||%s||%s"), 
			m_vecData[i].strNumber,
			verifyCode,
			link,
			remark);
			
		strContent += formattedRecord;
		validCount++;
		
		TRACE(_T("已添加记录 #%d: %s\n"), validCount, formattedRecord);
	}
	
	// 写入整个内容
	if (!strContent.IsEmpty())
	{
		file.WriteString(strContent);
		TRACE(_T("成功写入内容，长度: %d\n"), strContent.GetLength());
	}
	else
	{
		TRACE(_T("没有内容需要写入\n"));
	}
	
	file.Close();
	CString strDebug;
	strDebug.Format(_T("成功保存 %d 条记录到: %s"), validCount, strDataFile);
	TRACE(strDebug + _T("\n"));
	
	return TRUE;
}

// 窗口关闭消息处理
void CNumberLibraryDlg::OnClose()
{
	StopVerifyCodeTimer();
	SaveConfig();
	SaveDataFile();
	CDialogEx::OnClose();
}

// 导入文件菜单处理
void CNumberLibraryDlg::OnMenuImport()
{
	CFileDialog dlgFile(TRUE, _T("txt"), NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
		_T("文本文件(*.txt)|*.txt|所有文件(*.*)|*.*||"), this);
	
	if (dlgFile.DoModal() != IDOK)
		return;
	
	CString strFilePath = dlgFile.GetPathName();
	CStdioFile file;
	if (!file.Open(strFilePath, CFile::modeRead | CFile::typeText))
	{
		MessageBox(_T("无法打开文件！"), _T("错误"), MB_OK | MB_ICONERROR);
		return;
	}
	
	UpdateData(TRUE);
	CString strLine;
	int nCount = 0;
	int nFailed = 0;
	CString strFailedLines;
	
	while (file.ReadString(strLine))
	{
		strLine.Trim();
		if (strLine.IsEmpty())
			continue;
			
		CString strDebugLine = strLine;
		TRACE(_T("处理行: %s\n"), strDebugLine);
		if (ParseFileLine(strLine))
		{
			nCount++;
		}
		else
		{
			nFailed++;
			if (nFailed <= 5)
			{
				if (!strFailedLines.IsEmpty())
					strFailedLines += _T("\n");
				strFailedLines += strDebugLine;
			}
		}
	}
	
	file.Close();
	CString strMsg;
	if (nFailed > 0)
	{
		strMsg.Format(_T("成功导入 %d 条记录，失败 %d 条。"), nCount, nFailed);
		if (!strFailedLines.IsEmpty())
		{
			strMsg += _T("\n\n以下是部分失败的行:\n");
			strMsg += strFailedLines;
			
			if (nFailed > 5)
				strMsg += _T("\n...(更多行省略)");
		}
	}
	else
	{
		strMsg.Format(_T("成功导入 %d 条记录。"), nCount);
	}
	
	MessageBox(strMsg, _T("导入完成"), MB_OK | MB_ICONINFORMATION);
}

BOOL CNumberLibraryDlg::ParseFileLine(const CString& strLine)
{
	if (strLine.IsEmpty())
		return FALSE;
	
	CT2CA pszConvertedAnsiString(m_strNumberRegex);
	std::string strRegex(pszConvertedAnsiString);
	if (strRegex.empty())
	{
		MessageBox(_T("号码正则表达式不能为空！"), _T("错误"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	
	CT2CA pszInput(strLine);
	std::string strInput(pszInput);
	
	try
	{
		std::regex pattern(strRegex);
		std::smatch matches;
		if (std::regex_search(strInput, matches, pattern) && matches.size() > 2)
		{
			std::string strNumber = matches[1].str();
			std::string strLink = matches[2].str();
			CString number(strNumber.c_str());
			CString link(strLink.c_str());
			
			if (number.IsEmpty())
				return FALSE;
			
			for (size_t i = 0; i < m_vecData.size(); i++)
			{
				if (m_vecData[i].strNumber == number)
					return FALSE;
			}
			
			AddDataToList(number, _T(""), link, _T("开始获取"), _T(""));
			return TRUE;
		}
	}
	catch (const std::regex_error& e)
	{
		CString strErrorMsg;
		switch (e.code())
		{
		case std::regex_constants::error_collate:
			strErrorMsg = _T("正则错误: 无效的排序元素名称");
			break;
		case std::regex_constants::error_ctype:
			strErrorMsg = _T("正则错误: 无效的字符类名称");
			break;
		case std::regex_constants::error_escape:
			strErrorMsg = _T("正则错误: 无效的转义序列");
			break;
		case std::regex_constants::error_backref:
			strErrorMsg = _T("正则错误: 无效的后向引用");
			break;
		case std::regex_constants::error_brack:
			strErrorMsg = _T("正则错误: 不匹配的方括号");
			break;
		case std::regex_constants::error_paren:
			strErrorMsg = _T("正则错误: 不匹配的圆括号");
			break;
		case std::regex_constants::error_brace:
			strErrorMsg = _T("正则错误: 不匹配的大括号");
			break;
		case std::regex_constants::error_badbrace:
			strErrorMsg = _T("正则错误: 大括号内的无效范围");
			break;
		case std::regex_constants::error_range:
			strErrorMsg = _T("正则错误: 无效的字符范围");
			break;
		case std::regex_constants::error_space:
			strErrorMsg = _T("正则错误: 内存不足");
			break;
		case std::regex_constants::error_badrepeat:
			strErrorMsg = _T("正则错误: 重复操作符前没有有效的正则表达式");
			break;
		case std::regex_constants::error_complexity:
			strErrorMsg = _T("正则错误: 结果复杂度超出预设限制");
			break;
		case std::regex_constants::error_stack:
			strErrorMsg = _T("正则错误: 栈空间不足");
			break;
		default:
			strErrorMsg.Format(_T("正则错误: 未知错误 (%d)"), e.code());
		}
		
		TRACE(_T("%s\n"), strErrorMsg);
		MessageBox(strErrorMsg, _T("正则表达式错误"), MB_OK | MB_ICONERROR);
		return FALSE;
	}

	return FALSE;
}

// 双击列表处理函数
void CNumberLibraryDlg::OnNMDblclkList2(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	int nItem = pNMItemActivate->iItem;
	int nSubItem = pNMItemActivate->iSubItem;
	
	if (nItem >= 0 && nItem < m_listCtrl.GetItemCount())
	{
		// 根据点击的列执行不同操作
		if (nSubItem == 0) // "号码"列
		{
			// 复制号码到剪贴板
			CopyToClipboard(nItem, 0);
		}
		else if (nSubItem == 1) // "验证码"列
		{
			// 复制验证码到剪贴板
			CopyToClipboard(nItem, 1);
		}
		else if (nSubItem == 2) // "打开链接"列
		{
			// 打开链接
			OpenBrowserLink(nItem);
		}
		else if (nSubItem == 3) // "开始获取"列
		{
			// 开始获取验证码
			StartGetVerifyCode(nItem);
		}
		else if (nSubItem == 4) // "备注"列
		{
			CString strRemark = m_vecData[nItem].strRemark;
			CInputDialog dlg(strRemark);
			if (dlg.DoModal() == IDOK)
			{
				m_vecData[nItem].strRemark = dlg.m_strInput;
				m_listCtrl.SetItemText(nItem, 4, dlg.m_strInput);
				SaveDataFile();
			}
		}
	}
	
	*pResult = 0;
}

// 复制内容到剪贴板（通用方法）
BOOL CNumberLibraryDlg::CopyToClipboard(int nItem, int nType)
{
	if (nItem < 0 || nItem >= (int)m_vecData.size())
		return FALSE;
	
	CString strData;
	CString strDataType;
	
	if (nType == 0) // 号码
	{
		strData = m_vecData[nItem].strNumber;
		strDataType = _T("号码");
	}
	else if (nType == 1) // 验证码
	{
		strData = m_vecData[nItem].strVerifyCode;
		strDataType = _T("验证码");
	}
	else
	{
		return FALSE;
	}
	
	if (strData.IsEmpty())
	{
		CString strMsg;
		strMsg.Format(_T("该记录没有%s！"), strDataType);
		MessageBox(strMsg, _T("提示"), MB_OK | MB_ICONINFORMATION);
		return FALSE;
	}
	
	// 复制到剪贴板
	if (OpenClipboard())
	{
		EmptyClipboard();
		HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, (strData.GetLength() + 1) * sizeof(TCHAR));
		if (hGlobal != NULL)
		{
			// 锁定内存
			LPTSTR lpszData = (LPTSTR)GlobalLock(hGlobal);
			if (lpszData != NULL)
			{
				_tcscpy_s(lpszData, strData.GetLength() + 1, strData);
				GlobalUnlock(hGlobal);
				
				#ifdef _UNICODE
				SetClipboardData(CF_UNICODETEXT, hGlobal);
				#else
				SetClipboardData(CF_TEXT, hGlobal);
				#endif
				CloseClipboard();
				
				CString strMsg;
				strMsg.Format(_T("%s \"%s\" 已复制到剪贴板！"), strDataType, strData);
				MessageBox(strMsg, _T("复制成功"), MB_OK | MB_ICONINFORMATION);
				
				return TRUE;
			}
			GlobalFree(hGlobal);
		}
		CloseClipboard();
	}
	
	MessageBox(_T("复制到剪贴板失败！"), _T("错误"), MB_OK | MB_ICONERROR);
	return FALSE;
}

// 打开浏览器链接
BOOL CNumberLibraryDlg::OpenBrowserLink(int nItem)
{
	if (nItem < 0 || nItem >= (int)m_vecData.size())
		return FALSE;
	
	CString strLink = m_vecData[nItem].strLink;
	if (strLink.IsEmpty())
	{
		MessageBox(_T("该记录没有链接！"), _T("错误"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	
	// 确保链接格式正确（以http://或https://开头）
	if (strLink.Left(7).CompareNoCase(_T("http://")) != 0 &&
		strLink.Left(8).CompareNoCase(_T("https://")) != 0)
	{
		strLink = _T("http://") + strLink;
	}
	
	HINSTANCE hInstance = ShellExecute(NULL, _T("open"), strLink, NULL, NULL, SW_SHOWNORMAL);
	
	if ((int)hInstance <= 32)
	{
		CString strError;
		strError.Format(_T("无法打开链接！错误代码: %d"), (int)hInstance);
		MessageBox(strError, _T("错误"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	
	return TRUE;
}

// 开始获取验证码
BOOL CNumberLibraryDlg::StartGetVerifyCode(int nItem)
{
	if (nItem < 0 || nItem >= (int)m_vecData.size())
		return FALSE;
		
	int nStatusIndex = -1;
	for (size_t i = 0; i < m_vecVerifyStatus.size(); i++)
	{
		if (m_vecVerifyStatus[i].nItem == nItem)
		{
			nStatusIndex = (int)i;
			break;
		}
	}
	
	if (nStatusIndex >= 0)
	{
		StopVerifyItem(nStatusIndex, true);
		return TRUE;
	}
	
	if (m_vecData[nItem].strLink.IsEmpty())
	{
		MessageBox(_T("该记录没有链接！"), _T("错误"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	
	m_listCtrl.SetItemText(nItem, 3, _T("停止获取"));
	m_listCtrl.SetItemText(nItem, 1, _T("获取中..."));
	
	// 创建新状态
	VerifyItemStatus status;
	status.nItem = nItem;
	status.bIsVerifying = true;
	status.nVerifyMatchCount = 0;
	status.strLastVerifyCode = _T("");
	status.hThread = NULL;
	status.bThreadRunning = false;
	
	m_vecVerifyStatus.push_back(status);
	nStatusIndex = (int)m_vecVerifyStatus.size() - 1;
	UpdateData(TRUE);
	
	FetchVerifyCode(nItem, nStatusIndex);
	
	if (m_nTimerID == 0)
	{
		m_nTimerID = SetTimer(1, m_nRefreshTime * 1000, NULL);
	}
	
	return TRUE;
}

// 定时器回调
void CNumberLibraryDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == m_nTimerID)
	{
		for (size_t i = 0; i < m_vecVerifyStatus.size(); i++)
		{
			if (!m_vecVerifyStatus[i].bThreadRunning)
			{
				FetchVerifyCode(m_vecVerifyStatus[i].nItem, (int)i);
			}
			else
			{
				TRACE(_T("上一个获取线程仍在运行，跳过本次获取 [项目 %d]\n"), m_vecVerifyStatus[i].nItem);
			}
		}
	}
	
	CDialogEx::OnTimer(nIDEvent);
}

// 停止单个项目的验证
void CNumberLibraryDlg::StopVerifyItem(int nStatusIndex, bool controlUI)
{
	if (nStatusIndex < 0 || nStatusIndex >= (int)m_vecVerifyStatus.size())
		return;
		
	VerifyItemStatus& status = m_vecVerifyStatus[nStatusIndex];
	if (status.bThreadRunning && status.hThread != NULL)
	{
		DWORD dwResult = WaitForSingleObject(status.hThread, 1000);
		if (dwResult == WAIT_TIMEOUT)
		{
			TerminateThread(status.hThread, 0);
			TRACE(_T("强制终止线程 [项目 %d]\n"), status.nItem);
		}
		CloseHandle(status.hThread);
		status.hThread = NULL;
		status.bThreadRunning = false;
	}
	
	if (controlUI && status.nItem >= 0 && status.nItem < m_listCtrl.GetItemCount())
	{
		m_listCtrl.SetItemText(status.nItem, 3, _T("开始获取"));
		if (status.nVerifyMatchCount > 0 && !status.strLastVerifyCode.IsEmpty())
		{
			CString strStatus;
			strStatus.Format(_T("已停止(%d/%d)：%s"), status.nVerifyMatchCount, m_nVerifyCount, status.strLastVerifyCode);
			m_listCtrl.SetItemText(status.nItem, 1, strStatus);
		}
	}
	
	m_vecVerifyStatus.erase(m_vecVerifyStatus.begin() + nStatusIndex);
	if (m_vecVerifyStatus.empty() && m_nTimerID != 0)
	{
		KillTimer(m_nTimerID);
		m_nTimerID = 0;
	}
}

// 获取链接内容并提取验证码 - 启动异步线程
BOOL CNumberLibraryDlg::FetchVerifyCode(int nItem, int nStatusIndex)
{
	if (nItem < 0 || nItem >= (int)m_vecData.size())
		return FALSE;
		
	if (nStatusIndex < 0 || nStatusIndex >= (int)m_vecVerifyStatus.size())
		return FALSE;
		
	VerifyItemStatus& status = m_vecVerifyStatus[nStatusIndex];
	if (status.bThreadRunning && status.hThread != NULL)
	{
		TRACE(_T("等待上一个线程完成... [项目 %d]\n"), nItem);
		WaitForSingleObject(status.hThread, 500); // 等待最多500ms
		CloseHandle(status.hThread);
		status.hThread = NULL;
		status.bThreadRunning = false;
	}
	
	FETCH_THREAD_PARAM* pParam = new FETCH_THREAD_PARAM;
	pParam->pDlg = this;
	pParam->nItem = nItem;
	pParam->nStatusIndex = nStatusIndex;
	status.bThreadRunning = true;
	unsigned int threadID;
	status.hThread = (HANDLE)_beginthreadex(NULL, 0, &CNumberLibraryDlg::FetchThreadProc, pParam, 0, &threadID);
	
	if (status.hThread == NULL)
	{
		delete pParam;
		status.bThreadRunning = false;
		TRACE(_T("创建线程失败！[项目 %d]\n"), nItem);
		return FALSE;
	}
	
	return TRUE;
}

// 添加一个辅助函数用于检查字符串是否为乱码
BOOL IsGarbledText(const CString& strText)
{
	int nSpecialCharCount = 0;
	int nTotalLength = strText.GetLength();
	if (nTotalLength == 0)
		return FALSE;
	
	for (int i = 0; i < min(nTotalLength, 200); i++)
	{
		TCHAR ch = strText[i];
		if ((ch < 32) || 
		    (ch > 126 && ch < 160 && ch != 133) ||
		    ch == 0x3F ||
		    ch == 0xFFFD)
		{
			nSpecialCharCount++;
		}
	}
	
	double fSpecialRatio = (double)nSpecialCharCount / min(nTotalLength, 200);
	
	TRACE(_T("乱码检测 - 总字符: %d, 特殊字符: %d, 比例: %.2f%%\n"), 
		min(nTotalLength, 200), nSpecialCharCount, fSpecialRatio * 100);
	return (fSpecialRatio > 0.20);
}

// 改进编码转换函数
CString ConvertEncoding(const CStringA& strContentA)
{
	if (strContentA.IsEmpty())
	{
		TRACE(_T("源内容为空，无需转换编码\n"));
		return _T("");
	}
	
	TRACE(_T("开始尝试转换编码，源内容长度: %d 字节\n"), strContentA.GetLength());
	const int nEncodingsCount = 5;
	UINT uCodePages[nEncodingsCount] = {
		CP_UTF8,    // UTF-8
		CP_ACP,     // 系统默认ANSI
		936,        // 简体中文GB2312/GBK
		950,        // 繁体中文Big5
		65001       // UTF-8 (另一种表示法)
	};
	
	CString strCodePageNames[nEncodingsCount] = {
		_T("UTF-8 (CP_UTF8)"),
		_T("系统默认ANSI (CP_ACP)"),
		_T("简体中文GB2312/GBK (936)"),
		_T("繁体中文Big5 (950)"),
		_T("UTF-8 (65001)")
	};
	
	CString strResult;
	
	// 尝试每种编码，返回第一个不是乱码的结果
	for (int i = 0; i < nEncodingsCount; i++)
	{
		TRACE(_T("尝试使用编码: %s\n"), strCodePageNames[i]);
		
		int nLength = MultiByteToWideChar(uCodePages[i], 0, strContentA, strContentA.GetLength(), NULL, 0);
		if (nLength <= 0)
		{
			TRACE(_T("  编码 %s 计算缓冲区大小失败，错误码: %d\n"), strCodePageNames[i], GetLastError());
			continue;
		}
		
		LPWSTR pszWide = new WCHAR[nLength + 1];
		if (!pszWide)
		{
			TRACE(_T("  编码 %s 内存分配失败\n"), strCodePageNames[i]);
			continue;
		}
		
		int nConverted = MultiByteToWideChar(uCodePages[i], 0, strContentA, strContentA.GetLength(), pszWide, nLength);
		if (nConverted <= 0)
		{
			TRACE(_T("  编码 %s 转换失败，错误码: %d\n"), strCodePageNames[i], GetLastError());
			delete[] pszWide;
			continue;
		}
		
		pszWide[nConverted] = 0;
		strResult = pszWide;
		delete[] pszWide;
		if (!IsGarbledText(strResult))
		{
			TRACE(_T("成功使用编码 %s 解析内容，转换后长度: %d 字符\n"), 
				strCodePageNames[i], strResult.GetLength());
				
			// 检查是否包含中文字符
			bool hasChinese = false;
			for (int j = 0; j < min(strResult.GetLength(), 200) && !hasChinese; j++)
			{
				TCHAR ch = strResult[j];
				if (ch >= 0x4E00 && ch <= 0x9FFF)  // 基本汉字范围
				{
					hasChinese = true;
				}
			}
			
			if (hasChinese)
			{
				TRACE(_T("转换后的内容包含中文字符\n"));
			}
			
			return strResult;
		}
		else
		{
			TRACE(_T("  编码 %s 转换结果包含乱码，尝试下一种编码\n"), strCodePageNames[i]);
		}
	}
	
	TRACE(_T("所有编码尝试均失败，使用默认转换\n"));
	return CString(strContentA);
}

// 异步线程函数 - 获取验证码
UINT WINAPI CNumberLibraryDlg::FetchThreadProc(LPVOID pParam)
{
	FETCH_THREAD_PARAM* pThreadParam = (FETCH_THREAD_PARAM*)pParam;
	CNumberLibraryDlg* pDlg = pThreadParam->pDlg;
	int nItem = pThreadParam->nItem;
	int nStatusIndex = pThreadParam->nStatusIndex;

	delete pThreadParam;
	if (nStatusIndex < 0 || nStatusIndex >= (int)pDlg->m_vecVerifyStatus.size() || 
		pDlg->m_vecVerifyStatus[nStatusIndex].nItem != nItem)
	{
		TRACE(_T("线程发现状态已无效 [项目 %d]\n"), nItem);
		return 0;
	}

	VerifyItemStatus& status = pDlg->m_vecVerifyStatus[nStatusIndex];

	CString strLink = pDlg->m_vecData[nItem].strLink;
	if (strLink.Left(7).CompareNoCase(_T("http://")) != 0 &&
		strLink.Left(8).CompareNoCase(_T("https://")) != 0)
	{
		strLink = _T("http://") + strLink;
	}
	
	TRACE(_T("正在访问链接: %s [项目 %d]\n"), strLink, nItem);
	IStream* pStream = NULL;
	HRESULT hr = URLOpenBlockingStream(NULL, strLink, &pStream, 0, NULL);
	
	if (FAILED(hr) || pStream == NULL)
	{
		TRACE(_T("无法访问链接: %s, 错误码: 0x%08X [项目 %d]\n"), strLink, hr, nItem);

		CString strError;
		strError.Format(_T("无法访问链接，错误码: 0x%08X"), hr);
		status.bThreadRunning = false;
		pDlg->PostMessage(WM_USER + 100, MAKEWPARAM(nItem, nStatusIndex), (LPARAM)new CString(strError));
		return 1;
	}
	
	// 读取网页内容
	const int BUFFER_SIZE = 4096;
	char buffer[BUFFER_SIZE];
	ULONG bytesRead = 0;
	CStringA strContentA;
	
	while (SUCCEEDED(pStream->Read(buffer, BUFFER_SIZE - 1, &bytesRead)) && bytesRead > 0)
	{
		buffer[bytesRead] = 0;
		strContentA += buffer;
	}

	pStream->Release();
	CString strContent = ConvertEncoding(strContentA);

	if (strContent.GetLength() > 0)
	{
		CString strPreview = strContent.Left(min(200, strContent.GetLength()));
		TRACE(_T("获取到的内容预览: %s [项目 %d]\n"), strPreview, nItem);
		TRACE(_T("内容总长度: %d [项目 %d]\n"), strContent.GetLength(), nItem);
	}
	else
	{
		TRACE(_T("获取到的内容为空 [项目 %d]\n"), nItem);
		status.bThreadRunning = false;
		pDlg->PostMessage(WM_USER + 100, MAKEWPARAM(nItem, nStatusIndex), (LPARAM)new CString(_T("内容为空")));
		return 1;
	}
	

	TRACE(_T("使用的正则表达式: %s [项目 %d]\n"), pDlg->m_strVerifyCodeRegex, nItem);

	CString strVerifyCode;
	if (pDlg->ExtractVerifyCodeFromContent(strContent, strVerifyCode))
	{
		TRACE(_T("成功提取验证码: %s [项目 %d]\n"), strVerifyCode, nItem);
		pDlg->PostMessage(WM_USER + 101, MAKEWPARAM(nItem, nStatusIndex), (LPARAM)new CString(strVerifyCode));
	}
	else
	{
		TRACE(_T("无法从内容中提取验证码 [项目 %d]\n"), nItem);
		pDlg->PostMessage(WM_USER + 100, MAKEWPARAM(nItem, nStatusIndex), (LPARAM)new CString(_T("未匹配到验证码")));
	}

	status.bThreadRunning = false;
	return 0;
}

// 从网页内容中提取验证码
BOOL CNumberLibraryDlg::ExtractVerifyCodeFromContent(const CString& strContent, CString& strVerifyCode)
{
	CT2CA pszContentAnsi(strContent);
	std::string strContentAnsi(pszContentAnsi);

	CT2CA pszRegexAnsi(m_strVerifyCodeRegex);
	std::string strRegexAnsi(pszRegexAnsi);
	
	try
	{
		std::regex pattern(strRegexAnsi);
		std::smatch matches;

		TRACE(_T("尝试用正则表达式 %s 匹配内容\n"), m_strVerifyCodeRegex);
		
		if (std::regex_search(strContentAnsi, matches, pattern) && matches.size() > 1)
		{
			std::string strMatchCode = matches[1].str();
			strVerifyCode = CString(strMatchCode.c_str());

			TRACE(_T("正则匹配成功，找到验证码: %s\n"), strVerifyCode);
			TRACE(_T("匹配组数量: %d\n"), (int)matches.size());
			for (size_t i = 0; i < min((size_t)5, matches.size()); i++)
			{
				TRACE(_T("匹配组[%d]: %s\n"), (int)i, CString(matches[i].str().c_str()));
			}
			
			return TRUE;
		}
		else
		{
			TRACE(_T("正则匹配失败，没有找到匹配项\n"));
			
			if (strContent.Find(_T("YES")) != -1)
			{
				TRACE(_T("内容中包含关键字'YES'\n"));
			}
			
			std::regex simplePattern("\\d+");
			std::smatch simpleMatches;
			std::string str = strContentAnsi;
			
			TRACE(_T("尝试提取内容中的所有数字:\n"));
			int count = 0;
			while (std::regex_search(str, simpleMatches, simplePattern) && count < 10)
			{
				TRACE(_T("找到数字: %s\n"), CString(simpleMatches[0].str().c_str()));
				str = simpleMatches.suffix().str();
				count++;
			}
		}
	}
	catch (const std::regex_error& e)
	{
		CString strErrorMsg;
		switch (e.code())
		{
		case std::regex_constants::error_collate:
			strErrorMsg = _T("正则错误: 无效的排序元素名称");
			break;
		case std::regex_constants::error_ctype:
			strErrorMsg = _T("正则错误: 无效的字符类名称");
			break;
		case std::regex_constants::error_escape:
			strErrorMsg = _T("正则错误: 无效的转义序列");
			break;
		case std::regex_constants::error_backref:
			strErrorMsg = _T("正则错误: 无效的后向引用");
			break;
		case std::regex_constants::error_brack:
			strErrorMsg = _T("正则错误: 不匹配的方括号");
			break;
		case std::regex_constants::error_paren:
			strErrorMsg = _T("正则错误: 不匹配的圆括号");
			break;
		case std::regex_constants::error_brace:
			strErrorMsg = _T("正则错误: 不匹配的大括号");
			break;
		case std::regex_constants::error_badbrace:
			strErrorMsg = _T("正则错误: 大括号内的无效范围");
			break;
		case std::regex_constants::error_range:
			strErrorMsg = _T("正则错误: 无效的字符范围");
			break;
		case std::regex_constants::error_space:
			strErrorMsg = _T("正则错误: 内存不足");
			break;
		case std::regex_constants::error_badrepeat:
			strErrorMsg = _T("正则错误: 重复操作符前没有有效的正则表达式");
			break;
		case std::regex_constants::error_complexity:
			strErrorMsg = _T("正则错误: 结果复杂度超出预设限制");
			break;
		case std::regex_constants::error_stack:
			strErrorMsg = _T("正则错误: 栈空间不足");
			break;
		default:
			strErrorMsg.Format(_T("正则错误: 未知错误 (%d)"), e.code());
		}
		
		TRACE(_T("%s\n"), strErrorMsg);
		MessageBox(strErrorMsg, _T("正则表达式错误"), MB_OK | MB_ICONERROR);
	}
	
	return FALSE;
}

// 停止验证码获取
void CNumberLibraryDlg::StopVerifyCodeTimer()
{
	if (m_nTimerID != 0)
	{
		KillTimer(m_nTimerID);
		m_nTimerID = 0;
	}
	
	while (!m_vecVerifyStatus.empty())
	{
		StopVerifyItem(0, true);
	}
}

// 添加一个菜单项和处理函数，用于测试当前的验证码正则表达式
void CNumberLibraryDlg::OnMenuTestRegex()
{
	class CTestRegexDlg : public CDialog
	{
	public:
		CString m_strContent;
		CString m_strRegex;
		
		CTestRegexDlg(CString strRegex) 
			: CDialog(IDD_DIALOG_SIMPLE), m_strRegex(strRegex)
		{
		}
		
		void DoDataExchange(CDataExchange* pDX)
		{
			CDialog::DoDataExchange(pDX);
			DDX_Text(pDX, IDC_EDIT_REMARK, m_strContent);
		}
		
		BOOL OnInitDialog()
		{
			CDialog::OnInitDialog();
			SetWindowText(_T("测试正则表达式"));
			GetDlgItem(IDC_STATIC)->SetWindowText(_T("请输入要测试的内容:"));
			return TRUE;
		}
	};
	
	UpdateData(TRUE);

	CTestRegexDlg dlg(m_strVerifyCodeRegex);
	if (dlg.DoModal() == IDOK && !dlg.m_strContent.IsEmpty())
	{
		CString strVerifyCode;
		if (ExtractVerifyCodeFromContent(dlg.m_strContent, strVerifyCode))
		{
			CString strMsg;
			strMsg.Format(_T("成功提取验证码: %s\n\n正则表达式: %s"), 
				strVerifyCode, m_strVerifyCodeRegex);
			MessageBox(strMsg, _T("测试成功"), MB_OK | MB_ICONINFORMATION);
		}
		else
		{
			CString strMsg;
			strMsg.Format(_T("未能从内容中提取验证码\n\n正则表达式: %s\n\n请检查内容格式或正则表达式。"), 
				m_strVerifyCodeRegex);
			MessageBox(strMsg, _T("测试失败"), MB_OK | MB_ICONWARNING);
		}
	}
}

// 处理获取验证码错误消息
LRESULT CNumberLibraryDlg::OnFetchError(WPARAM wParam, LPARAM lParam)
{
	int nItem = LOWORD(wParam);
	int nStatusIndex = HIWORD(wParam);
	CString* pStrError = (CString*)lParam;
	
	TRACE(_T("获取错误: %s [项目 %d]\n"), *pStrError, nItem);
	if (nItem >= 0 && nItem < m_listCtrl.GetItemCount())
	{
		m_listCtrl.SetItemText(nItem, 1, *pStrError);
	}

	delete pStrError;
	
	return 0;
}

// 处理获取验证码成功消息
LRESULT CNumberLibraryDlg::OnFetchSuccess(WPARAM wParam, LPARAM lParam)
{
	int nItem = LOWORD(wParam);
	int nStatusIndex = HIWORD(wParam);
	CString* pStrVerifyCode = (CString*)lParam;

	if (nStatusIndex < 0 || nStatusIndex >= (int)m_vecVerifyStatus.size() || 
		m_vecVerifyStatus[nStatusIndex].nItem != nItem)
	{
		TRACE(_T("消息处理：状态已无效 [项目 %d]\n"), nItem);
		delete pStrVerifyCode;
		return 0;
	}
	
	VerifyItemStatus& status = m_vecVerifyStatus[nStatusIndex];

	if (nItem < 0 || nItem >= m_listCtrl.GetItemCount() || !status.bIsVerifying)
	{
		delete pStrVerifyCode;
		return 0;
	}
	
	// 检查是否与上次相同
	if (!status.strLastVerifyCode.IsEmpty() && status.strLastVerifyCode == *pStrVerifyCode)
	{
		status.nVerifyMatchCount++;
		TRACE(_T("验证码匹配计数: %d/%d [项目 %d]\n"), status.nVerifyMatchCount, m_nVerifyCount, nItem);
		
		// 检查是否达到验证次数
		if (status.nVerifyMatchCount >= m_nVerifyCount)
		{
			// 保存验证码
			m_vecData[nItem].strVerifyCode = *pStrVerifyCode;
			m_listCtrl.SetItemText(nItem, 1, *pStrVerifyCode);
			m_listCtrl.SetItemText(nItem, 3, _T("开始获取"));
			StopVerifyItem(nStatusIndex, true);
			CString strMsg;
			strMsg.Format(_T("项目 #%d 成功获取验证码: %s"), nItem + 1, *pStrVerifyCode);
			MessageBox(strMsg, _T("获取成功"), MB_OK | MB_ICONINFORMATION);
			SaveDataFile();
			delete pStrVerifyCode;
			return 0;
		}
	}
	else
	{
		status.nVerifyMatchCount = 1;
		status.strLastVerifyCode = *pStrVerifyCode;
		TRACE(_T("验证码与上次不同，重置计数: 1/%d [项目 %d]\n"), m_nVerifyCount, nItem);
	}
	

	CString strStatus;
	strStatus.Format(_T("获取中(%d/%d)..."), status.nVerifyMatchCount, m_nVerifyCount);
	m_listCtrl.SetItemText(nItem, 1, strStatus);

	delete pStrVerifyCode;
	
	return 0;
}

// 关于菜单处理函数
void CNumberLibraryDlg::OnProgramAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

CNumberLibraryDlg::~CNumberLibraryDlg()
{
	StopVerifyCodeTimer();
}

// 右键菜单处理函数
void CNumberLibraryDlg::OnNMRClickList2(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	CPoint ptCursor;
	GetCursorPos(&ptCursor);
	int nItem = pNMItemActivate->iItem;
	CMenu menu;
	menu.CreatePopupMenu();
	menu.AppendMenu(MF_STRING, ID_MENU_DELETE_ITEM, _T("删除此项"));
	menu.AppendMenu(MF_STRING, ID_MENU_DELETE_ALL, _T("全部删除"));
	if (nItem < 0)
	{
		menu.EnableMenuItem(ID_MENU_DELETE_ITEM, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	}
	
	menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptCursor.x, ptCursor.y, this);
	
	*pResult = 0;
}

// 删除菜单处理函数
void CNumberLibraryDlg::OnMenuDeleteItem()
{
	POSITION pos = m_listCtrl.GetFirstSelectedItemPosition();
	if (pos == NULL)
	{
		MessageBox(_T("请先选择一个项目！"), _T("提示"), MB_OK | MB_ICONINFORMATION);
		return;
	}

	int nItem = m_listCtrl.GetNextSelectedItem(pos);
	
	// 确认删除
	CString strConfirm;
	strConfirm.Format(_T("确定要删除 \"%s\" 这一项吗？"), m_vecData[nItem].strNumber);
	if (MessageBox(strConfirm, _T("确认删除"), MB_YESNO | MB_ICONQUESTION) != IDYES)
		return;
	
	
	if (DeleteItem(nItem))
	{
		SaveDataFile();
	}
}

// 全部删除菜单处理函数
void CNumberLibraryDlg::OnMenuDeleteAll()
{
	// 检查是否有数据
	if (m_listCtrl.GetItemCount() == 0)
	{
		MessageBox(_T("列表为空，没有可删除的项目！"), _T("提示"), MB_OK | MB_ICONINFORMATION);
		return;
	}
	
	// 确认删除
	if (MessageBox(_T("确定要删除所有项目吗？此操作不可恢复！"), 
		_T("确认删除"), MB_YESNO | MB_ICONQUESTION) != IDYES)
		return;
	
	if (DeleteAllItems())
	{
		SaveDataFile();
	}
}

// 删除指定项
BOOL CNumberLibraryDlg::DeleteItem(int nItem)
{
	// 检查索引是否有效
	if (nItem < 0 || nItem >= (int)m_vecData.size())
		return FALSE;
	
	
	int nStatusIndex = -1;
	for (size_t i = 0; i < m_vecVerifyStatus.size(); i++)
	{
		if (m_vecVerifyStatus[i].nItem == nItem)
		{
			nStatusIndex = (int)i;
			break;
		}
	}
	
	if (nStatusIndex >= 0)
	{
		StopVerifyItem(nStatusIndex, false);  // 不控制UI，因为稍后会删除该项
	}
	
	// 获取当前项的数据，用于日志
	CString strDebug;
	strDebug.Format(_T("删除项 #%d - 号码: %s, 验证码: %s, 链接: %s"), 
		nItem, m_vecData[nItem].strNumber, m_vecData[nItem].strVerifyCode, m_vecData[nItem].strLink);
	TRACE(strDebug + _T("\n"));
	m_vecData.erase(m_vecData.begin() + nItem);
	m_listCtrl.DeleteItem(nItem);
	
	// 更新其他正在验证的项目的索引
	for (size_t i = 0; i < m_vecVerifyStatus.size(); i++)
	{
		if (m_vecVerifyStatus[i].nItem > nItem)
		{
			m_vecVerifyStatus[i].nItem--;
		}
	}
	
	return TRUE;
}

// 删除所有项
BOOL CNumberLibraryDlg::DeleteAllItems()
{
	StopVerifyCodeTimer();
	
	m_vecData.clear();
	
	m_listCtrl.DeleteAllItems();
	
	return TRUE;
}

