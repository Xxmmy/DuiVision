#include "StdAfx.h"
#include "MenuEx.h"

CMenuEx::CMenuEx(CString strFont/* = TEXT("宋体")*/, int nFontWidth/* = 12*/, FontStyle fontStyle/* = FontStyleRegular*/)
	: CDlgPopup()
{
	m_strFont = strFont;
	m_nFontWidth = nFontWidth;
	m_fontStyle = fontStyle;
	m_uAlignment = DT_LEFT;
	m_uVAlignment = DT_TOP;

	m_nLeft = 30;
	m_nHeight = 30;
	m_nWidth = 113;
	m_nSeparatorHeight = 4;
}

CMenuEx::~CMenuEx(void)
{
}

BOOL CMenuEx::Create(CWnd *pParent, CPoint point, UINT uMessageID, UINT nResourceID, int nFrameSize/* = 4*/, int nMinWidth/* = 112*/, enumBackMode enBackMode/* = enBMFrame*/)
{
	CRect rc(point.x - nMinWidth / 2, point.y, point.x + nMinWidth / 2, point.y + nMinWidth);
	return CDlgPopup::Create(pParent, rc, uMessageID, nResourceID, enBackMode, nFrameSize);
}

BOOL CMenuEx::Create(CWnd *pParent, CPoint point, UINT uMessageID, CString strImage, int nFrameSize/* = 4*/, int nMinWidth/* = 112*/, enumBackMode enBackMode/* = enBMFrame*/)
{
	CRect rc(point.x - nMinWidth / 2, point.y, point.x + nMinWidth / 2, point.y + nMinWidth);
	return CDlgPopup::Create(pParent, rc, uMessageID, strImage, enBackMode, nFrameSize);
}

BOOL CMenuEx::Create(CWnd *pParent, CPoint point, UINT uMessageID)
{
	int nMinWidth = m_nWidth;
	CRect rc(point.x - nMinWidth / 2, point.y, point.x + nMinWidth / 2, point.y + nMinWidth);
	return CDlgPopup::Create(pParent, rc, uMessageID);
}

void CMenuEx::DrawWindowEx(CDC &dc, CRect rcClient)
{

}

// 重载加载XML节点函数，加载下层的Menu item信息
BOOL CMenuEx::Load(TiXmlElement* pXmlElem, BOOL bLoadSubControl)
{
	SetRect(CRect(0, 0, m_nWidth, m_nHeight));

	__super::Load(pXmlElem, bLoadSubControl);

	if(pXmlElem == NULL)
	{
		return FALSE;
	}

	if(!bLoadSubControl)
	{
		// 不加载子控件
		return TRUE;
	}

	// 菜单窗口宽度设置为popup窗口的宽度
	m_nWidth = m_size.cx;

	// 创建窗口
	Create(m_pParent, m_point, m_uMessageID);
	
	// 加载下层的item节点信息(正常情况下都使用DlgPopup的Load控件方式加载菜单项,下面的解析比较少用到)
	int nIdIndex = 100;
	TiXmlElement* pItemElem = NULL;
	for (pItemElem = pXmlElem->FirstChildElement("item"); pItemElem != NULL; pItemElem=pItemElem->NextSiblingElement())
	{
		CStringA strId = pItemElem->Attribute("id");
		int nId = nIdIndex;
		if(strId != "")
		{
			nId = atoi(strId);
		}

		CStringA strType = pItemElem->Attribute("type");
		CStringA strName = pItemElem->Attribute("name");
		CStringA strImage = pItemElem->Attribute("image");
		CStringA strTitle = pItemElem->Attribute("title");
		
		if(strType == "separator")
		{
			// 分隔线也可以用图片的方式
			AddSeparator();
			continue;
		}
		CString strTitleU = CA2T(strTitle, CP_UTF8);
		if(strImage.Find(".") != -1)	// 加载图片文件
		{
			CString strImgFile = DuiSystem::GetSkinPath() + CEncodingUtil::AnsiToUnicode(strImage);
			AddMenu(strTitleU, nIdIndex, strImgFile);
		}else
		if(!strImage.IsEmpty())
		{
			UINT nResourceID = atoi(strImage);
			AddMenu(strTitleU, nIdIndex, nResourceID);
		}else
		{
			AddMenu(strTitleU, nIdIndex);
		}

		nIdIndex++;
	}

	// 刷新各菜单控件的位置
	SetMenuPoint();

	m_bInit = TRUE;

    return TRUE;
}

// 加载指定名字的菜单节点
BOOL CMenuEx::LoadSubMenu(TiXmlElement* pXmlElem, CString strSubItemName)
{
	if(pXmlElem == NULL)
	{
		return FALSE;
	}

	// 递归遍历下层节点,看是否有指定名字的节点
	TiXmlElement* pItemElem = NULL;
	for (pItemElem = pXmlElem->FirstChildElement(); pItemElem != NULL; pItemElem=pItemElem->NextSiblingElement())
	{
		CStringA strNameA = pItemElem->Attribute("name");
		if(strSubItemName == CEncodingUtil::AnsiToUnicode(strNameA))
		{
			// 加载子菜单
			return Load(pItemElem);
		}
		if(LoadSubMenu(pItemElem, strSubItemName))
		{
			// 如果递归加载成功则返回,否则继续向下遍历查找
			return TRUE;
		}
	}
	return FALSE;
}

// 加载XML节点中定义的菜单和其他控件
BOOL CMenuEx::LoadXmlNode(TiXmlElement* pXmlElem, CString strXmlFile)
{
	if(pXmlElem == NULL)
	{
		return FALSE;
	}

	TiXmlElement* pControlElem = NULL;
	for (pControlElem = pXmlElem->FirstChildElement(); pControlElem != NULL; pControlElem=pControlElem->NextSiblingElement())
	{
		if(pControlElem != NULL)
		{
			CStringA strControlName = pControlElem->Value();
			CControlBase* pControl = _CreateControlByName(strControlName);
			if(pControl)
			{
				if(pControl->Load(pControlElem))
				{
					// 如果Load成功,则添加控件
					if(pControl->IsClass(CArea::GetClassName()) || pControl->IsClass(CFrame::GetClassName()))
					{
						// Area和Frame不能响应鼠标,必须加到Area列表中
						m_vecArea.push_back(pControl);
					}else
					{
						m_vecControl.push_back(pControl);
					}
					// 如果是菜单项控件,则设置菜单项的菜单XML属性
					if(pControl->IsClass(CMenuItem::GetClassName()))
					{
						((CMenuItem*)pControl)->SetMenuXml(strXmlFile);
					}
				}else
				{
					// 否则直接删除控件对象指针
					delete pControl;
				}
			}
		}
	}

	return TRUE;
}

// 加载XML文件中定义的菜单
BOOL CMenuEx::LoadXmlFile(CString strFileName, CString strSubItemName)
{
	TiXmlDocument xmlDoc;
	TiXmlElement* pDivElem = NULL;

	CString strXmlFile;
	if(strFileName.Find(_T(":")) == -1)
	{
		if(strFileName.Find(_T(".xml")) == -1)
		{
			strXmlFile = DuiSystem::Instance()->GetXmlFile(CEncodingUtil::UnicodeToAnsi(strFileName));
		}else
		{
			strXmlFile = DuiSystem::GetXmlPath() + strFileName;
		}
	}else
	{
		strXmlFile = strFileName;
	}

	xmlDoc.LoadFile(CEncodingUtil::UnicodeToAnsi(strXmlFile), TIXML_ENCODING_UTF8);
	if(!xmlDoc.Error())
	{
		pDivElem = xmlDoc.FirstChildElement(GetClassName());
		if(pDivElem != NULL)
		{
			if(!strSubItemName.IsEmpty())
			{
				// 加载一下XML的root节点的内容(不加载子控件)
				Load(pDivElem, FALSE);
				// 指定了子菜单名,则按照子菜单名进行查找并加载
				LoadSubMenu(pDivElem, strSubItemName);
			}else
			{
				// 加载menu节点(可以设置节点菜单项的XML文件属性)
				LoadXmlNode(pDivElem, strFileName);
			}
		}
	}

	return TRUE;
}

// 加载XML文件
BOOL CMenuEx::LoadXmlFile(CString strFileName, CWnd *pParent, CPoint point, UINT uMessageID, CString strSubItemName)
{
	m_pParent = pParent;
	m_point = point;
	m_uMessageID = uMessageID;

	TiXmlDocument xmlDoc;
	TiXmlElement* pDivElem = NULL;

	if(strFileName.Find(_T(":")) == -1)
	{
		if(strFileName.Find(_T(".xml")) == -1)
		{
			m_strXmlFile = DuiSystem::Instance()->GetXmlFile(CEncodingUtil::UnicodeToAnsi(strFileName));
		}else
		{
			m_strXmlFile = DuiSystem::GetXmlPath() + strFileName;
		}
	}else
	{
		m_strXmlFile = strFileName;
	}

	BOOL bRet = TRUE;
	xmlDoc.LoadFile(CEncodingUtil::UnicodeToAnsi(m_strXmlFile), TIXML_ENCODING_UTF8);
	if(!xmlDoc.Error())
	{
		pDivElem = xmlDoc.FirstChildElement(GetClassName());
		if(pDivElem != NULL)
		{
			if(!strSubItemName.IsEmpty())
			{
				// 加载一下XML的root节点的内容(不加载子控件)
				Load(pDivElem, FALSE);
				// 指定了子菜单名,则按照子菜单名进行查找并加载
				bRet = LoadSubMenu(pDivElem, strSubItemName);
			}else
			{
				// 加载menu节点属性
				bRet = Load(pDivElem);
			}
		}
	}

	return bRet;
}

// UI初始化,此函数在窗口的OnCreate函数中调用
void CMenuEx::InitUI(CRect rcClient)
{
	// 如果有菜单项的预设置值,则设置相应的值到控件
	if(m_vecMenuItemValue.size() > 0)
	{
		for (size_t i = 0; i < m_vecMenuItemValue.size(); i++)
		{
			MenuItemValue& itemValue = m_vecMenuItemValue.at(i);
			CControlBase* pControlBase = GetControl(itemValue.strName);
			if(pControlBase)
			{
				if(!itemValue.strTitle.IsEmpty())
				{
					((CControlBaseFont*)pControlBase)->SetTitle(itemValue.strTitle);
				}
				if(!itemValue.bVisible)
				{
					pControlBase->SetVisible(FALSE);
				}
				if(itemValue.bDisable)
				{
					pControlBase->SetDisable(TRUE);
				}
				if(itemValue.nCheck != -1)
				{
					((CMenuItem*)pControlBase)->SetCheck(itemValue.nCheck);
				}
			}
		}

		// 刷新菜单项位置信息
		SetMenuPoint();
	}
}

// 添加菜单项预设置信息(设置菜单项标题)
void CMenuEx::SetItemTitle(CString strName, CString strTitle)
{
	MenuItemValue itemValue;
	itemValue.strName = strName;
	itemValue.strTitle = strTitle;
	itemValue.bVisible = TRUE;
	itemValue.bDisable = FALSE;
	itemValue.nCheck = -1;
	m_vecMenuItemValue.push_back(itemValue);
}

// 添加菜单项预设置信息(设置菜单项可见性)
void CMenuEx::SetItemVisible(CString strName, BOOL bVisible)
{
	MenuItemValue itemValue;
	itemValue.strName = strName;
	itemValue.strTitle = _T("");
	itemValue.bVisible = bVisible;
	itemValue.bDisable = FALSE;
	itemValue.nCheck = -1;
	m_vecMenuItemValue.push_back(itemValue);
}

// 添加菜单项预设置信息(设置菜单项是否禁用)
void CMenuEx::SetItemDisable(CString strName, BOOL bDisable)
{
	MenuItemValue itemValue;
	itemValue.strName = strName;
	itemValue.strTitle = _T("");
	itemValue.bVisible = TRUE;
	itemValue.bDisable = bDisable;
	itemValue.nCheck = -1;
	m_vecMenuItemValue.push_back(itemValue);
}

// 添加菜单项预设置信息(设置菜单项是否选择)
void CMenuEx::SetItemCheck(CString strName, int nCheck)
{
	MenuItemValue itemValue;
	itemValue.strName = strName;
	itemValue.strTitle = _T("");
	itemValue.bVisible = TRUE;
	itemValue.bDisable = FALSE;
	itemValue.nCheck = nCheck;
	m_vecMenuItemValue.push_back(itemValue);
}

int CMenuEx::AddMenu(CString strText, UINT uMenuID, int nResourceID, BOOL bSelect, int nIndex)
{
	CControlBase * pControlBase = NULL;

	FontFamily fontFamily(m_strFont.AllocSysString());
	Font font(&fontFamily, (REAL)m_nFontWidth, m_fontStyle, UnitPixel);;

	StringFormat strFormat;
	strFormat.SetAlignment(StringAlignmentNear);
	strFormat.SetFormatFlags( StringFormatFlagsNoWrap | StringFormatFlagsMeasureTrailingSpaces);
	Size size = GetTextBounds(font, strFormat, strText);

	if(size.Width > m_nWidth - m_nLeft - 4)
	{
		m_nWidth = size.Width + m_nLeft + 4;
	}

	pControlBase = new CMenuItem(GetSafeHwnd(),this, uMenuID, CRect(0, 0, 0, 0), strText, m_nLeft, bSelect);
	((CControlBaseFont *)pControlBase)->SetFont(m_strFont, m_nFontWidth, m_fontStyle);
	if(nResourceID != -1)
	{
		((CMenuItem *)pControlBase)->SetBitmap(nResourceID);
	}

	if(nIndex >= 0 && nIndex < m_vecControl.size())
	{
		m_vecControl.insert(m_vecControl.begin() + nIndex, pControlBase);
	}
	else
	{
		m_vecControl.push_back(pControlBase);
	}

	SetMenuPoint();
	return m_vecControl.size();
}

int CMenuEx::AddMenu(CString strText, UINT uMenuID, CString strImage, BOOL bSelect, int nIndex)
{
	CControlBase * pControlBase = NULL;

	FontFamily fontFamily(m_strFont.AllocSysString());
	Font font(&fontFamily, (REAL)m_nFontWidth, m_fontStyle, UnitPixel);;

	StringFormat strFormat;
	strFormat.SetAlignment(StringAlignmentNear);
	strFormat.SetFormatFlags( StringFormatFlagsNoWrap | StringFormatFlagsMeasureTrailingSpaces);
	Size size = GetTextBounds(font, strFormat, strText);

	if(size.Width > m_nWidth - m_nLeft - 4)
	{
		m_nWidth = size.Width + m_nLeft + 4;
	}

	pControlBase = new CMenuItem(GetSafeHwnd(),this, uMenuID, CRect(0, 0, 0, 0), strText, m_nLeft, bSelect);
	((CControlBaseFont *)pControlBase)->SetFont(m_strFont, m_nFontWidth, m_fontStyle);
	if(!strImage.IsEmpty())
	{
		((CMenuItem *)pControlBase)->SetBitmap(strImage);
	}

	if(nIndex >= 0 && nIndex < m_vecControl.size())
	{
		m_vecControl.insert(m_vecControl.begin() + nIndex, pControlBase);
	}
	else
	{
		m_vecControl.push_back(pControlBase);
	}

	SetMenuPoint();
	return m_vecControl.size();
}

// 添加菜单分隔
int CMenuEx::AddSeparator(int nIndex)
{
	// 可以使用矩形控件，也可以使用图片控件
	CControlBase * pControlBase = new CRectangle(GetSafeHwnd(),this, -1, CRect(0, 0, 0, 0), Color(254, 227, 229, 230));

	if(nIndex >= 0 && nIndex < m_vecControl.size())
	{
		m_vecControl.insert(m_vecControl.begin() + nIndex, pControlBase);
	}
	else
	{
		m_vecControl.push_back(pControlBase);
	}

	SetMenuPoint();
	return m_vecControl.size();
}

// 设置菜单项位置
void CMenuEx::SetMenuPoint()
{
	int nXPos = 2;
	int nYPos = 2;
	CRect rc;
	for (size_t i = 0; i < m_vecControl.size(); i++)
	{
		CControlBase * pControlBase = m_vecControl[i];
		if(pControlBase->IsClass(CMenuItem::GetClassName()))	// 如果是MenuItem类型控件
		{
			CMenuItem* pMenuItem = (CMenuItem*)pControlBase;
			if(!pMenuItem->GetVisible())
			{
				// 菜单项不可见
				rc.SetRect(0,0,0,0);
			}else
			if(pMenuItem->IsSeparator())
			{
				// 分隔线
				rc.SetRect(m_nLeft + 1, nYPos + 1, m_nWidth - 1, nYPos + 2);
				nYPos += 4;
			}else
			{
				// 普通菜单项
				rc.SetRect(nXPos, nYPos, m_nWidth - 2, nYPos + m_nHeight);
				nYPos += m_nHeight;
			}
		}else
		if(-1 == pControlBase->GetControlID())
		{
			rc.SetRect(m_nLeft + 4, nYPos + 1, m_nWidth - 9, nYPos + 2);
			nYPos += 4;
		}/*
		else
		{
			rc.SetRect(nXPos, nYPos, m_nWidth - 2, nYPos + m_nHeight);
			nYPos += m_nHeight;
		}
		*/
		SetControlRect(pControlBase, rc);
	}
	nYPos += 2;
	SetWindowPos(NULL, 0, 0, m_nWidth, nYPos, SWP_NOMOVE);
	InvalidateRect(NULL);
}

// 获取父菜单对象
CMenuEx* CMenuEx::GetParentMenu()
{
	CDuiObject* pParentObj = GetParent();
	while((pParentObj != NULL) && (!pParentObj->IsClass("menu")))
	{
		if(pParentObj->IsClass("popup"))
		{
			pParentObj = ((CDlgPopup*)pParentObj)->GetParent();
		}else
		if(pParentObj->IsClass("dlg"))
		{
			pParentObj = ((CDlgBase*)pParentObj)->GetParent();
		}else
		{
			pParentObj = ((CControlBase*)pParentObj)->GetParent();
		}
	}
	if((pParentObj != NULL) && pParentObj->IsClass("menu"))
	{
		return (CMenuEx*)pParentObj;
	}

	return NULL;
}

// 获取当前处于活动状态的子菜单项
CMenuItem* CMenuEx::GetHoverMenuItem()
{
	for (size_t i = 0; i < m_vecControl.size(); i++)
	{
		CControlBase * pControlBase = m_vecControl[i];
		if(pControlBase->IsClass(CMenuItem::GetClassName()))	// 如果是MenuItem类型控件
		{
			CMenuItem* pMenuItem = (CMenuItem*)pControlBase;
			if(pMenuItem->IsHover())
			{
				return pMenuItem;
			}
		}
	}

	return NULL;
}

// 消息响应
LRESULT CMenuEx::OnMessage(UINT uID, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if((Msg != BUTTOM_UP) && (Msg != BUTTOM_CHECK))
	{
		return 0;
	}

	CControlBase* pControl = GetControl(uID);
	if(pControl && !pControl->GetAction().IsEmpty())
	{
		// 如果菜单项设置了action，则添加到动作任务队列中，通过任务来执行
		CString strControlName = pControl->GetName();
		CString strAction = pControl->GetAction();
		//CDuiObject* pParent = pControl->GetParent();
		CDlgBase* pParentDlg = pControl->GetParentDialog();

		DuiSystem::Instance()->AddDuiActionTask(uID, Msg, wParam, lParam, strControlName, strAction, pParentDlg);
	}else
	{
		// 否则就调用Popup的函数
		__super::OnMessage(uID, Msg, wParam, lParam);

		/*tagMenuInfo* pMenuInfo = new tagMenuInfo;
		pMenuInfo->uMenuID = uID;
		pMenuInfo->bSelect = (bool)lParam;
		pMenuInfo->bDown = (bool)wParam;
	
		PostMessage(m_uMessageID, Msg, (LPARAM)pMenuInfo);*/
	}

	if(Msg == BUTTOM_UP)
	{
		// 如果有父菜单,将父菜单关闭,不采用直接关闭的方法,而是设置自动关闭标识,并通过鼠标事件触发自动关闭
		CMenuEx* pParentMenu = GetParentMenu();
		if(pParentMenu && !pParentMenu->IsAutoClose())
		{
			pParentMenu->SetAutoClose(TRUE);
			pParentMenu->SetForegroundWindow();
			pParentMenu->PostMessage(WM_LBUTTONDOWN, 0, 0);
		}
		// 关闭自身
		CloseWindow();
	}

	return 0;
}

// 重载窗口去激活时候的关闭窗口操作
BOOL CMenuEx::OnNcCloseWindow()
{
	// 如果有父菜单,将父菜单窗口关闭
	CMenuEx* pParentMenu = GetParentMenu();
	if(pParentMenu && !pParentMenu->IsAutoClose())
	{
		// 如果鼠标在父菜单窗口中,则不关闭父窗口
		CMenuItem* pHoverItem = pParentMenu->GetHoverMenuItem();
		if(pHoverItem == NULL)
		{
			pParentMenu->SetAutoClose(TRUE);
			pParentMenu->SetForegroundWindow();
			pParentMenu->PostMessage(WM_LBUTTONDOWN, 0, 0);
		}
	}

	// 如果父对象是菜单项,则将菜单项中的弹出菜单指针设置为空
	CDuiObject* pParent = GetParent();
	if((pParent != NULL) && (pParent->IsClass(CMenuItem::GetClassName())))
	{
		((CMenuItem*)pParent)->SetPopupMenu(NULL);
	}

	// 关闭自身
	m_pParentDuiObject = NULL;
	CloseWindow();	

	return TRUE;
}