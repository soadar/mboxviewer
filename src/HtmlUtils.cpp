//
//////////////////////////////////////////////////////////////////
//
//  Windows Mbox Viewer is a free tool to view, search and print mbox mail archives..
//
// Source code and executable can be downloaded from
//  https://sourceforge.net/projects/mbox-viewer/  and
//  https://github.com/eneam/mboxviewer
//
//  Copyright(C) 2019  Enea Mansutti, Zbigniew Minciel
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the version 3 of GNU Affero General Public License
//  as published by the Free Software Foundation; 
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU
//  Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public
//  License along with this program; if not, write to the
//  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
//  Boston, MA  02110 - 1301, USA.
//
//////////////////////////////////////////////////////////////////
//


#include "StdAfx.h"
#include "SimpleString.h"
#include "TextUtilsEx.h"
#include "Browser.h"
#include "HtmlUtils.h"


#pragma component(browser, off, references)  // wliminates too many refrences warning but at what price ?
#include <Mshtml.h>
#pragma component(browser, on, references)
#include <atlbase.h>

#ifdef _DEBUG
#define HTML_ASSERT ASSERT
#else
#define HTML_ASSERT(x)
#endif

char *EatNLine(char* p, char* e) { while (p < e && *p++ != '\n'); return p; }

char *SkipEmptyLine(char* p, char* e)
{
	char *p_save = p;
	while ((p < e) && ((*p == ' ') || (*p == '\t')))  // eat empty lines
		p++;
	if ((*p == '\r') && (*p == '\n'))
		p += 2;
	else if ((*p == '\r') || (*p == '\n'))
		p++;
	else
		p = p_save;
	return p;
}

void BreakBeforeGoingCleanup()
{
	int deb = 1;
}

void HtmlUtils::MergeWhiteLines(SimpleString *workbuf, int maxOutLines)
{
	if (maxOutLines == 0) {
		workbuf->SetCount(0);
		return;
	}

	// Delete duplicate empty lines
	char *p = workbuf->Data();
	char *e = p + workbuf->Count();

	char *p_end_data;
	char *p_beg_data;
	char *p_save;
	char *p_data;

	unsigned long  len;
	int dataCount = 0;
	int emptyLineCount = 0;
	int outLineCnt = 0;

	while (p < e)
	{
		emptyLineCount = 0;
		p_save = 0;
		while ((p < e) && (p != p_save))
		{
			p_save = p;
			p = SkipEmptyLine(p, e);
			if (p != p_save)
				emptyLineCount++;

		}
		p_beg_data = p;

		while ((p < e) && !((*p == '\r') || (*p == '\n')))
		{
			p = EatNLine(p, e);
			outLineCnt++;
			if ((maxOutLines > 0) && (outLineCnt >= maxOutLines))
			{
				e = p;
				break;
			}
		}

		p_end_data = p;

		if (emptyLineCount > 0)
		{
			p_data = workbuf->Data() + dataCount;
			if ((p_beg_data - p_data) > 1) {
				memcpy(p_data, "\r\n", 2);
				dataCount += 2;
			}
			else
			{
				memcpy(p_data, "\n", 1);
				dataCount += 1;
			}
			outLineCnt++;
		}

		len = p_end_data - p_beg_data;
		//g_tu.hexdump("Data:\n", p_beg_data, len);

		if (len > 0) {
			p_data = workbuf->Data() + dataCount;
			memcpy(p_data, p_beg_data, len);
			dataCount += len;
		}
		if ((maxOutLines > 0) && (outLineCnt >= maxOutLines))
			break;
	}

	workbuf->SetCount(dataCount);

#if 0
	// Verify that outLineCnt is valid
	{
		if (maxOutLines <= 0)
			return;


		char *p = workbuf->Data();
		e = p + workbuf->Count();
		int lineCnt = 0;
		while (p < e)
		{
			p = EatNLine(p, e);
			lineCnt++;
		}
		if (lineCnt != outLineCnt)
			int deb = 1;
	}
#endif
}

void HtmlUtils::PrintToPrinterPageSetup(CWnd *parent)
{
	//CComBSTR cmdID(_T("PRINT"));
	//VARIANT_BOOL vBool;

	IHTMLDocument2 *lpHtmlDocument = 0;
	HRESULT hr;
	VARIANT val;
	VariantInit(&val);
	VARIANT valOut;
	VariantInit(&valOut);

	SimpleString inbuf;
	inbuf.Append("<html></html>");
	SimpleString workbuf;
	UINT inCodePage = CP_UTF8;

	BOOL retVal = CreateHTMLDocument(&lpHtmlDocument, &inbuf, &workbuf, inCodePage);
	if ((retVal == FALSE) || (lpHtmlDocument == 0)) {
		return;
	}

	IOleCommandTarget  *lpOleCommandTarget = 0;
	hr = lpHtmlDocument->QueryInterface(IID_IOleCommandTarget, (VOID**)&lpOleCommandTarget);
	if (FAILED(hr) || !lpOleCommandTarget)
	{
		BreakBeforeGoingCleanup();
		goto cleanup;
	}

	hr = lpOleCommandTarget->Exec(NULL, OLECMDID_PAGESETUP, OLECMDEXECOPT_PROMPTUSER, &val, &valOut);

	if (FAILED(hr))
	{
		BreakBeforeGoingCleanup();
		goto cleanup;
	}

cleanup:

	hr = VariantClear(&val);
	hr = VariantClear(&valOut);

	if (lpOleCommandTarget)
		lpOleCommandTarget->Release();

	if (lpHtmlDocument)
		lpHtmlDocument->Release();

}

void HtmlUtils::PrintHTMLDocumentToPrinter(SimpleString *inbuf, SimpleString *workbuf, UINT inCodePage, int printDialogType)
{
	//CComBSTR cmdID(_T("PRINT"));
	//VARIANT_BOOL vBool;

	IHTMLDocument2 *lpHtmlDocument = 0;
	HRESULT hr;
	VARIANT val;
	VariantInit(&val);
	VARIANT valOut;
	VariantInit(&valOut);

	BOOL retVal = CreateHTMLDocument(&lpHtmlDocument, inbuf, workbuf, inCodePage);
	if ((retVal == FALSE) || (lpHtmlDocument == 0)) {
		return;
	}

#if 0
	CHtmlEditCtrl PrintCtrl;

	if (!PrintCtrl.Create(NULL, WS_CHILD, CRect(0, 0, 0, 0), this, 1))
	{
		ASSERT(FALSE);
		return; // Error!
	}

	//CPrintDialog dlgl(FALSE);  INT_PTR userResult = dlgl.DoModal();
#endif

	//lpHtmlDocument->execCommand(cmdID, VARIANT_TRUE, val, &vBool);

	IOleCommandTarget  *lpOleCommandTarget = 0;
	hr = lpHtmlDocument->QueryInterface(IID_IOleCommandTarget, (VOID**)&lpOleCommandTarget);
	if (FAILED(hr) || !lpOleCommandTarget)
	{
		BreakBeforeGoingCleanup();
		goto cleanup;
	}

	DWORD nCmdId = OLECMDID_PRINT;
	DWORD nCmdOption = OLECMDEXECOPT_DONTPROMPTUSER;

	if (printDialogType == 1) {
		nCmdId = OLECMDID_PRINT;
		nCmdOption = OLECMDEXECOPT_PROMPTUSER;
	}
	else if (printDialogType == 2) {
		nCmdId = OLECMDID_PRINTPREVIEW;
		nCmdOption = OLECMDEXECOPT_PROMPTUSER;
	}

	hr = lpOleCommandTarget->Exec(NULL, nCmdId, nCmdOption, &val, &valOut);
	if (FAILED(hr))
	{
		BreakBeforeGoingCleanup();
		goto cleanup;
	}

cleanup:

	hr = VariantClear(&val);
	hr = VariantClear(&valOut);

	if (lpOleCommandTarget)
		lpOleCommandTarget->Release();

	if (lpHtmlDocument)
		lpHtmlDocument->Release();
}

void HtmlUtils::GetTextFromIHTMLDocument(SimpleString *inbuf, SimpleString *workbuf, UINT inCodePage, UINT outCodePage)
{
	IHTMLDocument2 *lpHtmlDocument = 0;
	LPDISPATCH lpDispatch = 0;
	HRESULT hr;

	if (lpHtmlDocument == 0)
	{
		BOOL retVal = CreateHTMLDocument(&lpHtmlDocument, inbuf, workbuf, inCodePage);
		if ((retVal == FALSE) || (lpHtmlDocument == 0)) {
			return;
		}
	}

	IHTMLElement *lpBodyElm = 0;
	hr = lpHtmlDocument->get_body(&lpBodyElm);
	//HTML_ASSERT(SUCCEEDED(hr) && lpBodyElm);
	if (!lpBodyElm) {
		lpHtmlDocument->Release();
		return;
	}

	// Need to remove STYLE tag otherwise it will be part of text from lpBodyElm->get_innerText(&bstrTxt);
	// TODO: IHTMLDocument2:get_text() is not the greatest, adding or missing text, and slow.
	// Don't think any work is done on MFC and c++ IHTMLDocument.
	// Implemented incomplete PrintIHTMLDocument based on IHTMLDocument framework but is incomplete. It still be slow.
	//
	RemoveStyleTagFromIHTMLDocument(lpBodyElm);

	CComBSTR bstrTxt;
	hr = lpBodyElm->get_innerText(&bstrTxt);
	HTML_ASSERT(SUCCEEDED(hr));
	if (FAILED(hr)) {
		lpBodyElm->Release();
		return;
	}

	int wlen = bstrTxt.Length();
	wchar_t *wstr = bstrTxt;

	//SimpleString *workbuf = MboxMail::m_workbuf;
	workbuf->ClearAndResize(10000);

	DWORD error;
	BOOL ret = TextUtilsEx::WStr2CodePage(wstr, wlen, outCodePage, workbuf, error);

	MergeWhiteLines(workbuf, -1);

	if (lpBodyElm)
		lpBodyElm->Release();

	if (lpHtmlDocument)
		lpHtmlDocument->Release();
}

BOOL HtmlUtils::CreateHTMLDocument(IHTMLDocument2 **lpDocument, SimpleString *inbuf, SimpleString *workbuf, UINT inCodePage)
{
	HRESULT hr;
	BOOL ret = TRUE;
	*lpDocument = 0;

	IHTMLDocument2 *lpDoc = 0;
	SAFEARRAY *psaStrings = 0;

	//CComBSTR bstr(inbuf->Data());
	//int bstrLen = bstr.Length();
	//int deb1 = 1;
	//BSTR  bstrTest = SysAllocString(L"TEST");  // Test heap allocations
	//SysFreeString(bstrTest);
	//int deb = 1;

#if 0
	USES_CONVERSION;
	BSTR  bstr = SysAllocString(A2W(inbuf->Data())); // efficient but relies on stack and not heap; but what is Data() is > stack ?
	int bstrLen = SysStringByteLen(bstr);
	int wlen = wcslen(bstr);
#else
	DWORD error;
	ret = TextUtilsEx::CodePage2WStr(inbuf, inCodePage, workbuf, error);
	OLECHAR *oledata = (OLECHAR*)workbuf->Data();
	BSTR  bstr = SysAllocString(oledata);
	int bstrLen = SysStringByteLen(bstr);
	int wlen = wcslen(bstr);
#endif

	hr = CoCreateInstance(CLSID_HTMLDocument, NULL, CLSCTX_INPROC_SERVER,
		IID_IHTMLDocument2, (void**)&lpDoc);
	HTML_ASSERT(SUCCEEDED(hr) && lpDoc);
	if (FAILED(hr) || !lpDoc) {
		BreakBeforeGoingCleanup();
		ret = FALSE;
		goto cleanup;
	}

	// Creates a new one-dimensional array
	psaStrings = SafeArrayCreateVector(VT_VARIANT, 0, 1);
	HTML_ASSERT(psaStrings);
	if (psaStrings == 0) {
		BreakBeforeGoingCleanup();
		ret = FALSE;
		goto cleanup;
	}
	VARIANT *param = 0;
	hr = SafeArrayAccessData(psaStrings, (LPVOID*)&param);
	HTML_ASSERT(SUCCEEDED(hr) && param);
	if (FAILED(hr) || (param == 0)) {
		BreakBeforeGoingCleanup();
		ret = FALSE;
		goto cleanup;
	}
	param->vt = VT_BSTR;
	param->bstrVal = bstr;

	// put_designMode(L"on");  should disable all activity such as running scripts
	// May need better solution
	hr = lpDoc->put_designMode(L"on");
	hr = lpDoc->writeln(psaStrings);

	HTML_ASSERT(SUCCEEDED(hr));
	if (FAILED(hr)) {
		BreakBeforeGoingCleanup();
		ret = FALSE;
		goto cleanup;
	}

cleanup:
	// Assume SafeArrayDestroy calls SysFreeString for each BSTR :)
	if (psaStrings != 0) {
		hr = SafeArrayUnaccessData(psaStrings);
		hr = SafeArrayDestroy(psaStrings);
	}
	if (ret == FALSE)
	{
		if (lpDoc) {
			hr = lpDoc->close();
			if (FAILED(hr)) {
				int deb = 1;
			}
			lpDoc->Release();
			lpDoc = 0;
		}
	}
	else
	{
		hr = lpDoc->close();
		if (FAILED(hr))
		{
			int deb = 1;
		}
		*lpDocument = lpDoc;
	}
	return ret;
}

void HtmlUtils::RemoveStyleTagFromIHTMLDocument(IHTMLElement *lpElm)
{
	CComBSTR emptyText("");
	CComBSTR bstrTag;
	//CComBSTR bstrHTML;
	//CComBSTR bstrTEXT;
	VARIANT index;
	HRESULT hr;

	IDispatch *lpCh = 0;
	hr = lpElm->get_children(&lpCh);
	if (FAILED(hr) || !lpCh)
	{
		return;
	}

	IHTMLElementCollection *lpChColl = 0;
	hr = lpCh->QueryInterface(IID_IHTMLElementCollection, (VOID**)&lpChColl);
	if (FAILED(hr) || !lpChColl)
	{
		BreakBeforeGoingCleanup();
		goto cleanup;
	}

	long items = 0;
	IDispatch *ppvDisp = 0;
	IHTMLElement *ppvElement = 0;

	lpChColl->get_length(&items);

	for (long j = 0; j < items; j++)
	{
		index.vt = VT_I4;
		index.lVal = j;
		ppvDisp = 0;
		hr = lpChColl->item(index, index, &ppvDisp);
		if (FAILED(hr) || !ppvDisp) {
			BreakBeforeGoingCleanup();
			goto cleanup;
		}
		if (ppvDisp)
		{
			ppvElement = 0;
			hr = ppvDisp->QueryInterface(IID_IHTMLElement, (void **)&ppvElement);
			if (FAILED(hr) || !ppvElement) {
				BreakBeforeGoingCleanup();
				goto cleanup;
			}
			if (ppvElement)
			{
				bstrTag.Empty();
				//bstrHTML.Empty();
				//bstrTEXT.Empty();

				ppvElement->get_tagName(&bstrTag);
				//ppvElement->get_outerHTML(&bstrHTML);
				//ppvElement->get_innerText(&bstrTEXT);

				CString tag(bstrTag);
				if (tag.CollateNoCase("style") == 0) {
					ppvElement->put_innerText(emptyText);
					int deb = 1;
				}

				RemoveStyleTagFromIHTMLDocument(ppvElement);

				ppvElement->Release();
				ppvElement = 0;

				int deb = 1;
			}
			ppvDisp->Release();
			ppvDisp = 0;
		}
		int deb = 1;
	}
cleanup:

	if (ppvElement)
		ppvElement->Release();

	if (ppvDisp)
		ppvDisp->Release();

	if (lpChColl)
		lpChColl->Release();

	if (lpCh)
		lpCh->Release();
}


BOOL HtmlUtils::FindElementByTagInIHTMLDocument(IHTMLDocument2 *lpDocument, IHTMLElement **ppvEl, CString &tag)
{
	// browse all elements and return first element found
	IHTMLElementCollection *pAll = 0;
	CComBSTR bstrTag;
	CComBSTR bstrHTML;
	CComBSTR bstrTEXT;
	VARIANT index;

	*ppvEl = 0;
	HRESULT hr = lpDocument->get_all(&pAll);
	if (FAILED(hr) || !pAll) {
		BreakBeforeGoingCleanup();
		goto cleanup;
	}

	long items = 0;
	IDispatch *ppvDisp = 0;
	IHTMLElement *ppvElement = 0;

	pAll->get_length(&items);

	for (long j = 0; j < items; j++)
	{
		index.vt = VT_I4;
		index.lVal = j;
		ppvDisp = 0;
		hr = pAll->item(index, index, &ppvDisp);
		if (FAILED(hr) || !ppvDisp) {
			BreakBeforeGoingCleanup();
			goto cleanup;
		}
		if (ppvDisp)
		{
			ppvElement = 0;
			hr = ppvDisp->QueryInterface(IID_IHTMLElement, (void **)&ppvElement);
			if (FAILED(hr) || !ppvElement) {
				BreakBeforeGoingCleanup();
				goto cleanup;
			}
			if (ppvElement)
			{
				bstrTag.Empty();
				//bstrHTML.Empty();
				//bstrTEXT.Empty();

				ppvElement->get_tagName(&bstrTag);
				//ppvElement->get_innerHTML(&bstrHTML);
				//ppvElement->get_innerText(&bstrTEXT);

				CString eltag(bstrTag);
				TRACE("TAG=%s\n", (LPCSTR)eltag);

				if (eltag.CompareNoCase(tag))
				{
					ppvElement->Release();
					ppvElement = 0;
				}
				else
					break;

				int deb = 1;
			}
			ppvDisp->Release();
			ppvDisp = 0;
		}
		int deb = 1;
	}

cleanup:
	if (ppvElement)
		*ppvEl = ppvElement;

	if (ppvDisp)
		ppvDisp->Release();

	if (pAll)
		pAll->Release();

	return FALSE;
}

void HtmlUtils::PrintIHTMLDocument(IHTMLDocument2 *lpDocument, SimpleString *workbuf)
{
	// Actually this will print BODY
	CString tag("body");
	IHTMLElement *lpElm = 0;
	FindElementByTagInIHTMLDocument(lpDocument, &lpElm, tag);
	if (!lpElm)
	{
		return;
	}

	CComBSTR bstrTEXT;
	lpElm->get_innerText(&bstrTEXT);

	CStringW text;
	PrintIHTMLElement(lpElm, text);

	workbuf->ClearAndResize(10000);

	const wchar_t *wstr = text.operator LPCWSTR();
	int wlen = text.GetLength();
	UINT outCodePage = CP_UTF8;
	DWORD error;
	BOOL ret = TextUtilsEx::WStr2CodePage((wchar_t *)wstr, wlen, outCodePage, workbuf, error);

	TRACE("TEXT=%s\n", workbuf->Data());
}

void HtmlUtils::PrintIHTMLElement(IHTMLElement *lpElm, CStringW &text)
{
	CComBSTR bstrTag;
	CComBSTR bstrHTML;
	CComBSTR bstrTEXT;
	VARIANT index;
	HRESULT hr;

	IDispatch *lpCh = 0;
	hr = lpElm->get_children(&lpCh);
	if (FAILED(hr) || !lpCh)
	{
		return;
	}

	IHTMLElementCollection *lpChColl = 0;
	hr = lpCh->QueryInterface(IID_IHTMLElementCollection, (VOID**)&lpChColl);
	if (FAILED(hr) || !lpChColl)
	{
		BreakBeforeGoingCleanup();
		goto cleanup;;
	}

	long items;
	IDispatch *ppvDisp = 0;
	IHTMLElement *ppvElement = 0;

	lpChColl->get_length(&items);

	// TODO: This doesn't quite work, need to find solution.
	if (items <= 0)
	{
		lpElm->get_tagName(&bstrTag);
		lpElm->get_outerHTML(&bstrHTML);
		lpElm->get_innerText(&bstrTEXT);

		text.Append(bstrTEXT);

		CString tag(bstrTag);
		//text.Append("\r\n\r\n");
		TRACE("TAG=%s\n", (LPCSTR)tag);
		//TRACE("TEXT=%s\n", (LPCSTR)innerText);
	}
	else
		text.Append(L"\r\n\r\n");

	for (long j = 0; j < items; j++)
	{
		index.vt = VT_I4;
		index.lVal = j;
		ppvDisp = 0;
		hr = lpChColl->item(index, index, &ppvDisp);
		if (FAILED(hr) || !ppvDisp) {
			BreakBeforeGoingCleanup();
			goto cleanup;
		}
		if (ppvDisp)
		{
			hr = ppvDisp->QueryInterface(IID_IHTMLElement, (void **)&ppvElement);
			if (FAILED(hr) || !ppvElement) {
				BreakBeforeGoingCleanup();
				goto cleanup;
			}
			if (ppvElement)
			{
				bstrTag.Empty();
				bstrHTML.Empty();
				bstrTEXT.Empty();

				ppvElement->get_tagName(&bstrTag);
				ppvElement->get_outerHTML(&bstrHTML);
				ppvElement->get_innerText(&bstrTEXT);


				CString tag(bstrTag);
				TRACE("TAG=%s\n", (LPCSTR)tag);

				if (tag.CollateNoCase("style") == 0)
					int deb = 1;

				if (tag.CollateNoCase("tbody") == 0)
					int deb = 1;

				if (tag.CollateNoCase("td") == 0)
					int deb = 1;

				if (tag.CollateNoCase("body") == 0)
					int deb = 1;

				if (tag.CollateNoCase("style") == 0) {
					CComBSTR emptyText("");
					ppvElement->put_innerText(emptyText);
					int deb = 1;
				}


				PrintIHTMLElement(ppvElement, text);


				ppvElement->Release();
				ppvElement = 0;

				int deb = 1;
			}
			ppvDisp->Release();
			ppvDisp = 0;
		}
		int deb = 1;
	}
cleanup:

	if (ppvElement)
		ppvElement->Release();

	if (ppvDisp)
		ppvDisp->Release();

	if (lpChColl)
		lpChColl->Release();

	if (lpCh)
		lpCh->Release();
}

void HtmlUtils::FindStringInIHTMLDocument(CBrowser &browser, CString searchID, CString &searchText, BOOL matchWord, BOOL matchCase, CString &matchStyle)
{
	// Based on "Adding a custom search feature to CHtmlViews" on the codeproject by  Marc Richarme, 22 Nov 2000
	// without explicit license
	//  https://www.codeproject.com/Articles/832/Adding-a-custom-search-feature-to-CHtmlViews
	// // mboxview development team did resolve some code issues, enhanced and optimized

	// <span id='mboxview_Search' style='color: white; background-color: darkblue'>pa</span>

	HRESULT hr;
	CComBSTR bstrTag;
	CComBSTR bstrHTML;
	CComBSTR bstrTEXT;
	CString htmlPrfix;

	unsigned long matchWordFlag = 2;
	unsigned long matchCaseFlag = 4;
	long  lFlags = 0; // lFlags : 0: match in reverse: 1 , Match partial Words 2=wholeword, 4 = matchcase
	if (matchWord)
		lFlags |= matchWordFlag;
	if (matchCase)
		lFlags |= matchCaseFlag;

	ClearSearchResultsInIHTMLDocument(browser, searchID);

	IHTMLDocument2 *lpHtmlDocument = NULL;
	LPDISPATCH lpDispatch = NULL;
	lpDispatch = browser.m_ie.GetDocument();
	HTML_ASSERT(lpDispatch);
	if (!lpDispatch)
		return;

	hr = lpDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&lpHtmlDocument);
	HTML_ASSERT(SUCCEEDED(hr) && lpHtmlDocument);
	if (!lpHtmlDocument) {
		lpDispatch->Release();
		return;
	}
	lpDispatch->Release();

	IHTMLElement *lpBodyElm = 0;
	IHTMLBodyElement *lpBody = 0;
	IHTMLTxtRange *lpTxtRange = 0;

	hr = lpHtmlDocument->get_body(&lpBodyElm);
	HTML_ASSERT(SUCCEEDED(hr) && lpBodyElm);
	if (!lpBodyElm) {
		lpHtmlDocument->Release();
		return;
	}
	lpHtmlDocument->Release();

	hr = lpBodyElm->QueryInterface(IID_IHTMLBodyElement, (void**)&lpBody);
	HTML_ASSERT(SUCCEEDED(hr) && lpBody);
	if (!lpBody) {
		lpBodyElm->Release();
		return;
	}
	lpBodyElm->Release();

	hr = lpBody->createTextRange(&lpTxtRange);
	HTML_ASSERT(SUCCEEDED(hr) && lpTxtRange);
	if (!lpTxtRange) {
		lpBody->Release();
		return;
	}
	lpBody->Release();

	CComBSTR html;
	CComBSTR newhtml;
	CComBSTR search(searchText.GetLength() + 1, (LPCTSTR)searchText);

	long t;
	VARIANT_BOOL bFound;
	long count = 0;

	CComBSTR Character(L"Character");
	CComBSTR Textedit(L"Textedit");

	htmlPrfix.Append("<span id='");
	htmlPrfix.Append((LPCTSTR)searchID);
	htmlPrfix.Append("' style='");
	htmlPrfix.Append((LPCTSTR)matchStyle);
	htmlPrfix.Append("'>");

	bool firstRange = false;
	while ((lpTxtRange->findText(search, count, lFlags, (VARIANT_BOOL*)&bFound) == S_OK) && (VARIANT_TRUE == bFound))
	{
		//IHTMLTxtRange *duplicateRange;
		//lpTxtRange->duplicate(&duplicateRange);

		IHTMLElement *parentText = 0;
		hr = lpTxtRange->parentElement(&parentText);
		if (SUCCEEDED(hr) && parentText)
		{
			bstrTag.Empty();
			bstrHTML.Empty();
			bstrTEXT.Empty();

			parentText->get_tagName(&bstrTag);
			parentText->get_innerHTML(&bstrHTML);
			parentText->get_innerText(&bstrTEXT);

			int deb = 1;
			// Ignore Tags: TITLE, what else ?
			bstrTag.ToLower();
			if (bstrTag == L"title") {
				lpTxtRange->moveStart(Character, 1, &t);
				lpTxtRange->moveEnd(Textedit, 1, &t);

				parentText->Release();
				continue;
			}
			parentText->Release();
		}
		if (parentText)
			parentText->Release();

		if (firstRange == false) {
			firstRange = true;
			if (lpTxtRange->select() == S_OK)
				lpTxtRange->scrollIntoView(VARIANT_TRUE);
		}

		newhtml.Empty();
		lpTxtRange->get_htmlText(&html);
		newhtml.Append((LPCTSTR)htmlPrfix);
		if (searchText == " ")
			newhtml.Append("&nbsp;"); // doesn't work very well, but prevents (some) strange presentation
		else
			newhtml.AppendBSTR(html);
		newhtml.Append("</span>");

		lpTxtRange->pasteHTML(newhtml);

		lpTxtRange->moveStart(Character, searchText.GetLength(), &t);
		lpTxtRange->moveEnd(Textedit, 1, &t);
	}
	if (lpTxtRange)
		lpTxtRange->Release();
}

void HtmlUtils::ClearSearchResultsInIHTMLDocument(CBrowser &browser, CString &searchID)
{
	// Based on "Adding a custom search feature to CHtmlViews" on the codeproject by  Marc Richarme, 22 Nov 2000
	// without explicit license
	//  https://www.codeproject.com/Articles/832/Adding-a-custom-search-feature-to-CHtmlViews
	// mboxview development team did resolve some code issues, enhanced and optimized

	CComBSTR testid(searchID.GetLength() + 1, searchID);
	CComBSTR testtag(5, "SPAN");

	HRESULT hr;
	IHTMLDocument2 *lpHtmlDocument = NULL;
	LPDISPATCH lpDispatch = NULL;
	lpDispatch = browser.m_ie.GetDocument();
	HTML_ASSERT(lpDispatch);
	if (!lpDispatch)
		return;

	hr = lpDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&lpHtmlDocument);
	HTML_ASSERT(SUCCEEDED(hr) && lpHtmlDocument);
	if (!lpHtmlDocument) {
		lpDispatch->Release();
		return;
	}
	lpDispatch->Release();

	IHTMLElementCollection *lpAllElements = 0;
	hr = lpHtmlDocument->get_all(&lpAllElements);
	HTML_ASSERT(SUCCEEDED(hr) && lpAllElements);
	if (!lpAllElements) {
		lpHtmlDocument->Release();
		return;
	}
	lpHtmlDocument->Release();

	CComBSTR id;
	CComBSTR tag;
	CComBSTR innerText;

	IUnknown *lpUnk;
	IEnumVARIANT *lpNewEnum;
	if (SUCCEEDED(lpAllElements->get__newEnum(&lpUnk)) && (lpUnk != NULL))
	{
		hr = lpUnk->QueryInterface(IID_IEnumVARIANT, (void**)&lpNewEnum);
		HTML_ASSERT(SUCCEEDED(hr) && lpNewEnum);
		if (!lpNewEnum) {
			lpAllElements->Release();
			return;
		}
		VARIANT varElement;
		IHTMLElement *lpElement;

		VariantInit(&varElement);
		while (lpNewEnum->Next(1, &varElement, NULL) == S_OK)
		{
			HTML_ASSERT(varElement.vt == VT_DISPATCH);
			hr = varElement.pdispVal->QueryInterface(IID_IHTMLElement, (void**)&lpElement);
			HTML_ASSERT(SUCCEEDED(hr) && lpElement);

			if (lpElement)
			{
				id.Empty();
				tag.Empty();
				lpElement->get_id(&id);
				lpElement->get_tagName(&tag);
				if ((id == testid) && (tag == testtag))
				{
					innerText.Empty();
					lpElement->get_innerHTML(&innerText);
					lpElement->put_outerHTML(innerText);
				}
			}
			hr = VariantClear(&varElement);
		}
	}
	lpAllElements->Release();
}