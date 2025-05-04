// NumberLibraryDlg.h: 头文件
//

#pragma once
#include <vector>

// 定义数据结构
struct NumberData
{
	CString strNumber;   // 号码
	CString strLink;     // 链接
	CString strVerifyCode; // 验证码
	CString strStartGet;   // 开始获取
	CString strRemark;     // 备注
};

// CNumberLibraryDlg 对话框
class CNumberLibraryDlg : public CDialogEx
{
// 构造
public:
	CNumberLibraryDlg(CWnd* pParent = nullptr);	// 标准构造函数
	virtual ~CNumberLibraryDlg();  // 析构函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_NUMBERLIBRARY_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_listCtrl;  // 列表控件变量
	
	// 数据存储
	std::vector<NumberData> m_vecData;  // 数据数组
	
	// 配置参数相关变量
	int m_nRefreshTime;    // 刷新时间
	int m_nVerifyCount;    // 验证次数
	CString m_strNumberRegex;  // 号码正则
	CString m_strVerifyCodeRegex;  // 验证码正则
	
	// 验证码获取相关变量
	UINT_PTR m_nTimerID;   // 定时器ID
	HANDLE m_hFetchThread; // 获取线程句柄
	bool m_bThreadRunning; // 线程是否在运行
	
	// 每个项目的验证状态
	struct VerifyItemStatus
	{
		int nItem;              // 项目索引
		bool bIsVerifying;      // 是否正在验证
		int nVerifyMatchCount;  // 匹配计数
		CString strLastVerifyCode; // 上次验证码
		HANDLE hThread;         // 线程句柄
		bool bThreadRunning;    // 线程是否运行中
		
		VerifyItemStatus() : nItem(-1), bIsVerifying(false), 
			nVerifyMatchCount(0), hThread(NULL), bThreadRunning(false) {}
	};
	
	// 状态管理
	std::vector<VerifyItemStatus> m_vecVerifyStatus; // 所有正在验证的项目
	
	// 控件变量关联
	CEdit m_editRefreshTime;
	CEdit m_editVerifyCount;
	CEdit m_editNumberRegex;
	CEdit m_editVerifyCodeRegex;
	
	// 配置文件操作方法
	BOOL LoadConfig();  // 加载配置
	BOOL SaveConfig();  // 保存配置
	
	// 数据文件操作方法
	BOOL LoadDataFile();  // 加载数据文件
	BOOL SaveDataFile();  // 保存数据文件
	
	// 添加数据到列表的方法
	void AddDataToList(const CString& number, const CString& verifyCode, 
		const CString& link, const CString& startGet, const CString& remark);
	
	// 窗口关闭消息处理
	afx_msg void OnClose();
	
	// 菜单相关处理
	afx_msg void OnMenuImport();  // 导入文件菜单处理
	afx_msg void OnMenuTestRegex(); // 测试正则表达式菜单处理
	
	// 解析文件行
	BOOL ParseFileLine(const CString& strLine);
	
	// 双击列表处理函数
	afx_msg void OnNMDblclkList2(NMHDR* pNMHDR, LRESULT* pResult);
	
	// 打开链接方法
	BOOL OpenBrowserLink(int nItem);
	
	// 复制内容到剪贴板（通用方法）
	BOOL CopyToClipboard(int nItem, int nType);
	
	// 开始获取验证码
	BOOL StartGetVerifyCode(int nItem);
	
	// 验证码获取计时器回调
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	
	// 获取链接内容并提取验证码
	BOOL FetchVerifyCode(int nItem, int nStatusIndex);
	
	// 异步线程函数
	static UINT WINAPI FetchThreadProc(LPVOID pParam);
	
	// 获取线程参数结构
	struct FETCH_THREAD_PARAM
	{
		CNumberLibraryDlg* pDlg;
		int nItem;
		int nStatusIndex;  // 状态在数组中的索引
	};
	
	// 从网页内容中提取验证码
	BOOL ExtractVerifyCodeFromContent(const CString& strContent, CString& strVerifyCode);
	
	// 停止获取验证码
	void StopVerifyCodeTimer();
	
	// 停止单个项目的验证
	void StopVerifyItem(int nStatusIndex, bool controlUI = true);
	
	// 处理获取验证码错误消息
	afx_msg LRESULT OnFetchError(WPARAM wParam, LPARAM lParam);
	
	// 处理获取验证码成功消息
	afx_msg LRESULT OnFetchSuccess(WPARAM wParam, LPARAM lParam);
	
	// 处理关于菜单
	afx_msg void OnProgramAbout();
	
	// 处理右键菜单
	afx_msg void OnNMRClickList2(NMHDR* pNMHDR, LRESULT* pResult);
	
	// 处理删除项菜单
	afx_msg void OnMenuDeleteItem();
	
	// 处理全部删除菜单
	afx_msg void OnMenuDeleteAll();
	
	// 删除指定项
	BOOL DeleteItem(int nItem);
	
	// 删除所有项
	BOOL DeleteAllItems();
};

// 辅助函数 - 获取剪贴板文本
BOOL GetClipboardText(CString& strText);

// 对话框类声明
class CInputDialog : public CDialog
{
public:
	CString m_strInput;
	
	CInputDialog(CString strInitValue = _T(""));
	
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	
	DECLARE_MESSAGE_MAP()
};

// 关于对话框类声明
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	enum { IDD = IDD_DIALOG_ABOUT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	afx_msg void OnStnClickedStaticWebsite();
	afx_msg void OnStnClickedStaticGithub();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	DECLARE_MESSAGE_MAP()

private:
	CFont m_linkFont;
	HCURSOR m_hLinkCursor;
	CStatic m_websiteLink;
	CStatic m_githubLink;
};
