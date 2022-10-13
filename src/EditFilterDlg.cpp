﻿/*____________________________________________________________________________

   ExifPro Image Viewer

   Copyright (C) 2000-2015 Michael Kowalski
____________________________________________________________________________*/

// EditFilterDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "EditFilterDlg.h"
#include "CatchAll.h"
#include "ExprCalculator.h"
#include "PhotoTags.h"
#include "Attributes.h"
#include "BalloonMsg.h"
#include "PhotoTagsCollection.h"
#include <boost/algorithm/string/trim.hpp>
#include "FilterRules.h"

using namespace boost::algorithm;

// EditFilterDlg dialog
String filter_rule_head =  _T("x = 1 if x > 0\r\n");
String filter_rule_end =  _T("then return true\r\nend");

EditFilterDlg::EditFilterDlg(CWnd* parent, const PhotoInfo& example, const PhotoTagsCollection& collection)
	: DialogChild(EditFilterDlg::IDD, parent), example_(example), collection_(collection)
{
	//filter_rule_ = _T("--and img.fl == 50\n--and img.iso > 100\n--and img.fn < 8.0\n");
}

EditFilterDlg::~EditFilterDlg()
{
}

void EditFilterDlg::DoDataExchange(CDataExchange* DX)
{
	DialogChild::DoDataExchange(DX);
	DDX_Control(DX, IDC_EXPR, expression_);
	DDX_Control(DX, IDC_LABEL_2, info_text_);
	DDX_Control(DX, IDC_TOOLBAR, insert_);
}


BEGIN_MESSAGE_MAP(EditFilterDlg, DialogChild)
	ON_BN_CLICKED(IDC_VERIFY, &EditFilterDlg::OnVerifyExpr)
	ON_NOTIFY(TBN_DROPDOWN, IDC_TOOLBAR, OnTbDropDown)
	ON_EN_SETFOCUS(IDC_EXPR, OnExprSetFocus)
END_MESSAGE_MAP()


void EditFilterDlg::OnExprSetFocus()
{
	if (expression_.m_hWnd)
	{
		int len= expression_.GetWindowTextLength();
		expression_.SetSel(-1, 0, true);
	}
}


const int INSERT_ATTRIB= 10;
const int INSERT_TAG= 11;
const int INSERT_XMP= 12;
const int INSERT_FILE= 13;

BOOL EditFilterDlg::OnInitDialog()
{
	try
	{
		DialogChild::OnInitDialog();

		BuildResizingMap();

		SetWndResizing(IDOK, DlgAutoResize::MOVE);
		SetWndResizing(IDCANCEL, DlgAutoResize::MOVE);
		SetWndResizing(IDC_EXPR, DlgAutoResize::RESIZE);
		SetWndResizing(IDC_LABEL_1, DlgAutoResize::MOVE_V);
		SetWndResizing(IDC_LABEL_2, DlgAutoResize::MOVE_V_RESIZE_H);
		SetWndResizing(IDC_LABEL_3, DlgAutoResize::MOVE_V);
		SetWndResizing(IDC_STATUS, DlgAutoResize::MOVE_V_RESIZE_H);
		SetWndResizing(IDC_VERIFY, DlgAutoResize::MOVE);
		//SetWndResizing(IDC_HELP_BTN, DlgAutoResize::MOVE_V);

		//TODO
//		SubclassHelpBtn(_T("EditFilterRule.htm"));
//if (CWnd* h= GetDlgItem(IDC_HELP_BTN))
//	h->ShowWindow(SW_HIDE);

		int cmds[]= { INSERT_ATTRIB, INSERT_TAG, INSERT_XMP, INSERT_FILE };
		insert_.SetPadding(8, 12);
		insert_.AddButtons("VVVV", cmds, 0, IDS_INSERT_TEXT);
		insert_.SetOnIdleUpdateState(false);
		insert_.EnableButton(INSERT_TAG, !collection_.Empty());

		const int tab_step= 45;
		info_text_.SetTabStops(tab_step);
		info_text_.SetWindowText(GetImgAttribsHelpString(IAH_BASIC_INFO | IAH_TAG_INFO).c_str());

		//CreateScriptEditingFont(expr_font_);

		const int MAX_EXPR= 65000;
		//expression_.LimitText(MAX_EXPR);
		//expression_.SetFont(&expr_font_);
		expression_.SetWindowText(filter_rule_.c_str());

		return false;  // return TRUE unless you set the focus to a control
	}
	CATCH_ALL

	EndDialog(IDCANCEL);
	return true;
}


bool EditFilterDlg::ReadExpr(String& filter_rule)
{
	try
	{
		CString expr;
		expression_.GetWindowText(expr);

		String err_msg;
		if (!VerifyExpr(expr, &err_msg))
		{
			SetDlgItemText(IDC_STATUS, (_T("Error: ") + err_msg).c_str());
			new BalloonMsg(expression_.GetWnd(), _T("Invalid Expression"), err_msg.c_str(), BalloonMsg::IERROR);
			return false;
		}

		filter_rule = expr;

		return true;
	}
	CATCH_ALL

	return false;
}


void EditFilterDlg::OnOK()
{
	if (ReadExpr(filter_rule_)){
		AdvFilter::SaveFilters(AdvFilter::GetFiltersPathName().c_str(), filter_rule_);
		EndDialog(IDOK);
	}
}


void EditFilterDlg::OnCancel()
{
	EndDialog(IDCANCEL);
}


void EditFilterDlg::OnVerifyExpr()
{
	try
	{
		CString expr;
		expression_.GetWindowText(expr);

		String err_msg;
		if (VerifyExpr(expr, &err_msg))
			SetDlgItemText(IDC_STATUS, (_T("Result: ") + err_msg).c_str());
		else
			SetDlgItemText(IDC_STATUS, (_T("Error: ") + err_msg).c_str());
	}
	CATCH_ALL
}


bool EditFilterDlg::VerifyExpr(const CString& expr, String* err_message)
{
	if (expr.IsEmpty())
	{
		if (err_message)
			*err_message = _T("Please enter valid expression.");

		return false;
	}
	
	CString expr_ = expr;
	iStringstream si(expr_.GetBuffer(0));
	String expr_i;
	String expr_all;
	for (;;)
	{
		if (!getline(si, expr_i, _T('\n')))
			break;
		trim_if(expr_i, is_from_range(_T('\0'), _T('\x20')));
		if(expr_i.length() != 0){
			expr_all += _T("and ");
			expr_all += expr_i;
			expr_all += _T("\n");
		}
	}
	
	expr_all = filter_rule_head + expr_all + filter_rule_end;

	ExprCalculator calc(expr_all.c_str());
	String err_msg;

	bool valid= calc.IsValid(err_msg, example_);

	if (err_message)
		*err_message = err_msg;

	return valid;
}


void EditFilterDlg::OnTbDropDown(NMHDR* nmhdr, LRESULT* result)
{
	NMTOOLBAR* info= reinterpret_cast<NMTOOLBAR*>(nmhdr);
	*result = TBDDRET_DEFAULT;

	CPoint pos(info->rcButton.left, info->rcButton.bottom);
//	CRect rect;
//	toolbar_wnd_.GetRect(ID_VIEW_MODE, rect);
//	CPoint pos(rect.left, rect.bottom);
	insert_.ClientToScreen(&pos);

	switch (info->iItem)
	{
	case INSERT_ATTRIB:
		OnViewAttribs(pos);
		break;

	case INSERT_TAG:
		OnViewTags(pos);
		break;

	case INSERT_XMP:
		if (const TCHAR* text= ImgMetaAttribsPopup(this, pos))
			InsertText(text);
		break;

	case INSERT_FILE:
		if (const TCHAR* text= ImgFileAttribsPopup(this, pos))
			expression_.ReplaceSel(text, true);
		break;
	}
}


void EditFilterDlg::OnViewTags(CPoint pos)
{
	CMenu menu;
	if (!menu.CreatePopupMenu())
		return;

	const size_t count= collection_.GetCount();
	for (size_t i= 0; i < count; ++i)
		menu.InsertMenu(static_cast<UINT>(i), MF_STRING, i + 1, collection_.Get(i).c_str());

	int cmd= menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, pos.x, pos.y, this);

	if (cmd == 0)
		return;

	cmd--;

	if (cmd >= 0 && cmd < count)
	{
		String tag= _T("img.tags[\"") + collection_.Get(cmd) + _T("\"]");
		InsertText(tag.c_str());
	}
	else
	{ ASSERT(false); }
}


void EditFilterDlg::OnViewAttribs(CPoint pos)
{
	if (const TCHAR* text= ImgAttribsPopup(this, pos))
		InsertText(text);
}


void EditFilterDlg::InsertText(const TCHAR* text)
{
	expression_.ReplaceSel(text, true);
}
